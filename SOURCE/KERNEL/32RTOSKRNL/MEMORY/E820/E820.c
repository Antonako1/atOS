#include <MEMORY/E820/E820.h>
#include <VESA/VBE.h>
#include <STD/MEM.h>
#include <RTOSKRNL_INTERNAL.h>
#include <STD/STRING.h>
#include <STD/ASM.h>

#define EXPECTED_MAX_SIZE 32
#define MIN_USER_SPACE_SIZE (MEM_USER_SPACE_END_MIN - MEM_USER_SPACE_BASE)



// Do NOT modify ever!
static U32 e820_entry_count __attribute__((section(".data"))) = 0;
static E820_ENTRY e820_entries[EXPECTED_MAX_SIZE] __attribute__((section(".data"))) = {0};
static E820Info g_E820Info __attribute__((section(".data"))) = {0};

// Helper function to check if a 32-bit value fits in 32 bits

E820Info *GET_E820_INFO(void) {
    return &g_E820Info;
}
E820_ENTRY *GET_E820_ENTRIES(void) {
    return e820_entries;
}
E820_ENTRY *GET_E820_ENTRY(U32 index) {
    if (index >= e820_entry_count) return NULL;
    return &e820_entries[index];
}
U32 e820_get_entry_count(void) {
    return e820_entry_count;
}
static inline BOOLEAN add_u32_would_overflow(U32 a, U32 b) {
    return b > (U32)(0xFFFFFFFFu - a);
}
E820_ENTRY *GET_E820_RAW_TABLE(void) {
    return (E820_ENTRY *)E820_TABLE_PHYS;
}

// Reads all usable RAM entries from the E820 table into g_E820Info
BOOLEAN E820_INIT(void) {
    E820_ENTRY *entry_table = (E820_ENTRY *)E820_TABLE_PHYS;
    U32 entries = 0;

    // Copy the E820 entries from the entry table to our local array

    for (U32 i = 0; i < EXPECTED_MAX_SIZE; i++) {
        if (entry_table[i].BaseAddressLow == 0 && entry_table[i].LengthLow == 0
            && entry_table[i].BaseAddressHigh == 0 && entry_table[i].LengthHigh == 0) {
            break;
        }
        // U8 buf[20];
        // ITOA_U(i, buf, 10);
        // VBE_DRAW_STRING(0, i * (VBE_CHAR_HEIGHT + 2), buf, VBE_WHITE, VBE_BLACK);
        // ITOA_U(entry_table[i].BaseAddressLow, buf, 16);
        // VBE_DRAW_STRING(50, i * (VBE_CHAR_HEIGHT + 2), buf, VBE_WHITE, VBE_BLACK);
        // ITOA_U(entry_table[i].LengthLow, buf, 16);
        // VBE_DRAW_STRING(150, i * (VBE_CHAR_HEIGHT + 2), buf, VBE_WHITE, VBE_BLACK);
        // ITOA_U(entry_table[i].Type, buf, 10);
        // VBE_DRAW_STRING(250, i * (VBE_CHAR_HEIGHT + 2), buf, VBE_WHITE, VBE_BLACK);
        if (entries < EXPECTED_MAX_SIZE) {
            e820_entries[entries++] = entry_table[i];
        }
    }
    // VBE_UPDATE_VRAM();
    // HLT;
    e820_entry_count = entries;

    g_E820Info.RawEntryCount = 0;
    for (U32 i = 0; i < e820_entry_count; i++) {
        // E820_ENTRY *entry = &e820_entries[i];
        // VBE_DRAW_STRING(10, 10 + i * 20, "E820 Entry: Base=0x", VBE_RED,    VBE_BLACK);
        // U8 buffer[17];
        // ITOA_U(entry->BaseAddressLow, buffer, 16);
        // buffer[16] = '\0';
        // VBE_DRAW_STRING(200, 10 + i * 20, buffer, VBE_WHITE, VBE_BLACK);
        // VBE_DRAW_STRING(300, 10 + i * 20, " Length=0x", VBE_RED,    VBE_BLACK);
        // ITOA_U(entry->LengthLow, buffer, 16);
        // buffer[16] = '\0';
        // VBE_DRAW_STRING(400, 10 + i * 20, buffer, VBE_WHITE, VBE_BLACK);
        // VBE_DRAW_STRING(500, 10 + i * 20, " Type=", VBE_RED,    VBE_BLACK);
        // ITOA_U(entry->Type, buffer, 10);
        // buffer[16] = '\0';
        // VBE_DRAW_STRING(550, 10 + i * 20, buffer, VBE_WHITE, VBE_BLACK);

        // VBE_UPDATE_VRAM();
        if(e820_entries[i].Type == TYPE_E820_RAM ||
           e820_entries[i].Type == 3 /* ACPI_RECLAIMABLE — safe to use without ACPI */) {
            if (!fits_in_32(e820_entries[i].BaseAddressHigh) ||
                !fits_in_32(e820_entries[i].LengthHigh)) continue;
            if (g_E820Info.RawEntryCount < E820_MAX_ENTRIES) {
                e820_entries[i].Type = TYPE_E820_RAM; // Treat reclaimable as RAM
                g_E820Info.RawEntries[g_E820Info.RawEntryCount++] = e820_entries[i];
            }
        } else {
            continue; // Skip non-RAM entries
        }
    }
    while ((0))
    {
        /* code */
    }
    
    if(g_E820Info.RawEntryCount == 0) {
        return FALSE; // No usable RAM entries found
    }
    return TRUE;
}

BOOLEAN REINIT_E820(void) {
    e820_entry_count = 0;
    MEMZERO(e820_entries, sizeof(e820_entries));
    MEMZERO(&g_E820Info, sizeof(g_E820Info));
    return E820_INIT();
}