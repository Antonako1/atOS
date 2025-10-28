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

static BOOLEAN draw_access_granted ATTRIB_DATA = FALSE;
static BOOLEAN keyboard_access_granted ATTRIB_DATA = FALSE;
static SHELL_INSTANCE shndl ATTRIB_DATA = { 0 };
U0 SHELL_LOOP(U0);
U0 INITIALIZE_SHELL();

U0 _start(U32 argc, PU8 argv[]) {
    draw_access_granted = FALSE;
    keyboard_access_granted = FALSE;

    INITIALIZE_SHELL();
    INIT_SHELL_VOUTPUT();
    OutputHandle hndl = GetOutputHandle();
    if(hndl->Column != 0) {
        for(;;);
    }
    INITIALIZE_DIR(&shndl);
    
    SHELL_LOOP();
}

SHELL_INSTANCE *GET_SHNDL(VOID) {
    return &shndl;
}

U0 INITIALIZE_SHELL() {
    PROC_MESSAGE msg;

    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_GET_FRAMEBUFFER, NULL, 0, 0);
    SEND_MESSAGE(&msg);
    
    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_MSG_SET_FOCUS, NULL, 0, PROC_GETPID());
    SEND_MESSAGE(&msg);

    msg = CREATE_PROC_MSG(0, PROC_GET_KEYBOARD_EVENTS, NULL, 0, 0);
    SEND_MESSAGE(&msg);
    
    shndl.cursor = GetOutputHandle();
    shndl.focused_pid = PROC_GETPID();
    shndl.previously_focused_pid = shndl.focused_pid;
    shndl.aborted = FALSE;
    SWITCH_LINE_EDIT_MODE();
    while(1) {
        MSG_LOOP();
        if(draw_access_granted && keyboard_access_granted) break;
    }
}

U0 SWITCH_CMD_INTERFACE_MODE(VOID) {
    shndl.state = STATE_CMD_INTERFACE;
}

U0 SWITCH_LINE_EDIT_MODE() {
    shndl.state = STATE_EDIT_LINE;
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
                break;
            case PROC_FRAMEBUFFER_GRANTED:
                draw_access_granted = TRUE;
                break;
        }
        FREE_MESSAGE(msg);
    }
}

U0 SHELL_LOOP(U0) {
    U32 i = 0;
    U32 j = 0;
    U32 pass = 0;
    

    RUN_BATSH_SCRIPT("/ATOS/ATOSHELL.SH");
    PUT_SHELL_START();

    while(1) {
        switch (shndl.state) {
            case STATE_CMD_INTERFACE:
                CMD_INTERFACE_LOOP();
                break;
            case STATE_EDIT_LINE:
            default:
                EDIT_LINE_LOOP();
                break;
        }
    }
}


BOOLEAN CREATE_STDOUT(U32 borrowers_pid) {
    if (shndl.stdout_count >= MAX_STDOUT_BUFFS) return FALSE;

    STDOUT *alloc = MAlloc(sizeof(STDOUT));
    if (!alloc) return FALSE;

    alloc->borrowers_pid = borrowers_pid;
    alloc->owner_pid = PROC_GETPID();
    alloc->proc_seq = 0;
    alloc->shell_seq = 0;
    MEMZERO(alloc->buf, STDOUT_MAX_LENGTH);
    shndl.stdouts[shndl.stdout_count++] = alloc;
    return TRUE;
}

BOOLEAN DELETE_STDOUT(U32 borrowers_pid) {
    for (U32 i = 0; i < shndl.stdout_count; i++) {
        if (shndl.stdouts[i]->borrowers_pid == borrowers_pid) {
            // MFree the memory for this STDOUT
            MFree(shndl.stdouts[i]);
            shndl.stdouts[i] = NULL;

            // Shift remaining stdouts down
            for (U32 j = i; j < shndl.stdout_count - 1; j++) {
                shndl.stdouts[j] = shndl.stdouts[j + 1];
            }

            shndl.stdouts[--shndl.stdout_count] = NULL;
            return TRUE;
        }
    }
    return FALSE;
}

STDOUT* GET_STDOUT(U32 borrowers_pid) {
    for (U32 i = 0; i < shndl.stdout_count; i++) {
        if (shndl.stdouts[i]->borrowers_pid == borrowers_pid)
            return shndl.stdouts[i];
    }
    return NULLPTR;
}
