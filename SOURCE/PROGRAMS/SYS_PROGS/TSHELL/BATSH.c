/*
 * BATSH.c — BATSH scripting language for TSHELL
 *
 * Ported from PROGRAMS/SHELL/BATSH.c with:
 *   - Bug fix: resolve_vars `IS_DIGIT_STR(value)` → `IS_DIGIT_STR(var_name)`
 *   - Implemented `if COND then CMD [else CMD] fi`
 *   - Implemented `loop COND` / `end`
 *   - All output via TPUT_* macros (PUTS/PUTC/PUT_DEC/etc.)
 */

#include <PROC/PROC.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>
#include <STD/AUDIO.h>
#include <STD/MEM.h>
#include <STD/PROC_COM.h>
#include <STD/DEBUG.h>
#include <PROGRAMS/SYS_PROGS/TSHELL/TSHELL.h>
#include <PROGRAMS/SYS_PROGS/TSHELL/BATSH.h>

/* ===================================================
 * Internal Shell State
 * =================================================== */
static ShellVar shell_vars[MAX_VARS] ATTRIB_DATA = {0};
static U32 shell_var_count ATTRIB_DATA = 0;

static U8 tmp_line[CUR_LINE_MAX_LENGTH] ATTRIB_DATA = { 0 };
static U8 batsh_mode ATTRIB_DATA = FALSE;

/* ===================================================
 * Command Table
 * =================================================== */

typedef VOID (*ShellCommandHandler)(U8 *line);

typedef struct {
    const CHAR *name;
    ShellCommandHandler handler;
    const CHAR *help;
} ShellCommand;

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
CMD_FUNC(TYPE);
CMD_FUNC(TAIL);
CMD_FUNC(COLOUR);
CMD_FUNC(SHELL);
CMD_FUNC(RESTART);
CMD_FUNC(SHUTDOWN);
#define CMD_NONE NULLPTR

/* =====================================
 * Batsh tokenizer and parser
 * ===================================== */

typedef enum {
    TOK_UNKNOWN,
    TOK_CMD,
    TOK_WORD,
    TOK_STRING,
    TOK_AND,
    TOK_OR,
    TOK_EQU,
    TOK_NEQ,
    TOK_LSS,
    TOK_LEQ,
    TOK_GTR,
    TOK_GEQ,
    TOK_IF,
    TOK_THEN,
    TOK_ELSE,
    TOK_FI,
    TOK_LOOP,
    TOK_END,
    TOK_BREAK,
    TOK_EXISTS,
    TOK_COMMENT,
    TOK_PIPE,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_ASSIGN,
    TOK_VAR,
    TOK_VAR_GLOBAL,
    TOK_REDIR_OUT,
    TOK_REDIR_IN,
    TOK_SEMI,
    TOK_ESCAPE,
    TOK_EOL,
} BATSH_TOKEN_TYPE;

typedef struct {
    const CHAR *keyword;
    BATSH_TOKEN_TYPE type;
} KEYWORD;

static const KEYWORD KEYWORDS[] ATTRIB_RODATA = {
    /* built-in commands */
    { "cd",        TOK_CMD },
    { "cd..",      TOK_CMD },
    { "help",      TOK_CMD },
    { "clear",     TOK_CMD },
    { "cls",       TOK_CMD },
    { "version",   TOK_CMD },
    { "exit",      TOK_CMD },
    { "echo",      TOK_CMD },
    { "tone",      TOK_CMD },
    { "soundoff",  TOK_CMD },
    { "dir",       TOK_CMD },
    { "mkdir",     TOK_CMD },
    { "rmdir",     TOK_CMD },
    { "colour",    TOK_CMD },

    /* Logic & conditionals */
    { "and",    TOK_AND },
    { "or",     TOK_OR },
    { "equ",    TOK_EQU },
    { "neq",    TOK_NEQ },
    { "lss",    TOK_LSS },
    { "leq",    TOK_LEQ },
    { "gtr",    TOK_GTR },
    { "geq",    TOK_GEQ },

    /* Flow control */
    { "if",     TOK_IF },
    { "then",   TOK_THEN },
    { "else",   TOK_ELSE },
    { "fi",     TOK_FI },
    { "loop",   TOK_LOOP },
    { "end",    TOK_END },
    { "break",  TOK_BREAK },
    { "exists", TOK_EXISTS },

    /* Misc */
    { "rem",    TOK_COMMENT },

    /* Single character */
    { "\"", TOK_STRING },
    { "|",  TOK_PIPE },
    { "+",  TOK_PLUS },
    { "-",  TOK_MINUS },
    { "*",  TOK_MUL },
    { "/",  TOK_DIV },
    { "(",  TOK_LPAREN },
    { ")",  TOK_RPAREN },
    { "{",  TOK_LBRACE },
    { "}",  TOK_RBRACE },
    { "=",  TOK_ASSIGN },
    { "@",  TOK_VAR },
    { "@@", TOK_VAR_GLOBAL },
    { ">",  TOK_REDIR_OUT },
    { "<",  TOK_REDIR_IN },
    { ";",  TOK_SEMI },
    { "\\", TOK_ESCAPE },
    { "#",  TOK_COMMENT },
};
#define KEYWORDS_COUNT (sizeof(KEYWORDS) / sizeof(KEYWORDS[0]))

static const ShellCommand shell_commands[] ATTRIB_RODATA = {
    { "help",      CMD_HELP,          "Show this help message" },
    { "clear",     CMD_CLEAR,         "Clear the screen" },
    { "cls",       CMD_CLEAR,         "Clear the screen" },
    { "version",   CMD_VERSION,       "Show shell version" },
    { "exit",      CMD_EXIT,          "Exit the shell" },
    { "echo",      CMD_ECHO,          "Echo text back to screen" },
    { "tone",      CMD_TONE,          "Play tone: tone <freqHz> <ms> [amp] [rate]" },
    { "soundoff",  CMD_SOUNDOFF,      "Stop AC97 playback" },
    { "cd",        CMD_CD,            "Change directory" },
    { "cd..",      CMD_CD_BACKWARDS,  "Change directory backwards" },
    { "dir",       CMD_DIR,           "List directory contents" },
    { "mkdir",     CMD_NONE,         "Create a directory, -h for help" },
    { "rmdir",     CMD_NONE,         "Remove a directory, -h for help" },
    { "type",      CMD_TYPE,          "Print file contents" },
    { "tail",      CMD_TAIL,          "Print tail of file contents" },
    { "colour",    CMD_COLOUR,        "Change console colour: colour <fg> <bg> [-h] [-q]" },
    { "shell",     CMD_SHELL,         "Starts a new shell process" },
    { "restart",   CMD_RESTART,       "Restarts the machine" },
    { "shutdown",  CMD_SHUTDOWN,      "Shuts down the machine" },
    { "time",      CMD_NONE,          "Print system time. -h for help" },
    { "pinfo",     CMD_NONE,          "Print process information. -h for help" },
    { "clock",     CMD_NONE,          "Display graphical clock" },
    { "shuttle",   CMD_NONE,          "Serial port communication. -h for help" },
    { "kill",      CMD_NONE,          "Kills a process with the given pid. -h for help"},
    { "cp",        CMD_NONE,          "Copies a file or directory. -h for help"},
    { "mv",        CMD_NONE,          "Moves a file or directory. -h for help"},
    { "touch",     CMD_NONE,          "Creates a file. -h for help"},
    { "del",       CMD_NONE,          "Deletes a file. -h for help"},
};
#define shell_command_count (sizeof(shell_commands) / sizeof(shell_commands[0]))

/* ===================================================
 * Argument parsing
 * =================================================== */

VOID RAW_LINE_TO_ARG_ARRAY(PU8 raw_line, ARG_ARRAY *out) {
    out->argc = 0;
    out->argv = NULLPTR;
    if (!raw_line) return;
    PU8 first_space = STRCHR(raw_line, ' ');

    out->argv = MAlloc(sizeof(PU8*) * MAX_ARGS);
    out->argv[out->argc++] = STRDUP(raw_line);

    PU8 args_start = first_space ? first_space + 1 : NULL;
    if (args_start && *args_start) {
        PU8 p = args_start;
        while (*p && out->argc < MAX_ARGS) {
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;

            BOOL in_quotes = FALSE;
            PU8 ArgBegin;
            if (*p == '"') {
                in_quotes = TRUE;
                p++;
                ArgBegin = p;
                while (*p && *p != '"') p++;
            } else {
                ArgBegin = p;
                while (*p && *p != ' ' && *p != '\t') p++;
            }

            U32 len = p - ArgBegin;
            PU8 arg = MAlloc(len + 1);
            STRNCPY(arg, ArgBegin, len);
            arg[len] = 0;
            out->argv[out->argc++] = arg;

            if (in_quotes && *p == '"') p++;
            while (*p == ' ' || *p == '\t') p++;
        }
    }
}

VOID DELETE_ARG_ARRAY(ARG_ARRAY *arr) {
    if (!arr) return;
    for (U32 i = 0; i < arr->argc; i++) {
        if (arr->argv[i]) MFree(arr->argv[i]);
        arr->argv[i] = NULLPTR;
    }
    if (arr->argv) MFree(arr->argv);
    arr->argv = NULLPTR;
    arr->argc = 0;
}

VOID BATSH_SET_MODE(U8 mode) { batsh_mode = mode; }
U8 BATSH_GET_MODE(VOID) { return batsh_mode; }

/* ===================================================
 * Variable system
 * =================================================== */

static S32 FIND_VAR(PU8 name) {
    if (!name) return -1;
    for (U32 i = 0; i < shell_var_count; i++) {
        if (STRICMP(name, shell_vars[i].name) == 0) return i;
    }
    U32 name_len = STRLEN(name);

    PU8 tmp1 = MAlloc(name_len + 2);
    if (!tmp1) return -1;
    tmp1[0] = '@';
    MEMCPY(tmp1 + 1, name, name_len);
    tmp1[name_len + 1] = '\0';
    for (U32 i = 0; i < shell_var_count; i++) {
        if (STRICMP(tmp1, shell_vars[i].name) == 0) {
            MFree(tmp1);
            return i;
        }
    }

    PU8 tmp2 = MAlloc(name_len + 4);
    if (!tmp2) { MFree(tmp1); return -1; }
    tmp2[0] = '@'; tmp2[1] = '{';
    MEMCPY(tmp2 + 2, name, name_len);
    tmp2[name_len + 2] = '}';
    tmp2[name_len + 3] = '\0';
    for (U32 i = 0; i < shell_var_count; i++) {
        if (STRICMP(tmp2, shell_vars[i].name) == 0) {
            MFree(tmp1); MFree(tmp2);
            return i;
        }
    }
    MFree(tmp1); MFree(tmp2);
    return -1;
}

VOID SET_VAR(PU8 name, PU8 value) {
    S32 idx = FIND_VAR(name);
    if (shell_var_count + 1 > MAX_VARS) return;
    if (idx >= 0 && idx != -1) {
        STRNCPY(shell_vars[idx].value, value, MAX_VAR_VALUE - 1);
    } else if (shell_var_count < MAX_VARS) {
        STRNCPY(shell_vars[shell_var_count].name, name, MAX_VAR_NAME - 1);
        STRNCPY(shell_vars[shell_var_count].value, value, MAX_VAR_VALUE - 1);
        shell_var_count++;
    }
}

PU8 GET_VAR(PU8 name) {
    S32 idx = FIND_VAR(name);
    return (idx >= 0 && idx != -1) ? shell_vars[idx].value : (PU8)NULLPTR;
}

static S32 FIND_INST_VAR(PU8 name, BATSH_INSTANCE *inst) {
    if (!name || !inst) return -1;
    for (U32 i = 0; i < inst->shell_var_count; i++) {
        if (STRICMP(name, inst->shell_vars[i].name) == 0) return i;
    }
    U32 name_len = STRLEN(name);

    PU8 tmp1 = MAlloc(name_len + 2);
    if (!tmp1) return -1;
    tmp1[0] = '@';
    MEMCPY(tmp1 + 1, name, name_len);
    tmp1[name_len + 1] = '\0';
    for (U32 i = 0; i < inst->shell_var_count; i++) {
        if (STRICMP(tmp1, inst->shell_vars[i].name) == 0) {
            MFree(tmp1);
            return i;
        }
    }

    PU8 tmp2 = MAlloc(name_len + 4);
    if (!tmp2) { MFree(tmp1); return -1; }
    tmp2[0] = '@'; tmp2[1] = '{';
    MEMCPY(tmp2 + 2, name, name_len);
    tmp2[name_len + 2] = '}';
    tmp2[name_len + 3] = '\0';
    for (U32 i = 0; i < inst->shell_var_count; i++) {
        if (STRICMP(tmp2, inst->shell_vars[i].name) == 0) {
            MFree(tmp1); MFree(tmp2);
            return i;
        }
    }
    MFree(tmp1); MFree(tmp2);
    return -1;
}

BOOLEAN SET_INST_VAR(PU8 name, PU8 value, BATSH_INSTANCE *inst) {
    if (!inst) return FALSE;
    if (inst->shell_var_count + 1 > MAX_VARS) return FALSE;
    S32 idx = FIND_INST_VAR(name, inst);
    if (idx >= 0 && idx != -1) {
        STRNCPY(inst->shell_vars[idx].value, value, MAX_VAR_VALUE - 1);
    } else if (inst->shell_var_count < MAX_VARS) {
        STRNCPY(inst->shell_vars[inst->shell_var_count].name, name, MAX_VAR_NAME - 1);
        STRNCPY(inst->shell_vars[inst->shell_var_count].value, value, MAX_VAR_VALUE - 1);
        inst->shell_var_count++;
    }
    return TRUE;
}

PU8 GET_INST_VAR(PU8 name, BATSH_INSTANCE *inst) {
    if (!inst) return NULL;
    S32 idx = FIND_INST_VAR(name, inst);
    return (idx >= 0 && idx != -1) ? inst->shell_vars[idx].value : NULLPTR;
}

PU8 GET_INST_ARG(U32 index, BATSH_INSTANCE *inst) {
    if (index > inst->arg_count) return NULLPTR;
    return inst->args[index];
}

KEYWORD *GET_KEYWORD_S(PU8 keyword) {
    for (U32 i = 0; i < KEYWORDS_COUNT; i++) {
        if (STRICMP(keyword, KEYWORDS[i].keyword) == 0) return (KEYWORD *)&KEYWORDS[i];
    }
    return NULLPTR;
}

KEYWORD *GET_KEYWORD_C(CHAR c) {
    U8 buf[2] = { c, '\0' };
    for (U32 i = 0; i < KEYWORDS_COUNT; i++) {
        if (STRICMP(buf, KEYWORDS[i].keyword) == 0) return (KEYWORD *)&KEYWORDS[i];
    }
    return NULLPTR;
}

/* ===================================================
 * AST types
 * =================================================== */

struct pos { U32 line; U32 row; };

typedef struct {
    BATSH_TOKEN_TYPE type;
    char text[CUR_LINE_MAX_LENGTH];
    struct pos;
} BATSH_TOKEN;

typedef enum {
    CMD_SIMPLE,
    CMD_ASSIGN,
    CMD_GLOB_ASSIGN,
    CMD_IF,
    CMD_LOOP,
    CMD_VAR,
} CMD_TOKEN_TYPE;

typedef struct BATSH_COMMAND {
    CMD_TOKEN_TYPE type;
    char **argv;
    int argc;

    /* For compound commands */
    struct BATSH_COMMAND *left;   /* then-branch or loop body */
    struct BATSH_COMMAND *right;  /* else-branch              */
    struct BATSH_COMMAND *next;   /* sequential execution     */

    /* Condition storage for if/loop */
    char *cond_left;
    char *cond_right;
    BATSH_TOKEN_TYPE cond_op;

    struct pos;
} BATSH_COMMAND, *PBATSH_CMD;

/* Forward declarations */
static BOOLEAN execute_master(BATSH_COMMAND *master, BATSH_INSTANCE *inst);
static BOOL resolve_vars(PU8 *line, BATSH_INSTANCE *inst);

/* ===================================================
 * Token helpers
 * =================================================== */

BOOLEAN ADD_TOKEN(BATSH_TOKEN ***tokens, U32 *token_len, PU8 buf, KEYWORD *kw) {
    if (!tokens || !token_len || !buf || !kw) return FALSE;
    BATSH_TOKEN **tmp = ReAlloc(*tokens, sizeof(BATSH_TOKEN*) * (*token_len + 1));
    if (!tmp) return FALSE;
    *tokens = tmp;

    BATSH_TOKEN *tok = MAlloc(sizeof(BATSH_TOKEN));
    if (!tok) return FALSE;

    STRNCPY(tok->text, buf, CUR_LINE_MAX_LENGTH);
    tok->text[CUR_LINE_MAX_LENGTH - 1] = '\0';
    tok->type = kw->type;

    (*tokens)[*token_len] = tok;
    (*token_len)++;
    return TRUE;
}

VOID PRINT_TOKENIZER_ERR(PU8 buf, KEYWORD *kw, struct pos pos) {
    PUTS("Tokenizer error at line ");
    PUT_DEC(pos.row);
    PUTS(", col ");
    PUT_DEC(pos.line);
    PUTS(": ");
    if (buf) PUTS(buf);
    PRINTNEWLINE();
}

static BOOL finalize_token(U8 *buf, U32 *ptr, BATSH_TOKEN ***tokens, U32 *token_len, struct pos pos) {
    if (*ptr == 0) return TRUE;
    buf[*ptr] = '\0';
    BOOLEAN matched = FALSE;
    for (U32 j = 0; j < KEYWORDS_COUNT; j++) {
        if (STRICMP(buf, KEYWORDS[j].keyword) == 0) {
            if (!ADD_TOKEN(tokens, token_len, buf, (KEYWORD *)&KEYWORDS[j])) return FALSE;
            matched = TRUE;
            break;
        }
    }
    if (!matched) {
        KEYWORD throwaway = { "", TOK_WORD };
        if (!ADD_TOKEN(tokens, token_len, buf, &throwaway)) return FALSE;
    }
    *ptr = 0;
    return TRUE;
}

/* ===================================================
 * Parsers
 * =================================================== */

BATSH_COMMAND *parse_word_or_cmd(BATSH_TOKEN **tokens, U32 len, U32 *i, BATSH_INSTANCE *inst) {
    (void)inst;
    PBATSH_CMD cmd_tkn = MAlloc(sizeof(BATSH_COMMAND));
    if (!cmd_tkn) return NULLPTR;
    MEMSET(cmd_tkn, 0, sizeof(BATSH_COMMAND));
    cmd_tkn->type = CMD_SIMPLE;
    cmd_tkn->next = NULL;

    BOOL end_next = FALSE;
    U32 escapes = 0;
    BOOL append_next = 0;
    BATSH_TOKEN *tkn = NULLPTR;

    for (; *i < len; (*i)++) {
        tkn = tokens[*i];
        append_next--;
        switch (tkn->type) {
            case TOK_EOL:
                if (escapes > 0) { escapes--; continue; }
                end_next = TRUE;
                break;
            case TOK_SEMI:
                end_next = TRUE;
                break;
            case TOK_ESCAPE:
                escapes++;
                continue;
            case TOK_DIV:
                append_next = 2;
                /* fall through */
            case TOK_PIPE:
            case TOK_PLUS:
            case TOK_MINUS:
            case TOK_MUL:
            case TOK_LPAREN:
            case TOK_RPAREN:
            case TOK_LBRACE:
            case TOK_RBRACE:
            case TOK_ASSIGN:
            case TOK_REDIR_OUT:
            case TOK_REDIR_IN:
            case TOK_CMD:
            case TOK_WORD:
            case TOK_STRING:
            case TOK_VAR:
            case TOK_VAR_GLOBAL:
                if (append_next == 1) {
                    cmd_tkn->argv[cmd_tkn->argc - 1] = STRAPPEND(cmd_tkn->argv[cmd_tkn->argc - 1], tkn->text);
                } else {
                    cmd_tkn->argv = ReAlloc(cmd_tkn->argv, sizeof(char*) * (cmd_tkn->argc + 1));
                    cmd_tkn->argv[cmd_tkn->argc++] = STRDUP(tkn->text);
                }
                break;
            default:
                if (tkn->type == TOK_IF || tkn->type == TOK_LOOP ||
                    tkn->type == TOK_THEN || tkn->type == TOK_ELSE ||
                    tkn->type == TOK_FI || tkn->type == TOK_END ||
                    tkn->type == TOK_BREAK) {
                    (*i)--;
                    end_next = TRUE;
                }
                break;
        }
        if (end_next) break;
    }

    if (tkn && tkn->type == TOK_DIV) {
        cmd_tkn->argv[cmd_tkn->argc - 1] = STRAPPEND(cmd_tkn->argv[cmd_tkn->argc - 1], tkn->text);
    }

    return cmd_tkn;
}

BATSH_COMMAND *parse_var_assignment(BATSH_TOKEN **tokens, U32 len, U32 *i, BATSH_INSTANCE *inst) {
    (void)inst;
    PBATSH_CMD var_tkn = MAlloc(sizeof(BATSH_COMMAND));
    if (!var_tkn) return NULLPTR;
    MEMSET(var_tkn, 0, sizeof(BATSH_COMMAND));
    var_tkn->type = CMD_SIMPLE;
    var_tkn->next = NULL;

    BOOL end_next = FALSE;
    U32 escapes = 0;
    BOOL has_assign = FALSE;
    U32 assign_track = 0;

    if (*i >= len ||
        !(tokens[*i]->type == TOK_VAR || tokens[*i]->type == TOK_VAR_GLOBAL))
        return var_tkn;

    U32 original_type = tokens[*i]->type;
    for (; *i < len; (*i)++) {
        BATSH_TOKEN *tkn = tokens[*i];
        assign_track--;
        switch (tkn->type) {
            case TOK_ESCAPE: escapes++; continue;
            case TOK_EOL:
                if (escapes > 0) { escapes--; continue; }
                end_next = TRUE;
                break;
            case TOK_SEMI: end_next = TRUE; break;
            case TOK_ASSIGN: assign_track = 2; continue;
            case TOK_STRING:
            case TOK_WORD:
            case TOK_VAR:
            case TOK_VAR_GLOBAL:
                if (assign_track == 1) has_assign = TRUE;
                var_tkn->argv = ReAlloc(var_tkn->argv, sizeof(char*) * (var_tkn->argc + 1));
                var_tkn->argv[var_tkn->argc++] = STRDUP(tkn->text);
                break;
            default:
                if (tkn->type == TOK_IF || tkn->type == TOK_LOOP ||
                    tkn->type == TOK_THEN || tkn->type == TOK_ELSE ||
                    tkn->type == TOK_FI || tkn->type == TOK_END) {
                    (*i)--;
                    end_next = TRUE;
                }
                break;
        }
        if (end_next) break;
    }

    if (has_assign) {
        if (original_type == TOK_VAR_GLOBAL) var_tkn->type = CMD_GLOB_ASSIGN;
        else var_tkn->type = CMD_ASSIGN;
    } else {
        var_tkn->type = CMD_SIMPLE;
    }

    return var_tkn;
}

/* ===================================================
 * IF / THEN / ELSE / FI parser
 *
 * Syntax: if LHS OP RHS then BODY [else BODY] fi
 * Where OP is one of: equ neq lss leq gtr geq exists
 * =================================================== */

BATSH_COMMAND *parse_if(BATSH_TOKEN **tokens, U32 len, U32 *i, BATSH_INSTANCE *inst) {
    PBATSH_CMD cmd = MAlloc(sizeof(BATSH_COMMAND));
    if (!cmd) return NULLPTR;
    MEMSET(cmd, 0, sizeof(BATSH_COMMAND));
    cmd->type = CMD_IF;

    /* Skip 'if' token */
    (*i)++;

    /* Parse condition: LHS OP RHS */
    if (*i < len) {
        cmd->cond_left = STRDUP(tokens[*i]->text);
        (*i)++;
    }
    if (*i < len) {
        cmd->cond_op = tokens[*i]->type;
        (*i)++;
    }
    if (*i < len && cmd->cond_op != TOK_EXISTS) {
        cmd->cond_right = STRDUP(tokens[*i]->text);
        (*i)++;
    }

    /* Skip 'then' */
    if (*i < len && tokens[*i]->type == TOK_THEN)
        (*i)++;

    /* Parse then-body: collect commands until 'else' or 'fi' */
    BATSH_COMMAND *then_head = NULL, *then_tail = NULL;
    while (*i < len) {
        if (tokens[*i]->type == TOK_ELSE || tokens[*i]->type == TOK_FI)
            break;
        if (tokens[*i]->type == TOK_EOL || tokens[*i]->type == TOK_SEMI) {
            (*i)++;
            continue;
        }

        BATSH_COMMAND *sub = NULL;
        switch (tokens[*i]->type) {
            case TOK_IF:   sub = parse_if(tokens, len, i, inst); break;
            case TOK_LOOP: /* handled below */ break;
            case TOK_VAR:
            case TOK_VAR_GLOBAL:
                sub = parse_var_assignment(tokens, len, i, inst); break;
            case TOK_CMD:
            case TOK_WORD:
            default:
                sub = parse_word_or_cmd(tokens, len, i, inst); break;
        }
        if (sub) {
            if (!then_head) then_head = then_tail = sub;
            else { then_tail->next = sub; then_tail = sub; }
        }
    }
    cmd->left = then_head;

    /* Parse optional else-body */
    if (*i < len && tokens[*i]->type == TOK_ELSE) {
        (*i)++; /* skip 'else' */

        BATSH_COMMAND *else_head = NULL, *else_tail = NULL;
        while (*i < len) {
            if (tokens[*i]->type == TOK_FI) break;
            if (tokens[*i]->type == TOK_EOL || tokens[*i]->type == TOK_SEMI) {
                (*i)++;
                continue;
            }

            BATSH_COMMAND *sub = NULL;
            switch (tokens[*i]->type) {
                case TOK_IF:   sub = parse_if(tokens, len, i, inst); break;
                case TOK_VAR:
                case TOK_VAR_GLOBAL:
                    sub = parse_var_assignment(tokens, len, i, inst); break;
                case TOK_CMD:
                case TOK_WORD:
                default:
                    sub = parse_word_or_cmd(tokens, len, i, inst); break;
            }
            if (sub) {
                if (!else_head) else_head = else_tail = sub;
                else { else_tail->next = sub; else_tail = sub; }
            }
        }
        cmd->right = else_head;
    }

    /* Skip 'fi' */
    if (*i < len && tokens[*i]->type == TOK_FI)
        (*i)++;

    return cmd;
}

/* ===================================================
 * LOOP / END parser
 *
 * Syntax: loop LHS OP RHS
 *         BODY
 *         end
 * =================================================== */

BATSH_COMMAND *parse_loop(BATSH_TOKEN **tokens, U32 len, U32 *i, BATSH_INSTANCE *inst) {
    PBATSH_CMD cmd = MAlloc(sizeof(BATSH_COMMAND));
    if (!cmd) return NULLPTR;
    MEMSET(cmd, 0, sizeof(BATSH_COMMAND));
    cmd->type = CMD_LOOP;

    /* Skip 'loop' token */
    (*i)++;

    /* Parse condition: LHS OP RHS */
    if (*i < len) {
        cmd->cond_left = STRDUP(tokens[*i]->text);
        (*i)++;
    }
    if (*i < len) {
        cmd->cond_op = tokens[*i]->type;
        (*i)++;
    }
    if (*i < len) {
        cmd->cond_right = STRDUP(tokens[*i]->text);
        (*i)++;
    }

    /* Skip EOL after condition */
    while (*i < len && (tokens[*i]->type == TOK_EOL || tokens[*i]->type == TOK_SEMI))
        (*i)++;

    /* Parse loop body: collect commands until 'end' */
    BATSH_COMMAND *body_head = NULL, *body_tail = NULL;
    while (*i < len) {
        if (tokens[*i]->type == TOK_END) break;
        if (tokens[*i]->type == TOK_EOL || tokens[*i]->type == TOK_SEMI) {
            (*i)++;
            continue;
        }

        BATSH_COMMAND *sub = NULL;
        switch (tokens[*i]->type) {
            case TOK_IF:   sub = parse_if(tokens, len, i, inst); break;
            case TOK_LOOP: sub = parse_loop(tokens, len, i, inst); break;
            case TOK_BREAK:
                (*i)++;
                /* Create a break sentinel */
                sub = MAlloc(sizeof(BATSH_COMMAND));
                MEMSET(sub, 0, sizeof(BATSH_COMMAND));
                sub->type = CMD_VAR; /* we use CMD_VAR as a break sentinel */
                sub->argc = -1;     /* magic marker for break */
                break;
            case TOK_VAR:
            case TOK_VAR_GLOBAL:
                sub = parse_var_assignment(tokens, len, i, inst); break;
            case TOK_CMD:
            case TOK_WORD:
            default:
                sub = parse_word_or_cmd(tokens, len, i, inst); break;
        }
        if (sub) {
            if (!body_head) body_head = body_tail = sub;
            else { body_tail->next = sub; body_tail = sub; }
        }
    }
    cmd->left = body_head;

    /* Skip 'end' */
    if (*i < len && tokens[*i]->type == TOK_END)
        (*i)++;

    return cmd;
}

/* ===================================================
 * Master parser
 * =================================================== */

BATSH_COMMAND *parse_master(BATSH_TOKEN **tokens, U32 len, BATSH_INSTANCE *inst) {
    BATSH_COMMAND *head = NULL, *tail = NULL;

    for (U32 i = 0; i < len;) {
        BATSH_TOKEN *tkn = tokens[i];
        BATSH_COMMAND *cmd = NULL;

        switch (tkn->type) {
            case TOK_WORD:
            case TOK_CMD:
                cmd = parse_word_or_cmd(tokens, len, &i, inst);
                break;
            case TOK_IF:
                cmd = parse_if(tokens, len, &i, inst);
                break;
            case TOK_LOOP:
                cmd = parse_loop(tokens, len, &i, inst);
                break;
            case TOK_VAR_GLOBAL:
            case TOK_VAR:
                cmd = parse_var_assignment(tokens, len, &i, inst);
                break;
            case TOK_SEMI:
            case TOK_EOL:
                i++;
                continue;
            default:
                i++;
                continue;
        }

        if (!cmd) { i++; continue; }
        if (!head) head = tail = cmd;
        else { tail->next = cmd; tail = cmd; }
    }

    return head;
}

/* ===================================================
 * AST cleanup
 * =================================================== */

void free_batsh_command(BATSH_COMMAND *cmd) {
    if (!cmd) return;
    if (cmd->argv) {
        for (int i = 0; i < cmd->argc; i++) {
            if (cmd->argv[i]) MFree(cmd->argv[i]);
        }
        MFree(cmd->argv);
    }
    if (cmd->cond_left) MFree(cmd->cond_left);
    if (cmd->cond_right) MFree(cmd->cond_right);
    free_batsh_command(cmd->left);
    free_batsh_command(cmd->right);
    free_batsh_command(cmd->next);
    MFree(cmd);
}

/* ===================================================
 * Variable resolution (BUG FIXED)
 * =================================================== */

static BOOL resolve_vars(PU8 *line, BATSH_INSTANCE *inst) {
    if (!line || !*line) return TRUE;

    PU8 tmp = STRDUP(*line);
    if (!tmp) return FALSE;

    PU8 out = MAlloc(STRLEN(tmp) * 2 + 1);
    if (!out) { MFree(tmp); return FALSE; }
    out[0] = '\0';

    U32 i = 0;
    while (tmp[i]) {
        if (tmp[i] == '@' && tmp[i + 1] == '{') {
            i += 2;
            U32 start = i;
            while (tmp[i] && tmp[i] != '}') i++;
            U32 len = i - start;
            if (tmp[i] == '}') i++;

            if (len > 0) {
                U8 var_name[128] = {0};
                STRNCPY(var_name, tmp + start, len);
                var_name[len] = '\0';

                PU8 value = NULL;

                /* BUG FIX: was IS_DIGIT_STR(value) — value is NULL here!
                 * Correct: check var_name for numeric arg indices */
                if (IS_DIGIT_STR(var_name) && inst) {
                    U32 arg_index = ATOI(var_name);
                    value = GET_INST_ARG(arg_index, inst);
                }

                S32 idx = 0;
                if (!value || !*value) {
                    idx = FIND_VAR(var_name);
                    if (idx >= 0) value = GET_VAR(var_name);
                }

                if (!value || !*value) {
                    idx = FIND_INST_VAR(var_name, inst);
                    if (idx >= 0) value = GET_INST_VAR(var_name, inst);
                }

                if (!value) value = (PU8)"";
                out = STRAPPEND(out, value);
            }
        } else {
            U8 ch[2] = { tmp[i], 0 };
            out = STRAPPEND(out, ch);
            i++;
        }
    }

    MFree(*line);
    *line = out;
    MFree(tmp);
    return TRUE;
}

/* ===================================================
 * Condition evaluation (for if/loop)
 * =================================================== */

static BOOL eval_condition(PU8 lhs_raw, BATSH_TOKEN_TYPE op, PU8 rhs_raw,
                           BATSH_INSTANCE *inst) {
    /* Resolve variables in both sides */
    PU8 lhs = STRDUP(lhs_raw ? lhs_raw : (PU8)"");
    PU8 rhs = STRDUP(rhs_raw ? rhs_raw : (PU8)"");
    resolve_vars(&lhs, inst);
    resolve_vars(&rhs, inst);

    BOOL result = FALSE;

    switch (op) {
    case TOK_EQU:
        result = (STRICMP(lhs, rhs) == 0);
        break;
    case TOK_NEQ:
        result = (STRICMP(lhs, rhs) != 0);
        break;
    case TOK_LSS: {
        I32 l = ATOI_I32(lhs), r = ATOI_I32(rhs);
        result = (l < r);
        break;
    }
    case TOK_LEQ: {
        I32 l = ATOI_I32(lhs), r = ATOI_I32(rhs);
        result = (l <= r);
        break;
    }
    case TOK_GTR: {
        I32 l = ATOI_I32(lhs), r = ATOI_I32(rhs);
        result = (l > r);
        break;
    }
    case TOK_GEQ: {
        I32 l = ATOI_I32(lhs), r = ATOI_I32(rhs);
        result = (l >= r);
        break;
    }
    case TOK_EXISTS: {
        /* Check if file/directory exists */
        FAT_LFN_ENTRY ent;
        U8 abs[512];
        if (ResolvePath(lhs, abs, sizeof(abs)) && FAT32_PATH_RESOLVE_ENTRY(abs, &ent))
            result = TRUE;
        else
            result = FALSE;
        break;
    }
    default:
        /* Try numeric comparison for unknown ops */
        result = (STRICMP(lhs, rhs) == 0);
        break;
    }

    MFree(lhs);
    MFree(rhs);
    return result;
}

/* ===================================================
 * Execution engine
 * =================================================== */

static BOOLEAN execute_master(BATSH_COMMAND *master, BATSH_INSTANCE *inst) {
    BATSH_COMMAND *cmd = master;

    while (cmd != NULLPTR) {
        switch (cmd->type) {
        case CMD_SIMPLE: {
            PU8 line_as_is = NULLPTR;
            for (U32 i = 0; i < (U32)cmd->argc; i++) {
                line_as_is = STRAPPEND(line_as_is, cmd->argv[i]);
            }
            if (!line_as_is) break;
            resolve_vars(&line_as_is, inst);
            HANDLE_COMMAND(line_as_is);
            MFree(line_as_is);
        } break;

        case CMD_GLOB_ASSIGN: {
            if (cmd->argc < 2) break;
            PU8 name = cmd->argv[0];
            PU8 value = NULL;
            for (U32 i = 1; i < (U32)cmd->argc; i++)
                value = STRAPPEND(value, cmd->argv[i]);
            resolve_vars(&value, inst);
            S32 idx = FIND_VAR(name);
            if (idx >= 0) SET_VAR(name, value);
            MFree(value);
        } break;

        case CMD_ASSIGN: {
            if (cmd->argc < 2) break;
            PU8 name = cmd->argv[0];
            PU8 value = NULL;
            for (U32 i = 1; i < (U32)cmd->argc; i++)
                value = STRAPPEND(value, cmd->argv[i]);
            resolve_vars(&value, inst);
            if (inst) {
                S32 idx = FIND_VAR(name);
                if (idx >= 0) SET_VAR(name, value);
                else SET_INST_VAR(name, value, inst);
            } else {
                S32 idx = FIND_VAR(name);
                if (idx >= 0) SET_VAR(name, value);
            }
            MFree(value);
        } break;

        case CMD_IF: {
            BOOL cond = eval_condition(cmd->cond_left, cmd->cond_op,
                                       cmd->cond_right, inst);
            if (cond) {
                if (cmd->left) execute_master(cmd->left, inst);
            } else {
                if (cmd->right) execute_master(cmd->right, inst);
            }
        } break;

        case CMD_LOOP: {
            /* Loop while condition is true */
            U32 max_iters = 10000; /* safety limit */
            U32 iters = 0;
            while (iters++ < max_iters) {
                BOOL cond = eval_condition(cmd->cond_left, cmd->cond_op,
                                           cmd->cond_right, inst);
                if (!cond) break;

                /* Execute body — check for break sentinel */
                BATSH_COMMAND *body = cmd->left;
                BOOL did_break = FALSE;
                while (body) {
                    if (body->type == CMD_VAR && body->argc == -1) {
                        did_break = TRUE;
                        break;
                    }
                    /* Execute single command */
                    BATSH_COMMAND saved_next = *body;
                    body->next = NULL;
                    execute_master(body, inst);
                    body->next = saved_next.next;
                    body = body->next;
                }
                if (did_break) break;
            }
        } break;

        default:
            break;
        }

        cmd = cmd->next;
    }

    return TRUE;
}

/* ===================================================
 * BATSH Parser / Lexer
 * =================================================== */

BOOLEAN PARSE_BATSH_INPUT(PU8 input, BATSH_INSTANCE *inst) {
    if (!input || !*input) return TRUE;
    U32 len = STRLEN(input);
    if (len == 0) return TRUE;

    U8 buf[CUR_LINE_MAX_LENGTH] = {0};
    U32 ptr = 0;
    BATSH_TOKEN **tokens = NULL;
    U32 token_len = 0;
    struct pos pos = { .line = 0, .row = 0 };
    KEYWORD *rem = GET_KEYWORD_S("rem");
    CHAR c = 0;
    BOOL back_to_singles = FALSE;
    BOOL relexed = FALSE;

    for (U32 i = 0; i < len; i++) {
        c = input[i];
        relex:
        if (c == '\n' || c == '\r') { pos.row++; pos.line = 0; }
        else { pos.line++; }

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            cmp_words:
            if (ptr > 0) {
                if (!back_to_singles) buf[ptr++] = c;
                buf[ptr] = '\0';
                if (STRICMP(buf, rem->keyword) == 0) {
                    for (; i < len; i++) {
                        if (input[i] == '\n' || input[i] == '\r') break;
                    }
                    ptr = 0;
                    if (back_to_singles) goto back_to_single_parse;
                    continue;
                }
                BOOLEAN matched = FALSE;
                for (U32 j = 0; j < KEYWORDS_COUNT; j++) {
                    if (STRICMP(buf, KEYWORDS[j].keyword) == 0) {
                        if (!ADD_TOKEN(&tokens, &token_len, buf, (KEYWORD *)&KEYWORDS[j])) goto error;
                        matched = TRUE;
                        break;
                    }
                }
                if (!matched) {
                    KEYWORD throwaway = { "", TOK_WORD };
                    if (!ADD_TOKEN(&tokens, &token_len, buf, &throwaway)) goto error;
                }
                ptr = 0;
            }
            if (c == '\n' || c == '\r') {
                KEYWORD kw = { "~", TOK_EOL };
                if (!ADD_TOKEN(&tokens, &token_len, kw.keyword, &kw)) goto error;
            }
            if (back_to_singles) goto back_to_single_parse;
            continue;
        }
        else if (c == '"') {
            i++;
            U32 start = i;
            while (i < len) {
                if (input[i] == '"' && input[i - 1] != '\\') { start--; i++; break; }
                i++;
            }
            U32 str_len = i - start;
            if (str_len >= CUR_LINE_MAX_LENGTH) str_len = CUR_LINE_MAX_LENGTH - 1;
            MEMCPY(buf, &input[start], str_len);
            buf[str_len] = '\0';

            KEYWORD str_kw = { "\"", TOK_STRING };
            if (!ADD_TOKEN(&tokens, &token_len, buf, &str_kw)) goto error;
            ptr = 0;
            continue;
        }
        else if (c == '#') {
            for (; i < len; i++) {
                if (input[i] == '\n' || input[i] == '\r') break;
            }
            KEYWORD str_kw = { "~", TOK_EOL };
            if (!ADD_TOKEN(&tokens, &token_len, buf, &str_kw)) goto error;
            ptr = 0;
            continue;
        }
        else if (c == '@') {
            BOOL is_global = FALSE;
            if (i + 1 < len && input[i + 1] == '@') { is_global = TRUE; i++; }
            i++;
            U32 start = i;
            BOOL was_bracket = FALSE;

            if (input[i] == '{') {
                i++;
                start = i;
                was_bracket = TRUE;
                while (i < len && input[i] != '}') i++;
            } else {
                while (i < len && ISALNUM(input[i])) i++;
            }

            U32 var_len = i - start + (was_bracket ? 3 : 1);
            if (var_len >= CUR_LINE_MAX_LENGTH) var_len = CUR_LINE_MAX_LENGTH - 1;
            MEMCPY(buf, &input[start - (was_bracket ? 2 : (is_global ? 3 : 1))], var_len);
            buf[var_len] = '\0';
            KEYWORD var_kw = { buf, is_global ? TOK_VAR_GLOBAL : TOK_VAR };

            if (!ADD_TOKEN(&tokens, &token_len, buf, &var_kw)) goto error;

            if (input[i] == '}') i++;
            if (STRCHR("|+-*/(){}=@><\\\n\t\r ", input[i])) i--;
            continue;
        }
        else if (c == ';') {
            if (ptr > 0) {
                KEYWORD kw = { ";", TOK_SEMI };
                if (!ADD_TOKEN(&tokens, &token_len, kw.keyword, &kw)) goto error;
                goto cmp_words;
            }
        }
        else if (c == '|' || c == '+' || c == '-' || c == '*' ||
                 c == '(' || c == ')' || c == '}' || c == '/' ||
                 c == '{' || c == '=' || c == '@' || c == '>' ||
                 c == '<' || c == '\\')
        {
            if (ptr > 0) { back_to_singles = TRUE; goto cmp_words; }

            back_to_single_parse:
            back_to_singles = FALSE;
            ptr = 0;
            buf[ptr] = c;
            buf[++ptr] = '\0';
            KEYWORD *kw = GET_KEYWORD_C(c);
            if (!kw) { PRINT_TOKENIZER_ERR(buf, NULLPTR, pos); goto error; }
            if (!ADD_TOKEN(&tokens, &token_len, buf, kw)) goto error;
            ptr = 0;
            continue;
        }

        buf[ptr++] = c;
        if (ptr >= CUR_LINE_MAX_LENGTH - 1) break;
    }

    if (relexed) {
        KEYWORD kw22 = { "~", TOK_EOL };
        if (!ADD_TOKEN(&tokens, &token_len, kw22.keyword, &kw22)) goto error;
    }
    if (ptr > 0 && !relexed) {
        c = ' ';
        relexed = TRUE;
        goto relex;
    }

    /* Parse */
    BATSH_COMMAND *root_cmd = parse_master(tokens, token_len, inst);
    if (!root_cmd) {
        PUTS("Parsing failed\n");
        goto error;
    }

    if (!execute_master(root_cmd, inst)) {
        PUTS("Executing failed\n");
        SET_VAR("ERRORLEVEL", "1");
        goto error;
    }
    SET_VAR("ERRORLEVEL", "0");

    U8 res = TRUE;
    goto cleanup;
error:
    res = FALSE;
cleanup:
    for (U32 i = 0; i < token_len; i++) {
        if (tokens[i]) MFree(tokens[i]);
    }
    if (tokens) MFree(tokens);
    if (root_cmd) free_batsh_command(root_cmd);
    return res;
}

/* ===================================================
 * Core Command Handler
 * =================================================== */

VOID HANDLE_COMMAND(U8 *line) {
    if (STRLEN(line) == 0) return;

    U8 command[64];
    U32 i = 0, j = 0;
    while (line[i] && line[i] != ' ' && j < sizeof(command) - 1)
        command[j++] = line[i++];
    command[j] = '\0';

    BOOLEAN found = FALSE;
    for (U32 k = 0; k < shell_command_count; k++) {
        if (STRICMP((CHAR*)command, shell_commands[k].name) == 0) {
            if (shell_commands[k].handler != NULLPTR) {
                shell_commands[k].handler(line);
                found = TRUE;
            }
            break;
        }
    }

    if (!found)
        CMD_UNKNOWN(line);
}

/* ===================================================
 * BATSH File Runner
 * =================================================== */

BOOLEAN RUN_BATSH_FILE(BATSH_INSTANCE *inst) {
    if (!inst) return FALSE;

    inst->echo = TRUE;
    inst->batsh_mode = TRUE;

    while (1) {
        U32 n = FREAD(inst->file, inst->line, inst->line_len - 1);
        if (n == 0) break;
        inst->line[n] = '\0';

        if (!PARSE_BATSH_INPUT(inst->line, inst)) {
            PUTS("BATSH parse error\n");
        }
    }

    return TRUE;
}

/* ===================================================
 * BATSH Script Loader
 * =================================================== */

BOOLEAN RUN_BATSH_SCRIPT(PU8 path, U32 argc, PPU8 argv) {
    PU8 dot = STRRCHR(path, '.');
    if (!dot || STRNICMP(dot, ".SH", 3) != 0) return FALSE;
    BATSH_INSTANCE *inst = CREATE_BATSH_INSTANCE(path, argc, argv);
    if (!inst) return FALSE;
    batsh_mode = TRUE;
    BOOLEAN res = RUN_BATSH_FILE(inst);
    DESTROY_BATSH_INSTANCE(inst);
    batsh_mode = FALSE;
    return res;
}

/* ===================================================
 * BATSH Instance lifecycle
 * =================================================== */

BATSH_INSTANCE *CREATE_BATSH_INSTANCE(PU8 path, U32 argc, PPU8 argv) {
    BATSH_INSTANCE *inst = MAlloc(sizeof(BATSH_INSTANCE));
    if (!inst) return NULLPTR;
    MEMZERO(inst, sizeof(BATSH_INSTANCE));
    inst->file = FOPEN(path, MODE_R | MODE_FAT32);
    if (!inst->file) {
        MFree(inst);
        return NULLPTR;
    }
    inst->line_len = SHELL_SCRIPT_LINE_MAX_LEN;
    inst->batsh_mode = 1;
    inst->child_proc = NULLPTR;
    inst->parent_proc = NULLPTR;
    inst->echo = 1;
    inst->shell_var_count = 0;

    inst->arg_count = argc;
    if (inst->arg_count > MAX_ARGS) inst->arg_count = MAX_ARGS;
    for (U32 i = 0; i < inst->arg_count; i++)
        inst->args[i] = STRDUP(argv[i]);

    inst->shndl = GET_SHNDL();
    inst->status_code = 0;
    return inst;
}

VOID DESTROY_BATSH_INSTANCE(BATSH_INSTANCE *inst) {
    if (!inst) return;
    for (U32 i = 0; i < inst->arg_count; i++)
        MFree(inst->args[i]);
    FCLOSE(inst->file);
    MFree(inst);
}

/* ===================================================
 * CMD file includes (same pattern as original shell)
 * =================================================== */

#define TAIL_LINE_COUNT 10
#include "CMD/MISC.c"
#include "CMD/AC97.c"
#include "CMD/CD.c"
#include "CMD/DIR.c"
#include "CMD/TYPE.c"
#include "CMD/TAIL.c"
#include "CMD/COLOUR.c"

/* ===================================================
 * Setup
 * =================================================== */

VOID SETUP_BATSH_PROCESSER(VOID) {
    SET_VAR("PATH",
        #include <ATOSH.PATH>
    );
    #include <ATOSH.ENV_VARS>
}

/* ===================================================
 * Process runner
 * =================================================== */

BOOLEAN RUN_PROCESS(PU8 line) {
    if (!line || !*line) return FALSE;

    U8 original_line[512];
    STRNCPY(original_line, line, sizeof(original_line) - 1);
    original_line[sizeof(original_line) - 1] = 0;
    str_trim(original_line);
    if (!*original_line) return FALSE;

    PU8 first_space = STRCHR(original_line, ' ');
    U32 prog_len = first_space ? (U32)(first_space - original_line) : STRLEN(original_line);

    PU8 prog_copy = MAlloc(prog_len + 1);
    STRNCPY(prog_copy, original_line, prog_len);
    prog_copy[prog_len] = 0;

    PU8 prog_name = STRDUP(prog_copy);
    PU8 last_slash = STRRCHR(prog_name, '/');
    if (last_slash) {
        PU8 tmp = STRDUP(last_slash + 1);
        MFree(prog_name);
        prog_name = tmp;
    }

    U8 abs_path_buf[512];
    FAT_LFN_ENTRY ent;
    BOOLEAN found = FALSE;
    BOOLEAN is_found_as_bin = FALSE;
    BOOLEAN is_found_as_sh  = FALSE;

    /* Try direct path */
    if (ResolvePath(prog_copy, abs_path_buf, sizeof(abs_path_buf))) {
        if (FAT32_PATH_RESOLVE_ENTRY(abs_path_buf, &ent)) {
            found = TRUE;
        } else {
            U8 candidate[512];
            STRCPY(candidate, abs_path_buf);
            STRNCAT(candidate, ".BIN", sizeof(candidate) - STRLEN(candidate) - 1);
            if (FAT32_PATH_RESOLVE_ENTRY(candidate, &ent)) {
                STRCPY(abs_path_buf, candidate);
                found = TRUE;
                is_found_as_bin = TRUE;
            } else {
                STRCPY(candidate, abs_path_buf);
                STRNCAT(candidate, ".SH", sizeof(candidate) - STRLEN(candidate) - 1);
                if (FAT32_PATH_RESOLVE_ENTRY(candidate, &ent)) {
                    STRCPY(abs_path_buf, candidate);
                    found = TRUE;
                    is_found_as_sh = TRUE;
                }
            }
        }
    }

    /* Search PATH */
    if (!found) {
        PU8 path_val = GET_VAR((PU8)"PATH");
        if (path_val && *path_val) {
            PU8 path_dup = STRDUP(path_val);
            PU8 saveptr = NULL;
            PU8 tok = STRTOK_R(path_dup, ";", &saveptr);
            while (tok && !found) {
                if (*tok) {
                    U8 candidate[512] = {0};
                    STRNCPY(candidate, tok, sizeof(candidate) - 1);
                    U32 clen = STRLEN(candidate);
                    if (clen && candidate[clen - 1] != '/' && candidate[clen - 1] != '\\')
                        STRNCAT(candidate, "/", sizeof(candidate) - STRLEN(candidate) - 1);
                    STRNCAT(candidate, prog_name, sizeof(candidate) - STRLEN(candidate) - 1);

                    if (ResolvePath(candidate, abs_path_buf, sizeof(abs_path_buf))) {
                        if (FAT32_PATH_RESOLVE_ENTRY(abs_path_buf, &ent)) {
                            found = TRUE; break;
                        }
                        U8 try_ext[512];
                        STRCPY(try_ext, abs_path_buf);
                        STRNCAT(try_ext, ".BIN", sizeof(try_ext) - STRLEN(try_ext) - 1);
                        if (FAT32_PATH_RESOLVE_ENTRY(try_ext, &ent)) {
                            STRCPY(abs_path_buf, try_ext);
                            found = TRUE; is_found_as_bin = TRUE; break;
                        }
                        STRCPY(try_ext, abs_path_buf);
                        STRNCAT(try_ext, ".SH", sizeof(try_ext) - STRLEN(try_ext) - 1);
                        if (FAT32_PATH_RESOLVE_ENTRY(try_ext, &ent)) {
                            STRCPY(abs_path_buf, try_ext);
                            found = TRUE; is_found_as_sh = TRUE; break;
                        }
                    }
                }
                tok = STRTOK_R(NULL, ";", &saveptr);
            }
            MFree(path_dup);
        }
    }

    if (!found) {
        PUTS("\nError: Failed to resolve path.\n");
        MFree(prog_name);
        MFree(prog_copy);
        return FALSE;
    }

    MFree(prog_copy);

    U32 file_size = 0;
    VOIDPTR data = FAT32_READ_FILE_CONTENTS(&file_size, &ent.entry);
    if (!data) {
        MFree(prog_name);
        return FALSE;
    }

    /* Parse arguments */
    PU8* argv = MAlloc(sizeof(PU8*) * MAX_ARGS);
    U32 argc = 0;
    argv[argc++] = STRDUP(abs_path_buf);

    PU8 args_start = first_space ? first_space + 1 : NULL;
    if (args_start && *args_start) {
        PU8 p = args_start;
        while (*p && argc < MAX_ARGS) {
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;
            BOOL in_quotes = FALSE;
            PU8 ArgBegin;
            if (*p == '"') { in_quotes = TRUE; p++; ArgBegin = p; while (*p && *p != '"') p++; }
            else { ArgBegin = p; while (*p && *p != ' ' && *p != '\t') p++; }
            U32 al = p - ArgBegin;
            PU8 arg = MAlloc(al + 1);
            STRNCPY(arg, ArgBegin, al);
            arg[al] = 0;
            argv[argc++] = arg;
            if (in_quotes && *p == '"') p++;
            while (*p == ' ' || *p == '\t') p++;
        }
    }

    PU8 ext = STRRCHR(abs_path_buf, '.');
    ext = ext ? (ext + 1) : (PU8)"";
    BOOLEAN result = FALSE;

    if (STRICMP(ext, "BIN") == 0) {
        U32 pid = PROC_GETPID();
        U32 res = START_PROCESS(abs_path_buf, data, file_size,
                                TCB_STATE_ACTIVE, pid, argv, argc);
        if (res) PRINTNEWLINE();
        result = (res != 0);
    }
    else if (STRICMP(ext, "SH") == 0) {
        result = RUN_BATSH_SCRIPT(abs_path_buf, argc, argv);
        for (U32 i = 0; i < argc; i++) { if (argv[i]) MFree(argv[i]); }
        MFree(argv);
    }

    MFree(prog_name);
    return result;
}
