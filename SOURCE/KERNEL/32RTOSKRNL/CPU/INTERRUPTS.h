/*+++
    Source/KERNEL/32RTOSKRNL/CPU/INTERRUPTS.h - Interrupts Definitions

    Part of atOS-RT

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    32-bit interrupt definitions for atOS-RT.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/24 - Antonako1
        Initial version. Contains GDT, IDT, and LDT constants.

REMARKS

    When compiling include:
        GDT/GDT.c
        IDT/IDT.c
        LDT/LDT.c
        ISR/ISR.c
---*/
#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "./IDT/IDT.h"
#include "./GDT/GDT.h"
#include "./IRQ/IRQ.h"


#endif // INTERRUPTS_H