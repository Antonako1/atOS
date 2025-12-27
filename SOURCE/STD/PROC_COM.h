/**
 * PROC_COM.h
 *
 * Process communication utilities for programs and processes.
 *
 * NOT usable in kernel code.
 * 
 * Search for PROC_COM for process communication utilities.
 * Search for SHELLCOM for shell communication utilities.
 * Search for KRNLCOM for kernel communication utilities.
 */

#ifndef PROC_COM_H
#define PROC_COM_H

#include <STD/TYPEDEF.h>
#include <PROC/PROC.h>
#include <DRIVERS/PS2/KEYBOARD.h> // for KEYPRESS and MODIFIERS structs
#include <PROGRAMS/SHELL/SHELL.h> // For STDOUT 

/**
 * If you ever add custom messaged, add them here
 */
typedef enum {
    SHELL_CMD_CREATE_STDOUT = 0x100000, // Create a new console stdout buffer for this process
    SHELL_RES_STDOUT_CREATED, // Response: stdout created successfully
    SHELL_RES_STDOUT_FAILED, // Response: stdout creation failed successfully
    SHELL_CMD_ENDED_MYSELF, // Inform shell that process will end itself
    SHELL_CMD_SHELL_FOCUS, // Inform shell to focus this process's screen buffer
    SHELL_CMD_INFO_ARRAYS, // Request array of information about shell (SHELL_INSTANCE)
    SHELL_RES_INFO_ARRAYS, // Response: array of information about shell (SHELL_INSTANCE)
    SHELL_RES_INFO_ARRAYS_FAILED, // Response: failed to get array of information about shell
    SHELL_CMD_EXECUTE_BATSH, // Executes a batsh command (sys command)
} COMMND_TYPE;


void PROC_INIT_CONSOLE();
void PRIC_INIT_GRAPHICAL();
BOOLEAN IS_PROC_INITIALIZED();
BOOLEAN IS_PROC_GUI_INITIALIZED();
VOID KILL_SELF();
VOID START_HALT();
VOID EXIT(U32 n);

SHELL_INSTANCE *GET_SHELL_HANDLE();
/**
 * PROC_COM
 */
U32 PROC_GETPID(U0); // Get your own PID
U8 *PROC_GETNAME(U0); // Get your own name
U32 PROC_GETPPID(U0); // Get your parent's PID, -1 if none
U8 *PROC_GETPARENTNAME(U0); // Get your parent's name, NULL if no name

U32 PROC_GETPID_BY_NAME(U8 *name); // Get PID of process by name, returns -1 if not found

TCB *GET_CURRENT_TCB(void); // Get your own TCB, NULL on error
TCB *GET_MASTER_TCB(void); // Get the master TCB, should never be NULL
TCB *GET_TCB_BY_PID(U32 pid); // Get TCB of process by PID, NULL if not found. MFree with FREE_TCB
TCB *GET_PARENT_TCB(void); // Get your parent's TCB, NULL if no
void FREE_TCB(TCB *tcb); // MFree a TCB received via GET_TCB_BY_PID

#define KERNEL_PID 0
#define CREATE_PROC_MSG(receiver, msg_type, data_ptr, data_sz, signal_val) \
    (PROC_MESSAGE){ \
        .sender_pid = PROC_GETPID(), \
        .receiver_pid = (receiver), \
        .type = (msg_type), \
        .data_provided = ((data_ptr) != NULL), \
        .data = (data_ptr), \
        .data_size = (data_sz), \
        .signal = (signal_val), \
        .timestamp = 0, /* filled by kernel */ \
        .read = FALSE, \
        .raw_data = NULL, \
        .raw_data_size = 0, \
    }

#define CREATE_PROC_MSG_RAW(receiver, msg_type, raw_ptr, raw_sz, signal_val) \
    (PROC_MESSAGE){ \
        .sender_pid = PROC_GETPID(), \
        .receiver_pid = (receiver), \
        .type = (msg_type), \
        .data_provided = FALSE, \
        .data = NULL, \
        .data_size = 0, \
        .signal = (signal_val), \
        .timestamp = 0, \
        .read = FALSE, \
        .raw_data = (raw_ptr), \
        .raw_data_size = (raw_sz), \
    }


/**
 * KRNLCOM
 */

// Send a message to another process or the kernel, Create struct with CREATE_PROC_MSG
VOID SEND_MESSAGE(PROC_MESSAGE *msg);

// Get amount of messages in your message queue
U32 MESSAGE_AMOUNT();

// Get the next message from your message queue, or NULL if none
// Removed from message queue after calling
PROC_MESSAGE *GET_MESSAGE();

// MFree a message received via GET_MESSAGE
VOID FREE_MESSAGE(PROC_MESSAGE *msg);

U32 GET_PIT_TICKS();

U32 GET_SYS_SECONDS();

U32 CPU_SLEEP(U32 ms); // Sleep for given milliseconds

VOID SYS_RESTART();

/**
 * SHELLCOM
 */

/*
Please note that if process is created with shell, parent pid will always be your shell.
It is recommended that if you create a process inside your own program, the parent would be the shell
*/
BOOLEAN START_PROCESS(
    U8 *proc_name, 
    VOIDPTR file, 
    U32 bin_size, 
    U32 initial_state, 
    U32 parent_pid,
    PPU8 argv,
    U32 argc
);
VOID KILL_PROCESS(U32 pid);
SHELL_INSTANCE *GET_SHELL_HANDLE(U0);
STDOUT *GET_PROC_STDOUT();

#endif //PROC_COM_H