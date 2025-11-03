#include <STD/IO.h>
#include <STD/PROC_COM.h>
#include <STD/STRING.h>
#include <STD/MEM.h>

#include <CPU/SYSCALL/SYSCALL.h>
#include <DRIVERS/PS2/KEYBOARD.h> // For definitions
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
VOID printf(CHAR *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    VFORMAT(console_putch, NULL, fmt, args);
    va_end(args);
}

static U32 prev_seq ATTRIB_DATA = 0;
static KP_DATA *kp ATTRIB_DATA = NULLPTR;

KP_DATA *get_kp_data() {
    if(!kp) kp = SYSCALL0(SYSCALL_GET_KP_DATA);
    return kp;
}

KEYPRESS *get_last_keypress() {
    KP_DATA *kp = get_kp_data();
    if(!kp) return NULLPTR;
    return &kp->prev;
}
KEYPRESS *get_latest_keypress() {
    KP_DATA *kp = get_kp_data();
    if(!kp) return NULLPTR;
    if(prev_seq != kp->seq) {
        prev_seq = kp->seq;
        return &kp->cur;
    }
    return NULLPTR;
}
KEYPRESS *get_latest_keypress_unconsumed() {
    KP_DATA *kp = get_kp_data();
    if (!kp) return NULLPTR;
    return &kp->cur; // no seq check, always return latest
}

MODIFIERS *get_modifiers() {
    KP_DATA *kp = get_kp_data();
    if(!kp) return NULLPTR;
    return &kp->mods;
}
U32 get_kp_seq() {
    KP_DATA *kp = get_kp_data();
    if(!kp) return NULLPTR;
    return kp->seq;
}
U8 keypress_to_char(U32 kcode) {
    U8 c = (U8)SYSCALL(SYSCALL_KEYPRESS_TO_CHARS, kcode, 0, 0, 0, 0);
    return c;
}