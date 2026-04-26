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

Added support for dynamic libraries in atOS. Libraries can now be loaded at runtime, allowing for more modular and flexible software development.

### TSHELL

Tab autocompletes now pathnames for commands, binaryes and scripts in the current directory and path. This makes it easier to run commands without having to type the full name, especially for long filenames.

Fixed relative path handling in TSHELL, so that commands like `./myprogram` or `../otherdir/script` work correctly regardless of the current working directory. This improves usability and consistency when running programs from the shell.