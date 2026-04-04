#include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONICS.h>
#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>

/* Forward declarations */
STATIC PU8 COLLECT_VALUE_TEXT(TOK_CURSOR *cur);

/* Current code mode — tracked by the AST builder so that RESOLVE_MNEMONIC
 * can prefer the native operand size when no register constrains it. */
STATIC ASM_DIRECTIVE ast_code_mode = DIR_CODE_TYPE_32;

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  AST BUILDER
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  Consumes the flat token stream from the lexer and produces an array of
 *  ASM_NODE structures that represent the program structure.
 *
 *  Section model:
 *    .data / .rodata  — variables only (NODE_DATA_VAR)
 *    .code            — labels, instructions, raw numbers
 *
 *  Token grammar summary:
 *    section_dir := '.' <directive>
 *    global_label := IDENTIFIER ':'
 *    local_label  := '@' '@' (NUMBER | IDENTIFIER) ':'
 *                 |  '@' ('f' | 'b')
 *    variable     := IDENTIFIER VAR_TYPE value_list
 *    instruction  := MNEMONIC [operand (',' operand)*]
 *    operand      := register | immediate | memory_ref | identifier
 *    memory_ref   := '[' operand_expr ']'
 */


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  SMALL HELPERS
 * ════════════════════════════════════════════════════════════════════════════
 */

/* Map a register enum to its bit-width (mirrors VERIFY_AST.c). */
STATIC ASM_OPERAND_SIZE ast_reg_size(ASM_REGS reg) {
    switch (reg) {
        /* 32-bit GPR */
        case REG_EAX: case REG_EBX: case REG_ECX: case REG_EDX:
        case REG_ESI: case REG_EDI: case REG_EBP: case REG_ESP:
        case REG_EIP:
        case REG_CR0: case REG_CR2: case REG_CR3: case REG_CR4:
        case REG_DR0: case REG_DR1: case REG_DR2: case REG_DR3:
        case REG_DR6: case REG_DR7:
            return SZ_32BIT;
        /* 16-bit GPR + segments */
        case REG_AX: case REG_BX: case REG_CX: case REG_DX:
        case REG_SI: case REG_DI: case REG_BP: case REG_SP:
        case REG_IP:
        case REG_CS: case REG_DS: case REG_ES:
        case REG_FS: case REG_GS: case REG_SS:
            return SZ_16BIT;
        /* 8-bit GPR */
        case REG_AL: case REG_AH: case REG_BL: case REG_BH:
        case REG_CL: case REG_CH: case REG_DL: case REG_DH:
            return SZ_8BIT;
        default:
            return SZ_NONE;
    }
}

/* Is this a segment register? (CS, DS, ES, FS, GS, SS) */
STATIC BOOL ast_is_seg_reg(ASM_REGS reg) {
    switch (reg) {
        case REG_CS: case REG_DS: case REG_ES:
        case REG_FS: case REG_GS: case REG_SS:
            return TRUE;
        default:
            return FALSE;
    }
}

/*
 * Check whether 'actual' operand type satisfies 'expected' from the table.
 * r/m accepts a plain register (or segment register), and a label reference
 * (OP_PTR) can appear where an immediate or memory operand is expected.
 */
STATIC BOOL ast_operand_type_ok(ASM_OPERAND_TYPE actual,
                                ASM_OPERAND_TYPE expected) {
    if (actual == expected)                              return TRUE;
    if (expected == OP_MEM  && actual == OP_REG)         return TRUE;  /* r/m */
    if (expected == OP_IMM  && actual == OP_PTR)         return TRUE;  /* rel */
    if (expected == OP_MEM  && actual == OP_PTR)         return TRUE;  /* [sym] */
    return FALSE;
}

/*
 * After all operands are parsed, scan asm_mnemonics[] for the best table
 * entry whose name, operand count, operand types, and operand sizes all
 * agree with the parsed operands.
 */
STATIC const ASM_MNEMONIC_TABLE *RESOLVE_MNEMONIC(
        PU8 name, ASM_OPERAND *operands, U32 op_count) {
    U32 tbl_len = sizeof(asm_mnemonics) / sizeof(asm_mnemonics[0]);

    /* Determine native operand size from the current code mode. */
    ASM_OPERAND_SIZE native_size =
        (ast_code_mode == DIR_CODE_TYPE_16) ? SZ_16BIT : SZ_32BIT;

    /* Check whether any operand constrains the size (register or
     * explicitly-sized memory reference).  When constrained the first
     * matching table entry is always correct.  When NOT constrained
     * (all operands are OP_IMM / unsized OP_PTR) we prefer the entry
     * whose size matches the native mode so that e.g. `push 0xb800`
     * in .use16 picks PUSH_IMM16 instead of PUSH_IMM32. */
    BOOL has_size_constraint = FALSE;
    for (U32 j = 0; j < op_count; j++) {
        if (operands[j].type == OP_REG || operands[j].type == OP_SEG) {
            has_size_constraint = TRUE;
            break;
        }
        if ((operands[j].type == OP_MEM || operands[j].type == OP_PTR)
            && operands[j].size != SZ_NONE) {
            has_size_constraint = TRUE;
            break;
        }
    }

    const ASM_MNEMONIC_TABLE *fallback = NULLPTR;

    for (U32 i = 0; i < tbl_len; i++) {
        const ASM_MNEMONIC_TABLE *tbl = &asm_mnemonics[i];

        /* ── Name must match ──────────────────────────────────────── */
        if (!tbl->name || STRICMP(name, tbl->name) != 0) continue;

        /* ── Operand count ────────────────────────────────────────── */
        if ((U32)tbl->operand_count != op_count) continue;

        /* ── Per-operand type check ───────────────────────────────── */
        BOOL ok = TRUE;
        for (U32 j = 0; j < op_count; j++) {
            if (!ast_operand_type_ok(operands[j].type, tbl->operand[j])) {
                ok = FALSE;
                break;
            }
        }
        if (!ok) continue;

        /* ── Size check: register/mem-with-prefix must match tbl->size ─ */
        if (tbl->size != SZ_NONE) {
            /*
             * For 0F-prefixed two-operand instructions (MOVZX/MOVSX, BSF/BSR)
             * tbl->size describes the SOURCE operand size, not the destination.
             * Skip size-checking the destination (operand 0) for these.
             */
            BOOL is_0f_two_op = (tbl->opcode_prefix == PFX_0F
                                 && (U32)tbl->operand_count == 2);

            for (U32 j = 0; j < op_count; j++) {
                if (is_0f_two_op && j == 0) continue; /* skip dest size check */

                if (operands[j].type == OP_REG || operands[j].type == OP_SEG) {
                    ASM_OPERAND_SIZE rs = ast_reg_size(operands[j].reg);
                    if (rs != SZ_NONE && rs != tbl->size) { ok = FALSE; break; }
                }
                if ((operands[j].type == OP_MEM || operands[j].type == OP_PTR)
                    && operands[j].size != SZ_NONE
                    && operands[j].size != tbl->size) {
                    ok = FALSE; break;
                }
            }
            if (!ok) continue;
        }

        /* ── Immediate bounds check: ensure OP_IMM values fit ────────── */
        if (tbl->size != SZ_NONE) {
            /* For 0x83 / 0x6B: the immediate is sign-extended from 8 bits,
             * so it must fit in [-128, 127].  This prevents ADD_RM16_IMM8
             * from matching e.g. `add ax, 511` which should use IMM16. */
            U8 imm_opc = tbl->opcode[0];
            if (tbl->opcode_prefix == PFX_0F) imm_opc = tbl->opcode[1];
            BOOL is_simm8 = (imm_opc == 0x83 || imm_opc == 0x6B);

            for (U32 j = 0; j < op_count; j++) {
                if (operands[j].type == OP_IMM) {
                    U32 v = operands[j].immediate;
                    if (tbl->size == SZ_8BIT  && v > 0xFF && v < 0xFFFFFF80u) {
                        ok = FALSE; break;
                    }
                    if (tbl->size == SZ_16BIT && v > 0xFFFF && v < 0xFFFF0000u) {
                        ok = FALSE; break;
                    }
                    /* 0x83 / 0x6B sign-extended imm8: value must be in [-128,127] */
                    if (is_simm8 && v > 0x7F && v < 0xFFFFFF80u) {
                        ok = FALSE; break;
                    }
                }
            }
            if (!ok) continue;
        }

        /* ── This entry matches ───────────────────────────────────── */

        /* Size is constrained by a register / sized memory operand —
         * the first match is always the correct one. */
        if (has_size_constraint)
            return tbl;

        /* No register constrains size — prefer entries whose size
         * matches the native operand width of the current code mode.
         * SZ_8BIT forms (e.g. PUSH imm8) are always optimal since
         * they sign-extend to the native size automatically. */
        if (tbl->size == native_size || tbl->size == SZ_8BIT
            || tbl->size == SZ_NONE)
            return tbl;

        /* Non-native size — save as fallback in case no native-sized
         * entry exists (e.g. value too large for native size). */
        if (!fallback) fallback = tbl;
    }

    return fallback;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  TOK CURSOR  —  implementation
 * ════════════════════════════════════════════════════════════════════════════
 */

PASM_TOK TOK_PEEK(TOK_CURSOR *cur) {
    if (cur->pos >= cur->arr->len) return NULLPTR;
    return cur->arr->toks[cur->pos];
}

PASM_TOK TOK_ADVANCE(TOK_CURSOR *cur) {
    if (cur->pos >= cur->arr->len) return NULLPTR;
    return cur->arr->toks[cur->pos++];
}

BOOL TOK_AT_END(TOK_CURSOR *cur) {
    PASM_TOK t = TOK_PEEK(cur);
    return (!t || t->type == TOK_EOF);
}

BOOL TOK_MATCH(TOK_CURSOR *cur, ASM_TOKEN_TYPE type) {
    PASM_TOK t = TOK_PEEK(cur);
    return (t && t->type == type);
}

BOOL TOK_EXPECT(TOK_CURSOR *cur, ASM_TOKEN_TYPE type, PU8 ctx) {
    if (TOK_MATCH(cur, type)) { TOK_ADVANCE(cur); return TRUE; }
    PASM_TOK t = TOK_PEEK(cur);
    printf("[AST] Expected %s but got '%s' at L%u C%u (%s)\n",
        TOKEN_TYPE_STR(type),
        t ? t->txt : "EOF",
        t ? t->line : 0,
        t ? t->col  : 0,
        ctx ? ctx : "");
    return FALSE;
}

VOID TOK_SKIP_EOL(TOK_CURSOR *cur) {
    while (TOK_MATCH(cur, TOK_EOL)) TOK_ADVANCE(cur);
}

/* Peek at a symbol value without consuming */
STATIC BOOL PEEK_SYMBOL(TOK_CURSOR *cur, ASM_SYMBOLS sym) {
    PASM_TOK t = TOK_PEEK(cur);
    return (t && t->type == TOK_SYMBOL && t->symbol == sym);
}

/* peek N tokens ahead (0 = current) */
STATIC PASM_TOK PEEK_N(TOK_CURSOR *cur, U32 n) {
    U32 pos = cur->pos + n;
    if (pos >= cur->arr->len) return NULLPTR;
    return cur->arr->toks[pos];
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  NODE ALLOCATOR
 * ════════════════════════════════════════════════════════════════════════════
 */
STATIC PASM_NODE ALLOC_NODE(ASM_NODE_TYPE type, PASM_TOK tok) {
    PASM_NODE n = MAlloc(sizeof(ASM_NODE));
    if (!n) return NULLPTR;
    MEMSET(n, 0, sizeof(ASM_NODE));
    n->type = type;
    n->line = tok ? tok->line : 0;
    n->col  = tok ? tok->col  : 0;
    return n;
}

STATIC BOOL PUSH_NODE(ASM_AST_ARRAY *arr, PASM_NODE node) {
    if (!node || arr->len >= MAX_NODES) return FALSE;
    arr->nodes[arr->len++] = node;
    return TRUE;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  SECTION DIRECTIVE PARSER
 * ════════════════════════════════════════════════════════════════════════════
 *  Syntax:  '.' directive_name
 *  Tokens at entry: TOK_SYMBOL('.')
 */
STATIC PASM_NODE PARSE_SECTION(TOK_CURSOR *cur) {
    TOK_ADVANCE(cur);   /* consume '.' */

    PASM_TOK dir_tok = TOK_PEEK(cur);
    if (!dir_tok || dir_tok->type != TOK_DIRECTIVE) {
        printf("[AST] Expected directive name after '.'\n");
        return NULLPTR;
    }
    TOK_ADVANCE(cur);   /* consume directive name */

    /* ── .org <number> ───────────────────────────────────────────────── */
    if (dir_tok->directive == DIR_ORG) {
        PASM_TOK val_tok = TOK_PEEK(cur);
        if (!val_tok || val_tok->type != TOK_NUMBER) {
            printf("[AST] Expected number after '.org' at L%u\n", dir_tok->line);
            return NULLPTR;
        }
        TOK_ADVANCE(cur);
        PASM_NODE n = ALLOC_NODE(NODE_ORG, dir_tok);
        if (!n) return NULLPTR;
        n->org.origin = val_tok->num_val;
        return n;
    }

    /* ── .times <expr> <type> <value> ────────────────────────────────── */
    if (dir_tok->directive == DIR_TIMES) {
        /* Collect count expression tokens until a type keyword (DB/DW/DD) */
        U8  expr_buf[BUF_SZ] = { 0 };
        U32 expr_len = 0;
        BOOL expr_first = TRUE;

        while (!TOK_AT_END(cur) && !TOK_MATCH(cur, TOK_EOL)) {
            PASM_TOK t = TOK_PEEK(cur);
            if (t->type == TOK_IDENT_VAR) break;  /* stop at DB/DW/DD */
            TOK_ADVANCE(cur);
            if (!expr_first && expr_len + 1 < BUF_SZ) expr_buf[expr_len++] = ' ';
            U32 tlen = STRLEN(t->txt);
            if (expr_len + tlen < BUF_SZ) {
                STRNCPY(expr_buf + expr_len, t->txt, BUF_SZ - expr_len - 1);
                expr_len += tlen;
            }
            expr_first = FALSE;
        }

        /* Parse type keyword */
        ASM_VAR_TYPE fill_type = TYPE_BYTE;
        PASM_TOK type_tok = TOK_PEEK(cur);
        if (type_tok && type_tok->type == TOK_IDENT_VAR) {
            fill_type = type_tok->var_type;
            TOK_ADVANCE(cur);
        }

        /* Collect fill value text */
        PU8 fill_raw = COLLECT_VALUE_TEXT(cur);

        PASM_NODE n = ALLOC_NODE(NODE_TIMES, dir_tok);
        if (!n) return NULLPTR;
        n->times.count_expr = (expr_len > 0) ? STRDUP(expr_buf) : NULLPTR;
        n->times.fill_type  = fill_type;
        n->times.fill_raw   = fill_raw;
        return n;
    }

    PASM_NODE n = ALLOC_NODE(NODE_SECTION, dir_tok);
    if (!n) return NULLPTR;
    n->dir.section = dir_tok->directive;
    return n;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  OPERAND VALUE PARSER (immediate / expression)
 * ════════════════════════════════════════════════════════════════════════════
 *  Parses one immediate atom: number, identifier (label ref), or '$'.
 *  For memory refs the bracket loop in PARSE_OPERAND handles the brackets.
 *  Returns the raw text joined into a freshly-allocated string, or NULLPTR.
 */
/* ── Numeric expression evaluator for compile-time immediates ─────────────── */
STATIC U32 parse_single_num(PU8 *pp) {
    PU8 p = *pp;
    while (*p == ' ' || *p == '\t') p++;
    U32 val = 0;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
        while ((*p >= '0' && *p <= '9') ||
               (*p >= 'a' && *p <= 'f') ||
               (*p >= 'A' && *p <= 'F')) {
            val <<= 4;
            if (*p >= '0' && *p <= '9') val += (U32)(*p - '0');
            else if (*p >= 'a') val += (U32)(*p - 'a') + 10u;
            else val += (U32)(*p - 'A') + 10u;
            p++;
        }
    } else if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) {
        p += 2;
        while (*p == '0' || *p == '1') { val = val * 2u + (U32)(*p - '0'); p++; }
    } else if (*p >= '0' && *p <= '9') {
        while (*p >= '0' && *p <= '9') { val = val * 10u + (U32)(*p - '0'); p++; }
    }
    *pp = p;
    return val;
}

/* Evaluate left-to-right: A + B - C * D / E  (handles +, -, *, /) */
STATIC U32 eval_imm_text(PU8 txt) {
    if (!txt || !txt[0]) return 0;
    PU8 p = txt;
    U32 result = parse_single_num(&p);
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (*p == '+') {
            p++;
            result += parse_single_num(&p);
        } else if (*p == '-') {
            p++;
            result -= parse_single_num(&p);
        } else if (*p == '*') {
            p++;
            result *= parse_single_num(&p);
        } else if (*p == '/') {
            p++;
            U32 b = parse_single_num(&p);
            if (b) result /= b;
        } else break;
    }
    return result;
}

STATIC PU8 COLLECT_VALUE_TEXT(TOK_CURSOR *cur) {
    U8  buf[BUF_SZ] = { 0 };
    U32 len = 0;
    BOOL first = TRUE;

    while (!TOK_AT_END(cur)) {
        PASM_TOK t = TOK_PEEK(cur);
        if (!t) break;

        /* stop on comma, colon, closing bracket, EOL/EOF */
        if (t->type == TOK_EOL || t->type == TOK_EOF) break;
        if (t->type == TOK_SYMBOL) {
            ASM_SYMBOLS s = t->symbol;
            if (s == SYM_COMMA || s == SYM_COLON || s == SYM_RBRACKET) break;
        }

        TOK_ADVANCE(cur);
        U32 tlen = STRLEN(t->txt);
        if (!first && len + 1 < BUF_SZ) buf[len++] = ' ';
        if (len + tlen < BUF_SZ) {
            STRNCPY(buf + len, t->txt, BUF_SZ - len - 1);
            len += tlen;
        }
        first = FALSE;
    }

    if (len == 0) return NULLPTR;
    return STRDUP(buf);
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  OPERAND PARSER
 * ════════════════════════════════════════════════════════════════════════════
 *  Fills one ASM_OPERAND.  Returns FALSE on fatal failure.
 */
STATIC BOOL PARSE_OPERAND(TOK_CURSOR *cur, ASM_OPERAND *out) {
    MEMSET(out, 0, sizeof(ASM_OPERAND));
    out->size = SZ_NONE;

    PASM_TOK t = TOK_PEEK(cur);
    if (!t) return FALSE;

    /* NEAR / FAR distance hint prefix (consumed before size prefixes) */
    BOOL want_far = FALSE;
    if (t->type == TOK_IDENT_VAR
        && (t->var_type == TYPE_NEAR || t->var_type == TYPE_FAR)) {
        want_far = (t->var_type == TYPE_FAR);
        TOK_ADVANCE(cur);
        /* optional PTR keyword after near/far */
        PASM_TOK maybe_ptr = TOK_PEEK(cur);
        if (maybe_ptr && maybe_ptr->type == TOK_IDENT_VAR
            && maybe_ptr->var_type == TYPE_PTR)
            TOK_ADVANCE(cur);
        t = TOK_PEEK(cur);
    }

    /* BYTE/WORD/DWORD PTR prefix */
    if (t->type == TOK_IDENT_VAR) {
        switch (t->var_type) {
            case TYPE_BYTE:  out->size = SZ_8BIT;  break;
            case TYPE_WORD:  out->size = SZ_16BIT; break;
            case TYPE_DWORD: out->size = SZ_32BIT; break;
            default: break;
        }
        TOK_ADVANCE(cur);
        /* optional PTR keyword */
        PASM_TOK maybe_ptr = TOK_PEEK(cur);
        if (maybe_ptr && maybe_ptr->type == TOK_IDENT_VAR
            && maybe_ptr->var_type == TYPE_PTR)
            TOK_ADVANCE(cur);
        t = TOK_PEEK(cur);
    }

    /* Register */
    if (t && t->type == TOK_REGISTER) {
        TOK_ADVANCE(cur);
        out->type = ast_is_seg_reg(t->reg) ? OP_SEG : OP_REG;
        out->reg  = t->reg;
        /* Infer size from register unless an explicit PTR override was given */
        if (out->size == SZ_NONE)
            out->size = ast_reg_size(t->reg);
        return TRUE;
    }

    /* Memory reference  [ ... ] */
    if (t && t->type == TOK_SYMBOL && t->symbol == SYM_LBRACKET) {
        TOK_ADVANCE(cur);   /* consume '[' */

        ASM_NODE_MEM *mem = MAlloc(sizeof(ASM_NODE_MEM));
        if (!mem) return FALSE;
        MEMSET(mem, 0, sizeof(ASM_NODE_MEM));
        mem->base_reg  = REG_NONE;
        mem->index_reg = REG_NONE;
        mem->segment   = REG_NONE;
        mem->scale     = 1;

        /*
         * Very simplified: scan tokens until ']' collecting:
         *  [seg:] reg  +  reg  *  scale  +  disp
         * We do a single pass and handle common patterns.
         */
        PASM_TOK r1 = TOK_PEEK(cur);
        if (r1 && r1->type == TOK_REGISTER) {
            TOK_ADVANCE(cur);
            /* Check for segment override: reg : ... */
            PASM_TOK maybe_colon = TOK_PEEK(cur);
            if (maybe_colon && maybe_colon->type == TOK_SYMBOL
                && maybe_colon->symbol == SYM_COLON) {
                /* Segment override — store and parse actual base */
                mem->segment = r1->reg;
                TOK_ADVANCE(cur);  /* consume ':' */
                PASM_TOK r2 = TOK_PEEK(cur);
                if (r2 && r2->type == TOK_REGISTER) {
                    TOK_ADVANCE(cur);
                    mem->base_reg = r2->reg;
                } else if (r2 && r2->type == TOK_IDENTIFIER) {
                    TOK_ADVANCE(cur);
                    mem->symbol_name = STRDUP(r2->txt);
                }
            } else {
                mem->base_reg = r1->reg;
            }
        } else if (r1 && r1->type == TOK_IDENTIFIER) {
            TOK_ADVANCE(cur);
            mem->symbol_name = STRDUP(r1->txt);
        } else if (r1 && r1->type == TOK_NUMBER) {
            /* Bare numeric address: [300] → displacement-only */
            TOK_ADVANCE(cur);
            mem->displacement = (S32)r1->num_val;
        }

        /* optional  + reg * scale + disp  chain */
        while (!TOK_AT_END(cur)) {
            PASM_TOK op_tok = TOK_PEEK(cur);
            if (!op_tok || op_tok->type == TOK_SYMBOL) {
                if (op_tok->symbol == SYM_RBRACKET) break;
            }
            if (op_tok->type == TOK_SYMBOL && op_tok->symbol == SYM_PLUS) {
                TOK_ADVANCE(cur);
                PASM_TOK next = TOK_PEEK(cur);
                if (next && next->type == TOK_REGISTER) {
                    TOK_ADVANCE(cur);
                    if (mem->index_reg == REG_NONE) mem->index_reg = next->reg;
                    /* peek * scale */
                    PASM_TOK star = TOK_PEEK(cur);
                    if (star && star->type == TOK_SYMBOL && star->symbol == SYM_ASTERISK) {
                        TOK_ADVANCE(cur);
                        PASM_TOK sc = TOK_PEEK(cur);
                        if (sc && sc->type == TOK_NUMBER) {
                            mem->scale = (S32)sc->num_val;
                            TOK_ADVANCE(cur);
                        }
                    }
                } else if (next && next->type == TOK_NUMBER) {
                    TOK_ADVANCE(cur);
                    mem->displacement += (S32)next->num_val;
                }
            } else if (op_tok->type == TOK_SYMBOL && op_tok->symbol == SYM_MINUS) {
                TOK_ADVANCE(cur);
                PASM_TOK next = TOK_PEEK(cur);
                if (next && next->type == TOK_NUMBER) {
                    TOK_ADVANCE(cur);
                    mem->displacement -= (S32)next->num_val;
                }
            } else {
                TOK_ADVANCE(cur);   /* skip unexpected */
            }
        }

        /* consume ']' */
        if (!TOK_EXPECT(cur, TOK_SYMBOL, "closing ']'")) {
            MFree(mem);
            return FALSE;
        }

        out->type    = OP_MEM;
        out->mem_ref = mem;
        return TRUE;
    }

    /* @f / @b / @@N  — local label reference in operand position */
    if (t && t->type == TOK_SYMBOL && t->symbol == SYM_AT) {
        PASM_TOK next = PEEK_N(cur, 1);
        if (next) {
            /* @f or @b */
            if (next->type == TOK_IDENTIFIER
                && (STRICMP(next->txt, "f") == 0 || STRICMP(next->txt, "b") == 0)) {
                TOK_ADVANCE(cur);   /* consume '@' */
                TOK_ADVANCE(cur);   /* consume 'f'/'b' */
                U8 ref_name[4];
                STRCPY(ref_name, "@");
                STRNCAT(ref_name, next->txt, 2);

                ASM_NODE_MEM *mem = MAlloc(sizeof(ASM_NODE_MEM));
                if (!mem) return FALSE;
                MEMSET(mem, 0, sizeof(ASM_NODE_MEM));
                mem->base_reg  = REG_NONE;
                mem->index_reg = REG_NONE;
                mem->segment   = REG_NONE;
                mem->scale     = 1;
                mem->symbol_name = STRDUP(ref_name);
                out->type    = OP_PTR;
                out->mem_ref = mem;
                return TRUE;
            }
            /* @@N */
            if (next->type == TOK_SYMBOL && next->symbol == SYM_AT) {
                TOK_ADVANCE(cur);   /* consume first '@' */
                TOK_ADVANCE(cur);   /* consume second '@' */
                PASM_TOK id = TOK_PEEK(cur);
                if (id && (id->type == TOK_NUMBER || id->type == TOK_IDENTIFIER)) {
                    TOK_ADVANCE(cur);
                    U8 ref_name[BUF_SZ] = { 0 };
                    STRCPY(ref_name, "@@");
                    STRNCAT(ref_name, id->txt, BUF_SZ - 3);

                    ASM_NODE_MEM *mem = MAlloc(sizeof(ASM_NODE_MEM));
                    if (!mem) return FALSE;
                    MEMSET(mem, 0, sizeof(ASM_NODE_MEM));
                    mem->base_reg  = REG_NONE;
                    mem->index_reg = REG_NONE;
                    mem->segment   = REG_NONE;
                    mem->scale     = 1;
                    mem->symbol_name = STRDUP(ref_name);
                    out->type    = OP_PTR;
                    out->mem_ref = mem;
                    return TRUE;
                }
            }
        }
    }

    /* Dot-prefixed label reference: .name → OP_PTR with ".name" */
    if (t && t->type == TOK_SYMBOL && t->symbol == SYM_DOT) {
        PASM_TOK dot_ident = PEEK_N(cur, 1);
        if (dot_ident && dot_ident->type == TOK_IDENTIFIER) {
            TOK_ADVANCE(cur);   /* consume '.' */
            TOK_ADVANCE(cur);   /* consume identifier */

            U8 ref_name[BUF_SZ] = { 0 };
            STRCPY(ref_name, ".");
            STRNCAT(ref_name, dot_ident->txt, BUF_SZ - 2);

            ASM_NODE_MEM *mem = MAlloc(sizeof(ASM_NODE_MEM));
            if (!mem) return FALSE;
            MEMSET(mem, 0, sizeof(ASM_NODE_MEM));
            mem->base_reg    = REG_NONE;
            mem->index_reg   = REG_NONE;
            mem->segment     = REG_NONE;
            mem->scale       = 1;
            mem->symbol_name = STRDUP(ref_name);
            out->type    = OP_PTR;
            out->mem_ref = mem;
            return TRUE;
        }
    }

    /* Negative immediate: - NUMBER → negate and treat as OP_IMM */
    if (t && t->type == TOK_SYMBOL && t->symbol == SYM_MINUS) {
        PASM_TOK next_num = PEEK_N(cur, 1);
        if (next_num && next_num->type == TOK_NUMBER) {
            TOK_ADVANCE(cur);   /* consume '-' */
            TOK_ADVANCE(cur);   /* consume number */
            out->type      = OP_IMM;
            out->immediate = (U32)(-(S32)next_num->num_val);
            return TRUE;
        }
    }

    /* Immediate — number, label ref, or '$' */
    {
        PU8 txt = COLLECT_VALUE_TEXT(cur);
        if (!txt) return FALSE;

        if (t && t->type == TOK_NUMBER) {
            out->type      = OP_IMM;
            out->immediate = eval_imm_text(txt);  /* evaluate full expression */
            MFree(txt);
            /* Check for far pointer syntax: SEG:OFF  (e.g. jmp 0x0000:0x2000) */
            PASM_TOK maybe_colon = TOK_PEEK(cur);
            if (maybe_colon && maybe_colon->type == TOK_SYMBOL
                && maybe_colon->symbol == SYM_COLON) {
                TOK_ADVANCE(cur);   /* consume ':' */
                PU8 off_txt = COLLECT_VALUE_TEXT(cur);
                if (off_txt) {
                    U32 seg = out->immediate;
                    U32 off = eval_imm_text(off_txt);
                    MFree(off_txt);
                    out->type      = OP_FAR;
                    out->immediate = (seg << 16) | (off & 0xFFFF);
                }
            }
        } else {
            /* symbol / label reference — store as mem with symbol name */
            ASM_NODE_MEM *mem = MAlloc(sizeof(ASM_NODE_MEM));
            if (!mem) { MFree(txt); return FALSE; }
            MEMSET(mem, 0, sizeof(ASM_NODE_MEM));
            mem->base_reg  = REG_NONE;
            mem->index_reg = REG_NONE;
            mem->segment   = REG_NONE;
            mem->scale     = 1;
            mem->symbol_name = txt;
            out->type    = OP_PTR;
            out->mem_ref = mem;
            /* jmp far label:imm  — label is the segment, imm is the offset */
            if (want_far) {
                PASM_TOK maybe_colon = TOK_PEEK(cur);
                if (maybe_colon && maybe_colon->type == TOK_SYMBOL
                    && maybe_colon->symbol == SYM_COLON) {
                    TOK_ADVANCE(cur);   /* consume ':' */
                    PU8 off_txt = COLLECT_VALUE_TEXT(cur);
                    if (off_txt) {
                        /* Segment is a symbolic label — store offset only;
                         * the symbol will be resolved as 0 at link time for
                         * far absolute pointers.  Encode what we can. */
                        U32 off = eval_imm_text(off_txt);
                        MFree(off_txt);
                        out->type      = OP_FAR;
                        out->immediate = (0u << 16) | (off & 0xFFFF);
                        /* mem_ref still holds the segment symbol for future
                         * linker support; immediate holds the offset. */
                    }
                }
            }
        }
        return TRUE;
    }
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  INSTRUCTION PARSER
 * ════════════════════════════════════════════════════════════════════════════
 *  Entry: current token is TOK_MNEMONIC.
 *  Syntax: MNEMONIC [operand (',' operand)*]
 */
STATIC PASM_NODE PARSE_INSTRUCTION(TOK_CURSOR *cur) {
    PASM_TOK mnem_tok = TOK_ADVANCE(cur);   /* consume mnemonic */

    PASM_NODE n = ALLOC_NODE(NODE_INSTRUCTION, mnem_tok);
    if (!n) return NULLPTR;

    /* ── 1. Parse operands first ─────────────────────────────────────── */
    n->instr.operand_count = 0;
    BOOL first = TRUE;

    while (!TOK_AT_END(cur) && !TOK_MATCH(cur, TOK_EOL)) {
        if (!first) {
            /* expect comma separator */
            if (!PEEK_SYMBOL(cur, SYM_COMMA)) break;
            TOK_ADVANCE(cur);   /* consume ',' */
        }
        first = FALSE;

        if (n->instr.operand_count >= MAX_OPERANDS) {
            printf("[AST] Too many operands at L%u\n", mnem_tok->line);
            break;
        }

        ASM_OPERAND op;
        if (!PARSE_OPERAND(cur, &op)) {
            printf("[AST] Bad operand in '%s' at L%u\n", mnem_tok->txt, mnem_tok->line);
            break;
        }
        n->instr.operands[n->instr.operand_count++] = op;
    }

    /* ── 1b. Shift/rotate by-CL or by-1 canonicalization ─────────────
     *  x86 shift/rotate instructions encode the CL and by-1 forms as
     *  single-operand instructions (CL/1 is implicit in the opcode).
     *  If the user writes "SHL EAX, CL" or "SHL EAX, 1", strip the
     *  second operand so that RESOLVE_MNEMONIC matches the _CL / _1
     *  single-operand table entries.
     */
    if (n->instr.operand_count == 2
        && (STRICMP(mnem_tok->txt, "shl")  == 0
         || STRICMP(mnem_tok->txt, "shr")  == 0
         || STRICMP(mnem_tok->txt, "sar")  == 0
         || STRICMP(mnem_tok->txt, "rol")  == 0
         || STRICMP(mnem_tok->txt, "ror")  == 0)) {
        ASM_OPERAND *op2 = &n->instr.operands[1];
        if (op2->type == OP_REG && op2->reg == REG_CL) {
            /* Strip CL — the _CL table entry is 1-operand */
            n->instr.operand_count = 1;
        } else if (op2->type == OP_IMM && op2->immediate == 1) {
            /* Strip literal 1 — the _1 table entry is 1-operand */
            n->instr.operand_count = 1;
        }
    }

    /* ── 2. Resolve mnemonic table entry by name + operand match ─────── */
    n->instr.table_entry = RESOLVE_MNEMONIC(
            mnem_tok->txt,
            n->instr.operands,
            n->instr.operand_count);

    if (!n->instr.table_entry) {
        printf("[AST] No matching mnemonic form for '%s' with %u operand(s) at L%u\n",
               mnem_tok->txt, n->instr.operand_count, mnem_tok->line);
    }

    return n;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  DATA VARIABLE PARSER
 * ════════════════════════════════════════════════════════════════════════════
 *  Entry: current token is TOK_IDENTIFIER (the variable name).
 *  The lookahead already confirmed the next token is TOK_IDENT_VAR.
 *
 *  Syntax:
 *    name  DB  "string", 0
 *    name  DW  1, 2, 3
 *    name  DD  0x1234
 */
STATIC PASM_NODE PARSE_DATA_VAR(TOK_CURSOR *cur) {
    PASM_TOK name_tok = TOK_ADVANCE(cur);   /* consume name identifier */
    PASM_TOK type_tok = TOK_ADVANCE(cur);   /* consume type keyword */

    PASM_VAR var = MAlloc(sizeof(ASM_VAR));
    if (!var) return NULLPTR;
    MEMSET(var, 0, sizeof(ASM_VAR));

    var->name     = STRDUP(name_tok->txt);
    var->var_type = type_tok->var_type;

    /* Collect raw value text (everything until EOL/EOF) */
    U8  val_buf[BUF_SZ] = { 0 };
    U32 val_len = 0;
    BOOL first = TRUE;
    U32  commas = 0;

    while (!TOK_AT_END(cur) && !TOK_MATCH(cur, TOK_EOL)) {
        PASM_TOK t = TOK_ADVANCE(cur);

        if (t->type == TOK_SYMBOL && t->symbol == SYM_COMMA) {
            commas++;
            if (val_len + 2 < BUF_SZ) { val_buf[val_len++] = ','; val_buf[val_len++] = ' '; }
            first = TRUE;
            continue;
        }

        /* Handle $IDENT — join $ + identifier as "$NAME" (variable length) */
        if (t->type == TOK_SYMBOL && t->symbol == SYM_DOLLAR) {
            PASM_TOK next_id = TOK_PEEK(cur);
            if (next_id && next_id->type == TOK_IDENTIFIER) {
                TOK_ADVANCE(cur);
                if (!first && val_len + 1 < BUF_SZ) val_buf[val_len++] = ' ';
                if (val_len + 1 < BUF_SZ) val_buf[val_len++] = '$';
                U32 nlen = STRLEN(next_id->txt);
                if (val_len + nlen < BUF_SZ) {
                    STRNCPY(val_buf + val_len, next_id->txt, BUF_SZ - val_len - 1);
                    val_len += nlen;
                }
                first = FALSE;
                continue;
            }
        }

        /* Handle SYM_MINUS followed by TOK_NUMBER: join as "-N" (no space) */
        if (t->type == TOK_SYMBOL && t->symbol == SYM_MINUS) {
            PASM_TOK next_num = TOK_PEEK(cur);
            if (next_num && next_num->type == TOK_NUMBER) {
                TOK_ADVANCE(cur);
                if (!first && val_len + 1 < BUF_SZ) val_buf[val_len++] = ' ';
                if (val_len + 1 < BUF_SZ) val_buf[val_len++] = '-';
                U32 nlen = STRLEN(next_num->txt);
                if (val_len + nlen < BUF_SZ) {
                    STRNCPY(val_buf + val_len, next_num->txt, BUF_SZ - val_len - 1);
                    val_len += nlen;
                }
                first = FALSE;
                continue;
            }
        }

        if (!first && val_len + 1 < BUF_SZ) val_buf[val_len++] = ' ';
        U32 tlen = STRLEN(t->txt);
        if (val_len + tlen < BUF_SZ) {
            STRNCPY(val_buf + val_len, t->txt, BUF_SZ - val_len - 1);
            val_len += tlen;
        }
        first = FALSE;
    }

    var->raw_value = (val_len > 0) ? STRDUP(val_buf) : NULLPTR;
    var->is_list   = (commas > 0);
    var->list_len  = commas + 1;

    PASM_NODE n = ALLOC_NODE(NODE_DATA_VAR, name_tok);
    if (!n) { MFree(var->name); MFree(var->raw_value); MFree(var); return NULLPTR; }
    n->data.var = var;
    return n;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  LOCAL LABEL PARSER
 * ════════════════════════════════════════════════════════════════════════════
 *  Entry: current token is '@' (SYM_AT).
 *
 *  Forms:
 *    @@1:   ->  {@, @, 1, :}
 *    @f     ->  {@, f}
 *    @b     ->  {@, b}
 */
STATIC PASM_NODE PARSE_LOCAL_LABEL(TOK_CURSOR *cur) {
    PASM_TOK at1 = TOK_ADVANCE(cur);    /* first '@' */
    U8  name_buf[BUF_SZ] = { 0 };
    U32 name_len = 0;

    PASM_TOK next = TOK_PEEK(cur);

    if (next && next->type == TOK_SYMBOL && next->symbol == SYM_AT) {
        /* @@<num>: */
        TOK_ADVANCE(cur);   /* consume second '@' */
        /* number or identifier follows */
        PASM_TOK id = TOK_PEEK(cur);
        if (id && (id->type == TOK_NUMBER || id->type == TOK_IDENTIFIER)) {
            TOK_ADVANCE(cur);
            STRNCPY(name_buf, "@@", BUF_SZ - 1);
            STRNCAT(name_buf, id->txt, BUF_SZ - 3);
            name_len = STRLEN(name_buf);
        }
        /* consume ':' */
        if (PEEK_SYMBOL(cur, SYM_COLON)) TOK_ADVANCE(cur);

        PASM_NODE n = ALLOC_NODE(NODE_LOCAL_LABEL, at1);
        if (!n) return NULLPTR;
        n->label.name = STRDUP(name_buf);
        return n;
    }

    /* @f or @b — standalone forward/backward reference (no-op at statement level) */
    if (next && next->type == TOK_IDENTIFIER
        && (STRICMP(next->txt, "f") == 0 || STRICMP(next->txt, "b") == 0)) {
        TOK_ADVANCE(cur);  /* consume f/b */
        return NULLPTR;    /* silently skip; only meaningful as instruction operand */
    }

    /* Unknown @ construct — treat as raw identifier */
    printf("[AST] Unexpected '@' construct at L%u C%u\n", at1->line, at1->col);
    return NULLPTR;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  MAIN AST BUILDER
 * ════════════════════════════════════════════════════════════════════════════
 */
ASM_AST_ARRAY *ASM_BUILD_AST(ASM_TOK_ARRAY *toks) {
    ASM_AST_ARRAY *arr = MAlloc(sizeof(ASM_AST_ARRAY));
    if (!arr) return NULLPTR;
    MEMSET(arr, 0, sizeof(ASM_AST_ARRAY));

    ASTRAC_ARGS   *args = GET_ARGS();
    TOK_CURSOR     cur  = { toks, 0 };
    ASM_DIRECTIVE  current_section  = DIR_NONE;
    ASM_DIRECTIVE  code_type        = DIR_CODE_TYPE_32;
    ast_code_mode = DIR_CODE_TYPE_32;

    while (!TOK_AT_END(&cur)) {
        TOK_SKIP_EOL(&cur);
        if (TOK_AT_END(&cur)) break;

        PASM_TOK t = TOK_PEEK(&cur);
        if (!t) break;

        PASM_NODE node = NULLPTR;

        /* ── '.' → dot-prefixed label  OR  section / org / times directive ── */
        if (t->type == TOK_SYMBOL && t->symbol == SYM_DOT) {
            /* Check if this is a dot-prefixed local label:  .name:
             * Peek ahead: DOT + IDENTIFIER + COLON */
            PASM_TOK dot_next  = PEEK_N(&cur, 1);
            PASM_TOK dot_colon = PEEK_N(&cur, 2);
            if (dot_next && dot_next->type == TOK_IDENTIFIER
                && dot_colon && dot_colon->type == TOK_SYMBOL
                && dot_colon->symbol == SYM_COLON) {
                PASM_TOK dot_tok = TOK_ADVANCE(&cur);  /* consume '.' */
                PASM_TOK id_tok  = TOK_ADVANCE(&cur);  /* consume identifier */
                TOK_ADVANCE(&cur);                       /* consume ':' */

                U8 label_buf[BUF_SZ] = { 0 };
                STRCPY(label_buf, ".");
                STRNCAT(label_buf, id_tok->txt, BUF_SZ - 2);

                node = ALLOC_NODE(NODE_LOCAL_LABEL, dot_tok);
                if (node) {
                    node->label.name = STRDUP(label_buf);
                    PUSH_NODE(arr, node);
                }
                continue;
            }

            /* Otherwise it's a section/org/times directive */
            node = PARSE_SECTION(&cur);
            if (node) {
                if (node->type == NODE_SECTION) {
                    ASM_DIRECTIVE new_sec = node->dir.section;
                    if (new_sec == DIR_CODE_TYPE_32 || new_sec == DIR_CODE_TYPE_16) {
                        code_type = new_sec;
                        ast_code_mode = new_sec;
                    }
                    else
                        current_section = new_sec;
                }
                PUSH_NODE(arr, node);
            }
            continue;
        }

        /* ── '@' → local label ──────────────────────────────────────── */
        if (t->type == TOK_SYMBOL && t->symbol == SYM_AT) {
            node = PARSE_LOCAL_LABEL(&cur);
            if (node) PUSH_NODE(arr, node);
            continue;
        }

        /* ── MNEMONIC → instruction ────────────────────────────────── */
        if (t->type == TOK_MNEMONIC) {
            node = PARSE_INSTRUCTION(&cur);
            if (node) PUSH_NODE(arr, node);
            continue;
        }

        /* ── IDENTIFIER → label def  OR  data variable ──────────────── */
        if (t->type == TOK_IDENTIFIER) {
            /* Look ahead: if next is ':', it's a global label */
            PASM_TOK lookahead = PEEK_N(&cur, 1);
            if (lookahead && lookahead->type == TOK_SYMBOL
                && lookahead->symbol == SYM_COLON) {
                PASM_TOK name_tok = TOK_ADVANCE(&cur);  /* name */
                TOK_ADVANCE(&cur);                       /* ':' */

                node = ALLOC_NODE(NODE_LABEL, name_tok);
                if (node) {
                    node->label.name = STRDUP(name_tok->txt);
                    PUSH_NODE(arr, node);
                }
                continue;
            }

            /* Look ahead: if next is a variable type keyword → data var */
            if (lookahead && lookahead->type == TOK_IDENT_VAR) {
                node = PARSE_DATA_VAR(&cur);
                if (node) {
                    if (current_section == DIR_NONE && !args->quiet)
                        printf("[AST] Warning: variable declared outside a data section at L%u\n",
                            node->line);
                    PUSH_NODE(arr, node);
                }
                continue;
            }

            /* Otherwise, emit as raw identifier token — skip for now */
            if (!args->quiet)
                printf("[AST] Unexpected identifier '%s' at L%u C%u\n",
                    t->txt, t->line, t->col);
            TOK_ADVANCE(&cur);
            continue;
        }

        /* ── NUMBER in .code → NODE_RAW_NUM ────────────────────────── */
        if (t->type == TOK_NUMBER) {
            PASM_TOK num_tok = TOK_ADVANCE(&cur);
            node = ALLOC_NODE(NODE_RAW_NUM, num_tok);
            if (node) {
                node->raw.value = num_tok->num_val;
                PUSH_NODE(arr, node);
            }
            continue;
        }

        /* ── Anything else — skip ─────────────────────────────────── */
        if (!args->quiet)
            printf("[AST] Skipping unexpected token '%s' (%s) at L%u C%u\n",
                t->txt, TOKEN_TYPE_STR(t->type), t->line, t->col);
        TOK_ADVANCE(&cur);
    }

    return arr;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  DESTROY_AST_ARR  —  deep free of the AST
 * ════════════════════════════════════════════════════════════════════════════
 */
VOID DESTROY_AST_ARR(ASM_AST_ARRAY *ast) {
    if (!ast) return;

    for (U32 i = 0; i < ast->len; i++) {
        PASM_NODE n = ast->nodes[i];
        if (!n) continue;

        switch (n->type) {
            case NODE_LABEL:
            case NODE_LOCAL_LABEL:
                if (n->label.name) MFree(n->label.name);
                break;

            case NODE_DATA_VAR:
                if (n->data.var) {
                    PASM_VAR v = n->data.var;
                    if (v->name)      MFree(v->name);
                    if (v->raw_value) MFree(v->raw_value);
                    MFree(v);
                }
                break;

            case NODE_INSTRUCTION:
                for (U32 j = 0; j < n->instr.operand_count; j++) {
                    ASM_OPERAND *op = &n->instr.operands[j];
                    if ((op->type == OP_MEM || op->type == OP_PTR) && op->mem_ref) {
                        if (op->mem_ref->symbol_name) MFree(op->mem_ref->symbol_name);
                        MFree(op->mem_ref);
                    }
                }
                break;

            case NODE_TIMES:
                if (n->times.count_expr) MFree(n->times.count_expr);
                if (n->times.fill_raw)   MFree(n->times.fill_raw);
                break;

            default:
                break;
        }

        MFree(n);
    }

    MFree(ast);
}
