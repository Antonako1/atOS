/*
 * TSHELL.c — Main entry point for the ATUI-based terminal shell
 *
 * Initialises ATUI, creates a full-screen window, sets up the terminal
 * output engine, starts the BATSH processor, runs the startup script,
 * and enters the main event loop.
 */

#include "TSHELL.h"
#include <STD/STRING.h>
#include <STD/IO.h>
#include <STD/PROC_COM.h>
#include <STD/ASM.h>
#include <STD/MEM.h>
#include <STD/DEBUG.h>
#include <CPU/PIT/PIT.h>
#include <STD/TASK.h>


/* ---- Singleton shell handle ---- */
static TSHELL_INSTANCE *shndl ATTRIB_DATA = NULLPTR;

TSHELL_INSTANCE *GET_SHNDL(VOID) { return shndl; }

/* ===================================================
 * Shell initialization and main loop
 * =================================================== */
VOID INIT_TSHELL(VOID) {
    shndl = MAlloc(sizeof(TSHELL_INSTANCE));
    if (!shndl) return 1;
    MEMZERO(shndl, sizeof(TSHELL_INSTANCE));
    shndl->self_pid  = PROC_GETPID();
    shndl->stdout_count = 0;
    for(U32 i = 0; i < MAX_STDOUT_BUFFS; i++)
        shndl->stdouts[i] = NULLPTR;
    shndl->type = 2;
    shndl->focused_pid = shndl->self_pid;
    shndl->previously_focused_pid = shndl->focused_pid;
    shndl->active_kb = TRUE;
    shndl->active_keybinds = TAK_DEFAULT;
    shndl->state = TSTATE_EDIT_LINE;
    TCB *tcb = GET_CURRENT_TCB();
    if (tcb) {
        tcb->info.state |= TCB_STATE_INFO_CHILD_PROC_HANDLER;
        PROC_MESSAGE msg = CREATE_PROC_MSG(KERNEL_PID, PROC_RECHECK_STATE, NULL, 0, tcb->info.state);
        SEND_MESSAGE(&msg);
    }
}

/* ===================================================
 * STDOUT management (IPC ring buffers for child procs)
 * =================================================== */

BOOLEAN CREATE_STDOUT(U32 borrowers_pid) {
    DEBUG_PRINTF("[TSHELL %d] CREATE_STDOUT for PID %u\n", shndl->self_pid, borrowers_pid);
    if (shndl->stdout_count >= MAX_STDOUT_BUFFS) return FALSE;

    STDOUT_BUF *alloc = MAlloc(sizeof(STDOUT_BUF));
    if (!alloc) return FALSE;

    alloc->borrowers_pid = borrowers_pid;
    alloc->owner_pid     = PROC_GETPID();
    alloc->proc_seq      = 0;
    alloc->shell_seq     = 0;
    alloc->buf_end       = 0;
    MEMZERO(alloc->buf, STDOUT_MAX_LENGTH);
    shndl->stdouts[shndl->stdout_count++] = alloc;
    return TRUE;
}

BOOLEAN DELETE_STDOUT(U32 borrowers_pid) {
    for (U32 i = 0; i < shndl->stdout_count; i++) {
        if (shndl->stdouts[i]->borrowers_pid == borrowers_pid) {
            MFree(shndl->stdouts[i]);
            shndl->stdouts[i] = NULLPTR;
            for (U32 j = i; j < shndl->stdout_count - 1; j++)
                shndl->stdouts[j] = shndl->stdouts[j + 1];
            shndl->stdouts[--shndl->stdout_count] = NULLPTR;
            return TRUE;
        }
    }
    return FALSE;
}

STDOUT_BUF *GET_STDOUT(U32 borrowers_pid) {
    for (U32 i = 0; i < shndl->stdout_count; i++) {
        if (shndl->stdouts[i]->borrowers_pid == borrowers_pid)
            return shndl->stdouts[i];
    }
    return NULLPTR;
}

/* ===================================================
 * Process termination handler
 * =================================================== */

VOID END_PROC_SHELL(U32 pid, U32 exit_code, BOOL end_proc) {
    DEBUG_PRINTF("[TSHELL %d] END_PROC_SHELL for %d (code %d)\n",
                 PROC_GETPID(), pid, exit_code);

    PROC_MESSAGE msg;
    U32 self_pid = shndl->self_pid;

    if (GET_TCB_BY_PID(pid)->info.state & TCB_STATE_INFO_CHILD_PROC_HANDLER)
        return;

    /* Flush any remaining STDOUT */
    STDOUT_BUF *std = GET_STDOUT(pid);
    if (std) {
        if (std->buf[0] != NULLT && std->proc_seq != std->shell_seq) {
            PUTS(std->buf);
            PRINTNEWLINE();
            std->shell_seq = std->proc_seq;
        }
    }

    DELETE_STDOUT(pid);

    /* Tell kernel to kill the process */
    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_MSG_KILL_PROCESS, 0, 0, pid);
    SEND_MESSAGE(&msg);

    /* Reclaim focus */
    if (shndl->focused_pid == pid) {
        msg = CREATE_PROC_MSG(KERNEL_PID, PROC_MSG_SET_FOCUS, NULL, 0, self_pid);
        SEND_MESSAGE(&msg);
    }

    if (!end_proc)
        PUT_SHELL_START();

    U8 buf[32];
    ITOA_U(exit_code, buf, 10);
    SET_VAR("ERRORLEVEL", buf);
    TPUT_NEWLINE();
    PUT_SHELL_START();
}

/* ===================================================
 * STDOUT flush loop
 * =================================================== */

static U0 EDIT_LINE_STDOUT_LOOP(VOID) {
    BOOL prev = shndl->active_kb;
    shndl->active_kb = FALSE;

    for (U32 i = 0; i < shndl->stdout_count; i++) {
        STDOUT_BUF *out = shndl->stdouts[i];
        if (out->shell_seq != out->proc_seq) {
            U32 seq = out->proc_seq;
            PUTS(out->buf);

            if (seq == out->proc_seq) {
                out->shell_seq = seq;
                out->buf_end   = 0;
                out->buf[0]    = NULLT;
            }
            
            TPUT_NEWLINE();
            PUT_SHELL_START();
        }
    }

    shndl->active_kb = prev;
}

/* ===================================================
 * Message loop (IPC)
 * =================================================== */

static U0 EDIT_LINE_MSG_LOOP(VOID) {
    PROC_MESSAGE *msg;
    while ((msg = GET_MESSAGE()) != NULLPTR) {
        switch (msg->type) {
        case SHELL_CMD_EXECUTE_BATSH:
            if (msg->data_provided && msg->raw_data && msg->raw_data_size > 0) {
                PARSE_BATSH_INPUT(msg->raw_data, NULLPTR);
                MFree(msg->raw_data);
            }
            break;

        case SHELL_CMD_INFO_ARRAYS: {
            PROC_MESSAGE res;
            U32 sz  = sizeof(TSHELL_INSTANCE);
            PU8 data = (PU8)shndl;
            if (!data)
                res = CREATE_PROC_MSG_RAW(msg->sender_pid,
                        SHELL_RES_INFO_ARRAYS_FAILED, NULL, 0, 0);
            else
                res = CREATE_PROC_MSG_RAW(msg->sender_pid,
                        SHELL_RES_INFO_ARRAYS, data, sz, 0);
            SEND_MESSAGE(&res);
        } break;

        case PROC_KILL_SHELL_KRNL:
            EXIT_SHELL();
            for (;;) YIELD();
            break;

        case SHELL_CMD_CREATE_STDOUT: {
            PROC_MESSAGE res;
            if (CREATE_STDOUT(msg->sender_pid)) {
                STDOUT_BUF *ptr = GET_STDOUT(msg->sender_pid);
                res = CREATE_PROC_MSG_RAW(msg->sender_pid,
                        SHELL_RES_STDOUT_CREATED, ptr, sizeof(STDOUT_BUF *), 0);
            } else {
                res = CREATE_PROC_MSG(msg->sender_pid,
                        SHELL_RES_STDOUT_FAILED, NULL, 0, 0);
            }
            SEND_MESSAGE(&res);
        } break;

        case SHELL_CMD_ENDED_MYSELF:
            END_PROC_SHELL(msg->sender_pid, msg->signal, FALSE);
            break;

        case SHELL_CMD_SHELL_FOCUS:
            shndl->previously_focused_pid = shndl->focused_pid;
            shndl->focused_pid = msg->sender_pid;
            break;

        default:
            break;
        }

        FREE_MESSAGE(msg);
    }

    /* Process keyboard input via ATUI (non-blocking) */
    PS2_KB_DATA *kp = ATUI_GETCH_NB();
    if (kp) {
        HANDLE_KB_EDIT_LINE(&kp->cur, &kp->mods);
    }
}

/* ===================================================
 * Main loop tick
 * =================================================== */

VOID EDIT_LINE_LOOP(VOID) {
    EDIT_LINE_STDOUT_LOOP();
    EDIT_LINE_MSG_LOOP();
    BLINK_CURSOR();
}

/* ===================================================
 * Exit
 * =================================================== */

VOID EXIT_SHELL(VOID) {
    for (U32 i = 0; i < shndl->stdout_count; i++) {
        if (shndl->stdouts[i]) MFree(shndl->stdouts[i]);
    }

    if (shndl->tout) {
        if (shndl->tout->win) ATUI_DELWIN(shndl->tout->win);
        MFree(shndl->tout);
    }

    MFree(shndl);
    shndl = NULLPTR;

    ATUI_DESTROY();
    ENABLE_SHELL_KEYBOARD();

    PROC_MESSAGE msg;
    msg = CREATE_PROC_MSG(0, PROC_KILL_SHELL_PROC, NULL, 0, 0);
    SEND_MESSAGE(&msg);
    KILL_SELF();
    for (;;) YIELD();
}

/* ===================================================
 * Entry point
 * =================================================== */

CMAIN() {
    (void)argc;
    (void)argv;

    
    DISABLE_SHELL_KEYBOARD();
    ON_EXIT(EXIT_SHELL);

    INIT_TSHELL();

    /* --- Initialise ATUI in raw mode (no echo) --- */
    ATUI_INITIALIZE(NULLPTR, ATUIC_RAW);
    ATUI_CURSOR_SET(FALSE);

    /* --- Create full-screen window --- */
    ATUI_DISPLAY *disp = GET_DISPLAY();
    ATUI_WINDOW *win = ATUI_NEWWIN(disp->rows, disp->cols, 0, 0);
    if (!win) { EXIT_SHELL(); return 1; }

    /* --- Allocate and init terminal output --- */
    shndl->tout = MAlloc(sizeof(TERM_OUTPUT));
    if (!shndl->tout) { EXIT_SHELL(); return 1; }
    MEMZERO(shndl->tout, sizeof(TERM_OUTPUT));
    INIT_TOUTPUT(shndl, win);

    /* --- Initialise subsystems --- */
    INITIALIZE_DIR(shndl);
    SETUP_BATSH_PROCESSER();

    /* --- Run startup script --- */
    DEBUG_PRINTF("[TSHELL %d] Running startup script\n", shndl->self_pid);
    RUN_BATSH_SCRIPT("/ATOS/TSHELL.SH", 0, NULLPTR);
    PUT_SHELL_START();

    DEBUG_PRINTF("[TSHELL %d] Entering main loop\n", shndl->self_pid);

    /* --- Main loop --- */
    while (1) {
        EDIT_LINE_LOOP();
    }

    return 0;
}
