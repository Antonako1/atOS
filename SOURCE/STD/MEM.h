/*+++
    Source/STD/MEM.h - Memory Management Definitions

    Part of atOS-RT

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    Memory management definitions for atOS-RT.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/19 - Antonako1
        Initial version. Contains memory management definitions.

REMARKS
---*/
#ifndef MEM_H
#define MEM_H

#include "./ATOSMINDEF.h"

#define MEM_POOL_SIZE 1024 * 1024 // 1MB
#define MEM_BLOCK_SIZE 64

U32 memcpy(U8* dest, CONST U8* src, U32 size) {
    U32 i;
    for (i = 0; i < size; i++) {
        dest[i] = src[i];
    }
    return size;
}

#endif // MEM_H