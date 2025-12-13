#ifndef BATSH_H
#define BATSH_H
#include <STD/TYPEDEF.h>
#include <STD/FS_DISK.h>

#ifdef __SHELL__


#define SHELL_SCRIPT_LINE_MAX_LEN 512
#define MAX_VARS 64
#define MAX_VAR_NAME 32
#define MAX_VAR_VALUE 256
#define MAX_ARGS 10

typedef struct {
    U8 name[MAX_VAR_NAME];
    U8 value[MAX_VAR_VALUE];
} ShellVar;

typedef struct {
    FILE *file;
    ShellVar shell_vars[MAX_VARS];
    U32 shell_var_count;

    PU8 args[MAX_ARGS];
    U32 arg_count;

    VOIDPTR shndl; // VOIDPTR because SHELL.h can't be included here. Safe to cast

    U32 status_code;
    
    struct BATSH_INSTANCE *child_proc;
    struct BATSH_INSTANCE *parent_proc;
    

    U8 echo;
    U8 batsh_mode;

    U8 line[SHELL_SCRIPT_LINE_MAX_LEN];
    U32 line_len;
} BATSH_INSTANCE;

U0 HANDLE_COMMAND(U8 *line);

BOOLEAN RUN_BATSH_FILE(BATSH_INSTANCE *inst);
BOOLEAN RUN_BATSH_SCRIPT(PU8 path, U32 argc, PPU8 argv);
BATSH_INSTANCE *CREATE_BATSH_INSTANCE(PU8 path, U32 argc, PPU8 argv);
VOID DESTROY_BATSH_INSTANCE(BATSH_INSTANCE *inst);

// Parses a line of batsh input and executes it. Use NULLPTR for global instance
BOOLEAN PARSE_BATSH_INPUT(PU8 line, BATSH_INSTANCE *inst);
VOID SETUP_BATSH_PROCESSER();

VOID BATSH_SET_MODE(U8 mode);
U8 BATSH_GET_MODE(void);
BOOLEAN RUN_PROCESS(PU8 line);

VOID SET_VAR(PU8 name, PU8 value);
PU8 GET_VAR(PU8 name);
#endif // __SHELL__

#endif // BATSH_H