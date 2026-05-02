#ifndef WAV_HEADER_H
#define WAV_HEADER_H

#include <STD/TYPEDEF.h>

TYPEDEF STRUCT {
    U8  chunk_id[4];     // "RIFF"
    U32 chunk_size;      // Size of the entire file minus 8 bytes
    U8  format[4];       // "WAVE"
} ATTRIB_PACKED WAV_RIFF_HEADER;

TYPEDEF STRUCT {
    U8  subchunk1_id[4]; // "fmt "
    U32 subchunk1_size;  // 16 for PCM
    U16 audio_format;    // 1 for Uncompressed PCM
    U16 num_channels;    // 1 for Mono, 2 for Stereo
    U32 sample_rate;     // e.g. 48000
    U32 byte_rate;       // sample_rate * num_channels * bits_per_sample / 8
    U16 block_align;     // num_channels * bits_per_sample / 8
    U16 bits_per_sample; // e.g. 16
} ATTRIB_PACKED WAV_FMT_HEADER;

TYPEDEF STRUCT {
    U8  subchunk2_id[4]; // "data"
    U32 subchunk2_size;  // Size of the raw audio data
} ATTRIB_PACKED WAV_DATA_HEADER;

TYPEDEF STRUCT {
    WAV_RIFF_HEADER riff;
    WAV_FMT_HEADER  fmt;
    WAV_DATA_HEADER data;
} ATTRIB_PACKED WAV_HEADER;

#endif // WAV_HEADER_H