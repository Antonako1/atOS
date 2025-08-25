#ifndef IDT_H
#define IDT_H

#include "../../../../STD/ATOSMINDEF.h"
#include "../../MEMORY/MEMORY.h"
#include "../ISR/ISR.h"

#define IDT_COUNT 256
#define IDT_MEM_BASE MEM_IDT_BASE
#define IDT_MEM_END (MEM_IDT_BASE + IDT_COUNT*8)
#define IDT_MEM_SIZE (IDT_MEM_END - IDT_MEM_BASE)
#if IDT_MEM_SIZE % 8 != 0
#error "IDT memory size is not a multiple of 8"
#endif
#define INT_GATE_32 0x8E

typedef struct __attribute__((packed)) {
    U16 OffsetLow;
    U16 Selector;
    U8  Zero;
    U8  TypeAttr;
    U16 OffsetHigh;
} IDTENTRY;

typedef struct __attribute__((packed)) {
    U16 size;
    U32 offset;
} IDTDESCRIPTOR;

void idt_set_gate(I32 idx, U32 handler_addr, U16 sel, U8 type_attr);
VOID IDT_INIT(U0);

#endif
