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
