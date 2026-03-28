/*
 * INPUTS.c — Keyboard event handler for TSHELL
 *
 * Uses ATUI_GETCH_NB() for non-blocking key polling.
 * Adds Shift+PageUp/PageDown for scrollback navigation.
 */

#include "TSHELL.h"
#include <STD/STRING.h>
#include <STD/IO.h>
#include <STD/PROC_COM.h>
#include <STD/MEM.h>
#include <STD/DEBUG.h>

VOID HANDLE_KB_EDIT_LINE(KEYPRESS *kp, MODIFIERS *mod) {
    if (!kp->pressed) return;
    TSHELL_INSTANCE *shndl = GET_SHNDL();
    if (!shndl->active_kb) return;

    switch (kp->keycode) {
    case KEY_ENTER:     HANDLE_LE_ENTER(); break;
    case KEY_BACKSPACE: HANDLE_LE_BACKSPACE(); break;
    case KEY_DELETE:    HANDLE_LE_DELETE(); break;
    case KEY_HOME:      HANDLE_LE_HOME(); break;
    case KEY_END:       HANDLE_LE_END(); break;

    case KEY_ARROW_DOWN:
        HANDLE_LE_ARROW_DOWN();
        break;
    case KEY_ARROW_UP:
        HANDLE_LE_ARROW_UP();
        break;

    case KEY_ARROW_LEFT:
        if (mod->ctrl) HANDLE_LE_CTRL_LEFT();
        else           HANDLE_LE_ARROW_LEFT();
        break;

    case KEY_ARROW_RIGHT:
        if (mod->ctrl) HANDLE_LE_CTRL_RIGHT();
        else           HANDLE_LE_ARROW_RIGHT();
        break;

    case KEY_PAGEUP:
        if (mod->shift) {
            TPUT_SCROLL_UP(AMOUNT_OF_ROWS / 2);
        }
        break;

    case KEY_PAGEDOWN:
        if (mod->shift) {
            TPUT_SCROLL_DOWN(AMOUNT_OF_ROWS / 2);
        }
        break;

    default:
        if (mod->ctrl) {
            switch (kp->keycode) {
            case KEY_A: HANDLE_LE_CTRL_A(); break;
            case KEY_E: HANDLE_LE_CTRL_E(); break;
            case KEY_U: HANDLE_LE_CTRL_U(); break;
            case KEY_K: HANDLE_LE_CTRL_K(); break;
            case KEY_W: HANDLE_LE_CTRL_W(); break;
            case KEY_Y: HANDLE_LE_CTRL_Y(); break;
            case KEY_L: HANDLE_LE_CTRL_L(); break;
            case KEY_C: HANDLE_CTRL_C();    break;
            default: break;
            }
        } else {
            HANDLE_LE_DEFAULT(kp, mod);
        }
        break;
    }

    /* Always refresh cursor after any key */
    BLINK_CURSOR();
}
