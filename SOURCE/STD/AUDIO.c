#include <STD/AUDIO.h>
#include <CPU/SYSCALL/SYSCALL.h>

U32 AUDIO_TONE(U32 freq, U32 ms, U32 amp, U32 rate) {
    return SYSCALL4(SYSCALL_AC97_TONE, freq, ms, amp, rate);
}

VOID AUDIO_STOP(void) {
    SYSCALL0(SYSCALL_AC97_STOP);
}

BOOLEAN AUDIO_IS_PLAYING(void) {
    return (BOOLEAN)SYSCALL0(SYSCALL_AC97_IS_PLAYING);
}

VOID AUDIO_PAUSE(BOOL pause) {
    SYSCALL1(SYSCALL_AC97_PAUSE, (U32)pause);
}

BOOL AUDIO_IS_PAUSED(void) {
    return (BOOL)SYSCALL0(SYSCALL_AC97_IS_PAUSED);
}

U32 AUDIO_GET_FRAME_POS(void) {
    return SYSCALL0(SYSCALL_AC97_GET_FRAME_POS);
}

BOOLEAN AUDIO_PLAY8(const U8* pcm, U32 frames) {
    return SYSCALL2(SYSCALL_AC97_PLAY8, (U32)pcm, frames);
}

BOOLEAN AUDIO_PLAY16(const U16* pcm, U32 frames) {
    return SYSCALL2(SYSCALL_AC97_PLAY16, (U32)pcm, frames);
}

U32 AUDIO_GET_8BIT_FRAME_POS(void) {
    return SYSCALL0(SYSCALL_AC97_GET_8BIT_FRAME_POS);
}

// bands[] must be at least n entries (max 8). Returns FALSE on bad args.
BOOLEAN AUDIO_GET_VIZ(U32 *bands, U32 n) {
    return (BOOLEAN)SYSCALL2(SYSCALL_AC97_GET_VIZ, (U32)bands, n);
}