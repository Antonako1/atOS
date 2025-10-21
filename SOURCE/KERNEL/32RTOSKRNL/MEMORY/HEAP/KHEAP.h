/*+++
    SOURCE/KERNEL/32RTOSKRNL/MEMORY/HEAP/KHEAP.h - Kernel Heap Management

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    Kernel heap memory allocation functions.
    This implementation uses a block-based allocation strategy with splitting
    and coalescing to manage memory efficiently.

AUTHORS
    Name(s)

REVISION HISTORY
    yyyy/mm/dd - Name(s)
        Description

REMARKS
    Additional remarks, if any.
---*/
#ifndef KHEAP_H
#define KHEAP_H
#include <STD/TYPEDEF.h>
#include <MEMORY/MEMORY.h>

/// @brief Allocates memory from the kernel heap.
/// @param size Size in bytes to allocate.
/// @return Pointer to the allocated memory, or NULL if allocation failed.
VOIDPTR KMALLOC(U32 size);

/// @brief Frees memory allocated from the kernel heap.
/// @param ptr Pointer to the memory to free.
/// @return None.
VOID KFREE(VOIDPTR ptr);

/// @brief Allocates zero-initialized memory from the kernel heap.
/// @param num Number of elements.
/// @param size Size of each element in bytes.
/// @return Pointer to the allocated memory, or NULL if allocation failed.
VOIDPTR KCALLOC(U32 num, U32 size);

/// @brief Reallocates memory in the kernel heap.
/// @param addr Pointer to the current memory block.
/// @param newSize New size of the memory block in bytes.
/// @return Pointer to new memory block, or NULL if allocation failed.
VOIDPTR KREALLOC(VOIDPTR addr, U32 newSize);

/// @brief Allocates memory from the kernel heap with specified alignment.
/// @param size Size in bytes to allocate.
/// @param alignment Alignment in bytes (must be a power of two).
/// @return Pointer to the allocated memory, or NULL if allocation failed.
VOIDPTR KMALLOC_ALIGN(U32 size, U32 alignment);

/// @brief Frees memory allocated with KMALLOC_ALIGN.
/// @param ptr Pointer to the memory to free.
/// @return None.
VOID KFREE_ALIGN(VOIDPTR ptr);

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

#define KHEAP_MAX_SIZE (MEM_KERNEL_HEAP_END - MEM_KERNEL_HEAP_BASE) // Maximum heap size in bytes
#define KHEAP_MAX_SIZE_PAGES (KHEAP_MAX_SIZE / PAGE_SIZE)
#define KHEAP_DEFAULT_SIZE_PAGES (KHEAP_MAX_SIZE_PAGES / 2) // Default initial heap size

/// @brief Initializes the kernel heap.
/// @param pageNum Number of pages to allocate for the heap.
/// @return TRUE if initialization succeeded, FALSE otherwise.
BOOLEAN KHEAP_INIT(U32 pageNum);

/// @brief Header for a memory block in the kernel heap.
/// Placed directly before the user-accessible memory region.
typedef struct ATTRIB_PACKED {
    U32 magic0;       // Magic value to validate block integrity
    U32 size;         // User-requested size (payload size)
    U32 magic1;       // Another magic value for extra validation
    U32 real_size;    // Allocated payload size (after alignment)
    BOOLEAN free;     // Flag indicating if the block is free
    U32 magic2;       // A third magic value
} KHeapBlock;

// Magic values help detect heap corruption, like buffer overflows or invalid frees.
#define KHEAP_BLOCK_MAGIC0 0xDEADBEEF
#define KHEAP_BLOCK_MAGIC1 0xFEEBDAED
#define KHEAP_BLOCK_MAGIC2 0xDABBADAF

/// @brief Structure holding information about the kernel heap.
typedef struct {
    U32 totalSize;      // Total size of the heap region in bytes
    U32 usedSize;       // Sum of real_size of all used blocks
    U32 freeSize;       // Sum of real_size of all free blocks
    VOIDPTR baseAddress;  // Start address of the heap
    VOIDPTR currentPtr;   // Pointer for the next-fit search start
} KHeap;

/// @brief Retrieves statistics and information about the kernel heap.
/// @return Pointer to the kernel's KHeap structure.
KHeap* KHEAP_GET_INFO(VOID);

/// @brief Retrieves a pointer to the header of the Nth block in the heap.
/// @param index The zero-based index of the block to retrieve.
/// @return Pointer to the KHeapBlock, or NULL if the index is out of bounds.
KHeapBlock* KHEAP_GET_X_BLOCK(U32 index);

/// @brief Expands the kernel heap by a given number of pages.
/// @param additionalPages The number of pages to add to the heap.
/// @return TRUE if expansion succeeded, FALSE otherwise.
BOOLEAN KHEAP_EXPAND(U32 additionalPages);

#endif // KHEAP_H
