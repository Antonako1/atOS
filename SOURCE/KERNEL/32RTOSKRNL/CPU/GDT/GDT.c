/*+++
    Source/KERNEL/32RTOSKRNL/CPU/GDT/GDT.c - GDT Implementation

    Part of atOS-RT

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    32-bit GDT and IDT implementations for atOS-RT.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/23 - Antonako1
        Initial version. Contains GDT structures and constants.

REMARKS

    Only for kernel usage.

---*/
#include "./GDT.h"

static GDTENTRY gdt[GDT_ENTRY_COUNT] __attribute__((aligned(8)));
static GDTDESCRIPTOR gdtr;

VOID GDT_INIT(VOID) {
    // Null descriptor
    gdt[0] = (GDTENTRY){0,0,0,0,0,0};

    // Kernel code segment
    gdt[1] = (GDTENTRY){0xFFFF, 0, 0, ACC_KCODE, GRAN_32_4K, 0};

    // Kernel data segment
    gdt[2] = (GDTENTRY){0xFFFF, 0, 0, ACC_KDATA, GRAN_32_4K, 0};

    gdtr.size = sizeof(gdt)-1;
    gdtr.offset = (U32)&gdt;

    __asm__ volatile("lgdt %0" : : "m"(gdtr));

    // Reload segment registers
    U16 sel = KDATA_SEL;   // make a proper 16-bit selector
    __asm__ volatile(
        "mov %0, %%ax\n"   // move the 16-bit value into AX
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        :
        : "r"(sel)
        : "ax"
    );



    // Far jump to reload CS
    __asm__ volatile(
        "ljmp %0, $1f\n"
        "1:\n" : : "i"(KCODE_SEL)
    );
}
