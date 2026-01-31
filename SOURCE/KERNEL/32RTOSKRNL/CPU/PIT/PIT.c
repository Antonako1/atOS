#include <CPU/PIT/PIT.h>
#include <CPU/PIC/PIC.h>
#include <STD/ASM.h>
#include <CPU/ISR/ISR.h>
#include <RTOSKRNL_INTERNAL.h>
#include <CPU/IDT/IDT.h>
#include <STD/STRING.h>
#include <VESA/VBE.h>
#include <PAGING/PAGING.h>
#include <DEBUG/KDEBUG.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_FREQUENCY 1193182

#define KCS 0x08
#define KDS 0x10
#define _STR_HELPER(x) #x
#define _STR(x) _STR_HELPER(x)

void pit_set_frequency(U32 freq) {
    U32 divisor = PIT_FREQUENCY / freq;

    // Send command byte: channel 0, access low+high, mode 3 (square wave), binary
    _outb(PIT_COMMAND, 0x36);

    // Send divisor low byte, then high byte
    _outb(PIT_CHANNEL0, (U8)(divisor & 0xFF));
    _outb(PIT_CHANNEL0, (U8)((divisor >> 8) & 0xFF));
}

static volatile U32 ticks __attribute__((section(".data"))) = 0;
static U32 hz __attribute__((section(".data"))) = 100;
static BOOLEAN initialized __attribute__((section(".data"))) = FALSE;

static volatile U32 next_task_esp_val __attribute__((section(".data"))) = 0;
static volatile U32 next_task_cr3_val __attribute__((section(".data"))) = 0;
static volatile U32 current_task_esp __attribute__((section(".data"))) = 0;

static volatile U32 next_task_num_switches __attribute__((section(".data"))) = 0;
static volatile U32 next_task_pid __attribute__((section(".data"))) = 0;

static volatile U32 PROC_ARGC ATTRIB_DATA = 0;
static volatile PPU8 PROC_ARGV ATTRIB_DATA = 0;

void set_args(TCB *tcb) {
    PROC_ARGC = tcb->argc;
    PROC_ARGV = tcb->argv;
}

static BOOL BLOCK_TASK_SWITCH ATTRIB_DATA = FALSE;
void set_next_task_pid(U32 pid) {
    next_task_pid = pid;
}
void set_next_task_num_switches(U32 num_switches) {
    next_task_num_switches = num_switches;
}

void set_next_task_esp_val(U32 esp) {
    next_task_esp_val = esp;
}
void set_next_task_cr3_val(U32 cr3) {
    next_task_cr3_val = cr3;
}

U32 get_current_task_esp(void) {
    return current_task_esp;
}

void update_next_cr3() {
    // legacy. not feeling like removing it right now
}

U0 PIT_WAIT_MS(U32 ms) {
    U32 start = ticks;
    U32 waitTicks = (hz * ms) / 1000;
    if(waitTicks == 0) waitTicks = 1; // minimum wait of 1 tick
    while((ticks - start) < waitTicks) {
        ASM_VOLATILE("hlt");
    }
}

U0 PIT_INIT(U0) {
    if(initialized) return;
    KDEBUG_PUTS("[PIT] Initializing\n");
    initialized = TRUE;
    hz = PIT_TICKS_HZ;
    pit_set_frequency(hz);
    idt_set_gate(PIT_VECTOR, (U32)isr_pit, KCS, 0x8E);
    KDEBUG_PUTS("[PIT] Gate set\n");
    ISR_REGISTER_HANDLER(PIC_REMAP_OFFSET + 0, isr_pit);
    KDEBUG_PUTS("[PIT] Handler registered\n");
    PIC_Unmask(0);
    KDEBUG_PUTS("[PIT] PIC unmasked\n");
    ticks = 0;
    KDEBUG_PUTS("[PIT] Initialized\n");
}

U32 *PIT_GET_TICKS_PTR() {
    return &ticks;
}

U32 *PIT_GET_HZ_PTR() {
    return &hz;
}

void SCHEDULER_YIELD(void) {
    isr_pit();  
}

__attribute__((naked)) void isr_pit(void) {
    asm volatile(
        // eip, cs, eflags are pushed by CPU automatically
        // We push the rest manually

        // Pushes all registers from previous task
        "pushl %%gs\n\t"
        "pushl %%fs\n\t"
        "pushl %%es\n\t"
        "pushl %%ds\n\t"
        "pushl %%edi\n\t"
        "pushl %%esi\n\t"
        "pushl %%ebp\n\t"
        "pushl %%esp\n\t"
        "pushl %%ebx\n\t"
        "pushl %%edx\n\t"
        "pushl %%ecx\n\t"
        "pushl %%eax\n\t"

        // Set kernel segments for safety
        "movw $" _STR(KDS) ", %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"

        // Save current task's stack pointer and call scheduler
        "movl %%esp, current_task_esp\n\t"

        "pushl %%esp\n\t" // push current esp as arg
        "call pit_handler_task_control\n\t"
        // returned esp in eax
        "addl $4, %%esp\n\t" // clean up arg
        
            // Load next task's ESP and CR3
        "movl next_task_cr3_val, %%eax\n\t"
        // Send EOI
        "movb $0x20, %%al\n\t"
        "outb %%al, $0x20\n\t"
        "outb %%al, $0xA0\n\t" // If slave PIC is used
        
        "movl %%eax, %%cr3\n\t"

        "movl next_task_esp_val, %%esp\n\t"

        /*
        Pseudocode:

        This is a bad hack, but it works for now.

        if next_task neq 0 and num_switches equ 1 then
            re-set registers
            eax, ecx, edx, ebx, esp , ebp, esi, edi = 0
            ds, es, fs, gs = KDS

            push argc and argv... (bruh)
            
            cs = KCS
            eflags = EFLAGS_IF (0x200)
            eip = entry point

        else
            // do normal task switch
        */
        "cmpl $0, next_task_pid\n\t"
        "je normal_task_switch\n\t" // if next_task_pid == 0, normal switch
        
        "cmpl $1, next_task_num_switches\n\t"
        "jne normal_task_switch\n\t" // if num_switches != 1, normal switch
        // New task, first time switch
        // Set up registers for new task
        "xorl %%eax, %%eax\n\t"
        "movl %%eax, %%ecx\n\t"
        "movl %%eax, %%edx\n\t"
        "movl %%eax, %%ebx\n\t"
        // "movl $0, %%esp\n\t" // ESP is already set to new task's esp
        "movl %%eax, %%ebp\n\t"
        "movl %%eax, %%esi\n\t"
        "movl %%eax, %%edi\n\t"
        "movw $" _STR(KDS) ", %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"   

        "pushl $0x0\n\t" //fuckass alignment
        
        // push argv pointer (U8 **)
        "movl PROC_ARGV, %%eax\n\t"
        "pushl %%eax\n\t"
        // push argc (U32)
        "movl PROC_ARGC, %%eax\n\t"
        "pushl %%eax\n\t"

        "pushl $0x0\n\t" //fuckass alignment pt2
        


        // Set up eip, cs, eflags for iret
        "pushl $0x200\n\t"  // eflags with IF set
                            // this enables interrupts when we iret
        "pushl $" _STR(KCS) "\n\t" // push kernel code segment for cs
        "pushl $" _STR(USER_BINARY_VADDR) "\n\t" // eip = entry point of new task
        "jmp end_switch\n\t"        
    "normal_task_switch:\n\t"
        // Pop all registers from the new esp
        "popl %%eax\n\t"
        "popl %%ecx\n\t"
        "popl %%edx\n\t"
        "popl %%ebx\n\t"
        "popl %%esp\n\t" // dummy esp
        "popl %%ebp\n\t"
        "popl %%esi\n\t"
        "popl %%edi\n\t"
        "popl %%ds\n\t"
        "popl %%es\n\t"
        "popl %%fs\n\t"
        "popl %%gs\n\t"
    "end_switch:\n\t"
        // Return to the new task
        // "sti\n\t"
        "iret\n\t" // return to the new task, which pops eip, cs, eflags
        : : : "memory"
    );
}
