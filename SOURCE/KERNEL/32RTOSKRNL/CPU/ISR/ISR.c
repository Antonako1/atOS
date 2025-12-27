/*+++
    Source/KERNEL/32RTOSKRNL/CPU/ISR/ISR.c - Interrupt Service Routine (ISR) Implementation

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    32-bit interrupt service routine (ISR) implementation for atOS.

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
#include "../../../../STD/ASM.h"
#include "../INTERRUPTS/INTERRUPTS.h"
#include "../PIC/PIC.h"

#ifdef __RTOS__
#include <CPU/SYSCALL/SYSCALL.h> // For SYSCALL_VECTOR
#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>
#include <DRIVERS/VIDEO/VBE.h>
#include <CPU/PIC/PIC.h>
#include <CPU/YIELD/YIELD.h>
#include <RTL8139/RTL8139.h>
static ISRHandler g_Handlers[IDT_COUNT] __attribute__((section(".data"))) = { 0 };
#else // __RTOS__
static ISRHandler g_Handlers[IDT_COUNT]  = { 0 };
#endif // __RTOS__


ISRHandler *ISR_GET_PTR(void) {
    return g_Handlers;
}

// This is the c-level exception handler
void isr_common_handler(I32 num, U32 errcode) {
    (void)errcode; (void)num;
    #ifdef __RTOS__
    CLI;
    if(num >= 32 && num < 48) {
        // IRQ - should not happen here
        switch (num)
        {
        default:
            panic(PANIC_TEXT("IRQ received in isr_common_handler!"), errcode);
            return;
        }
    } else {
        panic(PANIC_TEXT("Unhandled Exception. Error code provided as argument"), errcode);
    }
    #endif // __RTOS__
    HLT;
}

void double_fault_handler(I32 num, U32 errcode) {
    (void)errcode; (void)num;
    #ifdef __RTOS__
    CLI;
    // TODO: try recovering from double fault by killing offending process
    panic(PANIC_TEXT("Double Fault Exception"), PANIC_DOUBLE_FAULT);
    #endif // __RTOS__
    HLT;
}

void irq_common_handler(I32 vector, U32 errcode) {
    (void)errcode;
    // vector is 0x20..0x2F for IRQs
    #ifdef __RTOS__
    if (vector < PIC_REMAP_OFFSET || vector >= PIC_REMAP_END) {
        // Not an IRQ, should not happen here
        panic(PANIC_TEXT("Non-IRQ received in irq_common_handler!"), vector);
        return;
    }
    // Only dump if there is no registered handler (avoid noisy consoles)
    if (!g_Handlers[vector]) {
        if(vector != 0x21 && vector != 0x28) DUMP_ERRCODE(vector);
    }
    #endif // __RTOS__
    if (g_Handlers[vector]) {
        g_Handlers[vector](vector, errcode);
    } else {
        int irq = vector - PIC_REMAP_OFFSET; // 0x20
        if ((unsigned)irq < 16) pic_send_eoi(irq);
    }
}

#ifdef __RTOS__
#include <DRIVERS/VIDEO/VBE.h>

void undefined_opcode_handler(I32 num, U32 errcode, regs *r) {
    (void)num; (void)errcode; (void)r;
    CLI;
    panic_reg(r, PANIC_TEXT("Undefined Opcode Exception"), PANIC_UNDEFINED_OPCODE);
    HLT;
}

void fpu87_fault_handler(I32 num, U32 errcode, regs *r) {
    (void)num; (void)errcode; (void)r;
    CLI;
    panic_reg(r, PANIC_TEXT("Floating Point Exception"), PANIC_FPU_FAULT);
    HLT;
}

void de_fault_handler(I32 num, U32 errcode, regs *r) {
    (void)num; (void)errcode; (void)r;
    CLI;
    panic_reg(r, PANIC_TEXT("Divide Error Exception"), PANIC_DIVIDE_ERROR);
    HLT;
}

void nmi_handler(I32 num, U32 errcode, regs *r) {
    (void)num; (void)errcode; (void)r;
    CLI;
    panic_reg(r, PANIC_TEXT("Non-Maskable Interrupt"), PANIC_UNKNOWN_ERROR);
    HLT;
}

void page_fault_handler(I32 num, U32 errcode) {
    (void)num;
    // CR2 contains the faulting address
    U32 faulting_address;
    ASM_VOLATILE("mov %%cr2, %0" : "=r"(faulting_address));
    // Error code bits:
    // Bit 0: Present (0 = not-present page, 1 = protection violation)
    // Bit 1: Write (0 = read operation, 1 = write operation)
    // Bit 2: User (0 = kernel-mode access, 1 = user-mode access)
    // Bit 3: Reserved Write (1 = caused by reserved bits set to 1 in page directory)
    // Bit 4: Instruction Fetch (1 = caused by an instruction fetch)
    U8 present   = !(errcode & 0x1); // Page not present
    U8 rw        = errcode & 0x2;    // Write operation?
    U8 us        = errcode & 0x4;    // Processor was in user-mode?
    U8 reserved  = errcode & 0x8;    // Overwritten CPU-reserved bits of page entry?
    U8 id        = errcode & 0x10;   // Caused by an instruction fetch?

    U8 buf[128];
    ITOA_U(faulting_address, buf, 16);
    VBE_DRAW_STRING(10, 10, "Page fault at ", VBE_WHITE, VBE_BLACK);
    VBE_DRAW_STRING(150, 10, buf, VBE_WHITE, VBE_BLACK);
    VBE_DRAW_STRING(10, 30, "Error code: ", VBE_WHITE, VBE_BLACK);
    ITOA_U(errcode, buf, 16);
    VBE_DRAW_STRING(150, 30, buf, VBE_WHITE, VBE_BLACK);
    VBE_DRAW_STRING(10, 50, "Details:", VBE_WHITE, VBE_BLACK);
    VBE_DRAW_STRING(10, 70, present ? " - Not Present" : " - Protection Violation", VBE_WHITE, VBE_BLACK);
    VBE_DRAW_STRING(10, 90, rw ? " - Write Operation" : " - Read Operation", VBE_WHITE, VBE_BLACK);
    VBE_DRAW_STRING(10, 110, us ? " - User Mode" : " - Kernel Mode", VBE_WHITE, VBE_BLACK);
    system_halt();
}
#endif // __RTOS__

// This function is called from assembly
void isr_dispatch_c(int vector, U32 errcode, regs *regs_ptr) {
    #ifdef __RTOS__
    if (vector == SYSCALL_VECTOR) {
        // Syscall handling (must run regardless of g_Handlers)
        U32 syscall_num = regs_ptr->eax;
        U32 a1 = regs_ptr->ebx;
        U32 a2 = regs_ptr->ecx;
        U32 a3 = regs_ptr->edx;
        U32 a4 = regs_ptr->esi;
        U32 a5 = regs_ptr->edi;
        U32 ret = syscall_dispatcher(syscall_num, a1, a2, a3, a4, a5);
        regs_ptr->eax = ret; // Return value in eax
        return;
    }else if (vector == YIELD_VECTOR) {
        yield_handler(vector, errcode);
        return;
    }
    
    switch (vector)
    {
    case 0: // Divide Error
        de_fault_handler(vector, errcode, regs_ptr);
        break;
    case 2: // NMI
        nmi_handler(vector, errcode, regs_ptr);
        break;
    case 6: // Invalid Opcode
        undefined_opcode_handler(vector, errcode, regs_ptr);
        break;
    case 8: // Double Fault
        double_fault_handler(vector, errcode);
        break;
    case 14: // Page Fault
        page_fault_handler(vector, errcode);
        break;
    case 16: // FPU Fault
        fpu87_fault_handler(vector, errcode, regs_ptr);
        break;
    default:
        if(g_Handlers[errcode]) {
            g_Handlers[errcode](vector, errcode);
            return;
        }
    }
    #else
    (void)regs_ptr;

    if(g_Handlers[errcode]) {
        g_Handlers[errcode](vector, errcode);
        return;
    }
    #endif // __RTOS__
    
    isr_common_handler(vector, errcode);
}

void irq_dispatch_c(int vector, U32 errcode, regs *regs_ptr) {
    (void)regs_ptr;
    irq_common_handler(vector, errcode); // We send the full 0x20..0x2F vector
}


void ISR_REGISTER_HANDLER(U32 int_no, ISRHandler handler) {
    if(int_no < IDT_COUNT) {
        g_Handlers[int_no] = handler;
    }
}

U0 SETUP_ISRS(U0) {
    U16 cs = 0x08; // code selector
    U8 flags = 0x8E; // present, ring0, 32-bit interrupt gate
    void *isr[49] = {
        isr0,isr1,isr2,isr3,isr4,isr5,isr6,isr7,
        isr8,isr9,isr10,isr11,isr12,isr13,isr14,isr15,
        isr16,isr17,isr18,isr19,isr20,isr21,isr22,isr23,
        isr24,isr25,isr26,isr27,isr28,isr29,isr30,isr31,
        irq32, irq33, irq34, irq35, irq36, irq37, irq38, irq39,
        irq40, irq41, irq42, irq43, irq44, irq45, irq46, irq47,
        isr48
    };
    for(U32 i = 0; i < IDT_COUNT; i++) {
        if (i < 49) idt_set_gate(i, isr[i], cs, flags);
        else        idt_set_gate(i, isr[48], cs, flags);
    }
    #ifdef __RTOS__
        idt_set_gate(SYSCALL_VECTOR, isr_syscall, cs, flags);
    #endif // __RTOS__
}
VOID SETUP_ISR_HANDLERS(VOID) {
    for(int i = 0; i < IDT_COUNT; i++) {
        if(i >= PIC_REMAP_OFFSET && i < PIC_REMAP_END) {
            ISR_REGISTER_HANDLER(i, irq_common_handler); // No default handler
        } else {
            ISR_REGISTER_HANDLER(i, isr_common_handler); // Reserved / unused
        }
    }

    #ifdef __RTOS__
    ISR_REGISTER_HANDLER(0, de_fault_handler);
    ISR_REGISTER_HANDLER(6, undefined_opcode_handler);
    ISR_REGISTER_HANDLER(16, fpu87_fault_handler);
    ISR_REGISTER_HANDLER(14, page_fault_handler);
    ISR_REGISTER_HANDLER(8, double_fault_handler);
    ISR_REGISTER_HANDLER(2, nmi_handler);
    #endif // __RTOS__
}
