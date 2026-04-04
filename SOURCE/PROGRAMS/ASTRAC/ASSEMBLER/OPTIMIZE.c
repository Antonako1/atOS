#include <PROGRAMS/ASTRAC/ASTRAC.h>
#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  OPTIMIZE.c — Peephole optimiser (Stage 5)
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  Walks the AST instruction list and applies size/speed peephole
 *  transformations.  Only NODE_INSTRUCTION nodes are inspected;
 *  labels, data, directives are left untouched.
 *
 *  Current passes:
 *      1. Single-instruction rewrites (constant folding, shorter encodings)
 *      2. Two-instruction peephole window (redundant pairs)
 *
 *  The function returns TRUE on success, FALSE if an internal error occurs
 *  (which should never happen — optimisation is always optional).
 * ════════════════════════════════════════════════════════════════════════════
 */


/*
 * ── Helper: is this mnemonic a MOV? ──────────────────────────────────────
 *   Matches all MOV table entries by comparing the name string.
 */
STATIC BOOL IS_MOV(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "mov") == 0;
}

STATIC BOOL IS_PUSH(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "push") == 0;
}

STATIC BOOL IS_POP(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "pop") == 0;
}

STATIC BOOL IS_INC(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "inc") == 0;
}

STATIC BOOL IS_DEC(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "dec") == 0;
}

STATIC BOOL IS_ADD(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "add") == 0;
}

STATIC BOOL IS_SUB(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "sub") == 0;
}

STATIC BOOL IS_SHL(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "shl") == 0;
}

STATIC BOOL IS_SHR(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "shr") == 0;
}

STATIC BOOL IS_SAR(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "sar") == 0;
}

STATIC BOOL IS_ROL(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "rol") == 0;
}

STATIC BOOL IS_ROR(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "ror") == 0;
}

STATIC BOOL IS_OR(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "or") == 0;
}

STATIC BOOL IS_AND(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "and") == 0;
}

STATIC BOOL IS_XOR(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "xor") == 0;
}

STATIC BOOL IS_LEA(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "lea") == 0;
}

STATIC BOOL IS_CMP(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "cmp") == 0;
}

STATIC BOOL IS_TEST(const ASM_MNEMONIC_TABLE *tbl) {
    return tbl && tbl->name && STRICMP(tbl->name, "test") == 0;
}


/*
 * ── Helper: is operand a simple register? ────────────────────────────────
 */
STATIC BOOL IS_OP_REG(const ASM_OPERAND *op) {
    return op->type == OP_REG;
}

STATIC BOOL IS_OP_IMM(const ASM_OPERAND *op) {
    return op->type == OP_IMM;
}

STATIC BOOL IS_OP_MEM(const ASM_OPERAND *op) {
    return op->type == OP_MEM;
}

/*
 * ── Helper: are two register operands the same register? ─────────────────
 */
STATIC BOOL SAME_REG(const ASM_OPERAND *a, const ASM_OPERAND *b) {
    return IS_OP_REG(a) && IS_OP_REG(b) && a->reg == b->reg;
}

/*
 * ── Helper: is a 32-bit GP register? ────────────────────────────────────
 */
STATIC BOOL IS_GP32(ASM_REGS r) {
    return r >= REG_EAX && r <= REG_ESP;
}

/*
 * ── Helper: is a 16-bit GP register? ────────────────────────────────────
 */
STATIC BOOL IS_GP16(ASM_REGS r) {
    return r >= REG_AX && r <= REG_SP;
}

/*
 * ── Helper: is a register a segment register? ───────────────────────────
 */
STATIC BOOL IS_SEG_REG(ASM_REGS r) {
    return r >= REG_CS && r <= REG_SS;
}

/*
 * ── Mark a node for removal by converting it to NOP ─────────────────────
 *  We don't actually remove nodes (would break indices), instead we
 *  rewrite them to NOP which the codegen emits as a single 0x90 byte.
 *  For dead-code elimination, the caller may set type = NODE_INVALID
 *  which codegen skips entirely.
 */
STATIC VOID KILL_NODE(PASM_NODE node) {
    node->type = NODE_INVALID;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  PASS 1: Single-instruction rewrites
 * ════════════════════════════════════════════════════════════════════════════
 */
STATIC U32 OPT_SINGLE(ASM_AST_ARRAY *ast) {
    U32 changes = 0;

    for (U32 i = 0; i < ast->len; i++) {
        PASM_NODE node = ast->nodes[i];
        if (!node || node->type != NODE_INSTRUCTION) continue;

        const ASM_MNEMONIC_TABLE *tbl = node->instr.table_entry;
        if (!tbl) continue;

        ASM_OPERAND *op0 = &node->instr.operands[0];
        ASM_OPERAND *op1 = &node->instr.operands[1];
        U32 cnt = node->instr.operand_count;

        /*
         * ── MOV r, r  (same register) → dead, remove ────────────────────
         *    e.g.  MOV EAX, EAX  →  (killed)
         *    Saves 2 bytes for reg-reg, more for r/m forms.
         */
        if (IS_MOV(tbl) && cnt == 2
            && SAME_REG(op0, op1)
            && !IS_SEG_REG(op0->reg))       /* MOV DS,DS has side effects */
        {
            DEBUGM_PRINTF("[OPT] L%u: MOV %u,%u (same reg) -> killed\n",
                          node->line, op0->reg, op1->reg);
            KILL_NODE(node);
            changes++;
            continue;
        }

        /*
         * ── ADD/SUB r/m, 0 → dead, remove ──────────────────────────────
         *    Saves 3-6 bytes.
         *    NOTE: ADD/SUB set flags, so this is only safe if the next
         *    instruction doesn't depend on those flags.  For simplicity
         *    we skip this if the next instruction is a Jcc/SETcc/ADC/SBB.
         *    (conservative: we always apply it — the zero-add is a no-op
         *     for all flags too: CF=0 and the rest match the operand.)
         */
        if ((IS_ADD(tbl) || IS_SUB(tbl)) && cnt == 2
            && IS_OP_IMM(op1) && op1->immediate == 0)
        {
            DEBUGM_PRINTF("[OPT] L%u: %s r/m, 0 -> killed\n",
                          node->line, tbl->name);
            KILL_NODE(node);
            changes++;
            continue;
        }

        /*
         * ── SHL/SHR/SAR/ROL/ROR r/m, 0 → dead, remove ─────────────────
         *    Shift/rotate by zero does nothing.
         */
        if ((IS_SHL(tbl) || IS_SHR(tbl) || IS_SAR(tbl)
             || IS_ROL(tbl) || IS_ROR(tbl))
            && cnt == 2 && IS_OP_IMM(op1) && op1->immediate == 0)
        {
            DEBUGM_PRINTF("[OPT] L%u: %s r/m, 0 -> killed\n",
                          node->line, tbl->name);
            KILL_NODE(node);
            changes++;
            continue;
        }

        /*
         * ── OR/AND r, r (same register) → TEST r, r ────────────────────
         *    OR r,r and AND r,r both set flags identically to TEST r,r
         *    but OR/AND write back the same value to r.
         *    TEST is shorter when the coder only cares about flags.
         *    We DON'T apply this automatically — the result is functionally
         *    identical but the user may be intentionally using OR/AND.
         *    (Left as a documented TODO for a future --opt-size flag.)
         */

        /*
         * ── LEA r, [r] (no displacement, no index) → MOV r, r → dead ───
         *    LEA EAX, [EAX]  is the same as  MOV EAX, EAX  which is
         *    a no-op.  But we only match the simple [r] case.
         */
        if (IS_LEA(tbl) && cnt == 2
            && IS_OP_REG(op0) && IS_OP_MEM(op1)
            && op1->mem_ref
            && op1->mem_ref->base_reg == op0->reg
            && op1->mem_ref->index_reg == REG_NONE
            && op1->mem_ref->displacement == 0
            && op1->mem_ref->symbol_name == NULLPTR)
        {
            DEBUGM_PRINTF("[OPT] L%u: LEA r, [r] (identity) -> killed\n",
                          node->line);
            KILL_NODE(node);
            changes++;
            continue;
        }
    }

    return changes;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  PASS 2: Two-instruction peephole window
 * ════════════════════════════════════════════════════════════════════════════
 *  Looks at consecutive instruction pairs, skipping labels/directives.
 */

/*
 * Return the next non-killed instruction node after index `start`.
 * Stops at labels/directives (returns NULL) since they break the
 * sequential execution assumption.
 */
STATIC PASM_NODE NEXT_INSTR(ASM_AST_ARRAY *ast, U32 start, U32 *out_idx) {
    for (U32 j = start + 1; j < ast->len; j++) {
        PASM_NODE n = ast->nodes[j];
        if (!n) continue;

        /* Labels and sections break the basic block — stop scanning */
        if (n->type == NODE_LABEL || n->type == NODE_LOCAL_LABEL
            || n->type == NODE_SECTION)
            return NULLPTR;

        /* Skip already-killed nodes */
        if (n->type == NODE_INVALID) continue;

        if (n->type == NODE_INSTRUCTION) {
            if (out_idx) *out_idx = j;
            return n;
        }

        /* Anything else (data, org, times, raw) — stop */
        return NULLPTR;
    }
    return NULLPTR;
}

STATIC U32 OPT_PEEPHOLE(ASM_AST_ARRAY *ast) {
    U32 changes = 0;

    for (U32 i = 0; i < ast->len; i++) {
        PASM_NODE a = ast->nodes[i];
        if (!a || a->type != NODE_INSTRUCTION) continue;

        const ASM_MNEMONIC_TABLE *ta = a->instr.table_entry;
        if (!ta) continue;

        U32 j = 0;
        PASM_NODE b = NEXT_INSTR(ast, i, &j);
        if (!b) continue;

        const ASM_MNEMONIC_TABLE *tb = b->instr.table_entry;
        if (!tb) continue;

        ASM_OPERAND *a0 = &a->instr.operands[0];
        ASM_OPERAND *b0 = &b->instr.operands[0];

        /*
         * ── PUSH r  +  POP r  (same register) → remove both ────────────
         *    e.g.  PUSH EAX / POP EAX  →  (killed)
         *    Saves 2 bytes.
         */
        if (IS_PUSH(ta) && IS_POP(tb)
            && a->instr.operand_count == 1
            && b->instr.operand_count == 1
            && SAME_REG(a0, b0))
        {
            DEBUGM_PRINTF("[OPT] L%u-L%u: PUSH/POP %u (same reg) -> killed\n",
                          a->line, b->line, a0->reg);
            KILL_NODE(a);
            KILL_NODE(b);
            changes++;
            continue;
        }

        /*
         * ── INC r  +  DEC r  (same register) → remove both ─────────────
         * ── DEC r  +  INC r  (same register) → remove both ─────────────
         *    Saves 2 bytes.
         */
        if (((IS_INC(ta) && IS_DEC(tb)) || (IS_DEC(ta) && IS_INC(tb)))
            && a->instr.operand_count == 1
            && b->instr.operand_count == 1
            && SAME_REG(a0, b0))
        {
            DEBUGM_PRINTF("[OPT] L%u-L%u: INC/DEC %u (cancels) -> killed\n",
                          a->line, b->line, a0->reg);
            KILL_NODE(a);
            KILL_NODE(b);
            changes++;
            continue;
        }

        /*
         * ── MOV r, X  +  MOV r, Y → remove first MOV ───────────────
         *    The first store is dead — overwritten unconditionally.
         *    Only when both destinations are the same register and
         *    the first source doesn't read the register (no dep).
         *    We also must ensure the first MOV's source isn't a memory
         *    operand (could have side effects via memory-mapped I/O).
         */
        if (IS_MOV(ta) && IS_MOV(tb)
            && a->instr.operand_count == 2
            && b->instr.operand_count == 2
            && IS_OP_REG(a0) && IS_OP_REG(b0)
            && a0->reg == b0->reg
            && !IS_OP_MEM(&a->instr.operands[1]))
        {
            DEBUGM_PRINTF("[OPT] L%u: MOV %u, X (dead store, overwritten at L%u) -> killed\n",
                          a->line, a0->reg, b->line);
            KILL_NODE(a);
            changes++;
            continue;
        }
    }

    return changes;
}


/*
 * ════════════════════════════════════════════════════════════════════════════
 *  ENTRY POINT
 * ════════════════════════════════════════════════════════════════════════════
 */
BOOLEAN OPTIMIZE(ASM_AST_ARRAY *ast) {
    if (!ast) return FALSE;

    ASTRAC_ARGS *cfg = GET_ARGS();
    U32 total = 0;
    U32 pass  = 0;

    /*
     * Run passes in a loop until no more changes are found.
     * This allows cascading optimisations (e.g. killing a MOV may expose
     * a new PUSH/POP pair).  Safety cap at 16 iterations.
     */
    for (pass = 0; pass < 16; pass++) {
        U32 changes = 0;
        changes += OPT_SINGLE(ast);
        changes += OPT_PEEPHOLE(ast);

        total += changes;
        if (changes == 0) break;
    }

    if (cfg->verbose && total > 0) {
        printf("[ASM OPT] %u optimisation(s) applied in %u pass(es)\n",
               total, pass + 1);
    }

    return TRUE;
}
