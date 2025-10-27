#include <STD/IO.h>
#include <STD/PROC_COM.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <DRIVERS/PS2/KEYBOARD.h> // For definitions
#include <STD/MEM.h>

void putc(U8 c);
void puts(const U8 *str);
void cls(void);

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