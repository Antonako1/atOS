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

/*
Usage:
PS2_KB_MOUSE_DATA* kp = get_latest_keypress();
if(valid_kp(kp)) {
    // Handle new keypress

    update_kp_seq(kp); // Update sequence number
}
*/
BOOLEAN KB_MS_INIT();
PS2_KB_MOUSE_DATA *get_KB_MOUSE_DATA();

PS2_KB_DATA *get_KB_DATA();
PS2_KB_DATA *get_last_keypress();
PS2_KB_DATA *get_latest_keypress();
PS2_KB_DATA *get_latest_keypress_unconsumed();
MODIFIERS *get_modifiers();
U32 get_kp_seq();
U8 keypress_to_char(U32 kcode);
VOID update_kp_seq(PS2_KB_MOUSE_DATA *data);
BOOLEAN valid_kp(PS2_KB_MOUSE_DATA *data);

PS2_MOUSE_DATA *get_MS_DATA();
BOOLEAN valid_ms(MOUSE_DATA *data);
void update_ms_seq(PS2_MOUSE_DATA *data);
U32 get_ms_seq();
PS2_MOUSE_DATA *get_last_mousedata();
PS2_MOUSE_DATA *get_latest_mousedata();
BOOLEAN mouse1_pressed();
BOOLEAN mouse2_pressed();
BOOLEAN mouse3_pressed();
BOOLEAN scrollwheel_moved();

#endif // IO_H
