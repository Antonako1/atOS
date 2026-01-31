#include "./ATAPI.h"
#include "../VESA/VBE.h"
#include "../../../../STD/ASM.h"

#ifndef KERNEL_ENTRY
static U32 atapi_drive_info __attribute__((section(".data"))) = 0;
#endif // KERNEL_ENTRY

// Check if ATAPI device exists on the given channel and drive (master/slave)
BOOLEAN atapi_cdrom_exists(U16 base_port, U8 drive) {
    U8 status;

    // Select drive
    _outb(base_port + ATA_DRIVE_HEAD, drive);
    ata_io_wait(base_port);

    // Send IDENTIFY PACKET (0xA1)
    _outb(base_port + ATA_COMM_REG, ATAPI_CMD_IDENTIFY);
    ata_io_wait(base_port);

    status = _inb(base_port + ATA_COMM_REG);
    if (status == 0) return FALSE;  // no device at all

    // Wait for BSY to clear
    while (status & STAT_BSY) status = _inb(base_port + ATA_COMM_REG);

    if (status & STAT_ERR) return FALSE; // not ATAPI
    if (!(status & STAT_DRQ)) return FALSE; // not ready to transfer data

    // Read 256 words of IDENTIFY data
    U16 ident[256];
    for (int i = 0; i < 256; i++) {
        ident[i] = _inw(base_port + ATA_DATA);
    }

    // Word 0: device type info
    if ((ident[0] & 0xC000) == 0x8000) {
        return TRUE; // ATAPI device (CD/DVD)
    }

    return FALSE; // some other device
}

U32 ATAPI_CHECK() {
    // Check primary master
    if (atapi_cdrom_exists(ATA_PRIMARY_BASE, ATAPI_MASTER)) return ATAPI_PRIMARY_MASTER;

    // Check primary slave
    if (atapi_cdrom_exists(ATA_PRIMARY_BASE, ATAPI_SLAVE)) return ATAPI_PRIMARY_SLAVE;

    // Check secondary slave
    if (atapi_cdrom_exists(ATA_SECONDARY_BASE, ATAPI_SLAVE)) return ATAPI_SECONDARY_SLAVE;

    // Check secondary master
    if (atapi_cdrom_exists(ATA_SECONDARY_BASE, ATAPI_MASTER)) return ATAPI_SECONDARY_MASTER;

    return ATA_FAILED; // No ATAPI CD-ROM found
}

#ifndef KERNEL_ENTRY
U32 INITIALIZE_ATAPI() {
    if(!atapi_drive_info) 
        atapi_drive_info = ATAPI_CHECK();
    return atapi_drive_info;
}
U32 GET_ATAPI_INFO() {
    return atapi_drive_info;
}
#endif // KERNEL_ENTRY


int read_cdrom(U32 atapiWhere, U32 lba, U32 sectors, U16 *buffer) {
    U16 port = 0;
    BOOLEAN slave = 0;
    switch (atapiWhere) {
        case ATAPI_PRIMARY_MASTER:
            port = ATA_PRIMARY_BASE;
            break;
        case ATAPI_PRIMARY_SLAVE:
            port = ATA_PRIMARY_BASE;
            slave = 1;
            break;
        case ATAPI_SECONDARY_MASTER:
            port = ATA_SECONDARY_BASE;
            break;
        case ATAPI_SECONDARY_SLAVE:
            port = ATA_SECONDARY_BASE;
            slave = 1;
            break;
        default:
            return ATA_FAILED;
    }

    /* Prepare command */
    volatile U8 read_cmd[12] = {
        ATAPI_CMD_READ12, 0,
        (lba >> 24) & 0xFF, (lba >> 16) & 0xFF,
        (lba >> 8) & 0xFF,  (lba >> 0) & 0xFF,
        (sectors >> 24) & 0xFF, (sectors >> 16) & 0xFF,
        (sectors >> 8) & 0xFF,  (sectors >> 0) & 0xFF,
        0, 0
    };

    _outb(port + ATA_DRIVE_HEAD, 0xA0 | (slave << 4));
    ata_io_wait(port);
    _outb(port + ATA_ERR, 0x00);
    _outb(port + ATA_LBA_MID, ATAPI_SECTOR_SIZE & 0xFF);
    _outb(port + ATA_LBA_HI, ATAPI_SECTOR_SIZE >> 8);
    _outb(port + ATA_COMM_REG, 0xA0);
    ata_io_wait(port);

    /* Wait for readiness */
    while (1) {
        U8 status = _inb(port + ATA_COMM_REG);
        if (status & STAT_ERR)
            return ATA_FAILED;
        if (!(status & STAT_BSY) && (status & STAT_DRQ))
            break;
        ata_io_wait(port);
    }

    /* Send packet */
    _outsw(port + ATA_DATA, (U16 *)read_cmd, 6);

    /* Read sectors */
    for (U32 i = 0; i < sectors; i++) {
        /* Wait for DRQ */
        while (1) {
            U8 status = _inb(port + ATA_COMM_REG);
            if (status & STAT_ERR)
                return ATA_FAILED;
            if (!(status & STAT_BSY) && (status & STAT_DRQ))
                break;
        }

        /* Get transfer size (bytes) */
        int size = (_inb(port + ATA_LBA_HI) << 8) | _inb(port + ATA_LBA_MID);

        if (size <= 0 || (U32)size > ATAPI_SECTOR_SIZE)
            return ATA_FAILED;

        /* Calculate safe buffer offset */
        U32 offset = i * ATAPI_SECTOR_SIZE;
        _insw(port + ATA_DATA, (U16 *)((U8 *)buffer + offset), size / 2);
    }

    return ATA_SUCCESS;
}


U32 READ_CDROM(U32 atapiWhere, U32 lba, U32 sectors, U8 *buf) {
    if (atapiWhere == ATA_FAILED || !buf)
        return 0;

    return read_cdrom(atapiWhere, lba, sectors, (U16 *)buf);
}


U32 ATAPI_CALC_SECTORS(U32 len) {
    return (len + ATAPI_SECTOR_SIZE - 1) / ATAPI_SECTOR_SIZE;
}
#ifndef KERNEL_ENTRY
#endif // KERNEL_ENTRY
