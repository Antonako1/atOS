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
    { ">", TOK_REDIR_OUT },
    { "<", TOK_REDIR_IN },
    { ";", TOK_SEMI },
    { "\\", TOK_ESCAPE },
    { "#", TOK_COMMENT },
};
#define KEYWORDS_COUNT (sizeof(KEYWORDS) / sizeof(KEYWORDS[0]))

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
    { "type",     CMD_TYPE,    "Print file contents" },
    { "tail",     CMD_TAIL,    "Print tail of file contents" },

    // Any commands added here must be added 
    //   into the KEYWORDS array above as TOK_CMD in built-ins section!
};

#define shell_command_count (sizeof(shell_commands) / sizeof(shell_commands[0]))

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
    if (*i >= len || tokens[*i]->type != TOK_VAR)
        return var_tkn;

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
        var_tkn->type = CMD_ASSIGN;
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
        DEBUG_PUTS_LN(tkn->text);
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
                SET_VAR(name, value);
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
                if (input[i] == '"' && input[i - 1] != '\\')
                    break;
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
        else if(c == '@') {
            i++; // move past '@'
            U32 start = i;
            BOOL was_bracket = FALSE;
            if(input[i] == '{') {
                i++; // skip '{'
                start = i;
                was_bracket = TRUE;
                while(i < len && input[i] != '}') i++;
            } else {
                while(i < len && ISALNUM(input[i])) i++;
            }
            U32 var_len = i - start + (was_bracket ? 3 : 1);
            if(var_len >= CUR_LINE_MAX_LENGTH) var_len = CUR_LINE_MAX_LENGTH - 1;
            MEMCPY(buf, &input[start - (was_bracket ? 2 : 1)], var_len);
            buf[var_len] = '\0';

            KEYWORD var_kw = { buf, TOK_VAR };
            if(!ADD_TOKEN(&tokens, &token_len, buf, &var_kw)) goto error;

            if(input[i] == '}') i++; // skip closing brace
            if(input[i] == '=' ||
                input[i] == '|' ||
                input[i] == '+' ||
                input[i] == '-' ||
                input[i] == '*' ||
                input[i] == '(' ||
                input[i] == ')' ||
                input[i] == '}' ||
                input[i] == '/' ||
                input[i] == '{' ||
                input[i] == '=' ||
                input[i] == '@' ||
                input[i] == '>' ||
                input[i] == '<' ||
                input[i] == '{' ||
                input[i] == '}' ||
                input[i] == '\\' ||
                input[i] == '\n' ||
                input[i] == '\t' ||
                input[i] == '\r' ||
                input[i] == ' '
            ) i--; // reparse 
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
        goto error;
    }

    U8 res = TRUE;
    goto free;
error:
    res = FALSE;
free:
    for (U32 i = 0; i < token_len; i++) {
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
    if (!dot || STRNICMP(dot, ".SH", 3) != 0)
    return FALSE;
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

    if(!FOPEN(inst->file, path, MODE_R | MODE_FAT32)) {
        MFree(inst);
        return NULLPTR;
    }
    inst->line_len = SHELL_SCRIPT_LINE_MAX_LEN;
    MEMZERO(inst->line, inst->line_len);
    inst->batsh_mode = 1;
    inst->child_proc = NULLPTR;
    inst->parent_proc = NULLPTR;
    inst->echo = 1;
    inst->shell_var_count = 0;
    MEMZERO(inst->shell_vars, sizeof(inst->shell_vars));
    inst->arg_count = argc;
    if(inst->arg_count > MAX_ARGS) inst->arg_count = MAX_ARGS;
    for(U32 i = 0; i < inst->arg_count; i++) {
        STRCPY(inst->args[i], argv[i]);
    }
    MEMZERO(inst->args, sizeof(inst->args));
    inst->shndl = GET_SHNDL();
    inst->status_code = 0;
    return inst;
}

// ===================================================
// BATSH instance destroyer
// ===================================================
VOID DESTROY_BATSH_INSTANCE(BATSH_INSTANCE *inst) {
    if(!inst) return;
    FCLOSE(inst->file);
    MFree(inst);
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
    PRINTNEWLINE();
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
    SET_VAR("CD", GET_PATH());
}

VOID CMD_CD_BACKWARDS(PU8 line) { 
    (void)line; CD_BACKWARDS_DIR(); PRINTNEWLINE(); 
    SET_VAR("CD", GET_PATH());
}

VOID CMD_DIR(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);

    PU8 path_arg = PARSE_CD_RAW_LINE(tmp_line, 3);
    if (STRCMP(path_arg, "-") == 0) path_arg = GET_PREV_PATH();
    else if (STRCMP(path_arg, "~") == 0) path_arg = (PU8)"/HOME";
    else if (!path_arg || *path_arg == '\0') path_arg = (PU8)".";
    PRINT_CONTENTS_PATH(path_arg);
    PRINTNEWLINE();
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

#define TAIL_LINE_COUNT 10

VOID CMD_TYPE(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);
    PU8 path_arg = PARSE_CD_RAW_LINE(tmp_line, 4);
    if (!path_arg || !*path_arg) {
        PUTS("\ntype: missing file path.\n");
        return;
    }

    FAT_LFN_ENTRY entry;
    if (!FAT32_PATH_RESOLVE_ENTRY(path_arg, &entry)) {
        PUTS("\ntype: file not found.\n");
        return;
    }

    if (entry.entry.ATTRIB & FAT_ATTRB_DIR) {
        PUTS("\ntype: cannot display a directory.\n");
        return;
    }

    U32 file_size = entry.entry.FILE_SIZE;
    if (file_size == 0) {
        PUTS("\n(empty file)\n");
        return;
    }

    U8 *buffer = FAT32_READ_FILE_CONTENTS(&file_size, &entry.entry);
    if (!buffer) {
        PUTS("\ntype: failed to read file.\n");
        MFree(buffer);
        return;
    }

    buffer[file_size] = '\0';
    PUTS("\n");
    PUTS(buffer);
    PUTS("\n");

    MFree(buffer);
}

VOID CMD_TAIL(PU8 raw_line) {
    MEMZERO(tmp_line, sizeof(tmp_line));
    STRNCPY(tmp_line, raw_line, sizeof(tmp_line) - 1);
    PU8 path_arg = PARSE_CD_RAW_LINE(tmp_line, 4);
    if (!path_arg || !*path_arg) {
        PUTS("\ntail: missing file path.\n");
        return;
    }

    FAT_LFN_ENTRY entry;
    if (!FAT32_PATH_RESOLVE_ENTRY(path_arg, &entry)) {
        PUTS("\ntail: file not found.\n");
        return;
    }

    if (entry.entry.ATTRIB & FAT_ATTRB_DIR) {
        PUTS("\ntail: cannot read a directory.\n");
        return;
    }

    U32 file_size = entry.entry.FILE_SIZE;
    if (file_size == 0) {
        PUTS("\n(empty file)\n");
        return;
    }
    U8 *buffer = FAT32_READ_FILE_CONTENTS(&file_size, &entry.entry);
    if (!buffer) {
        PUTS("\ntype: failed to read file.\n");
        MFree(buffer);
        return;
    }

    buffer[file_size] = '\0';

    // Find start of last N lines
    U32 line_count = 0;
    for (U32 i = file_size; i > 0; i--) {
        if (buffer[i - 1] == '\n') {
            line_count++;
            if (line_count > TAIL_LINE_COUNT) {
                buffer[i] = '\0';
                PUTS("\n");
                PUTS(&buffer[i]);
                PUTS("\n");
                MFree(buffer);
                return;
            }
        }
    }

    // If fewer than N lines, print all
    PUTS("\n");
    PUTS(buffer);
    PUTS("\n");

    MFree(buffer);
}


VOID SETUP_BATSH_PROCESSER() {
    SET_VAR("PATH", 
        ";/ATOS/;"
        "/PROGRAMS/ASTRAC/;"
        "/PROGRAMS/GLYPH/;"
        "/PROGRAMS/HEXNEST/;"
        "/PROGRAMS/IRONCLAD/;"
        "/PROGRAMS/SATP/;"
        "/PROGRAMS/SHELL/;"
    );
    SET_VAR("HOME", "/HOME");
    SET_VAR("DOCS", "/HOME/DOCS");
    SET_VAR("CD", "/HOME/DOCS");
}


BOOLEAN RUN_PROCESS(PU8 line) {
    if (!line || !*line) return FALSE;
    // --------------------------------------------------
    // Prepare working buffers
    // --------------------------------------------------
    U8 tmp_line[256];
    STRNCPY(tmp_line, line, sizeof(tmp_line) - 1);
    str_trim(tmp_line);

    // --------------------------------------------------
    // Extract first token (program path)
    // --------------------------------------------------
    PU8 first_space = STRCHR(tmp_line, ' ');
    U32 prog_len = first_space ? (U32)(first_space - tmp_line) : STRLEN(tmp_line);

    PU8 prog_copy = MAlloc(prog_len + 1);
    STRNCPY(prog_copy, tmp_line, prog_len);
    prog_copy[prog_len] = 0;

    // Extract program name
    PU8 prog_name = STRDUP(prog_copy);
    PU8 last_slash = STRRCHR(prog_name, '/');
    if (last_slash) {
        PU8 tmp = STRDUP(last_slash + 1);
        MFree(prog_name);
        prog_name = tmp;
    }

    // --------------------------------------------------
    // Resolve absolute path or search in PATH
    // --------------------------------------------------
    U8 abs_path_buf[512];
    BOOLEAN found = FALSE;
    FAT_LFN_ENTRY ent;

    if (ResolvePath(prog_copy, abs_path_buf, sizeof(abs_path_buf))) {
        if(FAT32_PATH_RESOLVE_ENTRY(prog_copy, &ent))
            found = TRUE;
        else 
            goto no_luck_above;
    } else {
        no_luck_above:
        PU8 path_val = GET_VAR((PU8)"PATH");

        if (path_val && *path_val) {
            PU8 path_dup = STRDUP(path_val);
            PU8 saveptr = NULL;  // <-- state pointer for reentrant tokenization
            PU8 tok = STRTOK_R(path_dup, ";", &saveptr);

            while (tok != NULL) {
                // Skip empty tokens
                if(*tok == '\0') {
                    tok = STRTOK_R(NULL, ";", &saveptr);
                    continue;
                }

                U8 candidate[512] = {0};
                STRNCPY(candidate, tok, sizeof(candidate) - 1);

                // Append slash if missing
                U32 len = STRLEN(candidate);
                if(len > 0 && candidate[len - 1] != '/' && candidate[len - 1] != '\\') {
                    STRNCAT(candidate, "/", sizeof(candidate) - STRLEN(candidate) - 1);
                }

                STRNCAT(candidate, prog_name, sizeof(candidate) - STRLEN(candidate) - 1);

                if (ResolvePath(candidate, abs_path_buf, sizeof(abs_path_buf))) {
                    if(FAT32_PATH_RESOLVE_ENTRY(abs_path_buf, &ent)) {
                        found = TRUE;
                        break;
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

    PU8 abs_path = abs_path_buf;
    MFree(prog_copy);

    // --------------------------------------------------
    // Verify file exists
    // --------------------------------------------------
    if (!FAT32_PATH_RESOLVE_ENTRY(abs_path, &ent)) {
        PUTS("\nError: File not found.\n");
        MFree(prog_name);
        return FALSE;
    }

    // --------------------------------------------------
    // Load file contents
    // --------------------------------------------------
    U32 sz = 0;
    VOIDPTR data = FAT32_READ_FILE_CONTENTS(&sz, &ent.entry);
    if (!data) {
        MFree(prog_name);
        return FALSE;
    }

    U32 pid = PROC_GETPID();

    // --------------------------------------------------
    // Parse arguments
    // --------------------------------------------------
    #define MAX_ARGS 12
    PU8* argv = MAlloc(sizeof(PU8*) * MAX_ARGS);
    U32 argc = 0;
    argv[argc++] = STRDUP(abs_path); // argv[0]

    U8 arg_line[256];
    STRNCPY(arg_line, line, sizeof(arg_line) - 1);
    str_trim(arg_line);

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

    // --------------------------------------------------
    // Execute based on file type
    // --------------------------------------------------
    PU8 ext = STRRCHR(abs_path, '.');
    if (ext) ext++; else ext = (PU8)"";

    BOOLEAN result = FALSE;
    if (STRICMP(ext, "BIN") == 0) {
        U32 res = START_PROCESS(abs_path, data, sz, TCB_STATE_ACTIVE, pid, argv, argc);
        if (res) PRINTNEWLINE();
        result = res != 0;
    } else if (STRICMP(ext, "SH") == 0) {
        // TODO: Crashes!
        // result = RUN_BATSH_SCRIPT(abs_path, argc, argv);
        // for(U32 i = 0; i < argc; i++) {
        //     if(argv[i]) MFree(argv[i]);
        // }   
        // if(argv) MFree(argv);
        PUTS("\nThis would crash, not going to do that though!\n");
        result = TRUE;
    }

    MFree(prog_name);
    return result;
}
