/*+++
    SOURCE/STD/TASK.h - Task handling

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef STD_TASK_H
#define STD_TASK_H

#include <STD/TYPEDEF.h>
#include <CPU/YIELD/YIELD.h>

static inline VOID YIELD_RAW(VOID) {
    __asm__ volatile (
        "int $0x81"
    );
}

// In the kernel entry point and RTOS, we don't want to yield since the scheduler 
//   isn't running yet or we're already in a critical section. In user processes, 
//   we want to yield to allow other processes to run.
#if !defined(__RTOS__) && !defined(__KERNEL_ENTRY__)
#define YIELD() YIELD_RAW()
#else
#define YIELD() ((void)0) // No-op in kernel entry or RTOS contexts
#endif 

#endif
