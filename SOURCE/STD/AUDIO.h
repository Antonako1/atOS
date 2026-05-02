/*+++
    SOURCE/STD/AUDIO.h - AC97 audio driver functions

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef AUDIO_H
#define AUDIO_H
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

// Returns TRUE while PCM audio (AUDIO_PLAY8 or AUDIO_PLAY16) is still streaming
BOOLEAN AUDIO_IS_PLAYING(void);

// Pause (TRUE) or resume (FALSE) without losing playback position
VOID AUDIO_PAUSE(BOOL pause);
BOOL AUDIO_IS_PAUSED(void);

// Returns the current frame index (48000 frames per second for 16-bit stereo)
// Divide by 48000 to get seconds
U32 AUDIO_GET_FRAME_POS(void);

// Returns the current frame index for 8-bit mono playback (22050 frames per second)
// Divide by 22050 to get seconds
U32 AUDIO_GET_8BIT_FRAME_POS(void);

// Fill bands[0..n-1] (n <= 8) with smoothed peak amplitude (0..32767) per time-slice.
// Call at ~10 Hz from a UI loop to feed a level/visualizer display.
BOOLEAN AUDIO_GET_VIZ(U32 *bands, U32 n);

// Plays raw PCM audio data using AC97 driver
// pcm    = pointer to PCM audio data (8-bit unsigned or 16-bit signed, depending on the function)
// frames = number of audio frames (not bytes; for stereo 16-bit, one frame is 4 bytes: 2 bytes left + 2 bytes right)
// Returns 1 if playback started successfully, 0 on failure (e.g. invalid parameters, driver error)
BOOLEAN AUDIO_PLAY8(const U8* pcm, U32 frames);

// Plays raw PCM audio data using AC97 driver
// pcm    = pointer to PCM audio data (16-bit signed)
// frames = number of audio frames (not bytes; for stereo 16-bit, one frame is 4 bytes: 2 bytes left + 2 bytes right)
// Returns 1 if playback started successfully, 0 on failure (e.g. invalid parameters, driver error)
BOOLEAN AUDIO_PLAY16(const U16* pcm, U32 frames);

#endif // AUDIO_H