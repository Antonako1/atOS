/*+++
    SOURCE/STD/COMPORT.h - Basic COM port I/O functions

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

    NOTE: COM1 is for debug logging only, see STD/DEBUG.h
---*/
#ifndef STD_COMPORT_H
#define STD_COMPORT_H
#include <STD/TYPEDEF.h>
#include <DRIVERS/SERIAL/SERIAL.h> // For the COM port definitions

VOID COM_PORT_WRITE_BYTE(U16 port, U8 data); 
VOID COM_PORT_WRITE_DATA(U16 port, PU8 data, U32 len); 
VOID COM_PORT_READ_BYTE(U16 port, U8 data); 
VOID COM_PORT_READ_STRING(U16 port, PU8 data, U32 max_len); 
VOID COM_PORT_READ_BUFFER(U16 port, PU8 data, U32 max_len); 
PU8 COM_PORT_READ_BUFFER_HEAP(U16 port, U32 *out_len); 
PU8 COM_PORT_READ_STRING_HEAP(U16 port, U32 *out_len);

#define TURN_TO_COM_PORT(NAME, PORT) \
static inline VOID NAME##_WRITE_BYTE(U8 data) { COM_PORT_WRITE_BYTE(PORT, data); } \
static inline VOID NAME##_WRITE_DATA(PU8 data, U32 len) { COM_PORT_WRITE_DATA(PORT, data, len); } \
static inline VOID NAME##_READ_BYTE(U8 *data) { COM_PORT_READ_BYTE(PORT, *data); } \
static inline VOID NAME##_READ_STRING(U8 *data, U32 max_len) { COM_PORT_READ_STRING(PORT, data, max_len); } \
static inline VOID NAME##_READ_BUFFER(U8 *data, U32 max_len) { COM_PORT_READ_BUFFER(PORT, data, max_len); } \
static inline PU8 NAME##_READ_BUFFER_HEAP(U32 *out_len) { return COM_PORT_READ_BUFFER_HEAP(PORT, out_len); } \
static inline PU8 NAME##_READ_STRING_HEAP(U32 *out_len) { return COM_PORT_READ_STRING_HEAP(PORT, out_len); }

TURN_TO_COM_PORT(COM2_PORT, 0x2F8)
TURN_TO_COM_PORT(COM3_PORT, 0x3E8)
TURN_TO_COM_PORT(COM4_PORT, 0x2E8)

#endif // STD_COMPORT_H