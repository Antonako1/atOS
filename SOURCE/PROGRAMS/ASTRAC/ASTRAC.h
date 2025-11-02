#ifndef ASTRAC_H
#define ASTRAC_H
#include <STD/TYPEDEF.h>

#define VERSION "0.0.1"
#define TRADEMARK "AstraC Compiler, Assembler and Dissasembler"

typedef enum {
    NONE = 0x0000,
    COMPILE = 0x0001,
    ASSEMBLE =0x0002,
    DISSASEMBLE = 0x0004,
    PREPROCESS = 0x0008,
    BUILD = COMPILE | ASSEMBLE | PREPROCESS,
} BUILD_TYPE;

#define MAX_MACROS 255
#define MAX_INCLUDES 255
#define MAX_INPUT_FILES 255

typedef struct _ASTRAC_ARGS {
    PU8 macros[MAX_MACROS];
    U32 macro_count;

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