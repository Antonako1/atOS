#ifndef ATWAV_H
#define ATWAV_H

#include <STD/TYPEDEF.h>
#include <STD/FS_DISK.h>

typedef enum {
    WAV_MODE_PCM_16BIT_STEREO_48K = 1,
    WAV_MODE_PCM_8BIT_MONO_22050 = 2,
} WAV_AUDIO_MODE;

#define WAV22050_RATE 22050
#define WAV48000_RATE 48000

typedef struct {
    FILE *file; // File handle for the opened WAV file
    U32 audio_bytes_length; // Total length of audio data in bytes (from WAV header)
    U32 bytes_played; // Number of audio bytes that have been played so far
    U32 loop_count; // Number of times to loop the audio (0 for no looping, 1 for play once, etc.)
    U32 current_loop; // Current loop iteration (starts at 0)
    
    U8 volume; // Playback volume (0-100)
    I8 pan; // Playback pan (-100 left to 100 right)
    WAV_AUDIO_MODE mode; // Audio format mode (e.g. PCM 16-bit stereo, PCM 8-bit mono)
    U32 audio_data_offset; // Byte offset in the file where PCM data begins (after all chunk headers)
    VOIDPTR pcm_buf;       // Heap buffer holding the PCM data currently being streamed (must stay alive until playback ends)
} WAV_AUDIO_STREAM;

/// @brief Opens a WAV file and prepares it for streaming. Caller is responsible for closing the stream with WAV_CLOSE when done.
/// @param path Path to the WAV file to open
/// @return A pointer to a WAV_AUDIO_STREAM on success, or NULLPTR on failure (e.g. file not found, invalid format, unsupported features)
WAV_AUDIO_STREAM* WAV_OPEN(PU8 path);
VOID WAV_CLOSE(WAV_AUDIO_STREAM* stream);

/// @brief Starts playback of the given WAV audio stream. Returns immediately; playback continues asynchronously.
/// @param stream Pointer to an open WAV_AUDIO_STREAM returned by WAV_OPEN
VOID WAV_PLAY(WAV_AUDIO_STREAM* stream);

/// @brief Checks if the given WAV audio stream is still playing (i.e. has not reached the end of the audio data or completed all loops).
/// @param stream Pointer to an open WAV_AUDIO_STREAM returned by WAV_OPEN
/// @return TRUE if the stream is still playing, FALSE if it has finished.
BOOLEAN WAV_IS_STREAM_PLAYING(WAV_AUDIO_STREAM* stream);


U32 WAV_ELAPSED_SECONDS(WAV_AUDIO_STREAM* stream);
U32 WAV_TOTAL_SECONDS(WAV_AUDIO_STREAM* stream);

#endif // ATWAV_H