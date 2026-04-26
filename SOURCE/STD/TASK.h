#ifndef STD_TASK_H
#define STD_TASK_H

#include <STD/TYPEDEF.h>
#include <CPU/YIELD/YIELD.h>

static inline VOID YIELD_RAW(VOID) {
    __asm__ volatile (
        "int $0x81"
    );
}

#define YIELD() YIELD_RAW()

#endif
