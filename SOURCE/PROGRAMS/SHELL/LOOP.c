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


/**
 * CMD INTERFACE
 */
U0 CMD_INTERFACE_MSG_LOOP() {
    U32 msg_count = MESSAGE_AMOUNT();
    for(U32 i = 0; i <= msg_count; i++) {
        PROC_MESSAGE *msg = GET_MESSAGE();
        if (!msg) break;
        switch(msg->type) {
            case PROC_MSG_KEYBOARD:
                if (msg->data_provided && msg->data) {
                    KEYPRESS *kp = (KEYPRESS *)msg->data;
                    MODIFIERS *mods = (MODIFIERS *)((U8 *)msg->data + sizeof(KEYPRESS));
                    HANDLE_KB_CMDI(kp, mods);
                }
                break;
        }
        FREE_MESSAGE(msg);
    }
}

/**
 * Line edit
 */

U0 CMD_INTERFACE_LOOP() {
    CMD_INTERFACE_MSG_LOOP();
    BLINK_CURSOR();
}

U0 EDIT_LINE_MSG_LOOP() {
    U32 msg_count = MESSAGE_AMOUNT();
    for (U32 i = 0; i < msg_count; i++) {
        PROC_MESSAGE *msg = GET_MESSAGE();
        if (!msg) break;

        if (msg->type == PROC_MSG_KEYBOARD && msg->data_provided && msg->data) {
            KEYPRESS *kp = (KEYPRESS *)msg->data;
            MODIFIERS *mods = (MODIFIERS *)((U8 *)msg->data + sizeof(KEYPRESS));
            HANDLE_KB_EDIT_LINE(kp, mods);
        }
        FREE_MESSAGE(msg);
    }
}


U0 EDIT_LINE_LOOP() {
    EDIT_LINE_MSG_LOOP();
    BLINK_CURSOR();
}

/**
 * PROC MODE
 */