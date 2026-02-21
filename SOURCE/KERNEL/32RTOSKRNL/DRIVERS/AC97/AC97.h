#ifndef AC97_DRIVER_H
#define AC97_DRIVER_H

#include <STD/TYPEDEF.h>

BOOLEAN AC97_INIT(VOID);
BOOLEAN AC97_STATUS(VOID);
VOID    AC97_STOP(VOID);
VOID    AC97_HANDLER(U32 vector, U32 errcode);

// Plays PCM data by adding it to the mixer stream
BOOLEAN AC97_PLAY(const U16* pcm, U32 frames, U8 channels, U32 rate);

// Adds a new synthesizer voice to the mixer
BOOLEAN AC97_TONE(U32 freq, U32 duration_ms, U32 rate, U16 amp);

#endif