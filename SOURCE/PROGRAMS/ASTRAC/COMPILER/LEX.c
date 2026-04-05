/*
 * ════════════════════════════════════════════════════════════════════════════
 *  COMPILER/LEX.c  —  AC Language Lexer
 *
 *  Reads the preprocessed AC source file (ctx->tmp_src) line by line using
 *  READ_LOGICAL_LINE.  Produces a COMP_TOK_ARRAY (heap-allocated).
 *
 *  Design notes:
 *   • All identifiers and keywords are normalized to UPPER CASE.
 *   • Integer literals: decimal, 0x hex, 0b binary.
 *   • Float literals:   digits '.' digits (e.g. 3.14).
 *   • String literals:  recognised between double-quotes; escape sequences
 *     are preserved as-is for the code generator to emit.
 *   • Comments (//, /* *\/) are stripped by the preprocessor; the lexer
 *     therefore does not handle them.
 * ════════════════════════════════════════════════════════════════════════════
 */

#include "COMPILER.h"
#include "../ASTRAC.h"

/* ── Character classification ─────────────────────────────────────────────── */
#define IS_ALPHA(c)   (((c)>='A'&&(c)<='Z')||((c)>='a'&&(c)<='z')||(c)=='_')
#define IS_DIGIT(c)   ((c)>='0'&&(c)<='9')
#define IS_HEXDIG(c)  (IS_DIGIT(c)||((c)>='A'&&(c)<='F')||((c)>='a'&&(c)<='f'))
#define IS_ALNUM(c)   (IS_ALPHA(c)||IS_DIGIT(c))

/* ── Dynamic token array helpers ──────────────────────────────────────────── */
#define TOK_INIT_CAP 128

static BOOL tok_push(PCOMP_TOK_ARRAY arr, PCOMP_TOK tok) {
    if (arr->len >= arr->cap) {
        U32 new_cap = arr->cap == 0 ? TOK_INIT_CAP : arr->cap * 2;
        PCOMP_TOK *new_buf = (PCOMP_TOK *)ReAlloc(arr->toks, new_cap * sizeof(PCOMP_TOK));
        if (!new_buf) return FALSE;
        arr->toks = new_buf;
        arr->cap  = new_cap;
    }
    arr->toks[arr->len++] = tok;
    return TRUE;
}

static PCOMP_TOK make_tok(COMP_TOK_TYPE type, U32 line, U32 col) {
    PCOMP_TOK t = (PCOMP_TOK)MAlloc(sizeof(COMP_TOK));
    if (!t) return NULLPTR;
    MEMZERO(t, sizeof(COMP_TOK));
    t->type = type;
    t->line = line;
    t->col  = col;
    return t;
}

/* ── Keyword table (all strings already UPPER CASE) ──────────────────────── */
typedef struct { PU8 word; COMP_TOK_TYPE type; } CKEYWORD;

static CONST CKEYWORD kw_table[] ATTRIB_RODATA = {
    { "U8",       CTOK_KW_U8       },
    { "I8",       CTOK_KW_I8       },
    { "U16",      CTOK_KW_U16      },
    { "I16",      CTOK_KW_I16      },
    { "U32",      CTOK_KW_U32      },
    { "I32",      CTOK_KW_I32      },
    { "F32",      CTOK_KW_F32      },
    { "PU8",      CTOK_KW_PU8      },
    { "PU16",     CTOK_KW_PU16     },
    { "PU32",     CTOK_KW_PU32     },
    { "PPU8",     CTOK_KW_PPU8     },
    { "PPU16",    CTOK_KW_PPU16    },
    { "PPU32",    CTOK_KW_PPU32    },
    { "PI8",      CTOK_KW_PI8      },
    { "PI16",     CTOK_KW_PI16     },
    { "PI32",     CTOK_KW_PI32     },
    { "PPI8",     CTOK_KW_PPI8     },
    { "PPI16",    CTOK_KW_PPI16    },
    { "PPI32",    CTOK_KW_PPI32    },
    { "U0",       CTOK_KW_U0       },
    { "BOOL",     CTOK_KW_BOOL     },
    { "TRUE",     CTOK_KW_TRUE     },
    { "FALSE",    CTOK_KW_FALSE    },
    { "NULLPTR",  CTOK_KW_NULLPTR  },
    { "VOIDPTR",  CTOK_KW_VOIDPTR  },
    { "IF",       CTOK_KW_IF       },
    { "ELSE",     CTOK_KW_ELSE     },
    { "FOR",      CTOK_KW_FOR      },
    { "WHILE",    CTOK_KW_WHILE    },
    { "DO",       CTOK_KW_DO       },
    { "RETURN",   CTOK_KW_RETURN   },
    { "BREAK",    CTOK_KW_BREAK    },
    { "CONTINUE", CTOK_KW_CONTINUE },
    { "SWITCH",   CTOK_KW_SWITCH   },
    { "CASE",     CTOK_KW_CASE     },
    { "DEFAULT",  CTOK_KW_DEFAULT  },
    { "GOTO",     CTOK_KW_GOTO     },
    { "STRUCT",   CTOK_KW_STRUCT   },
    { "UNION",    CTOK_KW_UNION    },
    { "ENUM",     CTOK_KW_ENUM     },
    { "SIZEOF",   CTOK_KW_SIZEOF   },
    { "ASM",      CTOK_KW_ASM      },
    { NULLPTR,    CTOK_ERROR       },
};

static COMP_TOK_TYPE ident_to_kw(PU8 upper) {
    for (U32 i = 0; kw_table[i].word; i++)
        if (STRCMP(upper, kw_table[i].word) == 0)
            return kw_table[i].type;
    return CTOK_IDENT;
}

/* ── Operator / punctuator dispatch table ─────────────────────────────────── */
/*  Ordered: longest match first (3-char then 2-char then 1-char).            */
typedef struct { PU8 sym; U8 len; COMP_TOK_TYPE type; } CSYM;

static CONST CSYM sym_table[] ATTRIB_RODATA = {
    /* 3-char */
    { "<<=", 3, CTOK_SHL_ASSIGN   },
    { ">>=", 3, CTOK_SHR_ASSIGN   },
    /* 2-char */
    { "++",  2, CTOK_PLUSPLUS     },
    { "--",  2, CTOK_MINUSMINUS   },
    { "==",  2, CTOK_EQ           },
    { "!=",  2, CTOK_NEQ          },
    { "<=",  2, CTOK_LE           },
    { ">=",  2, CTOK_GE           },
    { "&&",  2, CTOK_AND          },
    { "||",  2, CTOK_OR           },
    { "<<",  2, CTOK_SHL          },
    { ">>",  2, CTOK_SHR          },
    { "->",  2, CTOK_ARROW        },
    { "+=",  2, CTOK_PLUS_ASSIGN  },
    { "-=",  2, CTOK_MINUS_ASSIGN },
    { "*=",  2, CTOK_STAR_ASSIGN  },
    { "/=",  2, CTOK_SLASH_ASSIGN },
    { "%=",  2, CTOK_PCT_ASSIGN   },
    { "&=",  2, CTOK_AMP_ASSIGN   },
    { "|=",  2, CTOK_PIPE_ASSIGN  },
    { "^=",  2, CTOK_CARET_ASSIGN },
    /* 1-char */
    { "+",   1, CTOK_PLUS         },
    { "-",   1, CTOK_MINUS        },
    { "*",   1, CTOK_STAR         },
    { "/",   1, CTOK_SLASH        },
    { "%",   1, CTOK_PERCENT      },
    { "=",   1, CTOK_ASSIGN       },
    { "<",   1, CTOK_LT           },
    { ">",   1, CTOK_GT           },
    { "!",   1, CTOK_NOT          },
    { "&",   1, CTOK_AMP          },
    { "|",   1, CTOK_PIPE         },
    { "^",   1, CTOK_CARET        },
    { "~",   1, CTOK_TILDE        },
    { "?",   1, CTOK_QUESTION     },
    { ":",   1, CTOK_COLON        },
    { "(",   1, CTOK_LPAREN       },
    { ")",   1, CTOK_RPAREN       },
    { "{",   1, CTOK_LBRACE       },
    { "}",   1, CTOK_RBRACE       },
    { "[",   1, CTOK_LBRACKET     },
    { "]",   1, CTOK_RBRACKET     },
    { ";",   1, CTOK_SEMICOLON    },
    { ",",   1, CTOK_COMMA        },
    { ".",   1, CTOK_DOT          },
    { NULLPTR, 0, CTOK_ERROR      },
};

static BOOL try_operator(PU8 buf, U32 i, U32 len,
                         COMP_TOK_TYPE *out_type, U32 *out_len) {
    for (U32 k = 0; sym_table[k].sym; k++) {
        U32 sl = sym_table[k].len;
        if (i + sl > len) continue;
        if (STRNCMP(buf + i, sym_table[k].sym, sl) == 0) {
            *out_type = sym_table[k].type;
            *out_len  = sl;
            return TRUE;
        }
    }
    return FALSE;
}

/* ── Escape sequence decoder ──────────────────────────────────────────────── */
static U8 decode_escape(U8 c) {
    switch (c) {
        case 'n':  return '\n';
        case 't':  return '\t';
        case 'r':  return '\r';
        case '0':  return '\0';
        case '\\': return '\\';
        case '"':  return '"';
        case '\'': return '\'';
        default:   return c;
    }
}

/* ── Lex a single source file ─────────────────────────────────────────────── */
static BOOL lex_file(PCOMP_TOK_ARRAY arr, PU8 path, PCOMP_CTX ctx) {
    FILE *file = FOPEN(path, MODE_R | MODE_FAT32);
    if (!file) {
        printf("[COMP LEX] Cannot open '%s'\n", path);
        ctx->errors++;
        return FALSE;
    }

    U8  linebuf[BUF_SZ];
    U8  tokbuf[BUF_SZ];
    U32 lineno = 0;

    while (READ_LOGICAL_LINE(file, linebuf, BUF_SZ)) {
        lineno++;
        U32 len = STRLEN(linebuf);
        U32 i   = 0;
        U32 col = 1;

        while (i < len) {
            U8 c = linebuf[i];

            /* ── Skip whitespace ── */
            if (IS_SPACE(c)) { i++; col++; continue; }

            /* ── String literal "..." ─────────────────────────── */
            if (c == '"') {
                U32 start_col = col;
                U32 tlen = 0;
                i++; col++;
                while (i < len && tlen + 2 < BUF_SZ) {
                    U8 cc = linebuf[i];
                    if (cc == '"') { i++; col++; break; }
                    if (cc == '\\' && i + 1 < len) {
                        tokbuf[tlen++] = decode_escape(linebuf[i + 1]);
                        i += 2; col += 2;
                    } else {
                        tokbuf[tlen++] = cc;
                        i++; col++;
                    }
                }
                tokbuf[tlen] = '\0';
                PCOMP_TOK t = make_tok(CTOK_STR_LIT, lineno, start_col);
                if (!t) goto oom;
                t->txt = STRDUP(tokbuf);
                if (!tok_push(arr, t)) goto oom;
                continue;
            }

            /* ── Char literal 'x' ─────────────────────────────── */
            if (c == '\'') {
                U32 start_col = col;
                U32 char_val  = 0;
                i++; col++;
                if (i < len) {
                    if (linebuf[i] == '\\' && i + 1 < len) {
                        char_val = decode_escape(linebuf[i + 1]);
                        i += 2; col += 2;
                    } else {
                        char_val = (U32)linebuf[i];
                        i++; col++;
                    }
                }
                if (i < len && linebuf[i] == '\'') { i++; col++; }
                tokbuf[0] = (U8)char_val; tokbuf[1] = '\0';
                PCOMP_TOK t = make_tok(CTOK_CHAR_LIT, lineno, start_col);
                if (!t) goto oom;
                t->ival = char_val;
                t->txt  = STRDUP(tokbuf);
                if (!tok_push(arr, t)) goto oom;
                continue;
            }

            /* ── Numeric literal ──────────────────────────────── */
            if (IS_DIGIT(c) || (c == '.' && i + 1 < len && IS_DIGIT(linebuf[i + 1]))) {
                U32 start_col = col;
                U32 tlen = 0;
                BOOL is_float = FALSE;

                /* Collect all digits, letters, dots for '0x', '0b', float */
                while (i < len && tlen + 1 < BUF_SZ) {
                    U8 cc = linebuf[i];
                    if (IS_ALNUM(cc) || cc == '.') {
                        if (cc == '.') is_float = TRUE;
                        tokbuf[tlen++] = cc;
                        i++; col++;
                    } else break;
                }
                tokbuf[tlen] = '\0';

                PCOMP_TOK t;
                if (is_float) {
                    t = make_tok(CTOK_FLOAT_LIT, lineno, start_col);
                    if (!t) goto oom;
                    t->fval = ATOF(tokbuf);
                } else {
                    t = make_tok(CTOK_INT_LIT, lineno, start_col);
                    if (!t) goto oom;
                    /* Detect base */
                    if (tokbuf[0] == '0' && (tokbuf[1]=='x'||tokbuf[1]=='X'))
                        t->ival = ATOI_HEX(tokbuf + 2);
                    else if (tokbuf[0] == '0' && (tokbuf[1]=='b'||tokbuf[1]=='B'))
                        t->ival = ATOI_BIN(tokbuf + 2);
                    else
                        t->ival = ATOI(tokbuf);
                }
                t->txt = STRDUP(tokbuf);
                if (!tok_push(arr, t)) goto oom;
                continue;
            }

            /* ── Identifier or keyword ────────────────────────── */
            if (IS_ALPHA(c)) {
                U32 start_col = col;
                U32 tlen = 0;
                while (i < len && IS_ALNUM(linebuf[i]) && tlen + 1 < BUF_SZ) {
                    tokbuf[tlen++] = linebuf[i];
                    i++; col++;
                }
                tokbuf[tlen] = '\0';
                /* Normalize to upper-case */
                STR_TOUPPER((U8*)tokbuf);
                COMP_TOK_TYPE ktype = ident_to_kw(tokbuf);
                PCOMP_TOK t = make_tok(ktype, lineno, start_col);
                if (!t) goto oom;
                t->txt = STRDUP(tokbuf);
                if (!tok_push(arr, t)) goto oom;
                continue;
            }

            /* ── Operator / punctuator ────────────────────────── */
            {
                COMP_TOK_TYPE op_type = CTOK_ERROR;
                U32 op_len = 0;
                if (try_operator(linebuf, i, len, &op_type, &op_len)) {
                    U8 symtxt[4];
                    U32 k;
                    for (k = 0; k < op_len && k < 3; k++) symtxt[k] = linebuf[i + k];
                    symtxt[k] = '\0';
                    PCOMP_TOK t = make_tok(op_type, lineno, col);
                    if (!t) goto oom;
                    t->txt = STRDUP(symtxt);
                    if (!tok_push(arr, t)) goto oom;
                    i   += op_len;
                    col += op_len;
                    continue;
                }
                /* Unknown character — report and skip */
                printf("[COMP LEX] %s:%u:%u: unexpected character '%c' (0x%02X)\n",
                       path, lineno, col, c, (U32)c);
                ctx->errors++;
                i++; col++;
            }
        }
    }

    FCLOSE(file);
    return TRUE;

oom:
    printf("[COMP LEX] Out of memory\n");
    ctx->errors++;
    FCLOSE(file);
    return FALSE;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  PUBLIC API
 * ════════════════════════════════════════════════════════════════════════════ */

PCOMP_TOK_ARRAY COMP_LEX(PCOMP_CTX ctx) {
    PCOMP_TOK_ARRAY arr = (PCOMP_TOK_ARRAY)MAlloc(sizeof(COMP_TOK_ARRAY));
    if (!arr) return NULLPTR;
    MEMZERO(arr, sizeof(COMP_TOK_ARRAY));

    if (!lex_file(arr, ctx->tmp_src, ctx)) {
        DESTROY_COMP_TOK_ARRAY(arr);
        return NULLPTR;
    }

    /* Append EOF sentinel */
    PCOMP_TOK eof = make_tok(CTOK_EOF, 0, 0);
    if (!eof || !tok_push(arr, eof)) {
        DESTROY_COMP_TOK_ARRAY(arr);
        return NULLPTR;
    }

    DEBUGM_PRINTF("[COMP LEX] %u tokens from '%s'\n", arr->len - 1U, ctx->tmp_src);
    return arr;
}

VOID DESTROY_COMP_TOK_ARRAY(PCOMP_TOK_ARRAY arr) {
    if (!arr) return;
    if (arr->toks) {
        for (U32 i = 0; i < arr->len; i++) {
            PCOMP_TOK t = arr->toks[i];
            if (t) {
                if (t->txt) MFree(t->txt);
                MFree(t);
            }
        }
        MFree(arr->toks);
    }
    MFree(arr);
}
