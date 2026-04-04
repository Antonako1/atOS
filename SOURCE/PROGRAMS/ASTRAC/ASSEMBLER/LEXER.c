#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>
#include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONICS.h>

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  KEYWORD TABLES
 * ════════════════════════════════════════════════════════════════════════════
 */

STATIC CONST KEYWORD registers[] ATTRIB_DATA = {
    /* General Purpose 32-bit */
    { "eax", REG_EAX }, { "ebx", REG_EBX }, { "ecx", REG_ECX }, { "edx", REG_EDX },
    { "esi", REG_ESI }, { "edi", REG_EDI }, { "ebp", REG_EBP }, { "esp", REG_ESP },

    /* General Purpose 16-bit */
    { "ax",  REG_AX  }, { "bx",  REG_BX  }, { "cx",  REG_CX  }, { "dx",  REG_DX  },
    { "si",  REG_SI  }, { "di",  REG_DI  }, { "bp",  REG_BP  }, { "sp",  REG_SP  },

    /* 8-bit */
    { "ah",  REG_AH  }, { "al",  REG_AL  }, { "bh",  REG_BH  }, { "bl",  REG_BL  },
    { "ch",  REG_CH  }, { "cl",  REG_CL  }, { "dh",  REG_DH  }, { "dl",  REG_DL  },

    /* Segment */
    { "cs",  REG_CS  }, { "ds",  REG_DS  }, { "es",  REG_ES  },
    { "fs",  REG_FS  }, { "gs",  REG_GS  }, { "ss",  REG_SS  },

    /* Control */
    { "cr0", REG_CR0 }, { "cr2", REG_CR2 }, { "cr3", REG_CR3 }, { "cr4", REG_CR4 },

    /* Debug */
    { "dr0", REG_DR0 }, { "dr1", REG_DR1 }, { "dr2", REG_DR2 }, { "dr3", REG_DR3 },
    { "dr6", REG_DR6 }, { "dr7", REG_DR7 },

    /* x87 FPU */
    { "st0", REG_ST0 }, { "st1", REG_ST1 }, { "st2", REG_ST2 }, { "st3", REG_ST3 },
    { "st4", REG_ST4 }, { "st5", REG_ST5 }, { "st6", REG_ST6 }, { "st7", REG_ST7 },

    /* MMX */
    { "mm0", REG_MM0 }, { "mm1", REG_MM1 }, { "mm2", REG_MM2 }, { "mm3", REG_MM3 },
    { "mm4", REG_MM4 }, { "mm5", REG_MM5 }, { "mm6", REG_MM6 }, { "mm7", REG_MM7 },

    /* SSE */
    { "xmm0", REG_XMM0 }, { "xmm1", REG_XMM1 }, { "xmm2", REG_XMM2 }, { "xmm3", REG_XMM3 },
    { "xmm4", REG_XMM4 }, { "xmm5", REG_XMM5 }, { "xmm6", REG_XMM6 }, { "xmm7", REG_XMM7 },

    { NULL, REG_NONE }
};

STATIC CONST KEYWORD directives[] ATTRIB_DATA = {
    { "data",   DIR_DATA          },
    { "rodata", DIR_RODATA        },
    { "code",   DIR_CODE          },
    { "use32",  DIR_CODE_TYPE_32  },
    { "use16",  DIR_CODE_TYPE_16  },
    { "org",    DIR_ORG           },
    { "times",  DIR_TIMES         },
    { NULL, 0 }
};

STATIC CONST KEYWORD symbols[] ATTRIB_DATA = {
    { ",",  SYM_COMMA     },
    { ":",  SYM_COLON     },
    { ";",  SYM_SEMICOLON },
    { "[",  SYM_LBRACKET  },
    { "]",  SYM_RBRACKET  },
    { "(",  SYM_LPAREN    },
    { ")",  SYM_RPAREN    },
    { "+",  SYM_PLUS      },
    { "-",  SYM_MINUS     },
    { "*",  SYM_ASTERISK  },
    { "/",  SYM_SLASH     },
    { "%",  SYM_PERCENT   },
    { "&",  SYM_AND       },
    { "|",  SYM_OR        },
    { "^",  SYM_XOR       },
    { "~",  SYM_TILDE     },
    { "<<", SYM_SHL       },
    { ">>", SYM_SHR       },
    { "=",  SYM_EQUALS    },
    { ".",  SYM_DOT       },
    { "@",  SYM_AT        },
    { "$",  SYM_DOLLAR    },
    { "$$", SYM_DOLLAR_DOLLAR },
    { NULL, SYM_NONE      }
};

STATIC CONST KEYWORD variable_types[] ATTRIB_DATA = {
    { "DB",    TYPE_BYTE   },
    { "DW",    TYPE_WORD   },
    { "DD",    TYPE_DWORD  },
    { "BYTE",  TYPE_BYTE   },
    { "WORD",  TYPE_WORD   },
    { "DWORD", TYPE_DWORD  },
    { "REAL4", TYPE_FLOAT  },
    { "PTR",   TYPE_PTR    },
    { NULL, TYPE_NONE }
};

const KEYWORD *get_registers()      { return registers;      }
const KEYWORD *get_directives()     { return directives;     }
const KEYWORD *get_symbols()        { return symbols;        }
const KEYWORD *get_variable_types() { return variable_types; }


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  TOKEN HELPER
 * ════════════════════════════════════════════════════════════════════════════
 */
STATIC BOOL ADD_TOKEN(ASM_TOK_ARRAY *arr, ASM_TOKEN_TYPE type,
                      PU8 txt, U32 uval, U32 line, U32 col) {
    if (arr->len >= MAX_TOKENS) return FALSE;

    PASM_TOK tok = MAlloc(sizeof(ASM_TOK));
    if (!tok) return FALSE;

    MEMSET(tok, 0, sizeof(ASM_TOK));
    tok->type    = type;
    tok->txt     = STRDUP(txt);
    tok->num_val = uval;
    tok->line    = line;
    tok->col     = col;

    arr->toks[arr->len++] = tok;
    return TRUE;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  LOOKUP HELPERS
 * ════════════════════════════════════════════════════════════════════════════
 */
STATIC CONST KEYWORD *lookup_keyword(CONST KEYWORD *arr, PU8 s) {
    if (!s || !*s) return NULL;
    for (U32 i = 0; arr[i].value; i++) {
        if (STRICMP(s, arr[i].value) == 0) return &arr[i];
    }
    return NULL;
}


/*
 * ── Number parser ───────────────────────────────────────────────────────────
 *  Parses dec, hex (0x), bin (0b), and returns the value via *out_val.
 *  Returns TRUE if `tokbuf` is a valid number literal.
 */
STATIC BOOL parse_number(PU8 tokbuf, U32 *out_val) {
    PU8 p = tokbuf;
    BOOL neg = FALSE;

    if (*p == '+') p++;
    else if (*p == '-') { neg = TRUE; p++; }

    if (!*p) return FALSE;

    U32 val = 0;

    if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
        /* Hex */
        p += 2;
        if (!*p) return FALSE;
        for (; *p; p++) {
            U8 c = *p;
            U32 d;
            if (c >= '0' && c <= '9')      d = c - '0';
            else if (c >= 'a' && c <= 'f')  d = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')  d = c - 'A' + 10;
            else return FALSE;
            val = (val << 4) | d;
        }
    } else if (*p == '0' && (p[1] == 'b' || p[1] == 'B')) {
        /* Binary */
        p += 2;
        if (!*p) return FALSE;
        for (; *p; p++) {
            if (*p != '0' && *p != '1') return FALSE;
            val = (val << 1) | (*p - '0');
        }
    } else {
        /* Decimal */
        if (!IS_DIGIT(*p)) return FALSE;
        for (; *p; p++) {
            if (!IS_DIGIT(*p)) return FALSE;
            val = val * 10 + (*p - '0');
        }
    }

    if (neg) val = (U32)(-(S32)val);
    *out_val = val;
    return TRUE;
}


/*
 * ── Match a symbol at position in the line buffer ───────────────────────────
 *  Tries 2-char symbols first (<<, >>), falls back to 1-char.
 *  On match sets *sym_enum, *sym_len (1 or 2) and returns TRUE.
 */
STATIC BOOL try_symbol_at(CONST U8 *buf, U32 pos, U32 buflen,
                          U32 *sym_enum, U32 *sym_len) {
    /* 2-char symbols first */
    if (pos + 1 < buflen) {
        U8 two[3] = { buf[pos], buf[pos + 1], '\0' };
        for (U32 i = 0; symbols[i].value; i++) {
            if (symbols[i].value[0] && symbols[i].value[1]
                && STRCMP(two, symbols[i].value) == 0) {
                *sym_enum = symbols[i].enum_val;
                *sym_len  = 2;
                return TRUE;
            }
        }
    }

    /* 1-char */
    U8 one[2] = { buf[pos], '\0' };
    for (U32 i = 0; symbols[i].value; i++) {
        if (symbols[i].value[0] == buf[pos] && symbols[i].value[1] == '\0') {
            *sym_enum = symbols[i].enum_val;
            *sym_len  = 1;
            return TRUE;
        }
    }
    return FALSE;
}


/*
 * ── Classify an accumulated text token ──────────────────────────────────────
 *  Checks in order: directive, register, variable type, mnemonic, number,
 *  then falls back to generic identifier.
 */
STATIC BOOL emit_token_from_buf(ASM_TOK_ARRAY *arr, PU8 tokbuf, U32 line, U32 col) {
    if (!tokbuf || !*tokbuf) return TRUE;
    DEBUGM_PRINTF("%s\n", tokbuf);

    /* Directive */
    const KEYWORD *kw;

    kw = lookup_keyword(directives, tokbuf);
    if (kw) return ADD_TOKEN(arr, TOK_DIRECTIVE, tokbuf, kw->enum_val, line, col);

    /* Register */
    kw = lookup_keyword(registers, tokbuf);
    if (kw) return ADD_TOKEN(arr, TOK_REGISTER, tokbuf, kw->enum_val, line, col);

    /* Variable type keyword (DB, DW, DD, BYTE, WORD, DWORD, REAL4, PTR) */
    kw = lookup_keyword(variable_types, tokbuf);
    if (kw) return ADD_TOKEN(arr, TOK_IDENT_VAR, tokbuf, kw->enum_val, line, col);

    /* Mnemonic — look up in the auto-generated table */
    for (U32 i = 0; i < sizeof(asm_mnemonics) / sizeof(asm_mnemonics[0]); i++) {
        if (asm_mnemonics[i].name && STRICMP(tokbuf, asm_mnemonics[i].name) == 0)
            return ADD_TOKEN(arr, TOK_MNEMONIC, tokbuf, asm_mnemonics[i].mnemonic, line, col);
    }

    /* Number literal (decimal, hex, binary) */
    U32 num_val = 0;
    if (parse_number(tokbuf, &num_val))
        return ADD_TOKEN(arr, TOK_NUMBER, tokbuf, num_val, line, col);

    /* Fallback: generic identifier */
    return ADD_TOKEN(arr, TOK_IDENTIFIER, tokbuf, 0, line, col);
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  MAIN LEXER
 * ════════════════════════════════════════════════════════════════════════════
 */
ASM_TOK_ARRAY *LEX(PASM_INFO info) {
    ASM_TOK_ARRAY *res = MAlloc(sizeof(ASM_TOK_ARRAY));
    if (!res) return NULLPTR;
    MEMSET(res, 0, sizeof(ASM_TOK_ARRAY));

    U8  linebuf[BUF_SZ];
    U8  tokenbuf[BUF_SZ];
    U32 token_len = 0;
    U32 lineno    = 0;

    for (U32 f = 0; f < info->tmp_file_count; f++) {
        FILE *file = FOPEN(info->tmp_files[f], MODE_R | MODE_FAT32);
        if (!file) {
            printf("[LEX] Cannot open '%s'\n", info->tmp_files[f]);
            DESTROY_TOK_ARR(res);
            return NULLPTR;
        }

        while (READ_LOGICAL_LINE(file, linebuf, sizeof(linebuf))) {
            lineno++;
            token_len = 0;
            U32 col = 1;
            U32 len = STRLEN(linebuf);

            for (U32 i = 0; i < len; ) {
                U8 c = linebuf[i];

                /* ── String literal "..." ── */
                if (c == '"') {
                    U32 start_col = col;
                    token_len = 0;
                    tokenbuf[token_len++] = '"';
                    i++; col++;
                    while (i < len && token_len + 1 < BUF_SZ) {
                        U8 cc = linebuf[i];
                        tokenbuf[token_len++] = cc;
                        i++; col++;
                        if (cc == '"' && tokenbuf[token_len - 2] != '\\') break;
                    }
                    tokenbuf[token_len] = '\0';
                    ADD_TOKEN(res, TOK_STRING, tokenbuf, 0, lineno, start_col);
                    token_len = 0;
                    continue;
                }

                /* ── Character literal 'x' ── */
                if (c == '\'') {
                    U32 start_col = col;
                    i++; col++;
                    U32 char_val = 0;
                    if (i < len && linebuf[i] == '\\') {
                        /* escape sequence */
                        i++; col++;
                        if (i < len) {
                            switch (linebuf[i]) {
                                case 'n':  char_val = '\n'; break;
                                case 'r':  char_val = '\r'; break;
                                case 't':  char_val = '\t'; break;
                                case '0':  char_val = '\0'; break;
                                case '\\': char_val = '\\'; break;
                                case '\'': char_val = '\''; break;
                                default:   char_val = linebuf[i]; break;
                            }
                            i++; col++;
                        }
                    } else if (i < len) {
                        char_val = linebuf[i];
                        i++; col++;
                    }
                    /* skip closing quote */
                    if (i < len && linebuf[i] == '\'') { i++; col++; }

                    U8 txt[6];
                    SPRINTF(txt, "'%c'", char_val ? char_val : '0');
                    ADD_TOKEN(res, TOK_NUMBER, txt, char_val, lineno, start_col);
                    continue;
                }

                /* ── Whitespace — flush accumulated token ── */
                if (IS_SPACE(c)) {
                    if (token_len > 0) {
                        tokenbuf[token_len] = '\0';
                        if (!emit_token_from_buf(res, tokenbuf, lineno, col - token_len)) {
                            FCLOSE(file);
                            DESTROY_TOK_ARR(res);
                            return NULLPTR;
                        }
                        token_len = 0;
                    }
                    i++; col++;
                    continue;
                }

                /* ── Symbol ── */
                U32 sym_enum = 0;
                U32 sym_len  = 0;
                if (try_symbol_at(linebuf, i, len, &sym_enum, &sym_len)) {
                    /* flush pending text */
                    if (token_len > 0) {
                        tokenbuf[token_len] = '\0';
                        if (!emit_token_from_buf(res, tokenbuf, lineno, col - token_len)) {
                            FCLOSE(file);
                            DESTROY_TOK_ARR(res);
                            return NULLPTR;
                        }
                        token_len = 0;
                    }

                    U8 symtxt[3] = { linebuf[i], '\0', '\0' };
                    if (sym_len == 2) { symtxt[1] = linebuf[i + 1]; symtxt[2] = '\0'; }
                    ADD_TOKEN(res, TOK_SYMBOL, symtxt, sym_enum, lineno, col);
                    i   += sym_len;
                    col += sym_len;
                    continue;
                }

                /* ── Normal character — accumulate ── */
                tokenbuf[token_len++] = c;
                if (token_len + 1 >= BUF_SZ) {
                    tokenbuf[token_len] = '\0';
                    if (!emit_token_from_buf(res, tokenbuf, lineno, col - token_len)) {
                        FCLOSE(file);
                        DESTROY_TOK_ARR(res);
                        return NULLPTR;
                    }
                    token_len = 0;
                }
                i++; col++;
            }

            /* End of line — flush and emit EOL */
            if (token_len > 0) {
                tokenbuf[token_len] = '\0';
                if (!emit_token_from_buf(res, tokenbuf, lineno, col - token_len)) {
                    FCLOSE(file);
                    DESTROY_TOK_ARR(res);
                    return NULLPTR;
                }
                token_len = 0;
            }
            ADD_TOKEN(res, TOK_EOL, "~", SYM_NONE, lineno, 0);
        }

        ADD_TOKEN(res, TOK_EOF, "EOF", 0, lineno + 1, 0);
        FCLOSE(file);
    }

    /* Debug dump */
    for (U32 i = 0; i < res->len; i++) {
        PASM_TOK t = res->toks[i];
        DEBUGM_PRINTF("[TOKEN %03u] %-10s '%s' val=0x%X (L%u C%u)\n",
            i, TOKEN_TYPE_STR(t->type), t->txt, t->num_val, t->line, t->col);
    }
    DEBUGM_PRINTF("Total tokens: %d\n", res->len);

    return res;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  TOKEN_TYPE_STR — human-readable token type for debug output
 * ════════════════════════════════════════════════════════════════════════════
 */
const char *TOKEN_TYPE_STR(ASM_TOKEN_TYPE t) {
    switch (t) {
        case TOK_EOL:         return "EOL";
        case TOK_EOF:         return "EOF";
        case TOK_LABEL:       return "LABEL";
        case TOK_LOCAL_LABEL: return "LOCAL_LBL";
        case TOK_MNEMONIC:    return "MNEMONIC";
        case TOK_REGISTER:    return "REGISTER";
        case TOK_NUMBER:      return "NUMBER";
        case TOK_STRING:      return "STRING";
        case TOK_SYMBOL:      return "SYMBOL";
        case TOK_IDENT_VAR:   return "VAR_TYPE";
        case TOK_IDENTIFIER:  return "IDENT";
        case TOK_DIRECTIVE:   return "DIRECTIVE";
        default:              return "UNKNOWN";
    }
}
