#include <PROGRAMS/ASTRAC/ASTRAC.h>
#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>
#include <PROGRAMS/ASTRAC/DISSASEMBLER/DISSASEMBLER.h>
#include <STD/GRAPHICS.h>
#include <STD/IO.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/BINARY.h>


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  HELP / VERSION
 * ════════════════════════════════════════════════════════════════════════════
 */

VOID PRINT_HELP() {
    printf("\n%s v%s\n\n", TRADEMARK, VERSION);
    printf(
        "Usage: ASTRAC.BIN {files.{C|ASM|BIN}...} [options]\n\n"

        "PIPELINE OPTIONS (choose one):\n"
            "\t-A, --assemble                  Assemble ASM files  -> BIN\n"
            "\t-C, --compile                   Compile C files     -> ASM\n"
            "\t-D, --disassemble               Disassemble BIN     -> ASM (.DSM)\n"
            "\t-B, --build                     Full pipeline: C -> ASM -> BIN\n"
            "\t-E, --preprocess                Run preprocessor only (C files)\n\n"

        "INPUT / OUTPUT:\n"
            "\t-o, --out <file>                Specify output file name\n"
            "\t-I, --include <dir>             Add include search directory\n"
            "\t-M, --macro <NAME[=VALUE]>      Define a preprocessor macro\n\n"

        "DISASSEMBLER OPTIONS:\n"
            "\t    --16                        Disassemble as 16-bit code\n"
            "\t    --32                        Disassemble as 32-bit code (default)\n\n"

        "INFORMATION:\n"
            "\t-V, --version                   Show version info and exit\n"
            "\t-v, --verbose                   Enable verbose output\n"
            "\t-q, --quiet                     Suppress normal output\n"
            "\t-h, --help                      Display this help and exit\n"
            "\t-i, --info <[asm|c]=<keyword>>  Info about register/directive/symbol/type\n"
    );
}

VOID PRINT_VERSION() {
    printf("%s v%s\n", TRADEMARK, VERSION);
}

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  GLOBALS & ACCESSORS
 * ════════════════════════════════════════════════════════════════════════════
 */

#define ARG_CMP1(x)    (STRCMP(arg, x) == 0)
#define ARG_CMP2(x, y) (ARG_CMP1(x) || ARG_CMP1(y))

static ASTRAC_ARGS args ATTRIB_DATA = { 0 };

ASTRAC_ARGS* GET_ARGS() { return &args; }

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  PIPELINE DISPATCH
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  Dispatch to the correct pipeline stage(s) based on args.build_type:
 *
 *   -A  (ASSEMBLE)     ->  Preprocess -> Lex -> AST -> Verify -> Optimize -> Gen
 *   -C  (COMPILE)      ->  (not yet implemented)
 *   -D  (DISASSEMBLE)  ->  (not yet implemented)
 *   -B  (BUILD)        ->  Compile then Assemble (not yet implemented)
 *
 *  Returns ASTRAC_OK on success, or an ASTRAC_ERR_* code on failure.
 */
ASTRAC_RESULT START_WORKLOAD() {
    if (args.build_type == NONE) {
        printf("[ASTRAC] Error: no build mode selected (use -A, -C, -D, or -B)\n");
        return ASTRAC_ERR_ARGS;
    }
    switch(args.build_type) {
        case DISASSEMBLE: {
            if (!args.quiet) printf("[ASTRAC] Starting disassembler pipeline...\n");
            return START_DISSASEMBLER();
        }
        case COMPILE:
            printf("[ASTRAC] C compilation not yet implemented.\n");
            return ASTRAC_ERR_COMPILE;
        case ASSEMBLE:
            if (!args.quiet) printf("[ASTRAC] Starting assembler pipeline...\n");
            return START_ASSEMBLING();
        case BUILD:
            printf("[ASTRAC] Full build pipeline not yet implemented.\n");
            return ASTRAC_ERR_COMPILE;
        default:
            printf("[ASTRAC] Error: unknown build mode 0x%X\n", args.build_type);
            return ASTRAC_ERR_INTERNAL;
    }
}

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  ARGUMENT PARSING & MAIN
 * ════════════════════════════════════════════════════════════════════════════
 */

VOID FREE_ARGS() {
    if (!args.got_outfile && args.outfile) {
        MFree(args.outfile);
        args.outfile = NULLPTR;
    }
}

/*
 * Derive a default output filename from the first input file:
 *   foo.ASM  + ASSEMBLE     -> foo.BIN
 *   foo.C    + COMPILE      -> foo.ASM
 *   foo.BIN  + DISASSEMBLE  -> foo.DSM
 */
static VOID DERIVE_OUTPUT_NAME() {
    args.outfile = STRDUP(args.input_files[0]);
    PU8 dot = STRRCHR(args.outfile, '.');
    if (dot) *dot = '\0';
    if (args.build_type & COMPILE)
        STRCAT(args.outfile, ".ASM");
     else if (args.build_type & ASSEMBLE)
        STRCAT(args.outfile, ".BIN");
     else if (args.build_type & DISASSEMBLE)
        STRCAT(args.outfile, ".DSM");
     else
        STRCAT(args.outfile, ".OUT");
}

U32 main(U32 argc, PPU8 argv) {
    /* ── no arguments: show help ────────────────────────────────── */
    if (argc < 2) {
        PRINT_HELP();
        return ASTRAC_ERR_ARGS;
    }

    printf("\n%s\n", TRADEMARK);
    MEMZERO(&args, sizeof(ASTRAC_ARGS));

    /* ── parse command-line arguments ───────────────────────────── */
    for (U32 i = 1; i < argc; i++) {
        PU8 arg = argv[i];

        if (ARG_CMP2("-h", "--help"))         { PRINT_HELP(); return ASTRAC_OK; }
        else if (ARG_CMP2("-V", "--version")) { PRINT_VERSION(); return ASTRAC_OK; }
        else if (ARG_CMP2("-v", "--verbose")) { args.verbose = TRUE; }
        else if (ARG_CMP2("-q", "--quiet"))   { args.quiet = TRUE; }
        else if (ARG_CMP2("-A", "--assemble"))    { args.build_type |= ASSEMBLE; }
        else if (ARG_CMP2("-C", "--compile"))     { args.build_type |= COMPILE; }
        else if (ARG_CMP2("-D", "--disassemble")) { args.build_type |= DISASSEMBLE; }
        else if (ARG_CMP1("--16"))                   { args.dsm_bits = 16; }
        else if (ARG_CMP1("--32"))                   { args.dsm_bits = 32; }
        else if (ARG_CMP2("-B", "--build"))       { args.build_type |= BUILD; }
        else if (ARG_CMP2("-o", "--out")) {
            if (++i < argc) {
                args.outfile = argv[i];
                args.got_outfile = TRUE;
            } else {
                printf("Error: missing filename after -o\n");
                return ASTRAC_ERR_ARGS;
            }
        }
        else if (ARG_CMP2("-I", "--include")) {
            if (++i < argc) {
                if (args.include_count >= MAX_INCLUDES) {
                    printf("Error: too many include directories (max %d)\n", MAX_INCLUDES);
                    return ASTRAC_ERR_ARGS;
                }
                args.includes[args.include_count++] = argv[i];
            } else {
                printf("Error: missing path after -I\n");
                return ASTRAC_ERR_ARGS;
            }
        }
        else if (ARG_CMP2("-M", "--macro")) {
            if (++i < argc) {
                PU8 equ = STRCHR(argv[i], '=');
                PU8 val = NULLPTR;
                if (equ) {
                    *equ = '\0';
                    val = equ + 1;
                }
                if (!DEFINE_MACRO(argv[i], val, &args.macros)) {
                    printf("Error: failed to define macro '%s'\n", argv[i]);
                    return ASTRAC_ERR_ARGS;
                }
            } else {
                printf("Error: define macros as -M NAME[=VALUE]\n");
                return ASTRAC_ERR_ARGS;
            }
        }
        else {
            /* Treat unrecognised arguments as input files */
            if (args.input_file_count >= MAX_INPUT_FILES) {
                printf("Error: too many input files (max %d)\n", MAX_INPUT_FILES);
                return ASTRAC_ERR_ARGS;
            }
            args.input_files[args.input_file_count++] = arg;
        }
    }

    /* ── validate ───────────────────────────────────────────────── */
    if (args.input_file_count == 0) {
        printf("Error: no input files specified.\n");
        PRINT_HELP();
        return ASTRAC_ERR_ARGS;
    }

    if (args.build_type == NONE)
        args.build_type = BUILD;

    if (args.dsm_bits == 0)
        args.dsm_bits = 32;

    if (!args.got_outfile)
        DERIVE_OUTPUT_NAME();

    if (args.verbose) {
        printf("[ASTRAC] Mode: 0x%X  |  Files: %u  |  Output: %s\n",
               args.build_type, args.input_file_count, args.outfile);
    }

    /* ── run pipeline ───────────────────────────────────────────── */
    ASTRAC_RESULT result = START_WORKLOAD();

    if (result != ASTRAC_OK && !args.quiet)
        printf("[ASTRAC] Pipeline failed (error %d)\n", result);
    else if (!args.quiet)
        printf("[ASTRAC] Done.\n");

    FREE_ARGS();
    return (U32)result;
}