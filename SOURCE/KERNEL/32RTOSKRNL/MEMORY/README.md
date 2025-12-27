# Memory Management in 32RTOS Kernel

This document outlines the memory management strategy and layout used in the 32RTOS kernel. The operating system employs a flat memory model with paging enabled, allowing for efficient memory allocation and multitasking.

See memory map below for details.

To understand how the memory is managed, used, accessed and allocated, please refer to the following submodules:
- [PAGING](./PAGING/README.md) - Virtual memory and paging structures. Most important read!
- [PAGEFRAME](./PAGEFRAME/README.md) - Physical page frame management
- [HEAP](./HEAP/README.md) - HEAP management

## Directory Structure

./SOURCE/KERNEL/32RTOSKRNL/MEMORY/
    ├── ./BYTEMAP              - Bitmap but modified for byte-level tracking
    ├── ./E820                 - E820 memory map parsing and handling 
    ├── ./HEAP                 - Heap management for the whole operating system 
    ├── ./PAGEFRAME            - Physical page frame management 
    ├── ./PAGING               - Paging structures and virtual memory management 
    ├── ./MEMORY.h             - Header file with memory map definitions
    └── README.md              - This file 

## Memory map for atOS!

| Start (inclusive)     | End (exclusive)         | Label     | Size  | Notes |
| --------------------- | --------------| ----------- | --------------| ----|
| `0x00000000`          | `0x00001000`            | Low Memory Reserved (IVT+BDA)  | 4 KiB                         | Real-mode tables          |
| `0x00001000`          | `0x00002000`            | Bootloader (stage 1)           | 4 KiB                         | Stage 1 bootloader        |
| `0x00002000`          | `0x00004000`            | Stage 2 loader buffers         | 12 KiB                        | Stage 2 bootloader        |
| `0x00006000`          | `0x00007000`            | E820 map copy                  | 4 KiB                         | E820 memory map copy      |
| `0x00007000`          | `0x0000C000`            | Kernel entry stub              | 17 KiB                        | Kernel entry stub (KERNEL.c)|
| `0x0009FC00`          | `0x00100000`            | BIOS reserved                  | ~0.4 MiB                      |                           |
| `0x00100000`          | `0x00550000`            | **Kernel image**               | ~4.3 MiB                      | Text/data sections        |
| `0x00550000`          | `0x08550000`            | **Kernel heap / kmalloc pool** | **128 MiB**                   | Whole MALLOC pool|
| `0x08550000`          | `0x08560000`            | Guard page below kernel stack  | 4 KiB                         |                           |
| `0x08560000`          | `0x08570000`            | Kernel stack                   | 64 KiB                        |                           |
| `0x08570000`          | `0x08580000`            | Guard page above kernel stack  | 4 KiB                         |                           |
| `0x08580000`          | `0x0B000000`            | Framebuffer (LFB)              | ~40 MiB max                   | Size reserved for 1920x1080  |
| `0x0B000000`          | `0x10000000`            | MMIO / Device-reserved         | ~80 MiB                       | PCI/graphics/apic areas   |
| `0x10000000`          | `0x20000000`            | **User space start**           | ~256 MiB                      | For processes. Please see ./PAGING for more details |
| `0x20000000`          | `0x30000000`            | **Library space start**        | ~256 MiB                      | For libraries Please see ./PAGING for more details |
| `0x30000000` → beyond | Free / future expansion | (512 MiB +)                    | Only present if > 512 MiB RAM |                           |
