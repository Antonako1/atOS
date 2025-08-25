/*+++
    Source/KERNEL/32RTOSKRNL/CPU/ISR/ISR.c - Interrupt Service Routine (ISR) Implementation

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
---*/
#include "./ISR.h"
#include "../IRQ/IRQ.h"
#include "../IDT/IDT.h"

ISRHandler g_Handlers[IDT_COUNT];


// Called by the ISR stub
void isr_handler(struct regs* r, U32 int_no) {
    r->int_no = int_no; // populate int_no

    if (int_no < IDT_COUNT && g_Handlers[int_no]) {
        g_Handlers[int_no](r);
    }
}

// Called by the global IRQ handler
void isr_default_handler(struct regs* r) {  
}
void irq_default_handler(struct regs* r) {

    if(r->int_no >= 32 && r->int_no < 48) {
        pic_send_eoi(r->int_no - 32);
    }
}
void page_fault_handler(struct regs* r) {

    U32 err = r->err_code;
    U32 fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));
    (void)fault_addr;
}
void gpf_handler(struct regs* r) { // General Protection Fault

    U32 err = r->err_code;
    (void)err;
    // for (;;) __asm__ volatile("hlt");
}
void double_fault_handler(struct regs* r) {
    U32 err = r->err_code;
    (void)err;
    // for (;;) __asm__ volatile("hlt");
}
void timer_handler(struct regs* r) { 
    irq_default_handler(r); 
    // Add your scheduler tick handling here
}
void keyboard_handler(struct regs* r) { 
    irq_default_handler(r); 
    // Read scancode from port 0x60 etc.
}





void ISR_REGISTER_HANDLER(U32 int_no, ISRHandler handler) {
    if(int_no < IDT_COUNT)
        g_Handlers[int_no] = handler;
}

VOID SETUP_ISR_HANDLERS(VOID) {
    for(int i = 0; i < IDT_COUNT; i++) {
        if(i < 32) {
            // CPU exceptions
            switch(i) {
                case 13: ISR_REGISTER_HANDLER(i, gpf_handler); break;
                case 14: ISR_REGISTER_HANDLER(i, page_fault_handler); break;
                case 8:  ISR_REGISTER_HANDLER(i, double_fault_handler); break;
                default: ISR_REGISTER_HANDLER(i, isr_default_handler); break;
            }
        } else if(i >= 32 && i < 48) {
            // Hardware IRQs
            switch(i) {
                case 32: ISR_REGISTER_HANDLER(i, timer_handler); break;
                case 33: ISR_REGISTER_HANDLER(i, keyboard_handler); break;
                default: ISR_REGISTER_HANDLER(i, irq_default_handler); break;
            }
        } else {
            // Reserved / unused
            ISR_REGISTER_HANDLER(i, isr_default_handler);
        }
    }
}


ISR_STUB(0)  // Division by Zero
ISR_STUB(1)  // Debug
ISR_STUB(2)  // NMI
ISR_STUB(3)  // Breakpoint
ISR_STUB(4)  // Overflow
ISR_STUB(5)  // Bounds Check
ISR_STUB(6)  // Invalid Opcode
ISR_STUB(7)  // Device Not Available
ISR_STUB_ERRCODE(8)  // Double Fault (error code pushed)
ISR_STUB(9)  // Coprocessor Segment Overrun
ISR_STUB_ERRCODE(10) // Invalid TSS
ISR_STUB_ERRCODE(11) // Segment Not Present
ISR_STUB_ERRCODE(12) // Stack Fault
ISR_STUB_ERRCODE(13) // General Protection Fault
ISR_STUB_ERRCODE(14) // Page Fault
ISR_STUB(15) // Reserved
ISR_STUB(16)
ISR_STUB_ERRCODE(17) // Alignment Check (pushes error code under some conditions)
ISR_STUB(18)
ISR_STUB(19)
ISR_STUB(20)
ISR_STUB(21)
ISR_STUB(22)
ISR_STUB(23)
ISR_STUB(24)
ISR_STUB(25)
ISR_STUB(26)
ISR_STUB(27)
ISR_STUB(28)
ISR_STUB(29)
ISR_STUB(30)
ISR_STUB(31)

/* IRQ stubs 32..47 */
ISR_STUB(32) // IRQ0 - System Timer
ISR_STUB(33) // IRQ1 - Keyboard
ISR_STUB(34) // IRQ2 - Cascade / Reserved
ISR_STUB(35) // IRQ3 - COM2
ISR_STUB(36) // IRQ4 - COM1
ISR_STUB(37) // IRQ5 - LPT2 / Sound Card
ISR_STUB(38) // IRQ6 - Floppy Disk
ISR_STUB(39) // IRQ7 - LPT1 / Parallel Port
ISR_STUB(40) // IRQ8 - RTC / CMOS
ISR_STUB(41) // IRQ9 - ACPI / Redirected IRQ2
ISR_STUB(42) // IRQ10 - General Purpose / Network
ISR_STUB(43) // IRQ11 - General Purpose / Network
ISR_STUB(44) // IRQ12 - Mouse
ISR_STUB(45) // IRQ13 - FPU / Coprocessor
ISR_STUB(46) // IRQ14 - Primary ATA
ISR_STUB(47) // IRQ15 - Secondary ATA

ISR_STUB(100) // Default stub for other interrupts

