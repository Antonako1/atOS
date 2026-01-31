/*
READ ME @ PAGING.H
*/
#include <PAGING/PAGING.h>
#include <STD/MEM.h>
#include <STD/ASM.h>
#include <MEMORY/PAGEFRAME/PAGEFRAME.h>
#include <MEMORY/MEMORY.h>
#include <PAGING/PAGING.h>
#include <STD/STRING.h>
#include <VESA/VBE.h>
#include <ERROR/ERROR.h>
#include <PROC/PROC.h>
#include <E820/E820.h>
#include <RTOSKRNL_INTERNAL.h>


static ADDR *page_directory __attribute__((section(".data"))) = NULL;

ADDR *get_page_directory(VOID) {
    return page_directory;
}


void identity_map_range(U32 *pd, U32 start, U32 end, U32 flags) {
    // inclusive start, exclusive end; page-align
    U32 s = start & ~0xFFF;
    U32 e = (end + 0xFFF) & ~0xFFF;
    for (U32 addr = s; addr < e; addr += PAGE_SIZE) {
        map_page(pd, addr, addr, flags);
    }
}

void identity_map_range_with_offset(U32 *pd, U32 start, U32 end, U32 offset, U32 flags) {
    // inclusive start, exclusive end; page-align
    U32 s = start & ~0xFFF;
    U32 e = (end + 0xFFF) & ~0xFFF;
    for (U32 addr = s; addr < e; addr += PAGE_SIZE) {
        map_page(pd, addr + offset, addr, flags);
    }
}

// Load CR3 with page directory physical address
VOID load_page_directory(ADDR phys_addr) {
    ASM_VOLATILE("mov %0, %%cr3" : : "r"(phys_addr) : "memory");
}

// Enable paging by setting CR0.PG
VOID enable_paging(VOID) {
    U32 cr0;
    ASM_VOLATILE("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Set PG
    ASM_VOLATILE("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

BOOLEAN PAGING_INIT(void) {
    if (page_directory) return TRUE;

    page_directory = (ADDR *)KREQUEST_PAGE();
    panic_if(!page_directory, PANIC_TEXT("Failed to allocate page directory"), PANIC_OUT_OF_MEMORY);
    MEMZERO(page_directory, PAGE_SIZE);

    // Sanity checks on layout
    panic_if((MEM_RTOSKRNL_BASE & 0xFFF) || (MEM_RTOSKRNL_END & 0xFFF),
         PANIC_TEXT("Kernel range not page-aligned"), PANIC_INVALID_ARGUMENT);

    panic_if((MEM_KERNEL_HEAP_BASE & 0xFFF) || (MEM_KERNEL_HEAP_END & 0xFFF),
            PANIC_TEXT("Heap range not page-aligned"), PANIC_INVALID_ARGUMENT);

    panic_if((STACK_0_BASE & 0xFFF) || (STACK_0_END & 0xFFF),
            PANIC_TEXT("Stack range not page-aligned"), PANIC_INVALID_ARGUMENT);

    panic_if(MEM_RTOSKRNL_END > MEM_KERNEL_HEAP_BASE,
            PANIC_TEXT("Kernel overlaps heap"), PANIC_INVALID_STATE);

    panic_if(MEM_KERNEL_HEAP_END + PAGE_SIZE != STACK_0_BASE,
            PANIC_TEXT("Missing guard page below stack"), PANIC_INVALID_STATE);

    panic_if(STACK_0_END + PAGE_SIZE != MEM_FRAMEBUFFER_BASE,
            PANIC_TEXT("Missing guard page above stack"), PANIC_INVALID_STATE);

    // Guard page must exist between heap and stack base
    panic_if(STACK_0_BASE < MEM_KERNEL_HEAP_END + PAGE_SIZE,
            PANIC_TEXT("No space for stack guard page"), PANIC_INVALID_STATE);

    // Also ensure we can subtract one page safely
    panic_if(STACK_0_BASE < PAGE_SIZE,
            PANIC_TEXT("Stack base too low for guard page"), PANIC_INVALID_STATE);

    // Identity-map all memory reported by E820
    E820Info *e820_entries = GET_E820_INFO();
    panic_if(!e820_entries || e820_entries->RawEntryCount == 0,
             PANIC_TEXT("No E820 entries found"), PANIC_INVALID_STATE);
    for (U32 i = 0; i < e820_entries->RawEntryCount; i++) {
        E820_ENTRY *e = &e820_entries->RawEntries[i];
        if (e->Type == TYPE_E820_RAM) { // usable RAM
            identity_map_range(page_directory, e->BaseAddressLow, e->BaseAddressLow + e->LengthLow, PAGE_PRW);
        }
    }

    // NOTE: If you map new ranges, add to PROC.c identity_map_range calls too!
    identity_map_range(page_directory, MEM_LOW_RESERVED_BASE, MEM_LOW_RESERVED_END, PAGE_PRW);
    identity_map_range(page_directory, MEM_E820_BASE,          MEM_E820_END,          PAGE_PRW);
    identity_map_range(page_directory, MEM_VESA_BASE,          MEM_VESA_END,          PAGE_PRW);
    VBE_MODEINFO *vmi = GET_VBE_MODE();
    if(vmi && vmi->PhysBasePtr && vmi->XResolution && vmi->YResolution && vmi->BitsPerPixel) {
        U32 fb_start = vmi->PhysBasePtr;
        U32 fb_size = vmi->YResolution * vmi->BytesPerScanLine;
        U32 fb_end = fb_start + fb_size;
        identity_map_range(page_directory, fb_start, fb_end, PAGE_PRW);
    }
    identity_map_range(page_directory, MEM_RTOSKRNL_BASE,      MEM_RTOSKRNL_END,      PAGE_PRW);
    identity_map_range(page_directory, MEM_KERNEL_HEAP_BASE,   MEM_KERNEL_HEAP_END,   PAGE_PRW);
    identity_map_range(page_directory, STACK_0_BASE,           STACK_0_END,           PAGE_PRW);
    identity_map_range(page_directory, MEM_FRAMEBUFFER_BASE,   MEM_FRAMEBUFFER_END,   PAGE_PRW);

    identity_map_range(page_directory, MEM_USER_SPACE_BASE, MEM_USER_SPACE_END_MIN, PAGE_PRW);
    load_page_directory((ADDR)page_directory);
    enable_paging();
    return TRUE;
}



// Maps one 4KB virtual page to a physical page in a page directory.
// Assumes pd is the *virtual address* of the page directory.
// Flags should include PAGE_PRESENT | PAGE_RW | PAGE_USER as needed.
void map_page(U32 *pd, U32 virt, U32 phys, U32 flags) {
    // Calculate indices
    U32 pd_index = (virt >> 22) & 0x3FF;
    U32 pt_index = (virt >> 12) & 0x3FF;

    // Get or create the page table for this directory entry
    U32 pt_phys;

    if (pd[pd_index] & PAGE_PRESENT) {
        // Page table already exists
        pt_phys = pd[pd_index] & ~0xFFF;
    } else {
        // Allocate a new page for the page table
        pt_phys = (U32)KREQUEST_PAGE();
        panic_if(!pt_phys, PANIC_TEXT("Failed to allocate page table"), PANIC_OUT_OF_MEMORY);

        // Zero out the new page table (using virtual address mapping)
        U32 *pt_virt = (U32 *)phys_to_virt_pd(pt_phys);
        MEMZERO(pt_virt, PAGE_SIZE);

        // Add to page directory
        pd[pd_index] = (pt_phys & ~0xFFF) | (PAGE_PRESENT | PAGE_READ_WRITE);
    }

    // Map physical page to virtual address
    U32 *pt = (U32 *)phys_to_virt_pd(pd[pd_index] & ~0xFFF);
    pt[pt_index] = (phys & ~0xFFF) | (flags & 0xFFF);

    // Invalidate TLB for that page
    ASM_VOLATILE("invlpg (%0)" : : "r"(virt) : "memory");
}




BOOLEAN unmap_page(U32 *pd, U32 virt) {
    U32 pd_index = (virt >> 22) & 0x3FF;
    U32 pt_index = (virt >> 12) & 0x3FF;

    if (!(pd[pd_index] & PAGE_PRESENT)) {
        return FALSE; // Page table not present
    }

    U32 pt_phys = pd[pd_index] & ~0xFFF;
    U32 *pt = (U32 *)phys_to_virt_pd(pt_phys);

    if (!(pt[pt_index] & PAGE_PRESENT)) {
        return FALSE; // Page not mapped
    }

    KFREE(pt_phys);

    pt[pt_index] = 0; // Unmap the page

    ASM_VOLATILE("invlpg (%0)" : : "r"(virt) : "memory");
    return TRUE;
}


void map_process_page(U32 *pd, U32 virt, U32 phys, U32 flags) {
    U32 pd_index = (virt >> 22) & 0x3FF;
    U32 pt_index = (virt >> 12) & 0x3FF;

    U32 pt_phys;
    if (pd[pd_index] & PAGE_PRESENT) {
        pt_phys = pd[pd_index] & ~0xFFF;
    } else {
        pt_phys = (U32)KREQUEST_PAGE();
        panic_if(!pt_phys, PANIC_TEXT("Failed to allocate page table"), PANIC_OUT_OF_MEMORY);

        void *pt_virt = proc_phys_to_virt(pt_phys);
        MEMZERO(pt_virt, PAGE_SIZE);

        /* Ensure PD entry includes present, rw and user bits as appropriate */
        pd[pd_index] = (pt_phys & ~0xFFF) | PAGE_PRW;
    }

    U32 *pt = (U32 *)proc_phys_to_virt(pt_phys);
    /* Ensure PTE includes present bit and any requested flags (user/rw) */
    pt[pt_index] = (phys & ~0xFFF) | (flags & 0xFFF) | PAGE_PRESENT;

    ASM_VOLATILE("invlpg (%0)" : : "r"(virt) : "memory");
}
