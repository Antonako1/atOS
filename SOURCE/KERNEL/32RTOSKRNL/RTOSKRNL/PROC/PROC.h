/*
Process management and multitasking for atOS-RT.

Basically, this is kernel-level user process management with context switching
*/
#ifndef RTOS_PROC_H
#define RTOS_PROC_H

#include <STD/TYPEDEF.h>
#include <CPU/ISR/ISR.h> // for regs struct
#include <MEMORY/PAGEFRAME/PAGEFRAME.h> // for USER_HEAP_BLOCK
#include <MEMORY/HEAP/KHEAP.h> // for USER_HEAP_BLOCK
#include <MEMORY/MEMORY.h>
#include <STD/ASM.h>

#define USER_BINARY_VADDR MEM_USER_SPACE_BASE
#define MAX_PROC_AMOUNT 30 // max amount of processes including master
// 4 KB. NOTE: Just a padding between binary and stack. Actual HEAP size is defined on the fly
#define USER_HEAP_SIZE (1024 * 4 * 4) 

#define USER_STACK_SIZE (1 * 1024 * 1024) // 1 MB
#define MAX_USER_BINARY_SIZE (16 * 1024 * 1024) // 16 MB max binary size
#define MAX_USER_MEM_SIZE MEM_USER_SPACE_END_MIN

// vaddr == paddr in this simple paging model
#define virt_to_phys_pd(virt) (U32)(virt)
#define phys_to_virt_pd(phys) (U32)(phys)
#define virt_to_phys_pt(virt) (U32)(virt)
#define phys_to_virt_pt(phys) (U32)(phys)

#define proc_virt_to_phys(virt) (virt - USER_BINARY_VADDR)
#define proc_phys_to_virt(phys) (phys + USER_BINARY_VADDR)

#define TCB_STATE_INACTIVE      0x0001
#define TCB_STATE_ACTIVE        0x0002
#define TCB_STATE_WAITING       0x0004
#define TCB_STATE_IMMORTAL      0x0008  // Cannot be killed
#define TCB_STATE_ZOMBIE        0x0010  // Dead, waiting for parent to reap
#define TCB_STATE_SLEEPING      0x0020  // Sleeping, can be woken up
#define TCB_STATE_KERNEL_WAIT   0x0040  // Waiting for kernel event (e.g. I/O)
#define TCB_STATE_INFO_CHILD_PROC_HANDLER 0x0020 // Handles and informs kernel of child process
#define TCB_KILL                0x40000000 // Mark for kill

/*
Trap frame is pushed automatically by the PIT interrupt handler and used for context switching.

Push order
    CPU pushes:
        eip, cs, eflags
    We push:
        gs, fs, es, ds
        edi, esi, ebp, esp (dummy), ebx, edx, ecx, eax
    (no ss/esp since we stay in ring0)

*/
#define SIZE_OF_INTERRUPT_PUSHED_REGS (sizeof(U32) * 12) // full push - eip, cs, eflags
typedef struct GeneralRegs {
    U32 eax, ecx, edx, ebx;
    U32 esp, ebp, esi, edi;
} __attribute__((packed)) GeneralRegs;

typedef struct SegmentRegs {
    U32 ds, es, fs, gs;
} __attribute__((packed)) SegmentRegs;

typedef struct CPURegs {
    U32 eip, cs, eflags;
} __attribute__((packed)) CPURegs;

typedef struct TrapFrame {
    // Manually pushed
    SegmentRegs seg;
    GeneralRegs gpr;

    // CPU interrupt auto-pushed
    CPURegs cpu;
} __attribute__((packed)) TrapFrame;

typedef enum {
    PROC_EVENT_INFORM_ON_KB_EVENTS = 0x0001, // Tells kernel to inform this process when keyboard events occur
    PROC_EVENT_INFORM_ON_MOUSE_EVENTS = 0x0002, // Tells kernel to inform this process when mouse events occur
} PROC_EVENT_TYPE;

#define TASK_NAME_MAX_LEN 64
typedef struct TaskInfo {
    U32 pid;
    U8 name[TASK_NAME_MAX_LEN]; // Required for ps command
    U32 state;
    U32 state_info; // Additional info about state, e.g. sleep duration

    U32 exit_code; // Exit code if exited

    U32 priority; // Not used yet

    U32 cpu_time; // in ticks, total CPU time used
    U32 num_switches; // number of times scheduled
    U32 event_types; // Bitfield of event types this process is interested in
} __attribute__((packed)) TaskInfo;

#define PROC_MSG_QUEUE_SIZE 30
#define PROC_MSG_SIZE 128


/*+++
All different types of predefined messages to kernel
---*/
typedef enum {
    PROC_MSG_NONE = 0,

    /*
    Kernel sent a keyboard event
    Data contains
    KEYPRESS and MODIFIERS structs back-to-back
    */
    PROC_MSG_KEYBOARD = 0x00000001,
    /*
    Kernel sent a mouse event
    Data contains
    MOUSE_EVENT struct
    */
    PROC_MSG_MOUSE = 0x00000002,

    // User sent a terminate request to kernel
    // Data, and message is ignored
    // Put termination code in signal
    PROC_MSG_TERMINATE_SELF = 0x00000004, // terminate self

    // Creates new process.
    // Signal ignored. Data must contain RUN_BINARY_STRUCT struct
    PROC_MSG_CREATE_PROCESS = 0x00000005,

    // Kills process
    // Signal == pid to kill
    PROC_MSG_KILL_PROCESS = 0x00000006,

    // User sent a sleep request to kernel
    // Data, signal and message are ignored
    PROC_MSG_SLEEP = 0x00000008, // go to sleep

    // User sent a wake request to kernel
    // Data, signal and message are ignored
    PROC_MSG_WAKE = 0x00000010,  // wake up if sleeping

    /*
    Set signal to PID of process to set focus to
    Data is ignored
    message is ignored
    */
    PROC_MSG_SET_FOCUS = 0x00000020,

    /*
    Empties the message queue of the process
    Data is ignored
    message is ignored
    */
    PROC_MSG_EMPTY_QUEUE = 0x00000040,

    // Request access to the framebuffer
    // Data, signal and message are ignored
    PROC_GET_FRAMEBUFFER = 0x00000080,
    PROC_FRAMEBUFFER_GRANTED = 0x00000082, // sent by kernel to inform process that framebuffer access is granted

    // Release access to the framebuffer
    // Data, signal and message are ignored
    PROC_RELEASE_FRAMEBUFFER = 0x00000081,

    // Request keyboard events
    // Data, signal and message are ignored
    PROC_GET_KEYBOARD_EVENTS = 0x00000100,
    PROC_KEYBOARD_EVENTS_GRANTED = 0x00000101,

    // Release keyboard events
    // Data, signal and message are ignored
    PROC_RELEASE_KEYBOARD_EVENTS = 0x00000101,
    
    // Request mouse events
    // Data, signal and message are ignored
    PROC_GET_MOUSE_EVENTS = 0x00000200,
    PROC_MOUSE_EVENTS_GRANTED = 0x00000201,

    // Release mouse events
    // Data, signal and message are ignored
    PROC_RELEASE_MOUSE_EVENTS = 0x00000201,

    PROC_MSG_CUSTOM, // user-defined message for process communication
} PROC_MESSAGE_TYPE;

typedef struct proc_message {
    U32 sender_pid; // Your PID
    U32 receiver_pid; // 0 for kernel
    PROC_MESSAGE_TYPE type; 
    
    BOOLEAN data_provided; // TRUE if data pointer is valid
    // pointer to message data, can be NULL
    // Needs to be freed by receiver if data_provided is TRUE
    // Use KFREE to free
    VOIDPTR data; 
    U32 data_size;

    U32 signal; // Provide signal value if needed

    U32 timestamp; // time sent in seconds since boot
    BOOLEAN read; // TRUE if message has been read
} PROC_MESSAGE;

typedef struct TCB {
    TaskInfo info;
    TrapFrame *tf; // saved trap frame for context switching
    U32 *stack_phys_base; // Saved as physical address
    U32 *stack_vtop; // Top of the stack as virtual address for this process

    // Memory management
    // User page directories
    U32 *pagedir; // Virtual address of page directory
    U32 *pagedir_phys; // Physical address of page directory

    VOIDPTR pages; // Pointer to allocated pages for this process (for freeing)
    U32 page_count; // Number of allocated pages

    U32 binary_size; // Size in bytes
    U32 binary_pages; // Amount of pages
    U32 heap_size;
    U32 heap_pages;
    U32 stack_size;
    U32 stack_pages;
    struct TCB *next; // Next TCB in circular linked list

    
    struct TCB *parent; // Parent process, NULL for master
    
    BOOLEAN framebuffer_mapped; // TRUE if framebuffer is needed by process
    U32 framebuffer_pages; // Number of pages allocated for framebuffer
    VOIDPTR framebuffer_phys; // Physical address of framebuffer
    VOIDPTR framebuffer_virt; // Virtual address of framebuffer
    
    PROC_MESSAGE msg_queue[PROC_MSG_QUEUE_SIZE]; // Simple fixed-size message queue
    U32 msg_count; // Number of messages in the queue
    U32 msg_queue_head; // Index of the head of the queue
    U32 msg_queue_tail; // Index of the tail of the queue
    
    U32 child_count; // Number of child processes
    VOIDPTR children[MAX_PROC_AMOUNT - 2]; // List of pointers to child TCBs (excluding master and self)

    U32 argc;
    PPU8 argv;
} __attribute__((packed)) TCB;



typedef struct {
    U8 proc_name[255]; 
    VOIDPTR file; 
    U32 bin_size; 
    U32 initial_state; 
    U32 parent_pid;
    PPU8 argv;
    U32 argc;
} RUN_BINARY_STRUCT;


#ifdef __RTOS__
TCB *get_master_tcb(void);
void init_multitasking(void);
void multitasking_shutdown(void);
void uninitialize_multitasking(void);
U32 get_current_pid(void);
U32 get_next_pid(void);
U32 get_last_pid(void);
TCB *get_current_tcb(void);

TCB *get_focused_task(void);
void handle_kernel_messages(void);
U32 get_ticks(void);

static inline U32 read_cr3(void) {
    U32 val;
    ASM_VOLATILE("mov %%cr3, %0" : "=r"(val));
    return val;
}
static inline void write_cr3(U32 val) {
    ASM_VOLATILE("mov %0, %%cr3" :: "r"(val) : "memory");
}

void free_message(PROC_MESSAGE *msg);
void send_msg(PROC_MESSAGE *msg);
U32 get_active_task_count(void);
U32 get_uptime_sec(void);

/// @brief Run a user binary in a new process
/// @param proc_name Name of the process. Max length is TASK_NAME_MAX_LEN-1.
/// @param file Pointer to binary data
/// @param bin_size Size of binary data in bytes
/// @param heap_size Size of user heap in bytes
/// @param stack_size Size of user stack in bytes
/// @param initial_state Initial state of the new process (e.g., TCB_STATE_ACTIVE)
/// @param parent_pid PID of the parent process, or -1 for no parent
/// @return TRUE on success, FALSE on failure
/// @note The binary must be a flat binary. 
/// @note IMPORTANT: Only for kernel
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
);
/// @brief Terminate a process by its PID
/// @param pid Process ID to terminate
/// @note IMPORTANT: Only for kernel
void KILL_PROCESS(U32 pid);

/// Internal functions, not for public use
/// pid == U32_MAX all processes
void early_debug_tcb(U32 pid);

TrapFrame* pit_handler_task_control(TrapFrame* tf);


TCB *get_tcb_by_pid(U32 pid);
TCB *get_tcb_by_name(U8 *name);
#endif // __RTOS__

#endif // RTOS_PROC_H