#include <STD/AUDIO.h>
#include <CPU/SYSCALL/SYSCALL.h>

U32 AUDIO_TONE(U32 freq, U32 ms, U32 amp, U32 rate) {
    return SYSCALL4(SYSCALL_AC97_TONE, freq, ms, amp, rate);
}

VOID AUDIO_STOP(void) {
    SYSCALL0(SYSCALL_AC97_STOP);
}
