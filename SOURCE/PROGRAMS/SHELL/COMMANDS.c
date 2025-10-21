#include "COMMANDS.h"
#include <PROGRAMS/SHELL/SHELL.h>
#include <STD/STRING.h>
#include <STD/AUDIO.h>

#define LEND "\r\n"


// ======================
// Command Table
// ======================

static const ShellCommand shell_commands[] ATTRIB_DATA = {
    { "help",     CMD_HELP,     "Show this help message" },
    { "clear",    CMD_CLEAR,    "Clear the screen" },
    { "cls",      CMD_CLEAR,    "Clear the screen" },
    { "version",  CMD_VERSION,  "Show shell version" },
    { "exit",     CMD_EXIT,     "Exit the shell" },
    { "echo",     CMD_ECHO,     "Echo text back to screen" },
    { "tone",     CMD_TONE,     "Play tone: tone <freqHz> <ms> [amp] [rate]" },
    { "soundoff", CMD_SOUNDOFF, "Stop AC97 playback" },
};
#define shell_command_count (sizeof(shell_commands) / sizeof(shell_commands[0]))

// ======================
// Command Implementations
// ======================

VOID CMD_HELP(U8 *line) {
    (void)line;
    PRINTNEWLINE();
    PUTS((U8*)"Available commands:" LEND);
    for (U32 i = 0; i < shell_command_count; i++) {
        PUTS((U8*)shell_commands[i].name);
        PUTS((U8*)" - ");
        PUTS((U8*)shell_commands[i].help);
        PRINTNEWLINE();
    }
}

VOID CMD_CLEAR(U8 *line) {
    (void)line;
    LE_CLS();
}

VOID CMD_VERSION(U8 *line) {
    (void)line;
    PRINTNEWLINE();
    PUTS((U8*)"atOS Shell version 1.0.0" LEND);
}

VOID CMD_EXIT(U8 *line) {
    (void)line;
    PRINTNEWLINE();
    PUTS((U8*)"Exiting shell..." LEND);
    // TODO: trigger shell termination event
}

VOID CMD_ECHO(U8 *line) {
    PRINTNEWLINE();
    if (STRLEN(line) > 5) {
        PUTS(line + 5);
        PRINTNEWLINE();
    } else {
        PRINTNEWLINE();
    }
}


VOID CMD_TONE(U8 *line) {
    U8 *s = line + STRLEN("tone ");
    U32 freq = ATOI(s);
    while (*s && *s != ' ') s++;
    if (*s == ' ') s++;
    U32 ms = ATOI(s);
    while (*s && *s != ' ') s++;

    U32 amp = 8000; // default
    if (*s == ' ') { s++; amp = ATOI(s); }
    while (*s && *s != ' ') s++;

    U32 rate = 48000; // default
    if (*s == ' ') { s++; rate = ATOI(s); }

    PRINTNEWLINE();
    if (freq == 0 || ms == 0) {
        PUTS((U8*)"Usage: tone <freqHz> <ms> [amp] [rate]" LEND);
        return;
    }

    U32 res = AUDIO_TONE(freq, ms, amp, rate);
    if (!res)
        PUTS((U8*)"tone: AC97 not available or failed" LEND);
    else
        PUTS((U8*)"tone: playing..." LEND);
}

VOID CMD_SOUNDOFF(U8 *line) {
    (void)line;
    PRINTNEWLINE();
    AUDIO_STOP();
    PUTS((U8*)"AC97: stopped" LEND);
}

VOID CMD_UNKNOWN(U8 *line) {
    PRINTNEWLINE();
    PUTS((U8*)"Unknown command: ");
    PUTS(line);
    PRINTNEWLINE();
}





U0 HANDLE_COMMAND(U8 *line) {
    OutputHandle cursor = GetOutputHandle();
    cursor->CURSOR_VISIBLE = FALSE;

    if (STRLEN(line) == 0) {
        PRINTNEWLINE();
        cursor->CURSOR_VISIBLE = TRUE;
        return;
    }

    // Extract first word (command)
    U8 command[64];
    U32 i = 0;

    while (line[i] && line[i] != ' ' && i < sizeof(command) - 1) {
        command[i] = line[i];
        i++;
    }
    command[i] = '\0';


    // Find command
    BOOLEAN found = FALSE;
    for (U32 j = 0; j < shell_command_count; j++) {
        if (STRCASECMP((CHAR*)command, shell_commands[j].name) == 0) {
            shell_commands[j].handler(line);
            found = TRUE;
            break;
        }
    }

    if (!found) {
        CMD_UNKNOWN(line);
    }

    cursor->CURSOR_VISIBLE = TRUE;
}