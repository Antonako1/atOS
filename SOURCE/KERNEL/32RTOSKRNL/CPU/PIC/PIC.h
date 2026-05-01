#ifndef PIC_H
#define PIC_H

#include "../../../STD/TYPEDEF.h"

#define PIC1_COMMAND_PORT 0x20
#define PIC1_DATA_PORT 0x21
#define PIC2_COMMAND_PORT 0xA0
#define PIC2_DATA_PORT 0xA1
#define PIC_EOI 0x20


void PIC_Unmask(int irq);
void PIC_Mask(int irq);

#define PIC_REMAP_OFFSET 0x20
#define PIC_REMAP_OFFSET2 0x28
#define PIC_REMAP_END     0x30

void pic_send_eoi(U8 irq);
void pic_remap(U8 offset1, U8 offset2);


#endif // PIC_H