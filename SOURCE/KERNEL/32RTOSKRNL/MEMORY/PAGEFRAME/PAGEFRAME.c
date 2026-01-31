#include <MEMORY/PAGEFRAME/PAGEFRAME.h>
#include <MEMORY/PAGING/PAGING.h>
#include <MEMORY/E820/E820.h>
#include <MEMORY/MEMORY.h>
#include <STD/MEM.h>
#include <STD/BINARY.h>
#include <STD/ASM.h>
#include <MEMORY/BYTEMAP/BYTEMAP.h>
#include <VESA/VBE.h>
#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>
#include <STD/MATH.h>

#include <ERROR/ERROR.h>

#include <MEMORY/HEAP/KHEAP.h>

#define BYTE_TO_PAGE(x) ((x) / PAGE_SIZE)
static BYTEMAP pageFrameBytemap __attribute__((section(".data"))) = {0, 0};
static PAGEFRAME_INFO pageFrameInfo __attribute__((section(".data"))) = {0, 0, 0, 0, FALSE};
static U8 bytemapData[MAX_BYTEMAP_SIZE] __attribute__((section(".data"))) = {0}; // enough for largest physical memory
static U32 next_kernel_virt __attribute__((section(".data"))) = KERNEL_VIRT_ALLOC_BASE;
PAGEFRAME_INFO* GET_PAGEFRAME_INFO() {
    return &pageFrameInfo;
}
// BYTEMAP* GET_PAGEFRAME_BYTEMAP() {
//     return &pageFrameBytemap;
// }

BOOLEAN is_page_aligned(U32 addr) {
    return (addr & (PAGE_SIZE - 1)) == 0;
}


void set_range(U32 start, U32 end, PageState look_for, PageState set_to) {
    U32 startFrame = start / PAGE_SIZE;
    U32 endFrame   = (end + PAGE_SIZE - 1) / PAGE_SIZE; // round up

    if (endFrame > pageFrameBytemap.size)
        endFrame = pageFrameBytemap.size;

    for (U32 f = startFrame; f < endFrame; f++) {
        if (BYTEMAP_GET(&pageFrameBytemap, f) == look_for) {
            BYTEMAP_SET(&pageFrameBytemap, f, set_to);
        }
    }
}

// Create BYTEMAP based on E820 info
BOOLEAN CREATE_PAGEFRAME_BYTEMAP() {
    E820Info *e820 = GET_E820_INFO();
    if (e820->RawEntryCount == 0) return FALSE;

    U32 maxAddress = 0;
    for (U32 i = 0; i < e820->RawEntryCount; i++) {
        E820_ENTRY *entry = &e820->RawEntries[i];
        if (entry->Type == TYPE_E820_RAM) {
            U32 endAddress = entry->BaseAddressLow + entry->LengthLow;
            if (endAddress > maxAddress) maxAddress = endAddress;
        }
    }

    U32 totalPages = pages_from_bytes(maxAddress);

    if (totalPages > MAX_BYTEMAP_SIZE) return FALSE;

    BYTEMAP_CREATE(totalPages, bytemapData, &pageFrameBytemap);
    if (!pageFrameBytemap.data) return FALSE;

    // 1. Default all pages to RESERVED
    for (U32 f = 0; f < totalPages; f++) {
        BYTEMAP_SET(&pageFrameBytemap, f, PS_RESERVED);
    }

    // 2. Mark E820 RAM regions as FREE
    for (U32 i = 0; i < e820->RawEntryCount; i++) {
        E820_ENTRY *entry = &e820->RawEntries[i];
        if (entry->Type == TYPE_E820_RAM) {
            U32 startFrame = page_align_down(entry->BaseAddressLow) / PAGE_SIZE;
            U32 endFrame   = page_align_up(entry->BaseAddressLow + entry->LengthLow) / PAGE_SIZE;

            if (endFrame > pageFrameBytemap.size) {
                endFrame = pageFrameBytemap.size;
            }

            for (U32 f = startFrame; f < endFrame; f++) {
                BYTEMAP_SET(&pageFrameBytemap, f, PS_FREE);
            }
        }
    }

    // Reserve critical regions
    VESA_INFO *vesa = GET_VESA_INFO();
    VBE_MODEINFO *vbe = GET_VBE_MODE();
    panic_if(!vesa, PANIC_TEXT("VESA info not available"), PANIC_INVALID_STATE);
    panic_if(!vbe, PANIC_TEXT("VBE info not available"), PANIC_INVALID_STATE);
    panic_if(!vbe->PhysBasePtr, PANIC_TEXT("VBE framebuffer not available"), PANIC_INVALID_STATE);
    
    set_range(0x00000000, 0x000FFFFF, PS_FREE, PS_RESERVED); // below 1MB
    set_range((U32)MEM_RTOSKRNL_BASE, (U32)MEM_RTOSKRNL_END, PS_FREE, PS_RESERVED); // kernel itself
    set_range(E820_TABLE_PHYS, E820_TABLE_END, PS_FREE, PS_RESERVED);         // E820 table
    set_range(MEM_VESA_BASE, VBE_MODE_LOAD_ADDRESS_PHYS + sizeof(VBE_MODEINFO), PS_FREE, PS_RESERVED);
    set_range(vbe->PhysBasePtr, vbe->PhysBasePtr + (vbe->YResolution * vbe->BytesPerScanLineLinear), PS_FREE, PS_RESERVED); // VESA framebuffer
    set_range(MEM_FRAMEBUFFER_BASE, MEM_FRAMEBUFFER_END, PS_FREE, PS_RESERVED); // video buffer
    set_range(MEM_KERNEL_HEAP_BASE, MEM_KERNEL_HEAP_END, PS_FREE, PS_RESERVED); // kernel heap
    set_range(MEM_BIOS_BASE, MEM_BIOS_END, PS_FREE, PS_RESERVED);
    set_range(MEM_RESERVED_BASE, MEM_RESERVED_END, PS_FREE, PS_RESERVED); // reserved MMIO / firmware
    set_range(STACK_0_BASE - PAGE_SIZE, STACK_0_BASE, PS_FREE, PS_RESERVED); // stack guard page
    set_range(STACK_0_BASE, STACK_0_END, PS_FREE, PS_RESERVED); // stack itself
    set_range(STACK_0_GUARD_BELOW_BASE, STACK_0_GUARD_BELOW_END, PS_FREE, PS_RESERVED); // unmapped guard
    set_range(STACK_0_BASE, STACK_0_END, PS_FREE, PS_RESERVED); // stack frames
    set_range(STACK_0_GUARD_ABOVE_BASE, STACK_0_GUARD_ABOVE_END, PS_FREE, PS_RESERVED); // unmapped guard

    set_range(
        MEM_USER_SPACE_BASE, 
        MEM_USER_SPACE_END_MIN, 
        PS_FREE, PS_FUSER
    ); // user space reserved

    // Above 4GB is reserved (not handled), this also protects against overflow
    // of 32-bit virtual addresses in requests
    set_range(0xFFFFF000, 0xFFFFFFFF, PS_FREE, PS_RESERVED); // above 4GB
    return TRUE;
}

BOOLEAN CREATE_PAGEFRAME() {
    pageFrameInfo.freeMemory = 0;
    pageFrameInfo.usedMemory = 0;
    pageFrameInfo.reservedMemory = 0;
    pageFrameInfo.pIndex = 0;
    pageFrameInfo.initialized = FALSE;
    pageFrameInfo.size = pageFrameBytemap.size;


    U32 totalFrames = pageFrameBytemap.size;  // number of pages
    for (U32 i = 0; i < totalFrames; i++) {
        switch (BYTEMAP_GET(&pageFrameBytemap, i)) {
            case PS_FREE:      pageFrameInfo.freeMemory     += PAGE_SIZE; break;
            case PS_ALLOC:     pageFrameInfo.usedMemory     += PAGE_SIZE; break;
            case PS_RESERVED:  pageFrameInfo.reservedMemory += PAGE_SIZE; break;
            case PS_LOCKED:    pageFrameInfo.usedMemory     += PAGE_SIZE; break;
            case PS_FUSER:    pageFrameInfo.usedMemory     += PAGE_SIZE; break;
            case PS_LUSER:    pageFrameInfo.usedMemory     += PAGE_SIZE; break;
            case PS_AUSER:    pageFrameInfo.usedMemory     += PAGE_SIZE; break;
            case PS_RUSER:    pageFrameInfo.reservedMemory += PAGE_SIZE; break;
            default: break;
        }
    }
    return TRUE;
}

BOOLEAN PAGEFRAME_INIT() {
    if (pageFrameInfo.initialized) {
        return TRUE; // Already initialized
    }
    if(!CREATE_PAGEFRAME_BYTEMAP()) {
        return FALSE;
    }
    if(!CREATE_PAGEFRAME()) {
        return FALSE;
    }
    pageFrameInfo.initialized = TRUE;
    return TRUE;
}

ADDR GET_FREE_RAM() {
    return pageFrameInfo.freeMemory;
}

ADDR GET_USED_RAM() {
    return pageFrameInfo.usedMemory;
}

ADDR GET_RESERVED_RAM() {
    return pageFrameInfo.reservedMemory;
}


ADDR REQUEST_PAGE_RAW(U32 look_for, U32 set_to) {
    BYTEMAP *bm = &pageFrameBytemap;
    PAGEFRAME_INFO *info = &pageFrameInfo;

    for (U32 i = info->pIndex; i < bm->size; i++) {
        if (BYTEMAP_GET(bm, i) == look_for) {
            BYTEMAP_SET(bm, i, set_to);
            info->freeMemory -= PAGE_SIZE;
            info->usedMemory += PAGE_SIZE;
            info->pIndex = (i + 1) % bm->size;
            return i * PAGE_SIZE;
        }
    }

    for (U32 i = 0; i < info->pIndex; i++) {
        if (BYTEMAP_GET(bm, i) == look_for) {
            BYTEMAP_SET(bm, i, set_to);
            info->freeMemory -= PAGE_SIZE;
            info->usedMemory += PAGE_SIZE;
            info->pIndex = (i + 1) % bm->size;
            return i * PAGE_SIZE;
        }
    }

    return 0; // no free pages
}


ADDR REQUEST_PAGES_RAW(U32 numPages, U32 look_for, U32 set_to) {
    if (numPages == 0) return 0;
    BYTEMAP *bm = &pageFrameBytemap;
    PAGEFRAME_INFO *info = &pageFrameInfo;

    U32 totalFrames = bm->size;

    for (int pass = 0; pass < 2; pass++) {
        U32 start = (pass == 0) ? info->pIndex : 0;
        U32 end   = (pass == 0) ? totalFrames : info->pIndex;

        U32 consecutive = 0, startIndex = 0;
        for (U32 i = start; i < end; i++) {
            if (BYTEMAP_GET(bm, i) == look_for) {
                if (consecutive == 0) startIndex = i;
                consecutive++;
                if (consecutive == numPages) {
                    for (U32 j = startIndex; j < startIndex + numPages; j++) {
                        BYTEMAP_SET(bm, j, set_to);
                    }
                    info->freeMemory -= numPages * PAGE_SIZE;
                    info->usedMemory += numPages * PAGE_SIZE;
                    info->pIndex = (startIndex + numPages) % totalFrames;
                    return startIndex * PAGE_SIZE;
                }
            } else {
                // Not consecutive
                consecutive = 0;
            }
        }
    }
    return 0;
}


VOID FREE_PS_RAW(ADDR addr, U32 set_to) {
    BYTEMAP *bm = &pageFrameBytemap;
    PAGEFRAME_INFO *info = &pageFrameInfo;

    U32 pageIndex = addr / PAGE_SIZE;
    if (pageIndex >= bm->size) {
        return; // Out of bounds
    }
    PageState state = BYTEMAP_GET(bm, pageIndex);
    if (state == set_to) {
        SET_ERROR_CODE(ERROR_DOUBLE_PAGE_FREE);
        return; // Already free
    }
    if (state == PS_RESERVED || state == PS_LOCKED) {
        SET_ERROR_CODE(state == PS_LOCKED ? ERROR_FREEING_OF_LOCKED_PAGE
                                          : ERROR_FREEING_OF_RESERVED_PAGE);
        return; // Cannot free reserved or locked page
    }
    BYTEMAP_SET(bm, pageIndex, set_to); // Mark as free
    info->freeMemory += PAGE_SIZE;
    info->usedMemory -= PAGE_SIZE;
    if (pageIndex < info->pIndex) {
        info->pIndex = pageIndex; // Update pIndex for next search
    }
}

VOID RESET_PAGEINDEX() {
    pageFrameInfo.pIndex = 0;
}

VOID LOCK_PS_RAW(ADDR addr, U32 look_for, U32 set_to) {
    U32 pageIndex = addr / PAGE_SIZE;
    if (pageIndex >= pageFrameBytemap.size) {
        return; // Out of bounds
    }
    if (BYTEMAP_GET(&pageFrameBytemap, pageIndex) == look_for) {
        BYTEMAP_SET(&pageFrameBytemap, pageIndex, set_to);
        pageFrameInfo.freeMemory -= PAGE_SIZE;
        pageFrameInfo.usedMemory += PAGE_SIZE;
    }
}

VOID UNLOCK_PS_RAW(ADDR addr, U32 look_for, U32 set_to) {
    U32 pageIndex = addr / PAGE_SIZE;
    if (pageIndex >= pageFrameBytemap.size) {
        return; // Out of bounds
    }
    if (BYTEMAP_GET(&pageFrameBytemap, pageIndex) == look_for) {
        BYTEMAP_SET(&pageFrameBytemap, pageIndex, set_to);
        pageFrameInfo.freeMemory += PAGE_SIZE;
        pageFrameInfo.usedMemory -= PAGE_SIZE;
    }
}

VOID RESERVE_PS_RAW(ADDR addr, U32 look_for, U32 set_to) {
    U32 pageIndex = addr / PAGE_SIZE;
    if (pageIndex >= pageFrameBytemap.size) {
        return; // Out of bounds
    }
    if (BYTEMAP_GET(&pageFrameBytemap, pageIndex) == look_for) {
        BYTEMAP_SET(&pageFrameBytemap, pageIndex, set_to);
        pageFrameInfo.freeMemory -= PAGE_SIZE;
        pageFrameInfo.reservedMemory += PAGE_SIZE;
    }
}

VOID UNRESERVE_PS_RAW(ADDR addr, U32 look_for, U32 set_to) {
    U32 pageIndex = addr / PAGE_SIZE;
    if (pageIndex >= pageFrameBytemap.size) {
        return; // Out of bounds
    }
    if (BYTEMAP_GET(&pageFrameBytemap, pageIndex) == look_for) {
        BYTEMAP_SET(&pageFrameBytemap, pageIndex, set_to);
        pageFrameInfo.freeMemory += PAGE_SIZE;
        pageFrameInfo.reservedMemory -= PAGE_SIZE;
    }
}

ADDR KREQUEST_PAGES(U32 numPages) {
    return REQUEST_PAGES_RAW(numPages, PS_FREE, PS_ALLOC);
}

ADDR KREQUEST_PAGE() {
    return REQUEST_PAGE_RAW(PS_FREE, PS_ALLOC);
}
VOID KFREE_PAGE(ADDR addr) {
    FREE_PS_RAW(addr, PS_FREE);
}
VOID KFREE_PAGES(ADDR addr, U32 numPages) {
    for (U32 i = 0; i < numPages; i++) {
        KFREE_PAGE(addr + (i * PAGE_SIZE));
    }
}

ADDR KREQUEST_USER_PAGES(U32 numPages) {
    return REQUEST_PAGES_RAW(numPages, PS_FUSER, PS_AUSER);
}

ADDR KREQUEST_USER_PAGE() {
    return REQUEST_PAGE_RAW(PS_FUSER, PS_AUSER);
}
VOID KFREE_USER_PAGE(ADDR addr) {
    FREE_PS_RAW(addr, PS_FUSER);
}
VOID KFREE_USER_PAGES(ADDR addr, U32 numPages) {
    for (U32 i = 0; i < numPages; i++) {
        KFREE_USER_PAGE(addr + (i * PAGE_SIZE));
    }
}

VOID KLOCK_PAGE(ADDR addr) {
    LOCK_PS_RAW(addr, PS_FREE, PS_LOCKED);
}

VOID KLOCK_PAGES(ADDR addr, U32 numPages) {
    for (U32 i = 0; i < numPages; i++) {
        LOCK_PS_RAW(addr + (i * PAGE_SIZE), PS_FREE, PS_LOCKED);
    }

}
VOID KUNLOCK_PAGE(ADDR addr) {
    UNLOCK_PS_RAW(addr, PS_LOCKED, PS_FREE);
}
VOID KUNLOCK_PAGES(ADDR addr, U32 numPages) {
    for (U32 i = 0; i < numPages; i++) {
        UNLOCK_PS_RAW(addr + (i * PAGE_SIZE), PS_LOCKED, PS_FREE);
    }
}
VOID KRESERVE_PAGE(ADDR addr) {
    RESERVE_PS_RAW(addr, PS_FREE, PS_RESERVED);
}
VOID KRESERVE_PAGES(ADDR addr, U32 numPages) {
    for (U32 i = 0; i < numPages; i++) {
        KRESERVE_PAGE(addr + (i * PAGE_SIZE));
    }
}
VOID KUNRESERVE_PAGE(ADDR addr) {
    UNRESERVE_PS_RAW(addr, PS_RESERVED, PS_FREE);
}
VOID KUNRESERVE_PAGES(ADDR addr, U32 numPages) {
    for (U32 i = 0; i < numPages; i++) {
        UNRESERVE_PS_RAW(addr + (i * PAGE_SIZE), PS_RESERVED, PS_FREE);
    }
}


// Request a single page
VOIDPTR KALLOC_PAGE(void) {
    ADDR phys = KREQUEST_PAGE(); // allocate physical frame
    return (VOIDPTR)phys;        // already identity-mapped
}

// Request multiple contiguous pages
VOIDPTR KALLOC_PAGES(U32 numPages) {
    if (!numPages) return NULL;
    ADDR phys_start = KREQUEST_PAGES(numPages);
    return (VOIDPTR)phys_start; // already identity-mapped
}

// Free a single page
VOID KFREE_PAGE_GLOBAL(VOIDPTR addr) {
    KFREE_PAGE((ADDR)addr); // no need to unmap
}

// Free multiple pages
VOID KFREE_PAGES_GLOBAL(VOIDPTR addr, U32 numPages) {
    for (U32 i = 0; i < numPages; i++) {
        KFREE_PAGE((ADDR)addr + i * PAGE_SIZE);
    }
}
