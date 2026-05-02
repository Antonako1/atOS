#ifndef AC97_DRIVER_H
#define AC97_DRIVER_H

#include <STD/TYPEDEF.h>

BOOLEAN AC97_INIT(VOID);
BOOLEAN AC97_STATUS(VOID);
VOID    AC97_STOP(VOID);
VOID    AC97_HANDLER(U32 vector, U32 errcode);

// Play 8-bit unsigned mono PCM at 22050 Hz (resampled to 48000 Hz internally)
BOOLEAN AC97_PLAY8(const U8* pcm, U32 frames);

// Play 16-bit signed stereo PCM at 44800 Hz (resampled to 48000 Hz internally)
BOOLEAN AC97_PLAY16(const U16* pcm, U32 frames);

// Adds a new synthesizer voice to the mixer
BOOLEAN AC97_TONE(U32 freq, U32 duration_ms, U32 rate, U16 amp);

#endif