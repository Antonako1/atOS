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
    PU8 val = GET_VAR_VALUE(name);  // Use GET_VAR to find variable once
    if (!val || !*val) return FALSE;

    if (
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

BOOLEAN SET_INST_VAR(PU8 name, PU8 value, BATSH_INSTANCE *inst) {
    if(!inst) return FALSE;
    if(inst->shell_var_count + 1 > MAX_VARS) return FALSE;
    STRNCPY(inst->shell_vars[inst->shell_var_count].name, name, MAX_VAR_NAME);
    STRNCPY(inst->shell_vars[inst->shell_var_count++].value, value, MAX_VAR_VALUE);
    return TRUE;
}
PU8 GET_INST_VAR(PU8 name, BATSH_INSTANCE *inst) {
    for(U32 i = 0; i < inst->shell_var_count; i++) {
        if(STRICMP(name, inst->shell_vars[i].name) == 0) return inst->shell_vars[i].value;
    }
    return NULL;
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
    TOK_AND,         // AND
    TOK_OR,          // OR
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

typedef struct BATSH_COMMAND {
    BATSH_TOKEN_TYPE type;

    char **argv;
    int argc;

    // For compound commands (pipe, if, loop)
    struct BATSH_COMMAND *left;
    struct BATSH_COMMAND *right;

    // Optional for future
    struct BATSH_COMMAND *next; // for sequential execution

    struct pos;
} BATSH_COMMAND;


BATSH_COMMAND *parse_command(BATSH_TOKEN **tokens, U32 *pos, U32 token_len);
BATSH_COMMAND *parse_simple(BATSH_TOKEN **tokens, U32 *pos, U32 token_len);
BATSH_COMMAND *parse_if(BATSH_TOKEN **tokens, U32 *pos, U32 token_len);
BATSH_COMMAND *parse_loop(BATSH_TOKEN **tokens, U32 *pos, U32 token_len);
BATSH_COMMAND *parse_sequence(BATSH_TOKEN **tokens, U32 *pos, U32 token_len);
BATSH_COMMAND *parse_arith_expr(BATSH_TOKEN **tokens, U32 *pos, U32 token_len);

BATSH_TOKEN *peek_token(BATSH_TOKEN **tokens, U32 pos, U32 token_len) {
    return pos < token_len ? tokens[pos] : NULL;
}

BATSH_TOKEN *consume_token(BATSH_TOKEN **tokens, U32 *pos, U32 token_len) {
    return (*pos < token_len) ? tokens[(*pos)++] : NULL;
}

BOOLEAN match_token(BATSH_TOKEN **tokens, U32 *pos, U32 token_len, BATSH_TOKEN_TYPE type) {
    BATSH_TOKEN *tok = peek_token(tokens, *pos, token_len);
    if (!tok) return FALSE;
    if (tok->type == type) {
        (*pos)++;
        return TRUE;
    }
    return FALSE;
}

BATSH_COMMAND *parse_sequence(BATSH_TOKEN **tokens, U32 *pos, U32 token_len) {
    BATSH_COMMAND *head = parse_command(tokens, pos, token_len);
    if (!head) return NULL;

    BATSH_COMMAND *current = head;
    while (*pos < token_len) {
        if (match_token(tokens, pos, token_len, TOK_SEMI)) {
            BATSH_COMMAND *next_cmd = parse_command(tokens, pos, token_len);
            if (!next_cmd) break;
            current->next = next_cmd;
            current = next_cmd;
        } else {
            break;
        }
    }
    return head;
}

BATSH_COMMAND *parse_simple(BATSH_TOKEN **tokens, U32 *pos, U32 token_len) {
    BATSH_TOKEN *tok = peek_token(tokens, *pos, token_len);
    if (!tok) return NULL;

    BATSH_COMMAND *cmd = MAlloc(sizeof(BATSH_COMMAND));
    MEMZERO(cmd, sizeof(BATSH_COMMAND));
    
    // Determine type
    if (tok->type == TOK_CMD) {
        cmd->type = TOK_CMD;  // built-in
    } else if (tok->type == TOK_WORD) {
        cmd->type = TOK_WORD; // external/script
    } else if (tok->type == TOK_VAR) {
        cmd->type = TOK_VAR;  // variable expansion
    }

    // argv collection (all three handled the same way here)
    cmd->argv = MAlloc(sizeof(char*) * (token_len - *pos + 1));
    cmd->argc = 0;
    cmd->argv[cmd->argc++] = STRDUP(tok->text);
    (*pos)++;

    while (*pos < token_len) {
        BATSH_TOKEN *t = peek_token(tokens, *pos, token_len);
        if (!t) break;
        if (t->type == TOK_WORD || t->type == TOK_VAR || t->type == TOK_STRING) {
            cmd->argv[cmd->argc++] = STRDUP(t->text);
            (*pos)++;
        } else {
            break;
        }
    }
    cmd->argv[cmd->argc] = NULL;
    return cmd;
}

BATSH_COMMAND *parse_if(BATSH_TOKEN **tokens, U32 *pos, U32 token_len) {
    if (!match_token(tokens, pos, token_len, TOK_IF)) return NULL;

    BATSH_COMMAND *cmd = MAlloc(sizeof(BATSH_COMMAND));
    MEMZERO(cmd, sizeof(BATSH_COMMAND));
    cmd->type = TOK_IF;

    // Condition expression until THEN
    U32 cond_pos = *pos;
    while (*pos < token_len && !match_token(tokens, pos, token_len, TOK_THEN)) {
        (*pos)++;
    }
    cmd->left = parse_arith_expr(tokens, &cond_pos, *pos);

    // Body until ELSE or FI
    U32 body_pos = *pos;
    while (*pos < token_len &&
           peek_token(tokens, *pos, token_len)->type != TOK_ELSE &&
           peek_token(tokens, *pos, token_len)->type != TOK_FI) {
        (*pos)++;
    }
    cmd->right = parse_sequence(tokens, &body_pos, *pos);

    // Optional ELSE
    if (peek_token(tokens, *pos, token_len) &&
        peek_token(tokens, *pos, token_len)->type == TOK_ELSE) {
        (*pos)++;
        U32 else_pos = *pos;
        while (*pos < token_len && peek_token(tokens, *pos, token_len)->type != TOK_FI) {
            (*pos)++;
        }
        cmd->next = parse_sequence(tokens, &else_pos, *pos);
    }

    // Consume FI
    match_token(tokens, pos, token_len, TOK_FI);
    return cmd;
}

BATSH_COMMAND *parse_loop(BATSH_TOKEN **tokens, U32 *pos, U32 token_len) {
    if (!match_token(tokens, pos, token_len, TOK_LOOP)) return NULL;

    BATSH_COMMAND *cmd = MAlloc(sizeof(BATSH_COMMAND));
    MEMZERO(cmd, sizeof(BATSH_COMMAND));
    cmd->type = TOK_LOOP;

    // Loop condition or count
    U32 cond_pos = *pos;
    while (*pos < token_len && peek_token(tokens, *pos, token_len)->type != TOK_END) {
        (*pos)++;
    }
    cmd->left = parse_arith_expr(tokens, &cond_pos, *pos);

    // Consume END
    match_token(tokens, pos, token_len, TOK_END);
    return cmd;
}

BATSH_COMMAND *parse_arith_expr(BATSH_TOKEN **tokens, U32 *pos, U32 token_len) {
    // For now, we collect TOK_WORD, TOK_VAR, TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV, TOK_LPAREN, TOK_RPAREN
    BATSH_COMMAND *expr = MAlloc(sizeof(BATSH_COMMAND));
    MEMZERO(expr, sizeof(BATSH_COMMAND));
    expr->type = TOK_UNKNOWN;

    expr->argv = MAlloc(sizeof(char*) * (token_len - *pos + 1));
    expr->argc = 0;

    while (*pos < token_len) {
        BATSH_TOKEN *tok = peek_token(tokens, *pos, token_len);
        if (!tok) break;

        if (tok->type == TOK_WORD || tok->type == TOK_VAR ||
            tok->type == TOK_PLUS || tok->type == TOK_MINUS ||
            tok->type == TOK_MUL || tok->type == TOK_DIV ||
            tok->type == TOK_LPAREN || tok->type == TOK_RPAREN) {
            expr->argv[expr->argc++] = STRDUP(tok->text);
            (*pos)++;
        } else {
            break;
        }
    }
    expr->argv[expr->argc] = NULL;
    return expr;
}

BATSH_COMMAND *parse_pipeline(BATSH_TOKEN **tokens, U32 *pos, U32 token_len) {
    // TODO: implement pipeline parsing
    // Should detect TOK_PIPE and set left/right commands
    return NULL;
}

BATSH_COMMAND *parse_redirection(BATSH_TOKEN **tokens, U32 *pos, U32 token_len) {
    // TODO: implement redirection parsing
    // Should detect TOK_REDIR_OUT, TOK_REDIR_IN and set target file
    return NULL;
}

// ==========================
// Updated parse_command entry point
// ==========================
BATSH_COMMAND *parse_command(BATSH_TOKEN **tokens, U32 *pos, U32 token_len) {
    BATSH_TOKEN *tok = peek_token(tokens, *pos, token_len);
    if (!tok) return NULL;

    switch(tok->type) {
        case TOK_CMD:
        case TOK_WORD:  // external command or unknown word
        case TOK_VAR:   // variable expansion as command
            return parse_simple(tokens, pos, token_len);
        case TOK_IF:
            return parse_if(tokens, pos, token_len);
        case TOK_LOOP:
            return parse_loop(tokens, pos, token_len);
        case TOK_PIPE:
            return parse_pipeline(tokens, pos, token_len);
        case TOK_REDIR_OUT:
        case TOK_REDIR_IN:
            return parse_redirection(tokens, pos, token_len);
        default:
            return NULL;
    }
}


BATSH_COMMAND *parse_tokens(BATSH_TOKEN **tokens, U32 token_len) {
    U32 pos = 0;
    return parse_sequence(tokens, &pos, token_len);
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

void execute_command(BATSH_COMMAND *cmd) {
    if (!cmd) return;

    switch (cmd->type) {
        case TOK_CMD: {
            for (U32 i = 0; i < shell_command_count; i++) {
                if (STRICMP(cmd->argv[0], shell_commands[i].name) == 0) {
                    // Reconstruct the line from argv
                    U32 line_len = 0;
                    for (int j = 0; j < cmd->argc; j++)
                        line_len += STRLEN(cmd->argv[j]) + 1;

                    U8 *line_buf = MAlloc(line_len);
                    if (!line_buf) return;

                    line_buf[0] = '\0';
                    for (int j = 0; j < cmd->argc; j++) {
                        STRCAT(line_buf, cmd->argv[j]);
                        if (j < cmd->argc - 1)
                            STRCAT(line_buf, " ");
                    }

                    shell_commands[i].handler(line_buf);
                    MFree(line_buf);
                    break;
                }
            }
            break;
        }
        case TOK_WORD:
            // TODO: execute external command or script
            break;
        case TOK_VAR:
            // TODO: resolve variable, then execute
            break;
        case TOK_IF:
            // TODO: evaluate condition, execute cmd->right or cmd->next
            break;
        case TOK_LOOP:
            // TODO: evaluate loop condition, execute cmd->right in loop
            break;
    }

    // Execute sequential commands
    if (cmd->next) execute_command(cmd->next);
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

// ===================================================
// BATSH Parser
// Takes either full file or line from cmd line
// Tokenizes, parses and runs the input
// ===================================================
BOOLEAN PARSE_BATSH_INPUT(PU8 input, BATSH_INSTANCE *inst) {
    if (!input || !*input) return TRUE;
    U32 len = STRLEN(input);
    U8 buf[CUR_LINE_MAX_LENGTH] = {0};
    U32 ptr = 0;
    BATSH_TOKEN **tokens = NULL;
    U32 token_len = 0;

    struct pos pos = { .line = 0, .row = 0 };
    KEYWORD *rem = GET_KEYWORD_S("rem");

    for (U32 i = 0; i < len; i++) {
        CHAR c = input[i];
        if (c == '\n' || c == '\r') {
            pos.row++;
            pos.line = 0;
        } else {
            pos.line++;
        }

        // Keywords and full words
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (ptr > 0) {
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
            continue;
        }
        else if(c == '#') {
            for(;i < len; i++) {
                CHAR c2 = input[i];
                if(c2 == '\n' || c2 == '\r') {
                    break;
                }
            }
            continue;
        }
        else if(c == '@') {
            i++; // move past '@'
            U32 start = i;
            if(input[i] == '{') {
                i++; // skip '{'
                start = i;
                while(i < len && input[i] != '}') i++;
            } else {
                while(i < len && ISALNUM(input[i])) i++;
            }
            U32 var_len = i - start;
            if(var_len >= CUR_LINE_MAX_LENGTH) var_len = CUR_LINE_MAX_LENGTH - 1;
            MEMCPY(buf, &input[start], var_len);
            buf[var_len] = '\0';

            KEYWORD var_kw = { buf, TOK_VAR };
            if(!ADD_TOKEN(&tokens, &token_len, buf, &var_kw)) goto error;

            if(input[i] == '}') i++; // skip closing brace
            continue;
        }
        else if (
            c == '|' ||
            c == '+' ||
            c == '-' ||
            c == '*' ||
            c == '/' ||
            c == '(' ||
            c == ')' ||
            c == '}' ||
            c == '{' ||
            c == '=' ||
            c == '@' ||
            c == '>' ||
            c == '<' ||
            c == '{' ||
            c == '}' ||
            c == ';' ||
            c == '\\' 
        ) 
        {
            buf[ptr] = c;
            buf[++ptr] = '\0';
            KEYWORD *kw = GET_KEYWORD_C(c);
            if(!kw) {
                PRINT_TOKENIZER_ERR(buf, NULLPTR, pos);
                goto error;
            }
            if(!ADD_TOKEN(&tokens, &token_len, buf, kw)) {
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

    // Parse the tokens
    BATSH_COMMAND *root_cmd = parse_tokens(tokens, token_len);
    if (!root_cmd) {
        PUTS("Parsing failed\n");
        goto error;
    }

    BATSH_COMMAND *cmd = root_cmd;
    DEBUG_NL();
    execute_command(cmd);
    while (cmd) {
        DEBUG_PUTS("Command type: ");
        DEBUG_HEX32(cmd->type);
        DEBUG_NL();

        for (int i = 0; i < cmd->argc; i++) {
            DEBUG_PUTS("Arg: ");
            DEBUG_PUTS_LN(cmd->argv[i]);
        }
        cmd = cmd->next;
    }

    U8 res = TRUE;
    goto free;
error:
    res = FALSE;
free:
    for (U32 i = 0; i < token_len; i++) {
        // DEBUG_HEX32(tokens[i]->type);
        // DEBUG_PUTS(" ");
        // DEBUG_PUTS_LN(tokens[i]->text);
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
BOOLEAN RUN_BATSH_SCRIPT(PU8 path) {
    PU8 dot = STRRCHR(path, '.');
    if (!dot || STRNICMP(dot, ".SH", 3) != 0)
    return FALSE;
    BATSH_INSTANCE *inst = CREATE_BATSH_INSTANCE(path);
    if(!inst) return FALSE;
    BOOLEAN res = RUN_BATSH_FILE(inst);
    DESTROY_BATSH_INSTANCE(inst);
    return res;
}

// ===================================================
// BATSH Instance initializer
// ===================================================
BATSH_INSTANCE *CREATE_BATSH_INSTANCE(PU8 path) {
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
    inst->arg_count = 0;
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

    // Copy the program portion (may be relative)
    PU8 prog_copy = MAlloc(prog_len + 1);
    STRNCPY(prog_copy, tmp_line, prog_len);
    prog_copy[prog_len] = 0;

    // Extract only the program name (last path component)
    PU8 prog_name = STRDUP(prog_copy);
    PU8 last_slash = STRRCHR(prog_name, '/');
    if (last_slash) {
        PU8 tmp = STRDUP(last_slash + 1);
        MFree(prog_name);
        prog_name = tmp;
    }


    // --------------------------------------------------
    // Resolve path (use a separate buffer)
    // --------------------------------------------------
    U8 abs_path_buf[256];
    if (!ResolvePath(prog_copy, abs_path_buf, sizeof(abs_path_buf))) {
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
    FAT_LFN_ENTRY ent;
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
    // Parse arguments (from original input line)
    // --------------------------------------------------
    #define MAX_ARGS 12
    PU8* argv = MAlloc(sizeof(PU8*) * MAX_ARGS);
    U32 argc = 0;

    argv[argc++] = STRDUP(abs_path); // argv[0]

    // Use a preserved copy of the original line
    U8 arg_line[256];
    STRNCPY(arg_line, line, sizeof(arg_line) - 1);
    str_trim(arg_line);

    PU8 first_space_copy = STRCHR(arg_line, ' ');
    PU8 args_start = first_space_copy ? first_space_copy + 1 : NULL;

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
    // Determine file type and execute
    // --------------------------------------------------
    PU8 ext = STRRCHR(abs_path, '.');
    if (ext) ext++; else ext = (PU8)"";

    if (STRICMP(ext, "BIN") == 0) {
        // Execute binary
        U32 res = START_PROCESS(abs_path, data, sz, TCB_STATE_ACTIVE, pid, argv, argc);
        if (res) PRINTNEWLINE();
        return res != 0;
    } else if (STRICMP(ext, "SH") == 0) {
        // TODO: run as shell script
        return FALSE;
    } else {
        return FALSE;
    }
}


