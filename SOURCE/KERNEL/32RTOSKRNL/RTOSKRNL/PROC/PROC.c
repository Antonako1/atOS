// README: Please see top of MEMORY/PAGING/PAGING.c for paging overview
#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>
#include <DRIVERS/VIDEO/VBE.h>
#include <STD/ASM.h>
#include <MEMORY/PAGEFRAME/PAGEFRAME.h>
#include <MEMORY/PAGING/PAGING.h>
#include <STD/STRING.h>
#include <MEMORY/HEAP/KHEAP.h>
#include <STD/MEM.h>
#include <ERROR/ERROR.h>
#include <CPU/PIT/PIT.h>
#include <PROC/PROC.h> 
#include <CPU/ISR/ISR.h> // for regs struct
#include <CPU/PIC/PIC.h>
#include <STD/BINARY.h>
#include <DRIVERS/PS2/KEYBOARD.h>
#define EFLAGS_IF 0x0200
#define KDS 0x10
#define KCS 0x08

static TCB master_tcb __attribute__((section(".data"))) = {0};
static TCB *current_tcb __attribute__((section(".data"))) = &master_tcb;
static U32 next_pid __attribute__((section(".data"))) = 1; // start from 1 since 0 is master tcb
static U8 initialized __attribute__((section(".data"))) = FALSE;
static U32 proc_amount __attribute__((section(".data"))) = 0;
static TCB *last_tcb __attribute__((section(".data"))) = &master_tcb;

// Shell informs kernel what task is focused
static TCB *focused_task __attribute__((section(".data"))) = NULL;

static inline U32 PROC_READ_ESP(void) {
    U32 esp;
    ASM_VOLATILE("mov %%esp, %0" : "=r"(esp));
    return esp;
}
static inline U32 PROC_READ_EBP(void) {
    U32 ebp;
    ASM_VOLATILE("mov %%ebp, %0" : "=r"(ebp));
    return ebp;
}
static inline U32 PROC_READ_EIP(void) {
    U32 eip;
    ASM_VOLATILE("call 1f; 1: pop %0" : "=r"(eip));
    return eip;
}
static inline void PROC_SET_ESP(U32 esp) {
    ASM_VOLATILE("mov %0, %%esp" :: "r"(esp));
}
static inline void PROC_SET_EBP(U32 ebp) {
    ASM_VOLATILE("mov %0, %%ebp" :: "r"(ebp));
}
static inline void PROC_SET_EIP(U32 eip) {
    ASM_VOLATILE("jmp *%0" :: "r"(eip) : "memory");
}

U32 get_active_task_count(void) {
    return proc_amount;
}
TCB *get_last_tcb(void) {
    return last_tcb;
}
TCB *get_current_tcb(void) {
    return current_tcb;
}
TCB *get_master_tcb(void) {
    return &master_tcb;
}
U32 get_current_pid(void) {
    for(TCB *t = &master_tcb; ; t = t->next) {
        if(t == current_tcb) return t->info.pid;
        if(t->next == &master_tcb) break;
    }
    return master_tcb.info.pid;
}
U32 get_last_pid(void) {
    return next_pid - 1;
}
U32 get_next_pid(void) {
    // TODO_FIXME: handle pid wrap-around and reuse of freed pids, though not critical now
    if(next_pid == 0xFFFFFFFF) next_pid = 2; // wrap around, avoid 0. 1 is usually SHELL
    return next_pid++;
}

static void init_master_tcb(void) {
    master_tcb.info.pid   = 0;
    master_tcb.info.state = TCB_STATE_IMMORTAL;
    master_tcb.info.cpu_time = 0;
    master_tcb.info.num_switches = 0;
    STRNCPY((char *)master_tcb.info.name, "32RTOSKRNL", TASK_NAME_MAX_LEN - 1);
    
    master_tcb.next  = &master_tcb;        // circular list

    master_tcb.stack_phys_base = NULL;          // not used for master
    master_tcb.stack_vtop  = NULL;
    master_tcb.tf         = NULL;          // will be filled in by ISR on first tick
    master_tcb.pagedir    = (read_cr3()  & ~0xFFF);
    master_tcb.pagedir_phys = (read_cr3()  & ~0xFFF); // current page directory
    proc_amount = 1;

    master_tcb.parent = NULL; // no parent
    master_tcb.child_count = 0;
    master_tcb.framebuffer_mapped = TRUE; // kernel uses direct framebuffer
    master_tcb.framebuffer_phys = FRAMEBUFFER_ADDRESS;
    master_tcb.framebuffer_virt = FRAMEBUFFER_ADDRESS;

    focused_task = &master_tcb; // start with master focused
}

void uninitialize_multitasking(void) {
    initialized = FALSE;
}

void multitasking_shutdown(void) {
    if (!initialized) return;

    // Stop preemption
    ASM_VOLATILE("cli");

    // Return to master address space
    current_tcb = &master_tcb;
    write_cr3((U32)master_tcb.pagedir);

    // Optional: force resume of master tf if needed (usually already current)
    // Leave ISR-driven restore to pick master’s tf on next interrupt, or
    // explicitly clear initialized to prevent preemption.
    initialized = FALSE;

    ASM_VOLATILE("sti");  // if you want interrupts back without scheduling
}

void init_multitasking(void) {
    STI;
    if (initialized) return;
    init_master_tcb();

    set_next_task_cr3_val((U32)master_tcb.pagedir);
    // Simple allocation test
    VOIDPTR page = KREQUEST_USER_PAGE();
    VOIDPTR addr = phys_to_virt_pd(page);
    panic_if(!addr, PANIC_TEXT("PANIC: Unable to request page!"), PANIC_OUT_OF_MEMORY);
    MEMZERO(addr, PAGE_SIZE);
    KFREE_USER_PAGE(page);

    // current_tcb starts on master
    current_tcb = &master_tcb;

    // Now preemption can commence
    initialized = TRUE;
    PIT_INIT();
}

typedef struct {
    U32 bin_base, bin_end;
    U32 heap_base, heap_end;
    U32 stack_base, stack_end;
} user_layout_t;

typedef struct {
    U32 *virt; // kernel-mapped pointer. 1:1 mapping
    U32 phys;  // physical page. Same as virt
} PD_HANDLE;

static inline U32 round_up_pages(U32 bytes) {
    U32 pages = pages_from_bytes(bytes);
    return pages * PAGE_SIZE;
}

void copy_pde_range(U32 *dest_pd, U32 *src_pd, U32 start_vaddr, U32 end_vaddr) {
    // Calculate the starting and ending PDE indices, aligned to 4MB boundaries
    U32 pde_start = start_vaddr >> 22;
    U32 pde_end = (end_vaddr + 0x3FFFFF) >> 22; // Add 4MB-1 to include the last page

    if(pde_start > pde_end || pde_end >= PAGE_ENTRIES) return; // Invalid range
    if(pde_start > USER_PDE && pde_end < USER_PDE_END) return; // Skip user PDEs 

    for (U32 i = pde_start; i <= pde_end; ++i) {
        if (src_pd[i] & PAGE_PRESENT) {
            dest_pd[i] = src_pd[i];
        }
    }
}

void copy_kernel_pdes_with_offset(U32 *new_pd, U32 *kernel_pd) {
    for(U32 i = 0; i < PAGE_ENTRIES; i++) {
        // Skip user space PDEs
        if(i >= USER_PDE && i <= USER_PDE_END) continue;
        if(kernel_pd[i] & PAGE_PRESENT) {
            new_pd[i] = kernel_pd[i];
        }
    }

    // VBE_MODEINFO *vmi = GET_VBE_MODE();
    
    // // Copy the specific kernel PDE ranges.
    // copy_pde_range(new_pd, kernel_pd, MEM_LOW_RESERVED_BASE, MEM_LOW_RESERVED_END);
    // copy_pde_range(new_pd, kernel_pd, MEM_E820_BASE, MEM_E820_END);
    // copy_pde_range(new_pd, kernel_pd, MEM_VESA_BASE, MEM_VESA_END);
    
    // if(vmi && vmi->PhysBasePtr) {
    //     U32 fb_start = vmi->PhysBasePtr;
    //     U32 fb_size = vmi->YResolution * vmi->BytesPerScanLine;
    //     U32 fb_end = fb_start + fb_size;
    //     copy_pde_range(new_pd, kernel_pd, fb_start, fb_end);
    // }
        
    // copy_pde_range(new_pd, kernel_pd, MEM_RTOSKRNL_BASE, MEM_RTOSKRNL_END);
    // copy_pde_range(new_pd, kernel_pd, MEM_KERNEL_HEAP_BASE, MEM_KERNEL_HEAP_END);
    // copy_pde_range(new_pd, kernel_pd, STACK_0_BASE, STACK_0_END);
    // copy_pde_range(new_pd, kernel_pd, MEM_FRAMEBUFFER_BASE, MEM_FRAMEBUFFER_END);
}
    
PD_HANDLE create_process_pagedir(void) {
    VOIDPTR new_pd_phys = (VOIDPTR)KREQUEST_USER_PAGE();
    if (!new_pd_phys) return (PD_HANDLE){0,0};

    MEMZERO(new_pd_phys, PAGE_SIZE);

    // Virtual starting point of maps is at the same position as USER_BINARY_VADDR
    return (PD_HANDLE){ .virt = USER_BINARY_VADDR, .phys = (U32)new_pd_phys };
}

void destroy_process_pagedir(U32 *pd) {
    if (!pd) return;

    // Loop through all user page directory entries (before kernel PDEs)
    #warning TODO: optimize by tracking allocated pages in TCB

    // Free the page directory itself
    KFREE_PAGE((VOIDPTR)pd);
}

/*
Data is now in order:
binary
heap
stack
*/
BOOLEAN compute_user_layout(VOIDPTR phys_addr, U32 binary_size, U32 heap_size, U32 stack_size, user_layout_t *out) {
    if (!out || !phys_addr) return FALSE;

    // Align all regions to page boundaries
    U32 bin_len   = round_up_pages(binary_size);
    U32 heap_len  = round_up_pages(heap_size);
    U32 stack_len = round_up_pages(stack_size);
    
    // Basic sanity
    if (bin_len == 0 || heap_len == 0 || stack_len == 0) return FALSE;
    
    // Compute bases
    U32 bin_base   = (U32)phys_addr;
    U32 heap_base  = bin_base + bin_len;
    U32 stack_base = heap_base + heap_len;
    
    // Check overflow/wrap and kernel boundary (optional)
    // Ensure no wrap-around
    if (heap_base < bin_base) return FALSE;
    if (stack_base < heap_base) return FALSE;

    out->bin_base   = bin_base;
    out->bin_end    = bin_base + bin_len;
    panic_if(!is_page_aligned(out->bin_base) || !is_page_aligned(out->bin_end),
         PANIC_TEXT("Binary region not page-aligned"), PANIC_INVALID_ARGUMENT);
    out->heap_base  = heap_base;
    out->heap_end   = heap_base + heap_len;
    panic_if(!is_page_aligned(out->heap_base) || !is_page_aligned(out->heap_end),
         PANIC_TEXT("Heap region not page-aligned"), PANIC_INVALID_ARGUMENT);
    
    out->stack_base = stack_base;
    out->stack_end  = stack_base + stack_len;
    panic_if(!is_page_aligned(out->stack_base) || !is_page_aligned(out->stack_end),
         PANIC_TEXT("Stack region not page-aligned"), PANIC_INVALID_ARGUMENT);

    return TRUE;
}

// #define MAP_IN_FUNCTIONS

static BOOLEAN map_user_binary_at(VOIDPTR pages, U32 page_count, user_layout_t *layout,
                                  U32 *pd, U32 vbase, U8 *binary_data, U32 binary_size) {
    if (!pages || !layout || !pd) return FALSE;
    if (page_count == 0) return FALSE;
    if (layout->bin_base == 0 || layout->bin_end == 0) return FALSE;
    if (vbase == 0) return FALSE;
    if (binary_size == 0) return FALSE;
    if (binary_size > (layout->bin_end - layout->bin_base)) return FALSE;
    
    for (U32 i = 0; i < page_count; i++) {
        U32 phys = (U32)pages + (i * PAGE_SIZE);
        
        U32 offset = i * PAGE_SIZE;
        U32 copy_size = (i == page_count - 1) ? (binary_size - offset) : PAGE_SIZE;
        MEMZERO(phys, PAGE_SIZE);
        if (copy_size > 0 && binary_data) {
            MEMCPY(phys, binary_data + offset, copy_size);
        }
        #ifdef MAP_IN_FUNCTIONS
        U32 vaddr = vbase + (i * PAGE_SIZE);
        map_page(pd, vaddr, phys, PAGE_PRW);
        #endif
    }
    return TRUE;
}


static BOOLEAN map_user_heap_at(VOIDPTR pages, U32 heap_pages, user_layout_t *layout, 
                                U32 *pd, U32 vbase, U32 heap_size) {
    if (!pages || !layout || !pd) return FALSE;
    if (!vbase || heap_size == 0) return FALSE;
    if (layout->heap_base == 0 || layout->heap_end == 0) return FALSE;
    if (heap_size > (layout->heap_end - layout->heap_base)) return FALSE;

    U32 start_page = (layout->heap_base - layout->bin_base) / PAGE_SIZE;

    for (U32 i = 0; i < heap_pages; i++) {
        VOIDPTR phys = (VOIDPTR)((U32)pages + (start_page + i) * PAGE_SIZE);
        MEMZERO((U32)phys, PAGE_SIZE);
        #ifdef MAP_IN_FUNCTIONS
        U32 vaddr = vbase + i * PAGE_SIZE;
        map_page(pd, vaddr, (U32)phys, PAGE_PRW);
        #endif
    }
    return TRUE;
}

static BOOLEAN map_user_stack_at(VOIDPTR pages, U32 stack_pages, user_layout_t *layout, 
                                U32 *pd, U32 vbase, U32 stack_size, TCB *proc) {
    if (!pages || !layout || !pd || !proc) return FALSE;
    if (!vbase || stack_size == 0) return FALSE;
    if (layout->stack_base == 0 || layout->stack_end == 0) return FALSE;
    if (stack_size > (layout->stack_end - layout->stack_base)) return FALSE;
    U32 start_page = (layout->stack_base - layout->bin_base) / PAGE_SIZE;
    for (U32 i = 0; i < stack_pages; i++) {
        VOIDPTR phys = (VOIDPTR)((U32)pages + (start_page + i) * PAGE_SIZE);
        MEMZERO((U32)phys, PAGE_SIZE);
        // MEMSET((U32)phys, 0xCC, PAGE_SIZE); // fill with int3 for easier debugging
        #ifdef MAP_IN_FUNCTIONS
        U32 vaddr = vbase + i * PAGE_SIZE;
        map_page(pd, vaddr, (U32)phys, PAGE_PRW);
        #endif
    }
    // Save stack top (as virtual address)
    proc->stack_phys_base = (U32 *)layout->stack_base; // physical base
    proc->stack_vtop = (U32 *)(vbase + stack_pages * PAGE_SIZE); // virtual top of stack seen by user process

    return TRUE;
}

void init_task_context(TCB *tcb, void (*entry)(void), U32 stack_size, U32 initial_state) {
    // Place the trap frame at the top of the stack
    // Stack grows down, so we subtract sizeof(TrapFrame) from the top of the stack
    // Stack top (virtual) is tcb->stack_vtop.
    // Stack base (physical) is tcb->stack_phys_base.
    // Both addresses are already set.
    U32 tf_phys_addr = (U32)tcb->stack_phys_base + 
                        (pages_from_bytes(stack_size) * PAGE_SIZE) - 
                        sizeof(TrapFrame);

    // Kernel pointer. Is already mapped and valid, pointer to physical address
    TrapFrame *k_tf = (TrapFrame *)(tf_phys_addr);

    // Initialize the trap frame
    MEMZERO(k_tf, sizeof(TrapFrame));

    k_tf->seg.ds = KDS; k_tf->seg.fs = KDS; k_tf->seg.es = KDS; k_tf->seg.gs = KDS;

    k_tf->gpr.edi = k_tf->gpr.esi = k_tf->gpr.ebp = k_tf->gpr.esp = 0;
    k_tf->gpr.ebx = k_tf->gpr.edx = k_tf->gpr.ecx = k_tf->gpr.eax = 0;

    k_tf->cpu.eip = (U32)entry;
    k_tf->cpu.cs = KCS;          // kernel CS
    k_tf->cpu.eflags = EFLAGS_IF;


    // Virtual TF pointer seen after CR3 switch:
    // This is the virtual address of the trap frame in the user process's address space
    // Already calculated as top of stack - sizeof(TrapFrame)
    tcb->tf = (TrapFrame *)((U32)tcb->stack_vtop - sizeof(TrapFrame));

    // Set initial state
    U8 IS_TCB_STATE_INACTIVE = IS_FLAG_SET(initial_state, TCB_STATE_INACTIVE);
    U8 IS_TCB_STATE_ACTIVE = IS_FLAG_SET(initial_state, TCB_STATE_ACTIVE);
    U8 IS_TCB_STATE_WAITING = IS_FLAG_SET(initial_state, TCB_STATE_WAITING);
    U8 IS_TCB_STATE_IMMORTAL = IS_FLAG_SET(initial_state, TCB_STATE_IMMORTAL);
    U8 IS_TCB_STATE_ZOMBIE = IS_FLAG_SET(initial_state, TCB_STATE_ZOMBIE);
    U8 IS_TCB_STATE_SLEEPING = IS_FLAG_SET(initial_state, TCB_STATE_SLEEPING);

    U8 IS_TCB_STATE_INFO_CHILD_PROC_HANDLER = IS_FLAG_SET(initial_state, TCB_STATE_INFO_CHILD_PROC_HANDLER);
    tcb->info.state = initial_state;
    if (IS_TCB_STATE_INFO_CHILD_PROC_HANDLER) {
        tcb->info.state_info = TCB_STATE_INFO_CHILD_PROC_HANDLER;
    } else {
        tcb->info.state_info = 0;
    }

    if(IS_TCB_STATE_INACTIVE) tcb->info.state = TCB_STATE_INACTIVE;
    else if(IS_TCB_STATE_ACTIVE) tcb->info.state = TCB_STATE_ACTIVE;
    else if(IS_TCB_STATE_WAITING) tcb->info.state = TCB_STATE_WAITING;
    else if(IS_TCB_STATE_IMMORTAL) tcb->info.state = TCB_STATE_IMMORTAL;
    else if(IS_TCB_STATE_ZOMBIE) tcb->info.state = TCB_STATE_ZOMBIE;
    else if(IS_TCB_STATE_SLEEPING) tcb->info.state = TCB_STATE_SLEEPING;
    else tcb->info.state = TCB_STATE_INACTIVE; // default to inactive if none specified
}


U32 *setup_user_process(TCB *proc, U8 *binary_data, U32 bin_size, U32 heap_size, U32 stack_size, U32 initial_state) {
    // Create page directory
    PD_HANDLE pdh = create_process_pagedir();
    if (!pdh.virt) return NULL;
    proc->pagedir = pdh.virt; // Virtual pointer to page directory, used by the process
    proc->pagedir_phys = pdh.phys; // Physical address of page directory, loaded into CR3 on context switch
    
    // Now we map all of kernel pages into the new page directory
    VOIDPTR kernel_pd_raw = get_page_directory();
    if(!kernel_pd_raw) {
        destroy_process_pagedir(proc->pagedir_phys);
        return NULL;
    }


    // Compute memory layout
    U32 bin_pages = pages_from_bytes(bin_size);
    U32 heap_pages = pages_from_bytes(heap_size);
    U32 stack_pages = pages_from_bytes(stack_size);
    VBE_MODEINFO* mode = GET_VBE_MODE();
    U32 framebuffer_sz = mode->BytesPerScanLineLinear * mode->YResolution;
    U32 framebuffer_pages = pages_from_bytes(framebuffer_sz);
    if (bin_pages == 0 || heap_pages == 0 || stack_pages == 0 || framebuffer_pages == 0) {
        destroy_process_pagedir(proc->pagedir_phys);
        return NULL;
    }
    framebuffer_pages++; // one page padding between stack
    proc->binary_size = bin_size;
    proc->binary_pages = bin_pages;
    proc->heap_size = heap_size;
    proc->heap_pages = heap_pages;
    proc->stack_size = stack_size;
    proc->stack_pages = stack_pages;
    proc->framebuffer_pages = framebuffer_pages;
    U32 amount_of_pages_needed = bin_pages + heap_pages + stack_pages + framebuffer_pages;

    VOIDPTR pages = KREQUEST_USER_PAGES(amount_of_pages_needed);
    if (!pages) {
        destroy_process_pagedir(proc->pagedir_phys);
        return NULL;
    }
    
    proc->pages = pages;
    proc->page_count = amount_of_pages_needed;

    user_layout_t layout;
    if (!compute_user_layout(pages, bin_size, heap_size, stack_size, &layout)) return NULL;
    // Now we have:
    // layout.bin_base, layout.bin_end
    // layout.heap_base, layout.heap_end
    // layout.stack_base, layout.stack_end
    //
    // All of these are presented as physical addresses and must be mapped into the new page directory
    
    // Copy kernel PDEs
    copy_kernel_pdes_with_offset(proc->pagedir_phys, kernel_pd_raw);

    
    // Map binary
    if (!map_user_binary_at(pages, bin_pages, &layout, proc->pagedir_phys, USER_BINARY_VADDR, binary_data, bin_size
    )) {
        destroy_process_pagedir(proc->pagedir_phys);
        return NULL;
    }

    // Map heap. Heap is right after binary
    U32 heap_vaddr = USER_BINARY_VADDR + (layout.heap_base - layout.bin_end);
    if(!map_user_heap_at(pages, heap_pages, &layout, proc->pagedir_phys, heap_vaddr, heap_size)) {
        destroy_process_pagedir(proc->pagedir_phys);
        return NULL;
    }
    // Map stack. Stack is right after heap
    U32 stack_vaddr = USER_BINARY_VADDR + (layout.stack_base - layout.bin_end);
    if(!map_user_stack_at(pages, stack_pages, &layout, proc->pagedir_phys, stack_vaddr, stack_size, proc)) {
        destroy_process_pagedir(proc->pagedir_phys);
        return NULL;
    }

    // Map pages ONLY after all physical pages are allocated and binary is copied
    #ifndef MAP_IN_FUNCTIONS
    for(U32 i = 0; i < amount_of_pages_needed; i++) {
        U32 phys = (U32)pages + (i * PAGE_SIZE);
        U32 vaddr = USER_BINARY_VADDR + (( (U32)phys - layout.bin_base));
        map_page(proc->pagedir_phys, vaddr, phys, PAGE_PRW);
    }
    #endif

    // +1 is to adjust for padding between stack and framebuffer
    proc->framebuffer_phys = (VOIDPTR)( (U32)pages + ( (bin_pages + heap_pages + stack_pages + 1) * PAGE_SIZE) );
    proc->framebuffer_virt = (VOIDPTR)USER_BINARY_VADDR + ( (bin_pages + heap_pages + stack_pages + 1) * PAGE_SIZE);
    proc->framebuffer_mapped = FALSE; // Flagged as not mapped yet. User process must request to draw to framebuffer

    // copy parent's framebuffer into virt
    if(proc->parent != NULL) {
        MEMCPY_OPT(proc->framebuffer_phys, proc->parent->framebuffer_phys, framebuffer_sz);
    }

    // Initialize trap frame
    init_task_context(proc, (void (*)(void))USER_BINARY_VADDR, stack_size, initial_state);
    
    return proc->pagedir_phys;
}

TCB *get_tcb_by_pid(U32 pid) {
    if (pid == 0) return &master_tcb;
    TCB *master = &master_tcb;
    TCB *curr = master->next;
    while(curr != master) {
        if(curr->info.pid == pid) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

TCB *get_tcb_by_name(U8 *name) {
    if (!name) return NULL;
    TCB *master = &master_tcb;
    TCB *curr = master->next;
    while(curr != master) {
        if(STRNCMP((char *)curr->info.name, (char *)name, TASK_NAME_MAX_LEN) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void add_tcb_to_scheduler(TCB *new_tcb) {
    panic_if(!new_tcb, PANIC_TEXT("Cannot add null TCB to scheduler!"), PANIC_INVALID_ARGUMENT);
    
    if (new_tcb->info.state == TCB_STATE_ACTIVE) {
        proc_amount += 1;
    }
    
    // Insert after master into the circular list
    TCB *master = &master_tcb;

    if (master->next == master) {
        // Only master exists
        master->next = new_tcb;
        new_tcb->next = master;
        return;
    }

    // Find last node (whose next is master)
    TCB *last = master;
    while (last->next != master) last = last->next;

    last->next = new_tcb;
    new_tcb->next = master;
}

// Helper: translate a virtual stack vaddr -> physical pointer into stack region
static inline VOID *stack_phys_ptr(TCB *proc, U32 phys_offset) {
    return (VOID *)((U8 *)proc->stack_phys_base + phys_offset);
}


BOOLEAN setup_user_stack_args(TCB *proc, U32 argc, U8 **argv) {
    if (!proc) return FALSE;
    if (argc > 1024) return FALSE;
    if (argc > 0 && !argv) return FALSE;

    U32 stack_size_bytes = proc->stack_pages * PAGE_SIZE;
    U32 tf_offset = stack_size_bytes - sizeof(TrapFrame); // trapframe at top of stack
    U32 cur_offset = 0;

    // Compute total string size
    U32 total_str_bytes = 0;
    for (U32 i = 0; i < argc; i++) {
        if (!argv[i]) return FALSE;
        total_str_bytes += STRLEN(argv[i]) + 1;
    }

    // Space for argv pointers
    U32 ptrs_bytes = (argc + 1) * sizeof(U32);
    U32 argc_bytes = sizeof(U32);

    // Align all regions
    U32 align = 4;
    U32 strings_start = tf_offset - total_str_bytes;
    strings_start &= ~(align - 1);

    U32 ptrs_start = strings_start - ptrs_bytes;
    ptrs_start &= ~(align - 1);

    U32 argc_offset = ptrs_start - argc_bytes;
    argc_offset &= ~(align - 1);

    // Check stack overflow
    if (argc_offset < cur_offset) return FALSE;

    // --- Copy strings into stack ---
    U32 str_offset = strings_start;
    for (U32 i = 0; i < argc; i++) {
        U32 len = STRLEN(argv[i]) + 1;
        MEMCPY(stack_phys_ptr(proc, str_offset), argv[i], len);
        str_offset += len;
    }

    // --- Write argv pointers ---
    U32 ptr_offset = ptrs_start;
    str_offset = strings_start;
    for (U32 i = 0; i < argc; i++) {
        *(U32 *)stack_phys_ptr(proc, ptr_offset) = str_offset; // offset in physical stack
        ptr_offset += sizeof(U32);
        str_offset += STRLEN(argv[i]) + 1;
    }

    // Terminating NULL
    *(U32 *)stack_phys_ptr(proc, ptr_offset) = 0;

    // Write argc
    *(U32 *)stack_phys_ptr(proc, argc_offset) = argc;

    // Update trapframe ESP (physical stack only)
    TrapFrame *k_tf = (TrapFrame *)stack_phys_ptr(proc, tf_offset);
    k_tf->gpr.esp = argc_offset;

    // Update TCB virtual trapframe pointer (optional, for scheduling)
    if (proc->tf) proc->tf->gpr.esp = argc_offset;

    return TRUE;
}


BOOLEAN RUN_BINARY(
    U8 *proc_name, 
    VOIDPTR file, 
    U32 bin_size, 
    U32 heap_size, 
    U32 stack_size, 
    U32 initial_state, 
    U32 parent_pid,
    PPU8 argv,
    U32 argc
) {
    KDEBUG_PUTS("[proc] RUN_BINARY enter name=\"");
    KDEBUG_PUTS(proc_name);
    KDEBUG_PUTS("\" size=0x"); 
    KDEBUG_HEX32(bin_size); 
    KDEBUG_PUTS(" heap=0x"); 
    KDEBUG_HEX32(heap_size); 
    KDEBUG_PUTS(" stack=0x"); 
    KDEBUG_HEX32(stack_size); 
    KDEBUG_PUTS("\n");

    if (!file || !proc_name) return FALSE;
    if (!initialized) return FALSE;
    if (bin_size == 0 || bin_size > MAX_USER_BINARY_SIZE) return FALSE;
    if (proc_amount >= MAX_PROC_AMOUNT) return FALSE;

    TCB *new_proc = KMALLOC(sizeof(TCB));
    panic_if(!new_proc, "Unable to allocate memory for TCB!", PANIC_OUT_OF_MEMORY);
    MEMZERO(new_proc, sizeof(TCB));

    if (parent_pid != U32_MAX)
        new_proc->parent = get_tcb_by_pid(parent_pid);

    KDEBUG_PUTS("[proc] setup_user_process...\n");
    panic_if(!setup_user_process(new_proc, (U8 *)file, bin_size, heap_size, stack_size, initial_state),
             PANIC_TEXT("Failed to set up user process"), PANIC_OUT_OF_MEMORY);
    KDEBUG_PUTS("[proc] setup_user_process OK\n");

    new_proc->info.pid = get_next_pid();
    STRNCPY((char *)new_proc->info.name, (char *)proc_name, TASK_NAME_MAX_LEN);
    new_proc->info.name[TASK_NAME_MAX_LEN - 1] = '\0';

    add_tcb_to_scheduler(new_proc);
    KDEBUG_PUTS("[proc] added to scheduler pid="); 
    KDEBUG_HEX32(new_proc->info.pid); 
    KDEBUG_PUTS("\n[proc] Stack at "); 
    KDEBUG_HEX32(new_proc->stack_phys_base); 
    KDEBUG_PUTS("\n");

    // ---- Setup arguments if provided ----
    if (argc > 0 && argv != NULLPTR) {
        if (!setup_user_stack_args(new_proc, argc, argv)) {
            KDEBUG_PUTS("[proc] Failed to setup argv for process\n");
            KILL_PROCESS(new_proc->info.pid);
            return FALSE;
        }
    } else {
        // If no args, set default ESP below trapframe
        U32 stack_vtop = (U32)new_proc->stack_vtop;
        U32 tf_vaddr = stack_vtop - sizeof(TrapFrame);
        TrapFrame *k_tf = (TrapFrame *)((U32)new_proc->stack_phys_base + (tf_vaddr - (stack_vtop - new_proc->stack_pages * PAGE_SIZE)));
        k_tf->gpr.esp = tf_vaddr; // empty stack below trapframe
    }

    return TRUE;
}


void remove_tcb_from_scheduler(TCB *tcb) {
    if (!tcb || tcb->info.state == TCB_STATE_IMMORTAL) return;

    TCB *prev = &master_tcb;
    TCB *curr = master_tcb.next;
    while (curr != &master_tcb) {
        if (curr == tcb) {
            prev->next = curr->next;
            if (tcb->info.state == TCB_STATE_ACTIVE) {
                proc_amount--;
            }
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}


void KILL_PROCESS(U32 pid) {
    if (pid == 0) return; // cannot kill master
    TCB *master = get_master_tcb();
    TCB *prev = master;
    TCB *curr = master->next;

    while(curr != master) {
        if(curr->info.pid == pid) {
            // Found the process to kill
            prev->next = curr->next; // remove from list

            // Free resources
            if(curr->stack_phys_base) {
                KFREE_PAGE(curr->stack_phys_base);
            }
            if(curr->pagedir) {
                // Free page directory and tables (not implemented here)
                // You would need to walk the page directory and free all user pages and tables
                destroy_process_pagedir(curr->pagedir);
            }
            remove_tcb_from_scheduler(curr);
            KFREE_USER_PAGES(curr->pages, curr->page_count);
            KFREE(curr);
            return;
            proc_amount--;
        }
        prev = curr;
        curr = curr->next;
    }
}

volatile static U32 tcks __attribute__((section(".data"))) = 0;
volatile static U32 last_screen_buf_update ATTRIB_DATA = 0;
TCB *find_next_active_task(void) {
    if (!initialized) return NULL;

    if(master_tcb.info.state != TCB_STATE_IMMORTAL) {
        // Master TCB is amidst something important, do not switch away
        return &master_tcb; // master must always be immortal
    }

    TCB *start = current_tcb;
    TCB *next = start->next;

    while(next != start) {
        if (next->info.state != TCB_STATE_INACTIVE && next->info.state != TCB_STATE_ZOMBIE) {
            return next;
        }
        next = next->next;
    }

    if (start->info.state != TCB_STATE_INACTIVE && start->info.state != TCB_STATE_ZOMBIE) {
        return start; // only current is active
    }

    // No active tasks available
    // Fallback to master task if no other active tasks
    return &master_tcb;
}

// Called from PIT ISR to perform task switch
// Arg: current trap frame (already pushed by ISR)
// Returns: new trap frame to load (or same if no switch)
// Called from PIT ISR to perform task switch
TrapFrame* pit_handler_task_control(TrapFrame *cur) {
    // todo: tick counter here
    tcks++;
    if(EVERY_HZ(tcks, REFRESH_HZ)) {
        flush_focused_framebuffer(); 
    }
    if (!initialized) {
        return cur;
    }    

    if (current_tcb) {
        current_tcb->tf = cur;
        current_tcb->info.cpu_time += PIT_TICK_MS;
    }
    
    TCB *next = find_next_active_task();
    
    if (!next) {
        // panic(PANIC_TEXT("No active tasks to switch to!"), PANIC_CONTEXT_SWITCH_FAILED);
        next = current_tcb; 
    }
    
    current_tcb = next;
    update_current_framebuffer();
    current_tcb->info.num_switches++;


    set_next_task_esp_val((U32)current_tcb->tf);
    set_next_task_cr3_val((U32)current_tcb->pagedir_phys);
    set_next_task_pid(current_tcb->info.pid);
    set_next_task_num_switches(current_tcb->info.num_switches);
    last_tcb = current_tcb;
    return current_tcb->tf;
}



void INC_rki_row(U32 *rki_row) {
    *rki_row += VBE_CHAR_HEIGHT + 2;
}
void early_debug_tcb(U32 pid) {
    // for early task debugging
    TCB *t = get_master_tcb();
    U32 rki_row = 0;
    VBE_DRAW_STRING(0, rki_row, "TCB Dump:", VBE_GREEN, VBE_BLACK);
    INC_rki_row(&rki_row);
    U8 buf[20];
    ITOA_U(proc_amount, buf, 10);
    VBE_DRAW_STRING(0, rki_row, "Active tasks:", VBE_GREEN, VBE_BLACK);
    VBE_DRAW_STRING(150, rki_row, buf, VBE_GREEN, VBE_BLACK);
    INC_rki_row(&rki_row);

    do {
        if(t->info.pid != pid) {
            if(pid != U32_MAX) continue;
        }
        if(pid == U32_MAX) break;

        INC_rki_row(&rki_row);

        VBE_DRAW_STRING(0, rki_row, "Process name:", VBE_GREEN, VBE_BLACK);
        VBE_DRAW_STRING(110, rki_row, t->info.name, VBE_GREEN, VBE_BLACK);
        INC_rki_row(&rki_row);

        ITOA(t->info.pid, buf, 10);
        VBE_DRAW_STRING(0, rki_row, "TCB PID:", VBE_GREEN, VBE_BLACK);
        VBE_DRAW_STRING(100, rki_row, buf, VBE_GREEN, VBE_BLACK);
        INC_rki_row(&rki_row);

        ITOA(t->info.state, buf, 10);
        VBE_DRAW_STRING(0, rki_row, "State:", VBE_GREEN, VBE_BLACK);
        VBE_DRAW_STRING(100, rki_row, buf, VBE_GREEN, VBE_BLACK);
        INC_rki_row(&rki_row);

        ITOA_U((U32)t->info.num_switches, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Switches:", VBE_GREEN, VBE_BLACK);
        VBE_DRAW_STRING(100, rki_row, buf, VBE_GREEN, VBE_BLACK);
        INC_rki_row(&rki_row);

        ITOA_U((U32)t->info.cpu_time, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "CPU Time (ms):", VBE_GREEN, VBE_BLACK);
        VBE_DRAW_STRING(150, rki_row, buf, VBE_GREEN, VBE_BLACK);
        INC_rki_row(&rki_row);

        ITOA_U((U32)t->msg_count, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Messages:", VBE_GREEN, VBE_BLACK);
        VBE_DRAW_STRING(150, rki_row, buf, VBE_GREEN, VBE_BLACK);
        INC_rki_row(&rki_row);

        ITOA_U((U32) focused_task == t, buf, 16);
        VBE_DRAW_STRING(0, rki_row, "Focused:", VBE_GREEN, VBE_BLACK);
        VBE_DRAW_STRING(150, rki_row, buf, VBE_GREEN, VBE_BLACK);
        INC_rki_row(&rki_row);

        set_rki_row(rki_row);
        DUMP_MEMORY((U32)t->pagedir_phys, PAGE_SIZE/8);
        DUMP_MEMORY((U32)t->pages, 256);
        DUMP_MEMORY(USER_BINARY_VADDR, 256);

        VBE_UPDATE_VRAM();
    } while((t = t->next) != get_master_tcb());
}



U32 get_ticks(void) {
    return tcks;
}

U32 get_uptime_sec(void) {
    return tcks / TICKS_PER_SECOND;
}

TCB *get_focused_task(void) {
    return focused_task;
}
void free_message(PROC_MESSAGE *msg) {
    if (!msg) return;
    if(msg->data_provided && msg->data) {
        KFREE(msg->data);
    }
    KFREE(msg);
}
// PROC_COMMANDS from tasks to kernel
void empty_msg_queue(U32 pid) {
    TCB *tcb = get_tcb_by_pid(pid);
    if (!tcb) return;
    
    // Clear message queue
    tcb->msg_queue_head = 0;
    tcb->msg_queue_tail = 0;
}

void add_message(TCB *t, PROC_MESSAGE *msg) {
    if (!t || !msg) return;
    U32 next_tail = (t->msg_queue_tail + 1) % PROC_MSG_QUEUE_SIZE;
    if (next_tail == t->msg_queue_head) {
        // Queue is full, drop message
        return;
    }
    // Add message into queue
    MEMCPY(&t->msg_queue[t->msg_queue_tail], msg, sizeof(PROC_MESSAGE));
    t->msg_queue_tail = next_tail;
    t->msg_count++;
}


void send_msg(PROC_MESSAGE *msg) {
    if (!msg) return;
    TCB *receiver = get_tcb_by_pid(msg->receiver_pid);

    if (!receiver) return;

    // Add message to queue
    add_message(receiver, msg);
}

// e.g., terminate self, sleep, wait, etc.
void handle_kernel_messages(void) {
    // Handle messages sent to kernel by tasks
    TCB *master = get_master_tcb();
    while(master->msg_queue_head != master->msg_queue_tail) {
        PROC_MESSAGE *msg = &master->msg_queue[master->msg_queue_head];
        switch(msg->type) {
            case PROC_MSG_SET_FOCUS:
                focused_task = get_tcb_by_pid(msg->signal);
                break;
            case PROC_MSG_TERMINATE_SELF:
                KILL_PROCESS(msg->sender_pid);
                break;
            case PROC_MSG_SLEEP:
                master->info.state = TCB_STATE_SLEEPING;
                break;
            case PROC_MSG_WAKE:
                if(master->info.state == TCB_STATE_SLEEPING) {
                    master->info.state = TCB_STATE_ACTIVE;
                }
                break;
            case PROC_MSG_EMPTY_QUEUE:
                empty_msg_queue(msg->sender_pid);
                break;
            case PROC_GET_FRAMEBUFFER:
                TCB *t = get_tcb_by_pid(msg->sender_pid);
                U8 buf[20];
                if(t) {
                    t->framebuffer_mapped = TRUE;
                    PROC_MESSAGE *resp = KMALLOC(sizeof(PROC_MESSAGE));
                    resp->sender_pid = 0; // from kernel
                    resp->receiver_pid = msg->sender_pid;
                    resp->type = PROC_FRAMEBUFFER_GRANTED;
                    resp->signal = 0    ;
                    resp->data_provided = FALSE;
                    resp->timestamp = get_uptime_sec();
                    resp->read = FALSE;
                    resp->data = NULL;
                    send_msg(resp);
                    free_message(resp);
                }
                break;
            case PROC_RELEASE_FRAMEBUFFER:
                t = get_tcb_by_pid(msg->sender_pid);
                if(t) {
                    t->framebuffer_mapped = FALSE;
                }
                break;
            case PROC_GET_KEYBOARD_EVENTS:
                t = get_tcb_by_pid(msg->sender_pid);
                if(t) {
                    if(IS_FLAG_SET(t->info.event_types, PROC_EVENT_INFORM_ON_KB_EVENTS)) {
                        // already enabled
                        break;
                    }
                    FLAG_SET(t->info.event_types, PROC_EVENT_INFORM_ON_KB_EVENTS);
                    PROC_MESSAGE *resp = KMALLOC(sizeof(PROC_MESSAGE));
                    resp->sender_pid = 0; // from kernel
                    resp->receiver_pid = msg->sender_pid;
                    resp->type = PROC_KEYBOARD_EVENTS_GRANTED;
                    resp->signal = 0;
                    resp->data_provided = FALSE;
                    resp->timestamp = get_uptime_sec();
                    resp->read = FALSE;
                    resp->data = NULL;
                    send_msg(resp);
                    free_message(resp);
                }
                break;
            case PROC_RELEASE_KEYBOARD_EVENTS:
                t = get_tcb_by_pid(msg->sender_pid);
                if(t) {
                    FLAG_UNSET(t->info.event_types, PROC_EVENT_INFORM_ON_KB_EVENTS);
                }
                break;
            case PROC_GET_MOUSE_EVENTS:
                t = get_tcb_by_pid(msg->sender_pid);
                if(t) {
                    if(IS_FLAG_SET(t->info.event_types, PROC_EVENT_INFORM_ON_MOUSE_EVENTS)) {
                        // already enabled
                        break;
                    }
                    FLAG_SET(t->info.event_types, PROC_EVENT_INFORM_ON_MOUSE_EVENTS);
                    PROC_MESSAGE *resp = KMALLOC(sizeof(PROC_MESSAGE));
                    resp->sender_pid = 0; // from kernel
                    resp->receiver_pid = msg->sender_pid;
                    resp->type = PROC_MOUSE_EVENTS_GRANTED;
                    resp->signal = 0;
                    resp->data_provided = FALSE;
                    resp->timestamp = get_uptime_sec();
                    resp->read = FALSE;
                    resp->data = NULL;
                    send_msg(resp);
                    free_message(resp);
                }
                break;
            case PROC_RELEASE_MOUSE_EVENTS:
                t = get_tcb_by_pid(msg->sender_pid);
                if(t) {
                    FLAG_UNSET(t->info.event_types, PROC_EVENT_INFORM_ON_MOUSE_EVENTS);
                }
                break;
            case PROC_MSG_NONE:
            default:
                // Unknown message type
                break;
        }
        master->msg_queue_head = (master->msg_queue_head + 1) % PROC_MSG_QUEUE_SIZE;
        if(master->msg_count > 0) master->msg_count--;
    }

    // Handle messages sent by kernel to tasks
    TCB *t = get_current_tcb();
    if (!t) return;
    KEYPRESS k = GET_CURRENT_KEY_PRESSED(); // saved for every process

    do {
        if(t->info.pid == 0) continue; // skip master
        PROC_EVENT_TYPE ev = t->info.event_types;
        if(IS_FLAG_SET(ev, PROC_EVENT_INFORM_ON_KB_EVENTS)) {
            if(!k.keycode == KEY_UNKNOWN) {
                MODIFIERS *mods = GET_KEYBOARD_MODIFIERS();
    
                PROC_MESSAGE *msg = KMALLOC(sizeof(PROC_MESSAGE));
                msg->sender_pid = 0; // from kernel
                msg->receiver_pid = t->info.pid;
                msg->type = PROC_MSG_KEYBOARD;
    
                msg->data_provided = TRUE;
                msg->data = KMALLOC(sizeof(KEYPRESS) + sizeof(MODIFIERS));
                if(!msg->data) {
                    KFREE(msg);
                    continue;
                }
                MEMZERO(msg->data, sizeof(KEYPRESS) + sizeof(MODIFIERS));
                MEMCPY(msg->data, &k, sizeof(KEYPRESS));
                MEMCPY((U8 *)msg->data + sizeof(KEYPRESS), mods, sizeof(MODIFIERS));
    
                msg->timestamp = get_uptime_sec();
                msg->read = FALSE;
    
                send_msg(msg);
                free_message(msg);
            } 
        }
    } while((t = t->next) != get_master_tcb());
}
