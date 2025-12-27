#include "./INTERRUPTS/INTERRUPTS.h"
#include "../../../STD/ASM.h"

// Common ISR handler wrapper
typedef void (*isr_t)(void);

// Adjusted ISR macros to pass correct regs pointer
// Exceptions without CPU error code
#define ISR_NOERRORCODE(n) \
__attribute__((naked)) void isr##n(void) { \
    asm volatile( \
        "pusha\n\t" \
        "movl %%esp, %%eax\n\t"   /* eax = current esp */ \
        "pushl %%eax\n\t"         /* push regs* argument */ \
        "pushl $0\n\t"            /* fake errcode */ \
        "pushl $" #n "\n\t"       /* vector */ \
        "call isr_dispatch_c\n\t" \
        "addl $12, %%esp\n\t"     /* pop args */ \
        "popa\n\t" \
        "iret\n\t" ::: "eax", "memory"); \
}

// Exceptions with CPU error code (e.g., page fault)
#define ISR_ERRORCODE(n) \
__attribute__((naked)) void isr##n(void) { \
    asm volatile( \
        "pusha\n\t" \
        "movl 32(%%esp), %%edx\n\t" /* edx = CPU error code BEFORE pushing more */ \
        "movl %%esp, %%eax\n\t" \
        "pushl %%eax\n\t"          /* push regs* */ \
        "pushl %%edx\n\t"          /* push CPU error code */ \
        "pushl $" #n "\n\t"        /* vector */ \
        "call isr_dispatch_c\n\t" \
        "addl $12, %%esp\n\t" \
        "popa\n\t" \
        "iret\n\t" ::: "eax","edx","memory"); \
}

// IRQ wrappers (vectors 0x20..0x2F)
#define IRQ_WRAPPER(vec) \
__attribute__((naked)) void irq##vec(void) { \
    asm volatile( \
        "pusha\n\t" \
        "movl %%esp, %%eax\n\t" \
        "pushl %%eax\n\t"         /* push regs* */ \
        "pushl $0\n\t"            /* fake errcode */ \
        "pushl $" #vec "\n\t"     /* vector */ \
        "call irq_dispatch_c\n\t" \
        "addl $12, %%esp\n\t" \
        "popa\n\t" \
        "iret\n\t" ::: "eax","memory"); \
}

// ----------------------------------------------------
// Generate stubs
// ----------------------------------------------------
// Exceptions (some with error code)

#ifdef __RTOS__
// #define ISR_NOERRORCODE ISR_ERRORCODE
#endif // __RTOS__

ISR_NOERRORCODE(0) // Divide by zero
ISR_NOERRORCODE(1) // Debug
ISR_NOERRORCODE(2) // Non-maskable interrupt
ISR_NOERRORCODE(3) // Breakpoint
ISR_NOERRORCODE(4) // Overflow
ISR_NOERRORCODE(5) // Bounds check
ISR_NOERRORCODE(6) // Invalid opcode
ISR_NOERRORCODE(7) // Device not available
ISR_ERRORCODE(8) // Double fault
ISR_NOERRORCODE(9) // Coprocessor segment overrun
ISR_ERRORCODE(10) // Invalid TSS
ISR_ERRORCODE(11) // Segment not present
ISR_ERRORCODE(12) // Stack segment fault
ISR_ERRORCODE(13) // General protection fault
ISR_ERRORCODE(14) // Page fault
ISR_NOERRORCODE(15) // Reserved
ISR_NOERRORCODE(16) // x87 FPU Floating Point Error
ISR_ERRORCODE(17) // Alignment check
ISR_NOERRORCODE(18) // Machine check
ISR_NOERRORCODE(19) // SIMD Floating Point Exception
ISR_NOERRORCODE(20) // Virtualization Exception
ISR_NOERRORCODE(21) // Control Protection Exception
ISR_NOERRORCODE(22) // Reserved
ISR_NOERRORCODE(23) // Reserved
ISR_NOERRORCODE(24) // Reserved
ISR_NOERRORCODE(25) // Reserved
ISR_NOERRORCODE(26) // Reserved
ISR_NOERRORCODE(27) // Reserved
ISR_NOERRORCODE(28) // Reserved
ISR_NOERRORCODE(29) // Reserved
ISR_NOERRORCODE(30) // Reserved
ISR_NOERRORCODE(31) // Reserved

// IRQs (32–47)
IRQ_WRAPPER(32) // IRQ0 - Timer
IRQ_WRAPPER(33) // IRQ1 - Keyboard 
IRQ_WRAPPER(34) // IRQ2 - Cascade
IRQ_WRAPPER(35) // IRQ3 - COM2
IRQ_WRAPPER(36) // IRQ4 - COM1
IRQ_WRAPPER(37) // IRQ5 - LPT2
IRQ_WRAPPER(38) // IRQ6 - Floppy
IRQ_WRAPPER(39) // IRQ7 - LPT1
IRQ_WRAPPER(40) // IRQ8 - RTC
IRQ_WRAPPER(41) // IRQ9 - ACPI
IRQ_WRAPPER(42) // IRQ10 - Reserved (RTL8139)
IRQ_WRAPPER(43) // IRQ11 - Reserved
IRQ_WRAPPER(44) // IRQ12 - PS2 Mouse
IRQ_WRAPPER(45) // IRQ13 - FPU / Coprocessor / Inter-processor
IRQ_WRAPPER(46) // IRQ14 - Primary ATA
IRQ_WRAPPER(47) // IRQ15 - Secondary ATA

// All remaining vectors
ISR_NOERRORCODE(48)
