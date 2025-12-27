#include <CPU/YIELD/YIELD.h>
#include <CPU/PIT/PIT.h>

void yield_handler(I32 vector, U32 errcode) {
    (void)vector; (void)errcode;
    // TCB *cur = get_current_tcb();
    // cur->info.request_yield = TRUE;
    // immediate_reschedule();
}