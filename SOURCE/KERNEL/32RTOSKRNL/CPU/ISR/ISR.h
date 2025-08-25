/*+++
    Source/KERNEL/32RTOSKRNL/CPU/ISR/ISR.h - Interrupt Service Routine (ISR) Implementation

    Part of atOS-RT

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    32-bit interrupt service routine (ISR) implementation for atOS-RT.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/24 - Antonako1
        Initial version. ISR handling functions added.
REMARKS
    When compiling, include ISR.c
---*/
#ifndef ISR_H
#define ISR_H
#include "../../../../STD/ATOSMINDEF.h"
#include "../../MEMORY/MEMORY.h"
#include "../IDT/IDT.h"

#define IDT_COUNT 256
#define IDT_MEM_BASE MEM_IDT_BASE
#define IDT_MEM_END (MEM_IDT_BASE + IDT_COUNT*8)
#define IDT_MEM_SIZE (IDT_MEM_END - IDT_MEM_BASE)
#if IDT_MEM_SIZE % 8 != 0
#error "IDT memory size is not a multiple of 8"
#endif

// ISR stub macro
#define ISR_STUB_DEF(n) \
    extern void isr_##n(void); \

// -----------------------
// Two stub macros:
//  - ISR_STUB(n)          : for exceptions/interrupts that DO NOT push an error code.
//                          This stub pushes a *dummy* error code (0) so that the stack layout
//                          seen by the C dispatcher is identical for both kinds.
//  - ISR_STUB_ERRCODE(n)  : for exceptions that *do* push an error code (CPU already pushed it).
//
// Both variants:
//  - save segment registers and general-purpose registers (but NOT ESP — don't save/restore ESP).
//  - push the interrupt number and a pointer to the saved regs, then call isr_handler.
//  - restore registers and iret.
// Notes:
//  - Use ISR_STUB_ERRCODE for vectors: 8, 10, 11, 12, 13, 14, 17 (these push an error code).
//  - Use ISR_STUB for other vectors (0..31 excluding the above, and IRQ stubs).
// -----------------------


#define ISR_STUB(n) \
__attribute__((naked)) void isr_##n(void) { \
    __asm__ volatile ( \
        "cli\n\t" \
        "push %%ds\n\t" \
        "push %%es\n\t" \
        "push %%fs\n\t" \
        "push %%gs\n\t" \
        "push %%eax\n\t" \
        "push %%ecx\n\t" \
        "push %%edx\n\t" \
        "push %%ebx\n\t" \
        "push %%ebp\n\t" \
        "push %%esi\n\t" \
        "push %%edi\n\t" \
        "push $0\n\t" /* dummy error code */ \
        "movl %0, %%eax\n\t" \
        "push %%eax\n\t" \
        "leal 8(%%esp), %%eax\n\t" /* pointer to regs struct */ \
        "push %%eax\n\t" \
        "call isr_handler\n\t" \
        "addl $8, %%esp\n\t" \
        "pop %%edi\n\t" \
        "pop %%esi\n\t" \
        "pop %%ebp\n\t" \
        "pop %%ebx\n\t" \
        "pop %%edx\n\t" \
        "pop %%ecx\n\t" \
        "pop %%eax\n\t" \
        "pop %%gs\n\t" \
        "pop %%fs\n\t" \
        "pop %%es\n\t" \
        "pop %%ds\n\t" \
        "iret\n\t" \
        : : "i"(n) \
    ); \
}
#define ISR_STUB_ERRCODE(n) \
__attribute__((naked)) void isr_##n(void) { \
    __asm__ volatile ( \
        "cli\n\t" \
        "push %%ds\n\t" \
        "push %%es\n\t" \
        "push %%fs\n\t" \
        "push %%gs\n\t" \
        "push %%eax\n\t" \
        "push %%ecx\n\t" \
        "push %%edx\n\t" \
        "push %%ebx\n\t" \
        "push %%ebp\n\t" \
        "push %%esi\n\t" \
        "push %%edi\n\t" \
        "movl %0, %%eax\n\t" \
        "push %%eax\n\t" \
        "leal (%%esp), %%eax\n\t" \
        "push %%eax\n\t" \
        "call isr_handler\n\t" \
        "addl $8, %%esp\n\t" \
        "pop %%edi\n\t" \
        "pop %%esi\n\t" \
        "pop %%ebp\n\t" \
        "pop %%ebx\n\t" \
        "pop %%edx\n\t" \
        "pop %%ecx\n\t" \
        "pop %%eax\n\t" \
        "pop %%gs\n\t" \
        "pop %%fs\n\t" \
        "pop %%es\n\t" \
        "pop %%ds\n\t" \
        "iret\n\t" \
        : : "i"(n) \
    ); \
}


struct regs {
    /* General-purpose registers pushed by stub */
    U32 edi, esi, ebp, ebx, edx, ecx, eax;
    /* Segment registers pushed by stub */
    U32 ds, es, fs, gs;
    /* CPU-pushed context (automatically pushed by CPU on interrupt) */
    U32 eip, cs, eflags;
    /* Optional error code (pushed by CPU or stub) */
    U32 err_code;
    /* Interrupt number (always pushed by stub) */
    U32 int_no;
};

typedef void (*ISRHandler)(struct regs* r);


void ISR_REGISTER_HANDLER(U32 int_no, ISRHandler handler);
extern ISRHandler g_Handlers[IDT_COUNT];

VOID SETUP_ISR_HANDLERS(VOID);

void isr_handler(struct regs* r, U32 int_no);
void isr_default_handler(struct regs* r);
void irq_default_handler(struct regs* r);
void page_fault_handler(struct regs* r);
void gpf_handler(struct regs* r);
void timer_handler(struct regs* r);
void keyboard_handler(struct regs* r);
void double_fault_handler(struct regs* r);

/* declare externs for stubs */
ISR_STUB_DEF(0) // Division by Zero
ISR_STUB_DEF(1) // Debug
ISR_STUB_DEF(2) // NMI
ISR_STUB_DEF(3) // Breakpoint
ISR_STUB_DEF(4) // Overflow
ISR_STUB_DEF(5) // Bounds Check
ISR_STUB_DEF(6) // Invalid Opcode
ISR_STUB_DEF(7) // Device Not Available
ISR_STUB_DEF(8) // Double Fault
ISR_STUB_DEF(9) // Coprocessor Segment Overrun
ISR_STUB_DEF(10) // Invalid TSS
ISR_STUB_DEF(11) // Segment Not Present
ISR_STUB_DEF(12) // Stack Fault
ISR_STUB_DEF(13) // General Protection Fault
ISR_STUB_DEF(14) // Page Fault
ISR_STUB_DEF(15) // Reserved
ISR_STUB_DEF(16)
ISR_STUB_DEF(17)
ISR_STUB_DEF(18)
ISR_STUB_DEF(19)
ISR_STUB_DEF(20)
ISR_STUB_DEF(21)
ISR_STUB_DEF(22)
ISR_STUB_DEF(23)
ISR_STUB_DEF(24)
ISR_STUB_DEF(25)
ISR_STUB_DEF(26)
ISR_STUB_DEF(27)
ISR_STUB_DEF(28)
ISR_STUB_DEF(29)
ISR_STUB_DEF(30)
ISR_STUB_DEF(31)
ISR_STUB_DEF(32) // IRQ0 - System Timer
ISR_STUB_DEF(33) // IRQ1 - Keyboard
ISR_STUB_DEF(34) // IRQ2 - Cascade / Reserved
ISR_STUB_DEF(35) // IRQ3 - COM2
ISR_STUB_DEF(36) // IRQ4 - COM1
ISR_STUB_DEF(37) // IRQ5 - LPT2 / Sound Card
ISR_STUB_DEF(38) // IRQ6 - Floppy Disk
ISR_STUB_DEF(39) // IRQ7 - LPT1 / Parallel Port
ISR_STUB_DEF(40) // IRQ8 - RTC / CMOS
ISR_STUB_DEF(41) // IRQ9 - ACPI / Redirected IRQ2
ISR_STUB_DEF(42) // IRQ10 - General Purpose / Network
ISR_STUB_DEF(43) // IRQ11 - General Purpose / Network
ISR_STUB_DEF(44) // IRQ12 - Mouse
ISR_STUB_DEF(45) // IRQ13 - FPU / Coprocessor
ISR_STUB_DEF(46) // IRQ14 - Primary ATA
ISR_STUB_DEF(47) // IRQ15 - Secondary ATA
ISR_STUB_DEF(100) // Default stub for other interrupts

#endif // ISR_H