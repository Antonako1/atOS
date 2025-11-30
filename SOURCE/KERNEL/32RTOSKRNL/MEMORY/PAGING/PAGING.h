/*+++
    SOURCE/KERNEL/32RTOSKRNL/MEMORY/PAGING/PAGING.h - Paging Management

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    Paging management for virtual memory allocation.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/10/03 - Antonako1
        Added this header.

REMARKS
    Do NOT call any of these functions before PAGING_INIT()!
    Do NOT call any of these functions if you don't know what you're doing!

See README.md
---*/
#ifndef PAGING_H
#define PAGING_H

#include <STD/TYPEDEF.h>
#include <MEMORY/MEMORY.h>
#include <PROC/PROC.h>

#define PAGE_PRESENT    0b000000001
#define PAGE_READ_WRITE 0b000000010
#define PAGE_USER       0b000000100
#define PAGE_WRITETHRU  0b000001000
#define PAGE_CACHEDIS   0b000010000
#define PAGE_ACCESSED   0b000100000
#define PAGE_DIRTY      0b001000000
#define PAGE_4MB        0b100000000
#define PAGE_PRW        (PAGE_PRESENT | PAGE_READ_WRITE)

#define USER_PDE        (MEM_USER_SPACE_BASE >> 22) // 9
#define USER_PDE_END    (MEM_USER_SPACE_END_MIN >> 22) // 15

#define PAGE_ENTRIES 1024

// Kernel dynamic memory mapping area (for e.g. kmalloc physical pages)
#define KERNEL_VIRT_ALLOC_BASE 0xD0000000

BOOLEAN PAGING_INIT(VOID);
ADDR *get_page_directory(VOID);
void map_page(U32 *pd, U32 virt, U32 phys, U32 flags);
BOOLEAN unmap_page(U32 *pd, U32 virt);
// void map_process_page(U32 *pd, U32 virt, U32 phys, U32 flags);

void identity_map_range_with_offset(U32 *pd, U32 start, U32 end, U32 offset, U32 flags);
void identity_map_range(U32 *pd, U32 start, U32 end, U32 flags);

/*
Here are a set of functions,
these are just like KREQUEST_PAGE and KFREE_PAGE from PAGEFRAME

They had legacy code inside which is why they exist...
*/
VOIDPTR KALLOC_PAGE(VOID);
VOIDPTR KALLOC_PAGES(U32 numPages);
VOID KFREE_PAGE_GLOBAL(VOIDPTR virt);
VOID KFREE_PAGES_GLOBAL(VOIDPTR virt, U32 numPages);
#endif // PAGING_H