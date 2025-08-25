#include "IDT.h"
#include "../GDT/GDT.h"
#include "../ISR/ISR.h"

// Pointer to memory-mapped IDT
static IDTDESCRIPTOR idtr;
// Low-level function to set a gate in memory
#define IDT_PTR ((IDTENTRY*)IDT_MEM_BASE)

// Low-level function to set a gate in memory
void idt_set_gate(I32 idx, U32 handler_addr, U16 sel, U8 type_attr) {
    IDTENTRY* gate = &IDT_PTR[idx];
    gate->OffsetLow  = handler_addr & 0xFFFF;
    gate->Selector   = sel;
    gate->Zero       = 0;
    gate->TypeAttr   = type_attr;
    gate->OffsetHigh = (handler_addr >> 16) & 0xFFFF;
}

VOID IDT_INIT(U0) {
    // first 32 entries
    void (*first32[32])(void) = {
        isr_0,  isr_1,  isr_2,  isr_3,  isr_4,  isr_5,  isr_6,  isr_7,
        isr_8,  isr_9,  isr_10, isr_11, isr_12, isr_13, isr_14, isr_15,
        isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23,
        isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31
    };
    void (*irq_stubs[16])(void) = {
        isr_32, isr_33, isr_34, isr_35, isr_36, isr_37, isr_38, isr_39,
        isr_40, isr_41, isr_42, isr_43, isr_44, isr_45, isr_46, isr_47
    };


    for (int i = 0; i < 256; ++i) {
        if(i < 32)
            idt_set_gate(i, (U32)first32[i], KCODE_SEL, INT_GATE_32);
        else if(i <= 32 && i < 48)
            idt_set_gate(i, (U32)irq_stubs[i - 32], KCODE_SEL, INT_GATE_32);
        else
            idt_set_gate(i, (U32)isr_100, KCODE_SEL, INT_GATE_32); // Null handler for now
    }



    idtr.size = IDT_COUNT * sizeof(IDTENTRY) - 1;
    idtr.offset = IDT_MEM_BASE;
    __asm__ volatile("lidt %0" : : "m"(idtr));
}
