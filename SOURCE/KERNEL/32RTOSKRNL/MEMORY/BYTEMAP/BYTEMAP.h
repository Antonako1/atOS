#ifndef BYTEMAP_H
#define BYTEMAP_H

#include <STD/TYPEDEF.h>
#include <MEMORY/PAGEFRAME/PAGEFRAME.h>

// These are defined here to fix compiler issues with circular includes
typedef enum PageState {
    PS_FREE        = 0, // Free page
    PS_ALLOC       = 1, // Allocated by the allocator

    PS_RESERVED    = 2,  // firmware/MMIO or not for allocator
    PS_LOCKED      = 3,  // temporary pin

    PS_FUSER        = 4,  // Free process page
    PS_LUSER = 5,  // Locked user process page
    PS_AUSER = 6,  // Allocated user process page
    PS_RUSER = 7,  // Reserved user process page

    PS_FLIB = 9, // Free library page
    PS_LLIB = 10, // Locked library page
    PS_ALIB = 11, // Allocated library page
    PS_RLIB = 8, // Reserved library page
} PageState;

// Maximum BYTEMAP size to handle up to 4GB of RAM with 4KB pages
#define TOTAL_PAGES (0x100000000 / 0x1000) // 4GB / 4KB
#define MAX_BYTEMAP_SIZE (TOTAL_PAGES)

typedef struct {
    U8 *data;
    U32 size; // in bytes
} BYTEMAP;

VOID BYTEMAP_CREATE(U32 size, VOIDPTR buff_addr, BYTEMAP *bm);
PageState BYTEMAP_GET(BYTEMAP *bm, U32 index);
BOOLEAN BYTEMAP_SET(BYTEMAP *bm, U32 index, PageState state);

#endif // BYTEMAP_H