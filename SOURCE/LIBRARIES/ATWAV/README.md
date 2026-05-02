# ATWAV — atOS's Audio Library

ATWAV is a simple audio library for atOS, designed to provide basic audio playback capabilities. It supports loading and playing WAV files, making it suitable for use in games and multimedia applications.

## Features

- Load and play WAV audio files
- Simple API for audio playback
- Designed for ease of use in atOS applications
- Supports:
  - 8-bit 22050 Hz, Mono PCM WAV files 
  - 16-bit 44800 Hz, Stereo PCM WAV files

To convert audio files to the supported WAV format, you can use tools like CloudConvert or FFmpeg. For example, with FFmpeg, you can run the following command:

```ffmpeg -i input_file.ext -c:a pcm_u8 -ac 1 -ar 22050 output_file.wav```
```ffmpeg -i input_file.ext -c:a pcm_s16le -ac 2 -ar 44800 output_file.wav```

In [CloudConvert](https://cloudconvert.com/wav-to-wav), select your audio file, choose WAV as the output format, and set the following options:

- Audio Codec: pcm_u8
- Audio Bit Rate: 176 kbps (8 bits * 22050 samples/sec * 1 channel / 1000)
- Channels: Mono
- Sample Rate: 22050 Hz
- Volume: (optional, adjust as needed)

- Audio Codec: pcm_s16le
- Audio Bit Rate: 1434 kbps (16 bits * 44800 samples/sec * 2 channels / 1000)
- Channels: Stereo
- Sample Rate: 44800 Hz
- Volume: (optional, adjust as needed)

## Usage