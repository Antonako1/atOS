#ifndef __SHELL__
#define __SHELL__
#endif 
#include <PROGRAMS/SHELL/SHELL.h>
#include <STD/STRING.h>
#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/PROC_COM.h>
#include <STD/ASM.h>
#include <STD/MEM.h>
#include <STD/GRAPHICS.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <PROGRAMS/SHELL/VOUTPUT.h>
#include <CPU/PIT/PIT.h>
// Include for DEBUG_PRINTF
#include <STD/DEBUG.h> 

static BOOLEAN draw_access_granted ATTRIB_DATA = FALSE;
static BOOLEAN keyboard_access_granted ATTRIB_DATA = FALSE;
static SHELL_INSTANCE* shndl ATTRIB_DATA = NULLPTR;
U0 SHELL_LOOP(U0);
U0 INITIALIZE_SHELL();

U0 _start(U32 argc, PU8 argv[]) {
    draw_access_granted = FALSE;
    keyboard_access_granted = FALSE;

    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] _start: Initializing shell components.\n", PROC_GETPID());
    // -------------

    INITIALIZE_SHELL();
    INIT_SHELL_VOUTPUT();
    OutputHandle hndl = GetOutputHandle();
    if(hndl->Column != 0) {
        for(;;);
    }
    shndl = MAlloc(sizeof(SHELL_INSTANCE));
    MEMZERO(shndl, sizeof(SHELL_INSTANCE));
    INITIALIZE_DIR(shndl);
    SETUP_BATSH_PROCESSER();
    
    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] _start: Entering SHELL_LOOP.\n", shndl->self_pid);
    // -------------

    SHELL_LOOP();
}

SHELL_INSTANCE *GET_SHNDL(VOID) {
    return shndl;
}

U0 INITIALIZE_SHELL() {
    PROC_MESSAGE msg;

    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] INITIALIZE_SHELL: Requesting FB and focus.\n", PROC_GETPID());
    // -------------

    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_GET_FRAMEBUFFER, NULL, 0, 0);
    SEND_MESSAGE(&msg);
    
    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_MSG_SET_FOCUS, NULL, 0, PROC_GETPID());
    SEND_MESSAGE(&msg);

    msg = CREATE_PROC_MSG(0, PROC_GET_KEYBOARD_EVENTS, NULL, 0, 0);
    SEND_MESSAGE(&msg);
    
    shndl->self_pid = PROC_GETPID();
    shndl->cursor = GetOutputHandle();
    shndl->focused_pid = PROC_GETPID();
    shndl->previously_focused_pid = shndl->focused_pid;
    shndl->aborted = FALSE;
    SWITCH_LINE_EDIT_MODE();

    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] INITIALIZE_SHELL: Waiting for access grants.\n", shndl->self_pid);
    // -------------

    while(1) {
        MSG_LOOP();
        if(draw_access_granted && keyboard_access_granted) break;
    }
    shndl->active_keybinds = AK_DEFAULT;

    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] INITIALIZE_SHELL: Access granted. Initialization complete.\n", shndl->self_pid);
    // -------------
}

U0 SWITCH_CMD_INTERFACE_MODE(VOID) {
    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] SWITCH_CMD_INTERFACE_MODE: State changed to CMD_INTERFACE.\n", shndl->self_pid);
    // -------------
    shndl->state = STATE_CMD_INTERFACE;
}

U0 SWITCH_LINE_EDIT_MODE() {
    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] SWITCH_LINE_EDIT_MODE: State changed to EDIT_LINE.\n", shndl->self_pid);
    // -------------
    shndl->state = STATE_EDIT_LINE;
}

U0 MSG_LOOP(U0) {
    U32 msg_count = MESSAGE_AMOUNT();
    for(U32 i = 0; i <= msg_count; i++) {
        PROC_MESSAGE *msg = GET_MESSAGE(); // Gets last message. Marked as read
        if (!msg) break;
        switch(msg->type) {
            case PROC_KEYBOARD_EVENTS_GRANTED:
                // Keyboard events granted
                keyboard_access_granted = TRUE;
                // --- DEBUG ---
                DEBUG_PRINTF("[SHELL %d] MSG_LOOP: Received PROC_KEYBOARD_EVENTS_GRANTED.\n", shndl->self_pid);
                // -------------
                break;
            case PROC_FRAMEBUFFER_GRANTED:
                draw_access_granted = TRUE;
                // --- DEBUG ---
                DEBUG_PRINTF("[SHELL %d] MSG_LOOP: Received PROC_FRAMEBUFFER_GRANTED.\n", shndl->self_pid);
                // -------------
                break;
        }
        FREE_MESSAGE(msg);
    }
}

U0 SHELL_LOOP(U0) {
    U32 i = 0;
    U32 j = 0;
    U32 pass = 0;
    
    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] SHELL_LOOP: Running ATOSHELL.SH script.\n", shndl->self_pid);
    // -------------

    if(RUN_BATSH_SCRIPT("/ATOS/ATOSHELL.SH", 0, NULLPTR))
        DEBUG_PRINTF("[SHELL %d] SHELL_LOOP: ATOSHELL.SH ran succesfully.\n", shndl->self_pid);
    else 
        DEBUG_PRINTF("[SHELL %d] SHELL_LOOP: ATOSHELL.SH didn't run succesfully.\n", shndl->self_pid);    
    PUT_SHELL_START();

    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] SHELL_LOOP: Entering main loop.\n", shndl->self_pid);
    // -------------
    
    while(1) {
        switch (shndl->state) {
            // case STATE_CMD_INTERFACE:
                // --- DEBUG ---
                // -------------
                // CMD_INTERFACE_LOOP();
                // break;
            case STATE_EDIT_LINE:
            default:
                // --- DEBUG ---
                // -------------
                EDIT_LINE_LOOP();
                break;
        }
    }
}


BOOLEAN CREATE_STDOUT(U32 borrowers_pid) {
    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] CREATE_STDOUT: Attempting to create STDOUT for PID %u.\n", shndl->self_pid, borrowers_pid);
    // -------------
    if (shndl->stdout_count >= MAX_STDOUT_BUFFS) {
        // --- DEBUG ---
        DEBUG_PRINTF("[SHELL %d] CREATE_STDOUT: Failed - Max buffers reached.\n", shndl->self_pid);
        // -------------
        return FALSE;
    }

    STDOUT *alloc = MAlloc(sizeof(STDOUT));
    if (!alloc) {
        // --- DEBUG ---
        DEBUG_PRINTF("[SHELL %d] CREATE_STDOUT: Failed - MAlloc failed.\n", shndl->self_pid);
        // -------------
        return FALSE;
    }

    alloc->borrowers_pid = borrowers_pid;
    alloc->owner_pid = PROC_GETPID();
    alloc->proc_seq = 0;
    alloc->shell_seq = 0;
    MEMZERO(alloc->buf, STDOUT_MAX_LENGTH);
    shndl->stdouts[shndl->stdout_count++] = alloc;

    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] CREATE_STDOUT: Success for PID %u.\n", shndl->self_pid, borrowers_pid);
    // -------------
    return TRUE;
}

BOOLEAN DELETE_STDOUT(U32 borrowers_pid) {
    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] DELETE_STDOUT: Attempting to delete STDOUT for PID %u.\n", shndl->self_pid, borrowers_pid);
    // -------------
    for (U32 i = 0; i < shndl->stdout_count; i++) {
        if (shndl->stdouts[i]->borrowers_pid == borrowers_pid) {
            // MFree the memory for this STDOUT
            MFree(shndl->stdouts[i]);
            shndl->stdouts[i] = NULL;

            // Shift remaining stdouts down
            for (U32 j = i; j < shndl->stdout_count - 1; j++) {
                shndl->stdouts[j] = shndl->stdouts[j + 1];
            }

            shndl->stdouts[--shndl->stdout_count] = NULL;
            // --- DEBUG ---
            DEBUG_PRINTF("[SHELL %d] DELETE_STDOUT: Success for PID %u.\n", shndl->self_pid, borrowers_pid);
            // -------------
            return TRUE;
        }
    }
    // --- DEBUG ---
    DEBUG_PRINTF("[SHELL %d] DELETE_STDOUT: Failed - PID %u not found.\n", shndl->self_pid, borrowers_pid);
    // -------------
    return FALSE;
}

STDOUT* GET_STDOUT(U32 borrowers_pid) {
    for (U32 i = 0; i < shndl->stdout_count; i++) {
        if (shndl->stdouts[i]->borrowers_pid == borrowers_pid)
            return shndl->stdouts[i];
    }
    return NULLPTR;
}