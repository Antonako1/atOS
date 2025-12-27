#include <CPU/YIELD/YIELD.h>
#include <CPU/PIT/PIT.h>

void yield_handler(I32 vector, U32 errcode) {
    (void)vector; (void)errcode;
    SCHEDULER_YIELD();
}