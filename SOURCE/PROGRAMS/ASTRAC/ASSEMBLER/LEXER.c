#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>
#include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONICS.h>

static const KEYWORD registers[] ATTRIB_DATA = {
    /* == General Purpose 32-bit == */
    { "eax", REG_EAX }, { "ebx", REG_EBX }, { "ecx", REG_ECX }, { "edx", REG_EDX },
    { "esi", REG_ESI }, { "edi", REG_EDI }, { "ebp", REG_EBP }, { "esp", REG_ESP },

    /* == General Purpose 16-bit == */
    { "ax", REG_AX }, { "bx", REG_BX }, { "cx", REG_CX }, { "dx", REG_DX },
    { "si", REG_SI }, { "di", REG_DI }, { "bp", REG_BP }, { "sp", REG_SP },

    /* == 8-bit high/low == */
    { "ah", REG_AH }, { "al", REG_AL }, { "bh", REG_BH }, { "bl", REG_BL },
    { "ch", REG_CH }, { "cl", REG_CL }, { "dh", REG_DH }, { "dl", REG_DL },

    /* == Segment Registers == */
    { "cs", REG_CS }, { "ds", REG_DS }, { "es", REG_ES },
    { "fs", REG_FS }, { "gs", REG_GS }, { "ss", REG_SS },

    /* == Control Registers == */
    { "cr0", REG_CR0 }, { "cr2", REG_CR2 }, { "cr3", REG_CR3 }, { "cr4", REG_CR4 },

    /* == Debug Registers == */
    { "dr0", REG_DR0 }, { "dr1", REG_DR1 }, { "dr2", REG_DR2 }, { "dr3", REG_DR3 },
    { "dr6", REG_DR6 }, { "dr7", REG_DR7 },

    /* == x87 FPU Registers == */
    { "st0", REG_ST0 }, { "st1", REG_ST1 }, { "st2", REG_ST2 }, { "st3", REG_ST3 },
    { "st4", REG_ST4 }, { "st5", REG_ST5 }, { "st6", REG_ST6 }, { "st7", REG_ST7 },

    /* == MMX Registers == */
    { "mm0", REG_MM0 }, { "mm1", REG_MM1 }, { "mm2", REG_MM2 }, { "mm3", REG_MM3 },
    { "mm4", REG_MM4 }, { "mm5", REG_MM5 }, { "mm6", REG_MM6 }, { "mm7", REG_MM7 },

    /* == SSE Registers (XMM) == */
    { "xmm0", REG_XMM0 }, { "xmm1", REG_XMM1 }, { "xmm2", REG_XMM2 }, { "xmm3", REG_XMM3 },
    { "xmm4", REG_XMM4 }, { "xmm5", REG_XMM5 }, { "xmm6", REG_XMM6 }, { "xmm7", REG_XMM7 },

    /* End marker */
    { NULL, REG_NONE }
};

static const KEYWORD directives[] ATTRIB_DATA = {
    { "data", DIR_DATA },       // data section
    { "rodata", DIR_RODATA },       // read-only data section
    { "code", DIR_CODE },       // code section
    { "use32", DIR_CODE_TYPE_32}, 
    { "use16", DIR_CODE_TYPE_16}, 
    { NULL, 0 }
};

static const KEYWORD symbols[] ATTRIB_DATA = {
    /* Delimiters */
    { ",", SYM_COMMA },
    { ":", SYM_COLON },
    { ";", SYM_SEMICOLON },

    /* Brackets for memory addressing */
    { "[", SYM_LBRACKET },
    { "]", SYM_RBRACKET },
    { "(", SYM_LPAREN },
    { ")", SYM_RPAREN },

    /* Arithmetic / Logical Operators */
    { "+", SYM_PLUS },
    { "-", SYM_MINUS },
    { "*", SYM_ASTERISK },
    { "/", SYM_SLASH },
    { "%", SYM_PERCENT },

    /* Bitwise Operators */
    { "&", SYM_AND },
    { "|", SYM_OR },
    { "^", SYM_XOR },
    { "~", SYM_TILDE },

    /* Shift Operators (two-character strings) */
    { "<<", SYM_SHL },
    { ">>", SYM_SHR },

    /* Assignment / Special */
    { "=", SYM_EQUALS },

    /* Others */
    { ".", SYM_DOT },
    { "@", SYM_AT },
    { "$", SYM_DOLLAR },

    /* End marker */
    { NULL, SYM_NONE }
};

static const KEYWORD variable_types[] ATTRIB_DATA = {
    {"DB", TYPE_BYTE},
    {"DB", TYPE_BYTE},
    {"DW", TYPE_WORD},
    {"WORD", TYPE_WORD},
    {"DD", TYPE_DWORD},
    {"DWORD", TYPE_DWORD},
    {"REAL4", TYPE_FLOAT},
    {"PTR", TYPE_PTR},
};

const KEYWORD *get_registers() {return &registers;}
const KEYWORD *get_directives() {return &directives;}
const KEYWORD *get_symbols() {return &symbols;}
const KEYWORD *get_variable_types() {return &variable_types;}

BOOL ADD_TOKEN(ASM_TOK_ARRAY* arr, ASM_TOKEN_TYPE type, PU8 txt, U32 _union, U32 line, U32 col) {
    if (arr->len >= MAX_TOKENS) return FALSE;

    PASM_TOK tok = MAlloc(sizeof(ASM_TOK));
    if(!tok) return FALSE;

    tok->type = type;
    tok->txt = STRDUP(txt);
    tok->num = _union;
    tok->line = line;
    tok->col = col;
    
    arr->toks[arr->len++] = tok;
    return TRUE;
}

// -------------------- helpers --------------------

static const KEYWORD *lookup_keyword(const KEYWORD *arr, PU8 s) {
    if (!s || !*s) return NULL;
    for (U32 i = 0; arr[i].value; i++) {
        if (STRICMP((U8*)s, (U8*)arr[i].value) == 0) return &arr[i];
    }
    return NULL;
}

static const KEYWORD *lookup_keyword_prefix(const KEYWORD *arr, PU8 s, U32 len) {
    if (!s || !*s) return NULL;
    for (U32 i = 0; arr[i].value; i++) {
        if ((U32)STRLEN(arr[i].value) == len && STRNICMP((U8*)s, (U8*)arr[i].value, len) == 0)
            return &arr[i];
    }
    return NULL;
}

// Check multi-char symbol first (<<, >>)
static BOOL try_symbol_at(const U8 *buf, U32 pos, U32 buflen, U32 *sym_enum) {
    // check two-char symbols first
    if (pos + 1 < buflen) {
        U8 twostr[3] = { buf[pos], buf[pos+1], '\0' };
        for (U32 i = 0; i < SYM_AMOUNT; i++) {
            if (symbols[i].value && STRCMP((PU8)twostr, (PU8)symbols[i].value) == 0) {
                *sym_enum = symbols[i].enum_val;
                return TRUE;
            }
        }
    }
    // single char
    U8 onestr[2] = { buf[pos], '\0' };
    for (U32 i = 0; i < SYM_AMOUNT; i++) {
        if (symbols[i].value && STRCMP((PU8)onestr, (PU8)symbols[i].value) == 0) {
            *sym_enum = symbols[i].enum_val;
            return TRUE;
        }
    }
    return FALSE;
}

// produce token from accumulated `tokbuf` (non-empty)
static BOOL emit_token_from_buf(ASM_TOK_ARRAY *arr, PU8 tokbuf, U32 line, U32 col) {
    if (!tokbuf || !*tokbuf) return TRUE;
    DEBUG_PRINTF("%s\n", tokbuf);
    // directive if starts with '.'
    if (tokbuf[0] == '.') {
        const KEYWORD *kw = lookup_keyword((const KEYWORD*)directives, tokbuf);
        if (kw) return ADD_TOKEN(arr, TOK_DIRECTIVE, tokbuf, kw->enum_val, line, col);
        else return ADD_TOKEN(arr, TOK_DIRECTIVE, tokbuf, DIR_NONE, line, col);
    }

    // registers (case-insensitive)
    const KEYWORD *rkw = lookup_keyword((const KEYWORD*)registers, tokbuf);
    if (rkw) return ADD_TOKEN(arr, TOK_REGISTER, tokbuf, rkw->enum_val, line, col);

    // variable types (DB, DW...) are often identifiers or directives; check case-insensitive
    const KEYWORD *vt = lookup_keyword((const KEYWORD*)variable_types, tokbuf);
    if (vt) return ADD_TOKEN(arr, TOK_IDENT_VAR, tokbuf, vt->enum_val, line, col);

    // mnemonics: treated as identifiers as of now. Parser works so only mnemonics are left as identifiers

    // numbers: decimal, hex (0x), binary (0b), Charecter 'c'
    // quick heuristic: starts with digit or '-' followed by digit or 0x/0b
    {
        PU8 p = tokbuf;
        BOOL negative = FALSE;
        if (*p == '+' || *p == '-') { negative = TRUE; p++; }
        if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
            // hex
            U32 idx = 0;
            for (PU8 q = p + 2; *q; q++) {
                CHAR c = *q;
                if (!( (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') )) { idx = 0; break; }
                idx++;
            }
            if (idx > 0) return ADD_TOKEN(arr, TOK_NUMBER, tokbuf, 0, line, col);
        } else if (*p == '0' && (p[1] == 'b' || p[1] == 'B')) {
            // binary
            U32 idx = 0;
            for (PU8 q = p + 2; *q; q++) {
                if (*q != '0' && *q != '1') { idx = 0; break; }
                idx++;
            }
            if (idx > 0) return ADD_TOKEN(arr, TOK_NUMBER, tokbuf, 0, line, col);
        } else {
            // decimal or floating (contains '.' or 'e'/'E')
            BOOL all_digits = TRUE;
            BOOL has_dot = FALSE;
            PU8 q = tokbuf;
            if (*q == '+' || *q == '-') q++;
            for (; *q; q++) {
                if (*q == '.') { has_dot = TRUE; continue; }
                if (!IS_DIGIT(*q)) { all_digits = FALSE; break; }
            }
            if (all_digits) return ADD_TOKEN(arr, TOK_NUMBER, tokbuf, 0, line, col);
        }
    }

    // string literal handled separately by lexer state; char literal like 'a'
    // If ends with ':' treat as label? We should have stripped ':'
    // fallback: identifier
    return ADD_TOKEN(arr, TOK_IDENTIFIER, tokbuf, 0, line, col);
}

// -------------------- main lexer --------------------
const char* TOKEN_TYPE_STR(ASM_TOKEN_TYPE t);
ASM_TOK_ARRAY *LEX(PASM_INFO info) {
    ASM_TOK_ARRAY *res = MAlloc(sizeof(ASM_TOK_ARRAY));
    if(!res) return NULLPTR;
    MEMSET(res, 0, sizeof(*res));

    U8 linebuf[BUF_SZ];
    U8 tokenbuf[BUF_SZ];
    U32 token_len = 0;
    U32 lineno = 0;

    for (U32 f = 0; f < info->tmp_file_count; f++) {
        FILE file;
        if (!FOPEN(&file, info->tmp_files[f], MODE_R | MODE_FAT32)) {
            printf("[ASM LEX] Unable to open tmp file '%s'\n", info->tmp_files[f]);
            DESTROY_TOK_ARR(res);
            return NULLPTR;
        }

        while (READ_LOGICAL_LINE(&file, linebuf, sizeof(linebuf))) {
            lineno++;
            token_len = 0;
            U32 col = 1;
            U32 len = STRLEN(linebuf);

            for (U32 i = 0; i < len; ) {
                U8 c = linebuf[i];

                // strings
                if (c == '"') {
                    U8 quote = c;
                    U32 start_col = col;
                    // collect full string including quotes
                    token_len = 0;
                    tokenbuf[token_len++] = quote;
                    i++; col++;
                    while (i < len) {
                        U8 cc = linebuf[i];
                        tokenbuf[token_len++] = cc;
                        i++; col++;
                        if (cc == quote && tokenbuf[token_len-2] != '\\') break;
                        if (token_len + 1 >= sizeof(tokenbuf)) break;
                    }
                    tokenbuf[token_len] = '\0';
                    ADD_TOKEN(res, TOK_STRING, tokenbuf, 0, lineno, start_col);
                    token_len = 0;
                    continue;
                }

                // whitespace -> flush tokenbuf
                if (IS_SPACE(c)) {
                    if (token_len > 0) {
                        tokenbuf[token_len] = '\0';
                        if (!emit_token_from_buf(res, tokenbuf, lineno, col - token_len)) {
                            FCLOSE(&file);
                            DESTROY_TOK_ARR(res);
                            return NULLPTR;
                        }
                        token_len = 0;
                    }
                    i++; col++;
                    continue;
                }

                // check symbol (try 2-char first)
                U32 sym_enum = 0;
                if (try_symbol_at(linebuf, i, len, &sym_enum)) {
                    // flush pending token
                    if (token_len > 0) {
                        tokenbuf[token_len] = '\0';
                        if (!emit_token_from_buf(res, tokenbuf, lineno, col - token_len)) {
                            FCLOSE(&file);
                            DESTROY_TOK_ARR(res);
                            return NULLPTR;
                        }
                        token_len = 0;
                    }
                    // add symbol token
                    U8 symtxt[3] = { linebuf[i], '\0', '\0' };
                    // if 2-char symbol
                    if (i + 1 < len) {
                        U8 two[3] = { linebuf[i], linebuf[i+1], '\0' };
                        BOOL matched2 = FALSE;
                        for (U32 s = 0; s < SYM_AMOUNT; s++) {
                            if (symbols[s].value && symbols[s].value[0] && symbols[s].value[1] && STRCMP((PU8)two, (PU8)symbols[s].value) == 0) {
                                // emit two-char
                                symtxt[0] = two[0]; symtxt[1] = two[1]; symtxt[2] = '\0';
                                ADD_TOKEN(res, TOK_SYMBOL, symtxt, symbols[s].enum_val, lineno, col);
                                i += 2; col += 2;
                                matched2 = TRUE;
                                break;
                            }
                        }
                        if (matched2) continue;
                    }
                    // fallback single char symbol
                    symtxt[0] = linebuf[i]; symtxt[1] = '\0';
                    const KEYWORD *k = lookup_keyword((const KEYWORD*)symbols, (PU8)symtxt);
                    U32 ev = k ? k->enum_val : 0;
                    ADD_TOKEN(res, TOK_SYMBOL, symtxt, ev, lineno, col);
                    i++; col++;
                    continue;
                }

                // normal char -> add to token buffer
                tokenbuf[token_len++] = c;
                if (token_len + 1 >= sizeof(tokenbuf)) {
                    // token too long
                    tokenbuf[token_len] = '\0';
                    if (!emit_token_from_buf(res, tokenbuf, lineno, col - token_len)) {
                        FCLOSE(&file);
                        DESTROY_TOK_ARR(res);
                        return NULLPTR;
                    }
                    token_len = 0;
                }
                i++; col++;
            }

            // end of line: flush token buffer
            if (token_len > 0) {
                tokenbuf[token_len] = '\0';
                if (!emit_token_from_buf(res, tokenbuf, lineno, col - token_len)) {
                    FCLOSE(&file);
                    DESTROY_TOK_ARR(res);
                    return NULLPTR;
                }
                token_len = 0;
            }

            // EOL token
            ADD_TOKEN(res, TOK_EOL, "~", SYM_NONE, lineno, 0); // or TOK_EOF/TOK_EOL depending on design
        }
        ADD_TOKEN(res, TOK_EOF, "EOF", 0, lineno + 1, 0);
        FCLOSE(&file);
    }
    for (U32 i = 0; i < res->len; i++) {
        PASM_TOK t = res->toks[i]; // note: toks[i] is a pointer in your ADD_TOKEN
        DEBUG_PRINTF("[TOKEN %03u] %s '%s' Enum=0x%X (Line %u, Col %u)\n",
            i, TOKEN_TYPE_STR(t->type), t->txt, t->num, t->line, t->col);
    }
    DEBUG_PRINTF("Len: %d\n", res->len);

    return res;
}
const char* TOKEN_TYPE_STR(ASM_TOKEN_TYPE t) {
    switch (t) {
        case TOK_LOCAL_LABEL: return "LOCAL";
        case TOK_IDENT_VAR: return "VAR";
        case TOK_EOL: return "EOL";
        case TOK_EOF: return "EOL";
        case TOK_SYMBOL: return "SYMBOL";
        case TOK_REGISTER: return "REGISTER";
        case TOK_DIRECTIVE: return "DIRECTIVE";
        case TOK_IDENTIFIER: return "IDENTIFIER";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING: return "STRING";
        default: return "UNKNOWN";
    }
}
