# CHANGELOG

The last entry here will be added to the build.iso.yml workflows summary. Dates are in YYYY.MM.DD format. 

## 2026.04.03 - 2026.04.04

### Astrac
See docs for more details: [AstraC Documentation](https://atos.antonako.net/astrac)

#### Assembler

Created the AstraC Assembler, a new assembler for atOS that compiles .ASM files to binary. It includes a multi-stage pipeline with tokenization, parsing, AST generation, verification, and code generation. The assembler supports labels, directives, and various operand types, and provides detailed error messages during verification.

#### Disassembler

Created the AstraC Disassembler, which takes atOS binary files and produces human-readable assembly (.DSM). It uses the same parsing and AST infrastructure as the assembler, but in reverse, to reconstruct assembly instructions from binary code.

### Bootloader 

Started working on VBR (volume boot record) bootloader.

### STD/STRING

- Added error-checking versions of string-to-number conversion functions (e.g. `ATOI_E`, `ATOF_E`) that return a boolean success value and output the result via pointer.

- STRI_REPLACE: Added a case-insensitive version of the string replace function.
- VSNPRINTF: Added a function for formatted string output with a va_list argument, similar to the standard C library function.
- SNPRINTF: Added a function for formatted string output to a buffer with size limit, similar to the standard C library function.

## 2026.04.05 - 2026.04.19

### AstaC Compiler
- Created the AstraC Compiler, which compiles .AC source files to assembly as of now. It includes a multi-stage pipeline with tokenization, parsing, AST generation, semantic analysis, and code generation. The compiler supports variables, functions, control flow statements, and basic data types. It also provides detailed error messages during semantic analysis.

- Added +,- support for VFORMAT

- Added chess 1v1 game to the atOS games collection, AI is still under development.

- Updated ATGL nodes to support more child nodes.

- Added AC_FH.h, a binary fileheader format for AstraC compiled files. It includes a magic number, version, and section headers for code and data.

## 2026.04.25 - 2026.05.xx

### PS/2 keyboard driver updates

[PR36](https://github.com/Antonako1/atOS/pull/36) - @retroaalto improved the PS/2 keyboard driver by adding a ring buffer to keyboard events to prevent event loss during high input rates.  

### YIELD

Added a 0x81 interrupt for yielding the current process's time slice voluntarily. This allows processes to give up the CPU when they are idle or waiting for something, improving overall system responsiveness and efficiency.

### Dynamic Libraries

Started working on dynamic library support in atOS.

### AMPLITE

Amplite is a AC97 tone generator program that allows users to create and play custom tones by adjusting parameters like amplitude, frequency, duration, and waveform type. It features a simple GUI with sliders and checkboxes for controlling these parameters, and it uses the AC97 audio interface to generate sound.

### Kernel

Added support for the 0x81 YIELD interrupt, allowing processes to voluntarily yield their time slice to improve multitasking performance.

Fixed loading FAT filesystem if one already exists, by properly checking for the presence of a valid FAT header and handling it accordingly.

### TSHELL

Tab autocompletes now pathnames for commands, binaryes and scripts in the current directory and path. This makes it easier to run commands without having to type the full name, especially for long filenames.

Fixed relative path handling in TSHELL, so that commands like `./myprogram` or `../otherdir/script` work correctly regardless of the current working directory. This improves usability and consistency when running programs from the shell.

Small bug fixes for the BATSH language, most features are now working as intended. 

## 2026.05.01 - 2026.05.03

### CPCD

Added the CPCD utility for copying files from a CD-ROM (ISO9660) to the hard drive. It supports copying large files efficiently and is designed to work with the FAT32 file system. The utility takes a source path inside the CD-ROM ISO image and a destination path on the hard drive, and performs the copy operation while handling the necessary filesystem interactions.

### ATWAV 

Added the ATWAV audio library for atOS, which provides basic audio playback capabilities for WAV files. It supports loading and playing WAV files in specific formats (8-bit 22050 Hz Mono and 16-bit 44800 Hz Stereo) and includes a simple API for audio playback. The library is designed for ease of use in atOS applications, particularly games and multimedia applications. Instructions for converting audio files to the supported WAV format using tools like CloudConvert or FFmpeg are also provided.

### FAT 

Added deferred-flush mode to the FAT filesystem implementation, allowing multiple FAT updates to be accumulated in memory and written to disk in a single batch operation. This can significantly improve performance when making many changes to the filesystem, such as when copying a large number of files from an ISO image. Also changed from ATA PIO to DMA for FAT reads and writes, which should further improve performance by allowing the CPU to do other work while the disk transfer is in progress.

### AC97 Audio

Added support for playing PCM audio data through the AC97 audio interface, with two new system calls: `AC97_PLAY8` for 8-bit mono audio and `AC97_PLAY16` for 16-bit stereo audio. These functions take a pointer to the PCM audio data and the number of frames to play, and return a boolean indicating success or failure. This allows applications to generate and play custom audio in real time, opening up possibilities for music, sound effects, and more immersive multimedia experiences on atOS.

### JAMMER

Added JAMMER, a WAV PCM audio player program for atOS that uses the new ATWAV library and AC97 audio support. JAMMER allows users to load and play WAV files in the supported formats, providing a simple interface for audio playback. It can be used to play music or sound effects in games and other applications on atOS, demonstrating the new audio capabilities of the system.

### SHIFT+ALT+<F1-F12>

Added support for switching between multiple virtual consoles using Shift+Alt+F1 through Shift+Alt+F12. This allows users to have multiple independent shell sessions or applications running in separate virtual consoles, improving multitasking and usability. Each virtual console maintains its own state, including the focused process and display output, allowing for a more flexible and powerful user experience on atOS.

### ATZP

Added ATZP, a simple ZIP file extractor utility for atOS. It supports extracting files from ZIP archives in the current directory, allowing users to easily access compressed files without needing to transfer them to another system for extraction. ATZP provides a command-line interface for specifying the ZIP file and the destination directory for extracted files, making it a convenient tool for managing compressed data on atOS.

### ZIP

ZIP is an utility program for managing ZIP archives using ATZP.

### ATHASH

Added ATHASH, a crypt library for atOS that provides base64 and sha1 encoding and decoding functions.

### HASH

Added a HASH utility for ATHASH testing, which allows users to compute and verify SHA-1 hashes of files and strings, as well as perform base64 encoding and decoding. This utility can be used for verifying file integrity, securely storing passwords, or any other use case where hashing is needed on atOS. It provides a simple command-line interface for specifying the input data and the desired operation (hashing or base64 encoding/decoding).

### TESTS

@retroaalto added a suite of tests for the std libraries in [PR](https://github.com/Antonako1/atOS/pull/38/)

## 2026.05.03 - 2026.05.xx