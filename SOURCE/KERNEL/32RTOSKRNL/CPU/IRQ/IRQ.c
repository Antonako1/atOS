/*+++
    Source/KERNEL/32RTOSKRNL/CPU/IRQ/IRQ.c - Interrupt Request (IRQ) Implementation

    Part of atOS-RT

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    32-bit interrupt request (IRQ) implementation for atOS-RT.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/24 - Antonako1
        Initial version. IRQ remapping functions added.
REMARKS
---*/

#include "./IRQ.h"
#include "../IDT/IDT.h"
#include "../ISR/ISR.h"
#include "../GDT/GDT.h"
#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1
#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

// Send End-of-Interrupt (EOI) to PICs
void pic_send_eoi(U8 irq) {
    if(irq >= 8) 
        outb(PIC2_CMD, 0x20); // Slave PIC
    outb(PIC1_CMD, 0x20);     // Master PIC
}

// Remap the PIC to avoid conflicts with CPU exceptions
void pic_remap() {
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

    outb(PIC1_DATA, 0x20); // Master PIC offset (IRQ0-7 -> INT 32-39)
    outb(PIC2_DATA, 0x28); // Slave PIC offset  (IRQ8-15 -> INT 40-47)

    outb(PIC1_DATA, 4);    // Tell Master about Slave at IRQ2
    outb(PIC2_DATA, 2);    // Tell Slave its cascade identity

    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, 0); // Unmask all IRQs
    outb(PIC2_DATA, 0);
}

// Initialize IRQs and set IDT gates
U0 IRQ_INIT(U0) {
    pic_remap();
}
