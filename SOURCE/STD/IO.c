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




static U32 prev_seq_kb ATTRIB_DATA = 0;
static U32 prev_seq_ms ATTRIB_DATA = 0;
static PS2_KB_MOUSE_DATA *kp ATTRIB_DATA = NULLPTR;


PS2_KB_MOUSE_DATA *get_KB_MOUSE_DATA() {
    if(!kp) kp = SYSCALL0(SYSCALL_GET_KB_MOUSE_DATA);
    return kp;
}
PS2_KB_DATA *get_KB_DATA() {
    if(!kp) return NULLPTR;
    return &kp->kb;
}

BOOLEAN KB_MS_INIT() {
    kp = get_KB_MOUSE_DATA();
}

PS2_KB_DATA *get_last_keypress() {
    PS2_KB_MOUSE_DATA *kp = get_KB_MOUSE_DATA();
    if(!kp) return NULLPTR;
    return &kp->kb.prev;
}
PS2_KB_DATA *get_latest_keypress() {
    if(!kp) return NULLPTR;
    kp->kb_event = FALSE;
    if(prev_seq_kb != kp->kb.seq) {
        prev_seq_kb = kp->kb.seq;
        kp->kb_event = TRUE;
        return &kp->kb.cur;
    }
    return NULLPTR;
}
PS2_KB_DATA *get_latest_keypress_unconsumed() {
    PS2_KB_MOUSE_DATA *kp = get_KB_MOUSE_DATA();
    if (!kp) return NULLPTR;
    return &kp->kb.cur; // no seq check, always return latest
}

MODIFIERS *get_modifiers() {
    PS2_KB_MOUSE_DATA *kp = get_KB_MOUSE_DATA();
    if(!kp) return NULLPTR;
    return &kp->kb.mods;
}
U32 get_kp_seq() {
    PS2_KB_MOUSE_DATA *kp = get_KB_MOUSE_DATA();
    if(!kp) return NULLPTR;
    return kp->kb.seq;
}
U8 keypress_to_char(U32 kcode) {
    U8 c = (U8)SYSCALL(SYSCALL_KEYPRESS_TO_CHARS, kcode, 0, 0, 0, 0);
    return c;
}

void sys(PU8 cmd) {
    U32 parent = PROC_GETPPID();
    PU8 cmd_dup = STRDUP(cmd); // Duplicate command for string sending. Will be freed by receiver
    if(!cmd_dup) return;
    PROC_MESSAGE msg = CREATE_PROC_MSG_RAW(parent, SHELL_CMD_EXECUTE_BATSH, cmd_dup, STRLEN(cmd_dup) + 1, 0);
    SEND_MESSAGE(&msg);
}

VOID update_kp_seq(PS2_KB_MOUSE_DATA *data) {
    if (!kp) return;
    data->kb.seq++;
}

BOOLEAN valid_kp(PS2_KB_MOUSE_DATA *data) {
    if(kp) return TRUE;
    return FALSE;
}


PS2_MOUSE_DATA *get_MS_DATA() {
    if(kp) return &kp->ms;
    return NULLPTR;
}
BOOLEAN valid_ms(MOUSE_DATA *kp) {
    if(kp) return TRUE;
    return FALSE;
}
void update_ms_seq(PS2_MOUSE_DATA *kp) {
    if(!kp) return;
    kp->seq++;
}
U32 get_ms_seq() {
    if(!kp) return 0;
    return kp->ms.seq;
}
PS2_MOUSE_DATA *get_last_mousedata() {
    if(!kp) return NULLPTR;
    return &kp->ms.prev;
}
PS2_MOUSE_DATA *get_latest_mousedata() {
    if(!kp) return NULLPTR;
    kp->kb_event = TRUE;
    if(kp->ms.seq != prev_seq_ms) {
        prev_seq_ms = kp->ms.seq;
        kp->ms_event = TRUE;
        return &kp->ms.cur;
    }
    return NULLPTR;
}

BOOLEAN mouse_moved() {
    if(!kp) return FALSE;
    return kp->ms.prev.x != kp->ms.cur.x || kp->ms.prev.y != kp->ms.cur.y;
}
BOOLEAN mouse1_pressed() {
    if(!kp) return FALSE;
    return kp->ms.cur.mouse1;
}
BOOLEAN mouse2_pressed() {
    if(!kp) return FALSE;
    return kp->ms.cur.mouse2;

}
BOOLEAN mouse3_pressed() {
    if(!kp) return FALSE;
    return kp->ms.cur.mouse3;

}
BOOLEAN scrollwheel_moved() {
    if(!kp) return FALSE;
    return kp->ms.cur.scroll_wheel != kp->ms.prev.scroll_wheel;
}