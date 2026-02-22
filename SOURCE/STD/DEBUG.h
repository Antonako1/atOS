/*+++
    SOURCE/STD/DEBUG.h - COM1 debug logging

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef STD_DEBUG_H
#define STD_DEBUG_H

#include <STD/TYPEDEF.h>

/*
 * User-space debug utilities.
 * Writes to QEMU/Bochs debug port (0xE9) and COM1 (0x3F8).
 * Visible in: -debugcon file:OUTPUT/DEBUG/debug.log
 */

#define DEBUG_PORT 0xE9
#define COM1_BASE  0x3F8

// Initialize serial output (COM1)
// Note: COM1 is used for debugging. COM2-4 are used for other purposes. See STD/COMPORT.h
/// @warning Not necessary to call this function
// void DEBUG_INIT(void);

// Write a single character
void DEBUG_PUTC(U8 c);

// Write a string (null-terminated)
void DEBUG_PUTS(U8 *s);

// Write a string. Line-end added automatically
void DEBUG_PUTS_LN(PU8 s);

// Print 32-bit hexadecimal
void DEBUG_HEX32(U32 value);

// Dump memory (simple hex view)
void MEMORY_DUMP(const void *addr, U32 length);

void DEBUG_PRINTF(PU8 fmt, ...);

// Put line end
// void DEBUG_NL();
#endif // STD_DEBUG_H
