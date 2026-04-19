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