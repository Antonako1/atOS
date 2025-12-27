/*+++
    SOURCE/STD/BITMAP.h - Basic bitmap

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef BITMAP_H
#define BITMAP_H
#include <STD/TYPEDEF.h>

typedef struct {
    U8 *data;
    U32 size; // in bytes
} BITMAP;

#define BITMAP_STATE_SET    1
#define BITMAP_STATE_UNSET  0

VOID BITMAP_CREATE(U32 size, VOIDPTR buff_addr, BITMAP *bm);
U32 BITMAP_GET(BITMAP *bm, U32 index);
BOOLEAN BITMAP_SET(BITMAP *bm, U32 index, U32 state);


#endif // BITMAP_H