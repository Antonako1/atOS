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

// Returns TRUE if PCM audio (play8 or play16) is still streaming
BOOLEAN AC97_IS_PLAYING(VOID);
// Pause (TRUE) or resume (FALSE) PCM streaming without losing position
VOID AC97_PAUSE(BOOL pause);
BOOL AC97_IS_PAUSED(VOID);

// Returns the current 48 kHz source frame index (divide by 48000 for seconds)
U32 AC97_GET_FRAME_POS(VOID);
U32 AC97_GET_8BIT_FRAME_POS(VOID);

// Fills 'bands[0..n-1]' with smoothed peak amplitude (0..32767) for each
// of the 8 time-slice energy buckets computed during the last DMA buffer fill.
// Useful for drawing a simple level/waveform visualizer. Returns FALSE if
// bands is NULL or n is 0. n is clamped to 8 internally.
BOOLEAN AC97_GET_VIZ(U32 *bands, U32 n);
#endif