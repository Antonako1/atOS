# ASTRAC Assembler — Syntax & Usage Reference

Complete reference manual for the **ASTRAC** i386 flat-binary assembler built into atOS.

---

## Table of Contents

1. [Overview](#overview)
2. [Pipeline](#pipeline)
3. [Command-Line Usage](#command-line-usage)
4. [Source File Structure](#source-file-structure)
5. [Preprocessor](#preprocessor)
6. [Directives](#directives)
7. [Labels](#labels)
8. [Data Variables](#data-variables)
9. [Number Literals](#number-literals)
10. [String Literals](#string-literals)
11. [Registers](#registers)
12. [Operand Syntax](#operand-syntax)
13. [Instruction Reference](#instruction-reference)
14. [Encoding Details](#encoding-details)
15. [Error Reference](#error-reference)
16. [Examples](#examples)

---

## Overview

ASTRAC ASM is a flat-binary x86 assembler targeting the i386 (32-bit protected mode)
instruction set. It runs entirely inside atOS and produces raw binary output — no
ELF, PE, or other object format wrappers.

Key characteristics:

- **Intel syntax** (destination first: `MOV EAX, 1`)
- 8-bit, 16-bit, and 32-bit operand support
- 472 instruction table entries (186 unique mnemonics) covering the full i386 ISA
- C-style preprocessor (`#include`, `#define` with inline math, conditional compilation)
- Local labels (`@@N:`, `@f`, `@b`) scoped to enclosing global label
- BYTE/WORD/DWORD PTR size overrides
- Full ModR/M + SIB encoding with displacements

---

## Pipeline

The assembler runs a **six-stage pipeline** in order:

| # | Stage        | Description |
|---|--------------|-------------|
| 1 | Preprocess   | Resolve `#include`, `#define`, conditionals; strip comments |
| 2 | Lex          | Tokenise source into a flat token stream (up to 4096 tokens) |
| 3 | AST          | Parse tokens into structured AST nodes (up to 4096 nodes) |
| 4 | Verify       | Two-pass semantic validation (labels, types, sizes, bounds) |
| 5 | Optimize     | Peephole optimisation *(planned — currently a no-op stub)* |
| 6 | Generate     | Emit flat binary output with full x86 instruction encoding |

If any stage fails, the pipeline halts and reports an error code:

| Code | Name                   | Meaning |
|------|------------------------|---------|
| 0    | `ASTRAC_OK`            | Success |
| 1    | `ASTRAC_ERR_ARGS`      | Bad or missing arguments |
| 2    | `ASTRAC_ERR_PREPROCESS`| Preprocessor failure |
| 3    | `ASTRAC_ERR_LEX`       | Lexer failure |
| 4    | `ASTRAC_ERR_AST`       | AST construction failure |
| 5    | `ASTRAC_ERR_VERIFY`    | Verification failure |
| 6    | `ASTRAC_ERR_OPTIMIZE`  | Optimisation failure |
| 7    | `ASTRAC_ERR_CODEGEN`   | Code generation failure |

---

## Command-Line Usage

ASTRAC accepts the following arguments and flags:

| Flag | Description |
|------|-------------|
| `-o <file>` | Set output file path |
| `-v` | Verbose output (print progress, offsets, label addresses) |
| `-q` | Quiet mode (suppress non-fatal warnings) |
| `-I <path>` | Add include search path (up to 255) |
| `-D NAME=VALUE` | Define a preprocessor macro |

**Build types** (selected by the host compiler/shell):

| Mode | Description |
|------|-------------|
| `ASSEMBLE` | Assemble `.ASM` sources to flat binary |
| `COMPILE` | Compile C to assembly |
| `BUILD` | Compile + assemble |
| `DISASSEMBLE` | Disassemble binary to assembly |

**Limits:**

| Resource | Maximum |
|----------|---------|
| Input files | 255 |
| Include paths | 255 |
| Preprocessor macros | 255 |
| Macro name / value length | 255 characters |
| Tokens | 4096 |
| AST nodes | 4096 |
| Labels (verification) | 512 |
| Labels (code generation) | 1024 |

---

## Source File Structure

A typical source file contains sections, labels, data declarations, and instructions:

```asm
; Comment (stripped by preprocessor)

#include <header.INC>        ; Include another file
#define BUFFER_SIZE 256      ; Define a macro

.data                        ; Mutable data section
msg   DB "Hello", 0          ; Null-terminated string
count DD 0                   ; 32-bit integer

.rodata                      ; Read-only data section
magic DW 0xABCD              ; 16-bit constant

.code                        ; Code section
.use32                       ; 32-bit mode (default)

_start:                      ; Global label
    MOV EAX, 1
    ADD EAX, [count]
    JMP _start

.use16                       ; Switch to 16-bit mode
    MOV AX, BX
```

Lines are terminated by newlines. The semicolon `;` begins a line comment
(removed during preprocessing). Backslash `\` at end of line continues to
the next physical line.

---

## Preprocessor

The preprocessor runs before lexing and supports these directives:

| Directive | Syntax | Description |
|-----------|--------|-------------|
| `#include` | `#include <file>` | Include and inline another source file |
| `#define` | `#define NAME VALUE` | Define a macro (constant expressions are evaluated) |
| `#undef` | `#undef NAME` | Remove a macro definition |
| `#ifdef` | `#ifdef NAME` | Compile following block if macro is defined |
| `#ifndef` | `#ifndef NAME` | Compile following block if macro is **not** defined |
| `#elif` | `#elif` | Else-if branch in conditional block |
| `#else` | `#else` | Else branch in conditional block |
| `#endif` | `#endif` | End conditional block |
| `#error` | `#error message` | Emit a compile-time error and stop |
| `#warning` | `#warning message` | Emit a compile-time warning |

### Example

```asm
#define DEBUG 1
#define VERSION 2

#ifdef DEBUG
    INT3                     ; breakpoint
#else
    NOP
#endif

#ifndef RELEASE
#warning "Building in debug mode"
#endif
```

Macros defined with `-D` on the command line are available during preprocessing.
Macro expansion substitutes every occurrence of the macro name with its value
in all subsequent lines.

### Inline Math in `#define`

When a `#define` value is a pure constant integer expression, the preprocessor
evaluates it at define-time and stores the numeric result. Previously-defined
macros in the expression are expanded first.

Supported operators (in precedence order, lowest to highest):

| Precedence | Operators | Description |
|------------|-----------|-------------|
| 1 | `\|` | Bitwise OR |
| 2 | `^` | Bitwise XOR |
| 3 | `&` | Bitwise AND |
| 4 | `<<` `>>` | Shift left / right |
| 5 | `+` `-` | Addition / subtraction |
| 6 | `*` `/` `%` | Multiplication / division / modulo |
| 7 | `-` `+` `~` | Unary minus / plus / bitwise NOT |
| 8 | `(` `)` | Parenthesised sub-expression |

Numeric literal formats: decimal (`42`), hexadecimal (`0xFF`), binary (`0b1010`).

```asm
#define PAGE_SIZE   4096
#define PAGE_MASK   (PAGE_SIZE - 1)       ; evaluates to 4095
#define FLAGS       (0x01 | 0x04 | 0x10)  ; evaluates to 21
#define SECTOR_SZ   512
#define SECTORS     (SECTOR_SZ * 2)        ; evaluates to 1024
#define SHIFTED     (1 << 16)              ; evaluates to 65536
```

If the value contains non-numeric content (identifiers, strings, etc.), it is
stored as a literal text substitution as before.

---

## Directives

Section and mode directives are prefixed with a dot (`.`):

| Directive | Description |
|-----------|-------------|
| `.data` | Switch to the **mutable data** section |
| `.rodata` | Switch to the **read-only data** section |
| `.code` | Switch to the **code** (text) section |
| `.use32` | Set **32-bit** code mode (protected mode default) |
| `.use16` | Set **16-bit** code mode (real mode) |
| `.org ADDR` | Set the **origin address** (base offset for label calculations) |
| `.times N db V` | Fill **N** bytes with value **V** (supports `$` and `$$` in expressions) |

### Section Model

- **`.data`** and **`.rodata`** sections contain only data variable declarations
  (`DB`, `DW`, `DD`)
- **`.code`** sections contain labels, instructions, and raw numeric literals
- `.use16` / `.use32` change the operand-size default but do not switch sections

### Origin Directive

`.org` sets the base address for offset calculations. Labels and `$` references
are computed relative to this origin:

```asm
.org 0x7C00          ; bootloader loads at 0x7C00
```

### Fill Directive

`.times` emits repeated byte values to pad or fill regions:

```asm
.times 510-($-$$) db 0   ; pad to byte 510 (from section start)
0xAA55                    ; boot signature
```

`$` is the current output offset, `$$` is the offset of the current section start.
The expression `$-$$` gives the number of bytes emitted so far in the current section.

---

## Labels

### Global Labels

Any identifier followed by a colon defines a global label:

```asm
_start:
my_function:
loop_top:
```

Label names are **case-sensitive**.

### Local Labels (`@@`)

Local labels use the `@` prefix and are **scoped to their enclosing global label**.
Two different functions can each define `@@done:` without collision.

| Form | Syntax | Description |
|------|--------|-------------|
| Define | `@@1:` | Define a numbered local label (number or text after `@@`) |
| Forward | `@f` | Reference the **next** local label |
| Backward | `@b` | Reference the **previous** local label |

```asm
strncopy:
    @@done:              ; internally scoped as strncopy.@@done
    RET

strncmp:
    @@done:              ; internally scoped as strncmp.@@done — no collision
    RET
```

`@f` and `@b` also respect scoping — they resolve to the nearest local
label within the current global scope.

### Label Limits

- Verification pass: up to **512** labels
- Code generation: up to **1024** labels

Duplicate label names produce an error.

---

## Data Variables

Data variables are declared in `.data` or `.rodata` sections:

```
name  TYPE  value [, value ...]
```

### Type Keywords

| Keyword(s) | Size | Description |
|------------|------|-------------|
| `DB`, `BYTE` | 8-bit | Byte — also used for string literals |
| `DW`, `WORD` | 16-bit | Word |
| `DD`, `DWORD` | 32-bit | Double-word |
| `REAL4` | 32-bit | IEEE 754 single-precision float |

### Syntax Examples

```asm
.data
msg     DB "Hello, world!", 0   ; String with null terminator
flag    DB 1                    ; Single byte
arr     DW 1, 2, 3, 4          ; Comma-separated word array
addr    DD 0x00100000           ; 32-bit address
pi      REAL4 3.14             ; Float constant
buffer  DB 0                   ; Single initialised byte
```

### Variable Byte-Size References

The `$` prefix before a variable name evaluates to the **byte size** of that
variable's data. This is useful for storing string lengths:

```asm
.data
msg     DB "Hello, world!", 0
msg_len DD $msg               ; = number of bytes in 'msg' (14)
arr     DW 1, 2, 3
arr_len DD $arr               ; = number of bytes in 'arr' (6)
```

### Negative Values

Negative numbers are supported in data declarations. They are stored as
two's complement values:

```asm
word_neg  DW -1               ; stored as 0xFFFF
dword_neg DD -100              ; stored as 0xFFFFFF9C
```

### Rules

- Variable names must be **unique** (duplicates are an error)
- `DW`, `DD`, `REAL4` initialisers must be **numeric** or **`$variable`** references (no string literals)
- `DB` accepts both strings (double-quoted) and numeric values
- Comma-separated values create a **list** (stored contiguously)

---

## Number Literals

| Format | Prefix | Examples | Parsed Value |
|--------|--------|----------|-------------|
| Decimal | *(none)* | `42`, `1234` | 42, 1234 |
| Hexadecimal | `0x` or `0X` | `0xFF`, `0x100` | 255, 256 |
| Binary | `0b` or `0B` | `0b1010`, `0B11110000` | 10, 240 |
| Character | `'...'` | `'A'`, `'\n'` | 65, 10 |
| Negative | `-` | `-1` | 0xFFFFFFFF |

**Bare numbers** in `.code` sections produce raw bytes in the output, sized
automatically: 1 byte if ≤ 0xFF, 2 bytes if ≤ 0xFFFF, 4 bytes otherwise.

```asm
.code
0x55            ; emits byte 0x55
0xAA55          ; emits 2 bytes (little-endian)
0xDEADBEEF      ; emits 4 bytes (little-endian)
```

---

## String Literals

Strings are enclosed in **double quotes** and used in `DB` declarations:

```asm
msg  DB "Hello, world!", 0
```

### Escape Sequences

| Escape | Value | Description |
|--------|-------|-------------|
| `\n` | 0x0A | Newline (LF) |
| `\r` | 0x0D | Carriage return (CR) |
| `\t` | 0x09 | Horizontal tab |
| `\0` | 0x00 | Null terminator |
| `\\` | 0x5C | Literal backslash |
| `\"` | 0x22 | Literal double quote |

Strings are emitted as raw bytes — each character becomes one byte. A trailing
`, 0` after the string adds a null terminator:

```asm
greeting DB "Hi\n", 0    ; 4 bytes: 'H', 'i', 0x0A, 0x00
```

---

## Registers

### General Purpose

| 32-bit | 16-bit | 8-bit High | 8-bit Low |
|--------|--------|------------|-----------|
| `EAX` | `AX` | `AH` | `AL` |
| `EBX` | `BX` | `BH` | `BL` |
| `ECX` | `CX` | `CH` | `CL` |
| `EDX` | `DX` | `DH` | `DL` |
| `ESI` | `SI` | — | — |
| `EDI` | `DI` | — | — |
| `EBP` | `BP` | — | — |
| `ESP` | `SP` | — | — |

### Segment Registers (16-bit)

`CS`, `DS`, `ES`, `FS`, `GS`, `SS`

### Control Registers (32-bit)

`CR0`, `CR2`, `CR3`, `CR4`

### Debug Registers (32-bit)

`DR0`, `DR1`, `DR2`, `DR3`, `DR6`, `DR7`

### x87 FPU Stack (80-bit internal)

`ST0`, `ST1`, `ST2`, `ST3`, `ST4`, `ST5`, `ST6`, `ST7`

### MMX (64-bit)

`MM0`, `MM1`, `MM2`, `MM3`, `MM4`, `MM5`, `MM6`, `MM7`

### SSE (128-bit)

`XMM0`, `XMM1`, `XMM2`, `XMM3`, `XMM4`, `XMM5`, `XMM6`, `XMM7`

### Register Encoding (3-bit numbers)

| # | 8-bit | 16-bit | 32-bit | Segment | Control | FPU | MMX | SSE |
|---|-------|--------|--------|---------|---------|-----|-----|-----|
| 0 | AL | AX | EAX | ES | CR0 | ST0 | MM0 | XMM0 |
| 1 | CL | CX | ECX | CS | — | ST1 | MM1 | XMM1 |
| 2 | DL | DX | EDX | SS | CR3 | ST2 | MM2 | XMM2 |
| 3 | BL | BX | EBX | DS | CR4 | ST3 | MM3 | XMM3 |
| 4 | AH | SP | ESP | FS | — | ST4 | MM4 | XMM4 |
| 5 | CH | BP | EBP | GS | — | ST5 | MM5 | XMM5 |
| 6 | DH | SI | ESI | — | — | ST6 | MM6 | XMM6 |
| 7 | BH | DI | EDI | — | — | ST7 | MM7 | XMM7 |

---

## Operand Syntax

Instructions take zero to four operands separated by commas:

```asm
MNEMONIC                         ; no operands
MNEMONIC dst                     ; one operand
MNEMONIC dst, src                ; two operands
```

### Register Operands

Use register names directly. Size is inferred from the register:

```asm
MOV EAX, EBX        ; 32-bit
MOV AX, BX          ; 16-bit
MOV AL, BH          ; 8-bit
```

### Immediate Operands

Numeric constants in any supported literal format:

```asm
MOV EAX, 42          ; decimal
MOV EAX, 0xFF        ; hex
MOV EAX, 0b1010      ; binary
MOV AL, 'A'          ; character
```

### Memory Operands

Memory references are enclosed in square brackets `[ ]`:

```asm
[EAX]                        ; base register
[EBX + 100]                  ; base + displacement
[ESI + ECX]                  ; base + index
[ESI + ECX*4]                ; base + index × scale
[ESI + ECX*4 + 16]           ; base + index × scale + displacement
[my_var]                     ; symbol / variable address
[300]                        ; displacement-only (absolute address)
```

**Scale** must be `1`, `2`, `4`, or `8`.

**Displacement** can be positive or negative.

### Segment Override Prefixes

Memory operands can include a segment register override using `SEG:` syntax
inside the brackets:

```asm
MOV AX, [CS:BX]         ; code segment override (prefix 2Eh)
MOV [ES:DI], AX          ; extra segment override (prefix 26h)
MOV [SS:BP], SP          ; stack segment override (prefix 36h)
MOV AL, [FS:DI]          ; FS override (prefix 64h)
MOV AH, [GS:SI]          ; GS override (prefix 65h)
```

The segment override prefix byte is emitted automatically before the instruction.

### Size Prefix

When operand size is ambiguous (e.g. memory-only operands), use an explicit
size prefix:

```asm
MOV BYTE PTR [EAX], 0       ; 8-bit store
MOV WORD PTR [EAX], 0       ; 16-bit store
MOV DWORD PTR [EAX], 0      ; 32-bit store
INC BYTE [EAX]              ; PTR keyword is optional
```

The `PTR` keyword after `BYTE`/`WORD`/`DWORD` is optional.

### Label References

Label names used as operands are resolved to addresses:

```asm
JMP _start          ; relative jump to label
CALL my_func        ; relative call
MOV EAX, [my_var]   ; load from data variable address
```

### Current Location Counter

`$` represents the **current output offset** (location counter).
`$$` represents the **start offset of the current section**.

```asm
JMP $               ; infinite loop (jumps to itself)
.times 512-($-$$) db 0  ; pad to 512 bytes from section start
```

---

## Instruction Reference

All 472 instruction entries (186 unique mnemonics) grouped by category. Each entry specifies the
mnemonic name, opcode, operand sizes, and a brief description.

### No-Operand Instructions

Simple single-byte instructions with no operands.

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `NOP` | 90 | No operation |
| `HLT` | F4 | Halt processor until interrupt |
| `RET` | C3 | Return from near procedure |
| `RETF` | CB | Return from far procedure |
| `LEAVE` | C9 | Restore stack frame (`MOV ESP,EBP; POP EBP`) |
| `PUSHA` | 60 | Push all general-purpose registers |
| `PUSHAD` | 60 | Push all 32-bit general-purpose registers |
| `POPA` | 61 | Pop all general-purpose registers |
| `POPAD` | 61 | Pop all 32-bit general-purpose registers |
| `PUSHF` | 9C | Push EFLAGS onto stack |
| `PUSHFD` | 9C | Push 32-bit EFLAGS onto stack |
| `POPF` | 9D | Pop EFLAGS from stack |
| `POPFD` | 9D | Pop 32-bit EFLAGS from stack |
| `INT3` | CC | Breakpoint trap (debug interrupt 3) |
| `INTO` | CE | Interrupt on overflow (`INT 4` if OF=1) |
| `IRET` | CF | Return from interrupt |
| `CLC` | F8 | Clear carry flag |
| `STC` | F9 | Set carry flag |
| `CMC` | F5 | Complement (toggle) carry flag |
| `CLI` | FA | Clear interrupt flag (disable interrupts) |
| `STI` | FB | Set interrupt flag (enable interrupts) |
| `CLD` | FC | Clear direction flag |
| `STD` | FD | Set direction flag |
| `SAHF` | 9E | Store AH into low 8 bits of EFLAGS |
| `LAHF` | 9F | Load low 8 bits of EFLAGS into AH |
| `XLAT` | D7 | Table look-up: `AL = [EBX + AL]` |
| `AAA` | 37 | ASCII adjust AL after addition |
| `AAS` | 3F | ASCII adjust AL after subtraction |
| `DAA` | 27 | Decimal adjust AL after addition |
| `DAS` | 2F | Decimal adjust AL after subtraction |
| `CWDE` | 98 | Sign-extend AX into EAX |
| `CDQ` | 99 | Sign-extend EAX into EDX:EAX |
| `CBW` | 98 | Sign-extend AL into AX |
| `CWD` | 99 | Sign-extend AX into DX:AX (16-bit) |
| `WAIT` | 9B | Wait for pending FPU exceptions |

---

### PUSH / POP

**Register-in-opcode form** (single register):

| Mnemonic | Sizes | Opcode | Description |
|----------|-------|--------|-------------|
| `PUSH r32` | 32-bit | 50+rd | Push 32-bit register |
| `PUSH r16` | 16-bit | 66 50+rw | Push 16-bit register |
| `POP r32` | 32-bit | 58+rd | Pop into 32-bit register |
| `POP r16` | 16-bit | 66 58+rw | Pop into 16-bit register |

**Immediate form:**

| Mnemonic | Sizes | Opcode | Description |
|----------|-------|--------|-------------|
| `PUSH imm8` | 8-bit | 6A | Push sign-extended imm8 |
| `PUSH imm32` | 32-bit | 68 | Push imm32 |
| `PUSH imm16` | 16-bit | 66 68 | Push imm16 |

**Memory/register form** (ModR/M):

| Mnemonic | Sizes | Opcode | Description |
|----------|-------|--------|-------------|
| `PUSH r/m32` | 32-bit | FF /6 | Push r/m32 |
| `PUSH r/m16` | 16-bit | 66 FF /6 | Push r/m16 |
| `POP r/m32` | 32-bit | 8F /0 | Pop into r/m32 |
| `POP r/m16` | 16-bit | 66 8F /0 | Pop into r/m16 |

```asm
PUSH EAX            ; push register
PUSH 0x1234         ; push immediate
PUSH DWORD PTR [EBX]; push from memory
POP ECX             ; pop into register
```

---

### MOV — Data Movement

| Form | 8-bit | 16-bit (66h) | 32-bit |
|------|-------|-------------|--------|
| `MOV r/m, r` | 88 | 66 89 | 89 |
| `MOV r, r/m` | 8A | 66 8B | 8B |
| `MOV r/m, imm` | C6 /0 | 66 C7 /0 | C7 /0 |
| `MOV r, imm` | B0+rb | 66 B8+rw | B8+rd |

```asm
MOV EAX, 1              ; MOV r32, imm32
MOV [EBX], AL            ; MOV r/m8, r8
MOV AX, [ESI]            ; MOV r16, r/m16
MOV DWORD PTR [EDI], 0   ; MOV r/m32, imm32
```

---

### ADD — Addition

Sets all arithmetic flags (OF, SF, ZF, AF, PF, CF).

| Form | 8-bit | 16-bit (66h) | 32-bit |
|------|-------|-------------|--------|
| `ADD r/m, r` | 00 | 66 01 | 01 |
| `ADD r, r/m` | 02 | 66 03 | 03 |
| `ADD r/m, imm8` | 80 /0 | 66 83 /0 | 83 /0 |
| `ADD r/m, imm16/32` | — | 66 81 /0 | 81 /0 |

```asm
ADD EAX, EBX         ; r32 + r32
ADD EAX, 100         ; r32 + imm (auto-selects imm8 or imm32)
ADD [ESI], AL         ; r/m8 + r8
ADD AX, [EDI]         ; r16 + r/m16
```

---

### SUB — Subtraction

Sets all arithmetic flags. Same encoding structure as ADD with opcodes
28/29/2A/2B and extension /5.

```asm
SUB EAX, EBX
SUB ECX, 1
SUB BYTE PTR [EAX], 5
```

---

### AND / OR / XOR — Bitwise Logic

Clear OF and CF. Set SF, ZF, PF.

| Mnemonic | r/m,r | r,r/m | Extension | Description |
|----------|-------|-------|-----------|-------------|
| `AND` | 20/21 | 22/23 | /4 | Bitwise AND |
| `OR` | 08/09 | 0A/0B | /1 | Bitwise OR |
| `XOR` | 30/31 | 32/33 | /6 | Bitwise XOR |

All three support 8/16/32-bit variants with the same pattern as ADD/SUB
(imm8 via 80/83, imm16/32 via 81).

```asm
AND EAX, 0xFF       ; mask low byte
OR  EAX, 1           ; set bit 0
XOR EAX, EAX         ; clear register (fast zero)
```

---

### CMP — Compare

Sets all arithmetic flags. Result is discarded (like SUB without storing).
Opcodes: 38/39/3A/3B, extension /7.

```asm
CMP EAX, 0
CMP BYTE PTR [ESI], 'A'
CMP AX, BX
```

---

### TEST — Logical Compare

Performs bitwise AND, sets flags, discards result. Opcodes: 84/85 (r/m, r)
and F6/F7 /0 (r/m, imm).

```asm
TEST EAX, EAX        ; check if zero
TEST AL, 0x80         ; check sign bit
```

---

### INC / DEC / NOT / NEG — Unary Operations

**Register-in-opcode form** (INC/DEC only):

| Mnemonic | 32-bit | 16-bit |
|----------|--------|--------|
| `INC r` | 40+rd | 66 40+rw |
| `DEC r` | 48+rd | 66 48+rw |

**ModR/M form** (all four, on r/m):

| Mnemonic | 8-bit | 16/32-bit | Extension | Description |
|----------|-------|-----------|-----------|-------------|
| `INC` | FE /0 | FF /0 | /0 | Increment by 1 |
| `DEC` | FE /1 | FF /1 | /1 | Decrement by 1 |
| `NOT` | F6 /2 | F7 /2 | /2 | Bitwise NOT (one's complement) |
| `NEG` | F6 /3 | F7 /3 | /3 | Two's complement negate |

```asm
INC EAX
DEC BYTE PTR [ESI]
NOT ECX
NEG EDX
```

---

### MUL / IMUL / DIV / IDIV — Multiply & Divide

Single-operand form on r/m:

| Mnemonic | 8-bit | 16/32-bit | Extension | Description |
|----------|-------|-----------|-----------|-------------|
| `MUL` | F6 /4 | F7 /4 | /4 | Unsigned multiply |
| `IMUL` | F6 /5 | F7 /5 | /5 | Signed multiply |
| `DIV` | F6 /6 | F7 /6 | /6 | Unsigned divide |
| `IDIV` | F6 /7 | F7 /7 | /7 | Signed divide |

**Result locations:**

| Size | Multiply result | Divide quotient | Divide remainder |
|------|----------------|-----------------|-----------------|
| 8-bit | AX | AL | AH |
| 16-bit | DX:AX | AX | DX |
| 32-bit | EDX:EAX | EAX | EDX |

```asm
MUL ECX              ; EDX:EAX = EAX × ECX
IMUL BL              ; AX = AL × BL (signed)
DIV ECX              ; EAX = EDX:EAX / ECX, EDX = remainder
```

---

### Shift & Rotate Operations

Five operations, each with three sub-forms (by 1, by CL, by imm8):

| Mnemonic | Extension | Description |
|----------|-----------|-------------|
| `SHL` | /4 | Shift left (logical) |
| `SHR` | /5 | Shift right (logical, zero-fill) |
| `SAR` | /7 | Shift right (arithmetic, sign-preserving) |
| `ROL` | /0 | Rotate left |
| `ROR` | /1 | Rotate right |

**Opcode pattern:**

| Sub-form | 8-bit | 16/32-bit |
|----------|-------|-----------|
| By 1 | D0 /ext | D1 /ext (66h for 16-bit) |
| By CL | D2 /ext | D3 /ext (66h for 16-bit) |
| By imm8 | C0 /ext | C1 /ext (66h for 16-bit) |

```asm
SHL EAX, 1           ; shift left by 1
SHR EAX, CL          ; shift right by CL
SAR AX, 4            ; arithmetic shift right by 4
ROL ECX, 1           ; rotate left by 1
ROR BYTE PTR [ESI], CL  ; rotate right by CL
```

---

### LEA — Load Effective Address

Computes the memory address without accessing memory:

| Form | Opcode | Size |
|------|--------|------|
| `LEA r32, m` | 8D | 32-bit |
| `LEA r16, m` | 66 8D | 16-bit |

```asm
LEA EAX, [EBX + ECX*4 + 8]   ; EAX = EBX + ECX×4 + 8
LEA ESI, [EDI + 100]          ; ESI = EDI + 100
```

---

### XCHG — Exchange

**Register-in-opcode form** (exchange with EAX/AX):

| Mnemonic | Sizes | Opcode |
|----------|-------|--------|
| `XCHG r32, EAX` | 32-bit | 90+rd |
| `XCHG r16, AX` | 16-bit | 66 90+rw |

**ModR/M form:**

| Form | 8-bit | 16-bit | 32-bit |
|------|-------|--------|--------|
| `XCHG r/m, r` | 86 | 66 87 | 87 |

```asm
XCHG EAX, EBX       ; exchange EAX and EBX
XCHG [ESI], AL       ; exchange memory byte with AL
```

---

### INT — Software Interrupt

```asm
INT 0x21             ; call interrupt 0x21
INT3                 ; breakpoint (single-byte, opcode CC)
```

---

### RET — Return

| Form | Opcode | Description |
|------|--------|-------------|
| `RET` | C3 | Near return |
| `RET imm16` | C2 | Near return, pop imm16 extra bytes |
| `RETF` | CB | Far return |
| `RETF imm16` | CA | Far return, pop imm16 extra bytes |

---

### Conditional Jumps — Short (rel8)

| Mnemonic | Opcode | Condition |
|----------|--------|-----------|
| `JO` | 70 | Overflow (OF=1) |
| `JNO` | 71 | Not overflow (OF=0) |
| `JB` / `JC` | 72 | Below / Carry (CF=1) |
| `JNB` / `JNC` | 73 | Not below / No carry (CF=0) |
| `JZ` / `JE` | 74 | Zero / Equal (ZF=1) |
| `JNZ` / `JNE` | 75 | Not zero / Not equal (ZF=0) |
| `JBE` | 76 | Below or equal (CF=1 or ZF=1) |
| `JNBE` / `JA` | 77 | Above (CF=0 and ZF=0) |
| `JS` | 78 | Sign (SF=1) |
| `JNS` | 79 | Not sign (SF=0) |
| `JP` | 7A | Parity even (PF=1) |
| `JNP` | 7B | Parity odd (PF=0) |
| `JL` | 7C | Less (SF≠OF) |
| `JNL` / `JGE` | 7D | Not less / Greater or equal (SF=OF) |
| `JLE` | 7E | Less or equal (ZF=1 or SF≠OF) |
| `JNLE` / `JG` | 7F | Greater (ZF=0 and SF=OF) |

### Unconditional Jumps & Calls

| Mnemonic | Opcode | Type | Description |
|----------|--------|------|-------------|
| `JMP rel8` | EB | Short | Jump ±127 bytes |
| `JMP rel32` | E9 | Near | Jump ±2GB |
| `JMP r/m32` | FF /4 | Indirect | Jump to address in register/memory |
| `CALL rel32` | E8 | Near | Call procedure |
| `CALL r/m32` | FF /2 | Indirect | Call via register/memory |

### Near Conditional Jumps (rel32, 0F-prefixed)

Same conditions as short jumps but with 32-bit displacement. Opcodes
`0F 80` through `0F 8F`.

```asm
JZ short_label       ; short jump (rel8)
JZ far_label         ; near jump (0F 84 rel32) — selected by distance
```

### Loop Instructions

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `LOOP` | E2 | Decrement ECX; jump if ECX ≠ 0 |
| `LOOPE` | E1 | Decrement ECX; jump if ECX ≠ 0 and ZF=1 |
| `LOOPNE` | E0 | Decrement ECX; jump if ECX ≠ 0 and ZF=0 |
| `JCXZ` | E3 | Jump if CX/ECX = 0 |

---

### MOVZX / MOVSX — Zero/Sign-Extend

| Mnemonic | Opcode | Source | Dest | Description |
|----------|--------|--------|------|-------------|
| `MOVZX r32, r/m8` | 0F B6 | 8-bit | 32-bit | Zero-extend byte to dword |
| `MOVZX r32, r/m16` | 0F B7 | 16-bit | 32-bit | Zero-extend word to dword |
| `MOVZX r16, r/m8` | 66 0F B6 | 8-bit | 16-bit | Zero-extend byte to word |
| `MOVSX r32, r/m8` | 0F BE | 8-bit | 32-bit | Sign-extend byte to dword |
| `MOVSX r32, r/m16` | 0F BF | 16-bit | 32-bit | Sign-extend word to dword |
| `MOVSX r16, r/m8` | 66 0F BE | 8-bit | 16-bit | Sign-extend byte to word |

```asm
MOVZX EAX, AL        ; zero-extend AL into EAX
MOVSX ECX, WORD PTR [ESI]  ; sign-extend 16-bit mem to 32-bit
```

---

### BSF / BSR — Bit Scan

| Mnemonic | Opcode | Sizes | Description |
|----------|--------|-------|-------------|
| `BSF r, r/m` | 0F BC | 16/32 | Bit scan forward (find lowest set bit) |
| `BSR r, r/m` | 0F BD | 16/32 | Bit scan reverse (find highest set bit) |

```asm
BSF EAX, ECX         ; EAX = index of lowest set bit in ECX
BSR EAX, ECX         ; EAX = index of highest set bit in ECX
```

---

### SETcc — Set Byte on Condition

Set r/m8 to 1 if condition is true, 0 otherwise. All use 8-bit operands.

`SETO`, `SETNO`, `SETB`, `SETNB`, `SETZ`, `SETNZ`, `SETBE`, `SETNBE`,
`SETS`, `SETNS`, `SETP`, `SETNP`, `SETL`, `SETNL`, `SETLE`, `SETNLE`

Opcodes: `0F 90` through `0F 9F`.

```asm
CMP EAX, EBX
SETZ AL              ; AL = 1 if EAX == EBX, else 0
SETL BL              ; BL = 1 if EAX < EBX (signed)
```

---

### System Instructions

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `CPUID` | 0F A2 | CPU identification (Pentium+) |
| `RDTSC` | 0F 31 | Read time-stamp counter into EDX:EAX (Pentium+) |
| `LGDT m` | 0F 01 /2 | Load GDT register from 6-byte memory operand |
| `LIDT m` | 0F 01 /3 | Load IDT register from 6-byte memory operand |
| `SGDT m` | 0F 01 /0 | Store GDT register to 6-byte memory operand |
| `SIDT m` | 0F 01 /1 | Store IDT register to 6-byte memory operand |

```asm
LGDT [gdt_descriptor]
LIDT [idt_descriptor]
CPUID
RDTSC
```

### AAM / AAD — BCD Adjust

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `AAM imm8` | D4 | ASCII adjust AX after multiply (base in imm8) |
| `AAD imm8` | D5 | ASCII adjust AX before divide (base in imm8) |

---

### ADC — Add with Carry

Adds source, destination, and carry flag (CF). Same encoding pattern as ADD.
Opcodes: 10/11/12/13, extension /2, 80/83/81.

```asm
ADC EAX, EBX         ; EAX = EAX + EBX + CF
ADC EAX, 100         ; EAX = EAX + 100 + CF
ADC AL, [ESI]        ; AL = AL + [ESI] + CF
```

---

### SBB — Subtract with Borrow

Subtracts source and carry flag from destination. Same encoding pattern as SUB.
Opcodes: 18/19/1A/1B, extension /3, 80/83/81.

```asm
SBB EAX, EBX         ; EAX = EAX - EBX - CF
SBB ECX, 1           ; ECX = ECX - 1 - CF
```

---

### RCL / RCR — Rotate Through Carry

Rotate bits left/right through the carry flag. Same operand forms as SHL/SHR
(by 1, by CL, by imm8).

| Mnemonic | Extension | Description |
|----------|-----------|-------------|
| `RCL` | /2 | Rotate left through carry |
| `RCR` | /3 | Rotate right through carry |

```asm
RCL EAX, 1           ; rotate left through CF by 1
RCR ECX, CL          ; rotate right through CF by CL
RCL BYTE PTR [ESI], 4 ; rotate memory byte left through CF by 4
```

---

### String Operations

String instructions operate on bytes/words/dwords at `DS:ESI` (source) and
`ES:EDI` (destination). Direction flag controls increment/decrement.

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `MOVSB` | A4 | Move byte `[ESI]` → `[EDI]` |
| `MOVSW` | 66 A5 | Move word `[ESI]` → `[EDI]` |
| `MOVSD` | A5 | Move dword `[ESI]` → `[EDI]` |
| `STOSB` | AA | Store AL → `[EDI]` |
| `STOSW` | 66 AB | Store AX → `[EDI]` |
| `STOSD` | AB | Store EAX → `[EDI]` |
| `LODSB` | AC | Load `[ESI]` → AL |
| `LODSW` | 66 AD | Load `[ESI]` → AX |
| `LODSD` | AD | Load `[ESI]` → EAX |
| `CMPSB` | A6 | Compare byte `[ESI]` vs `[EDI]` |
| `CMPSW` | 66 A7 | Compare word `[ESI]` vs `[EDI]` |
| `CMPSD` | A7 | Compare dword `[ESI]` vs `[EDI]` |
| `SCASB` | AE | Compare AL vs `[EDI]` |
| `SCASW` | 66 AF | Compare AX vs `[EDI]` |
| `SCASD` | AF | Compare EAX vs `[EDI]` |
| `INSB` | 6C | Input byte from port DX → `[EDI]` |
| `INSW` | 66 6D | Input word from port DX → `[EDI]` |
| `INSD` | 6D | Input dword from port DX → `[EDI]` |
| `OUTSB` | 6E | Output byte `[ESI]` → port DX |
| `OUTSW` | 66 6F | Output word `[ESI]` → port DX |
| `OUTSD` | 6F | Output dword `[ESI]` → port DX |

### Repeat Prefixes

| Prefix | Opcode | Description |
|--------|--------|-------------|
| `REP` | F3 | Repeat while ECX ≠ 0 |
| `REPE` / `REPZ` | F3 | Repeat while ECX ≠ 0 and ZF=1 |
| `REPNE` / `REPNZ` | F2 | Repeat while ECX ≠ 0 and ZF=0 |

```asm
REP MOVSB            ; copy ECX bytes from [ESI] to [EDI]
REP STOSD            ; fill ECX dwords at [EDI] with EAX
REPE CMPSB           ; compare strings until mismatch or ECX=0
REPNE SCASB          ; scan for AL in [EDI] string
```

---

### IN / OUT — Port I/O

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `IN AL, imm8` | E4 | Input byte from port imm8 |
| `IN AX, imm8` | 66 E5 | Input word from port imm8 |
| `IN EAX, imm8` | E5 | Input dword from port imm8 |
| `IN AL, DX` | EC | Input byte from port DX |
| `IN AX, DX` | 66 ED | Input word from port DX |
| `IN EAX, DX` | ED | Input dword from port DX |
| `OUT imm8, AL` | E6 | Output byte to port imm8 |
| `OUT imm8, AX` | 66 E7 | Output word to port imm8 |
| `OUT imm8, EAX` | E7 | Output dword to port imm8 |
| `OUT DX, AL` | EE | Output byte to port DX |
| `OUT DX, AX` | 66 EF | Output word to port DX |
| `OUT DX, EAX` | EF | Output dword to port DX |

```asm
IN AL, 0x60          ; read keyboard port
OUT 0x20, AL         ; send EOI to PIC
IN EAX, DX           ; read dword from port in DX
OUT DX, AL           ; write byte to port in DX
```

---

### Segment Register Operations

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `PUSH ES` | 06 | Push ES |
| `PUSH CS` | 0E | Push CS |
| `PUSH SS` | 16 | Push SS |
| `PUSH DS` | 1E | Push DS |
| `PUSH FS` | 0F A0 | Push FS |
| `PUSH GS` | 0F A8 | Push GS |
| `POP ES` | 07 | Pop ES |
| `POP SS` | 17 | Pop SS |
| `POP DS` | 1F | Pop DS |
| `POP FS` | 0F A1 | Pop FS |
| `POP GS` | 0F A9 | Pop GS |
| `MOV sreg, r/m16` | 8E | Load segment register |
| `MOV r/m16, sreg` | 8C | Store segment register |

```asm
PUSH DS
POP ES               ; ES = DS
MOV DS, AX           ; load data segment
```

---

### Bit Test & Modify

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `BT r/m, r` | 0F A3 | Bit test — copy bit to CF |
| `BT r/m, imm8` | 0F BA /4 | Bit test (immediate bit index) |
| `BTS r/m, r` | 0F AB | Bit test and set |
| `BTS r/m, imm8` | 0F BA /5 | Bit test and set (immediate) |
| `BTR r/m, r` | 0F B3 | Bit test and reset (clear) |
| `BTR r/m, imm8` | 0F BA /6 | Bit test and reset (immediate) |
| `BTC r/m, r` | 0F BB | Bit test and complement (toggle) |
| `BTC r/m, imm8` | 0F BA /7 | Bit test and complement (immediate) |

```asm
BT EAX, 5            ; test bit 5, result in CF
BTS DWORD PTR [ESI], ECX  ; test and set bit ECX
BTR EAX, 0            ; test and clear bit 0
```

---

### SHLD / SHRD — Double-Precision Shift

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `SHLD r/m, r, imm8` | 0F A4 | Shift left double (3 operands) |
| `SHLD r/m, r, CL` | 0F A5 | Shift left double by CL |
| `SHRD r/m, r, imm8` | 0F AC | Shift right double (3 operands) |
| `SHRD r/m, r, CL` | 0F AD | Shift right double by CL |

```asm
SHLD EAX, EBX, 8     ; shift EAX left 8, fill from EBX high bits
SHRD EDX, EAX, CL    ; shift EDX right by CL, fill from EAX
```

---

### IMUL — Multi-Operand Forms

In addition to the single-operand `IMUL r/m` form:

| Form | Opcode | Description |
|------|--------|-------------|
| `IMUL r, r/m` | 0F AF | Two-operand signed multiply |
| `IMUL r, r/m, imm8` | 6B | Three-operand signed multiply (imm8) |
| `IMUL r, r/m, imm32` | 69 | Three-operand signed multiply (imm32) |

```asm
IMUL EAX, ECX        ; EAX = EAX × ECX (signed)
IMUL EAX, ECX, 10    ; EAX = ECX × 10 (signed)
IMUL ESI, [EDI], 100 ; ESI = [EDI] × 100 (signed)
```

---

### MOV — Control & Debug Registers

| Form | Opcode | Description |
|------|--------|-------------|
| `MOV r32, CRn` | 0F 20 | Read control register |
| `MOV CRn, r32` | 0F 22 | Write control register |
| `MOV r32, DRn` | 0F 21 | Read debug register |
| `MOV DRn, r32` | 0F 23 | Write debug register |

```asm
MOV EAX, CR0          ; read CR0
OR EAX, 1             ; set PE bit
MOV CR0, EAX          ; write CR0 — enter protected mode
MOV DR7, EBX          ; set debug control register
```

---

### LDS / LES / LFS / LGS / LSS — Load Far Pointer

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `LDS r, m` | C5 | Load pointer into DS:reg |
| `LES r, m` | C4 | Load pointer into ES:reg |
| `LFS r, m` | 0F B4 | Load pointer into FS:reg |
| `LGS r, m` | 0F B5 | Load pointer into GS:reg |
| `LSS r, m` | 0F B2 | Load pointer into SS:reg |

```asm
LDS ESI, [ptr]        ; ESI = [ptr], DS = [ptr+4]
LSS ESP, [stack_ptr]   ; switch stack: SS:ESP from memory
```

---

### 80286/80386 System Instructions

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `BOUND r, m` | 62 | Check array bounds (INT 5 if out of range) |
| `ENTER imm16, imm8` | C8 | Create stack frame |
| `ARPL r/m16, r16` | 63 | Adjust RPL field of segment selector |
| `LAR r, r/m` | 0F 02 | Load access rights byte |
| `LSL r, r/m` | 0F 03 | Load segment limit |
| `LTR r/m16` | 0F 00 /3 | Load task register |
| `STR r/m16` | 0F 00 /1 | Store task register |
| `LLDT r/m16` | 0F 00 /2 | Load local descriptor table register |
| `SLDT r/m16` | 0F 00 /0 | Store local descriptor table register |
| `LMSW r/m16` | 0F 01 /6 | Load machine status word |
| `SMSW r/m16` | 0F 01 /4 | Store machine status word |
| `VERR r/m16` | 0F 00 /4 | Verify segment for reading |
| `VERW r/m16` | 0F 00 /5 | Verify segment for writing |
| `CLTS` | 0F 06 | Clear task-switched flag in CR0 |
| `LOCK` | F0 | Assert LOCK# signal prefix |

```asm
ENTER 16, 0           ; create 16-byte stack frame (nesting level 0)
LEAVE                 ; destroy stack frame (MOV ESP,EBP; POP EBP)
LTR AX               ; load task register
LLDT BX               ; load LDTR
LMSW AX               ; load machine status word
```

---

### Conditional Jump Aliases

Common alternative mnemonics for conditional jumps (both rel8 short and
rel32 near forms):

| Alias | Canonical | Condition |
|-------|-----------|-----------|
| `JE` | `JZ` | Equal / Zero (ZF=1) |
| `JNE` | `JNZ` | Not equal / Not zero (ZF=0) |
| `JC` | `JB` | Carry / Below (CF=1) |
| `JNC` | `JNB` | No carry / Not below (CF=0) |
| `JA` | `JNBE` | Above (CF=0 and ZF=0) |
| `JAE` | `JNB` | Above or equal (CF=0) |
| `JG` | `JNLE` | Greater (signed) |
| `JGE` | `JNL` | Greater or equal (signed) |
| `JNA` | `JBE` | Not above (CF=1 or ZF=1) |
| `JNAE` | `JB` | Not above or equal (CF=1) |
| `JNG` | `JLE` | Not greater (signed) |
| `JNGE` | `JL` | Not greater or equal (signed) |
| `JPE` | `JP` | Parity even (PF=1) |
| `JPO` | `JNP` | Parity odd (PF=0) |

Additional loop aliases:

| Alias | Canonical | Description |
|-------|-----------|-------------|
| `LOOPZ` | `LOOPE` | Loop while zero |
| `LOOPNZ` | `LOOPNE` | Loop while not zero |
| `JECXZ` | `JCXZ` | Jump if ECX = 0 |
| `REPZ` | `REPE` | Repeat while zero |
| `REPNZ` | `REPNE` | Repeat while not zero |

---

## Encoding Details

This section describes how instructions are encoded into bytes. The byte
stream layout for any instruction is:

```
[legacy prefix] [segment override] [0F] [opcode] [ModR/M [SIB [disp]]] [immediate]
```

### Legacy Prefixes

| Byte | Name | Purpose |
|------|------|---------|
| `66h` | Operand-size override | Selects 16-bit operands in 32-bit mode |
| `F2h` | REPNE / SSE | Repeat / SSE instruction prefix |
| `F3h` | REP / REPE / SSE | Repeat / SSE instruction prefix |

### Segment Override Prefixes

| Segment | Prefix Byte |
|---------|-------------|
| ES | `26h` |
| CS | `2Eh` |
| SS | `36h` |
| DS | `3Eh` |
| FS | `64h` |
| GS | `65h` |

### Encoding Types

**`ENC_DIRECT`** — Opcode only, no operands. Example: `NOP` → `90`.

**`ENC_REG_OPCODE`** — Register number encoded in the low 3 bits of the opcode
byte (+rb/+rw/+rd). Optional trailing immediate. Example: `PUSH EAX` →
`50` (50h + 0 for EAX).

**`ENC_IMM`** — Opcode followed by immediate value(s). For relative jumps,
the immediate is the signed displacement. Example: `INT 0x21` → `CD 21`.

**`ENC_MODRM`** — Opcode + ModR/M byte, optionally followed by SIB byte,
displacement, and immediate.

### ModR/M Byte

```
  Bits:  7  6 | 5  4  3 | 2  1  0
         mod  |   reg   |   r/m
```

| mod | Meaning |
|-----|---------|
| 00 | Memory, no displacement (except r/m=101 → disp32) |
| 01 | Memory, 8-bit signed displacement |
| 10 | Memory, 32-bit displacement |
| 11 | Register-direct (no memory access) |

**reg field:** Either a register operand number (for `/r` instructions) or an
extension digit `/0` through `/7` (for group instructions like `ADD r/m, imm`).

**r/m field:** Register or memory operand. When r/m = `100` (binary), a SIB
byte follows.

### SIB Byte

Required when the base register is ESP/SP or an index register is used:

```
  Bits:  7  6 | 5  4  3 | 2  1  0
        scale |  index  |  base
```

| scale | Multiplier |
|-------|-----------|
| 00 | ×1 |
| 01 | ×2 |
| 10 | ×4 |
| 11 | ×8 |

Index = `100` means no index register.

### Immediate Encoding

All immediates are emitted in **little-endian** byte order. Size depends on
the instruction form:

- Opcodes `83h` and `6Bh` always use 8-bit sign-extended immediates
- Otherwise, immediate size matches the instruction's operand size (8/16/32-bit)

### 16-bit in 32-bit Mode

In 32-bit protected mode, 16-bit operands require the `66h` operand-size
override prefix. The assembler **automatically** selects the correct encoding
based on register sizes:

```asm
ADD AX, BX           ; automatically uses 66h prefix (16-bit regs)
ADD EAX, EBX         ; no prefix (32-bit is default)
```

---

## Error Reference

### AST Builder Errors

| Error | Cause |
|-------|-------|
| `Expected directive name after '.'` | Dot not followed by a valid directive |
| `Too many operands at L<n>` | More than 4 operands |
| `Bad operand in '<mnem>' at L<n>` | Operand parse failure |
| `No matching mnemonic form for '<mnem>' with <n> operand(s) at L<n>` | No table entry matches the operand types/sizes |
| `Unexpected '@' construct at L<n>` | Malformed local label |
| `Warning: variable declared outside a data section at L<n>` | Data variable before `.data`/`.rodata` |

### Verification Errors

| Error | Cause |
|-------|-------|
| `Instruction has no mnemonic table entry` | Unresolved instruction |
| `'<mnem>' expects <n> operand(s), got <n>` | Wrong operand count |
| `'<mnem>' operand <n> - type mismatch` | Operand type incompatible with instruction |
| `'<mnem>' operand <n> - register is <n>-bit, instruction requires <n>-bit` | Register size mismatch |
| `'<mnem>' operand <n> - explicit <n>-bit size conflicts with <n>-bit instruction` | PTR size vs instruction size |
| `'<mnem>' operand <n> - invalid SIB scale <n>` | Scale not 1, 2, 4, or 8 |
| `'<mnem>' operand <n> - invalid segment override` | Non-segment register as override |
| `Operand <n> references undefined symbol '<name>'` | Label not found |\n| `Operand <n> references undefined local symbol '<name>'` | @@N label not defined |
| `'<mnem>' operand <n> - immediate 0x<X> out of range for <n>-bit operand` | Value too large |
| `Duplicate label '<name>'` | Same label defined twice |
| `Duplicate variable name '<name>'` | Same variable name used twice |
| `Non-BYTE variable '<name>' has non-numeric initializer` | String literal in DW/DD |
| `Raw number 0x<X> exceeds byte range` | Bare number > 0xFF in code |

### Code Generation Errors

| Error | Cause |
|-------|-------|
| `Failed to encode variable '<name>'` | Data emission failure |
| `Failed to encode instruction` | Instruction encoding failure |
| `Exceeded maximum number of labels` | More than 1024 labels |
| `Failed to create/open output file` | File I/O error |\n| `ERROR: undefined symbol '<name>'` | Label not found during code generation |

---

## Examples

### Minimal Bootloader Stub

```asm
.use16
.code

_start:
    CLI                      ; disable interrupts
    XOR AX, AX
    MOV DS, AX
    MOV SS, AX
    MOV SP, 0x7C00

    MOV SI, msg
    CALL print_string
    HLT

print_string:
    LODSB
    TEST AL, AL
    JZ .done
    MOV AH, 0x0E
    INT 0x10
    JMP print_string
.done:
    RET

.data
msg DB "Hello from ASTRAC!", 0

.code
0x55                         ; raw byte
0xAA                         ; boot signature byte 2
```

### 32-bit Protected Mode Example

```asm
#define SCREEN_BASE 0xB8000

.use32
.data
counter DD 0
message DB "Count: ", 0

.code
_start:
    ; Clear the counter
    MOV DWORD PTR [counter], 0

    ; Load screen base address
    MOV EDI, SCREEN_BASE

    ; Write character 'A' with white-on-black attribute
    MOV AL, 'A'
    MOV AH, 0x0F
    MOV [EDI], AX

loop_top:
    ; Increment counter
    INC DWORD PTR [counter]
    MOV EAX, [counter]

    ; Check if we've reached 1000
    CMP EAX, 1000
    JNB done

    ; Bit manipulation example
    AND EAX, 0xFF
    SHL EAX, 1
    OR  EAX, 1
    XOR EBX, EBX

    ; Array access with SIB
    LEA ESI, [EDI + EAX*4 + 8]

    JMP loop_top

done:
    HLT
```

### Demonstrating All Operand Forms

```asm
.use32
.code

; Register-to-register (all sizes)
MOV EAX, EBX             ; 32-bit
MOV AX, BX               ; 16-bit (auto 66h prefix)
MOV AL, BL               ; 8-bit

; Immediate-to-register (all sizes)
MOV EAX, 0xDEADBEEF      ; 32-bit immediate
MOV AX, 0x1234            ; 16-bit immediate
MOV AL, 0xFF              ; 8-bit immediate

; Memory with various addressing modes
MOV EAX, [ESI]                    ; base only
MOV EAX, [ESI + 4]                ; base + disp8
MOV EAX, [ESI + 0x1000]           ; base + disp32
MOV EAX, [ESI + EDI]              ; base + index
MOV EAX, [ESI + EDI*4]            ; base + index*scale
MOV EAX, [ESI + EDI*4 + 100]      ; base + index*scale + disp

; Explicit size prefixes for ambiguous memory operands
INC BYTE PTR [EAX]
INC WORD PTR [EAX]
INC DWORD PTR [EAX]
MOV BYTE [ESI], 0         ; PTR keyword optional

; All shift/rotate forms
SHL EAX, 1                ; by constant 1
SHL EAX, CL               ; by CL register
SHL EAX, 4                ; by immediate

; System instructions
CLI
STI
LGDT [gdt_ptr]
LIDT [idt_ptr]
CPUID
RDTSC

; Conditional set
CMP EAX, EBX
SETZ AL
SETNZ BL
SETL CL
```
