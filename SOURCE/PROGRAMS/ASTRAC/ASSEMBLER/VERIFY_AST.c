#include <PROGRAMS/ASTRAC/ASTRAC.h>
#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  AST VERIFICATION
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  Validates the AST against semantic rules:
 *    1. Instruction operand count/type matches mnemonic table
 *    2. Register and memory references are valid
 *    3. Immediates fit in required bit-width
 *    4. Label references have corresponding definitions
 *    5. Data sections have valid variable declarations
 */


/*
 * ────────────────────────────────────────────────────────────────────────────
 *  LABEL TABLE  (first pass collection)
 * ────────────────────────────────────────────────────────────────────────────
 */
typedef struct {
    PU8  name;
    U32  node_index;        /* index in AST where this label is defined */
} LABEL_ENTRY, *PLABEL_ENTRY;

#define MAX_LABELS 512

typedef struct {
    PLABEL_ENTRY entries[MAX_LABELS];
    U32 len;
} LABEL_TABLE, *PLABEL_TABLE;

STATIC PLABEL_TABLE alloc_label_table() {
    PLABEL_TABLE t = MAlloc(sizeof(LABEL_TABLE));
    if (t) MEMSET(t, 0, sizeof(LABEL_TABLE));
    return t;
}

STATIC VOID free_label_table(PLABEL_TABLE t) {
    if (!t) return;
    for (U32 i = 0; i < t->len; i++) {
        if (t->entries[i]) {
            if (t->entries[i]->name) MFree(t->entries[i]->name);
            MFree(t->entries[i]);
        }
    }
    MFree(t);
}

STATIC PLABEL_ENTRY find_label(PLABEL_TABLE t, PU8 name) {
    if (!t || !name) return NULLPTR;
    for (U32 i = 0; i < t->len; i++) {
        if (t->entries[i] && STRICMP(t->entries[i]->name, name) == 0) {
            return t->entries[i];
        }
    }
    return NULLPTR;
}

STATIC BOOL add_label(PLABEL_TABLE t, PU8 name, U32 node_index) {
    if (!t || t->len >= MAX_LABELS) return FALSE;
    PLABEL_ENTRY e = MAlloc(sizeof(LABEL_ENTRY));
    if (!e) return FALSE;
    e->name = STRDUP(name);
    e->node_index = node_index;
    t->entries[t->len++] = e;
    return TRUE;
}

/*
 * ────────────────────────────────────────────────────────────────────────────
 *  SMALL HELPERS
 * ────────────────────────────────────────────────────────────────────────────
 */

/* Returns TRUE when `data` can be parsed as a decimal, hex, or binary integer. */
STATIC BOOL is_numeric_value(PU8 data) {
    if (!data || !*data) return FALSE;
    PU8 p = data;
    if (*p == '-') p++;           /* skip leading minus for negative literals */
    if (!*p) return FALSE;
    U32 tmp;
    if (ATOI_E(p, &tmp))     return TRUE;
    if (ATOI_HEX_E(p, &tmp)) return TRUE;
    if (ATOI_BIN_E(p, &tmp)) return TRUE;
    return FALSE;
}

/* Map a register enum to its bit-width. */
STATIC ASM_OPERAND_SIZE reg_size(ASM_REGS reg) {
    switch (reg) {
        /* 32-bit GPR */
        case REG_EAX: case REG_EBX: case REG_ECX: case REG_EDX:
        case REG_ESI: case REG_EDI: case REG_EBP: case REG_ESP:
        case REG_EIP:
        /* Control / Debug - 32-bit */
        case REG_CR0: case REG_CR2: case REG_CR3: case REG_CR4:
        case REG_DR0: case REG_DR1: case REG_DR2: case REG_DR3:
        case REG_DR6: case REG_DR7:
            return SZ_32BIT;

        /* 16-bit GPR + IP */
        case REG_AX: case REG_BX: case REG_CX: case REG_DX:
        case REG_SI: case REG_DI: case REG_BP: case REG_SP:
        case REG_IP:
        /* Segment registers - 16-bit */
        case REG_CS: case REG_DS: case REG_ES:
        case REG_FS: case REG_GS: case REG_SS:
            return SZ_16BIT;

        /* 8-bit GPR */
        case REG_AL: case REG_AH: case REG_BL: case REG_BH:
        case REG_CL: case REG_CH: case REG_DL: case REG_DH:
            return SZ_8BIT;

        default:
            return SZ_NONE;     /* FPU / MMX / XMM - size not applicable */
    }
}

/*
 * Check whether 'actual' operand type satisfies 'expected' from the table.
 * In x86 encoding an r/m operand (OP_MEM in the table) also accepts a
 * plain register, and a label reference (OP_PTR) can appear where an
 * immediate or memory operand is expected.
 */
STATIC BOOL operand_type_compatible(ASM_OPERAND_TYPE actual,
                                    ASM_OPERAND_TYPE expected) {
    if (actual == expected)                              return TRUE;
    if (expected == OP_MEM  && actual == OP_REG)         return TRUE;  /* r/m */
    if (expected == OP_IMM  && actual == OP_PTR)         return TRUE;  /* rel */
    if (expected == OP_MEM  && actual == OP_PTR)         return TRUE;  /* [sym] */
    return FALSE;
}

/* Return TRUE when 'value' (treated as unsigned OR sign-extended) fits in 'size'. */
STATIC BOOL imm_in_bounds(U32 value, ASM_OPERAND_SIZE size) {
    switch (size) {
        case SZ_8BIT:  return (value <= 0xFF)    || (value >= 0xFFFFFF80);
        case SZ_16BIT: return (value <= 0xFFFF)  || (value >= 0xFFFF8000);
        case SZ_32BIT: return TRUE;
        default:       return TRUE;     /* SZ_NONE - no restriction */
    }
}

/* Verify a symbol name from an operand exists in the label table. */
STATIC BOOL verify_symbol_ref(PU8 sym, PLABEL_TABLE labels, U32 line, U32 op_idx) {
    if (!sym || !*sym) return TRUE;                     /* no symbol → ok */
    if (*sym == '$')   return TRUE;                     /* $ (current offset) */
    if (STRICMP(sym, "@f") == 0 || STRICMP(sym, "@b") == 0)
        return TRUE;                                    /* @f/@b resolved in codegen */
    if ((*sym == '@' && sym[1] == '@') || *sym == '.') 
        return TRUE;                                    /* @@name/.name scoped + resolved in codegen */
    if (find_label(labels, sym)) return TRUE;
    printf("[ASM VERIFY] Line %u: Operand %u references undefined symbol '%s'\n",
           line, op_idx + 1, sym);
    return FALSE;
}


/*
 * ────────────────────────────────────────────────────────────────────────────
 *  VERIFICATION HELPERS
 * ────────────────────────────────────────────────────────────────────────────
 */

STATIC BOOL verify_instruction(PASM_NODE node, PLABEL_TABLE labels) {
    if (!node || node->type != NODE_INSTRUCTION) return FALSE;

    const ASM_MNEMONIC_TABLE *tbl = node->instr.table_entry;
    if (!tbl) {
        printf("[ASM VERIFY] Line %u: Instruction has no mnemonic table entry\n", node->line);
        return FALSE;
    }

    /* ── Operand count ────────────────────────────────────────────────────── */
    if (node->instr.operand_count != tbl->operand_count) {
        printf("[ASM VERIFY] Line %u: '%s' expects %u operand(s), got %u\n",
            node->line, tbl->name,
            tbl->operand_count, node->instr.operand_count);
        return FALSE;
    }

    /* ── Per-operand checks ───────────────────────────────────────────────── */
    BOOL ok = TRUE;

    for (U32 i = 0; i < node->instr.operand_count; i++) {
        ASM_OPERAND      *op       = &node->instr.operands[i];
        ASM_OPERAND_TYPE  expected = tbl->operand[i];

        /* ── Type compatibility ───────────────────────────────────────────── */
        if (!operand_type_compatible(op->type, expected)) {
            printf("[ASM VERIFY] Line %u: '%s' operand %u - type mismatch\n",
                   node->line, tbl->name, i + 1);
            ok = FALSE;
            continue;
        }

        switch (op->type) {

        case OP_NONE:
            printf("[ASM VERIFY] Line %u: '%s' operand %u is OP_NONE\n",
                   node->line, tbl->name, i + 1);
            ok = FALSE;
            break;

        /* ── Register ─────────────────────────────────────────────────────── */
        case OP_REG:
            if (op->reg == REG_NONE) {
                printf("[ASM VERIFY] Line %u: '%s' operand %u - register is REG_NONE\n",
                       node->line, tbl->name, i + 1);
                ok = FALSE;
                break;
            }
            /* Size validation: if the table prescribes an operand size the
               register must match (ignore when SZ_NONE - size-agnostic).
               For 0F-prefixed two-operand instructions (MOVZX/MOVSX) the
               table size describes the SOURCE; skip checking the destination. */
            if (tbl->size != SZ_NONE) {
                BOOL is_0f_two_op = (tbl->opcode_prefix == PFX_0F
                                     && (U32)tbl->operand_count == 2);
                if (!(is_0f_two_op && i == 0)) {
                    ASM_OPERAND_SIZE rs = reg_size(op->reg);
                    if (rs != SZ_NONE && rs != tbl->size) {
                        printf("[ASM VERIFY] Line %u: '%s' operand %u - register is %u-bit, "
                               "instruction requires %u-bit\n",
                               node->line, tbl->name, i + 1, rs, tbl->size);
                        ok = FALSE;
                    }
                }
            }
            break;

        /* ── Memory reference ─────────────────────────────────────────────── */
        case OP_MEM:
            if (!op->mem_ref) {
                printf("[ASM VERIFY] Line %u: '%s' operand %u - OP_MEM with null mem_ref\n",
                       node->line, tbl->name, i + 1);
                ok = FALSE;
                break;
            }
            /* Must have at least one addressing component.
             * Bare displacement-only is valid: [300] = disp32. */
            if (op->mem_ref->base_reg  == REG_NONE &&
                op->mem_ref->index_reg == REG_NONE &&
                !op->mem_ref->symbol_name &&
                op->mem_ref->displacement == 0) {
                printf("[ASM VERIFY] Line %u: '%s' operand %u - memory has no base, index, or symbol\n",
                       node->line, tbl->name, i + 1);
                ok = FALSE;
                break;
            }
            /* Explicit size (BYTE/WORD/DWORD PTR) must match instruction size */
            if (op->size != SZ_NONE && tbl->size != SZ_NONE
                && op->size != tbl->size) {
                printf("[ASM VERIFY] Line %u: '%s' operand %u - explicit %u-bit size "
                       "conflicts with %u-bit instruction\n",
                       node->line, tbl->name, i + 1, op->size, tbl->size);
                ok = FALSE;
            }
            /* Scale must be 1, 2, 4, or 8 */
            if (op->mem_ref->scale != 1 && op->mem_ref->scale != 2 &&
                op->mem_ref->scale != 4 && op->mem_ref->scale != 8) {
                printf("[ASM VERIFY] Line %u: '%s' operand %u - invalid SIB scale %d\n",
                       node->line, tbl->name, i + 1, op->mem_ref->scale);
                ok = FALSE;
            }
            /* Segment override must be a segment register */
            if (op->mem_ref->segment != REG_NONE) {
                if (op->mem_ref->segment != REG_CS && op->mem_ref->segment != REG_DS &&
                    op->mem_ref->segment != REG_ES && op->mem_ref->segment != REG_FS &&
                    op->mem_ref->segment != REG_GS && op->mem_ref->segment != REG_SS) {
                    printf("[ASM VERIFY] Line %u: '%s' operand %u - invalid segment override\n",
                           node->line, tbl->name, i + 1);
                    ok = FALSE;
                }
            }
            /* Verify symbol reference if present */
            if (op->mem_ref->symbol_name) {
                if (!verify_symbol_ref(op->mem_ref->symbol_name, labels, node->line, i))
                    ok = FALSE;
            }
            break;

        /* ── Immediate ────────────────────────────────────────────────────── */
        case OP_IMM: {
            ASM_OPERAND_SIZE sz = (op->size != SZ_NONE) ? op->size : tbl->size;
            if (!imm_in_bounds(op->immediate, sz)) {
                printf("[ASM VERIFY] Line %u: '%s' operand %u - immediate 0x%X out of range "
                       "for %u-bit operand\n",
                       node->line, tbl->name, i + 1, op->immediate, sz);
                ok = FALSE;
            }
            break;
        }

        /* ── Pointer / label reference ────────────────────────────────────── */
        case OP_PTR:
            if (!op->mem_ref || !op->mem_ref->symbol_name) {
                printf("[ASM VERIFY] Line %u: '%s' operand %u - OP_PTR with no symbol\n",
                       node->line, tbl->name, i + 1);
                ok = FALSE;
            } else {
                if (!verify_symbol_ref(op->mem_ref->symbol_name, labels, node->line, i))
                    ok = FALSE;
            }
            break;

        /* ── Segment / memory-offset - basic acceptance ───────────────────── */
        case OP_SEG:
        case OP_MOFFS:
            break;

        default:
            printf("[ASM VERIFY] Line %u: '%s' operand %u - unknown type %u\n",
                   node->line, tbl->name, i + 1, op->type);
            ok = FALSE;
            break;
        }
    }

    return ok;
}


STATIC BOOL verify_data_variable(PASM_NODE node, ASM_AST_ARRAY *ast, U32 node_idx) {
    if (!node || node->type != NODE_DATA_VAR) return FALSE;

    PASM_VAR var = node->data.var;
    if (!var) {
        printf("[ASM VERIFY] Line %u: Data variable node has null var\n", node->line);
        return FALSE;
    }

    /* ── Variable type must be valid ──────────────────────────────────────── */
    if (var->var_type == TYPE_NONE || var->var_type >= TYPE_AMOUNT) {
        printf("[ASM VERIFY] Line %u: Invalid variable type\n", node->line);
        return FALSE;
    }

    /* ── Variable name must be non-empty ──────────────────────────────────── */
    if (!var->name || !*var->name) {
        printf("[ASM VERIFY] Line %u: Variable has no name\n", node->line);
        return FALSE;
    }

    /* ── Duplicate name detection (scan earlier nodes) ────────────────────── */
    for (U32 i = 0; i < node_idx; i++) {
        PASM_NODE prev = ast->nodes[i];
        if (!prev || prev->type != NODE_DATA_VAR) continue;
        if (!prev->data.var || !prev->data.var->name) continue;
        if (STRICMP(prev->data.var->name, var->name) == 0) {
            printf("[ASM VERIFY] Line %u: Duplicate variable name '%s' "
                   "(first defined at line %u)\n",
                   node->line, var->name, prev->line);
            return FALSE;
        }
    }

    /* ── Per-type value checks ────────────────────────────────────────────── */
    switch (var->var_type) {
        case TYPE_BYTE:
            if (var->is_list && var->list_len == 0) {
                printf("[ASM VERIFY] Line %u: Byte list has zero elements\n", node->line);
                return FALSE;
            }
            if (var->raw_value == NULLPTR) {
                printf("[ASM VERIFY] Line %u: Variable has no initializer\n", node->line);
                return FALSE;
            }
            break;

        case TYPE_FLOAT:
            /* REAL4 accepts float or integer literals — skip strict numeric check */
            if (var->raw_value == NULLPTR) {
                printf("[ASM VERIFY] Line %u: Variable has no initializer\n", node->line);
                return FALSE;
            }
            break;

        case TYPE_DWORD:
        case TYPE_PTR:
        case TYPE_WORD:
            if (var->is_list && var->list_len == 0) {
                printf("[ASM VERIFY] Line %u: List variable has zero elements\n", node->line);
                return FALSE;
            }
            if (var->raw_value == NULLPTR) {
                printf("[ASM VERIFY] Line %u: Variable has no initializer\n", node->line);
                return FALSE;
            }
            /* Non-BYTE types must have numeric initializers, not string literals.
             * Allow $VARNAME references (variable byte-size) as valid. */
            if (var->raw_value[0] != '$' && !is_numeric_value(var->raw_value)) {
                printf("[ASM VERIFY] Line %u: Non-BYTE variable '%s' has non-numeric initializer\n",
                       node->line, var->name);
                return FALSE;
            }
            break;

        default:
            break;
    }

    return TRUE;
}

STATIC BOOL verify_label(PASM_NODE node) {
    if (!node || (node->type != NODE_LABEL && node->type != NODE_LOCAL_LABEL))
        return FALSE;

    if (!node->label.name || !*node->label.name) {
        printf("[ASM VERIFY] Line %u: Label has no name\n", node->line);
        return FALSE;
    }

    return TRUE;
}

PU8 get_node_type_name(ASM_NODE_TYPE type) {
    switch (type) {
        case NODE_INVALID:      return "INVALID";
        case NODE_INSTRUCTION:  return "INSTRUCTION";
        case NODE_LABEL:        return "LABEL";
        case NODE_LOCAL_LABEL:  return "LOCAL_LABEL";
        case NODE_DATA_VAR:     return "DATA_VAR";
        case NODE_SECTION:      return "SECTION";
        case NODE_RAW_NUM:      return "RAW_NUM";
        case NODE_ORG:          return "ORG";
        case NODE_TIMES:        return "TIMES";
        default:               return "UNKNOWN";
    }
}

PU8 get_var_raw_value(ASM_NODE *node) {
    if(!node || node->type != NODE_DATA_VAR || !node->data.var) return NULLPTR;
    return node->data.var->raw_value;
}

STATIC VOID DEBUG_PRINT_AST(ASM_AST_ARRAY *ast) {
    DEBUGM_PRINTF("=== AST DUMP (%u nodes) ===\n", ast->len);
    for (U32 i = 0; i < ast->len; i++) {
        PASM_NODE node = ast->nodes[i];
        if (!node) continue;
        DEBUGM_PRINTF("Node %u: Type %s, raw_val: %s, Line %u\n", i, get_node_type_name(node->type), get_var_raw_value(node), node->line);
        /* Additional per-node details could be printed here for debugging */
    }
    DEBUGM_PRINTF("=== END AST DUMP ===\n");
}

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  MAIN VERIFIER
 * ════════════════════════════════════════════════════════════════════════════
 */
BOOLEAN VERIFY_AST(ASM_AST_ARRAY *ast, PASM_INFO info) {
    ASTRAC_ARGS  *cfg = GET_ARGS();

    if (!ast || ast->len == 0) {
        if (cfg->verbose) printf("[ASM VERIFY] AST is empty or null.\n");
        return FALSE;
    }

    if (!info) {
        if (cfg->verbose) printf("[ASM VERIFY] ASM_INFO is null.\n");
        return FALSE;
    }

    /* ── Pass 1: Collect all labels (check for duplicates) ─────────────── */
    PLABEL_TABLE labels = alloc_label_table();
    if (!labels) {
        printf("[ASM VERIFY] Failed to allocate label table\n");
        return FALSE;
    }

    U32 error_count = 0;

    /* Track current global label scope for local label scoping */
    PU8 scope = NULLPTR;

    for (U32 i = 0; i < ast->len; i++) {
        PASM_NODE node = ast->nodes[i];
        if (!node) continue;

        if (node->type == NODE_LABEL) {
            scope = node->label.name;
        }

        if (node->type == NODE_LABEL || node->type == NODE_LOCAL_LABEL) {
            /* Build scoped name for local labels (@@name, .name) */
            PU8 check_name = node->label.name;
            U8  scoped[BUF_SZ];
            if (node->type == NODE_LOCAL_LABEL && check_name && scope
                && (  (check_name[0] == '@' && check_name[1] == '@')
                    || check_name[0] == '.')) {
                MEMSET(scoped, 0, sizeof(scoped));
                STRNCPY(scoped, scope, BUF_SZ - 1);
                STRNCAT(scoped, ".", BUF_SZ - STRLEN(scoped) - 1);
                STRNCAT(scoped, check_name, BUF_SZ - STRLEN(scoped) - 1);
                check_name = scoped;
            }
            if (find_label(labels, check_name)) {
                printf("[ASM VERIFY] Line %u: Duplicate label '%s'\n",
                       node->line, check_name);
                error_count++;
                continue;
            }
            if (!add_label(labels, check_name, i)) {
                printf("[ASM VERIFY] Failed to add label (table full)\n");
                free_label_table(labels);
                return FALSE;
            }
        }

        /* Also register data variable names so [varname] refs resolve */
        if (node->type == NODE_DATA_VAR && node->data.var
            && node->data.var->name) {
            if (!find_label(labels, node->data.var->name)) {
                add_label(labels, node->data.var->name, i);
            }
        }
    }

    if (cfg->verbose) printf("[ASM VERIFY] Collected %u labels\n", labels->len);

    /* ── Pass 2: Verify each node ──────────────────────────────────────── */
    for (U32 i = 0; i < ast->len; i++) {
        PASM_NODE node = ast->nodes[i];
        if (!node) continue;

        BOOL ok = TRUE;

        switch (node->type) {
            case NODE_INSTRUCTION:
                ok = verify_instruction(node, labels);
                break;
            case NODE_DATA_VAR:
                ok = verify_data_variable(node, ast, i);
                break;
            case NODE_LABEL:
            case NODE_LOCAL_LABEL:
                ok = verify_label(node);
                break;
            case NODE_SECTION:
            case NODE_ORG:
            case NODE_TIMES:
                ok = TRUE;
                break;
            case NODE_RAW_NUM:
                /* Raw numbers are auto-sized: 1/2/4 bytes — always valid */
                ok = TRUE;
                break;
            default:
                printf("[ASM VERIFY] Line %u: Unknown node type %u\n",
                       node->line, node->type);
                ok = FALSE;
                break;
        }

        if (!ok) error_count++;
    }

    free_label_table(labels);

    if (error_count > 0) {
        printf("[ASM VERIFY] %u error(s) found\n", error_count);
        return FALSE;
    }

    if (cfg->verbose) printf("[ASM VERIFY] All %u nodes verified OK\n", ast->len);

    if(cfg->verbose) DEBUG_PRINT_AST(ast);
    return TRUE;
}

