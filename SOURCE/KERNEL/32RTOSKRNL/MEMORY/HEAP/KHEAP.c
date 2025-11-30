#include <MEMORY/HEAP/KHEAP.h>
#include <MEMORY/PAGEFRAME/PAGEFRAME.h>
#include <ERROR/ERROR.h>
#include <STD/MEM.h>
#include <MEMORY/MEMORY.h>
#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>
#include <STD/ASM.h>
#include <STD/ASSERT.h>
#include <PROC/PROC.h>

/*+++

Kernel Heap Management

---*/

#define KHEAP_ALIGN 8u
#define ALIGN_UP_U32(x, a) ( (((U32)(x)) + ((a) - 1u)) & ~((a) - 1u) )
// #define CUR_PID (get_current_tcb()->info.pid)
// Minimum user-allocatable tail after splitting (avoid tiny fragments)
// A new block requires a header and at least KHEAP_ALIGN bytes of payload.
#define MIN_SPLIT_TAIL (sizeof(KHeapBlock) + KHEAP_ALIGN)

static KHeap kernelHeap __attribute__((section(".data"))) = {0, 0, 0, NULL, NULL};
static U32 heap_initialized __attribute__((section(".data"))) = FALSE;
static U32 heap_init_pages __attribute__((section(".data"))) = 0;

static inline U8 *kheap_base_ptr(void) { return (U8*)kernelHeap.baseAddress; }
static inline U8 *kheap_end_ptr(void)  { return (U8*)kernelHeap.baseAddress + kernelHeap.totalSize; }

// Helper to check block validity using magic numbers.
// This is critical for detecting heap corruption early.
static void validate_block(KHeapBlock* block) {
    panic_if(block->magic0 != KHEAP_BLOCK_MAGIC0, "KHEAP block magic0 corruption", (U32)block);
    panic_if(block->magic1 != KHEAP_BLOCK_MAGIC1, "KHEAP block magic1 corruption", (U32)block);
    panic_if(block->magic2 != KHEAP_BLOCK_MAGIC2, "KHEAP block magic2 corruption", (U32)block);
}

BOOLEAN KHEAP_INIT(U32 pageNum) {
    if (heap_initialized) return TRUE;
    panic_if(pageNum == 0, "KHEAP_INIT with zero pages", 0);

    U32 heap_total_pages = KHEAP_MAX_SIZE_PAGES;
    if (pageNum > heap_total_pages) {
        SET_ERROR_CODE(ERROR_KHEAP_OUT_OF_MEMORY);
        return FALSE;
    }
    heap_init_pages = pageNum;

    // Make sure the region is available to us: turn RESERVED into LOCKED (allocated)
    KLOCK_PAGES(MEM_KERNEL_HEAP_BASE, pageNum);

    kernelHeap.baseAddress = (VOIDPTR)MEM_KERNEL_HEAP_BASE; // identity-mapped
    kernelHeap.totalSize   = pageNum * PAGE_SIZE;
    kernelHeap.usedSize    = 0;
    // freeSize is the total available payload space.
    kernelHeap.freeSize    = kernelHeap.totalSize - sizeof(KHeapBlock);
    kernelHeap.currentPtr  = kernelHeap.baseAddress;

    // Initialize the first free block
    KHeapBlock *block = (KHeapBlock*)kernelHeap.baseAddress;
    block->real_size = kernelHeap.totalSize - sizeof(KHeapBlock);
    block->size = 0;   // no user allocation yet
    block->free = 1;
    block->magic0 = KHEAP_BLOCK_MAGIC0;
    block->magic1 = KHEAP_BLOCK_MAGIC1;
    block->magic2 = KHEAP_BLOCK_MAGIC2;
    

    heap_initialized = TRUE;
    return TRUE;
}

BOOLEAN KHEAP_EXPAND(U32 additionalPages) {
    panic_if(!heap_initialized, "KHEAP_EXPAND on uninitialized heap", 0);
    if (additionalPages == 0) return FALSE;

    U32 heap_total_pages = (MEM_KERNEL_HEAP_END - MEM_KERNEL_HEAP_BASE) / PAGE_SIZE;
    if (heap_init_pages + additionalPages > heap_total_pages) {
        SET_ERROR_CODE(ERROR_KHEAP_OUT_OF_MEMORY);
        return FALSE;
    }

    // Lock new pages
    KLOCK_PAGES(MEM_KERNEL_HEAP_BASE + heap_init_pages * PAGE_SIZE, additionalPages);

    U32 oldTotalSize = kernelHeap.totalSize;
    U32 additionalSize = additionalPages * PAGE_SIZE;
    
    kernelHeap.totalSize += additionalSize;
    // New space adds a payload and subtracts a new header size
    kernelHeap.freeSize  += additionalSize - sizeof(KHeapBlock);
    heap_init_pages += additionalPages;

    // Create new free block at the old end
    KHeapBlock *newBlock = (KHeapBlock*)((U8*)kernelHeap.baseAddress + oldTotalSize);
    newBlock->real_size = additionalSize - sizeof(KHeapBlock);
    newBlock->size = 0;
    newBlock->free = 1;
    newBlock->magic0 = KHEAP_BLOCK_MAGIC0;
    newBlock->magic1 = KHEAP_BLOCK_MAGIC1;
    newBlock->magic2 = KHEAP_BLOCK_MAGIC2;

    // Try to merge with previous block if it was the last block and is free
    KHeapBlock *scan = (KHeapBlock*)kernelHeap.baseAddress;
    KHeapBlock *last = NULLPTR;

    while ((U8*)scan < (U8*)newBlock) {
        last = scan;
        U8 *nextAddr = (U8*)scan + sizeof(KHeapBlock) + scan->real_size;
        if (nextAddr >= (U8*)newBlock) break;
        scan = (KHeapBlock*)nextAddr;
    }

    if (last && last->free) {
        validate_block(last);
        // Merge the new block into the last one
        last->real_size += sizeof(KHeapBlock) + newBlock->real_size;
        
        // The header of the new block is now part of the free payload
        kernelHeap.freeSize += sizeof(KHeapBlock);
    }

    return TRUE;
}

KHeap* KHEAP_GET_INFO(VOID) {
    if (!heap_initialized) return NULLPTR;
    return &kernelHeap;
}

KHeapBlock* KHEAP_GET_X_BLOCK(U32 index) {
    if (!heap_initialized) return NULLPTR;

    U8 *heapEnd = kheap_end_ptr();
    KHeapBlock *block = (KHeapBlock*)kernelHeap.baseAddress;
    U32 currentIndex = 0;

    // Iterate through blocks safely
    while ((U8*)block + sizeof(KHeapBlock) <= heapEnd) {
        if (currentIndex == index)
            return block;

        U8 *nextAddr = (U8*)block + sizeof(KHeapBlock) + block->real_size;
        // Check if the next block would start beyond the heap
        if (nextAddr >= heapEnd) break;
        block = (KHeapBlock*)nextAddr;
        currentIndex++;
    }

    return NULLPTR;
}

/* ---------------- Fixed KMALLOC / KFREE family (32-bit safe) ---------------- */

/* KMALLOC: allocate payload of `size` bytes (rounded up to KHEAP_ALIGN) */
VOIDPTR KMALLOC(U32 size) {
    U32 aligned_size;
    U8 *heap_begin, *heap_end, *start_ptr;
    KHeapBlock *best_fit;

    panic_if(!heap_initialized, "KMALLOC on uninitialized heap", 0);
    panic_if(size == 0, "KMALLOC with size 0", 0);
    ASSERT(sizeof(KHeapBlock) % KHEAP_ALIGN == 0);

    aligned_size = ALIGN_UP_U32(size, KHEAP_ALIGN);

    /* Validate currentPtr before use; if invalid, reset to base */
    {
        U8* cp = (U8*)kernelHeap.currentPtr;
        if (cp < kheap_base_ptr() || cp >= kheap_end_ptr()) {
            kernelHeap.currentPtr = kernelHeap.baseAddress;
        } else {
            KHeapBlock *test = (KHeapBlock*)cp;
            if (test->magic0 != KHEAP_BLOCK_MAGIC0 ||
                test->magic1 != KHEAP_BLOCK_MAGIC1 ||
                test->magic2 != KHEAP_BLOCK_MAGIC2) {
                kernelHeap.currentPtr = kernelHeap.baseAddress;
            }
        }
    }

restart_search:;
    heap_begin = kheap_base_ptr();
    heap_end   = kheap_end_ptr();
    start_ptr  = (U8*)kernelHeap.currentPtr;
    best_fit   = NULLPTR;

    /* Pass 1: scan from currentPtr to heap end */
    {
        U8* scan_ptr = start_ptr;
        while (scan_ptr + sizeof(KHeapBlock) <= heap_end) {
            KHeapBlock *block = (KHeapBlock*)scan_ptr;
            U8 *next_ptr;

            /* Validate header magic (if corrupt, stop this pass) */
            if (block->magic0 != KHEAP_BLOCK_MAGIC0 ||
                block->magic1 != KHEAP_BLOCK_MAGIC1 ||
                block->magic2 != KHEAP_BLOCK_MAGIC2) {
                break;
            }

            /* Sanity: real_size must be > 0 and next header must fit */
            if (block->real_size == 0) break;
            next_ptr = scan_ptr + sizeof(KHeapBlock) + block->real_size;
            if (next_ptr > heap_end) break;
            // if (next_ptr + sizeof(KHeapBlock) > heap_end) break;
            if (block->free && block->real_size >= aligned_size) {
                best_fit = block;
                goto have_fit;
            }

            scan_ptr = next_ptr;
        }
    }

    /* Pass 2: scan from base to currentPtr */
    {
        U8* scan_ptr = heap_begin;
        while (scan_ptr + sizeof(KHeapBlock) <= start_ptr) {
            KHeapBlock *block = (KHeapBlock*)scan_ptr;
            U8 *next_ptr;

            /* Header must be valid */
            validate_block(block);

            if (block->free && block->real_size >= aligned_size) {
                best_fit = block;
                goto have_fit;
            }

            scan_ptr = next_ptr;
        }
    }

have_fit:
    if (best_fit) {
        KHeapBlock *block = best_fit;
        U32 original_payload = block->real_size;

        /* Decide whether to split the free block. We require room for
           a new header + minimum payload (KHEAP_ALIGN) after the allocated payload. */
        if (original_payload >= aligned_size + sizeof(KHeapBlock) + KHEAP_ALIGN) {
            /* Split block */
            U8 *new_hdr_ptr = (U8*)block + sizeof(KHeapBlock) + aligned_size;
            KHeapBlock *new_free_block = (KHeapBlock*) new_hdr_ptr;

            new_free_block->real_size = original_payload - aligned_size - sizeof(KHeapBlock);
            new_free_block->size = 0;
            new_free_block->free = 1;
            new_free_block->magic0 = KHEAP_BLOCK_MAGIC0;
            new_free_block->magic1 = KHEAP_BLOCK_MAGIC1;
            new_free_block->magic2 = KHEAP_BLOCK_MAGIC2;

            /* Shrink allocated block payload to exactly aligned_size */
            block->real_size = aligned_size;

            /* Accounting:
               - allocated payload (aligned_size) is removed from free payload
               - creation of a new header consumes sizeof(KHeapBlock) bytes of payload and is NOT counted in freeSize
               So freeSize must decrease by aligned_size + sizeof(KHeapBlock).
            */
            kernelHeap.freeSize -= (aligned_size + sizeof(KHeapBlock));
            kernelHeap.usedSize += aligned_size;
        } else {
            /* Take whole block */
            kernelHeap.freeSize -= original_payload;
            kernelHeap.usedSize += original_payload;
            /* block->real_size remains original_payload */
        }

        block->free = 0;
        block->size = size;

        /* Advance currentPtr to next header (wrap to base if at end) */
        {
            U8 *next_hdr = (U8*)block + sizeof(KHeapBlock) + block->real_size;
            kernelHeap.currentPtr = (next_hdr < heap_end) ? (VOIDPTR)next_hdr : kernelHeap.baseAddress;
        }

        /* Return pointer to payload area */
        return (VOIDPTR)((U8*)block + sizeof(KHeapBlock));
    }

    /* No suitable block found: try to expand the heap by enough pages and retry */
    {
        U32 needed = aligned_size + sizeof(KHeapBlock);
        U32 pages_to_add = (needed + PAGE_SIZE - 1u) / PAGE_SIZE;
        if (KHEAP_EXPAND(pages_to_add)) {
            goto restart_search;
        }
    }

    panic_if(TRUE, "KMALLOC: Kernel out of memory", size);
    return NULLPTR;
}

/* KFREE: free a payload pointer previously returned by KMALLOC */
VOID KFREE(VOIDPTR ptr) {
    KHeapBlock *current_block, *block_to_merge_from;
    KHeapBlock *prev_scanner, *next_block;

    panic_if(ptr == NULLPTR, "KFREE: attempt to free null pointer", 0);
    panic_if(!heap_initialized, "KFREE on uninitialized heap", 0);

    /* Recover header */
    current_block = (KHeapBlock*)((U8*)ptr - sizeof(KHeapBlock));


    /* Bounds & integrity checks */
    panic_if((U8*)current_block < kheap_base_ptr() || (U8*)current_block >= kheap_end_ptr(),
             "KFREE: pointer is out of heap bounds", (U32)ptr);
    validate_block(current_block);
    panic_if(current_block->free, "KFREE: double free detected", (U32)ptr);

    /* Mark free and adjust accounting (payload-only) */
    current_block->free = 1;
    kernelHeap.usedSize -= current_block->real_size;
    kernelHeap.freeSize += current_block->real_size;
    current_block->size = 0;

    block_to_merge_from = current_block;

    /* --- Coalesce with previous block if possible --- */
    prev_scanner = (KHeapBlock*)kheap_base_ptr();
    while ((U8*)prev_scanner + sizeof(KHeapBlock) <= (U8*)current_block) {
        KHeapBlock *candidate_next = (KHeapBlock*)((U8*)prev_scanner + sizeof(KHeapBlock) + prev_scanner->real_size);

        /* If candidate_next goes past current_block, heap is inconsistent -> stop */
        if ((U8*)candidate_next > (U8*)current_block) break;

        if (candidate_next == current_block) {
            /* Found previous block header */
            if (prev_scanner->free) {
                /* Merge backward: prev_scanner absorbs current_block */
                prev_scanner->real_size += sizeof(KHeapBlock) + current_block->real_size;
                kernelHeap.freeSize += sizeof(KHeapBlock); /* header reclaimed into free payload */
                block_to_merge_from = prev_scanner;

                /* Zero-out the old header area to make misuse obvious */
                MEMZERO(current_block, sizeof(KHeapBlock));
            }
            break;
        }

        /* advance */
        prev_scanner = candidate_next;
    }

    /* --- Coalesce forward if next block is free --- */
    next_block = (KHeapBlock*)((U8*)block_to_merge_from + sizeof(KHeapBlock) + block_to_merge_from->real_size);
    if ((U8*)next_block + sizeof(KHeapBlock) <= kheap_end_ptr()) {
        /* Check header validity before using */
        if (next_block->magic0 == KHEAP_BLOCK_MAGIC0 &&
            next_block->magic1 == KHEAP_BLOCK_MAGIC1 &&
            next_block->magic2 == KHEAP_BLOCK_MAGIC2) {

            if (next_block->free) {
                block_to_merge_from->real_size += sizeof(KHeapBlock) + next_block->real_size;
                kernelHeap.freeSize += sizeof(KHeapBlock); /* reclaimed header */
                MEMZERO(next_block, sizeof(KHeapBlock));
            }
        }
    }

    /* Optional: zero user payload (for security) -- commented out to avoid extra work in kernel
       MEMZERO((U8*)ptr, block_to_merge_from->size);
    */
}

/* KCALLOC: allocate and zero memory */
VOIDPTR KCALLOC(U32 num, U32 size) {
    /* Detect overflow in multiplication without 64-bit: use division test */
    U32 total = num * size;
    panic_if(size != 0 && total / size != num, "KCALLOC: integer overflow", total);

    VOIDPTR ptr = KMALLOC(total);
    if (ptr) MEMZERO(ptr, total);
    return ptr;
}

/* KREALLOC: reallocate (simple implementation) */
VOIDPTR KREALLOC(VOIDPTR addr, U32 newSize) {
    if (!addr) return KMALLOC(newSize);
    if (newSize == 0) {
        KFREE(addr);
        return NULLPTR;
    }

    KHeapBlock *block = (KHeapBlock*)((U8*)addr - sizeof(KHeapBlock));
    validate_block(block);

    /* If the new size fits into the current payload, keep it */
    if (newSize <= block->real_size) {
        block->size = newSize;
        return addr;
    }

    /* Otherwise allocate a new block and copy */
    VOIDPTR newPtr = KMALLOC(newSize);
    if (newPtr) {
        MEMCPY(newPtr, addr, block->size);
        KFREE(addr);
    }
    return newPtr;
}

/* KMALLOC_ALIGN / KFREE_ALIGN: aligned allocation helpers (store raw pointer before aligned pointer) */
VOIDPTR KMALLOC_ALIGN(U32 size, U32 alignment) {
    panic_if(!heap_initialized, "KMALLOC_ALIGN on uninitialized heap", 0);
    panic_if(size == 0, "KMALLOC_ALIGN with size 0", 0);
    panic_if((alignment & (alignment - 1)) != 0, "KMALLOC_ALIGN alignment not a power of two", alignment);

    /* Reserve extra space for alignment adjustment and to store original pointer */
    U32 total_size = size + alignment - 1 + sizeof(VOIDPTR);
    VOIDPTR raw_ptr = KMALLOC(total_size);
    if (!raw_ptr) return NULLPTR;

    /* Compute aligned pointer after room for stored original pointer */
    U32 start_addr = (U32)raw_ptr + (U32)sizeof(VOIDPTR);
    U32 aligned_addr = (start_addr + (alignment - 1)) & ~(alignment - 1);
    VOIDPTR aligned_ptr = (VOIDPTR)aligned_addr;

    /* Store the original pointer directly before aligned_ptr */
    VOIDPTR *storage = (VOIDPTR*)((U8*)aligned_ptr - sizeof(VOIDPTR));
    *storage = raw_ptr;

    return aligned_ptr;
}

VOID KFREE_ALIGN(VOIDPTR ptr) {
    panic_if(ptr == NULLPTR, "KFREE_ALIGN: attempt to free null pointer", 0);
    panic_if(!heap_initialized, "KFREE_ALIGN on uninitialized heap", 0);

    /* Retrieve the original raw pointer stored before the aligned pointer */
    VOIDPTR *storage = (VOIDPTR*)((U8*)ptr - sizeof(VOIDPTR));
    VOIDPTR raw_ptr = *storage;

    KFREE(raw_ptr);
}
