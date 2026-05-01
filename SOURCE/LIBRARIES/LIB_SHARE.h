// Define LIB_DEV for development functions from LIB_SHARE.c
#ifndef LIB_SHARE_H
#define LIB_SHARE_H

#include <MEMORY/MEMORY.h>
#include <PROC/PROC.h>

#ifdef LIB_DEV
#define LIB_SHARED_BASE MEM_LIBRARY_BASE
#define LIB_SHARED_END MEM_LIBRARY_END
#define LIB_MAX_SIZE (LIB_SHARED_END - LIB_SHARED_BASE)
#define LIB_SLOT_SIZE 0x1000000 // 16 MB per library slot
#define LIB_MAX_SLOTS (LIB_MAX_SIZE / LIB_SLOT_SIZE)

#ifndef LIB_MAX_FUNCS
#   define LIB_MAX_FUNCS 64
#endif

typedef struct {
    U32 lib_id;
    TCB *owner; // TCB of this library.
} LIBRARY;

typedef struct {
    U32 lib_id;
    void *funcs[LIB_MAX_FUNCS];
} LIB_API;

#endif // LIB_DEV

#endif // LIB_SHARE_H