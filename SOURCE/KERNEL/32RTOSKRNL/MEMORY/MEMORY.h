// MEMORY.h - Master memory include file for 32RTOSKRNL
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
//
// DESCRIPTION:
//     Master memory definitions for 32RTOSKRNL memory regions.
//
// AUTHORS:
//     Antonako1
//
// REVISION HISTORY:
//     2025/05/26 - Antonako1
//         Created this file.
//      2025/08/19 - Antonako1
//         Updated memory regions.
// REMARKS:
//     See MEMORY.md for a detailed description of the memory regions.

#ifndef MEMORY_H
#define MEMORY_H

// Real Mode / BIOS areas
#define MEM_IVT_BASE              0x00000000
#define MEM_IVT_END               0x000003FF
#define MEM_BDA_BASE              0x00000400
#define MEM_BDA_END               0x000004FF
#define MEM_BIOS_RESERVED_BASE    0x00000500
#define MEM_BIOS_RESERVED_END     0x00000FFF

// Bootloader + Kernel early
#define MEM_BOOTLOADER_BASE       0x00001000
#define MEM_BOOTLOADER_END        0x00001FFF
#define MEM_KRNL_BASE             0x00002000
#define MEM_KRNL_END              0x00007FFF
#define MEM_E820_BASE             0x00008000
#define MEM_E820_END              0x00008FFF

// VESA / Descriptor tables
#define MEM_VESA_CTRL_INFO_BASE   0x00009000
#define MEM_VESA_CTRL_INFO_END    0x000091FF
#define MEM_VBE_MODE_INFO_BASE    0x00009200
#define MEM_VBE_MODE_INFO_END     0x000092FF

// IDT / GDT / LDT
#define MEM_GDT_BASE              0x00009300
#define MEM_GDT_END               0x000093FF
#define MEM_IDT_BASE              0x00009400
#define MEM_IDT_END               0x00009BFF
#define MEM_LDT_BASE              0x00009C00
#define MEM_LDT_END               0x00009DFF

// 512kb of free memory
#define MEM_UNUSED_BASE           0x00009E00
#define MEM_UNUSED_END            0x00009FFF

// Legacy video / Stack
#define MEM_VGA_BASE              0x000A0000
#define MEM_VGA_END               0x000BFFFF
#define MEM_STACK_BASE            0x00080000
#define MEM_STACK_END             0x000BFFFF

// BIOS ROM
#define MEM_BIOS_ROM_BASE         0x000C0000
#define MEM_BIOS_ROM_END          0x000FFFFF

// Kernel memory
#define MEM_KERNEL_DATA_BASE      0x00100000
#define MEM_KERNEL_DATA_END       0x001FFFFF
#define MEM_RTOSKRNL_BASE         0x00200000
#define MEM_RTOSKRNL_END          0x003FFFFF
#define MEM_KERNEL_HEAP_BASE      0x00400000
#define MEM_KERNEL_HEAP_END       0x005FFFFF
#define MEM_PROGRAM_TMP_BASE      0x00600000
#define MEM_PROGRAM_TMP_END       0x00DFFFFF

// Paging + Framebuffer
#define MEM_PAGING_BASE           0x00E00000
#define MEM_PAGING_END            0x00EFFFFF

#define MEM_FRAMEBUFFER_BASE      0x00F00000
#define MEM_FRAMEBUFFER_END       0x012004EF

// ACPI / APIC
#define MEM_ACPI_APIC_BASE        0x01201000
#define MEM_ACPI_APIC_END         0x012FFFFF

// Reserved
#define MEM_RESERVED_BASE         0x01300000
#define MEM_RESERVED_END          0x076FFFFF

// User space
#define MEM_USER_SPACE_BASE       0x07700000
#define MEM_USER_SPACE_END        0x1FFFFFFF


#endif // MEMORY_H
