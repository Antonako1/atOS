/*+++
    Source/STD/IO.h - Input/Output Definitions

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    Standard input/output definitions for atOS.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version. Contains standard I/O function declarations.

REMARKS
    Use this header to include standard I/O functions in your application.
    This file should NOT be included in kernel or low-level drivers, 
        as it may introduce unwanted dependencies and conflicts, and
        may increase binary size.

Function table:
    - puts
    - putc
---*/
#ifndef IO_H
#define IO_H


#include <STD/TYPEDEF.h>
#include <DRIVERS/PS2/KEYBOARD_MOUSE.h> // For definitions

#define line_end "\r\n"
void putc(U8 c);
void puts(U8 *str);


// Executes a batsh shell command
// cmd: Command string
void sys(PU8 cmd);

/**
 * Supports
 * 
 * %s
 * %c
 * %d
 * %[00-0x8]x
 * %%
 */
VOID printf(PU8 fmt, ...);

// handled by runtime
BOOLEAN KB_MS_INIT();

/*
Simple keyboard example

BOOL8 running = TRUE;
while(running){
    PS2_KB_DATA *kp = kb_poll();
    if(kp && kp->cur.pressed) {
        switch (kp->cur.keycode)
        {
        case KEY_ESC:
            running = FALSE;
            break;
        }
    }
}
*/
PS2_KB_DATA *kb_poll(void); // polls and returns latest keypress data
PS2_KB_DATA *kb_peek(void); // returns keypress data gotten after last poll
PS2_KB_DATA *kb_last(void); // return previous keypress data
MODIFIERS  *kb_mods(void); // returns modifier states

PS2_MOUSE_DATA *mouse_poll(void); // polls and returns latest mouse data
PS2_MOUSE_DATA *mouse_peek(void); // returns mouse data gotten after last poll
PS2_MOUSE_DATA *mouse_last(void); // return previous mouse data

U8 keypress_to_char(U32 kcode);

#endif // IO_H
