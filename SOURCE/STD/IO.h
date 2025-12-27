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
#include <DRIVERS/PS2/KEYBOARD.h> // For definitions

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
KP_DATA* kp = get_latest_keypress();
if(valid_kp(kp)) {
    // Handle new keypress

    update_kp_seq(kp);
}
*/
KP_DATA *get_kp_data();
KEYPRESS *get_last_keypress();
KEYPRESS *get_latest_keypress();
KEYPRESS *get_latest_keypress_unconsumed();
MODIFIERS *get_modifiers();
U32 get_kp_seq();
U8 keypress_to_char(U32 kcode);
VOID update_kp_seq(KP_DATA *kp);
BOOLEAN valid_kp(KP_DATA *kp);
#endif // IO_H
