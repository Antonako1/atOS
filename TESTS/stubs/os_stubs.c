/* Host implementations of OS heap and memory functions.
   We forward-declare libc functions to avoid pulling in host headers
   that define size_t before TYPEDEF.h is parsed. */

extern void* malloc(unsigned int size);
extern void  free(void* ptr);
extern void* realloc(void* ptr, unsigned int newSize);
extern void* calloc(unsigned int num, unsigned int size);

#include <STD/TYPEDEF.h>
#include <STD/MEM.h>

U0 *MAlloc(U32 size) { return malloc(size); }
VOID MFree(U0* ptr) { free(ptr); }
U0 *ReAlloc(U0* ptr, U32 newSize) { return realloc(ptr, newSize); }
U0 *CAlloc(U32 num, U32 size) { return calloc(num, size); }

U0 *MEMCPY(U0* dest, CONST U0* src, U32 size) {
    U8 *d = dest;
    CONST U8 *s = src;
    for (U32 i = 0; i < size; i++) d[i] = s[i];
    return dest;
}

U0 *MEMZERO(U0* dest, U32 size) {
    U8 *d = dest;
    for (U32 i = 0; i < size; i++) d[i] = 0;
    return dest;
}

/* Stub for ON_EXIT (declared in RUNTIME/RUNTIME.h) */
BOOL ON_EXIT(exit_func_t fn) { (void)fn; return TRUE; }
