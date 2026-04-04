/*+++
    SOURCE/PROGRAMS/ASTRAC/DISSASEMBLER/DISSASEMBLER.h - Disassembler definitions

    Part of AstraC

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef DISSASEMBLER_H
#define DISSASEMBLER_H

#include <ASTRAC.h>

/**
 * Disassembler state/context
 */
typedef struct {
    PU8 buffer;      /* Binary data to disassemble */
    U32 size;        /* Total size of buffer */
    U32 offset;      /* Current offset in buffer */
    U32 base_addr;   /* Load address (for labels) */
} DISASM_CTX;

/**
 * Disassemble binary file into a .DSM (Disassembled Assembly) file.
 * @return ASTRAC_RESULT
 */
ASTRAC_RESULT START_DISSASEMBLER();


#endif /* DISSASEMBLER_H */
