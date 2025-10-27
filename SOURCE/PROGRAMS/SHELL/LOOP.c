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




/**
 * Line edit
 */


U0 EDIT_LINE_MSG_LOOP() {
    U32 msg_count = MESSAGE_AMOUNT();
    for (U32 i = 0; i < msg_count; i++) {
        PROC_MESSAGE *msg = GET_MESSAGE();
        if (!msg) break;

        switch (msg->type) {

            case SHELL_CMD_CREATE_STDOUT: {
                PROC_MESSAGE res;
                if (CREATE_STDOUT(msg->sender_pid)) {
                    STDOUT *ptr = GET_STDOUT(msg->sender_pid);
                    DEBUG_PUTS("STDOUT POINTER FOR ");
                    DEBUG_HEX32(msg->sender_pid);
                    DEBUG_PUTS(" AT ");
                    DEBUG_HEX32(ptr);
                    DEBUG_PUTS("\n");
                    res = CREATE_PROC_MSG_RAW(msg->sender_pid, SHELL_RES_STDOUT_CREATED, ptr, sizeof(STDOUT*), 0);
                    MEMORY_DUMP(&res, sizeof(PROC_MESSAGE));
                } else {
                    res = CREATE_PROC_MSG(msg->sender_pid, SHELL_RES_STDOUT_FAILED, NULL, 0, 0);
                }
                SEND_MESSAGE(&res);
            } break;


            case SHELL_CMD_ENDED_MYSELF: {
                STDOUT *proc_out = GET_STDOUT(msg->sender_pid);
                if (proc_out) {
                    // 🟢 Flush any remaining buffer before deletion
                    if (proc_out->buf[0] != '\0') {
                        PUTS(proc_out->buf);
                        PRINTNEWLINE();
                    }

                    // Now delete the stdout
                    DELETE_STDOUT(msg->sender_pid);
                }

                // Print termination message
                PUTS("Process pid 0x");
                PUT_HEX(msg->sender_pid);
                PUTS(" terminated with code 0x");
                PUT_HEX(msg->signal);
                PUTC('\n');

                // Restore shell prompt
                PUT_SHELL_START();

                // Return focus to shell
                PROC_MESSAGE res = CREATE_PROC_MSG(KERNEL_PID, PROC_MSG_SET_FOCUS, NULL, 0, PROC_GETPID());
                SEND_MESSAGE(&res);
            } break;

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
            MEMZERO(out->buf, STDOUT_MAX_LENGTH);
            out->buf_end = 0;
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