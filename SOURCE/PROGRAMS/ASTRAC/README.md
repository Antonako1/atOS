# AstraC C compiler and Assembler for atOS

./ASSEMBLER contains source for the assembler. More about the assembly can be read in DOCS

./DISSASSEMBLER contains source for the assembler. More about the dissassembly can be read in DOCS

./COMPILER contains source for the compiler. More about the compiler and language can be read in DOCS

Usage: 

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

    -M, --macro <define[=value]>    Add a preprocessor macro for C or ASM
    
    -V, --version                   Show version info and exit
    -v, --verbose                   Enable verbose output
    -q, --quiet                     Suppress normal output
    -h, --help                      Display this help and exit
```