#include <PROC/PROC.h>
#include <PROGRAMS/SHELL/SHELL.h>
#include <PROGRAMS/SHELL/BATSH.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>
#include <STD/AUDIO.h>
#include <STD/MEM.h>
#include <STD/PROC_COM.h>

#define LEND "\r\n"

#define MAX_VARS 64
#define MAX_VAR_NAME 32
#define MAX_VAR_VALUE 128

typedef struct {
    U8 name[MAX_VAR_NAME];
    U8 value[MAX_VAR_VALUE];
} ShellVar;


// ===================================================
// Internal Shell State
// ===================================================
static ShellVar shell_vars[MAX_VARS] ATTRIB_DATA = {0};
static U32 shell_var_count ATTRIB_DATA = 0;

static U8 affect_self ATTRIB_DATA = TRUE;
static U8 tmp_line[CUR_LINE_MAX_LENGTH] ATTRIB_DATA = { 0 };
static U8 batsh_mode ATTRIB_DATA = FALSE; // TRUE when command is from BATSH

// ===================================================
// Command Table
// ===================================================

typedef VOID (*ShellCommandHandler)(U8 *line);

// Command entry structure
typedef struct {
    const CHAR *name;
    ShellCommandHandler handler;
    const CHAR *help;
} ShellCommand;

// === Command function prototypes ===
#define CMD_FUNC(CMD) void CMD_##CMD(U8 *line)

CMD_FUNC(HELP);
CMD_FUNC(CLEAR);
CMD_FUNC(VERSION);
CMD_FUNC(EXIT);
CMD_FUNC(ECHO);
CMD_FUNC(TONE);
CMD_FUNC(SOUNDOFF);
CMD_FUNC(UNKNOWN);
CMD_FUNC(CD);
CMD_FUNC(CD_BACKWARDS);
CMD_FUNC(DIR);
CMD_FUNC(MKDIR);
CMD_FUNC(RMDIR);

static const ShellCommand shell_commands[] ATTRIB_RODATA = {
    { "help",     CMD_HELP,     "Show this help message" },
    { "clear",    CMD_CLEAR,    "Clear the screen" },
    { "cls",      CMD_CLEAR,    "Clear the screen" },
    { "version",  CMD_VERSION,  "Show shell version" },
    { "exit",     CMD_EXIT,     "Exit the shell" },
    { "echo",     CMD_ECHO,     "Echo text back to screen" },
    { "tone",     CMD_TONE,     "Play tone: tone <freqHz> <ms> [amp] [rate]" },
    { "soundoff", CMD_SOUNDOFF, "Stop AC97 playback" },
    { "cd",       CMD_CD,       "Change directory" },
    { "cd..",     CMD_CD_BACKWARDS, "Change directory backwards" },
    { "dir",      CMD_DIR,      "List directory contents" },
    { "mkdir",    CMD_MKDIR,    "Create a directory" },
    { "rmdir",    CMD_RMDIR,    "Remove a directory" },
};
#define shell_command_count (sizeof(shell_commands) / sizeof(shell_commands[0]))

VOID BATSH_SET_MODE(U8 mode) {
    batsh_mode = mode;
}

U8 BATSH_GET_MODE(void) {
    return batsh_mode;
}

// Find variable index by name
static S32 FIND_VAR(PU8 name) {
    for (U32 i = 0; i < shell_var_count; i++) {
        if (STRICMP(name, shell_vars[i].name) == 0) return i;
    }
    return -1;
}

PU8 GET_VAR_VALUE(PU8 name) {
    for (U32 i = 0; i < shell_var_count; i++) {
        if (STRICMP(name, shell_vars[i].name) == 0) return shell_vars[i].value;
    }
    return NULLPTR;
}

// Accepts: TRUE, FALSE, ON, OFF, 1, 0
BOOLEAN VAR_VALUE_TRUE(PU8 name) {
    PU8 val = NULLPTR;
    for (U32 i = 0; i < shell_var_count; i++) {
        if (STRICMP(name, shell_vars[i].name) == 0) val = shell_vars[i].value;
    }
    if(!val) return FALSE;
    if(
        STRICMP(val, "1") == 0 ||
        STRICMP(val, "ON") == 0 ||
        STRICMP(val, "TRUE") == 0
    ) return TRUE;
    return FALSE;
}

// Set or update a shell variable
VOID SET_VAR(PU8 name, PU8 value) {
    S32 idx = FIND_VAR(name);
    if (idx >= 0) {
        STRNCPY(shell_vars[idx].value, value, MAX_VAR_VALUE - 1);
    } else if (shell_var_count < MAX_VARS) {
        STRNCPY(shell_vars[shell_var_count].name, name, MAX_VAR_NAME - 1);
        STRNCPY(shell_vars[shell_var_count].value, value, MAX_VAR_VALUE - 1);
        shell_var_count++;
    }
}

// Get variable value
PU8 GET_VAR(PU8 name) {
    S32 idx = FIND_VAR(name);
    return (idx >= 0) ? shell_vars[idx].value : (PU8)"";
}

BOOLEAN CREATE_VAR(PU8 name, PU8 value) {
    if (!name || !*name) return FALSE;

    // Trim whitespace from name
    U32 len = STRLEN(name);
    while (len > 0 && (name[len - 1] == ' ' || name[len - 1] == '\t')) name[--len] = '\0';

    S32 idx = FIND_VAR(name);
    if (idx >= 0) {
        // Update existing variable
        STRNCPY(shell_vars[idx].value, value ? value : (PU8)"" , MAX_VAR_VALUE - 1);
    } else {
        // Create new variable
        if (shell_var_count >= MAX_VARS) return FALSE;
        STRNCPY(shell_vars[shell_var_count].name, name, MAX_VAR_NAME - 1);
        STRNCPY(shell_vars[shell_var_count].value, value ? value : (PU8)"" , MAX_VAR_VALUE - 1);
        shell_var_count++;
    }
    return TRUE;
}

// Replace @var in line with its value (in-place)
VOID REPLACE_VARS_IN_LINE(PU8 line) {
    U8 buffer[SHELL_SCRIPT_LINE_MAX_LEN] = {0};
    U32 bi = 0, li = 0;

    while (line[li] && bi < sizeof(buffer) - 1) {
        if (line[li] == '@') {
            li++;
            U8 varname[MAX_VAR_NAME] = {0};
            U32 vi = 0;
            while (line[li] && line[li] != ' ' && line[li] != '\t' && line[li] != '\n' && vi < MAX_VAR_NAME - 1)
                varname[vi++] = line[li++];
            varname[vi] = '\0';
            PU8 value = GET_VAR(varname);
            for (U32 j = 0; value[j] && bi < sizeof(buffer) - 1; j++) buffer[bi++] = value[j];
        } else {
            buffer[bi++] = line[li++];
        }
    }
    buffer[bi] = '\0';
    STRNCPY(line, buffer, SHELL_SCRIPT_LINE_MAX_LEN - 1);
}

// ===================================================
// Shell Echo Control
// ===================================================
VOID SHELL_SET_ECHO(U8 on) { 
    CREATE_VAR("echo", on ? "on" : "off");
}
U8 SHELL_GET_ECHO(void) { 
    return VAR_VALUE_TRUE("echo"); 
}

// ===================================================
// BATSH Line Parser
// ===================================================
BOOLEAN HANDLE_BATSH_LINE(PU8 line) {
    if (!line || !*line) return TRUE;

    while (*line == ' ' || *line == '\t') line++;

    if (*line == '\0' || *line == '#')
        return TRUE;

    // Variable assignment
    if (*line == '@') {
        line++; // skip '@'
        U8 varname[MAX_VAR_NAME] = {0};
        U8 varvalue[MAX_VAR_VALUE] = {0};
        U32 i = 0;
        while (line[i] && line[i] != '=' && i < MAX_VAR_NAME - 1) { varname[i] = line[i]; i++; }
        varname[i] = '\0';
        if (line[i] == '=') {
            STRNCPY(varvalue, line + i + 1, MAX_VAR_VALUE - 1);
            CREATE_VAR(varname, varvalue);
        }
        if(!BATSH_GET_MODE()) PRINTNEWLINE();
        return TRUE;
    }

    REPLACE_VARS_IN_LINE(line);

    if (STRNICMP(line, "pause", 5) == 0) {
        PUTS((PU8)"Press any key to continue..." LEND);
        // SHELL_WAIT_KEY();
        return TRUE;
    }

    // Flag to indicate BATSH execution
    HANDLE_COMMAND(line);

    return TRUE;
}

// ===================================================
// Core Command Handler
// ===================================================
VOID HANDLE_COMMAND(U8 *line) {
    OutputHandle cursor = GetOutputHandle();
    cursor->CURSOR_VISIBLE = FALSE;

    if (STRLEN(line) == 0) {
        PRINTNEWLINE();
        cursor->CURSOR_VISIBLE = TRUE;
        return;
    }

    // Extract command name
    U8 command[64];
    U32 i = 0;
    while (line[i] && line[i] != ' ' && i < sizeof(command) - 1)
        command[i++] = line[i];
    command[i] = '\0';

    BOOLEAN found = FALSE;
    for (U32 j = 0; j < shell_command_count; j++) {
        if (STRICMP((CHAR*)command, shell_commands[j].name) == 0) {
            shell_commands[j].handler(line);
            found = TRUE;
            break;
        }
    }

    if (!found)
        CMD_UNKNOWN(line);

    cursor->CURSOR_VISIBLE = TRUE;
}

// ===================================================
// BATSH File Runner
// ===================================================
BOOLEAN RUN_BATSH_FILE(FILE *file) {
    if (!file) return FALSE;
    BATSH_SET_MODE(TRUE);
    OutputHandle crs = GetOutputHandle();
    U32 prev = crs->CURSOR_VISIBLE;
    crs->CURSOR_VISIBLE = FALSE;
    U8 line[SHELL_SCRIPT_LINE_MAX_LEN] = { 0 };
    while (FILE_GET_LINE(file, line, SHELL_SCRIPT_LINE_MAX_LEN)) {
        U32 len = STRLEN(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        if (SHELL_GET_ECHO()) {
            PUTS(line);
        }
        HANDLE_BATSH_LINE(line);
    }
    crs->CURSOR_VISIBLE = prev;
    return TRUE;
}

// ===================================================
// BATSH Script Loader
// ===================================================
BOOLEAN RUN_BATSH_SCRIPT(PU8 path) {
    PU8 dot = STRRCHR(path, '.');
    if (!dot || STRNICMP(dot, ".SH", 3) != 0)
        return FALSE;

    FILE file;
    if (!FOPEN(&file, path, MODE_R | MODE_FAT32)) {
        PUTS((PU8)"Unable to open script file." LEND);
        return FALSE;
    }

    BOOLEAN res = RUN_BATSH_FILE(&file);
    FCLOSE(&file);
    return res;
}

// ===================================================
// Command Implementations
// ===================================================
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

VOID CMD_CLEAR(U8 *line) { (void)line; LE_CLS(); }
VOID CMD_VERSION(U8 *line) { (void)line; PRINTNEWLINE(); PUTS((U8*)"atOS Shell version 1.0.0" LEND); }
VOID CMD_EXIT(U8 *line) { (void)line; PRINTNEWLINE(); PUTS((U8*)"Exiting shell..." LEND); }

VOID CMD_ECHO(U8 *line) {
    PRINTNEWLINE();
    if (STRLEN(line) > 5) PUTS(line + 5);
}

VOID CMD_TONE(U8 *line) {
    U8 *s = line + STRLEN("tone ");
    U32 freq = ATOI(s);
    while (*s && *s != ' ') s++;
    if (*s == ' ') s++;
    U32 ms = ATOI(s);
    while (*s && *s != ' ') s++;

    U32 amp = 8000;
    if (*s == ' ') { s++; amp = ATOI(s); }
    while (*s && *s != ' ') s++;

    U32 rate = 48000;
    if (*s == ' ') { s++; rate = ATOI(s); }

    PRINTNEWLINE();
    if (freq == 0 || ms == 0) {
        PUTS((U8*)"Usage: tone <freqHz> <ms> [amp] [rate]" LEND);
        return;
    }

    U32 res = AUDIO_TONE(freq, ms, amp, rate);
    PUTS(res ? (U8*)"tone: playing..." LEND : (U8*)"tone: AC97 not available" LEND);
}

VOID CMD_SOUNDOFF(U8 *line) { (void)line; AUDIO_STOP(); PUTS((U8*)"AC97: stopped" LEND); }

VOID CMD_UNKNOWN(U8 *line) {
    if(!RUN_PROCESS(line)) {
        PRINTNEWLINE();
        PUTS((U8*)"Unknown command or file type: ");
        PUTS(line);
        PRINTNEWLINE();
    }
}

// Directory-related
VOID CMD_CD(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);

    PU8 path_arg = PARSE_CD_RAW_LINE(tmp_line, 2);
    if (STRCMP(path_arg, "-") == 0) CD_PREV();
    else if (STRCMP(path_arg, "~") == 0) CD_INTO((PU8)"/HOME");
    else if (!path_arg || *path_arg == '\0') CD_INTO((PU8)"/");
    else CD_INTO(path_arg);
}

VOID CMD_CD_BACKWARDS(PU8 line) { (void)line; CD_BACKWARDS_DIR(); PRINTNEWLINE(); }

VOID CMD_DIR(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);

    PU8 path_arg = PARSE_CD_RAW_LINE(tmp_line, 3);
    if (STRCMP(path_arg, "-") == 0) path_arg = GET_PREV_PATH();
    else if (STRCMP(path_arg, "~") == 0) path_arg = (PU8)"/HOME";
    else if (!path_arg || *path_arg == '\0') path_arg = (PU8)".";
    PRINT_CONTENTS_PATH(path_arg);
}

VOID CMD_MKDIR(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);
    PU8 path_arg = PARSE_CD_RAW_LINE(tmp_line, 5);
    MAKE_DIR(path_arg);
}

VOID CMD_RMDIR(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);
    PU8 path_arg = PARSE_CD_RAW_LINE(tmp_line, 5);
    if (!REMOVE_DIR(path_arg)) return;
}

VOID SETUP_BATSH_PROCESSER() {
    CREATE_VAR("echo", "on");
}


BOOLEAN RUN_PROCESS(PU8 line) {
    if (!line || !*line) return FALSE;

    // Copy and trim input line
    STRNCPY(tmp_line, line, sizeof(tmp_line) - 1);
    str_trim(tmp_line);

    // Extract first token (program path)
    PU8 first_space = STRCHR(tmp_line, ' ');
    U32 prog_len = first_space ? (U32)(first_space - tmp_line) : STRLEN(tmp_line);

    // argv[0] = program name only (last path component)
    PU8 prog_name = MAlloc(prog_len + 1);
    STRNCPY(prog_name, tmp_line, prog_len);
    prog_name[prog_len] = 0;

    PU8 last_slash = STRRCHR(prog_name, '/');
    if (last_slash) {
        PU8 tmp = STRDUP(last_slash + 1);
        MFree(prog_name);
        prog_name = tmp;
    }

    // Resolve full path (absolute)
    PU8 full_path = MAlloc(prog_len + 1);
    STRNCPY(full_path, tmp_line, prog_len);
    full_path[prog_len] = 0;

    if (!ResolvePath(full_path, tmp_line, sizeof(tmp_line))) {
        PUTS("\nError: Failed to resolve path.\n");
        MFree(prog_name);
        MFree(full_path);
        return FALSE;
    }
    MFree(full_path);

    PU8 abs_path = tmp_line;
    PUTS(abs_path);

    // Verify file exists
    FAT_LFN_ENTRY ent;
    if (!FAT32_PATH_RESOLVE_ENTRY(abs_path, &ent)) {
        PUTS("\nError: File not found.\n");
        MFree(prog_name);
        return FALSE;
    }

    // Read file contents
    U32 sz = 0;
    VOIDPTR data = FAT32_READ_FILE_CONTENTS(&sz, &ent.entry);
    if (!data) {
        MFree(prog_name);
        return FALSE;
    }

    U32 pid = PROC_GETPID();

    // -------------------------
    // Argument parsing
    // -------------------------
    #define MAX_ARGS 12
    PU8* argv = MAlloc(sizeof(PU8*) * MAX_ARGS);
    U32 argc = 0;
    argv[argc++] = prog_name; // argv[0]

    PU8 args_start = first_space ? first_space + 1 : NULL;
    if (args_start && *args_start) {
        PU8 p = args_start;
        while (*p && argc < MAX_ARGS) {
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;

            PU8 arg_start = p;
            BOOL in_quotes = FALSE;

            if (*p == '"') {
                in_quotes = TRUE;
                arg_start = ++p;
                while (*p && *p != '"') p++;
            } else {
                while (*p && *p != ' ' && *p != '\t') p++;
            }

            U32 len = p - arg_start;
            PU8 arg = MAlloc(len + 1);
            STRNCPY(arg, arg_start, len);
            arg[len] = 0;

            argv[argc++] = arg;

            if (in_quotes && *p == '"') p++;
        }
    }

    // -------------------------
    // Determine file type and execute
    // -------------------------
    PU8 ext = STRRCHR(abs_path, '.');
    if (ext) ext++; // skip dot
    else ext = (PU8)"";

    if (STRICMP(ext, "BIN") == 0) {
        // Execute binary
        U32 res = START_PROCESS(abs_path, data, sz, TCB_STATE_ACTIVE, pid, argv, argc);
        if (res) PRINTNEWLINE();
        return res != 0;
    } else if (STRICMP(ext, "SH") == 0) {
        // TODO: run as shell script
    } else {
        return FALSE;
    }
}
