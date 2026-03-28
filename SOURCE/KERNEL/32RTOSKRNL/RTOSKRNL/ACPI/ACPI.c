#include <ACPI/ACPI.h>
#include <STD/ASM.h>

#include <ACPI/ACPI.h>
#include <STD/STRING.h>
#include <STD/ASM.h>
#include <STD/MEM.h>

/* Find the RSDP in low memory (0xE0000–0xFFFFF) */
static ACPI_RSDP* find_rsdp(void) {
    U8* ptr = (U8*)0x000E0000;
    U8* end = (U8*)0x000FFFFF;
    while(ptr < end) {
        if(MEMCMP(ptr, ACPI_SIG_RSDP, 8) == 0) {
            return (ACPI_RSDP*)ptr;
        }
        ptr += 16; // RSDP is 16-byte aligned
    }
    return NULL;
}

/* Retrieve the FADT from ACPI tables */
ACPI_FADT* ACPI_GET_FADT(void) {
    ACPI_RSDP* rsdp = find_rsdp();
    if(!rsdp) return NULL;

    U32 entryCount;
    ACPI_SDT_HEADER* table;

    if(rsdp->Revision >= 2 && rsdp->XsdtAddressLow) {
        // 64-bit XSDT (ACPI 2.0+)
        ACPI_XSDT* xsdt = (ACPI_XSDT*)(U32)rsdp->XsdtAddressLow;
        entryCount = (xsdt->Header.Length - sizeof(ACPI_SDT_HEADER)) / 8;
        for(U32 i = 0; i < entryCount; i++) {
            table = (ACPI_SDT_HEADER*)(U32)xsdt->Entries[i].Low;
            if(MEMCMP(table->Signature, ACPI_SIG_FADT, 4) == 0)
                return (ACPI_FADT*)table;
        }
    } else {
        // 32-bit RSDT
        ACPI_RSDT* rsdt = (ACPI_RSDT*)(U32)rsdp->RsdtAddress;
        entryCount = (rsdt->Header.Length - sizeof(ACPI_SDT_HEADER)) / 4;
        for(U32 i = 0; i < entryCount; i++) {
            table = (ACPI_SDT_HEADER*)(U32)rsdt->Entries[i];
            if(MEMCMP(table->Signature, ACPI_SIG_FADT, 4) == 0)
                return (ACPI_FADT*)table;
        }
    }
    return NULL;
}

/* ACPI shutdown */
void ACPI_SHUTDOWN_SYSTEM(void) {
    ACPI_FADT* fadt = ACPI_GET_FADT();
    if(!fadt) HLT;

    U16 pm1a = (U16)fadt->Pm1aCntBlk;

    U16 shutdown = (5 << 10) | (1 << 13);

    asm volatile("cli");
    _outw(pm1a, shutdown);
    HLT;
}