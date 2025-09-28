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
#include "../../DRIVERS/PIT/PIT.h"

#ifdef __RTOS__
#include <CPU/SYSCALL/SYSCALL.h> // For SYSCALL_VECTOR
// #include <RTOSKRNL/RTOSKRNL_INTERNAL.h>
#endif // __RTOS__

static ISRHandler g_Handlers[IDT_COUNT] = { 0 };

ISRHandler *ISR_GET_PTR(void) {
    return g_Handlers;
}

// This is the c-level exception handler
void isr_common_handler(I32 num, U32 errcode) {
    (void)errcode; (void)num;
    HLT;
}

void double_fault_handler(I32 num, U32 errcode) {
    (void)errcode; (void)num;
    HLT;
}


void irq_common_handler(I32 vector, U32 errcode) {
    (void)errcode;
    if (g_Handlers[vector]) g_Handlers[vector](vector, 0);
    int irq = vector - PIC_REMAP_OFFSET; // 0x20
    if ((unsigned)irq < 16) pic_send_eoi(irq);

}

#ifdef __RTOS__
#include <DRIVERS/VIDEO/VOUTPUT.h>
#endif // __RTOS__

// This function is called from assembly
void isr_dispatch_c(int vector, U32 errcode, regs *regs_ptr) {
    #ifdef __RTOS__
    if (vector == SYSCALL_VECTOR) {
        // DUMP_REGS(regs_ptr);
        // DUMP_ERRCODE(errcode);
        // DUMP_INTNO(vector);
        // DUMP_MEMORY((U32)regs_ptr, 64);
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
    }
    #else
    (void)regs_ptr;
    #endif // __RTOS__
    if(g_Handlers[vector]) {
        g_Handlers[vector](vector, errcode);
        return;
    }
    isr_common_handler(vector, errcode);
}

void irq_dispatch_c(int vector, U32 errcode, regs *regs_ptr) {
    (void)regs_ptr;
    irq_common_handler(vector, errcode);

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
        if(i < 32) {
            switch(i) {
                case 8:
                    ISR_REGISTER_HANDLER(i, double_fault_handler);
                    break;
                default:
                    ISR_REGISTER_HANDLER(i, isr_common_handler); // CPU exceptions
            }
        } else if(i >= 32 && i < 48) {
            ISR_REGISTER_HANDLER(i, irq_common_handler); // No default handler
        } 
        #ifdef __RTOS__
        else if(i == SYSCALL_VECTOR) {
            ISR_REGISTER_HANDLER(i, isr_syscall);
        }
        #endif // __RTOS__
        else {
            ISR_REGISTER_HANDLER(i, isr_common_handler); // Reserved / unused
        }
    }
}