#ifndef ASTRAC_H
#define ASTRAC_H
#include <STD/TYPEDEF.h>
#include <STD/FS_DISK.h>

#define VERSION "0.0.1"
#define TRADEMARK "AstraC Compiler, Assembler and Dissasembler"

typedef enum {
    NONE = 0x0000,
    COMPILE = 0x0001,
    ASSEMBLE =0x0002,
    DISSASEMBLE = 0x0004,
    BUILD = COMPILE | ASSEMBLE,
} BUILD_TYPE;


#define MAX_MACROS 255
#define MAX_INCLUDES 255
#define MAX_INPUT_FILES 255

#define MAX_MACRO_VALUE 255
#define BUF_SZ 512

#define ASM_PREPROCESSOR 1
#define C_PREPROCESSOR 2

#define MAX_FILES MAX_INPUT_FILES

typedef struct {
    U8 name[MAX_MACRO_VALUE];
    U8 value[MAX_MACRO_VALUE];
} MACRO, *PMACRO;

typedef struct {
    PMACRO macros[MAX_MACROS];
    U32 len;
} MACRO_ARR;



BOOL DEFINE_MACRO(PU8 name, PU8 value, MACRO_ARR *arr);
PMACRO GET_MACRO(PU8 name, MACRO_ARR *arr);
VOID UNDEFINE_MACRO(PU8 name, MACRO_ARR *arr);
VOID FREE_MACROS(MACRO_ARR *arr);

// returns TRUE if a logical (possibly multi-physical) line was read into `out`
// out_size is the byte size of out buffer. Uses FILE_GET_LINE(file, tmp, sizeof(tmp))
// as your physical line reader. Returns FALSE on EOF/no-line or on error.
BOOL READ_LOGICAL_LINE(FILE *file, U8 *out, U32 out_size);
BOOL IS_EMPTY(PU8 line);

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

#endif ASTRAC_H