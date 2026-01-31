// README: Please see top of MEMORY/PAGING/PAGING.c for paging overview
#include <PROC/PROC.h> 

#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>

#include <DRIVERS/VESA/VBE.h>
#include <DRIVERS/PS2/KEYBOARD_MOUSE.h>

#include <MEMORY/PAGEFRAME/PAGEFRAME.h>
#include <MEMORY/PAGING/PAGING.h>
#include <MEMORY/HEAP/KHEAP.h>

#include <STD/ASM.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/BINARY.h>

#include <ERROR/ERROR.h>

#include <CPU/PIT/PIT.h>
#include <CPU/ISR/ISR.h> // for regs struct
#include <CPU/PIC/PIC.h>
#include <CPU/FPU/FPU.h>

#include <DEBUG/KDEBUG.h>
#define EFLAGS_IF 0x0200
#define KDS 0x10
#define KCS 0x08

static TCB master_tcb __attribute__((section(".data"))) = {0};
static TCB *current_tcb __attribute__((section(".data"))) = &master_tcb;
static U32 next_pid __attribute__((section(".data"))) = 1; // start from 1 since 0 is master tcb
static U8 initialized __attribute__((section(".data"))) = FALSE;
static U32 proc_amount __attribute__((section(".data"))) = 0;
static TCB *last_tcb __attribute__((section(".data"))) = &master_tcb;
static TCB *current_shell ATTRIB_DATA = &master_tcb;
static TCB *last_fpu_user ATTRIB_DATA = NULL;
static volatile BOOL8 immediate_reschedule_val ATTRIB_DATA = 0; 

// Shell informs kernel what task is focused
static volatile TCB *focused_task __attribute__((section(".data"))) = NULL;

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
TCB *get_last_fpu_user(void) {
    return last_fpu_user;
}
void set_last_fpu_user(TCB *tcb) {
    last_fpu_user = tcb;
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

    fpu_zero_init(&master_tcb);
    focused_task = &master_tcb; // start with master focused
    KDEBUG_PUTS("[proc] Master TCB initialized\n");
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
    KDEBUG_PUTS("[proc] Starting up multitasking\n");
    STI;
    immediate_reschedule_val=FALSE;
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
    last_fpu_user = NULL;
    KDEBUG_PUTS("[proc] Multitasking initialized\n");
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
    KDEBUG_PUTS("[proc] Got pages:");
    KDEBUG_HEX32(new_pd_phys);
    KDEBUG_PUTS("\n");
    MEMZERO(new_pd_phys, PAGE_SIZE);
    KDEBUG_PUTS("[proc] Zeroed.\n");

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
    KDEBUG_PUTS("[proc] Entered setup_user_process\n");
    // Create page directory
    PD_HANDLE pdh = create_process_pagedir();
    if (!pdh.virt) return NULL;
    KDEBUG_PUTS("[proc] Created process pagedir\n");
    proc->pagedir = pdh.virt; // Virtual pointer to page directory, used by the process
    proc->pagedir_phys = pdh.phys; // Physical address of page directory, loaded into CR3 on context switch
    
    // Now we map all of kernel pages into the new page directory
    VOIDPTR kernel_pd_raw = get_page_directory();
    if(!kernel_pd_raw) {
        destroy_process_pagedir(proc->pagedir_phys);
        return NULL;
    }
    KDEBUG_PUTS("[proc] Got kernel pd\n");


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
    KDEBUG_PUTS("[proc] Got pages\n");
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
    KDEBUG_PUTS("[proc] Kernel PDEs copied\n");

    
    // Map binary
    if (!map_user_binary_at(pages, bin_pages, &layout, proc->pagedir_phys, USER_BINARY_VADDR, binary_data, bin_size
    )) {
        destroy_process_pagedir(proc->pagedir_phys);
        return NULL;
    }
    KDEBUG_PUTS("[proc] Mapped binary\n");

    // Map heap. Heap is right after binary
    U32 heap_vaddr = USER_BINARY_VADDR + (layout.heap_base - layout.bin_end);
    if(!map_user_heap_at(pages, heap_pages, &layout, proc->pagedir_phys, heap_vaddr, heap_size)) {
        destroy_process_pagedir(proc->pagedir_phys);
        return NULL;
    }
    KDEBUG_PUTS("[proc] Mapped stack\n");
    // Map stack. Stack is right after heap
    U32 stack_vaddr = USER_BINARY_VADDR + (layout.stack_base - layout.bin_end);
    if(!map_user_stack_at(pages, stack_pages, &layout, proc->pagedir_phys, stack_vaddr, stack_size, proc)) {
        destroy_process_pagedir(proc->pagedir_phys);
        return NULL;
    }
    KDEBUG_PUTS("[proc] Mapped heap\n");

    // Map pages ONLY after all physical pages are allocated and binary is copied
    #ifndef MAP_IN_FUNCTIONS
    for(U32 i = 0; i < amount_of_pages_needed; i++) {
        U32 phys = (U32)pages + (i * PAGE_SIZE);
        U32 vaddr = USER_BINARY_VADDR + (( (U32)phys - layout.bin_base));
        map_page(proc->pagedir_phys, vaddr, phys, PAGE_PRW);
    }
    #endif
    KDEBUG_PUTS("[proc] Mapped pagedir\n");

    // +1 is to adjust for padding between stack and framebuffer
    proc->framebuffer_phys = (VOIDPTR)( (U32)pages + ( (bin_pages + heap_pages + stack_pages + 1) * PAGE_SIZE) );
    proc->framebuffer_virt = (VOIDPTR)USER_BINARY_VADDR + ( (bin_pages + heap_pages + stack_pages + 1) * PAGE_SIZE);
    proc->framebuffer_mapped = FALSE; // Flagged as not mapped yet. User process must request to draw to framebuffer
    KDEBUG_PUTS("[proc] Framebuffer initialized\n");

    // copy parent's framebuffer into virt
    if(proc->parent != NULL) {
        MEMCPY_OPT(proc->framebuffer_phys, proc->parent->framebuffer_phys, framebuffer_sz);
        KDEBUG_PUTS("[proc] Frambuffer copied\n");
    }

    // Initialize trap frame
    init_task_context(proc, (void (*)(void))USER_BINARY_VADDR, stack_size, initial_state);
    KDEBUG_PUTS("[proc] Trapframe initialized\n");

    // init fpu
    fpu_zero_init(proc);

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
    for(U32 i = 0; i < argc; i++) {
        KDEBUG_PUTS(argv[i]);
        KDEBUG_PUTS(" | ");
    }
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

    new_proc->info.pid = get_next_pid();
    
    KDEBUG_PUTS("[proc] setup_user_process...\n");
    panic_if(!setup_user_process(new_proc, (U8 *)file, bin_size, heap_size, stack_size, initial_state),
             PANIC_TEXT("Failed to set up user process"), PANIC_OUT_OF_MEMORY);
    KDEBUG_PUTS("[proc] setup_user_process OK\n");

    STRNCPY((char *)new_proc->info.name, (char *)proc_name, TASK_NAME_MAX_LEN);
    new_proc->info.name[TASK_NAME_MAX_LEN - 1] = '\0';

    new_proc->argc = argc;
    if (argc > 0) {
        new_proc->argv = KMALLOC(argc * sizeof(PPU8));
        if (!new_proc->argv) {
            KILL_PROCESS(new_proc);
            return; // Stop execution!
        }

        for (U32 i = 0; i < argc; i++) {
            new_proc->argv[i] = STRDUP(argv[i]);
        }
    } else {
        new_proc->argv = NULLPTR;
    }
    // VBE_CLEAR_SCREEN(VBE_RED);

    // DUMP_MEMORY(new_proc->argv, 100);
    // for(U32 i = 0; i < new_proc->argc;i++) {
    //     DUMP_MEMORY(new_proc->argv[i], 100);
    //     DUMP_STRING(new_proc->argv[i]);
    // }
    // HLT;

    add_tcb_to_scheduler(new_proc);
    KDEBUG_PUTS("[proc] added to scheduler pid="); 
    KDEBUG_HEX32(new_proc->info.pid); 
    KDEBUG_PUTS("\n[proc] Stack at "); 
    KDEBUG_HEX32(new_proc->stack_phys_base); 
    KDEBUG_PUTS("\n");

    if(initial_state & TCB_STATE_INFO_CHILD_PROC_HANDLER) {
        current_shell = new_proc;
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
    if (pid == 0) return; // cannot kill master process

    TCB *target = get_tcb_by_pid(pid);
    if (!target) return;

    if (target->info.state == TCB_STATE_IMMORTAL) return; // don't kill kernel task

    // If the process being killed is currently running, switch to master first
    if (target == current_tcb) {
        KDEBUG_PUTS("[proc] Killing current task, switching to master...\n");
        current_tcb = &master_tcb;
        write_cr3((U32)master_tcb.pagedir);
    }

    // If this was the focused task, reset focus to master
    if (target == focused_task) {
        focused_task = current_shell;
    }
    if(target == last_fpu_user) 
        last_fpu_user = NULL;
    // Remove from scheduler (this adjusts proc_amount automatically)
    remove_tcb_from_scheduler(target);

    // Free argv if present
    if (target->argv) {
        for (U32 i = 0; i < target->argc; i++) {
            if (target->argv[i]) {
                KFREE(target->argv[i]);
                target->argv[i] = NULL;
            }
        }
        KFREE(target->argv);
        target->argv = NULL;
    }

    // Free process memory pages
    if (target->pages && target->page_count > 0) {
        KFREE_USER_PAGES(target->pages, target->page_count);
        target->pages = NULL;
    }

    // Free page directory
    if (target->pagedir) {
        destroy_process_pagedir(target->pagedir);
        target->pagedir = NULL;
    }

    // Free stack (redundant if included in pages)
    if (target->stack_phys_base) {
        KFREE_PAGE(target->stack_phys_base);
        target->stack_phys_base = NULL;
    }

    // Finally free the TCB itself
    KFREE(target);

    KDEBUG_PUTS("[proc] Process killed successfully\n");
    KDEBUG_HEX32(pid);
    KDEBUG_PUTC('\n');
    
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
static U32 past = FALSE;
TrapFrame* pit_handler_task_control(TrapFrame *cur) {
    // todo: tick counter here
    tcks++;
    if(EVERY_HZ(tcks, REFRESH_HZ) || past) {
        past = TRUE;
        if(current_tcb->info.pid == master_tcb.info.pid || current_tcb->info.pid == focused_task->info.pid) {
            flush_focused_framebuffer(); 
            past = FALSE;
        }
    }

    UPDATE_KP_MOUSE_DATA();

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

    set_args(current_tcb);
    set_next_task_esp_val((U32)current_tcb->tf);
    set_next_task_cr3_val((U32)current_tcb->pagedir_phys);
    set_next_task_pid(current_tcb->info.pid);
    set_next_task_num_switches(current_tcb->info.num_switches);
    last_tcb = current_tcb;
    fpu_mark_task_switched();
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


void TCB_DUMP(TCB *t) {
    if (!t) {
        KDEBUG_PUTS("TCB: NULL\n");
        return;
    }

    KDEBUG_PUTS("===== TCB Dump =====\n");

    KDEBUG_PUTS("PID: ");
    KDEBUG_HEX32(t->info.pid);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Name: ");
    for (int i = 0; i < TASK_NAME_MAX_LEN && t->info.name[i]; i++) {
        KDEBUG_PUTC(t->info.name[i]);
    }
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("State: ");
    KDEBUG_HEX32(t->info.state);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("State Info: ");
    KDEBUG_HEX32(t->info.state_info);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Exit code: ");
    KDEBUG_HEX32(t->info.exit_code);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Priority: ");
    KDEBUG_HEX32(t->info.priority);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("CPU time: ");
    KDEBUG_HEX32(t->info.cpu_time);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Num switches: ");
    KDEBUG_HEX32(t->info.num_switches);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("FPU initialized: ");
    KDEBUG_HEX32(t->info.fpu_initialized);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Stack physical base: ");
    KDEBUG_HEX32((U32)t->stack_phys_base);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Stack virtual top: ");
    KDEBUG_HEX32((U32)t->stack_vtop);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Pagedir (virt): ");
    KDEBUG_HEX32((U32)t->pagedir);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Pagedir (phys): ");
    KDEBUG_HEX32((U32)t->pagedir_phys);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Heap size: ");
    KDEBUG_HEX32(t->heap_size);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Heap pages: ");
    KDEBUG_HEX32(t->heap_pages);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Stack size: ");
    KDEBUG_HEX32(t->stack_size);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Stack pages: ");
    KDEBUG_HEX32(t->stack_pages);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Child count: ");
    KDEBUG_HEX32(t->child_count);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Framebuffer mapped: ");
    KDEBUG_HEX32(t->framebuffer_mapped);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Framebuffer pages: ");
    KDEBUG_HEX32(t->framebuffer_pages);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Framebuffer physical: ");
    KDEBUG_HEX32((U32)t->framebuffer_phys);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Framebuffer virtual: ");
    KDEBUG_HEX32((U32)t->framebuffer_virt);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Message queue count: ");
    KDEBUG_HEX32(t->msg_count);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Message queue head: ");
    KDEBUG_HEX32(t->msg_queue_head);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Message queue tail: ");
    KDEBUG_HEX32(t->msg_queue_tail);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Args count: ");
    KDEBUG_HEX32(t->argc);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("Argv pointer: ");
    KDEBUG_HEX32((U32)t->argv);
    KDEBUG_PUTS("\n");

    KDEBUG_PUTS("====================\n");
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

void immediate_reschedule() {
    immediate_reschedule_val = TRUE;
}

void handle_immediate_reschedule() {
    if(immediate_reschedule_val) {
        immediate_reschedule_val = FALSE;
    }
}

/**
 * Message passing, handling and queue management for processes
 */

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
static U32 last_kb_seq = 0;
static U32 last_mouse_seq = 0;
static PS2_KB_MOUSE_DATA *data = NULLPTR;

void kernel_loop_init() {
    data = GET_KB_MOUSE_DATA();
}

void handle_kernel_messages(void) {
    // Handle messages sent to kernel by tasks
    TCB *master = get_master_tcb();
    while(master->msg_queue_head != master->msg_queue_tail) {
        PROC_MESSAGE *msg = &master->msg_queue[master->msg_queue_head];
        switch(msg->type) {
            case PROC_MSG_SET_FOCUS:
                KDEBUG_PUTS("[proc_msg] Switching focus to ");
                KDEBUG_HEX32(msg->signal);
                KDEBUG_PUTS("\n");
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
            case PROC_MSG_CREATE_PROCESS: 
                {
                    if(!msg->data_provided || !msg->data || msg->data_size < sizeof(RUN_BINARY_STRUCT)) {
                        KDEBUG_PUTS("[proc_msg] No data provided for PROC_MSG_CREATE_PROCESS\n");
                        goto free_create_proc_data;
                    };
                    KDEBUG_PUTS("[proc_msg] Creating new process\n");
                    RUN_BINARY_STRUCT *sct = msg->data;
                    RUN_BINARY(
                        sct->proc_name,
                        sct->file,
                        sct->bin_size,
                        USER_HEAP_SIZE,
                        USER_STACK_SIZE,
                        sct->initial_state,
                        sct->parent_pid,
                        sct->argv,
                        sct->argc
                    );
                    // KDEBUG_PUTS("Freeing args\n");
                    for(U32 i = 0; i < sct->argc; i++) {
                        KDEBUG_HEX32(i);
                        KDEBUG_PUTC('\n');
                        KFREE(sct->argv[i]);
                    }
                    // KDEBUG_PUTS("To array\n");
                    KFREE(sct->argv);
                    free_create_proc_data:
                    // KDEBUG_PUTS("Freeing data\n");

                    KFREE(sct);
                    KDEBUG_PUTS("[proc_msg] New process created\n");
                }
                break;
            case PROC_MSG_KILL_PROCESS:
                KILL_PROCESS(msg->signal);
                break;
            case PROC_MSG_NONE:
            default:
                // Unknown message type
                break;
        }
        // next message
        master->msg_queue_head = (master->msg_queue_head + 1) % PROC_MSG_QUEUE_SIZE;
        if(master->msg_count > 0) master->msg_count--;
    }

    // Handle messages sent by kernel to tasks
    TCB *t = get_current_tcb();
    if (!t) return;

    if (data->kb.seq != last_kb_seq) {
        last_kb_seq = data->kb.seq;

        if (data->kb.cur.pressed &&
            data->kb.mods.alt &&
            data->kb.mods.shift &&
            data->kb.cur.keycode == KEY_DELETE) {
            system_reboot();
        }
    }

    if(data->ms.seq != last_mouse_seq) {
        last_mouse_seq = data->ms.seq;
    }
}