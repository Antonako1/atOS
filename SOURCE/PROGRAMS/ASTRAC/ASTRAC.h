#ifndef ASTRAC_H
#define ASTRAC_H
#include <STD/TYPEDEF.h>
#include <STD/FS_DISK.h>

#define VERSION "0.0.1"
#define TRADEMARK "AstraC Compiler, Assembler and Disassembler"

// debug mode print
#ifdef DEBUG
#define DEBUGM_PRINTF(...) DEBUG_PRINTF(__VA_ARGS__)
#else 
#define DEBUGM_PRINTF(...) ((void)0)
#endif
/*
 * ── BUILD MODES ─────────────────────────────────────────────────────────────
 * These flags control which stage(s) of the pipeline run.
 * They can be combined (BUILD = COMPILE | ASSEMBLE).
 */
typedef enum {
    NONE        = 0x0000,
    COMPILE     = 0x0001,   /* C source  -> ASM output          */
    ASSEMBLE    = 0x0002,   /* ASM input -> binary output        */
    BUILD       = COMPILE | ASSEMBLE,  /* Full pipeline: C -> ASM -> BIN */
    DISASSEMBLE = 0x0004,   /* Binary    -> readable ASM output  */
} BUILD_TYPE;

/*
 * ── RETURN / ERROR CODES ────────────────────────────────────────────────────
 * Every pipeline stage returns one of these so callers can diagnose failures.
 */
typedef enum {
    ASTRAC_OK               = 0,    /* Success                     */
    ASTRAC_ERR_ARGS         = 1,    /* Bad or missing arguments    */
    ASTRAC_ERR_PREPROCESS   = 2,    /* Preprocessor stage failed   */
    ASTRAC_ERR_LEX          = 3,    /* Lexer stage failed          */
    ASTRAC_ERR_AST          = 4,    /* AST construction failed     */
    ASTRAC_ERR_VERIFY       = 5,    /* AST verification failed     */
    ASTRAC_ERR_OPTIMIZE     = 6,    /* Optimization pass failed    */
    ASTRAC_ERR_CODEGEN      = 7,    /* Code generation failed      */
    ASTRAC_ERR_DISASSEMBLE  = 8,    /* Disassembly failed          */
    ASTRAC_ERR_COMPILE      = 9,    /* Compilation failed          */
    ASTRAC_ERR_INTERNAL     = 0xFF, /* Internal / unexpected error */
} ASTRAC_RESULT;

/*
 * ── LIMITS ──────────────────────────────────────────────────────────────────
 */
#define MAX_MACROS          255
#define MAX_INCLUDES        255
#define MAX_INPUT_FILES     255
#define MAX_MACRO_VALUE     255
#define BUF_SZ              512
#define MAX_FILES           MAX_INPUT_FILES

/*
 * ── PREPROCESSOR MODES ──────────────────────────────────────────────────────
 */
#define ASM_PREPROCESSOR    1
#define C_PREPROCESSOR      2

/*
 * ── MACRO STORAGE ───────────────────────────────────────────────────────────
 */
typedef struct {
    U8 name[MAX_MACRO_VALUE];
    U8 value[MAX_MACRO_VALUE];
} MACRO, *PMACRO;

typedef struct {
    PMACRO macros[MAX_MACROS];
    U32 len;
} MACRO_ARR;

/* Macro manipulation */
BOOL   DEFINE_MACRO(PU8 name, PU8 value, MACRO_ARR *arr);
PMACRO GET_MACRO(PU8 name, MACRO_ARR *arr);
VOID   UNDEFINE_MACRO(PU8 name, MACRO_ARR *arr);
VOID   FREE_MACROS(MACRO_ARR *arr);

/*
 * ── LINE READING ────────────────────────────────────────────────────────────
 * READ_LOGICAL_LINE coalesces backslash-continued physical lines into one
 * logical line in `out`.  Returns TRUE on success, FALSE on EOF or error.
 */
BOOL READ_LOGICAL_LINE(FILE *file, U8 *out, U32 out_size);
BOOL IS_EMPTY(PU8 line);

/*
 * ── ARGUMENT STRUCTURE ──────────────────────────────────────────────────────
 * Filled once by main() argument parsing; read by every pipeline stage via
 * GET_ARGS().
 */
typedef struct _ASTRAC_ARGS {
    MACRO_ARR macros;

    PU8 includes[MAX_INCLUDES];
    U32 include_count;

    PU8 outfile;
    BOOL got_outfile;

    PU8 input_files[MAX_INPUT_FILES];
    U32 input_file_count;

    BOOL verbose;
    BOOL quiet;

    BUILD_TYPE build_type;
} ASTRAC_ARGS;

ASTRAC_ARGS* GET_ARGS();
VOID         FREE_ARGS();

#endif // ASTRAC_H