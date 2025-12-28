#ifndef RTOSKRNL_H
#define RTOSKRNL_H
#ifndef RTOS_KERNEL
#define RTOS_KERNEL
#endif // RTOS_KERNEL

#ifndef NO_INCLUDES
#include <STD/ASM.h>
#include <STD/TYPEDEF.h>
#include <STD/STRING.h>
#include <STD/MEM.h>

#include <DRIVERS/VIDEO/VBE.h>
#include <DRIVERS/VIDEO/VESA.h>
#include <DRIVERS/PS2/KEYBOARD_MOUSE.h>
#include <DRIVERS/ATAPI/ATAPI.h>
#include <DRIVERS/ATA_PIIX3/ATA_PIIX3.h>
#include <DRIVERS/ATA_PIO/ATA_PIO.h>
#include <DRIVERS/CMOS/CMOS.h>
#include <DRIVERS/BEEPER/BEEPER.h>
#include <DRIVERS/RTL8139/RTL8139.h>
#include <DRIVERS/AC97/AC97.h>

#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>
#include <RTOSKRNL/PROC/PROC.h>

#include <MEMORY/E820/E820.h>
#include <MEMORY/PAGING/PAGING.h>
#include <MEMORY/PAGEFRAME/PAGEFRAME.h>
#include <MEMORY/HEAP/KHEAP.h>

#include <CPU/PIT/PIT.h>
#include <CPU/PIC/PIC.h>
#include <CPU/GDT/GDT.h>
#include <CPU/IDT/IDT.h>
#include <CPU/ISR/ISR.h>
#include <CPU/IRQ/IRQ.h>
#include <CPU/FPU/FPU.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <CPU/INTERRUPTS/INTERRUPTS.h>
#endif // NO_INCLUDES

__attribute__((noreturn))
U0 rtos_kernel(U0);

__attribute__((noreturn, section(".text")))
void _start(U0);


#endif // RTOSKRNL_H
