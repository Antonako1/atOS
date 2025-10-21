#ifndef AUDIO_H
#define AUDIO_H
#include <CPU/SYSCALL/SYSCALL.h>
#include <STD/TYPEDEF.h>

// Plays a simple tone using AC97 driver
// freq = frequency in Hz
// ms   = duration in milliseconds
// amp  = amplitude (0–65535, or 0–100 if normalized)
// rate = sample rate (e.g. 44100 or 48000)
//
// Returns 1 if successful, 0 if failed
U32 AUDIO_TONE(U32 freq, U32 ms, U32 amp, U32 rate);

// Stops current tone or playback
VOID AUDIO_STOP(void);

#endif // AUDIO_H