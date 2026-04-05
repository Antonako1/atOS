#include <PROGRAMS/ASTRAC/ASTRAC.h>
#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  LABEL / SYMBOL TRACKING
 * ════════════════════════════════════════════════════════════════════════════
 */
typedef struct {
    PU8 name;
    U32 offset;
    U32 byte_size;          /* byte size for variables (used for $VAR) */
    ASM_DIRECTIVE section;
} ASM_PTR;

#define MAX_PTRS 1024

typedef struct {
    U32 code;
    U32 data;
    U32 rodata;
    U32 origin;             /* .org base address    */
    ASM_DIRECTIVE current_section;
    ASM_DIRECTIVE code_type;
    ASM_PTR ptrs[MAX_PTRS];
    U32 arr_tail;
} ASM_PTR_ARRAY;

STATIC ASM_PTR_ARRAY ptrs;

#define FIRST_PASS 0
#define SECOND_PASS 1
STATIC U8 CURRENT_PASS ATTRIB_DATA = FIRST_PASS; /* 0 = first pass, 1 = second pass (resolving labels) */
/*
 * ════════════════════════════════════════════════════════════════════════════
 *  HELPERS
 * ════════════════════════════════════════════════════════════════════════════
 */

/* Current output offset for whatever section we're in. */
STATIC U32 CURRENT_OFFSET(VOID) {
    switch (ptrs.current_section) {
        case DIR_CODE: case DIR_CODE_TYPE_32: case DIR_CODE_TYPE_16:
            return ptrs.code;
        case DIR_DATA:   return ptrs.data;
        case DIR_RODATA: return ptrs.rodata;
        default:         return ptrs.code;
    }
}

/* Write bytes to output and advance the section offset. */
STATIC VOID EMIT(FILE *file, const U8 *buf, U32 len) {
    if (file) FWRITE(file, (VOIDPTR)buf, len);

    switch (ptrs.current_section) {
        case DIR_CODE: case DIR_CODE_TYPE_32: case DIR_CODE_TYPE_16:
            ptrs.code += len; break;
        case DIR_DATA:   ptrs.data   += len; break;
        case DIR_RODATA: ptrs.rodata += len; break;
        default: break;
    }
}

/* Convenience: emit a single byte. */
STATIC VOID EMIT_U8(FILE *f, U8 val) { EMIT(f, &val, 1); }

/* Emit a little-endian 16-bit value. */
STATIC VOID EMIT_U16(FILE *f, U16 val) {
    U8 buf[2] = { val & 0xFF, (val >> 8) & 0xFF };
    EMIT(f, buf, 2);
}

/* Emit a little-endian 32-bit value. */
STATIC VOID EMIT_U32(FILE *f, U32 val) {
    U8 buf[4] = { val & 0xFF, (val >> 8) & 0xFF,
                  (val >> 16) & 0xFF, (val >> 24) & 0xFF };
    EMIT(f, buf, 4);
}

/* Emit an immediate of the given size (little-endian). */
STATIC VOID EMIT_IMM(FILE *f, U32 val, ASM_OPERAND_SIZE sz) {
    DEBUGM_PRINTF("[ASM GEN] Emitting immediate value 0x%X with size %u bits\n", val, sz);
    switch (sz) {
        case SZ_8BIT:  EMIT_U8(f, (U8)val);   break;
        case SZ_16BIT: EMIT_U16(f, (U16)val);  break;
        case SZ_32BIT: EMIT_U32(f, val);       break;
        default:       EMIT_U32(f, val);       break;
    }
}


/*
 * ── Label table ──────────────────────────────────────────────────────────────
 */
STATIC BOOL ADD_ASM_PTR(PU8 name, U32 offset, ASM_DIRECTIVE section) {
    /* Update existing entry if already present */
    for (U32 i = 0; i < ptrs.arr_tail; i++) {
        if (ptrs.ptrs[i].name && STRICMP(ptrs.ptrs[i].name, name) == 0) {
            ptrs.ptrs[i].offset  = offset;
            ptrs.ptrs[i].section = section;
            return TRUE;
        }
    }
    if (ptrs.arr_tail >= MAX_PTRS) {
        printf("[ASM GEN] Error: Exceeded maximum number of labels (%u)\n", MAX_PTRS);
        return FALSE;
    }
    ptrs.ptrs[ptrs.arr_tail].name      = name;
    ptrs.ptrs[ptrs.arr_tail].offset    = offset;
    ptrs.ptrs[ptrs.arr_tail].byte_size = 0;
    ptrs.ptrs[ptrs.arr_tail].section   = section;
    ptrs.arr_tail++;
    return TRUE;
}

STATIC ASM_PTR *FIND_ASM_PTR(PU8 name) {
    for (U32 i = 0; i < ptrs.arr_tail; i++) {
        if (ptrs.ptrs[i].name && STRICMP(ptrs.ptrs[i].name, name) == 0)
            return &ptrs.ptrs[i];
    }
    return NULLPTR;
}

/*
 * Resolve a symbol name to its file offset (NOT including origin).
 * Handles special symbols: $ (current offset), $$ (0 / section start).
 */
STATIC U32 RESOLVE_SYMBOL(PU8 name) {
    if (!name) return 0;
    if (STRCMP(name, "$")  == 0) return CURRENT_OFFSET();
    if (STRCMP(name, "$$") == 0) return 0;  /* section start = file offset 0 */
    ASM_PTR *p = FIND_ASM_PTR(name);
    if (!p) {
        if(CURRENT_PASS == SECOND_PASS)
            printf("[ASM GEN] ERROR: undefined symbol '%s'\n", name);
        return 0;
    }
    return p->offset;
}

/* Resolve a symbol to its absolute address (file offset + origin). */
STATIC U32 RESOLVE_SYMBOL_ADDR(PU8 name) {
    return RESOLVE_SYMBOL(name) + ptrs.origin;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  EXPRESSION EVALUATOR
 * ════════════════════════════════════════════════════════════════════════════
 *  Simple recursive-descent evaluator for arithmetic expressions in
 *  .times count, .org value, and data initializers.
 *
 *  Supports:  numbers (dec / 0x hex / 0b bin),  $  $$  $IDENT
 *             +  -  *  /  (  )  unary -
 */
STATIC U32 eval_add_sub(PU8 *pp);   /* forward decl */

STATIC VOID skip_ws(PU8 *pp) {
    while (**pp == ' ' || **pp == '\t') (*pp)++;
}

STATIC U32 parse_num_expr(PU8 *pp) {
    skip_ws(pp);
    PU8 p = *pp;

    /* Hex */
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
        U32 val = 0;
        while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f')
               || (*p >= 'A' && *p <= 'F')) {
            U32 d;
            if (*p >= '0' && *p <= '9')      d = *p - '0';
            else if (*p >= 'a' && *p <= 'f') d = *p - 'a' + 10;
            else                             d = *p - 'A' + 10;
            val = (val << 4) | d;
            p++;
        }
        *pp = p;
        return val;
    }

    /* Binary */
    if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) {
        p += 2;
        U32 val = 0;
        while (*p == '0' || *p == '1') {
            val = (val << 1) | (*p - '0');
            p++;
        }
        *pp = p;
        return val;
    }

    /* Decimal */
    U32 val = 0;
    while (*p >= '0' && *p <= '9') {
        val = val * 10 + (*p - '0');
        p++;
    }
    *pp = p;
    return val;
}

STATIC U32 eval_atom(PU8 *pp) {
    skip_ws(pp);

    /* Unary minus */
    if (**pp == '-') {
        (*pp)++;
        return (U32)(-(S32)eval_atom(pp));
    }

    /* Parentheses */
    if (**pp == '(') {
        (*pp)++;
        U32 val = eval_add_sub(pp);
        skip_ws(pp);
        if (**pp == ')') (*pp)++;
        return val;
    }

    /* $$ (origin / section start) */
    if ((*pp)[0] == '$' && (*pp)[1] == '$') {
        *pp += 2;
        return ptrs.origin;
    }

    /* $IDENT (variable byte size) */
    if ((*pp)[0] == '$' && (((*pp)[1] >= 'A' && (*pp)[1] <= 'Z')
                         || ((*pp)[1] >= 'a' && (*pp)[1] <= 'z')
                         || (*pp)[1] == '_')) {
        (*pp)++;   /* skip $ */
        U8 name[128] = { 0 };
        U32 n = 0;
        while (((**pp >= 'A' && **pp <= 'Z') || (**pp >= 'a' && **pp <= 'z')
                || (**pp >= '0' && **pp <= '9') || **pp == '_') && n < 127) {
            name[n++] = **pp;
            (*pp)++;
        }
        ASM_PTR *p = FIND_ASM_PTR(name);
        return p ? p->byte_size : 0;
    }

    /* $ (current offset + origin) */
    if (**pp == '$') {
        (*pp)++;
        return CURRENT_OFFSET() + ptrs.origin;
    }

    /* Number literal */
    return parse_num_expr(pp);
}

STATIC U32 eval_mul_div(PU8 *pp) {
    U32 v = eval_atom(pp);
    skip_ws(pp);
    while (**pp == '*' || **pp == '/') {
        U8 op = **pp;
        (*pp)++;
        U32 r = eval_atom(pp);
        if (op == '*') v *= r;
        else if (r != 0) v /= r;
        skip_ws(pp);
    }
    return v;
}

STATIC U32 eval_add_sub(PU8 *pp) {
    U32 v = eval_mul_div(pp);
    skip_ws(pp);
    while (**pp == '+' || **pp == '-') {
        U8 op = **pp;
        (*pp)++;
        U32 r = eval_mul_div(pp);
        if (op == '+') v += r;
        else v -= r;
        skip_ws(pp);
    }
    return v;
}

/* Evaluate a text expression to a U32 value. */
STATIC U32 EVAL_EXPR(PU8 text) {
    if (!text || !*text) return 0;
    PU8 p = text;
    return eval_add_sub(&p);
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  x86 REGISTER ENCODING
 * ════════════════════════════════════════════════════════════════════════════
 *  Returns the 3-bit register number used in ModR/M, SIB, and +rb/+rd.
 */
STATIC U8 REG_NUM(ASM_REGS reg) {
    switch (reg) {
        case REG_AL: case REG_AX: case REG_EAX: case REG_ES: case REG_CR0:
        case REG_ST0: case REG_MM0: case REG_XMM0:
            return 0;
        case REG_CL: case REG_CX: case REG_ECX: case REG_CS: case REG_CR2:
        case REG_ST1: case REG_MM1: case REG_XMM1:
            return 1;
        case REG_DL: case REG_DX: case REG_EDX: case REG_SS: case REG_CR3:
        case REG_ST2: case REG_MM2: case REG_XMM2:
            return 2;
        case REG_BL: case REG_BX: case REG_EBX: case REG_DS: case REG_CR4:
        case REG_ST3: case REG_MM3: case REG_XMM3:
            return 3;
        case REG_AH: case REG_SP: case REG_ESP: case REG_FS:
        case REG_ST4: case REG_MM4: case REG_XMM4:
            return 4;
        case REG_CH: case REG_BP: case REG_EBP: case REG_GS:
        case REG_ST5: case REG_MM5: case REG_XMM5:
            return 5;
        case REG_DH: case REG_SI: case REG_ESI:
        case REG_ST6: case REG_MM6: case REG_XMM6:
            return 6;
        case REG_BH: case REG_DI: case REG_EDI:
        case REG_ST7: case REG_MM7: case REG_XMM7:
            return 7;
        default:
            return 0;
    }
}

/* Build a ModR/M byte:  mod(2) | reg(3) | rm(3) */
STATIC U8 MODRM(U8 mod, U8 reg, U8 rm) {
    return (U8)((mod << 6) | ((reg & 7) << 3) | (rm & 7));
}

/* Build a SIB byte:  scale(2) | index(3) | base(3) */
STATIC U8 SIB(U8 scale, U8 index, U8 base) {
    U8 ss;
    switch (scale) {
        case 1: ss = 0; break;
        case 2: ss = 1; break;
        case 4: ss = 2; break;
        case 8: ss = 3; break;
        default: ss = 0; break;
    }
    return (U8)((ss << 6) | ((index & 7) << 3) | (base & 7));
}

/* Does this register require a SIB byte when used as r/m base? (ESP) */
STATIC BOOL NEEDS_SIB(ASM_REGS reg) {
    return (reg == REG_ESP || reg == REG_SP);
}

/* Is this a 16-bit base or index register? */
STATIC BOOL IS_REG16(ASM_REGS reg) {
    return reg == REG_AX || reg == REG_BX || reg == REG_CX || reg == REG_DX
        || reg == REG_SI || reg == REG_DI || reg == REG_BP || reg == REG_SP;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  EMIT 16-BIT ModR/M + DISPLACEMENT
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  16-bit addressing uses a completely different r/m encoding (no SIB):
 *    r/m=000  [BX+SI]      r/m=100  [SI]
 *    r/m=001  [BX+DI]      r/m=101  [DI]
 *    r/m=010  [BP+SI]      r/m=110  [BP+disp] or [disp16] if mod=00
 *    r/m=011  [BP+DI]      r/m=111  [BX]
 *
 *  Displacement is 0 (mod=00), 8-bit (mod=01), or 16-bit (mod=10).
 */
STATIC VOID EMIT_MODRM_16(FILE *f, U8 reg_field, ASM_OPERAND *op) {
    if (op->type == OP_REG || op->type == OP_SEG) {
        /* mod=11 register-direct — same as 32-bit */
        EMIT_U8(f, MODRM(3, reg_field, REG_NUM(op->reg)));
        return;
    }

    ASM_NODE_MEM *mem = op->mem_ref;
    if (!mem) { EMIT_U8(f, MODRM(0, reg_field, 6)); EMIT_U16(f, 0); return; }

    /* Pure displacement (no base, no index) — mod=00 r/m=110 [disp16] */
    if (mem->base_reg == REG_NONE && mem->index_reg == REG_NONE) {
        EMIT_U8(f, MODRM(0, reg_field, 6));
        if (mem->symbol_name)
            EMIT_U16(f, (U16)RESOLVE_SYMBOL_ADDR(mem->symbol_name));
        else
            EMIT_U16(f, (U16)mem->displacement);
        return;
    }

    S32 disp = mem->displacement;
    if (mem->symbol_name)
        disp += (S32)RESOLVE_SYMBOL_ADDR(mem->symbol_name);

    /* Map base+index to r/m field */
    ASM_REGS base = mem->base_reg;
    ASM_REGS idx  = mem->index_reg;
    U8 rm = 0;
    BOOL found = FALSE;

    /* Two-register combinations */
    if (base != REG_NONE && idx != REG_NONE) {
        if      ((base==REG_BX&&idx==REG_SI)||(base==REG_SI&&idx==REG_BX)) { rm=0; found=TRUE; }
        else if ((base==REG_BX&&idx==REG_DI)||(base==REG_DI&&idx==REG_BX)) { rm=1; found=TRUE; }
        else if ((base==REG_BP&&idx==REG_SI)||(base==REG_SI&&idx==REG_BP)) { rm=2; found=TRUE; }
        else if ((base==REG_BP&&idx==REG_DI)||(base==REG_DI&&idx==REG_BP)) { rm=3; found=TRUE; }
    }

    /* Single-register */
    if (!found) {
        ASM_REGS r = (base != REG_NONE) ? base : idx;
        switch (r) {
            case REG_SI: rm = 4; found = TRUE; break;
            case REG_DI: rm = 5; found = TRUE; break;
            case REG_BP: rm = 6; found = TRUE; break;
            case REG_BX: rm = 7; found = TRUE; break;
            default:
                /* Fall back — shouldn't happen in valid 16-bit code */
                rm = 6; found = TRUE;
                break;
        }
    }

    /* Determine mod field */
    U8 mod;
    if (disp == 0 && rm != 6) {
        mod = 0;    /* [BP] always needs mod=01 minimum (rm=6 mod=00 means [disp16]) */
    } else if (disp >= -128 && disp <= 127) {
        mod = 1;    /* disp8 */
    } else {
        mod = 2;    /* disp16 */
    }

    EMIT_U8(f, MODRM(mod, reg_field, rm));

    if (mod == 1) EMIT_U8(f, (U8)(disp & 0xFF));
    else if (mod == 2) EMIT_U16(f, (U16)(disp & 0xFFFF));
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  EMIT ModR/M + SIB + DISPLACEMENT (32-bit addressing)
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  reg_field: the /r or /digit value (3 bits placed in ModR/M reg field).
 *  operand:   the r/m side operand (OP_REG or OP_MEM).
 */
STATIC VOID EMIT_MODRM(FILE *f, U8 reg_field, ASM_OPERAND *op) {
    if (op->type == OP_REG || op->type == OP_SEG) {
        /* mod=11  register-direct */
        EMIT_U8(f, MODRM(3, reg_field, REG_NUM(op->reg)));
        return;
    }

    /* OP_MEM — memory reference */
    ASM_NODE_MEM *mem = op->mem_ref;
    if (!mem) { EMIT_U8(f, MODRM(0, reg_field, 5)); EMIT_U32(f, 0); return; }

    /* Pure displacement (no base, no index) — [disp32] or [symbol] */
    if (mem->base_reg == REG_NONE && mem->index_reg == REG_NONE) {
        EMIT_U8(f, MODRM(0, reg_field, 5));
        if (mem->symbol_name) {
            EMIT_U32(f, RESOLVE_SYMBOL_ADDR(mem->symbol_name));
        } else {
            EMIT_U32(f, (U32)mem->displacement);
        }
        return;
    }

    S32 disp = mem->displacement;
    if (mem->symbol_name) {
        disp += (S32)RESOLVE_SYMBOL_ADDR(mem->symbol_name);
    }

    U8 base_num = (mem->base_reg != REG_NONE) ? REG_NUM(mem->base_reg) : 0;

    /* Determine mod field from displacement size */
    U8 mod;
    if (disp == 0 && base_num != 5) {   /* EBP always needs disp8 at minimum */
        mod = 0;
    } else if (disp >= -128 && disp <= 127) {
        mod = 1;    /* disp8 */
    } else {
        mod = 2;    /* disp32 */
    }

    /* SIB required if base is ESP/SP or an index register is present */
    if (mem->index_reg != REG_NONE || NEEDS_SIB(mem->base_reg)) {
        U8 idx = (mem->index_reg != REG_NONE) ? REG_NUM(mem->index_reg) : 4; /* 4=none */
        EMIT_U8(f, MODRM(mod, reg_field, 4));   /* r/m = 100 → SIB follows */
        EMIT_U8(f, SIB((U8)mem->scale, idx, base_num));
    } else {
        EMIT_U8(f, MODRM(mod, reg_field, base_num));
    }

    /* Emit displacement */
    if (mod == 1) EMIT_U8(f, (U8)(disp & 0xFF));
    else if (mod == 2) EMIT_U32(f, (U32)disp);
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  ENCODE ONE INSTRUCTION
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  Byte stream layout: [prefix] [0F] opcode [ModR/M [SIB [disp]]] [imm]
 *
 *  ENC_DIRECT      — opcode only (e.g. NOP, RET, HLT)
 *  ENC_REG_OPCODE  — opcode + rd/rw in low 3 bits, then optional imm
 *  ENC_IMM         — opcode + immediate value only (e.g. INT, PUSH imm)
 *  ENC_MODRM       — opcode + ModR/M (± SIB ± disp) + optional imm
 */
STATIC BOOL ENCODE_INSTRUCTION(FILE *f, PASM_NODE node) {
    const ASM_MNEMONIC_TABLE *tbl = node->instr.table_entry;
    if (!tbl) {
        printf("[ASM GEN] Line %u: Instruction has no table entry — skipping\n",
               node->line);
        return TRUE;    /* non-fatal: already diagnosed by AST builder */
    }

    DEBUGM_PRINTF("[ASM GEN] Encoding '%s' (opcode 0x%02X) at line %u\n",
                 tbl->name, tbl->opcode[0], node->line);

    /* ── 1. Legacy prefix ──────────────────────────────────────────────
     *  In 32-bit mode: 0x66 for 16-bit instructions  (table says PF_66)
     *  In 16-bit mode: skip 0x66 for 16-bit (native), add 0x66 for 32-bit
     *  REP/REPNE (F2/F3) always emit regardless of mode.
     */
    if (tbl->prefix == PF_66) {
        /* 16-bit instruction — only emit prefix in 32-bit mode */
        if (ptrs.code_type != DIR_CODE_TYPE_16)
            EMIT_U8(f, 0x66);
    } else if (tbl->prefix == PF_F2 || tbl->prefix == PF_F3) {
        EMIT_U8(f, (U8)tbl->prefix);
    } else if (tbl->prefix == PF_NONE && tbl->size == SZ_32BIT
               && ptrs.code_type == DIR_CODE_TYPE_16) {
        /* 32-bit instruction in 16-bit mode — needs override prefix */
        EMIT_U8(f, 0x66);
    }

    /* ── 2. Segment override (if first mem operand has segment set) ──── */
    for (U32 k = 0; k < node->instr.operand_count; k++) {
        ASM_OPERAND *op = &node->instr.operands[k];
        if (op->type == OP_MEM && op->mem_ref && op->mem_ref->segment != REG_NONE) {
            switch (op->mem_ref->segment) {
                case REG_ES: EMIT_U8(f, 0x26); break;
                case REG_CS: EMIT_U8(f, 0x2E); break;
                case REG_SS: EMIT_U8(f, 0x36); break;
                case REG_DS: EMIT_U8(f, 0x3E); break;
                case REG_FS: EMIT_U8(f, 0x64); break;
                case REG_GS: EMIT_U8(f, 0x65); break;
                default: break;
            }
            break;  /* only the first override */
        }
    }

    /* ── 3. Opcode byte(s) ────────────────────────────────────────────── */
    switch (tbl->encoding) {

    case ENC_DIRECT:
        /* Single opcode byte, no operands. */
        EMIT_U8(f, tbl->opcode[0]);
        break;

    case ENC_REG_OPCODE: {
        /* Opcode + register number in low 3 bits (+rb / +rw / +rd).
         * First (or only) operand is the register. */
        U8 opc = tbl->opcode[0];
        if (node->instr.operand_count > 0 &&
            node->instr.operands[0].type == OP_REG) {
            opc += REG_NUM(node->instr.operands[0].reg);
        }
        if (tbl->opcode_prefix == PFX_0F) EMIT_U8(f, 0x0F);
        EMIT_U8(f, opc);

        /* If there's a second immediate operand (e.g. MOV r32, imm32) */
        if (node->instr.operand_count >= 2 &&
            node->instr.operands[1].type == OP_IMM) {
            EMIT_IMM(f, node->instr.operands[1].immediate, tbl->size);
        }
        /* Label/symbol reference in immediate slot (e.g. MOV r32, label) */
        else if (node->instr.operand_count >= 2 &&
                 node->instr.operands[1].type == OP_PTR &&
                 node->instr.operands[1].mem_ref &&
                 node->instr.operands[1].mem_ref->symbol_name) {
            U32 addr = RESOLVE_SYMBOL_ADDR(node->instr.operands[1].mem_ref->symbol_name);
            EMIT_IMM(f, addr, tbl->size);
        }
        break;
    }

    case ENC_IMM:
        /* Opcode followed by immediate value(s).
         * Used for INT, PUSH imm, RET imm, and also relative jumps. */
        if (tbl->opcode_prefix == PFX_0F) EMIT_U8(f, 0x0F);
        EMIT_U8(f, tbl->opcode[0]);

        /* Emit each immediate / pointer operand */
        for (U32 k = 0; k < node->instr.operand_count; k++) {
            ASM_OPERAND *op = &node->instr.operands[k];
            if (op->type == OP_IMM) {
                /* Relative jumps: compute displacement from current position */
                if (tbl->rel_type != RL_NONE) {
                    ASM_OPERAND_SIZE rsz;
                    switch (tbl->rel_type) {
                        case RL_REL8:  rsz = SZ_8BIT;  break;
                        case RL_REL16: rsz = SZ_16BIT; break;
                        case RL_REL32:
                            /* In 16-bit code mode, near rel uses 16-bit disp */
                            rsz = (ptrs.code_type == DIR_CODE_TYPE_16)
                                ? SZ_16BIT : SZ_32BIT;
                            break;
                        default:       rsz = tbl->size; break;
                    }
                    /* For now, emit the raw value; a link pass would fix these. */
                    EMIT_IMM(f, op->immediate, rsz);
                } else {
                    EMIT_IMM(f, op->immediate, tbl->size);
                }
            } else if (op->type == OP_PTR && op->mem_ref && op->mem_ref->symbol_name) {
                /* Label reference in an immediate slot (e.g. CALL label) */
                U32 target = RESOLVE_SYMBOL(op->mem_ref->symbol_name);
                if (tbl->rel_type != RL_NONE) {
                    /* Relative: displacement = target − (current + imm_size) */
                    ASM_OPERAND_SIZE rsz;
                    U32 imm_bytes;
                    switch (tbl->rel_type) {
                        case RL_REL8:  rsz = SZ_8BIT;  imm_bytes = 1; break;
                        case RL_REL16: rsz = SZ_16BIT; imm_bytes = 2; break;
                        case RL_REL32:
                            /* In 16-bit code mode, near rel uses 16-bit disp */
                            if (ptrs.code_type == DIR_CODE_TYPE_16) {
                                rsz = SZ_16BIT; imm_bytes = 2;
                            } else {
                                rsz = SZ_32BIT; imm_bytes = 4;
                            }
                            break;
                        default:       rsz = SZ_32BIT; imm_bytes = 4; break;
                    }
                    S32 rel = (S32)target - (S32)(CURRENT_OFFSET() + imm_bytes);
                    EMIT_IMM(f, (U32)rel, rsz);
                } else {
                    /* Absolute: include origin */
                    EMIT_IMM(f, target + ptrs.origin, tbl->size);
                }
            } else if (op->type == OP_FAR) {
                /* Far pointer: lower 16 = offset, upper 16 = segment */
                U16 off = (U16)(op->immediate & 0xFFFF);
                U16 seg = (U16)(op->immediate >> 16);
                EMIT_U8(f, (U8)(off & 0xFF));
                EMIT_U8(f, (U8)(off >> 8));
                EMIT_U8(f, (U8)(seg & 0xFF));
                EMIT_U8(f, (U8)(seg >> 8));
            }
        }
        break;

    case ENC_MODRM: {
        /* Opcode + ModR/M + optional SIB/displacement + optional immediate.
         *
         * modrm_ext != MODRM_NONE  →  /digit (reg field = extension digit)
         * modrm_ext == MODRM_NONE  →  /r     (reg field = register operand)
         *
         * Operand layout depends on the OPS_* macro used:
         *   r/m, r   — operand[0]=r/m  operand[1]=reg  → encode /r
         *   r, r/m   — operand[0]=reg  operand[1]=r/m  → encode /r
         *   r/m, imm — operand[0]=r/m  operand[1]=imm  → encode /digit
         *   r/m only — operand[0]=r/m                   → encode /digit
         */
        if (tbl->opcode_prefix == PFX_0F) EMIT_U8(f, 0x0F);
        EMIT_U8(f, tbl->opcode[0]);

        /* Figure out which operand is the r/m side and which is the /r reg. */
        ASM_OPERAND *rm_op  = NULLPTR;
        ASM_OPERAND *reg_op = NULLPTR;
        ASM_OPERAND *imm_op = NULLPTR;
        U8 reg_field = 0;

        if (tbl->modrm_ext != MODRM_NONE) {
            /* /digit form: the ModR/M reg field is the extension digit */
            reg_field = (U8)tbl->modrm_ext;
            if (node->instr.operand_count >= 1)
                rm_op = &node->instr.operands[0];
            if (node->instr.operand_count >= 2 &&
                node->instr.operands[1].type == OP_IMM)
                imm_op = &node->instr.operands[1];
        } else {
            /* /r form: one operand supplies the reg field, the other r/m */
            if (node->instr.operand_count >= 2) {
                /* Determine direction from the operand type layout:
                 *   tbl->operand[0]==OP_MEM  → op0=r/m, op1=reg
                 *   tbl->operand[0]==OP_REG  → op0=reg, op1=r/m */
                if (tbl->operand[0] == OP_REG || tbl->operand[0] == OP_SEG) {
                    reg_op = &node->instr.operands[0];
                    rm_op  = &node->instr.operands[1];
                } else {
                    rm_op  = &node->instr.operands[0];
                    reg_op = &node->instr.operands[1];
                }
                reg_field = (reg_op && (reg_op->type == OP_REG || reg_op->type == OP_SEG))
                          ? REG_NUM(reg_op->reg) : 0;
            } else if (node->instr.operand_count == 1) {
                rm_op = &node->instr.operands[0];
                reg_field = 0;
            }
        }

        /* Emit ModR/M (+ SIB + displacement) */
        if (rm_op) {
            /* Choose 16-bit or 32-bit addressing.
             *
             * Default comes from the current code mode (.use16 / .use32).
             * If the memory operand explicitly uses registers, the register
             * width overrides the default (e.g. [eax] in .use16 → 32-bit,
             * [bx+si] in .use32 → 16-bit).
             */
            BOOL use_16 = (ptrs.code_type == DIR_CODE_TYPE_16);
            if (rm_op->type == OP_MEM && rm_op->mem_ref) {
                ASM_REGS b = rm_op->mem_ref->base_reg;
                ASM_REGS x = rm_op->mem_ref->index_reg;
                if (b != REG_NONE || x != REG_NONE) {
                    /* Registers present — they dictate addressing mode */
                    use_16 = ((b != REG_NONE && IS_REG16(b))
                           || (x != REG_NONE && IS_REG16(x)));
                }
            }
            if (use_16)
                EMIT_MODRM_16(f, reg_field, rm_op);
            else
                EMIT_MODRM(f, reg_field, rm_op);
        }

        /* Emit trailing immediate if present */
        if (imm_op) {
            /* Determine immediate size:
             *   If table operand[1] is OP_IMM and the OPS macro encodes a
             *   smaller imm (e.g. OPS_RM32_IMM8), we need the imm size,
             *   not the instruction's overall size.
             *   Convention: if opcode is 83h/6Bh, imm is always 8-bit
             *   sign-extended regardless of tbl->size. */
            U8 opc = tbl->opcode[0];
            if (tbl->opcode_prefix == PFX_0F) opc = tbl->opcode[1];
            ASM_OPERAND_SIZE imm_sz = tbl->size;
            if (opc == 0x83 || opc == 0x6B || opc == 0xC0 || opc == 0xC1) {
                imm_sz = SZ_8BIT;   /* sign-extended imm8 / shift-rotate imm8 */
            }
            EMIT_IMM(f, imm_op->immediate, imm_sz);
        }
        break;
    }

    default:
        printf("[ASM GEN] Line %u: Unknown encoding type %u for '%s'\n",
               node->line, tbl->encoding, tbl->name);
        return FALSE;
    }

    return TRUE;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  ENCODE DATA VARIABLE
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  Emits the raw binary data for DB / DW / DD declarations.
 *  Writes each initialiser element according to the declared type width.
 */
STATIC BOOL ENCODE_DATA_VAR(FILE *f, PASM_NODE node) {
    PASM_VAR var = node->data.var;
    if (!var || !var->name) return FALSE;

    /* Record the variable's address in the label table */
    ADD_ASM_PTR(var->name, CURRENT_OFFSET(), ptrs.current_section);

    DEBUGM_PRINTF("[ASM GEN] Variable '%s' at offset 0x%X with raw value: %s\n",
                 var->name, CURRENT_OFFSET(), var->raw_value ? var->raw_value : "(null)");

    if (!var->raw_value) return TRUE;  /* no initialiser */

    /* Determine element byte-width from type */
    U32 elem_sz;
    switch (var->var_type) {
        case TYPE_BYTE:  elem_sz = 1; break;
        case TYPE_WORD:  elem_sz = 2; break;
        case TYPE_DWORD: case TYPE_FLOAT: case TYPE_PTR:
            elem_sz = 4; break;
        default: elem_sz = 1; break;
    }

    /* DB with a string literal — raw bytes (already includes any trailing 0) */
    if (var->var_type == TYPE_BYTE && var->raw_value[0] == '"') {
        PU8 p = var->raw_value + 1;   /* skip opening quote */
        while (*p && *p != '"') {
            U8 ch = *p;
            if (ch == '\\' && *(p + 1)) {
                p++;
                switch (*p) {
                    case 'n': ch = '\n'; break;
                    case 'r': ch = '\r'; break;
                    case 't': ch = '\t'; break;
                    case '0': ch = '\0'; break;
                    case '\\': ch = '\\'; break;
                    case '"': ch = '"'; break;
                    default:  ch = *p; break;
                }
            }
            EMIT_U8(f, ch);
            p++;
        }
        /* Handle trailing comma-separated values after the string
         * e.g.  msg DB "Hello", 0  → raw_value = `"Hello", 0`
         * We scan past the closing quote + comma and parse remaining numbers. */
        if (*p == '"') p++;
        while (*p) {
            while (*p == ',' || *p == ' ') p++;
            if (!*p) break;
            U32 num = 0;
            if (ATOI_E(p, &num) || ATOI_HEX_E(p, &num) || ATOI_BIN_E(p, &num)) {
                EMIT_U8(f, (U8)num);
            }
            /* Advance to next comma or end */
            while (*p && *p != ',') p++;
        }
        return TRUE;
    }

    /* REAL4 — IEEE 754 single-precision float literal(s).
     * raw_value may look like "3 . 14" (tokens joined with spaces) or
     * "-2 . 718" — we strip internal spaces before parsing.
     * Supports comma-separated lists: "3 . 14, -2 . 718, 0 . 0"
     */
    if (var->var_type == TYPE_FLOAT) {
        /* Build a clean copy with spaces removed */
        U8 clean[BUF_SZ];
        U32 ci = 0;
        for (PU8 s = var->raw_value; *s && ci < BUF_SZ - 1; s++) {
            if (*s != ' ' && *s != '\t') clean[ci++] = *s;
        }
        clean[ci] = '\0';

        PU8 p = clean;
        while (*p) {
            while (*p == ',') p++;
            if (!*p) break;

            /* Parse a float value manually:  [-]digits[.digits] */
            BOOL neg = FALSE;
            if (*p == '-') { neg = TRUE; p++; }
            else if (*p == '+') p++;

            /* Integer part */
            U32 int_part = 0;
            while (*p >= '0' && *p <= '9') {
                int_part = int_part * 10 + (*p - '0');
                p++;
            }

            /* Fractional part */
            U32 frac_num = 0;
            U32 frac_div = 1;
            if (*p == '.') {
                p++;
                while (*p >= '0' && *p <= '9') {
                    frac_num = frac_num * 10 + (*p - '0');
                    frac_div *= 10;
                    p++;
                }
            }

            /* Convert to float (simple conversion — no libc strtof needed) */
            /* Compute value = int_part + frac_num / frac_div */
            /* Use integer arithmetic to approximate IEEE 754 single-precision:
             *   float = (-1)^s × 2^(e-127) × 1.mantissa
             * For simplicity, use a union-based cast with manual computation. */
            U32 ieee_bits = 0;
            U32 total_scaled = int_part * frac_div + frac_num; /* value * frac_div */
            if (total_scaled == 0) {
                ieee_bits = neg ? 0x80000000u : 0x00000000u;
            } else {
                /* Normalise: find the position of the MSB in total_scaled */
                U32 val = total_scaled;
                S32 exp = 0;

                /* Scale val so it is in the range [1*frac_div, 2*frac_div) */
                /* i.e. compute log2(total_scaled / frac_div) */
                /* First: find overall bit-width of total_scaled */
                U32 msb = 0;
                U32 tmp = val;
                while (tmp > 1) { tmp >>= 1; msb++; }

                /* Also find bit-width of frac_div */
                U32 div_msb = 0;
                tmp = frac_div;
                while (tmp > 1) { tmp >>= 1; div_msb++; }

                /* exponent = msb - div_msb (as float = val / frac_div) */
                exp = (S32)msb - (S32)div_msb;

                /* Align mantissa bits:
                 * We want 23 bits of mantissa from: val / frac_div
                 * Shift val left enough to get 24 significant bits (including implicit 1)
                 * then divide by frac_div.
                 */
                U32 mantissa;
                if (msb >= 24) {
                    mantissa = val >> (msb - 23);
                    /* Adjust with fractional denominator */
                    /* We need: (val / frac_div) normalised to 1.xxxx */
                    /* mantissa = (val << (23 - msb + div_msb)) / frac_div; but avoid overflow */
                    /* Simpler: compute directly with 64-bit-like step */
                    U32 shift_up = 23;
                    if (msb > div_msb) {
                        U32 int_exp = msb - div_msb;
                        exp = (S32)int_exp;
                        /* val_shifted = val << (23 - int_exp) if possible, else val >> (int_exp - 23) */
                        if (int_exp <= 23) {
                            U32 s = 23 - int_exp;
                            mantissa = (val << s) / frac_div;
                        } else {
                            U32 s = int_exp - 23;
                            mantissa = (val >> s) / frac_div;
                        }
                    } else {
                        exp = -((S32)(div_msb - msb));
                        U32 s = 23 + (div_msb - msb);
                        if (s < 31) mantissa = (val << s) / frac_div;
                        else mantissa = 0;
                    }
                } else {
                    if (msb >= div_msb) {
                        exp = (S32)(msb - div_msb);
                        U32 s = 23 - exp;
                        if (s < 31) mantissa = (val << s) / frac_div;
                        else mantissa = 0;
                    } else {
                        exp = -((S32)(div_msb - msb));
                        U32 s = 23 + (div_msb - msb);
                        if (s < 31) mantissa = (val << s) / frac_div;
                        else mantissa = 0;
                    }
                }

                mantissa &= 0x7FFFFF; /* strip implicit leading 1 */
                U32 biased_exp = (U32)(exp + 127) & 0xFF;
                ieee_bits = (neg ? 0x80000000u : 0) | (biased_exp << 23) | mantissa;
            }

            EMIT_U32(f, ieee_bits);

            while (*p && *p != ',') p++;
        }
        return TRUE;
    }

    /* Numeric initialiser(s) — parse comma-separated values */
    PU8 p = var->raw_value;
    while (*p) {
        while (*p == ' ' || *p == ',') p++;
        if (!*p) break;

        /* $VARNAME — variable byte size */
        if (*p == '$') {
            p++;
            while (*p == ' ') p++;   /* skip spaces between $ and name */
            U8 name_buf[128] = { 0 };
            U32 nb = 0;
            while (*p && *p != ',' && *p != ' ' && nb < 127)
                name_buf[nb++] = *p++;
            name_buf[nb] = '\0';
            ASM_PTR *var_ptr = FIND_ASM_PTR(name_buf);
            EMIT_IMM(f, var_ptr ? var_ptr->byte_size : 0,
                     (elem_sz == 1) ? SZ_8BIT
                   : (elem_sz == 2) ? SZ_16BIT
                                    : SZ_32BIT);
            continue;
        }

        U32 num = 0;
        BOOL parsed = FALSE;
        if (ATOI_HEX_E(p, &num))      parsed = TRUE;
        else if (ATOI_BIN_E(p, &num))  parsed = TRUE;
        else if (ATOI_E(p, &num))      parsed = TRUE;

        if (parsed) {
            EMIT_IMM(f, num, (elem_sz == 1) ? SZ_8BIT
                           : (elem_sz == 2) ? SZ_16BIT
                                            : SZ_32BIT);
        } else {
            /* Label reference — resolve to absolute address */
            U8 name_buf[128] = { 0 };
            U32 nb = 0;
            while (*p && *p != ',' && *p != ' ' && nb < 127)
                name_buf[nb++] = *p++;
            name_buf[nb] = '\0';
            EMIT_IMM(f, RESOLVE_SYMBOL_ADDR(name_buf),
                     (elem_sz == 1) ? SZ_8BIT
                   : (elem_sz == 2) ? SZ_16BIT
                                    : SZ_32BIT);
            continue;   /* already advanced p */
        }

        /* Advance past the number text */
        while (*p && *p != ',' && *p != ' ') p++;
    }

    return TRUE;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  LOCAL LABEL SCOPING
 * ════════════════════════════════════════════════════════════════════════════
 *  Make @@name and .name labels local to their enclosing global label
 *  by renaming them to "globalname.@@name" / "globalname..name".
 *  This allows multiple functions to each define @@done: without collision.
 *
 *  Must run BEFORE Pass 1 (label recording) and BEFORE RESOLVE_LOCAL_REFS.
 */
STATIC BOOL IS_LOCAL_NAME(PU8 name) {
    if (!name) return FALSE;
    if (name[0] == '@' && name[1] == '@') return TRUE;   /* @@N label */
    if (name[0] == '.')                   return TRUE;    /* .name label */
    return FALSE;
}

STATIC PU8 MAKE_SCOPED_NAME(PU8 scope, PU8 local) {
    if (!scope || !*scope) return STRDUP(local);          /* no scope yet, keep as-is */
    /* Build "scope.local" */
    U8 buf[BUF_SZ];
    MEMSET(buf, 0, sizeof(buf));
    STRNCPY(buf, scope, BUF_SZ - 1);
    STRNCAT(buf, ".", BUF_SZ - STRLEN(buf) - 1);
    STRNCAT(buf, local, BUF_SZ - STRLEN(buf) - 1);
    return STRDUP(buf);
}

STATIC VOID SCOPE_LOCAL_LABELS(ASM_AST_ARRAY *ast) {
    PU8 scope = NULLPTR;   /* current enclosing global label name */

    for (U32 i = 0; i < ast->len; i++) {
        PASM_NODE node = ast->nodes[i];
        if (!node) continue;

        /* Update scope at each global label */
        if (node->type == NODE_LABEL) {
            scope = node->label.name;
            continue;
        }

        /* Rename local label definitions */
        if (node->type == NODE_LOCAL_LABEL && IS_LOCAL_NAME(node->label.name)) {
            PU8 old = node->label.name;
            node->label.name = MAKE_SCOPED_NAME(scope, old);
            MFree(old);
            continue;
        }

        /* Rename local label references in instruction operands */
        if (node->type == NODE_INSTRUCTION) {
            for (U32 j = 0; j < node->instr.operand_count; j++) {
                ASM_OPERAND *op = &node->instr.operands[j];
                if (op->type != OP_PTR || !op->mem_ref || !op->mem_ref->symbol_name)
                    continue;
                PU8 sym = op->mem_ref->symbol_name;
                /* Skip @f / @b — those are resolved by RESOLVE_LOCAL_REFS */
                if (STRICMP(sym, "@f") == 0 || STRICMP(sym, "@b") == 0)
                    continue;
                if (IS_LOCAL_NAME(sym)) {
                    PU8 old = op->mem_ref->symbol_name;
                    op->mem_ref->symbol_name = MAKE_SCOPED_NAME(scope, old);
                    MFree(old);
                }
            }
        }
    }
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  @f / @b  RESOLUTION
 * ════════════════════════════════════════════════════════════════════════════
 *  After Pass 1 collects all label offsets, resolve forward (@f) and
 *  backward (@b) local label references in instruction operands.
 */
STATIC VOID RESOLVE_LOCAL_REFS(ASM_AST_ARRAY *ast) {
    for (U32 i = 0; i < ast->len; i++) {
        PASM_NODE node = ast->nodes[i];
        if (!node || node->type != NODE_INSTRUCTION) continue;

        for (U32 j = 0; j < node->instr.operand_count; j++) {
            ASM_OPERAND *op = &node->instr.operands[j];
            if (op->type != OP_PTR || !op->mem_ref || !op->mem_ref->symbol_name)
                continue;

            if (STRICMP(op->mem_ref->symbol_name, "@f") == 0) {
                /* Find next @@N label definition after this instruction.
                 * After scoping, names look like "func.@@done", so check
                 * for "@@" anywhere in the name. */
                for (U32 k = i + 1; k < ast->len; k++) {
                    PASM_NODE n = ast->nodes[k];
                    if (n && n->type == NODE_LOCAL_LABEL && n->label.name
                        && STRSTR(n->label.name, "@@")) {
                        MFree(op->mem_ref->symbol_name);
                        op->mem_ref->symbol_name = STRDUP(n->label.name);
                        break;
                    }
                }
            } else if (STRICMP(op->mem_ref->symbol_name, "@b") == 0) {
                /* Find previous @@N label definition before this instruction */
                for (S32 k = (S32)i - 1; k >= 0; k--) {
                    PASM_NODE n = ast->nodes[k];
                    if (n && n->type == NODE_LOCAL_LABEL && n->label.name
                        && STRSTR(n->label.name, "@@")) {
                        MFree(op->mem_ref->symbol_name);
                        op->mem_ref->symbol_name = STRDUP(n->label.name);
                        break;
                    }
                }
            }
        }
    }
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  GEN_EMIT_PASS  —  single pass over the AST
 * ════════════════════════════════════════════════════════════════════════════
 *  When f is NULL the pass only calculates offsets and records labels
 *  (Pass 1).  When f is a real file pointer it emits binary data (Pass 2).
 */
STATIC BOOL GEN_EMIT_PASS(FILE *f, ASM_AST_ARRAY *ast, ASTRAC_ARGS *cfg) {

    for (U32 i = 0; i < ast->len; i++) {
        PASM_NODE node = ast->nodes[i];
        if (!node) continue;

        switch (node->type) {

        /* ── Section directive ────────────────────────────────────────── */
        case NODE_SECTION:
            switch (node->dir.section) {
                case DIR_CODE_TYPE_32:
                    ptrs.code_type = DIR_CODE_TYPE_32;
                    if (cfg->verbose && f)
                        printf("[ASM GEN] .use32 at line %u\n", node->line);
                    break;
                case DIR_CODE_TYPE_16:
                    ptrs.code_type = DIR_CODE_TYPE_16;
                    if (cfg->verbose && f)
                        printf("[ASM GEN] .use16 at line %u\n", node->line);
                    break;
                default:
                    ptrs.current_section = node->dir.section;
                    if (cfg->verbose && f)
                        printf("[ASM GEN] Section switch at line %u\n", node->line);
                    break;
            }
            break;

        /* ── .org ─────────────────────────────────────────────────────── */
        case NODE_ORG:
            ptrs.origin = node->org.origin;
            if (cfg->verbose && f)
                printf("[ASM GEN] .org 0x%X at line %u\n",
                       node->org.origin, node->line);
            break;

        /* ── .times <expr> <type> <value> ─────────────────────────────── */
        case NODE_TIMES: {
            U32 count = node->times.count_expr
                      ? EVAL_EXPR(node->times.count_expr) : 0;
            U32 elem_sz;
            switch (node->times.fill_type) {
                case TYPE_BYTE:  elem_sz = 1; break;
                case TYPE_WORD:  elem_sz = 2; break;
                case TYPE_DWORD: case TYPE_FLOAT: case TYPE_PTR:
                    elem_sz = 4; break;
                default: elem_sz = 1; break;
            }
            U32 fill_val = 0;
            if (node->times.fill_raw)
                fill_val = EVAL_EXPR(node->times.fill_raw);
            ASM_OPERAND_SIZE fsz = (elem_sz == 1) ? SZ_8BIT
                                 : (elem_sz == 2) ? SZ_16BIT
                                                   : SZ_32BIT;
            if (cfg->verbose && f)
                printf("[ASM GEN] .times %u x %u-byte fill=0x%X at line %u\n",
                       count, elem_sz, fill_val, node->line);
            for (U32 j = 0; j < count; j++)
                EMIT_IMM(f, fill_val, fsz);
            break;
        }

        /* ── Label definition ─────────────────────────────────────────── */
        case NODE_LABEL:
        case NODE_LOCAL_LABEL:
            ADD_ASM_PTR(node->label.name, CURRENT_OFFSET(), ptrs.current_section);
            if (cfg->verbose && f)
                printf("[ASM GEN] Label '%s' at offset 0x%X\n",
                       node->label.name, CURRENT_OFFSET());
            break;

        /* ── Data variable ────────────────────────────────────────────── */
        case NODE_DATA_VAR:
            if (node->data.var) {
                U32 before = CURRENT_OFFSET();
                if (!ENCODE_DATA_VAR(f, node)) {
                    printf("[ASM GEN] Line %u: Failed to encode variable '%s'\n",
                           node->line,
                           node->data.var->name ? node->data.var->name : "(null)");
                    return FALSE;
                }
                U32 after = CURRENT_OFFSET();
                /* Record byte_size so $VARNAME can resolve */
                ASM_PTR *vp = FIND_ASM_PTR(node->data.var->name);
                if (vp) vp->byte_size = after - before;
            }
            break;

        /* ── Raw number in .code ──────────────────────────────────────── */
        case NODE_RAW_NUM: {
            U32 val  = node->raw.value;
            U32 size;
            if (ptrs.code_type == DIR_CODE_TYPE_16) {
                /* In 16-bit mode the native word is 2 bytes; only use
                 * 4 bytes when the value exceeds 16-bit range. */
                size = (val <= 0xFF) ? 1 : (val <= 0xFFFF) ? 2 : 4;
            } else {
                size = (val <= 0xFF) ? 1 : (val <= 0xFFFF) ? 2 : 4;
            }
            EMIT(f, (U8 *)&val, size);
            break;
        }

        /* ── Instruction ──────────────────────────────────────────────── */
        case NODE_INSTRUCTION:
            if (!ENCODE_INSTRUCTION(f, node)) {
                printf("[ASM GEN] Line %u: Failed to encode instruction\n", node->line);
                return FALSE;
            }
            break;

        default:
            break;
        }
    }

    return TRUE;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  GEN_BINARY  —  main code generation entry point
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  Two-pass architecture:
 *    Pass 1  — walk all nodes with f=NULL (no file writes), collecting
 *              label offsets and variable byte sizes.
 *    Between — resolve @f/@b local label references.
 *    Pass 2  — walk all nodes with the real output file, emitting binary.
 */
BOOLEAN GEN_BINARY(ASM_AST_ARRAY *ast, PASM_INFO info) {

    ASTRAC_ARGS *cfg = GET_ARGS();
    if (!ast || ast->len == 0) {
        if (cfg->verbose) printf("[ASM GEN] AST is empty or null.\n");
        return FALSE;
    }
    if (!info) {
        if (cfg->verbose) printf("[ASM GEN] ASM_INFO is null.\n");
        return FALSE;
    }

    /* ── Reset state ──────────────────────────────────────────────────── */
    MEMSET(&ptrs, 0, sizeof(ptrs));
    ptrs.current_section = DIR_NONE;
    ptrs.code_type       = DIR_CODE_TYPE_32;
    
    CURRENT_PASS = FIRST_PASS;

    /* ── Scope local labels (@@name, .name) to enclosing global label ──── */
    SCOPE_LOCAL_LABELS(ast);

    /* ──────────────────────────────────────────────────────────────────
     *  Pass 1:  Calculate offsets — no file writes (f = NULL)
     * ────────────────────────────────────────────────────────────────── */
    if (!GEN_EMIT_PASS(NULLPTR, ast, cfg)) {
        printf("[ASM GEN] Pass 1 (offset calculation) failed\n");
        return FALSE;
    }

    if (cfg->verbose)
        printf("[ASM GEN] Pass 1 complete: %u labels, code=%u data=%u rodata=%u origin=0x%X\n",
               ptrs.arr_tail, ptrs.code, ptrs.data, ptrs.rodata, ptrs.origin);

    /* ── Resolve @f / @b references ───────────────────────────────────── */
    RESOLVE_LOCAL_REFS(ast);

    /* ── Reset offsets for Pass 2 (keep labels + origin) ──────────────── */
    ptrs.code    = 0;
    ptrs.data    = 0;
    ptrs.rodata  = 0;
    ptrs.current_section = DIR_NONE;
    ptrs.code_type       = DIR_CODE_TYPE_32;
    ptrs.origin          = cfg->org;

    /* ── Open output file ─────────────────────────────────────────────── */
    PU8 outputfile = cfg->outfile;
    if (FILE_EXISTS(outputfile)) FILE_DELETE(outputfile);
    if (!FILE_CREATE(outputfile)) {
        printf("[ASM GEN] Failed to create output file: %s\n", outputfile);
        return FALSE;
    }
    FILE *out = FOPEN(outputfile, MODE_A | MODE_FAT32);
    if (!out) {
        printf("[ASM GEN] Failed to open output file: %s\n", outputfile);
        return FALSE;
    }

    /* ──────────────────────────────────────────────────────────────────
     *  Pass 2:  Emit binary
     * ────────────────────────────────────────────────────────────────── */
    CURRENT_PASS = SECOND_PASS;
    if (!GEN_EMIT_PASS(out, ast, cfg)) {
        FCLOSE(out);
        return FALSE;
    }

    if (cfg->verbose) printf("[ASM GEN] Finished writing output file: %s, sz %u bytes\n", outputfile, out->sz);
    FCLOSE(out);
    return TRUE;
}
