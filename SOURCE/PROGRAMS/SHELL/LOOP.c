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
#include <STD/DEBUG.h>
#include <STD/GRAPHICS.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <PROGRAMS/SHELL/VOUTPUT.h>
#include <CPU/PIT/PIT.h>


/**
 * CMD INTERFACE
 * 
 * 
 */
U0 CMD_INTERFACE_MSG_LOOP() {
    U32 msg_count = MESSAGE_AMOUNT();
    for(U32 i = 0; i <= msg_count; i++) {
        PROC_MESSAGE *msg = GET_MESSAGE();
        if (!msg) break;
        // switch(msg->type) {
        //     case PROC_MSG_KEYBOARD:
        //         if (msg->data_provided && msg->data) {
        //             KEYPRESS *kp = (KEYPRESS *)msg->data;
        //             MODIFIERS *mods = (MODIFIERS *)((U8 *)msg->data + sizeof(KEYPRESS));
        //             HANDLE_KB_CMDI(kp, mods);
        //         }
        //         break;
        // }
        FREE_MESSAGE(msg);
    }
    KEYPRESS *kp = (KEYPRESS *)get_latest_keypress();
    if(kp) {
        MODIFIERS *mods = (MODIFIERS *)get_modifiers();
        HANDLE_KB_EDIT_LINE(kp, mods);
    }
}


U0 CMD_INTERFACE_LOOP() {
    CMD_INTERFACE_MSG_LOOP();
    BLINK_CURSOR();
}


VOID END_PROC_SHELL(U32 pid, U32 exit_code, BOOL end_proc) {
    // Print termination message
    // PUTS("Process pid 0x");
    // PUT_HEX(pid);
    // PUTS(" terminated with code 0x");
    // PUT_HEX(exit_code);
    // PUTC('\n');
    DEBUG_PRINTF("[SHELL %d] Process end acknowledged\n", PROC_GETPID());
    // Restore shell prompt
    PROC_MESSAGE msg;
    U32 self_pid = GET_SHNDL()->self_pid;
    STDOUT*std = GET_STDOUT(pid);
    // flush framebuffer
    if(std) {
        BOOL prev = GetOutputHandle()->CURSOR_BLINK;
        GetOutputHandle()->CURSOR_BLINK = FALSE;
        if(std->buf[0] != NULLT && std->proc_seq != std->shell_seq) {
            PUTS(std->buf);
            PRINTNEWLINE();
        }
        GetOutputHandle()->CURSOR_BLINK = prev;
    }
    DELETE_STDOUT(pid);
    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_MSG_KILL_PROCESS, 0, 0, pid);
    SEND_MESSAGE(&msg);
    // Return focus to shell
    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_MSG_SET_FOCUS, NULL, 0, self_pid);
    SEND_MESSAGE(&msg);
    if(!end_proc) PUT_SHELL_START();
    U8 buf[32];
    ITOA_U(exit_code, buf, 10);
    SET_VAR("ERRORLEVEL", buf);
}

/**
 * Line edit
 */


U0 EDIT_LINE_MSG_LOOP() {
    U32 msg_count = MESSAGE_AMOUNT();
    PROC_MESSAGE *msg = NULLPTR;
    while ((msg = GET_MESSAGE()) != NULL) {
        switch (msg->type) {
            case SHELL_CMD_INFO_ARRAYS: {
                PROC_MESSAGE res;
                U32 sz = sizeof(SHELL_INSTANCE);
                PU8 data = GET_SHNDL();
                if(!data) res = CREATE_PROC_MSG_RAW(msg->sender_pid, SHELL_RES_INFO_ARRAYS_FAILED, NULL, 0, 0);
                else res = CREATE_PROC_MSG_RAW(msg->sender_pid, SHELL_RES_INFO_ARRAYS, data, sz, 0);
                SEND_MESSAGE(&res);
            } break;
            
            case SHELL_CMD_CREATE_STDOUT: {
                PROC_MESSAGE res;
                if (CREATE_STDOUT(msg->sender_pid)) {
                    STDOUT *ptr = GET_STDOUT(msg->sender_pid);
                    res = CREATE_PROC_MSG_RAW(msg->sender_pid, SHELL_RES_STDOUT_CREATED, ptr, sizeof(STDOUT*), 0);
                } else res = CREATE_PROC_MSG(msg->sender_pid, SHELL_RES_STDOUT_FAILED, NULL, 0, 0);
                
                SEND_MESSAGE(&res);
            } break;
            

            case SHELL_CMD_ENDED_MYSELF: {
                END_PROC_SHELL(msg->sender_pid, msg->signal, FALSE);
            } break;

            case SHELL_CMD_SHELL_FOCUS: {
                SHELL_INSTANCE *shndl = GET_SHNDL();
                shndl->previously_focused_pid = shndl->focused_pid;
                shndl->focused_pid = msg->sender_pid;
            } break;
            default: break;
        }

        FREE_MESSAGE(msg);
    }

    // Process keyboard input
    KEYPRESS *kp = (KEYPRESS *)get_latest_keypress();
    if (kp) {
        MODIFIERS *mods = (MODIFIERS *)get_modifiers();
        HANDLE_KB_EDIT_LINE(kp, mods);
    }
}

U0 EDIT_LINE_STDOUT_LOOP() {
    SHELL_INSTANCE *shndl = GET_SHNDL();
    for (U32 i = 0; i < shndl->stdout_count; i++) {
        STDOUT *out = shndl->stdouts[i];
        if (out->shell_seq != out->proc_seq) {
            PUTS(out->buf);
            out->shell_seq = out->proc_seq;
            out->buf_end = 0;
            out->buf[out->buf_end] = NULLT;
            PRINTNEWLINE();
        }
    }
}

U0 EDIT_LINE_LOOP() {
    EDIT_LINE_STDOUT_LOOP();
    EDIT_LINE_MSG_LOOP();
    BLINK_CURSOR();
}


/**
 * PROC MODE
 */