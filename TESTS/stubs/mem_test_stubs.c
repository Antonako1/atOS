/* Stubs specifically for test_mem (compiles real MEM.c).
   MEM.c defines its own MAlloc/MFree/ReAlloc/CAlloc/MEMCPY/MEMSET/MEMMOVE/MEMCMP/MEMZERO
   via SYSCALL. We just need to provide SYSCALL_STUB and ON_EXIT. */

extern void* malloc(unsigned int size);
extern void  free(void* ptr);
extern void* realloc(void* ptr, unsigned int newSize);
extern void* calloc(unsigned int num, unsigned int size);

#include <STD/TYPEDEF.h>
#include <CPU/SYSCALL/SYSCALL.h>

unsigned long SYSCALL_STUB(unsigned long num, unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5) {
    (void)a3; (void)a4; (void)a5;
    switch (num) {
        case SYSCALL_KMALLOC:  return (unsigned long)malloc(a1);
        case SYSCALL_KFREE:    free((void*)a1); return 0;
        case SYSCALL_KREALLOC: return (unsigned long)realloc((void*)a1, a2);
        case SYSCALL_KCALLOC:  return (unsigned long)calloc(a1, a2);
        default: return 0;
    }
}

BOOL ON_EXIT(exit_func_t fn) { (void)fn; return TRUE; }
