#ifndef RTOSKRNL_INTERNAL_H
#define RTOSKRNL_INTERNAL_H
#include <STD/TYPEDEF.h>
#include <MEMORY/PAGEFRAME/PAGEFRAME.h> // for USER_HEAP_BLOCK
#include <CPU/ISR/ISR.h> // for regs struct

// This shows ONLY filename and line number in panic messages
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define PANIC_TEXT(x) x " (" __FILE__":"STR(__LINE__) ")"
#define PANIC_IF_TXT(x) x, PANIC_TEXT(#x)

typedef enum {

    PANIC_NONE = 0,
    PANIC_INITIALIZATION_FAILED,
    PANIC_OUT_OF_MEMORY,
    PANIC_INVALID_PROCESS,
    PANIC_STACK_OVERFLOW,
    PANIC_INVALID_TCB,
    PANIC_CONTEXT_SWITCH_FAILED,
    PANIC_SCHEDULER_FAILED,
    PANIC_PIT_INIT_FAILED,
    PANIC_CONTEXT_SWITCH_NULL_TCB,
    PANIC_NO_HARDWARE,
    PANIC_HARDWARE_FAILURE,
    
    PANIC_INVALID_STATE,
    PANIC_INVALID_ARGUMENT,
    PANIC_NOT_IMPLEMENTED,
    PANIC_PAGE_FAULT,
    PANIC_INVALID_MEMORY_ACCESS,
    PANIC_DOUBLE_ALLOCATION,

    PANIC_DOUBLE_FAULT,
    PANIC_FPU_FAULT,
    PANIC_UNDEFINED_OPCODE,
    PANIC_DIVIDE_ERROR,
    PANIC_PAGE_FAULT_IN_KERNEL,


    PANIC_KERNEL_SHELL_GENERAL_FAILURE,
    PANIC_UNKNOWN_ERROR
} PANIC_CODES;

void panic_reg(regs *r, const U8 *msg, U32 errmsg);
void panic(const U8 *msg, U32 errnum);
void panic_if(BOOL condition, const U8 *msg, U32 errnum);
void panic_debug(const U8 *msg, U32 errnum);
void panic_debug_if(BOOL condition, const U8 *msg, U32 errnum);
void assert(BOOL condition);
void system_halt(VOID);
void system_halt_if(BOOL condition);
void system_reboot(VOID);
void system_reboot_if(BOOL condition);
void system_shutdown(VOID);
void system_shutdown_if(BOOL condition); 
void DUMP_STACK(U32 esp, U32 count);
void DUMP_REGS(regs *r);
void DUMP_ERRCODE(U32 errcode);
void DUMP_INTNO(U32 int_no);
void DUMP_MEMORY(U32 addr, U32 length);
void DUMP_STRING(STRING buf);
void DUMP_STRINGN(STRING buf, U32 n);

// Kernel internal functions
void set_rki_row(U32 rki_row);

void LOAD_AND_RUN_KERNEL_SHELL(VOID);
BOOL initialize_filestructure(VOID);
void RTOSKRNL_LOOP(VOID);
#endif // RTOSKRNL_INTERNAL_H