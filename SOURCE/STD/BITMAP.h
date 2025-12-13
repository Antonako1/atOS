#ifndef BITMAP_H
#define BITMAP_H
#include <STD/TYPEDEF.h>

typedef struct {
    U8 *data;
    U32 size; // in bytes
} BITMAP;

VOID BITMAP_CREATE(U32 size, VOIDPTR buff_addr, BITMAP *bm);
U32 BITMAP_GET(BITMAP *bm, U32 index);
BOOLEAN BITMAP_SET(BITMAP *bm, U32 index, U32 state);


#endif // BITMAP_H