#include "ATWAV.h"
#include "WAV_HEADER.h"
#include <STD/DEBUG.h>
#include <STD/FS_DISK.h>
#include <STD/AUDIO.h>

WAV_AUDIO_STREAM* WAV_OPEN(PU8 path) {
    if (!path) return NULLPTR;

    FILE *file = FOPEN(path, MODE_FR);
    if (!file) return NULLPTR;

    /* Read and validate WAV header */
    WAV_HEADER header;
    FSEEK(file, 0);
    if (FREAD(file, &header, sizeof(WAV_HEADER)) != sizeof(WAV_HEADER)) {
        FCLOSE(file);
        DEBUG_PRINTF("[ATWAV] Failed to read WAV header from file: %s\n", path);
        return NULLPTR;
    }

    if (header.riff.chunk_id[0] != 'R' || header.riff.chunk_id[1] != 'I' ||
        header.riff.chunk_id[2] != 'F' || header.riff.chunk_id[3] != 'F' ||
        header.riff.format[0] != 'W' || header.riff.format[1] != 'A' ||
        header.riff.format[2] != 'V' || header.riff.format[3] != 'E' ||
        header.fmt.subchunk1_id[0] != 'f' || header.fmt.subchunk1_id[1] != 'm' ||
        header.fmt.subchunk1_id[2] != 't' || header.fmt.subchunk1_id[3] != ' ') {
        FCLOSE(file);
        DEBUG_PRINTF("[ATWAV] Invalid WAV header in file: %s\n", path);
        return NULLPTR;
    }

    /* If the chunk after fmt is LIST (or any non-data chunk), skip it and
     * read the real data chunk.  Repeat until we find "data" or EOF.       */
    while (header.data.subchunk2_id[0] != 'd' || header.data.subchunk2_id[1] != 'a' ||
           header.data.subchunk2_id[2] != 't' || header.data.subchunk2_id[3] != 'a') {
        /* Seek past this unknown chunk (id=4 bytes already consumed, skip body) */
        U32 skip = header.data.subchunk2_size;
        FSEEK(file, FTELL(file) + skip);
        if (FREAD(file, &header.data, sizeof(WAV_DATA_HEADER)) != sizeof(WAV_DATA_HEADER)) {
            FCLOSE(file);
            DEBUG_PRINTF("[ATWAV] Could not find data chunk in file: %s\n", path);
            return NULLPTR;
        }
    }

    /* For simplicity, only support PCM format 
        * with 2 channels, 48 kHz sample rate, 16-bit samples, 1434kbps.
        * with 1 channel, 22050 Hz sample rate, 8-bit samples, 176kbps.
    */
    if ((header.fmt.audio_format != 1
        || header.fmt.num_channels != 2
        || header.fmt.sample_rate != 48000
        || header.fmt.bits_per_sample != 16)
        && 
        (header.fmt.audio_format != 1
        || header.fmt.num_channels != 1
        || header.fmt.sample_rate != 22050
        || header.fmt.bits_per_sample != 8)
    ) {
        FCLOSE(file);
        DEBUG_PRINTF("[ATWAV] Unsupported WAV format in file: %s (format: %d, channels: %d, sample_rate: %d, bits_per_sample: %d)\n",
            path, header.fmt.audio_format, header.fmt.num_channels, header.fmt.sample_rate, header.fmt.bits_per_sample);
        return NULLPTR;
    }

    // At this point we have a valid WAV header and can prepare the stream for reading audio data.
    WAV_AUDIO_STREAM *stream = MAlloc(sizeof(WAV_AUDIO_STREAM));
    if (!stream) {
        FCLOSE(file);
        DEBUG_PRINTF("[ATWAV] Failed to allocate memory for WAV_AUDIO_STREAM\n");
        return NULLPTR;
    }
    stream->file = file;

    // The audio data starts immediately after the header (or after any skipped chunks).
    stream->audio_bytes_length = header.data.subchunk2_size;
    stream->audio_data_offset  = FTELL(file); // file cursor is now at first PCM byte
    stream->bytes_played = 0;
    stream->pcm_buf = NULLPTR;
    stream->loop_count = 0;
    stream->current_loop = 0;
    stream->volume = 100; // Default to max volume
    stream->pan = 0; // Default to center pan

    if(header.fmt.audio_format == 1 && header.fmt.num_channels == 2 && header.fmt.sample_rate == 48000 && header.fmt.bits_per_sample == 16) {
        stream->mode = WAV_MODE_PCM_16BIT_STEREO_48K;
    } else if (header.fmt.audio_format == 1 && header.fmt.num_channels == 1 && header.fmt.sample_rate == 22050 && header.fmt.bits_per_sample == 8) {
        stream->mode = WAV_MODE_PCM_8BIT_MONO_22050;
    } else {
        // This should never happen due to the earlier check, but just in case:
        FCLOSE(file);
        MFree(stream);
        DEBUG_PRINTF("[ATWAV] Unrecognized WAV format after validation in file: %s\n", path);
        return NULLPTR;
    }

    return stream;
}
VOID WAV_CLOSE(WAV_AUDIO_STREAM* stream) {
    if (!stream) return;
    AUDIO_STOP(); // ensure the driver is no longer referencing pcm_buf
    if (stream->pcm_buf) { MFree(stream->pcm_buf); stream->pcm_buf = NULLPTR; }
    if (stream->file) FCLOSE(stream->file);
    MFree(stream);
}

VOID WAV_PLAY(WAV_AUDIO_STREAM* stream) {
    if (!stream || !stream->file) return;

    U32 data_size = stream->audio_bytes_length;
    if (data_size == 0) return;

    /* Free any previous playback buffer (stop the driver first so it is no longer streaming it) */
    if (stream->pcm_buf) {
        AUDIO_STOP();
        MFree(stream->pcm_buf);
        stream->pcm_buf = NULLPTR;
    }

    /* Allocate a buffer for the raw PCM data — kept alive in the stream until playback ends */
    VOIDPTR buf = MAlloc(data_size);
    if (!buf) {
        DEBUG_PRINTF("[ATWAV] Failed to allocate PCM buffer (%u bytes)\n", data_size);
        return;
    }

    /* Seek to the start of audio data (right after all chunk headers) and read */
    FSEEK(stream->file, stream->audio_data_offset);
    U32 read = FREAD(stream->file, buf, data_size);
    if (read != data_size) {
        DEBUG_PRINTF("[ATWAV] Short read: expected %u bytes, got %u\n", data_size, read);
        MFree(buf);
        return;
    }

    /* Store the buffer in the stream — it MUST remain valid while the AC97 driver
     * is mixing from it.  Free it only when playback ends or in WAV_CLOSE.          */
    stream->pcm_buf = buf;

    BOOLEAN started = FALSE;
    if (stream->mode == WAV_MODE_PCM_16BIT_STEREO_48K) {
        /* One frame = 2 bytes L + 2 bytes R = 4 bytes */
        U32 frames = data_size / 4;
        started = AUDIO_PLAY16((const U16 *)buf, frames);

    } else if (stream->mode == WAV_MODE_PCM_8BIT_MONO_22050) {
        /* One frame = 1 byte */
        U32 frames = data_size;
        started = AUDIO_PLAY8((const U8 *)buf, frames);

    } else {
        DEBUG_PRINTF("[ATWAV] Cannot play audio stream with unrecognized mode: %d\n", stream->mode);
    }

    if (started) {
        stream->bytes_played = data_size;
    } else {
        /* Playback failed — safe to free immediately */
        MFree(stream->pcm_buf);
        stream->pcm_buf = NULLPTR;
    }
}

BOOLEAN WAV_IS_PLAYING(WAV_AUDIO_STREAM* stream) {
    if (!stream || !stream->pcm_buf) return FALSE;
    return AUDIO_IS_PLAYING();
}

BOOLEAN WAV_IS_STREAM_PLAYING(WAV_AUDIO_STREAM* stream) {
    if (!stream) return FALSE;
    return stream->bytes_played < stream->audio_bytes_length;
}

U32 WAV_ELAPSED_SECONDS(WAV_AUDIO_STREAM* stream) {
    if (!stream) return 0;
    if(stream->mode == WAV_MODE_PCM_8BIT_MONO_22050) {
        U32 frames_played = AUDIO_GET_8BIT_FRAME_POS(); // For 8-bit mono, the frame position is the same as byte position
        return frames_played / WAV22050_RATE;
    }
    U32 frames_played = AUDIO_GET_FRAME_POS() / WAV48000_RATE; // AC97 frame position is in terms of 48 kHz stereo frames, so divide by 48000 to get seconds
    return frames_played;
}
U32 WAV_TOTAL_SECONDS(WAV_AUDIO_STREAM* stream) {
    if (!stream) return 0;

    if (stream->mode == WAV_MODE_PCM_8BIT_MONO_22050)
        return stream->audio_bytes_length / WAV22050_RATE;          /* 1 byte per sample, 22050 samples/s */
    return stream->audio_bytes_length / (WAV48000_RATE * 4u);       /* 16-bit stereo: 4 bytes per sample frame */
}
