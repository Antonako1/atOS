#include <DRIVERS/ATA_PIO/ATA_PIO.h>
#include <STD/ASM.h>
#include <STD/TYPEDEF.h>

static U32 PIO_IDENTIFIER ATTRIB_DATA = 0;
// Using standard 0x100000 timeout for consistency and safety
#define POLLING_TIME 0x100000 

// ATA drive identify
int ata_read_identify_to_buffer(U16 base_port, U8 drive, U16 *ident_out) {
    _outb(base_port + ATA_DRIVE_HEAD, drive);
    ata_io_wait(base_port);
    _outb(base_port + ATA_COMM_REG, ATA_CMD_IDENTIFY);
    ata_io_wait(base_port);

    U8 status = _inb(base_port + ATA_COMM_REG);
    if (status == 0) return 0;

    U32 timeout = 1000000;
    while ((_inb(base_port + ATA_COMM_REG) & STAT_BSY) && timeout--) {}
    status = _inb(base_port + ATA_COMM_REG);
    if (!(status & STAT_ERR)) {
        for (int i = 0; i < 256; i++) 
            ident_out[i] = _inw(base_port + ATA_DATA);
        if (ident_out[0] & 0x8000) return FALSE; // It's ATAPI -> skip
        return 1; // ata drive
    }

    return 0;
}

BOOLEAN ATA_PIO_DRIVE_EXISTS(U16 base_port, U8 drive) {
    U16 ident[256];
    return ata_read_identify_to_buffer(base_port, drive, ident) != 0;
}

// Identify first connected drive
U32 ATA_PIO_IDENTIFY(void) {
    if (ATA_PIO_DRIVE_EXISTS(ATA_PRIMARY_BASE, ATA_MASTER))   return PIO_IDENTIFIER = ATA_PRIMARY_MASTER;
    if (ATA_PIO_DRIVE_EXISTS(ATA_PRIMARY_BASE, ATA_SLAVE))    return PIO_IDENTIFIER = ATA_PRIMARY_SLAVE;
    
    if (ATA_PIO_DRIVE_EXISTS(ATA_SECONDARY_BASE, ATA_MASTER)) return PIO_IDENTIFIER = ATA_SECONDARY_MASTER;
    if (ATA_PIO_DRIVE_EXISTS(ATA_SECONDARY_BASE, ATA_SLAVE))  return PIO_IDENTIFIER = ATA_SECONDARY_SLAVE;
    
    return PIO_IDENTIFIER = ATA_FAILED;
}

U32 ATA_PIO_GET_IDENTIFIER(VOID) {
    if(!PIO_IDENTIFIER) return ATA_PIO_IDENTIFY();
    return PIO_IDENTIFIER;
}

BOOLEAN ATA_PIO_INIT() {
    // Identify first connected drive
    if(ATA_PIO_IDENTIFY() == ATA_FAILED) return FALSE;

    // Optional: Reset drives or clear status registers
    _outb(ATA_PRIMARY_BASE + ATA_COMM_REG, 0); 
    _outb(ATA_SECONDARY_BASE + ATA_COMM_REG, 0);

    return TRUE;
}

// Helper to wait for the drive to be ready (BSY clear) and request data (DRQ set).
static BOOLEAN ata_wait_ready_and_data_request(U16 base) {
    // Poll the Status Register until BSY clears or ERR/DRQ sets (400ns delay is applied by ata_io_wait)
    U32 timeout = POLLING_TIME;
    while (timeout--) {
        U8 status = _inb(base + ATA_COMM_REG);

        if (status & STAT_ERR) {
            // Read Error register to clear the pending interrupt status and get diagnosis.
            volatile U8 error_code = _inb(base + ATA_ERR);
            (void)error_code; // Suppress unused variable warning if logging is not implemented
            return FALSE;
        }

        // BSY clear and DRQ set means the drive is ready for data transfer
        if (!(status & STAT_BSY) && (status & STAT_DRQ)) return TRUE;

        ata_io_wait(base);
    }
    return FALSE; // Timeout
}

// Generic PIO sector read/write (LBA28) — true multi-sector transfer
BOOLEAN ATA_PIO_XFER(U8 device, U32 lba, U8 sector_count, VOIDPTR buffer, BOOLEAN write) {
    U16 base;
    U8 drive;
    switch (device)
    {
    case ATA_PRIMARY_MASTER:
        base = ATA_PRIMARY_BASE;
        drive = ATA_MASTER;
        break;
    case ATA_SECONDARY_MASTER:
        base = ATA_SECONDARY_BASE;
        drive = ATA_MASTER;
        break;
    case ATA_PRIMARY_SLAVE:
        base = ATA_PRIMARY_BASE;
        drive = ATA_SLAVE;
        break;
    case ATA_SECONDARY_SLAVE:
        base = ATA_SECONDARY_BASE;
        drive = ATA_SLAVE;
        break;
    default:
        return FALSE;        
    }

    if (sector_count == 0) return FALSE;

    U16* buf = (U16*)buffer;

    // 1. Select Drive and upper LBA bits; 400 ns delay
    _outb(base + ATA_DRIVE_HEAD, drive | 0xE0 | ((lba >> 24) & 0x0F));
    ata_io_wait(base); ata_io_wait(base); ata_io_wait(base); ata_io_wait(base);

    // 2. Program sector count and 24-bit LBA address
    _outb(base + ATA_SECCOUNT, sector_count);
    _outb(base + ATA_LBA_LO,   (U8)(lba & 0xFF));
    _outb(base + ATA_LBA_MID,  (U8)((lba >> 8) & 0xFF));
    _outb(base + ATA_LBA_HI,   (U8)((lba >> 16) & 0xFF));

    // 3. Issue single READ or WRITE command for all sectors
    _outb(base + ATA_COMM_REG, write ? ATA_PIO_CMD_WRITE28 : ATA_PIO_CMD_READ28);
    ata_io_wait(base);

    // 4. Transfer one sector at a time, waiting for DRQ before each
    for (U8 s = 0; s < sector_count; s++) {
        // Poll for DRQ (drive ready to transfer next sector)
        if (!ata_wait_ready_and_data_request(base)) return FALSE;

        U16 *sector_buf = buf + (s * ATA_PIO_SECTOR_WORDS);

        if (write) {
            for (int i = 0; i < 256; i++) _outw(base + ATA_DATA, sector_buf[i]);
            /* Wait for BSY to clear before writing the next sector (or finishing).
             * Do NOT issue any ATA command here — the multi-sector transfer is
             * still in progress; the drive will assert DRQ again for the next sector. */
            U32 poll_timeout = POLLING_TIME;
            while ((_inb(base + ATA_COMM_REG) & STAT_BSY) && poll_timeout--) {
                ata_io_wait(base);
            }
            if (poll_timeout == 0) return FALSE;
        } else {
            for (int i = 0; i < 256; i++) sector_buf[i] = _inw(base + ATA_DATA);
        }
    }

    /* After all write sectors are done, issue a single CACHE FLUSH so the data
     * is committed to persistent storage before we return. */
    if (write) {
        _outb(base + ATA_COMM_REG, 0xE7); /* WRITE_CACHE_FLUSH */
        U32 flush_timeout = POLLING_TIME;
        while ((_inb(base + ATA_COMM_REG) & STAT_BSY) && flush_timeout--) {
            ata_io_wait(base);
        }
        if (flush_timeout == 0) return FALSE;
    }

    return ATA_SUCCESS;
}

BOOLEAN ATA_PIO_READ_SECTORS_EXT(U8 device_id, U32 lba, U8 sector_count, VOIDPTR out_buffer) {
    return ATA_PIO_XFER(device_id, lba, sector_count, out_buffer, FALSE);
}
BOOLEAN ATA_PIO_WRITE_SECTORS_EXT(U8 device_id, U32 lba, U8 sector_count, VOIDPTR in_buffer) {
    return ATA_PIO_XFER(device_id, lba, sector_count, in_buffer, TRUE);
}

BOOLEAN ATA_PIO_READ_SECTORS(U32 lba, U8 sector_count, VOIDPTR out_buffer) {
    return ATA_PIO_XFER((U8)ATA_PIO_GET_IDENTIFIER(), lba, sector_count, out_buffer, FALSE);
}
BOOLEAN ATA_PIO_WRITE_SECTORS(U32 lba, U8 sector_count, VOIDPTR in_buffer) {
    return ATA_PIO_XFER((U8)ATA_PIO_GET_IDENTIFIER(), lba, sector_count, in_buffer, TRUE);
}

U32 ATA_CALC_SEC_COUNT(U32 bytes) {
    return (bytes + (ATA_PIO_SECTOR_SIZE - 1)) / ATA_PIO_SECTOR_SIZE;
}