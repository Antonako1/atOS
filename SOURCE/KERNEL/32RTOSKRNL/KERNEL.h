/*+++
    Source/KERNEL/32RTOSKRNL/KERNEL.h - 32-bit Kernel Entry master header

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    32-bit kernel entry point master header for atOS.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version. Contains kernel entry point declarations and text-mode output API.

REMARKS
    To be used only by the kernel entry point implementation. (KERNEL.c)
    This file should NOT be included in user applications or libraries.
---*/

#ifndef KERNEL_H
#define KERNEL_H
#include "../../STD/TYPEDEF.h"
#include "../../STD/ASM.h"
#ifndef KERNEL_ENTRY
#define KERNEL_ENTRY
#endif // KERNEL_ENTRY

#include "./DRIVERS/VESA/VBE.h"
#include "./DRIVERS/ATAPI/ATAPI.h"
#include "./CPU/INTERRUPTS/INTERRUPTS.h"

#define RM2LA(seg, off)  (((U32)(seg) << 4) + (U32)(off))
#define FAR_PTR_TO_LINEAR(ptr)  RM2LA(((ptr) >> 16) & 0xFFFF, (ptr) & 0xFFFF)

/* -------------------- Kernel Entry -------------------- */

/* Forward declaration of the main kernel entry function */
__attribute__((noreturn))
U0 kernel_entry_main(U0);

/* Startup function called by bootloader / linker script */
__attribute__((noreturn, section(".text")))
void _start(void);



#endif /* KERNEL_H */
