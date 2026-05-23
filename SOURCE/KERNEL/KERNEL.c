/*+++
    Source/KERNEL/KERNEL.c - 32-bit Kernel Entry Point

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    32-bit kernel entry point for atOS.

AUTHORS
    Antonako1
    
REVISION HISTORY
    2025/05/26 - Antonako1
        Initial version. Switched from KERNEL.asm to KERNEL.c.
    2025/08/18 - Antonako1
        Displays VESA/VBE, E820, GDT and IDT information.
        Initializes video mode and clears the screen.

REMARKS
    Do NOT touch this file unless you know what you're doing.
    There are some problems with the stack that I haven't figured out yet,
    so be careful when modifying this file.
      Atleast too large of a call stack frame will cause issues.

    max compiled file size for KRNL.BIN is 0x4000 bytes
---*/
#include "./32RTOSKRNL/KERNEL.h"
#define ISO9660_ONLY_DEFINES
#include "./32RTOSKRNL/FS/ISO9660/ISO9660.h"
#include <PIT.h>
#include "./32RTOSKRNL/CPU/STACK/STACK.h"
#include "./32RTOSKRNL/DEBUG/KDEBUG.h"

/* -------- memdisk RAM-backed ISO read -------- */
/*
 * When booted via PXE + syslinux memdisk, there is no physical ATAPI device.
 * memdisk maps the entire ISO image into contiguous physical RAM and hooks
 * INT 13h to serve reads from that region.  In 32-bit protected mode we
 * cannot call INT 13h, but we CAN read directly from the RAM copy once we
 * know its base address.
 *
 * Detection: scan physical memory starting at 1 MB in ISO-sector (2048-byte)
 * steps.  For each candidate ISO base B, the Primary Volume Descriptor lives
 * at B + 16*2048.  Check for the standard "CD001" signature there.
 */
#define ATAPI_MEMDISK   0xFE   /* fake atapi_status for the memdisk path */

static U32 g_iso_base = 0;    /* set by memdisk_find_iso(), 0 if not found */

/* Return the physical base address of the in-RAM ISO image, or 0 if not found. */
static U32 memdisk_find_iso(void) {
    const U32 pvd_off  = 16 * 2048;  /* PVD is at LBA 16 */
    const U32 scan_end = 0x40000000; /* scan up to 1 GB */
    U32 base;
    for (base = 0x100000; base < scan_end; base += 2048) {
        U8 *pvd = (U8 *)(base + pvd_off);
        if (pvd[0] == 1 &&
            pvd[1] == 'C' && pvd[2] == 'D' &&
            pvd[3] == '0' && pvd[4] == '0' && pvd[5] == '1' &&
            pvd[6] == 1) {
            return base;
        }
    }
    return 0;
}

/* Copy ISO sectors from the in-RAM image into buf. */
static U32 memdisk_read(U32 lba, U32 sectors, U8 *buf) {
    U8 *src = (U8 *)(g_iso_base + lba * 2048);
    U32 n = sectors * 2048;
    U32 i;
    for (i = 0; i < n; i++) buf[i] = src[i];
    return ATA_SUCCESS;
}

/*
 * Unified sector read: routes to the memdisk RAM path or to ATAPI port I/O
 * depending on how the kernel was booted.
 */
static U32 read_cdrom_sector(U32 atapi_status, U32 lba, U32 sectors, U8 *buf) {
    if (atapi_status == ATAPI_MEMDISK)
        return memdisk_read(lba, sectors, buf);
    return READ_CDROM(atapi_status, lba, sectors, buf);
}
/* --------------------------------------------- */

typedef struct {
    U32 ExtentLengthLE;
    U32 ExtentLocationLE_LBA;
} RTOSKRNL;

void normalize_path(U8 *path) {
    for (int i = 0; path[i]; i++) {
        if (path[i] == ';') {
            path[i] = '\0';
            break;
        }
    }
}

U32 __strlen(const U8 *str) {
    const U8 *s = str;
    while (*s) {
        s++;
    }
    return s - str;
}

BOOLEAN strcmp(const U8 *str1, const U8 *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (*str1 == *str2);
}

U8 *strncpy(U8 *dest, const U8 *src, U32 n) {
    U8 *d = dest;
    const U8 *s = src;
    while (n && *s) {
        *d++ = *s++;
        n--;
    }
    while (n--) {
        *d++ = 0;
    }
    return dest;
}

#define CDROM_SECTOR_SIZE 2048
U32 CALC_SECTOR(U32 extent_length) {
    return (extent_length + CDROM_SECTOR_SIZE - 1) / CDROM_SECTOR_SIZE;
}

U0 *strchr(const U8 *str, U8 c) {
    while (*str) {
        if (*str == c) {
            return (U8 *)str;
        }
        str++;
    }
    return NULL;
}

U0 *memmove(void* dest, const void* src, U32 n) {
    U8* d = (U8*)dest;
    const U8* s = (const U8*)src;
    if (d < s) {
        for (U32 i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
        for (U32 i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

U0 *memcpy(void* dest, const void* src, U32 n) {
    U8* d = (U8*)dest;
    const U8* s = (const U8*)src;
    for (U32 i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// Searches for a file inside the ISO9660 filesystem's root directory
BOOLEAN SEARCH_FILE(
    U32 lba,
    U32 extentLength, 
    RTOSKRNL *rks, 
    U8 *target, 
    U8 *original_target,
    U0 *buffer, 
    U32 ATAPI_STATUS
) {
    U32 offset = 0;
    U8 record_name[21];

    // Load root directory record
    if(
        read_cdrom_sector(
            ATAPI_STATUS,
            lba,
            1,
            buffer
        ) == ATA_FAILED
    ) {
        return FALSE;
    }

    U32 i = 0;
    while(offset < extentLength) {
        IsoDirectoryRecord *record = (IsoDirectoryRecord *)((U8*)(buffer + offset));
        strncpy(record_name, record->fileIdentifier, record->fileNameLength);
        record_name[record->fileNameLength] = '\0';  // Null-terminate the string
        normalize_path(record_name);
        U8 test[20];
        memcpy(test, record->fileIdentifier, record->fileNameLength);
        test[record->fileNameLength] = '\0';
        if(record->length == 0) break;
        if (!(record->fileFlags & 0b00000010)) {
            // Inside a file
            if (strcmp(record_name, target)) {
                rks->ExtentLengthLE = record->extentLengthLE;
                rks->ExtentLocationLE_LBA = record->extentLocationLE_LBA;
                return TRUE;
            }
        } else {
            // Inside a directory
            if(strcmp(record_name, target)) {
                U8 *slash = strchr(original_target, '/');
                if(!slash) return FALSE;
                memmove(original_target, slash + 1, __strlen(slash));
                memcpy(target, original_target, __strlen(original_target));
                slash = strchr(target, '/');
                if (slash) {
                    *slash = '\0';
                } else {
                    target[__strlen(original_target)] = '\0';
                }
                return SEARCH_FILE(record->extentLocationLE_LBA, record->extentLengthLE, rks, target, original_target, buffer, ATAPI_STATUS);
            }
        }
        offset += record->length;
        i += VBE_CHAR_HEIGHT + 2;
    }

    return TRUE;
}

static inline void a(U8 c) {
    U8 *bios_vram = (U8*)0xB8000;
    bios_vram[0] = c;
    bios_vram[1] = 0x1A;

}

U0 kernel_after_gdt(U0);

__attribute__((noreturn))
void kernel_entry_main(U0) {
    CLI;
    vesa_check();
    vbe_check();
    GDT_INIT(); // Re-setup GDT just in case
    kernel_after_gdt();
    HLT;
}

// init comport 0

U8 to_hex(U8 val) {
    if (val < 10) return '0' + val;
    return 'A' + (val - 10);
}  
    
U0 kernel_after_gdt(U0) {
    IDT_INIT();                       // Setup IDT
    SETUP_ISR_HANDLERS();
    IRQ_INIT();
    STI;        // Enable interrupts
    // KDEBUG_INIT();
    // KDEBUG_PUTC('A');



    U0 *RTOSKRNL_ADDRESS = (U0*)MEM_RTOSKRNL_BASE; // Main kernel address
    // VBE_DRAW_LINE(0, 0, SCREEN_WIDTH, 0, VBE_WHITE); // Top border
    // VBE_UPDATE_VRAM();
    U32 row = 0;
    U32 atapi_status;
    #define rowinc row += VBE_CHAR_HEIGHT + 2

    /* Try memdisk RAM image first (PXE boot), fall back to ATAPI (QEMU/real hw) */
    g_iso_base = memdisk_find_iso();
    if (g_iso_base) {
        atapi_status = ATAPI_MEMDISK;
    } else {
        atapi_status = ATAPI_CHECK();
        if (atapi_status == ATA_FAILED) {
            VBE_DRAW_STRING(0, row,
                // "ATAPI check failed"
                ""
                , VBE_WHITE, VBE_BLACK);
            VBE_UPDATE_VRAM();
            rowinc;
            HLT;
        }
    }
    
    // Read ATOS/32RTOSKR.BIN;1 from disk
    // We will read the binary with ATAPI operations, 
    // not with DISK/ISO9660, to save the binary size of this file
    // Read buffer address will be at MEM_RESERVED_BASE
    PrimaryVolumeDescriptor *pvd = (PrimaryVolumeDescriptor*)0x01344000; //MEM_RESERVED_BASE
    
    // Read buffer will be just after PVD in memory
    U8 *read_buffer = (U8*)(MEM_RESERVED_BASE + 2048);
    // Buffers limited to 21 to save binary size
    U8 filename[21] = "ATOS\0"; 
    U8 original_target[21] = "ATOS/32RTOSKR.BIN\0";
    RTOSKRNL rks;
    rks.ExtentLengthLE = 0;
    rks.ExtentLocationLE_LBA = 0;
    U8 *bios_vram = (U8*)0xB8000;
    
    bios_vram[0] = to_hex(g_iso_base & 0xF);
    bios_vram[1] = 0x1A;
    bios_vram[2] = to_hex((g_iso_base >> 4) & 0xF);
    bios_vram[3] = 0x1A;
    bios_vram[4] = to_hex((g_iso_base >> 8) & 0xF);
    bios_vram[5] = 0x1A;
    bios_vram[6] = to_hex((g_iso_base >> 12) & 0xF);
    bios_vram[7] = 0x1A;
    bios_vram[8] = to_hex((g_iso_base >> 16) & 0xF);
    bios_vram[9] = 0x1A;
    bios_vram[10] = to_hex((g_iso_base >> 20) & 0xF);
    bios_vram[11] = 0x1A;
    bios_vram[12] = to_hex((g_iso_base >> 24) & 0xF);
    bios_vram[13] = 0x1A;
    bios_vram[14] = to_hex((g_iso_base >> 28) & 0xF);
    bios_vram[15] = 0x1A;
    // Read PVD at lba 16
    if(read_cdrom_sector(atapi_status, 16, 1, (U8*)pvd) == ATA_FAILED) {
        VBE_DRAW_STRING(0, row, 
            // "CDROM read failed"
            ""
            , VBE_WHITE, VBE_BLACK);
        VBE_UPDATE_VRAM();
        rowinc;
        HLT;
    }

    // check PVD legitimacy
    if(
        pvd->TypeCode != 1 ||
        pvd->standardIdentifier[0] != 'C' ||
        pvd->standardIdentifier[1] != 'D' ||
        pvd->standardIdentifier[2] != '0' ||
        pvd->standardIdentifier[3] != '0' ||
        pvd->standardIdentifier[4] != '1' ||
        pvd->version != 1
    ) {
        VBE_DRAW_STRING(0, row, 
            // "Invalid PVD"
            ""
            , VBE_WHITE, VBE_BLACK);
        VBE_UPDATE_VRAM();
        rowinc;

        HLT;
    }
    bios_vram[32] = 'F';
    bios_vram[33] = 0x1A;
    if(!SEARCH_FILE(
        pvd->rootDirectoryRecord.extentLocationLE_LBA,
        pvd->rootDirectoryRecord.extentLengthLE,
        &rks,
        filename,
        original_target,
        read_buffer,
        atapi_status
    )
    ) {
        rowinc;
        VBE_DRAW_STRING(0, row, 
            // "File not found"
            ""
            , VBE_WHITE, VBE_BLACK);
        VBE_UPDATE_VRAM();
        bios_vram[32] = 'Z';
        bios_vram[33] = 0x1A;
        rowinc;
        HLT;
    }
        bios_vram[32] = 'G';
        bios_vram[33] = 0x1A;
    U32 sectors = CALC_SECTOR(rks.ExtentLengthLE);
    if(
        read_cdrom_sector(
            atapi_status,
            rks.ExtentLocationLE_LBA,
            sectors,
            (U8*)RTOSKRNL_ADDRESS
        ) == ATA_FAILED
    ) {
        VBE_DRAW_STRING(0, row, 
            // "Failed to read 32RTOSKRNL"
            ""
            , VBE_WHITE, VBE_BLACK);
        VBE_UPDATE_VRAM();
        rowinc;
        bios_vram[32] = 'I';
        bios_vram[33] = 0x1A;
        HLT;
    }
    bios_vram[32] = 'p';
    bios_vram[33] = 0x1A;
    void (*entry)(void) = (void(*)(void))MEM_RTOSKRNL_BASE;
    __asm__ volatile("jmp *%0" :: "r"(entry) : "memory");

    HLT;
}
    
__attribute__((noreturn, section(".text")))
void _start(void) {
    kernel_entry_main();
}
