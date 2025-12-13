#include <STD/BITMAP.h>

VOID BITMAP_CREATE(U32 size, VOIDPTR buff_addr, BITMAP *bm) {
    if (!bm || !buff_addr || size == 0) return;
    bm->data = (U8 *)buff_addr;
    bm->size = size;
    MEMZERO(bm->data, size);
}
U32 BITMAP_GET(BITMAP *bm, U32 index) {
    if (!bm || index >= bm->size * 8) return 0;
    return (bm->data[index / 8] >> (index % 8)) & 1;
}
BOOLEAN BITMAP_SET(BITMAP *bm, U32 index, U32 state) {
    if (!bm || index >= bm->size * 8) return FALSE;
    if (state) {
        bm->data[index / 8] |= (1 << (index % 8));
    } else {
        bm->data[index / 8] &= ~(1 << (index % 8));
    }
    return TRUE;
}
