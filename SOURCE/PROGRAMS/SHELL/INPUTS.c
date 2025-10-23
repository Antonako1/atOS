#include <PROGRAMS/SHELL/SHELL.h>
#include <STD/STRING.h>
#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/MEM.h>

VOID HANDLE_KB_CMDI(KEYPRESS *kp, MODIFIERS *mod) {
    if(!kp->pressed) return; // We only care about key presses, not releases
    
    switch (kp->keycode)
    {
    case KEY_ENTER: HANDLE_KEY_ENTER(); break;
    case KEY_BACKSPACE: HANDLE_BACKSPACE(); break;
    case KEY_DELETE: HANDLE_KEY_DELETE(); break;
    case KEY_ARROW_DOWN: HANDLE_KEY_ARROW_DOWN(); break;
    case KEY_ARROW_UP: HANDLE_KEY_ARROW_UP(); break;
    case KEY_ARROW_LEFT: HANDLE_KEY_ARROW_LEFT(); break;
    case KEY_ARROW_RIGHT: HANDLE_KEY_ARROW_RIGHT(); break;  
    case KEY_INSERT: TOGGLE_INSERT_MODE(); break;
    case KEY_C:
        if (mod->ctrl) {
            HANDLE_CTRL_C();
            break;
        }
    default:
        U8 c = keypress_to_char(kp->keycode);
        if (c) PUTC(c);
        break;
    }
}

VOID HANDLE_KB_EDIT_LINE(KEYPRESS *kp, MODIFIERS *mod) {
    if (!kp->pressed) return;

    switch (kp->keycode) {
        case KEY_INSERT: TOGGLE_INSERT_MODE(); break;
        case KEY_ENTER: HANDLE_LE_ENTER(); break;
        case KEY_BACKSPACE: HANDLE_LE_BACKSPACE(); break;
        case KEY_DELETE: HANDLE_LE_DELETE(); break;
        case KEY_ARROW_LEFT: HANDLE_LE_ARROW_LEFT(); break;
        case KEY_ARROW_RIGHT: HANDLE_LE_ARROW_RIGHT(); break;
        case KEY_ARROW_DOWN: HANDLE_LE_ARROW_DOWN(); break;
        case KEY_ARROW_UP: HANDLE_LE_ARROW_UP(); break;
        default: HANDLE_LE_DEFAULT(kp, mod); break;
    }
    HANDLE_LE_CURSOR();
}
