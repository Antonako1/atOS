# AstraC — A 16 and 32-bit Compiler, Assembler & Disassembler for atOS

AstraC is the native toolchain for atOS. It takes AC source or assembly and
produces flat binaries that run on the atOS kernel.

## Directory layout

| Path | Purpose |
|------|---------|
| `ASSEMBLER/` | x86 assembler — preprocess, lex, parse, verify, optimise, codegen |
| `COMPILER/`  | C compiler front-end (WIP) |
| `DISSASEMBLER/` | Binary → readable ASM disassembler |
| `SHARED/`    | Code shared across all three tools (preprocessor, macros) |

## Build pipeline (simplified)

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌──────────────┐
│  AC Source  │───>│ Preprocessor│───>│  Compiler   │───>│  ASM Source  │
│  (.AC)      │    │             │    │  (-CA)      │    │  (.ASM)      │
└─────────────┘    └─────────────┘    └─────────────┘    └──────┬───────┘
                                                                │
                      ┌─────────────────────────────────────────┘
                      ▼
                   ┌─────────────┐    ┌─────────────┐
                   │   Lexer     │───>│  AST Build  │
                   │             │    │             │
                   └─────────────┘    └──────┬──────┘
                                             │
                      ┌──────────────────────┘
                      ▼
               ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
               │  Verify AST │───>│  Optimise   │───>│  Codegen    │
               │             │    │   (lightly) │    │  -> .BIN    │
               └─────────────┘    └─────────────┘    └─────────────┘
```

### Modes

| Flag | Name | Input | Output |
|------|------|-------|--------|
| `-A` | Assemble    | `.ASM` | `.BIN` |
| `-C` | Compile     | `.AC`   | `.ASM` |
| `-D` | Disassemble | `.BIN` | `.DSM` |
| `-B` | Build       | `.AC`   | `.BIN` (compile + assemble) |

## Usage

```sh
ASTRAC.BIN {file.{AC|ASM|BIN}} [options]

    GENERAL OPTIONS:
        -h, --help                      Show this help message
        --version                       Show compiler version
        -v, --verbose                   Increase verbosity
        -o <file>                       Write output to <file>
        -q, --quiet                     Suppress non-error output
        -o, --out <file>                Specify output file name
        -D <macro>[=val]                Define macro for preprocessor (like -DNAME=1)
        -E,                             Preprocess only, do not assemble/compile AC -> AC/ ASM -> ASM
        -O, --org <address>             Set origin address for assembly output (default: 0x10000000)

    COMPILER OPTIONS:
        -C, --compile                   Compile source to binary AC -> BIN
        --no-runtime                    Do not link the runtime library
        --no-std                        Do not link the standard library
        -g, --gui                       GUI mode
        -c, --console                   Console mode
        -w, --warnings-as-errors        Treat warnings as errors
        --debug                         Emit debug symbols in the assembly representation
        -S                              Emit assembly and stop AC -> ASM

    ASSEMBLER OPTIONS:
        -A, --assemble                  Assemble ASM -> BIN
        
    DISASSEMBLER OPTIONS:
        --disassemble                   Disassemble BIN -> DSM
        --16                            Disassemble as 16-bit code
        --32                            Disassemble as 32-bit code (default)
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