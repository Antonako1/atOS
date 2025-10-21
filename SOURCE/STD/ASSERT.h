#ifndef STD_ASSERT_H
#define STD_ASSERT_H

#include <STD/TYPEDEF.h>
#ifdef __RTOS__
#include <RTOSKRNL/RTOSKRNL_INTERNAL.h>
#endif // __RTOS__

#ifdef __cplusplus
extern "C" {
#endif
#ifndef __RTOS__
    #define ASSERT(expr) ((void)0)
#else
    #define ASSERT(expr) \
        do { \
            if ((expr) == FALSE) { \
                panic((const U8*)PANIC_TEXT("Assertion failed: " #expr), PANIC_INVALID_STATE); \
            } \
        } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif // STD_ASSERT_H
