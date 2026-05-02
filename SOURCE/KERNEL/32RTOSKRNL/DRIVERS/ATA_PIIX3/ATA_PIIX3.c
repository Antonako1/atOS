#include <DRIVERS/ATA_PIIX3/ATA_PIIX3.h>
#include <MEMORY/HEAP/KHEAP.h>
#include <RTOSKRNL_INTERNAL.h>
#include <STD/ASM.h>
#include <STD/MEM.h>
#include <STD/STRING.h>
#include <DRIVERS/VESA/VBE.h>
#include <CPU/PIC/PIC.h>

/*
Add handler manually to IRQ handler tree
Get IRQ num from PCI
*/

#define POLLING_TIME 0xFFFFF

typedef struct {
    U32 phys_addr;
    U16 byte_count;
    U16 flags; // bit 15 = end-of-table
} ATTRIB_PACKED PRDT_ENTRY;

static PRDT_ENTRY* PRDT ATTRIB_DATA = NULL;
static U8* DMA_BUFFER ATTRIB_DATA = NULL;
static U32 BM_BASE_PRIMARY ATTRIB_DATA = 0;
static U32 BM_BASE_SECONDARY ATTRIB_DATA = 0;
static U32 IDENTIFIER ATTRIB_DATA = 0;

static BOOL BM_PRIMARY_IS_IO ATTRIB_DATA = FALSE;
static BOOL BM_SECONDARY_IS_IO ATTRIB_DATA = FALSE;

typedef struct {
    U8 MW_DMA_0;
    U8 MW_DMA_1;
    U8 MW_DMA_2;
    U8 UDMA_0;
    U8 UDMA_1;
} MDA_MODES;

static MDA_MODES supported_dma_modes ATTRIB_DATA = { 0, 0, 0, 0, 0 };

static volatile BOOL dma_done ATTRIB_DATA = FALSE;
static VOID* dma_target_buf ATTRIB_DATA = NULL;
static U32 dma_bytes ATTRIB_DATA = 0;
static U8 dma_write ATTRIB_DATA = FALSE;


static BOOL ATA_PIIX3_LOCATE_BUS_MASTER(void) {
    U32 pci_device_count = PCI_GET_DEVICE_COUNT();
    BM_BASE_PRIMARY = 0;
    BM_BASE_SECONDARY = 0;

    for (U32 i = 0; i < pci_device_count; i++) {
        PCI_DEVICE_ENTRY* dev = PCI_GET_DEVICE(i);
        if (dev->header.class_code == PCI_CLASS_MASS_STORAGE &&
            dev->header.subclass  == PCI_SUBCLASS_IDE) {

            U32 bm_bar = dev->header.bar4;
            BOOL bm_is_io = bm_bar & 0x01;
            U32 bm_base = bm_is_io ? (bm_bar & ~0x3) : (bm_bar & ~0xF);

            BM_BASE_PRIMARY = bm_base + 0x00;
            BM_BASE_SECONDARY = bm_base + 0x08;
            BM_PRIMARY_IS_IO = bm_is_io;
            BM_SECONDARY_IS_IO = bm_is_io;

            if(!PCI_ENABLE_BUS_MASTERING(dev->bus, dev->slot, dev->func)) return FALSE;
        }
    }
    return TRUE;
}


BOOLEAN ATA_DRIVE_EXISTS(U16 base_port, U8 drive) {
    U16 ident[256];
    return ata_read_identify_to_buffer(base_port, drive, ident) != 0;
}

// Identify first connected drive
U32 ATA_IDENTIFY(void) {
    if (BM_BASE_PRIMARY) {
        if (ATA_DRIVE_EXISTS(ATA_PRIMARY_BASE, ATA_MASTER))   return IDENTIFIER = ATA_PRIMARY_MASTER;
        if (ATA_DRIVE_EXISTS(ATA_PRIMARY_BASE, ATA_SLAVE))    return IDENTIFIER = ATA_PRIMARY_SLAVE;
    }

    if (BM_BASE_SECONDARY) {
        if (ATA_DRIVE_EXISTS(ATA_SECONDARY_BASE, ATA_MASTER)) return IDENTIFIER = ATA_SECONDARY_MASTER;
        if (ATA_DRIVE_EXISTS(ATA_SECONDARY_BASE, ATA_SLAVE))  return IDENTIFIER = ATA_SECONDARY_SLAVE;
    }

    return IDENTIFIER = ATA_FAILED;
}

U32 ATA_GET_IDENTIFIER(VOID) {
    if(!IDENTIFIER) return ATA_IDENTIFY();
    return IDENTIFIER;
}

// Initialize DMA (polling only)
BOOLEAN ATA_PIIX3_INIT(VOID) {
    if (PRDT && DMA_BUFFER) return TRUE;

    // Locate the bus master base addresses
    panic_if(!ATA_PIIX3_LOCATE_BUS_MASTER(), PANIC_TEXT("Failed to initialize bus master"), PANIC_INITIALIZATION_FAILED);
    if (!BM_BASE_PRIMARY && !BM_BASE_SECONDARY) return FALSE;

    // Identify first available drive
    U32 identification = ATA_GET_IDENTIFIER();
    if (identification == ATA_FAILED) return FALSE;
    // panic_debug("A",0);
    PRDT = (PRDT_ENTRY*)KMALLOC_ALIGN(sizeof(PRDT_ENTRY), ATA_PIIX3_PRDT_ALIGN);
    panic_if(!PRDT, PANIC_TEXT("Failed to allocate PRDT"), PANIC_OUT_OF_MEMORY);
    MEMZERO(PRDT, sizeof(PRDT_ENTRY));

    DMA_BUFFER = KMALLOC_ALIGN(BUS_MASTER_LIMIT_PER_ENTRY, ATA_PIIX3_BUFFER_ALIGN);
    panic_if(!DMA_BUFFER, PANIC_TEXT("Failed to allocate DMA buffer"), PANIC_OUT_OF_MEMORY);
    MEMZERO(DMA_BUFFER, BUS_MASTER_LIMIT_PER_ENTRY);

    // Clear BM command + status registers
    if (BM_BASE_PRIMARY) {
        bm_write8(BM_BASE_PRIMARY, BM_PRIMARY_IS_IO, BM_COMMAND_OFFSET, 0);
        bm_write8(BM_BASE_PRIMARY, BM_PRIMARY_IS_IO, BM_STATUS_OFFSET, BM_STATUS_ACK);
    }
    if (BM_BASE_SECONDARY) {
        bm_write8(BM_BASE_SECONDARY, BM_SECONDARY_IS_IO, BM_COMMAND_OFFSET, 0);
        bm_write8(BM_BASE_SECONDARY, BM_SECONDARY_IS_IO, BM_STATUS_OFFSET, BM_STATUS_ACK);
    }

    // Detect supported DMA modes on connected drives
    U16 ident[256];
    if (identification == ATA_PRIMARY_MASTER || identification == ATA_PRIMARY_SLAVE) {
        U8 drive_sel = (identification == ATA_PRIMARY_SLAVE) ? ATA_SLAVE : ATA_MASTER;
        if (ata_read_identify_to_buffer(ATA_PRIMARY_BASE, drive_sel, ident)) {
            U16 w63 = ident[63];
            U16 w88 = ident[88];
            supported_dma_modes.MW_DMA_0 = (w63 & (1 << 0)) != 0;
            supported_dma_modes.MW_DMA_1 = (w63 & (1 << 1)) != 0;
            supported_dma_modes.MW_DMA_2 = (w63 & (1 << 2)) != 0;
            supported_dma_modes.UDMA_0    = (w88 & (1 << 0)) != 0;
            supported_dma_modes.UDMA_1    = (w88 & (1 << 1)) != 0;
        }
    }
    if (identification == ATA_SECONDARY_MASTER || identification == ATA_SECONDARY_SLAVE) {
        U8 drive_sel = (identification == ATA_SECONDARY_SLAVE) ? ATA_SLAVE : ATA_MASTER;
        if (ata_read_identify_to_buffer(ATA_SECONDARY_BASE, drive_sel, ident)) {
            U16 w63 = ident[63];
            U16 w88 = ident[88];
            supported_dma_modes.MW_DMA_0 |= (w63 & (1 << 0)) != 0;
            supported_dma_modes.MW_DMA_1 |= (w63 & (1 << 1)) != 0;
            supported_dma_modes.MW_DMA_2 |= (w63 & (1 << 2)) != 0;
            supported_dma_modes.UDMA_0    |= (w88 & (1 << 0)) != 0;
            supported_dma_modes.UDMA_1    |= (w88 & (1 << 1)) != 0;
        }
    }
    // todo: get irq from pci
    ISR_REGISTER_HANDLER(PIC_REMAP_OFFSET + 14, ATA_IRQ_HANDLER);
    ISR_REGISTER_HANDLER(PIC_REMAP_OFFSET + 15, ATA_IRQ_HANDLER);
    PIC_Unmask(14);
    PIC_Unmask(15);
    return TRUE;
}


// Generic DMA sector read/write (BM-status polling — works with or without IRQs enabled)
BOOLEAN ATA_PIIX3_XFER(U8 device, U32 lba, U8 sectors, VOIDPTR buf, BOOLEAN write) {
    if (!PRDT || !DMA_BUFFER) return FALSE;

    U32 total_bytes = sectors * ATA_PIIX3_SECTOR_SIZE;
    if (total_bytes > 0x10000) return FALSE;

    BOOL is_secondary = (device == ATA_SECONDARY_MASTER || device == ATA_SECONDARY_SLAVE);
    BOOL is_slave     = (device == ATA_PRIMARY_SLAVE    || device == ATA_SECONDARY_SLAVE);
    U16 base    = is_secondary ? ATA_SECONDARY_BASE    : ATA_PRIMARY_BASE;
    U32 bm_base = is_secondary ? BM_BASE_SECONDARY     : BM_BASE_PRIMARY;
    BOOL is_io  = is_secondary ? BM_SECONDARY_IS_IO    : BM_PRIMARY_IS_IO;

    // Setup PRDT + DMA buffer
    MEMZERO(DMA_BUFFER, total_bytes);
    PRDT[0].phys_addr  = (U32)DMA_BUFFER;
    PRDT[0].byte_count = (U16)(total_bytes >= 0x10000 ? 0 : total_bytes); // 0 == 64 KB per PIIX3 spec
    PRDT[0].flags      = END_OF_TABLE_FLAG;
    bm_write32(bm_base, is_io, BM_PRDT_ADDR_OFFSET, (U32)PRDT);

    if (write) MEMCPY(DMA_BUFFER, buf, total_bytes);

    /* Keep globals in sync so the IRQ handler, if it fires, does the right thing */
    dma_target_buf = buf;
    dma_bytes      = total_bytes;
    dma_write      = write;
    dma_done       = FALSE;

    /* Clear stale IRQ and error bits (write-1-to-clear), then set direction */
    bm_write8(bm_base, is_io, BM_STATUS_OFFSET,  BM_STATUS_ERROR | BM_STATUS_IRQ);
    bm_write8(bm_base, is_io, BM_COMMAND_OFFSET, write ? BM_CMD_WRITE : BM_CMD_READ);

    /* Program ATA registers (0xE0: fixed bits, bit6=LBA, bit4=slave select) */
    _outb(base + ATA_DRIVE_HEAD, 0xE0 | (is_slave ? 0x10 : 0x00) | ((lba >> 24) & 0x0F));
    ata_io_wait(base);
    _outb(base + ATA_SECCOUNT, sectors);
    _outb(base + ATA_LBA_LO,  (U8)(lba & 0xFF));
    _outb(base + ATA_LBA_MID, (U8)((lba >> 8)  & 0xFF));
    _outb(base + ATA_LBA_HI,  (U8)((lba >> 16) & 0xFF));
    _outb(base + ATA_COMM_REG, write ? ATA_MDA_CMD_WRITE28 : ATA_MDA_CMD_READ28);

    /* Start DMA engine */
    bm_write8(bm_base, is_io, BM_COMMAND_OFFSET,
              (write ? BM_CMD_WRITE : BM_CMD_READ) | BM_CMD_START_STOP);

    /* -----------------------------------------------------------------------
     * Wait for completion by polling the Bus-Master Status register directly.
     *
     * Why polling instead of waiting on dma_done (set by IRQ handler):
     *   The BM_STATUS_IRQ flag (bit 2) is set by HARDWARE the moment the IDE
     *   drive asserts its interrupt line.  This happens regardless of whether
     *   the CPU's IF flag is set and regardless of whether the PIC actually
     *   delivers the interrupt.  Polling this bit is therefore safe in every
     *   calling context:
     *     - early boot (interrupts enabled, works as before)
     *     - inside a syscall handler entered via an interrupt gate (IF=0, old
     *       IRQ-driven wait would spin forever; polling still works)
     *     - inside any other ISR
     *
     * If the IRQ handler fires first (IF is set) it will set dma_done and we
     * break out on the dma_done check before we even read BM_STATUS; either
     * path reaches the same cleanup code below.
     * --------------------------------------------------------------------- */
    U32 timeout = POLLING_TIME * 10;
    BOOL completed = FALSE;
    while (timeout > 0) {
        /* Fast path: IRQ handler already ran (interrupts were enabled) */
        if (dma_done) { completed = TRUE; break; }

        /* Slow path: poll BM_STATUS for the hardware-set IRQ flag           */
        U8 bm_st = bm_read8(bm_base, is_io, BM_STATUS_OFFSET);
        if (bm_st & BM_STATUS_IRQ) {
            /* Transfer complete — mirror what the IRQ handler would do */
            completed = TRUE;
            break;
        }
        if (bm_st & BM_STATUS_ERROR) {
            /* DMA error flagged by hardware */
            break;
        }
        cpu_relax();
        timeout--;
    }

    /* Stop the DMA engine unconditionally */
    bm_write8(bm_base, is_io, BM_COMMAND_OFFSET, BM_STATUS_STOP);

    /* Re-read status, ack the IRQ flag and error bit (write-1-to-clear) */
    U8 final_status = bm_read8(bm_base, is_io, BM_STATUS_OFFSET);
    bm_write8(bm_base, is_io, BM_STATUS_OFFSET, final_status | BM_STATUS_IRQ | BM_STATUS_ERROR);

    /* Clear the ATA interrupt at the drive level */
    _inb(base + ATA_COMM_REG);

    if (!completed || (final_status & BM_STATUS_ERROR))
        return FALSE;

    /* Copy result to caller's buffer if this was a read and the IRQ handler
     * hasn't already done it (dma_done means the handler copied already). */
    if (!write && !dma_done) {
        MEMCPY(buf, DMA_BUFFER, total_bytes);
    }

    /* Signal to the IRQ handler (if it fires late) that we handled it */
    dma_done = TRUE;

    return TRUE;
}

BOOLEAN ATA_PIIX3_READ_SECTORS_EXT(U8 device, U32 lba, U8 sectors, VOIDPTR out) {
    return ATA_PIIX3_XFER(device, lba, sectors, out, FALSE);
}

BOOLEAN ATA_PIIX3_WRITE_SECTORS_EXT(U8 device, U32 lba, U8 sectors, VOIDPTR in) {
    return ATA_PIIX3_XFER(device, lba, sectors, in, TRUE);
}

BOOLEAN ATA_PIIX3_READ_SECTORS(U32 lba, U8 sectors, VOIDPTR out) {
    U8 device = ATA_GET_IDENTIFIER();
    return ATA_PIIX3_XFER(device, lba, sectors, out, FALSE);
}

BOOLEAN ATA_PIIX3_WRITE_SECTORS(U32 lba, U8 sectors, VOIDPTR in) {
    U8 device = ATA_GET_IDENTIFIER();
    return ATA_PIIX3_XFER(device, lba, sectors, in, TRUE);
}


void ATA_IRQ_HANDLER(U32 vector, U32 errcode) {
    (void)errcode;

    // Determine primary/secondary channel
    U32 bm_base = (vector == PIC_REMAP_OFFSET + 14) ? BM_BASE_PRIMARY : BM_BASE_SECONDARY;
    BOOL is_io = (vector == PIC_REMAP_OFFSET + 14) ? BM_PRIMARY_IS_IO : BM_SECONDARY_IS_IO;
    U16 base_port = (vector == PIC_REMAP_OFFSET + 14) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;

    // Read BM status
    U8 status = bm_read8(bm_base, is_io, BM_STATUS_OFFSET);

    // Stop DMA engine
    bm_write8(bm_base, is_io, BM_COMMAND_OFFSET, BM_STATUS_STOP);

    // Clear the interrupt and error bits in BM status (Write 1 to clear)
    bm_write8(bm_base, is_io, BM_STATUS_OFFSET, status | BM_STATUS_IRQ | BM_STATUS_ERROR);

    // Read the regular ATA status to clear the IRQ safely at the drive level
    _inb(base_port + ATA_COMM_REG);

    // If the polling path in ATA_PIIX3_XFER already completed the transfer
    // (dma_done == TRUE), we only needed to acknowledge the IRQ above — skip
    // the buffer copy and the error check to avoid double-copying or a false panic.
    if (dma_done) {
        // Ack IRQ (fall-through to EOI below)
        goto ata_irq_ack;
    }

    // Check for errors
    if (status & BM_STATUS_ERROR) {
        panic("ATA DMA error", status);
    }

    // Copy DMA buffer if reading
    if (!dma_write && dma_target_buf) {
        MEMCPY(dma_target_buf, DMA_BUFFER, dma_bytes);
    }

    // Signal completion
    dma_done = TRUE;

    ata_irq_ack:;

    // Ack IRQ
    pic_send_eoi(vector - PIC_REMAP_OFFSET);
}
