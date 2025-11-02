#include <PROGRAMS/ASTRAC/ASTRAC.h>
#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>
#include <STD/GRAPHICS.h>
#include <STD/IO.h>
#include <STD/RUNTIME.h>
#include <STD/STRING.h>
#include <STD/MEM.h>



VOID PRINT_HELP() {
    printf("\n%s. \n", TRADEMARK);
    printf(
        "ASTRAC.BIN {files.{C|ASM|BIN}...} [options]\n"

        "OPTIONS:"
            "\t-o, --out <file>                Specify output file name (default: derived from input)\n"
            "\t-A, --assemble                  Assemble input ASM files into object binaries\n"
            "\t-C, --compile                   Compile C source files into object binaries\n"
            "\t-D, --disassemble               Disassemble a binary back into readable ASM\n"
            "\t-E, --preprocess                Run preprocessor only (for C files)\n"
            "\t-B, --build                     Perform full build pipeline (compile + assemble)\n\n"
            
            "\t-I, --include <dir>             Add an include directory for headers\n\n"

            "\t-M, --macro <define[=value]>    Add a preprocessor macro for C or ASM\n\n"
            
            "\t-V, --version                   Show version info and exit\n"
            "\t-v, --verbose                   Enable verbose output\n"
            "\t-q, --quiet                     Suppress normal output\n"
            "\t-h, --help                      Display this help and exit\n"
    );
}

VOID PRINT_VERSION() {
    printf("%s. Version %s\n", TRADEMARK, VERSION);
}

#define ARG_CMP1(x) STRCMP(arg, x) == 0
#define ARG_CMP2(x, y) ARG_CMP1(x) || ARG_CMP1(y)

static ASTRAC_ARGS args ATTRIB_DATA = { 0 };

ASTRAC_ARGS* GET_ARGS() {return &args;}

U32 START_WORKLOAD() {
    if(args.build_type & NONE) {
        return 0x1F; // Should not happen
    } 
    else if(args.build_type & DISSASEMBLE) {
        return 0x2F;
    }
    else if(args.build_type & COMPILE) {
        return 0x5F;
    }
    else if(args.build_type & ASSEMBLE) {
        return START_ASSEMBLING();
    } else { // args.built_type & BUILD
        return 0x7F;
    }
    return 0;
}

U32 main(U32 argc, PPU8 argv) {
    if (argc < 2) {
        PRINT_HELP();
        return 1;
    }

    printf("\n%s\n", TRADEMARK);
    MEMZERO(&args, sizeof(ASTRAC_ARGS));

    for (U32 i = 1; i < argc; i++) {
        PU8 arg = argv[i];
        printf("[%d] %s\n", i, arg);
        if (ARG_CMP2("-h", "--help")) { PRINT_HELP(); return 0; }
        else if (ARG_CMP2("-v", "--verbose")) { args.verbose = TRUE; }
        else if (ARG_CMP2("-q", "--quiet")) { args.quiet = TRUE; }
        else if (ARG_CMP2("-A", "--assemble")) { args.build_type |= ASSEMBLE; }
        else if (ARG_CMP2("-C", "--compile")) { args.build_type |= COMPILE; }
        else if (ARG_CMP2("-D", "--disassemble")) { args.build_type |= DISSASEMBLE; }
        else if (ARG_CMP2("-B", "--build")) { args.build_type |= BUILD; }
        else if (ARG_CMP2("-o", "--out")) {
            if (++i < argc) {
                args.outfile = argv[i];
                args.got_outfile = TRUE;
            } else {
                printf("Missing output file after -o\n");
                return 1;
            }
        }
        else if (ARG_CMP2("-I", "--include")) {
            if (++i < argc) args.includes[args.include_count++] = argv[i];
        }
        else if (ARG_CMP2("-M", "--macro")) {
            if (++i < argc) {
                PU8 equ = STRCHR(argv[i], '=');
                PU8 val = NULLPTR;
                if(equ) {
                    *equ = '\0';    // terminate macro name
                    val = equ + 1;  // value starts after '='
                }
                DEFINE_MACRO(
                    argv[i], 
                    val, 
                    args.macros.macros[args.macros.len]
                );
            } else {
                printf("Define macros as -M NAME=VALUE\n");
                return 1;
            }
        }
        else {
            args.input_files[args.input_file_count++] = arg;
        }
    }

    if (args.input_file_count == 0) {
        PRINT_HELP();
        return 1;
    }

    if (args.build_type == NONE)
        args.build_type = BUILD;

    if (!args.got_outfile) {
        args.outfile = STRDUP(args.input_files[0]);
        PU8 dot = STRRCHR(args.outfile, '.');
        if (dot) *dot = '\0';

        if (args.build_type & DISSASEMBLE ||
            args.build_type & COMPILE
        ) STRCAT(args.outfile, ".ASM");
        else if (args.build_type & ASSEMBLE) STRCAT(args.outfile, ".BIN");
    }

    U32 retval = START_WORKLOAD();
    FREE_ARGS();
    return retval;
}



VOID FREE_ARGS() {
    if(!args.got_outfile) {
        MFree(args.outfile);
    }
}