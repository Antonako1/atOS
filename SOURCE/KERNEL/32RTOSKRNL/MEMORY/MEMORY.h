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
//     2025/08/19 - Antonako1
//         Updated memory regions.
//     2025/08/27 - Antonako1
//         Optimized memory map for protected mode, reuse low memory, moved kernel to 0x20000.
//
// REMARKS:
//     See MEMORY.md for a detailed description of the memory regions.

#ifndef MEMORY_H
#define MEMORY_H

// Low Memory / Reused BIOS areas
#define MEM_LOW_RESERVED_BASE      0x00000000
#define MEM_LOW_RESERVED_END       0x00000FFF

// Bootloader + Early Kernel Stub
#define MEM_BOOTLOADER_BASE        0x00001000
#define MEM_BOOTLOADER_END         0x00001FFF
#define MEM_KRNL_STUB_BASE         0x00002000
#define MEM_KRNL_STUB_END          0x00007FFF
#define MEM_E820_BASE              0x00008000
#define MEM_E820_END               0x00008FFF

#define MEM_DRIVE_LETTER_BASE      0x00008000
#define MEM_DRIVE_LETTER_END       0x00008001
#define EARLY_FREE_DATA_BASE       0x00008002
#define EARLY_FREE_DATA_END        0x000093FF
// Early Stack
#define MEM_STACK_BASE             0x00009300
#define MEM_STACK_END              0x00009FFF

// Free / Temporary Buffers
#define MEM_TEMP_BASE              0x0000A000
#define MEM_TEMP_END               0x0000FFFF

// Kernel entry point
#define MEM_KRNL_BASE              0x00020000
#define MEM_KRNL_END               0x003FFFFF  // 3.5 MiB size

// RTOS kernel
#define MEM_RTOSKRNL_BASE              0x00024000
#define MEM_RTOSKRNL_END               0x00424000

// Kernel Heap
#define MEM_KERNEL_HEAP_BASE       0x00600000
#define MEM_KERNEL_HEAP_END        0x007FFFFF

// Program / Temp
#define MEM_PROGRAM_TMP_BASE       0x00600000
#define MEM_PROGRAM_TMP_END        0x00DFFFFF

// Paging Structures
#define MEM_PAGING_BASE            0x00E00000
#define MEM_PAGING_END             0x00EFFFFF

// Framebuffer
#define MEM_FRAMEBUFFER_BASE       0x00F00000
#define MEM_FRAMEBUFFER_END        0x012004EF

// ACPI / APIC
#define MEM_ACPI_APIC_BASE         0x01201000
#define MEM_ACPI_APIC_END          0x012FFFFF

// Reserved
#define MEM_RESERVED_BASE          0x01300000
#define MEM_RESERVED_END           0x076FFFFF

// User Space
#define MEM_USER_SPACE_BASE        0x07700000
#define MEM_USER_SPACE_END         0x1FFFFFFF

#endif // MEMORY_H
