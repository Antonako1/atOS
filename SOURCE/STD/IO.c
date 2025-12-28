#include <STD/IO.h>
#include <STD/PROC_COM.h>
#include <STD/STRING.h>
#include <STD/MEM.h>

#include <CPU/SYSCALL/SYSCALL.h>
#include <DRIVERS/PS2/KEYBOARD_MOUSE.h> // For definitions
#include <PROGRAMS/SHELL/SHELL.h>
#include <STD/DEBUG.h>
#include <STD/ARG.h>

void putc(U8 c) {
    STDOUT *stdout = GET_PROC_STDOUT();
    if (!stdout) return;

    if (stdout->buf_end < STDOUT_MAX_LENGTH - 1) {
        stdout->buf[stdout->buf_end++] = c;
        stdout->buf[stdout->buf_end] = '\0';  // keep null-terminated
    }

    stdout->proc_seq++;
}

void puts(U8 *str) {
    STDOUT *stdout = GET_PROC_STDOUT();
    if (!stdout || !str) return;
    U32 len = STRNLEN(str, STDOUT_MAX_LENGTH);
    U32 space_left = STDOUT_MAX_LENGTH - 1 - stdout->buf_end;
    
    if (len > space_left)
    len = space_left;
    
    MEMCPY(&stdout->buf[stdout->buf_end], str, len);
    stdout->buf_end += len;
    stdout->buf[stdout->buf_end] = '\0'; // ensure null-terminated
    
    stdout->proc_seq++;
}
// For printf (direct console)
VOID console_putch(CHAR c, VOID *ctx) {
    (VOID)ctx;
    putc(c);
}
VOID printf(PU8 fmt, ...) {
    va_list args;
    va_start(args, fmt);
    VFORMAT(console_putch, NULL, fmt, args);
    va_end(args);
}

void sys(PU8 cmd) {
    U32 parent = PROC_GETPPID();
    PU8 cmd_dup = STRDUP(cmd); // Duplicate command for string sending. Will be freed by receiver
    if(!cmd_dup) return;
    PROC_MESSAGE msg = CREATE_PROC_MSG_RAW(parent, SHELL_CMD_EXECUTE_BATSH, cmd_dup, STRLEN(cmd_dup) + 1, 0);
    SEND_MESSAGE(&msg);
}


typedef struct {
    PS2_KB_MOUSE_DATA *shared;
    U32 last_kb_seq;
    U32 last_ms_seq;
} INPUT_CTX;

static INPUT_CTX input ATTRIB_DATA = {0};


PS2_KB_MOUSE_DATA *get_KB_MOUSE_DATA() {
    return input.shared;
}

BOOLEAN KB_MS_INIT() {
    if(!input.shared) 
        input.shared = SYSCALL0(SYSCALL_GET_KB_MOUSE_DATA);
    return input.shared != NULLPTR;
}

PS2_KB_DATA *kb_poll(void) {
    if (!input.shared) return NULLPTR;

    volatile PS2_KB_DATA *kb = &input.shared->kb;

    if (kb->seq != input.last_kb_seq) {
        input.last_kb_seq = kb->seq;
        return (PS2_KB_DATA *)&kb->cur;
    }
    return NULLPTR;
}

PS2_KB_DATA *kb_peek(void) {
    if (!input.shared) return NULLPTR;
    return &input.shared->kb.cur;
}

PS2_KB_DATA *kb_last(void) {
    if (!input.shared) return NULLPTR;
    return &input.shared->kb.prev;
}

MODIFIERS *kb_mods(void) {
    if (!input.shared) return NULLPTR;
    return &input.shared->kb.mods;
}

PS2_MOUSE_DATA *mouse_poll(void) {
    if (!input.shared) return NULLPTR;

    if (input.shared->ms.seq != input.last_ms_seq) {
        input.last_ms_seq = input.shared->ms.seq;
        return &input.shared->ms.cur;
    }
    return NULLPTR;
}

PS2_MOUSE_DATA *mouse_peek(void) {
    if(!input.shared) return NULLPTR;
    return &input.shared->ms.cur;
}
PS2_MOUSE_DATA *mouse_last(void) {
    if(!input.shared) return NULLPTR;
    return &input.shared->ms.prev;
}

U8 keypress_to_char(U32 kcode) {
    U8 c = (U8)SYSCALL(SYSCALL_KEYPRESS_TO_CHARS, kcode, 0, 0, 0, 0);
    return c;
}
