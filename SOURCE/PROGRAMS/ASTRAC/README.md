# AstraC — Compiler, Assembler & Disassembler for atOS

AstraC is the native toolchain for atOS.  It takes C source or assembly and
produces flat binaries that run on the atOS kernel.

## Directory layout

| Path | Purpose |
|------|---------|
| `ASSEMBLER/` | x86 assembler — preprocess, lex, parse, verify, optimise, codegen |
| `COMPILER/`  | C compiler front-end (WIP) |
| `DISSASEMBLER/` | Binary → readable ASM disassembler (WIP) |
| `SHARED/`    | Code shared across all three tools (preprocessor, macros) |

## Build pipeline (As of now)

```
┌─────────────┐    ┌─────────────┐    ┌──────────────┐
│  C Source   │───>│  Compiler   │───>│  ASM Source  │
│  (.C)       │    │  (-C)       │    │  (.ASM)      │
└─────────────┘    └─────────────┘    └──────┬───────┘
                                             │
                      ┌──────────────────────┘
                      ▼
               ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
               │ Preprocessor│───>│   Lexer     │───>│  AST Build  │
               │             │    │             │    │             │
               └─────────────┘    └─────────────┘    └──────┬──────┘
                                                            │
                      ┌─────────────────────────────────────┘
                      ▼
               ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
               │  Verify AST │───>│  Optimise   │───>│  Codegen    │
               │             │    │             │    │  -> .BIN    │
               └─────────────┘    └─────────────┘    └─────────────┘
```

### Modes

| Flag | Name | Input | Output |
|------|------|-------|--------|
| `-A` | Assemble    | `.ASM` | `.BIN` |
| `-C` | Compile     | `.C`   | `.ASM` |
| `-D` | Disassemble | `.BIN` | `.ASM` |
| `-B` | Build       | `.C`   | `.BIN` (compile + assemble) |

## Usage

```sh
ASTRAC.BIN {files.{C|ASM|BIN}...} [options]

OPTIONS:
    -o, --out <file>                Specify output file name (default: derived from input)
    -A, --assemble                  Assemble input ASM files into object binaries
    -C, --compile                   Compile C source files into object binaries
    -D, --disassemble               Disassemble a binary back into readable ASM
    -E, --preprocess                Run preprocessor only (for C files)
    -B, --build                     Perform full build pipeline (compile + assemble)

    -I, --include <dir>             Add an include directory for headers
    -M, --macro <NAME[=VALUE]>      Define a preprocessor macro

    -V, --version                   Show version info and exit
    -v, --verbose                   Enable verbose output
    -q, --quiet                     Suppress normal output
    -h, --help                      Display this help and exit
    -i, --info <[asm|c]=<keyword>>  Info about a register, directive, symbol, or type
```

## Return codes

| Code | Meaning |
|------|---------|
| 0    | Success |
| 1    | Bad or missing arguments |
| 2    | Preprocessor failed |
| 3    | Lexer failed |
| 4    | AST construction failed |
| 5    | AST verification failed |
| 6    | Optimisation failed |
| 7    | Code generation failed |
| 8    | Disassembly failed |
| 9    | Compilation failed |
| 255  | Internal error |