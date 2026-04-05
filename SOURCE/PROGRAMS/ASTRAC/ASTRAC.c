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
        "Usage: ASTRAC.BIN {file.{AC|ASM|BIN}} [options]\n\n"

        "GENERAL OPTIONS:\n"
            "\t-h, --help                      Show this help message\n"
            "\t    --version                   Show compiler version\n"
            "\t-v, --verbose                   Increase verbosity\n"
            "\t-o <file>                       Write output to <file>\n"
            "\t-q, --quiet                     Suppress non-error output\n"
            "\t-o, --out <file>                Specify output file name\n"
            "\t-D <macro>[=val]                Define macro for preprocessor (like -D NAME=1)\n"
            "\t-E,                             Preprocess only, do not assemble/compile AC -> AC/ ASM -> ASM\n"
            "\t-O, --org <address>             Set origin address for assembly output (default: 0x10000000)\n\n"

        "COMPILER OPTIONS:\n"
            "\t-C, --compile                   Compile source to binary AC -> BIN\n"
            "\t    --no-runtime                Do not link the runtime library\n"
            "\t    --no-std                    Do not link the standard library\n"
            "\t-g, --gui                       GUI mode\n"
            "\t-c, --console                   Console mode\n"
            "\t-w, --warnings-as-errors        Treat warnings as errors\n"
            "\t    --debug                     Emit debug symbols in the assembly representation\n"
            "\t-S                              Emit assembly and stop AC -> ASM\n\n"

        "ASSEMBLER OPTIONS:\n"
            "\t-A, --assemble                  Assemble ASM -> BIN\n\n"

        "DISASSEMBLER OPTIONS:\n"
            "\t--disassemble                   Disassemble BIN -> DSM\n"
            "\t    --16                        Disassemble as 16-bit code\n"
            "\t    --32                        Disassemble as 32-bit code (default)\n\n"

        "INFORMATION:\n"
            "\t-V, --version                   Show version info and exit\n"
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
 *   -A  (ASSEMBLE)        ->  Preprocess -> Lex -> AST -> Verify -> Optimize -> Gen
 *   -C  (COMPILE)         ->  Preprocess -> Lex -> Parse -> Verify -> Codegen (.ASM)
 *   -D  (DISASSEMBLE)     ->  Binary -> human-readable ASM
 *   -B  (BUILD)           ->  Compile then Assemble (C -> ASM -> BIN)
 *   -E  (PREPROCESS_ONLY) ->  Preprocess only
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
        case PREPROCESS_ONLY:
            if (!args.quiet) printf("[ASTRAC] Starting preprocessor only...\n");
            /* fall through to compiler which runs preprocessing */
            return START_COMPILER();
        case COMPILE:
            if (!args.quiet) printf("[ASTRAC] Starting compiler pipeline...\n");
            return START_COMPILER();
        case BUILD:
            if (!args.quiet) printf("[ASTRAC] Starting full build pipeline...\n");
            return START_COMPILER();
        case ASSEMBLE:
            if (!args.quiet) printf("[ASTRAC] Starting assembler pipeline...\n");
            return START_ASSEMBLING();
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
 *   foo.ASM  + ASSEMBLE         -> foo.BIN
 *   foo.AC   + COMPILE (-S)     -> foo.ASM
 *   foo.AC   + COMPILE          -> foo.BIN  (via BUILD)
 *   foo.BIN  + DISASSEMBLE      -> foo.DSM
 *   foo.AC   + PREPROCESS_ONLY  -> foo.AC   (preprocessed)
 */
static VOID DERIVE_OUTPUT_NAME() {
    args.outfile = STRDUP(args.input_files[0]);
    PU8 dot = STRRCHR(args.outfile, '.');
    if (dot) *dot = '\0';
    if (args.build_type == PREPROCESS_ONLY)
        STRCAT(args.outfile, ".PP");             /* preprocessed source */
    else if (args.emit_asm_only)
        STRCAT(args.outfile, ".ASM");            /* -S: stop after compile */
    else if (args.build_type & ASSEMBLE)
        STRCAT(args.outfile, ".BIN");
    else if (args.build_type & COMPILE)
        STRCAT(args.outfile, ".ASM");            /* -C without -A */
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
        /* Pipeline modes */
        else if (ARG_CMP2("-A", "--assemble"))    { args.build_type |= ASSEMBLE; }
        else if (ARG_CMP2("-C", "--compile"))     { args.build_type |= BUILD; }
        else if (ARG_CMP1("--disassemble"))        { args.build_type |= DISASSEMBLE; }
        else if (ARG_CMP1("-E"))                  { args.build_type |= PREPROCESS_ONLY; }
        /* Disassembler bitness */
        else if (ARG_CMP1("--16"))                { args.dsm_bits = 16; }
        else if (ARG_CMP1("--32"))                { args.dsm_bits = 32; }
        /* Compiler options */
        else if (ARG_CMP1("--no-runtime"))        { args.no_runtime = TRUE; }
        else if (ARG_CMP1("--no-std"))            { args.no_std = TRUE; }
        else if (ARG_CMP2("-g", "--gui"))         { args.gui = TRUE; }
        else if (ARG_CMP2("-c", "--console"))     { args.console = TRUE; }
        else if (ARG_CMP2("-w", "--warnings-as-errors")) { args.warnings_as_errors = TRUE; }
        else if (ARG_CMP1("--debug"))             { args.debug_symbols = TRUE; }
        else if (ARG_CMP1("-S"))                  { args.emit_asm_only = TRUE; args.build_type |= COMPILE; }
        else if (ARG_CMP2("-O", "--org")) {
            if (++i < argc) {
                /* Accept decimal or hex (0x...) */
                BOOL ok = FALSE;
                PU8 a = argv[i];
                if (a[0] == '0' && (a[1] == 'x' || a[1] == 'X'))
                    ok = ATOI_HEX_E(a + 2, &args.org);
                else
                    ok = ATOI_E(a, &args.org);
                if (!ok) {
                    printf("Error: invalid address after -O: '%s'\n", argv[i]);
                    return ASTRAC_ERR_ARGS;
                }
            } else {
                printf("Error: missing address after -O\n");
                return ASTRAC_ERR_ARGS;
            }
        }
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
        else if (ARG_CMP1("-D")) {
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
                printf("Error: define macros as -D NAME[=VALUE]\n");
                return ASTRAC_ERR_ARGS;
            }
        }
        else if (ARG_CMP2("-i", "--info")) {
            if (++i < argc) {
                printf("[ASTRAC] Info lookup for '%s': not yet implemented.\n", argv[i]);
                return ASTRAC_OK;
            } else {
                printf("Error: missing keyword after -i (usage: -i [asm|c]=<keyword>)\n");
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

    if (args.org == 0)
        args.org = 0x10000000;

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