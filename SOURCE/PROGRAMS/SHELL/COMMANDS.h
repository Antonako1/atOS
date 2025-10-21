#ifndef COMMANDS_H
#define COMMANDS_H
#include <STD/TYPEDEF.h>


// Function prototypes for command handling

U0 HANDLE_COMMAND(U8 *line);

typedef VOID (*ShellCommandHandler)(U8 *line);

// Command entry structure
typedef struct {
    const CHAR *name;
    ShellCommandHandler handler;
    const CHAR *help;
} ShellCommand;

// === Command function prototypes ===
VOID CMD_HELP(U8 *line);
VOID CMD_CLEAR(U8 *line);
VOID CMD_VERSION(U8 *line);
VOID CMD_EXIT(U8 *line);
VOID CMD_ECHO(U8 *line);
VOID CMD_TONE(U8 *line);
VOID CMD_SOUNDOFF(U8 *line);
VOID CMD_UNKNOWN(U8 *line);


#endif // COMMANDS_H