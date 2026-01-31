#ifndef SYSCALL_H
#define SYSCALL_H

#ifdef KERNEL_ENTRY
#error "Not to be used inside KERNEL.c"
#endif // KERNEL_ENTRY

#ifndef KERNEL_ENTRY
#include <STD/TYPEDEF.h>

#define SYSCALL_VECTOR 0x80

typedef U32 (*SYSCALL_HANDLER)(U32, U32, U32, U32, U32);

#include <STD/TYPEDEF.h>

typedef U32 (*SYSCALL_HANDLER)(U32, U32, U32, U32, U32);

/* generate enum */
#define SYSCALL_ENTRY(id, fn) id,
enum {
    #include <CPU/SYSCALL/SYSCALL_LIST.h>
    SYSCALL_MAX
};
#undef SYSCALL_ENTRY

/* prototypes (declare kernel functions) */
#define SYSCALL_ENTRY(id, fn) U32 fn(U32, U32, U32, U32, U32);
    #include <CPU/SYSCALL/SYSCALL_LIST.h>
#undef SYSCALL_ENTRY

/* user-space macros (unchanged) ... */
#define SYSCALL(num, a1, a2, a3, a4, a5) ({ \
    U32 ret; \
    __asm__ volatile ( \
        "int $0x80" \
        : "=a"(ret) \
        : "a"(num), "b"(a1), "c"(a2), "d"(a3), "S"(a4), "D"(a5) \
        : "memory" \
    ); \
    ret; \
})

#define SYSCALL0(num) SYSCALL(num, NULL, NULL, NULL, NULL, NULL)
#define SYSCALL1(num, a1) SYSCALL(num, a1, NULL, NULL, NULL, NULL)
#define SYSCALL2(num, a1, a2) SYSCALL(num, a1, a2, NULL, NULL, NULL)
#define SYSCALL3(num, a1, a2, a3) SYSCALL(num, a1, a2, a3, NULL, NULL)
#define SYSCALL4(num, a1, a2, a3, a4) SYSCALL(num, a1, a2, a3, a4, NULL)
#define SYSCALL5(num, a1, a2, a3, a4, a5) SYSCALL(num, a1, a2, a3, a4, a5)



#ifdef __RTOS__
U32 syscall_dispatcher(U32 num, U32 a1, U32 a2, U32 a3, U32 a4, U32 a5);
__attribute__((naked)) void isr_syscall(void);  
#endif // __RTOS__

#endif // KERNEL_ENTRY
#endif // SYSCALL_H