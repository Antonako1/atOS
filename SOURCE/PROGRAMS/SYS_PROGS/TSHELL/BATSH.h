#ifndef BATSH_H
#define BATSH_H

#include <STD/TYPEDEF.h>
#include <STD/FS_DISK.h>

#define SHELL_SCRIPT_LINE_MAX_LEN 512
#define MAX_VARS 64
#define MAX_VAR_NAME 32
#define MAX_VAR_VALUE 256
#define MAX_ARGS 10

typedef struct {
    U8 name[MAX_VAR_NAME];
    U8 value[MAX_VAR_VALUE];
} ShellVar;

typedef struct _BATSH_INSTANCE {
    FILE *file;
    ShellVar shell_vars[MAX_VARS];
    U32 shell_var_count;

    PU8 args[MAX_ARGS];
    U32 arg_count;

    VOIDPTR shndl;

    U32 status_code;

    struct _BATSH_INSTANCE *child_proc;
    struct _BATSH_INSTANCE *parent_proc;

    U8 echo;
    U8 batsh_mode;

    U8 line[SHELL_SCRIPT_LINE_MAX_LEN];
    U32 line_len;
} BATSH_INSTANCE;

#endif /* BATSH_H */
