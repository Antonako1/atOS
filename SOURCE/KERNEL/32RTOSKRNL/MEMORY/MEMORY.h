// MEMORY.h - Master memory include file for 32RTOSKRNL
#ifndef MEMORY_H
#define MEMORY_H

// ----------------------------------------------------------
// Low Memory / BIOS areas
// ----------------------------------------------------------
#define MEM_LOW_RESERVED_BASE       0x00000000
#define MEM_LOW_RESERVED_END        0x00000FFF

#define MEM_E820_BASE               0x00006000
#define MEM_E820_END                0x00006384

// VESA / VBE Info Structures
#define MEM_VESA_BASE               0x00006400
#define MEM_VESA_END                0x00007000

// BIOS Reserved (Option ROMs, ACPI, etc.)
#define MEM_BIOS_BASE               0x000E0000
#define MEM_BIOS_END                0x00100000

// ----------------------------------------------------------
// Kernel Image and Core
// ----------------------------------------------------------
#define MEM_KRNL_ENTRY_BASE         0x00100000
#define MEM_KRNL_ENTRY_END          0x00112000

#define MEM_RTOSKRNL_BASE           0x00100000
#define MEM_RTOSKRNL_END            0x00550000   // ~4.3 MiB kernel image

// ----------------------------------------------------------
// Kernel Heap (128 MiB)
// ----------------------------------------------------------
// Leave 1 page (4 KiB) guard before stack base
#define MEM_RTOSKRNL_HEAP_AREA_BASE 0x00550000
#define MEM_RTOSKRNL_HEAP_AREA_END  0x08550000   // 128 MiB heap

// Derived aliases
#define MEM_KERNEL_HEAP_BASE        MEM_RTOSKRNL_HEAP_AREA_BASE
#define MEM_KERNEL_HEAP_END         MEM_RTOSKRNL_HEAP_AREA_END

// ----------------------------------------------------------
// Kernel Stack (with guards)
// ----------------------------------------------------------
#define STACK_0_GUARD_BELOW_BASE    0x08550000
#define STACK_0_GUARD_BELOW_END     0x08551000   // 4 KiB guard

#define STACK_0_BASE                0x08551000
#define STACK_0_END                 0x08561000   // 64 KiB kernel stack

#define STACK_0_GUARD_ABOVE_BASE    0x08561000
#define STACK_0_GUARD_ABOVE_END     0x08562000   // 4 KiB guard

// ----------------------------------------------------------
// Framebuffer (VBE LFB, up to ~40 MiB)
// ----------------------------------------------------------
// Shifted up to follow kernel stack guard
#define MEM_FRAMEBUFFER_BASE        0x08562000
#define MEM_FRAMEBUFFER_END         0x0B000000   // ~40 MiB typical (1920x1080x4)

// ----------------------------------------------------------
// Reserved MMIO / Firmware / PCI
// ----------------------------------------------------------
#define MEM_RESERVED_BASE           0x0B000000
#define MEM_RESERVED_END            0x10000000   // ~80 MiB MMIO

// ----------------------------------------------------------
// User Space Area
// ----------------------------------------------------------
#define MEM_USER_SPACE_BASE         0x10000000
#define MEM_USER_SPACE_END_MIN      0x20000000   // 256 MiB user space min

// ----------------------------------------------------------
// System Memory Limits
// ----------------------------------------------------------
#define MIN_ALLOWED_MEMORY          0x100000    // 1 MiB minimum
#define MAX_ALLOWED_MEMORY          (MIN_ALLOWED_MEMORY + 0x3FEE00000)  // End of last free E820 RAM region below 16 TiB
#define MAX_PHYS_ADDRESS            0xFFFFFFFF   // 4 GiB max physical address

// ----------------------------------------------------------
// Summary (512 MiB min system):
//  - Kernel image:  ~4 MiB
//  - Kernel heap:  128 MiB
//  - Framebuffer:   40 MiB
//  - MMIO:          80 MiB
//  - User space:   256 MiB
// ----------------------------------------------------------

#endif // MEMORY_H
