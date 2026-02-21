#ifndef __SHELL__
#define __SHELL__
#endif 
#include <PROC/PROC.h>
#include <PROGRAMS/SHELL/SHELL.h>
#include <PROGRAMS/SHELL/BATSH.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>
#include <STD/AUDIO.h>
#include <STD/MEM.h>
#include <STD/PROC_COM.h>
#include <STD/DEBUG.h>

#define LEND "\r\n"

#define MAX_ARGS 12


// ===================================================
// Internal Shell State
// ===================================================
static ShellVar shell_vars[MAX_VARS] ATTRIB_DATA = {0};
static U32 shell_var_count ATTRIB_DATA = 0;

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
CMD_FUNC(TYPE);
CMD_FUNC(TAIL);
CMD_FUNC(RMDIR);
CMD_FUNC(COLOUR);
#define CMD_NONE NULLPTR




// =====================================
//
// Batsh tokenizer and parser
// Raw commands can be at `Command Implementations`
// Command tables defined below
//
// =====================================

typedef enum {
    TOK_UNKNOWN,

    // Keywords
    TOK_CMD, // built-in commands
    TOK_WORD,
    TOK_STRING,
    TOK_AND,  // AND
    TOK_OR,   // OR
    TOK_EQU, // EQU
    TOK_NEQ, // NEQ
    TOK_LSS, // LSS
    TOK_LEQ, // LEQ
    TOK_GTR, // GTR
    TOK_GEQ, // GEQ
    TOK_IF, // IF
    TOK_THEN, // THEN
    TOK_ELSE, // ELSE
    TOK_FI, // FI
    TOK_LOOP, // LOOP
    TOK_END, // END
    TOK_EXISTS, // EXISTS
    TOK_COMMENT,    // rem, #

    // Other tokens
    TOK_PIPE, // |
    TOK_PLUS, // +
    TOK_MINUS, // -
    TOK_MUL, // *
    TOK_DIV, // /
    TOK_LPAREN, // (
    TOK_RPAREN, // )
    TOK_LBRACE, // {
    TOK_RBRACE, // }
    TOK_ASSIGN, // =
    TOK_VAR, // @
    TOK_VAR_GLOBAL, // @@
    TOK_REDIR_OUT,   // >
    TOK_REDIR_IN,    // <


    /**
     * About TOK_SEMI:
     * 
     * When lexing line dir a;
     * 
     * It is parsed as
     *   dir
     *   ;
     *   a
     * 
     * Semi is placed before the token it affects!
     * 
     */
    TOK_SEMI, // ;
    TOK_ESCAPE, // \ 

    
    TOK_EOL,         // end of line
} BATSH_TOKEN_TYPE;

typedef struct {
    const CHAR *keyword;
    BATSH_TOKEN_TYPE type;
} KEYWORD;

static const KEYWORD KEYWORDS[] ATTRIB_RODATA = {
    // built-in
    { "cd", TOK_CMD },
    { "cd..", TOK_CMD },
    { "help", TOK_CMD },
    { "clear", TOK_CMD },
    { "cls", TOK_CMD },
    { "version", TOK_CMD },
    { "exit", TOK_CMD },
    { "echo", TOK_CMD },
    { "tone", TOK_CMD },
    { "soundoff", TOK_CMD },
    { "dir", TOK_CMD },
    { "mkdir", TOK_CMD },
    { "rmdir", TOK_CMD },
    { "colour", TOK_CMD },

    // Logic & conditionals
    { "and",    TOK_AND },
    { "or",     TOK_OR },
    { "equ",    TOK_EQU },
    { "neq",    TOK_NEQ },
    { "lss",    TOK_LSS },
    { "leq",    TOK_LEQ },
    { "gtr",    TOK_GTR },
    { "geq",    TOK_GEQ },

    // Flow control
    { "if",     TOK_IF },
    { "then",   TOK_THEN },
    { "else",   TOK_ELSE },
    { "fi",     TOK_FI },
    { "loop",   TOK_LOOP },
    { "end",    TOK_END },
    { "exists", TOK_EXISTS },

    // Misc
    { "rem",    TOK_COMMENT },

    // Single character
    { "\"", TOK_STRING },
    { "|", TOK_PIPE },
    { "+", TOK_PLUS },
    { "-", TOK_MINUS },
    { "*", TOK_MUL },
    { "/", TOK_DIV },
    { "(", TOK_LPAREN },
    { ")", TOK_RPAREN },
    { "{", TOK_LBRACE },
    { "}", TOK_RBRACE },
    { "=", TOK_ASSIGN },
    { "@", TOK_VAR },
    { "@@", TOK_VAR_GLOBAL },
    { ">", TOK_REDIR_OUT },
    { "<", TOK_REDIR_IN },
    { ";", TOK_SEMI },
    { "\\", TOK_ESCAPE },
    { "#", TOK_COMMENT },
};
#define KEYWORDS_COUNT (sizeof(KEYWORDS) / sizeof(KEYWORDS[0]))


static const ShellCommand shell_commands[] ATTRIB_RODATA = {
    #include <ATOSH.HELP>
    // Any commands added here must be added 
    //   into the KEYWORDS array above as TOK_CMD in built-ins section!
};

#define shell_command_count (sizeof(shell_commands) / sizeof(shell_commands[0]))



typedef struct _ARG_ARRAY {
    U32 argc;
    PPU8 argv;
} ARG_ARRAY;

VOID RAW_LINE_TO_ARG_ARRAY(PU8 raw_line, ARG_ARRAY *out) {
    out->argc = 0;
    out->argv = NULLPTR;
    if(!raw_line) return;
    PU8 first_space = STRCHR(raw_line, ' ');

    out->argv = MAlloc(sizeof(PU8*) * MAX_ARGS);

    out->argv[out->argc++] = STRDUP(raw_line);  // argv[0] = program path

    // Now parse only arguments:
    PU8 args_start = first_space ? first_space + 1 : NULL;

    if (args_start && *args_start) {
        PU8 p = args_start;

        while (*p && out->argc < MAX_ARGS) {
            // skip leading whitespace
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;

            BOOL in_quotes = FALSE;
            PU8 ArgBegin;

            if (*p == '"') {
                in_quotes = TRUE;
                p++;                 // skip opening quote
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

            if (in_quotes && *p == '"')
                p++; // skip closing quote

            // skip trailing whitespace before next argument
            while (*p == ' ' || *p == '\t') p++;
        }
    }
}

VOID DELETE_ARG_ARRAY(ARG_ARRAY *arr) {
    if(!arr) return;
    for(U32 i = 0; i < arr->argc; i++) {
        if(arr->argv[i]) MFree(arr->argv[i]);
        arr->argv[i] = NULLPTR;
    }
    if(arr->argv) MFree(arr->argv);
    arr->argv = NULLPTR;
    arr->argc = 0;
}

VOID BATSH_SET_MODE(U8 mode) {
    batsh_mode = mode;
}

U8 BATSH_GET_MODE(void) {
    return batsh_mode;
}

// Find variable index by name
// Find global variable index by name
static S32 FIND_VAR(PU8 name) {
    if (!name) return -1;

    // 1. Try matching directly
    for (U32 i = 0; i < shell_var_count; i++) {
        if (STRICMP(name, shell_vars[i].name) == 0)
            return i;
    }

    U32 name_len = STRLEN(name);

    // 2. Try matching with "@name"
    PU8 tmp1 = MAlloc(name_len + 2); // '@' + name + '\0'
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

    // 3. Try matching with "@{name}"
    PU8 tmp2 = MAlloc(name_len + 4); // "@{" + name + "}" + '\0'
    if (!tmp2) {
        MFree(tmp1);
        return -1;
    }

    tmp2[0] = '@';
    tmp2[1] = '{';
    MEMCPY(tmp2 + 2, name, name_len);
    tmp2[name_len + 2] = '}';
    tmp2[name_len + 3] = '\0';

    for (U32 i = 0; i < shell_var_count; i++) {
        if (STRICMP(tmp2, shell_vars[i].name) == 0) {
            MFree(tmp1);
            MFree(tmp2);
            return i;
        }
    }

    MFree(tmp1);
    MFree(tmp2);
    return -1;
}


// Set or update a shell variable
VOID SET_VAR(PU8 name, PU8 value) {
    S32 idx = FIND_VAR(name);
    if(shell_var_count + 1 > MAX_VARS) return FALSE;
    if (idx >= 0 && idx != -1) {
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
    return (idx >= 0 && idx != -1) ? shell_vars[idx].value : (PU8)NULLPTR;
}

static S32 FIND_INST_VAR(PU8 name, BATSH_INSTANCE *inst) {
    if (!name || !inst) return -1;

    // 1. Try matching directly
    for (U32 i = 0; i < inst->shell_var_count; i++) {
        if (STRICMP(name, inst->shell_vars[i].name) == 0)
            return i;
    }

    U32 name_len = STRLEN(name);

    // 2. Try matching with "@name"
    PU8 tmp1 = MAlloc(name_len + 2); // '@' + name + '\0'
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

    // 3. Try matching with "@{name}"
    PU8 tmp2 = MAlloc(name_len + 4); // "@{" + name + "}" + '\0'
    if (!tmp2) {
        MFree(tmp1);
        return -1;
    }

    tmp2[0] = '@';
    tmp2[1] = '{';
    MEMCPY(tmp2 + 2, name, name_len);
    tmp2[name_len + 2] = '}';
    tmp2[name_len + 3] = '\0';

    for (U32 i = 0; i < inst->shell_var_count; i++) {
        if (STRICMP(tmp2, inst->shell_vars[i].name) == 0) {
            MFree(tmp1);
            MFree(tmp2);
            return i;
        }
    }

    MFree(tmp1);
    MFree(tmp2);
    return -1;
}

BOOLEAN SET_INST_VAR(PU8 name, PU8 value, BATSH_INSTANCE *inst) {
    if(!inst) return FALSE;
    if(inst->shell_var_count + 1 > MAX_VARS) return FALSE;
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
    if(!inst) return NULL;
    S32 idx = FIND_INST_VAR(name, inst);
    return (idx >= 0 && idx != -1) ? inst->shell_vars[idx].value : NULLPTR;
}

PU8 GET_INST_ARG(U32 index, BATSH_INSTANCE *inst) {
    if(index > inst->arg_count) return NULLPTR;
    return inst->args[index];
}

KEYWORD *GET_KEYWORD_S(PU8 keyword) {
    for(U32 i = 0; i<KEYWORDS_COUNT;i++) {
        if(STRICMP(keyword, KEYWORDS[i].keyword) == 0) return &KEYWORDS[i];
    }
    return NULLPTR;
}
KEYWORD *GET_KEYWORD_C(CHAR c) {
    U8 buf[2];
    buf[0] = c;
    buf[1] = '\0';
    for(U32 i = 0; i<KEYWORDS_COUNT;i++) {
        if(STRICMP(buf, KEYWORDS[i].keyword) == 0) return &KEYWORDS[i];
    }
    return NULLPTR;
}










struct pos {
    U32 line;
    U32 row;
};

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

    // For compound commands (pipe, if, loop)
    struct BATSH_COMMAND *left;
    struct BATSH_COMMAND *right;

    // Optional for future
    struct BATSH_COMMAND *next; // for sequential execution

    struct pos;
} BATSH_COMMAND, *PBATSH_CMD;









BATSH_COMMAND *parse_if(BATSH_TOKEN **tokens, U32 len, U32 *i, BATSH_INSTANCE *inst) {

}
BATSH_COMMAND *parse_loop(BATSH_TOKEN **tokens, U32 len, U32 *i, BATSH_INSTANCE *inst) {

}

BATSH_COMMAND *parse_var_assignment(BATSH_TOKEN **tokens, U32 len, U32 *i, BATSH_INSTANCE *inst) {
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
    // Move to next token
    for (; *i < len; (*i)++) {
        BATSH_TOKEN *tkn = tokens[*i];
        assign_track--;
        switch (tkn->type) {
            case TOK_ESCAPE:
                escapes++;
                continue;

            case TOK_EOL:
                if (escapes > 0) { escapes--; continue; }
                end_next = TRUE;
                break;

            case TOK_SEMI:
                end_next = TRUE;
                break;

            case TOK_ASSIGN:
                assign_track = 2;
                continue;
            case TOK_STRING:
            case TOK_WORD:
            case TOK_VAR:
            case TOK_VAR_GLOBAL:
                if(assign_track == 1) has_assign = TRUE;
                var_tkn->argv = ReAlloc(var_tkn->argv, sizeof(char*) * (var_tkn->argc + 1));
                var_tkn->argv[var_tkn->argc++] = STRDUP(tkn->text);
                break;

            default:
                // stop on new block or command start
                if (tkn->type == TOK_IF || tkn->type == TOK_LOOP) {
                    (*i)--; 
                    end_next = TRUE;
                }
                break;
        }

        if (end_next) break;
    }

    if (has_assign) {
        if(original_type == TOK_VAR_GLOBAL) var_tkn->type = CMD_GLOB_ASSIGN;
        else var_tkn->type = CMD_ASSIGN;
    } else {
        var_tkn->type = CMD_SIMPLE;
    }

    return var_tkn;
}

BATSH_COMMAND *parse_word_or_cmd(BATSH_TOKEN **tokens, U32 len, U32 *i, BATSH_INSTANCE *inst) {
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
            case TOK_DIV: // For paths and such.
                append_next = 2;
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
                if(append_next == 1) {
                    cmd_tkn->argv[cmd_tkn->argc - 1] = STRAPPEND(cmd_tkn->argv[cmd_tkn->argc - 1], tkn->text);
                } else {
                    cmd_tkn->argv = ReAlloc(cmd_tkn->argv, sizeof(char*) * (cmd_tkn->argc + 1));
                    cmd_tkn->argv[cmd_tkn->argc++] = STRDUP(tkn->text);
                }
                break;

            default:
                if (tkn->type == TOK_IF || tkn->type == TOK_LOOP) {
                    (*i)--; // let master handle it
                    end_next = TRUE;
                }
                break;
        }

        if (end_next) break;
    }

    if(tkn->type == TOK_DIV) {
        cmd_tkn->argv[cmd_tkn->argc - 1] = STRAPPEND(cmd_tkn->argv[cmd_tkn->argc - 1], tkn->text);
    }

    return cmd_tkn;
}

BATSH_COMMAND *parse_master(BATSH_TOKEN **tokens, U32 len, BATSH_INSTANCE *inst) {
    BATSH_COMMAND *head = NULL, *tail = NULL;

    for (U32 i = 0; i < len;) {
        BATSH_TOKEN *tkn = tokens[i];
        BATSH_COMMAND *cmd = NULL;
        // DEBUG_PUTS_LN(tkn->text);

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

        if (!head)
            head = tail = cmd;
        else {
            tail->next = cmd;
            tail = cmd;
        }
    }

    return head;
}

void free_batsh_command(BATSH_COMMAND *cmd) {
    if (!cmd) return;

    // Free argv strings
    if (cmd->argv) {
        for (int i = 0; i < cmd->argc; i++) {
            if (cmd->argv[i]) MFree(cmd->argv[i]);
        }
        MFree(cmd->argv);
    }

    // Recursively free compound commands
    free_batsh_command(cmd->left);
    free_batsh_command(cmd->right);
    free_batsh_command(cmd->next);

    // Free the command struct itself
    MFree(cmd);
}

BOOL resolve_var_math(PU8 line, I32 *answ_out, BATSH_INSTANCE *inst) {
    if (!line || !answ_out) return FALSE;

    PU8 tmp = STRDUP(line);
    if (!tmp) return FALSE;

    // TODO: implement actual math resolution later, for now just 0
    *answ_out = 0;

    MFree(tmp);
    return TRUE;
}


BOOL resolve_vars(PU8 *line, BATSH_INSTANCE *inst) {
    if (!line || !*line) return TRUE;

    PU8 tmp = STRDUP(*line);
    if (!tmp) return FALSE;

    PU8 out = MAlloc(STRLEN(*tmp) * 2 + 1); // allocate larger buffer
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


                if (IS_DIGIT_STR(value) && inst) {
                    U32 arg_index = ATOI(var_name);
                    value = GET_INST_ARG(arg_index, inst);
                }

                S32 idx = 0; 
                if (!value || !*value) {
                    idx = FIND_VAR(var_name);
                    if (idx >= 0)
                        value = GET_VAR(var_name);
                }

                if (!value || !*value) {
                    idx = FIND_INST_VAR(var_name, inst);
                    if (idx >= 0)
                        value = GET_INST_VAR(var_name, inst);
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

    // Reassign line to the new buffer and free the old
    MFree(*line);
    *line = out;

    MFree(tmp);
    return TRUE;
}



BOOLEAN execute_master(BATSH_COMMAND *master, BATSH_INSTANCE *inst) {
    BATSH_COMMAND *cmd = master;

    while (cmd != NULLPTR) {
        switch (cmd->type) {
        case CMD_SIMPLE: {
            PU8 line_as_is = NULLPTR;
            for (U32 i = 0; i < cmd->argc; i++) {
                // line_as_is = STRAPPEND_SEPARATOR(line_as_is, cmd->argv[i], ' ');
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

            for (U32 i = 1; i < cmd->argc; i++) {
                value = STRAPPEND(value, cmd->argv[i]);
            }
            resolve_vars(&value, inst);
            S32 idx = FIND_VAR(name);
            if (idx >= 0) SET_VAR(name, value);
            MFree(value);
        } break;
        case CMD_ASSIGN: {
            if (cmd->argc < 2) break;
            PU8 name = cmd->argv[0];
            PU8 value = NULL;

            for (U32 i = 1; i < cmd->argc; i++) {
                value = STRAPPEND(value, cmd->argv[i]);
            }
            resolve_vars(&value, inst);
            if (inst) {
                S32 idx = FIND_VAR(name);
                if (idx >= 0)
                    SET_VAR(name, value);
                else
                    SET_INST_VAR(name, value, inst);
            } else {
                S32 idx = FIND_VAR(name);
                if (idx >= 0) SET_VAR(name, value);
            }

            MFree(value);
        } break;

        default:
            break;
        }

        cmd = cmd->next;
    }

    return TRUE;
}

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
    PUTS(buf);
    PRINTNEWLINE();
}

static BOOL finalize_token(U8 *buf, U32 *ptr, BATSH_TOKEN ***tokens, U32 *token_len, struct pos pos) {
    if (*ptr == 0) return TRUE;

    buf[*ptr] = '\0';
    BOOLEAN KEYWORD_ADDED = FALSE;
    for (U32 j = 0; j < KEYWORDS_COUNT; j++) {
        if (STRICMP(buf, KEYWORDS[j].keyword) == 0) {
            if (!ADD_TOKEN(tokens, token_len, buf, &KEYWORDS[j])) return FALSE;
            KEYWORD_ADDED = TRUE;
            break;
        }
    }

    if (!KEYWORD_ADDED) {
        KEYWORD throwaway = { "", TOK_WORD };
        if (!ADD_TOKEN(tokens, token_len, buf, &throwaway)) return FALSE;
    }

    *ptr = 0;
    return TRUE;
}


// ===================================================
// BATSH Parser
// Takes either full file or line from cmd line
// Tokenizes, parses and runs the input
// ===================================================
BOOLEAN PARSE_BATSH_INPUT(PU8 input, BATSH_INSTANCE *inst) {
    if (!input || !*input) return TRUE;
    U32 len = STRLEN(input);
    if(len == 0) {
        return TRUE;
    }
    U8 buf[CUR_LINE_MAX_LENGTH] = {0};
    U32 ptr = 0;
    BATSH_TOKEN **tokens = NULL;
    U32 token_len = 0;
    // Lex input
    struct pos pos = { .line = 0, .row = 0 };
    KEYWORD *rem = GET_KEYWORD_S("rem");
    CHAR c = 0;

    BOOL back_to_singles = FALSE;

    BOOL relexed = FALSE;
    for (U32 i = 0; i < len; i++) {
        c = input[i];
        relex:
        if (c == '\n' || c == '\r') {
            pos.row++;
            pos.line = 0;
        } else {
            pos.line++;
        }
        // Keywords and full words
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            cmp_words:
            if (ptr > 0) {
                if(!back_to_singles) {
                    buf[ptr++] = c;
                }
                buf[ptr] = '\0';
                if(STRICMP(buf, rem->keyword) == 0) {
                    // Loop until end of line
                    for(;i < len; i++) {
                        CHAR c2 = input[i];
                        if(c2 == '\n' || c2 == '\r') {
                            break;
                        }
                    }
                    ptr = 0;
                    if(back_to_singles) {
                        goto back_to_single_parse;
                    }
                    continue;
                }
                BOOLEAN KEYWORD_ADDED = FALSE;
                for (U32 j = 0; j < KEYWORDS_COUNT; j++) {
                    if (STRICMP(buf, KEYWORDS[j].keyword) == 0) {
                        if (!ADD_TOKEN(&tokens, &token_len, buf, &KEYWORDS[j])) {
                            PRINT_TOKENIZER_ERR(buf, &KEYWORDS[j], pos);
                            goto error;
                        }
                        KEYWORD_ADDED = TRUE;
                        break;
                    }
                }

                if(!KEYWORD_ADDED) {
                    KEYWORD throwaway = { "", TOK_WORD };
                    if(!ADD_TOKEN(&tokens, &token_len, buf, &throwaway)) {
                        PRINT_TOKENIZER_ERR(buf, &throwaway, pos);
                        goto error;
                    }
                }
                ptr = 0;
            }
            if(c == '\n' || c == '\r') {
                KEYWORD kw = { "~", TOK_EOL };
                if(!ADD_TOKEN(&tokens, &token_len, kw.keyword, &kw)) goto error;
            }
            if(back_to_singles) {
                goto back_to_single_parse;
            }
            continue;
        } 
        else if(c == '"') {
            // parse quoted string
            i++;
            U32 start = i;
            while (i < len) {
                if (input[i] == '"' && input[i - 1] != '\\') {
                    start--;
                    i++;
                    break;
                }
                i++;
            }
            U32 str_len = i - start;
            if (str_len >= CUR_LINE_MAX_LENGTH)
                str_len = CUR_LINE_MAX_LENGTH - 1;
            MEMCPY(buf, &input[start], str_len);
            buf[str_len] = '\0';

            KEYWORD str_kw = { "\"", TOK_STRING };
            if (!ADD_TOKEN(&tokens, &token_len, buf, &str_kw)) goto error;
            ptr = 0;
            continue;
        }
        else if(c == '#') {
            for(;i < len; i++) {
                CHAR c2 = input[i];
                if(c2 == '\n' || c2 == '\r') {
                    break;
                }
            }
            KEYWORD str_kw = { "~", TOK_EOL };
            if (!ADD_TOKEN(&tokens, &token_len, buf, &str_kw)) goto error;
            ptr = 0;
            continue;
        }
        else if (c == '@') {
            // Check if it's @@ (global variable)
            BOOL is_global = FALSE;
            if (i + 1 < len && input[i + 1] == '@') {
                is_global = TRUE;
                i++; // skip second '@'
            }

            i++; // move past '@' or '@@'
            U32 start = i;
            BOOL was_bracket = FALSE;

            if (input[i] == '{') {
                i++; // skip '{'
                start = i;
                was_bracket = TRUE;
                while (i < len && input[i] != '}') i++;
            } else {
                while (i < len && ISALNUM(input[i])) i++;
            }

            U32 var_len = i - start + (was_bracket ? 3 : 1);
            if (var_len >= CUR_LINE_MAX_LENGTH)
                var_len = CUR_LINE_MAX_LENGTH - 1;

            MEMCPY(buf, &input[start - (was_bracket ? 2 : (is_global ? 3 : 1))], var_len);
            buf[var_len] = '\0';
            KEYWORD var_kw = { buf, is_global ? TOK_VAR_GLOBAL : TOK_VAR };

            if (!ADD_TOKEN(&tokens, &token_len, buf, &var_kw))
                goto error;

            if (input[i] == '}') i++; // skip closing brace
            // Allow re-parsing next operator
            if (STRCHR("|+-*/(){}=@><\\\n\t\r ", input[i])) i--;

            continue;
        }
        else if(c == ';') {
            if(ptr > 0) {
                KEYWORD kw = { ";", TOK_SEMI };
                if(!ADD_TOKEN(&tokens, &token_len, kw.keyword, &kw)) goto error;
                goto cmp_words;
            }
        }
        else if (
            c == '|' ||
            c == '+' ||
            c == '-' ||
            c == '*' ||
            c == '(' ||
            c == ')' ||
            c == '}' ||
            c == '/' ||
            c == '{' ||
            c == '=' ||
            c == '@' ||
            c == '>' ||
            c == '<' ||
            c == '{' ||
            c == '}' ||
            c == '\\'
        )
        {
            if (ptr > 0) {
                back_to_singles = TRUE;
                goto cmp_words;
            }

            back_to_single_parse:
            back_to_singles = FALSE;
            ptr = 0;
            buf[ptr] = c;
            buf[++ptr] = '\0';
            KEYWORD *kw = GET_KEYWORD_C(c);
            if (!kw) {
                PRINT_TOKENIZER_ERR(buf, NULLPTR, pos);
                goto error;
            }
            if (!ADD_TOKEN(&tokens, &token_len, buf, kw)) {
                PRINT_TOKENIZER_ERR(buf, kw, pos);
                goto error;
            }
            ptr = 0;
            continue;
        }


        buf[ptr++] = c;
        if (ptr >= CUR_LINE_MAX_LENGTH - 1)
            break;
    }
    if(relexed) {
        KEYWORD kw22 = { "~", TOK_EOL };
        if(!ADD_TOKEN(&tokens, &token_len, kw22.keyword, &kw22)) goto error;

    }
    if (ptr > 0 && !relexed) {
        c = ' ';
        relexed = TRUE;
        goto relex;
    }

    // Parse the tokens
    BATSH_COMMAND *root_cmd = parse_master(tokens, token_len, inst);
    if (!root_cmd) {
        PUTS("Parsing failed\n");
        goto error;
    }

    if(!execute_master(root_cmd, inst)) {
        PUTS("Executing failed\n");
        SET_VAR("ERRORLEVEL", "1");
        goto error;
    }
    SET_VAR("ERRORLEVEL", "0");

    U8 res = TRUE;
    goto free;
error:
    res = FALSE;
free:
    for (U32 i = 0; i < token_len; i++) {
        // DEBUG_PRINTF("%s\n", tokens[i]->text);
        if (tokens[i]) MFree(tokens[i]);
    }
    if (tokens) MFree(tokens);
    if (root_cmd) free_batsh_command(root_cmd);
    return res;
}


// ===================================================
// Core Command Handler
// ===================================================
VOID HANDLE_COMMAND(U8 *line) {
    OutputHandle cursor = GetOutputHandle();
    cursor->CURSOR_VISIBLE = FALSE;

    if (STRLEN(line) == 0) {
        cursor->CURSOR_VISIBLE = TRUE;
        return;
    }

    // Extract command name
    U8 command[64];
    U32 i = 0;
    U32 j = 0;

    while (line[i] && line[i] != ' ' && j < sizeof(command) - 1)
        command[j++] = line[i++];
    command[j] = '\0';


    BOOLEAN found = FALSE;
    for (U32 j = 0; j < shell_command_count; j++) {
        if (STRICMP((CHAR*)command, shell_commands[j].name) == 0) {
            if(shell_commands[j].handler != NULLPTR) {
                shell_commands[j].handler(line);
                found = TRUE;
            }
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
BOOLEAN RUN_BATSH_FILE(BATSH_INSTANCE *inst) {
    if (!inst) return FALSE;
    OutputHandle crs = GetOutputHandle();
    U32 prev = crs->CURSOR_VISIBLE;
    crs->CURSOR_VISIBLE = FALSE;

    inst->echo = TRUE;
    inst->batsh_mode = TRUE;

    while (1) {
        U32 n = FREAD(inst->file, inst->line, inst->line_len - 1);
        if(n == 0) break; // EOF
        inst->line[n] = '\0'; // null-terminate exactly after read bytes

        if(!PARSE_BATSH_INPUT(inst->line, inst)) {
            PUTS("BATSH parse error\n");
        }
    }

    crs->CURSOR_VISIBLE = prev;
    return TRUE;
}



// ===================================================
// BATSH Script Loader
// ===================================================
BOOLEAN RUN_BATSH_SCRIPT(PU8 path, U32 argc, PPU8 argv) {
    PU8 dot = STRRCHR(path, '.');
    if (!dot || STRNICMP(dot, ".SH", 3) != 0) return FALSE;
    BATSH_INSTANCE *inst = CREATE_BATSH_INSTANCE(path, argc, argv);
    if(!inst) return FALSE;
    BOOLEAN res = RUN_BATSH_FILE(inst);
    DESTROY_BATSH_INSTANCE(inst);
    return res;
}

// ===================================================
// BATSH Instance initializer
// ===================================================
BATSH_INSTANCE *CREATE_BATSH_INSTANCE(PU8 path, U32 argc, PPU8 argv) {
    BATSH_INSTANCE *inst = MAlloc(sizeof(BATSH_INSTANCE));
    if(!inst) return NULLPTR;
    // DEBUG_PRINTF("%s\n", path);
    MEMZERO(inst, sizeof(BATSH_INSTANCE));
    inst->file = FOPEN(path, MODE_R | MODE_FAT32);
    if(!inst->file) {
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
    if(inst->arg_count > MAX_ARGS) inst->arg_count = MAX_ARGS;
    for(U32 i = 0; i < inst->arg_count; i++) {
        inst->args[i] = STRDUP(argv[i]);
    }

    inst->shndl = GET_SHNDL();
    inst->status_code = 0;
    return inst;
}

// ===================================================
// BATSH instance destroyer
// ===================================================
VOID DESTROY_BATSH_INSTANCE(BATSH_INSTANCE *inst) {
    if(!inst) return;
    for(U32 i=0;i<inst->arg_count;i++) {
        MFree(inst->args[i]);
    }
    FCLOSE(inst->file);
    MFree(inst);
}



















#define TAIL_LINE_COUNT 10
#include <PROGRAMS/SHELL/CMD/MISC.c>
#include <PROGRAMS/SHELL/CMD/AC97.c>
#include <PROGRAMS/SHELL/CMD/CD.c>
#include <PROGRAMS/SHELL/CMD/DIR.c>
#include <PROGRAMS/SHELL/CMD/MKDIR.c>
#include <PROGRAMS/SHELL/CMD/RMDIR.c>
#include <PROGRAMS/SHELL/CMD/TYPE.c>
#include <PROGRAMS/SHELL/CMD/TAIL.c>
#include <PROGRAMS/SHELL/CMD/COLOUR.c>


VOID SETUP_BATSH_PROCESSER() {
    SET_VAR("PATH", 
        #include <ATOSH.PATH>
    );
    #include <ATOSH.ENV_VARS>
}


BOOLEAN RUN_PROCESS(PU8 line) {
    if (!line || !*line) return FALSE;

    // ---------------------------------------------------------------------
    // Make ONE safe copy of the raw command line (for parsing path & args)
    // ---------------------------------------------------------------------
    U8 original_line[512];
    STRNCPY(original_line, line, sizeof(original_line) - 1);
    original_line[sizeof(original_line) - 1] = 0;
    str_trim(original_line);

    if (!*original_line) return FALSE;

    // ---------------------------------------------------------------------
    // Extract first token (program path)
    // ---------------------------------------------------------------------
    PU8 first_space = STRCHR(original_line, ' ');
    U32 prog_len = first_space ? (U32)(first_space - original_line)
                               : STRLEN(original_line);

    PU8 prog_copy = MAlloc(prog_len + 1);
    STRNCPY(prog_copy, original_line, prog_len);
    prog_copy[prog_len] = 0;

    // ---------------------------------------------------------------------
    // Extract pure program name (strip directories)
    // ---------------------------------------------------------------------
    PU8 prog_name = STRDUP(prog_copy);
    PU8 last_slash = STRRCHR(prog_name, '/');
    if (last_slash) {
        PU8 tmp = STRDUP(last_slash + 1);
        MFree(prog_name);
        prog_name = tmp;
    }

    // ---------------------------------------------------------------------
    // Try resolving directly, else search PATH
    // ---------------------------------------------------------------------
    U8 abs_path_buf[512];
    FAT_LFN_ENTRY ent;
    BOOLEAN found = FALSE;
    BOOLEAN is_found_as_bin = FALSE;
    BOOLEAN is_found_as_sh  = FALSE;

    // ---- First attempt: raw program path ----
    if (ResolvePath(prog_copy, abs_path_buf, sizeof(abs_path_buf))) {
        if (FAT32_PATH_RESOLVE_ENTRY(abs_path_buf, &ent)) {
            found = TRUE;
        } else {
            // Try with extensions
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

    // ---- If still not found, search PATH ----
    if (!found) {
        PU8 path_val = GET_VAR((PU8)"PATH");

        if (path_val && *path_val) {
            PU8 path_dup = STRDUP(path_val);
            PU8 saveptr  = NULL;
            PU8 tok      = STRTOK_R(path_dup, ";", &saveptr);

            while (tok && !found) {
                if (*tok) {
                    U8 candidate[512] = {0};
                    STRNCPY(candidate, tok, sizeof(candidate) - 1);

                    // Append slash if needed
                    U32 len = STRLEN(candidate);
                    if (len && candidate[len - 1] != '/' && candidate[len - 1] != '\\') {
                        STRNCAT(candidate, "/", sizeof(candidate) - STRLEN(candidate) - 1);
                    }

                    STRNCAT(candidate, prog_name, sizeof(candidate) - STRLEN(candidate) - 1);

                    // Try resolving
                    if (ResolvePath(candidate, abs_path_buf, sizeof(abs_path_buf))) {
                        if (FAT32_PATH_RESOLVE_ENTRY(abs_path_buf, &ent)) {
                            found = TRUE;
                            break;
                        }

                        // Try .BIN / .SH
                        U8 try_ext[512];

                        STRCPY(try_ext, abs_path_buf);
                        STRNCAT(try_ext, ".BIN", sizeof(try_ext) - STRLEN(try_ext) - 1);
                        if (FAT32_PATH_RESOLVE_ENTRY(try_ext, &ent)) {
                            STRCPY(abs_path_buf, try_ext);
                            found = TRUE;
                            is_found_as_bin = TRUE;
                            break;
                        }

                        STRCPY(try_ext, abs_path_buf);
                        STRNCAT(try_ext, ".SH", sizeof(try_ext) - STRLEN(try_ext) - 1);
                        if (FAT32_PATH_RESOLVE_ENTRY(try_ext, &ent)) {
                            STRCPY(abs_path_buf, try_ext);
                            found = TRUE;
                            is_found_as_sh = TRUE;
                            break;
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

    // ---------------------------------------------------------------------
    // Load file contents
    // ---------------------------------------------------------------------
    U32 file_size = 0;
    VOIDPTR data = FAT32_READ_FILE_CONTENTS(&file_size, &ent.entry);
    if (!data) {
        MFree(prog_name);
        return FALSE;
    }

    // ---------------------------------------------------------------------
    // Parse arguments (from original_line)
    // ---------------------------------------------------------------------
    PU8* argv = MAlloc(sizeof(PU8*) * MAX_ARGS);
    U32 argc = 0;

    argv[argc++] = STRDUP(abs_path_buf);  // argv[0] = program path

    // Now parse only arguments:
    PU8 args_start = first_space ? first_space + 1 : NULL;

    if (args_start && *args_start) {
        PU8 p = args_start;

        while (*p && argc < MAX_ARGS) {
            // skip leading whitespace
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;

            BOOL in_quotes = FALSE;
            PU8 ArgBegin;

            if (*p == '"') {
                in_quotes = TRUE;
                p++;                 // skip opening quote
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

            argv[argc++] = arg;

            if (in_quotes && *p == '"')
                p++; // skip closing quote

            // skip trailing whitespace before next argument
            while (*p == ' ' || *p == '\t') p++;
        }
    }

    // ---------------------------------------------------------------------
    // Determine extension (.bin or .sh)
    // ---------------------------------------------------------------------
    PU8 ext = STRRCHR(abs_path_buf, '.');
    ext = ext ? (ext + 1) : (PU8)"";

    BOOLEAN result = FALSE;

    // ---------------------------------------------------------------------
    // Execute binary
    // ---------------------------------------------------------------------
    if (STRICMP(ext, "BIN") == 0) {
        U32 pid = PROC_GETPID();
        U32 res = START_PROCESS(abs_path_buf, data, file_size,
                                TCB_STATE_ACTIVE, pid, argv, argc);
        if (res) PRINTNEWLINE();
        result = (res != 0);
    }

    // ---------------------------------------------------------------------
    // Execute script
    // ---------------------------------------------------------------------
    else if (STRICMP(ext, "SH") == 0) {
        result = RUN_BATSH_SCRIPT(abs_path_buf, argc, argv);
        for (U32 i = 0; i < argc; i++) {
            if (argv[i]) MFree(argv[i]);
            SET_NULL(argv[i]);
        }
        MFree(argv);
        SET_NULL(argv);
    }

    // ---------------------------------------------------------------------
    // Cleanup
    // Only prog_name is cleared in all cases.
    // PROC frees args for us
    // prog_name is deep copied into RUN_BINARY_STRUCT
    // ---------------------------------------------------------------------
    MFree(prog_name);
    SET_NULL(prog_name);
    return result;
}
