#ifndef MNEMONICS_H
#define MNEMONICS_H
#include <STD/TYPEDEF.h>
#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  MNEMONIC X-MACRO  —  stepped ladder
 *
 *  The list (MNEMONIC_LIST.h) is included TWICE:
 *    Pass 1  →  ASM_MNEMONIC enum    (each macro emits  id,)
 *    Pass 2  →  asm_mnemonics[]      (each macro emits a full struct literal)
 *
 * ─── RUNGS ──────────────────────────────────────────────────────────────────
 *
 *  Every macro takes a 'desc' string as its LAST parameter — a short
 *  human-readable description stored in the table's "description" field.
 *
 *  MNEM_NOOPS(id, name, op1, desc)
 *    No operands, single-byte opcode, no ModRM.
 *    Defaults: ENC_DIRECT | OPS_NONE | OPN_NONE | SZ_NONE | MEM_NONE
 *
 *  MNEM_REG_ENC(id, name, op1, ops, sz, desc)
 *    Register encoded in low 3 bits of opcode (+rd).
 *    Defaults: ENC_REG_OPCODE | OPN_ONE | MEM_NONE
 *
 *  MNEM_IMM(id, name, op1, ops, cnt, sz, desc)
 *    Immediate operand(s), no ModRM.
 *    Defaults: ENC_IMM | MEM_NONE
 *
 *  MNEM_RM(id, name, op1, ops, cnt, sz, ext, desc)
 *    ModRM encoding.  ext = MODRM_NONE for /r, MODRM_0..7 for extension digit.
 *    Defaults: ENC_MODRM | has_modrm=TRUE | MEM_RW | modified_flags=FLG_ARITH
 *
 *  MNEM_RM_NF(id, name, op1, ops, cnt, sz, ext, desc)
 *    Same as MNEM_RM but does NOT modify flags (NF = No Flags).
 *    Use for: PUSH/POP rm, CALL/JMP indirect, LEA, XCHG, MOV.
 *    Defaults: ENC_MODRM | has_modrm=TRUE | all flags = FLG_NONE
 *
 *  MNEM_RM_P(id, name, pfx, op1, ops, cnt, sz, ext, desc)
 *    Same as MNEM_RM but with an explicit legacy prefix (e.g. PF_66 for
 *    16-bit operand-size override).  Sets FLG_ARITH.
 *
 *  MNEM_RM_NF_P(id, name, pfx, op1, ops, cnt, sz, ext, desc)
 *    Same as MNEM_RM_NF but with an explicit legacy prefix.  No flags.
 *
 *  MNEM_REL(id, name, op1, rel_type, desc)
 *    PC-relative branch (rel8 / rel32).
 *    Defaults: ENC_IMM | OPN_ONE | SZ_NONE | MEM_NONE | tested_flags=FLG_ALL
 *
 *  MNEM_0F(id, name, op2, ops, cnt, sz, ext, desc)
 *    Two-byte (0F xx) opcode, ModRM.
 *    Defaults: PFX_0F | ENC_MODRM | has_modrm=TRUE | PROC_80386 | EX_PREFIX_0F
 *
 *  MNEM_0F_REL(id, name, op2, rel_type, desc)
 *    Two-byte (0F xx) conditional branch, rel32.
 *    Defaults: PFX_0F | ENC_IMM | OPN_ONE | PROC_80386 | tested_flags=FLG_ALL
 *
 *  MNEMONIC(id, name, prefix, opcode, enc, ops, cnt, sz,
 *           reg_fixed, ext, has_modrm, proc, status, mode,
 *           mem, rel, lock, opcode_ext,
 *           tst_f, mod_f, def_f, undef_f, flg_val, desc)
 *    Full explicit form — all 24 fields supplied manually.
 * ════════════════════════════════════════════════════════════════════════════
 */


/* ── PASS 1 : enum values ────────────────────────────────────────────────── */
#define MNEM_NOOPS(id,name,op1,desc)                              id,
#define MNEM_REG_ENC(id,name,op1,ops,sz,desc)                     id,
#define MNEM_IMM(id,name,op1,ops,cnt,sz,desc)                     id,
#define MNEM_RM(id,name,op1,ops,cnt,sz,ext,desc)                  id,
#define MNEM_RM_NF(id,name,op1,ops,cnt,sz,ext,desc)               id,
#define MNEM_RM_P(id,name,pfx,op1,ops,cnt,sz,ext,desc)            id,
#define MNEM_RM_NF_P(id,name,pfx,op1,ops,cnt,sz,ext,desc)         id,
#define MNEM_REL(id,name,op1,rel_type,desc)                       id,
#define MNEM_0F(id,name,op2,ops,cnt,sz,ext,desc)                  id,
#define MNEM_0F_REL(id,name,op2,rel_type,desc)                    id,
#define MNEMONIC(id,name,prefix,opcode,enc,ops,cnt,sz,   \
                 reg_fixed,ext,has_modrm,proc,status,    \
                 mode,mem,rel,lock,opcode_ext,            \
                 tst_f,mod_f,def_f,undef_f,flg_val,desc) id,

typedef enum _ASM_MNEMONIC {
    MNEM_NONE = 0,
    #include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONIC_LIST.h>
    MNEM_COUNT,
} ASM_MNEMONIC;

#undef MNEM_NOOPS
#undef MNEM_REG_ENC
#undef MNEM_IMM
#undef MNEM_RM
#undef MNEM_RM_NF
#undef MNEM_RM_P
#undef MNEM_RM_NF_P
#undef MNEM_REL
#undef MNEM_0F
#undef MNEM_0F_REL
#undef MNEMONIC

#define OPCODE(...) { __VA_ARGS__ }


/* ── PASS 2 : table struct literals ─────────────────────────────────────── */

/* No operands, single-byte opcode, no ModRM */
#define MNEM_NOOPS(id,name,op1,desc)                                          \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_DIRECT, OPS_NONE,   \
      OPN_NONE, SZ_NONE, REG_NONE, MODRM_NONE, FALSE,                        \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_NONE, RL_NONE,                   \
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

/* Register encoded in low 3 opcode bits (+rd) */
#define MNEM_REG_ENC(id,name,op1,ops,sz,desc)                                 \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_REG_OPCODE, ops,    \
      OPN_ONE, sz, REG_NONE, MODRM_NONE, FALSE,                              \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_NONE, RL_NONE,                   \
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

/* Immediate operand(s), no ModRM */
#define MNEM_IMM(id,name,op1,ops,cnt,sz,desc)                                 \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_IMM, ops,           \
      cnt, sz, REG_NONE, MODRM_NONE, FALSE,                                  \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_NONE, RL_NONE,                   \
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

/* ModRM encoding — ext=MODRM_NONE for /r, MODRM_0..7 for digit */
#define MNEM_RM(id,name,op1,ops,cnt,sz,ext,desc)                              \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_MODRM, ops,         \
      cnt, sz, REG_NONE, ext, TRUE,                                           \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_RW, RL_NONE,                     \
      X_NONE, EX_NONE, FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE, desc },

/* ModRM encoding, no flag side-effects (PUSH/POP/CALL/JMP/LEA/MOV/XCHG) */
#define MNEM_RM_NF(id,name,op1,ops,cnt,sz,ext,desc)                          \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_MODRM, ops,         \
      cnt, sz, REG_NONE, ext, TRUE,                                           \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_RW, RL_NONE,                     \
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

/* ModRM with explicit prefix — for 16-bit operand-size override or REP etc. */
#define MNEM_RM_P(id,name,pfx,op1,ops,cnt,sz,ext,desc)                       \
    { id, name, pfx, PFX_NONE, OPCODE(op1,0x00), ENC_MODRM, ops,             \
      cnt, sz, REG_NONE, ext, TRUE,                                           \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_RW, RL_NONE,                     \
      X_NONE, EX_NONE, FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE, desc },

/* ModRM, no flags, with explicit prefix */
#define MNEM_RM_NF_P(id,name,pfx,op1,ops,cnt,sz,ext,desc)                    \
    { id, name, pfx, PFX_NONE, OPCODE(op1,0x00), ENC_MODRM, ops,             \
      cnt, sz, REG_NONE, ext, TRUE,                                           \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_RW, RL_NONE,                     \
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

/* PC-relative branch */
#define MNEM_REL(id,name,op1,rel_type,desc)                                   \
    { id, name, PF_NONE, PFX_NONE, OPCODE(op1,0x00), ENC_IMM, OPS_REL8,      \
      OPN_ONE, SZ_NONE, REG_NONE, MODRM_NONE, FALSE,                         \
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_NONE, rel_type,                  \
      X_NONE, EX_NONE, FLG_ALL, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

/* Two-byte (0F xx) opcode, ModRM */
#define MNEM_0F(id,name,op2,ops,cnt,sz,ext,desc)                              \
    { id, name, PF_NONE, PFX_0F, OPCODE(0x0F,op2), ENC_MODRM, ops,           \
      cnt, sz, REG_NONE, ext, TRUE,                                           \
      PROC_80386, ST_DOCUMENTED, MODE_ANY, MEM_RW, RL_NONE,                   \
      X_NONE, EX_PREFIX_0F, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

/* Two-byte (0F xx) conditional branch, rel32 */
#define MNEM_0F_REL(id,name,op2,rel_type,desc)                                \
    { id, name, PF_NONE, PFX_0F, OPCODE(0x0F,op2), ENC_IMM, OPS_REL32,       \
      OPN_ONE, SZ_NONE, REG_NONE, MODRM_NONE, FALSE,                         \
      PROC_80386, ST_DOCUMENTED, MODE_ANY, MEM_NONE, rel_type,                \
      X_NONE, EX_PREFIX_0F, FLG_ALL, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, desc },

/* Full explicit form */
#define MNEMONIC(id,name,prefix,opcode,enc,ops,cnt,sz,                        \
                 reg_fixed,ext,has_modrm,proc,status,                         \
                 mode,mem,rel,lock,opcode_ext,                                 \
                 tst_f,mod_f,def_f,undef_f,flg_val,desc)                      \
    { id, name, prefix, PFX_NONE, opcode, enc, ops,                           \
      cnt, sz, reg_fixed, ext, has_modrm,                                     \
      proc, status, mode, mem, rel, lock, opcode_ext,                         \
      tst_f, mod_f, def_f, undef_f, flg_val, desc },


/*
 * ─── MNEMONIC TABLE ──────────────────────────────────────────────────────────
 * asm_mnemonics[0]         = MNEM_NONE sentinel
 * asm_mnemonics[MNEM_FOO]  = direct O(1) lookup by enum value
 */
static const ASM_MNEMONIC_TABLE asm_mnemonics[] = {
    /* [0] MNEM_NONE sentinel */
    { MNEM_NONE, "none", PF_NONE, PFX_NONE, OPCODE(0x00,0x00), ENC_DIRECT, OPS_NONE,
      OPN_NONE, SZ_NONE, REG_NONE, MODRM_NONE, FALSE,
      PROC_ANY, ST_DOCUMENTED, MODE_ANY, MEM_NONE, RL_NONE,
      X_NONE, EX_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, "" },
    #include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONIC_LIST.h>
};

#undef MNEM_NOOPS
#undef MNEM_REG_ENC
#undef MNEM_IMM
#undef MNEM_RM
#undef MNEM_RM_NF
#undef MNEM_RM_P
#undef MNEM_RM_NF_P
#undef MNEM_REL
#undef MNEM_0F
#undef MNEM_0F_REL
#undef MNEMONIC

#endif /* MNEMONICS_H */
