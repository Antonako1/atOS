#ifndef SYSCALL_H
#define SYSCALL_H

#include <STD/TYPEDEF.h>

/* Generate enum from syscall list */
#define SYSCALL_ENTRY(id, fn) id,
enum {
    #include <CPU/SYSCALL/SYSCALL_LIST.h>
    SYSCALL_MAX
};
#undef SYSCALL_ENTRY

/* Host stub: all args and return are unsigned long to preserve full 64-bit pointers */
extern unsigned long SYSCALL_STUB(unsigned long num, unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5);

#define SYSCALL(num, a1, a2, a3, a4, a5) SYSCALL_STUB(num, a1, a2, a3, a4, a5)
#define SYSCALL0(num) SYSCALL_STUB(num, 0, 0, 0, 0, 0)
#define SYSCALL1(num, a1) SYSCALL_STUB(num, a1, 0, 0, 0, 0)
#define SYSCALL2(num, a1, a2) SYSCALL_STUB(num, a1, a2, 0, 0, 0)
#define SYSCALL3(num, a1, a2, a3) SYSCALL_STUB(num, a1, a2, a3, 0, 0)
#define SYSCALL4(num, a1, a2, a3, a4) SYSCALL_STUB(num, a1, a2, a3, a4, 0)
#define SYSCALL5(num, a1, a2, a3, a4, a5) SYSCALL_STUB(num, a1, a2, a3, a4, a5)

#endif
