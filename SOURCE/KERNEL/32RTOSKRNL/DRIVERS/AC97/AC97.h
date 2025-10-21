#ifndef AC97_DRIVER_H
#define AC97_DRIVER_H

#include <STD/TYPEDEF.h>

BOOLEAN AC97_INIT(VOID);
BOOLEAN AC97_STATUS(VOID);
VOID AC97_STOP(VOID);
VOID AC97_HANDLER(U32 vector, U32 errcode);

// Simple PCM playback helper: 16-bit little-endian, stereo
// If pcm == NULL, generates a short 440 Hz tone for smoke test
BOOLEAN AC97_PLAY(const U16* pcm, U32 frames, U8 channels, U32 rate);

// Generate and play a tone at frequency Hz for duration ms.
// rate: sample rate (e.g., 48000). amp: amplitude (0..32767 typical)
BOOLEAN AC97_TONE(U32 freq, U32 duration_ms, U32 rate, U16 amp);


#endif // AC97_DRIVER_H
