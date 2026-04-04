/*
 * MNEMONIC_LIST.h — i386 instruction table entries (8/16/32-bit variants)
 *
 * Included twice by MNEMONICS.h:
 *   Pass 1  →  ASM_MNEMONIC enum  (id,)
 *   Pass 2  →  asm_mnemonics[]    (struct literal)
 *
 * Shortcut macros (all take 'desc' string as last parameter):
 *   MNEM_NOOPS   (id, name, op1, desc)
 *   MNEM_REG_ENC (id, name, op1, ops, sz, desc)
 *   MNEM_IMM     (id, name, op1, ops, cnt, sz, desc)
 *   MNEM_RM      (id, name, op1, ops, cnt, sz, ext, desc)      ← FLG_ARITH
 *   MNEM_RM_NF   (id, name, op1, ops, cnt, sz, ext, desc)      ← no flags
 *   MNEM_RM_P    (id, name, pfx, op1, ops, cnt, sz, ext, desc) ← prefix + FLG_ARITH
 *   MNEM_RM_NF_P (id, name, pfx, op1, ops, cnt, sz, ext, desc) ← prefix + no flags
 *   MNEM_REL     (id, name, op1, rel_type, desc)
 *   MNEM_0F      (id, name, op2, ops, cnt, sz, ext, desc)
 *   MNEM_0F_REL  (id, name, op2, rel_type, desc)
 *   MNEMONIC     (full 24-field form)
 *
 * Size encoding convention in enum names:
 *   _RM8  / _R8  / _IMM8    →  8-bit operands
 *   _RM16 / _R16 / _IMM16   →  16-bit operands (66h prefix in 32-bit mode)
 *   _RM32 / _R32 / _IMM32   →  32-bit operands (default in 32-bit mode)
 */


/* ════════════════════════════════════════════════════════════════════════════
 *  NO-OPERAND  (single-byte opcode, ENC_DIRECT)
 *  These have no size variants — they operate implicitly.
 * ════════════════════════════════════════════════════════════════════════════ */
MNEM_NOOPS(MNEM_NOP,    "nop",   0x90, "No operation")
MNEM_NOOPS(MNEM_HLT,    "hlt",   0xF4, "Halt processor until interrupt")
MNEM_NOOPS(MNEM_RET,    "ret",   0xC3, "Return from near procedure")
MNEM_NOOPS(MNEM_RETF,   "retf",  0xCB, "Return from far procedure")
MNEM_NOOPS(MNEM_LEAVE,  "leave", 0xC9, "Restore stack frame (MOV ESP,EBP; POP EBP)")
MNEM_NOOPS(MNEM_PUSHA,  "pusha", 0x60, "Push all general-purpose registers")
MNEM_NOOPS(MNEM_POPA,   "popa",  0x61, "Pop all general-purpose registers")
MNEM_NOOPS(MNEM_PUSHF,  "pushf", 0x9C, "Push EFLAGS onto stack")
MNEM_NOOPS(MNEM_POPF,   "popf",  0x9D, "Pop EFLAGS from stack")
MNEM_NOOPS(MNEM_INT3,   "int3",  0xCC, "Breakpoint trap (debug interrupt 3)")
MNEM_NOOPS(MNEM_INTO,   "into",  0xCE, "Interrupt on overflow (INT 4 if OF=1)")
MNEM_NOOPS(MNEM_IRET,   "iret",  0xCF, "Return from interrupt")
MNEM_NOOPS(MNEM_CLC,    "clc",   0xF8, "Clear carry flag")
MNEM_NOOPS(MNEM_STC,    "stc",   0xF9, "Set carry flag")
MNEM_NOOPS(MNEM_CMC,    "cmc",   0xF5, "Complement (toggle) carry flag")
MNEM_NOOPS(MNEM_CLI,    "cli",   0xFA, "Clear interrupt flag (disable interrupts)")
MNEM_NOOPS(MNEM_STI,    "sti",   0xFB, "Set interrupt flag (enable interrupts)")
MNEM_NOOPS(MNEM_CLD,    "cld",   0xFC, "Clear direction flag (forward string ops)")
MNEM_NOOPS(MNEM_STD,    "std",   0xFD, "Set direction flag (backward string ops)")
MNEM_NOOPS(MNEM_SAHF,   "sahf",  0x9E, "Store AH into low 8 bits of EFLAGS")
MNEM_NOOPS(MNEM_LAHF,   "lahf",  0x9F, "Load low 8 bits of EFLAGS into AH")
MNEM_NOOPS(MNEM_XLAT,   "xlat",  0xD7, "Table look-up: AL = [EBX + AL]")
MNEM_NOOPS(MNEM_AAA,    "aaa",   0x37, "ASCII adjust AL after addition")
MNEM_NOOPS(MNEM_AAS,    "aas",   0x3F, "ASCII adjust AL after subtraction")
MNEM_NOOPS(MNEM_DAA,    "daa",   0x27, "Decimal adjust AL after addition")
MNEM_NOOPS(MNEM_DAS,    "das",   0x2F, "Decimal adjust AL after subtraction")
MNEM_NOOPS(MNEM_CWDE,   "cwde",  0x98, "Sign-extend AX into EAX (32-bit)")
MNEM_NOOPS(MNEM_CDQ,    "cdq",   0x99, "Sign-extend EAX into EDX:EAX (32-bit)")
MNEM_NOOPS(MNEM_CBW,    "cbw",   0x98, "Sign-extend AL into AX (16-bit)")
MNEM_NOOPS(MNEM_WAIT,   "wait",  0x9B, "Wait for pending FPU exceptions")


/* ════════════════════════════════════════════════════════════════════════════
 *  REGISTER-IN-OPCODE  (+rd / +rw, ENC_REG_OPCODE)
 *
 *  The register number is encoded in the low 3 bits of the opcode byte.
 *  16-bit forms use the 66h operand-size prefix.
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 32-bit (default in protected mode) ───────────────────────────────────── */
MNEM_REG_ENC(MNEM_PUSH_R32,   "push",  0x50, OPS_REG, SZ_32BIT, "Push r32 onto stack")
MNEM_REG_ENC(MNEM_POP_R32,    "pop",   0x58, OPS_REG, SZ_32BIT, "Pop stack top into r32")
MNEM_REG_ENC(MNEM_INC_R32,    "inc",   0x40, OPS_REG, SZ_32BIT, "Increment r32 by 1")
MNEM_REG_ENC(MNEM_DEC_R32,    "dec",   0x48, OPS_REG, SZ_32BIT, "Decrement r32 by 1")
MNEM_REG_ENC(MNEM_XCHG_EAX,  "xchg",  0x90, OPS_REG, SZ_32BIT, "Exchange r32 with EAX")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEMONIC(MNEM_PUSH_R16, "push",
         PF_66, OPCODE(0x50,0x00), ENC_REG_OPCODE, OPS_REG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Push r16 onto stack")
MNEMONIC(MNEM_POP_R16, "pop",
         PF_66, OPCODE(0x58,0x00), ENC_REG_OPCODE, OPS_REG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Pop stack top into r16")
MNEMONIC(MNEM_INC_R16, "inc",
         PF_66, OPCODE(0x40,0x00), ENC_REG_OPCODE, OPS_REG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_ARITH_NO_C, FLG_ARITH_NO_C, FLG_NONE, FLG_NONE,
         "Increment r16 by 1")
MNEMONIC(MNEM_DEC_R16, "dec",
         PF_66, OPCODE(0x48,0x00), ENC_REG_OPCODE, OPS_REG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_ARITH_NO_C, FLG_ARITH_NO_C, FLG_NONE, FLG_NONE,
         "Decrement r16 by 1")
MNEMONIC(MNEM_XCHG_AX, "xchg",
         PF_66, OPCODE(0x90,0x00), ENC_REG_OPCODE, OPS_REG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Exchange r16 with AX")


/* ════════════════════════════════════════════════════════════════════════════
 *  IMMEDIATE-ONLY  (ENC_IMM, no ModRM)
 * ════════════════════════════════════════════════════════════════════════════ */
MNEM_IMM(MNEM_INT,          "int",   0xCD, OPS_IMM8,   OPN_ONE, SZ_8BIT,  "Software interrupt (vector in imm8)")
MNEM_IMM(MNEM_PUSH_IMM8,    "push",  0x6A, OPS_IMM8,   OPN_ONE, SZ_8BIT,  "Push sign-extended imm8 onto stack")
MNEM_IMM(MNEM_PUSH_IMM32,   "push",  0x68, OPS_IMM32,  OPN_ONE, SZ_32BIT, "Push imm32 onto stack")
MNEM_IMM(MNEM_RET_IMM,      "ret",   0xC2, OPS_IMM16,  OPN_ONE, SZ_16BIT, "Near return, pop imm16 bytes from stack")
MNEM_IMM(MNEM_RETF_IMM,     "retf",  0xCA, OPS_IMM16,  OPN_ONE, SZ_16BIT, "Far return, pop imm16 bytes from stack")
MNEM_IMM(MNEM_AAM,          "aam",   0xD4, OPS_IMM8,   OPN_ONE, SZ_8BIT,  "ASCII adjust AX after multiply (base imm8)")
MNEM_IMM(MNEM_AAD,          "aad",   0xD5, OPS_IMM8,   OPN_ONE, SZ_8BIT,  "ASCII adjust AX before divide (base imm8)")

/* ── 16-bit push immediate (66h prefix) ─────────────────────────────────── */
MNEMONIC(MNEM_PUSH_IMM16, "push",
         PF_66, OPCODE(0x68,0x00), ENC_IMM, OPS_IMM16, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Push imm16 onto stack")


/* ════════════════════════════════════════════════════════════════════════════
 *  MOV  (no flag side-effects → MNEM_RM_NF / MNEM_RM_NF_P)
 *
 *  Opcodes: 88/89 = r→r/m, 8A/8B = r/m→r, C6/C7 /0 = imm→r/m, B0+/B8+ = imm→r
 *  8-bit uses 88/8A/C6/B0, 16/32-bit uses 89/8B/C7/B8 (66h selects 16-bit).
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit ────────────────────────────────────────────────────────────────── */
MNEM_RM_NF(MNEM_MOV_RM8_R8,      "mov", 0x88, OPS_RM8_R8,    OPN_TWO, SZ_8BIT,  MODRM_NONE, "Move r8 to r/m8")
MNEM_RM_NF(MNEM_MOV_R8_RM8,      "mov", 0x8A, OPS_R8_RM8,    OPN_TWO, SZ_8BIT,  MODRM_NONE, "Move r/m8 to r8")
MNEM_RM_NF(MNEM_MOV_RM8_IMM8,    "mov", 0xC6, OPS_RM8_IMM8,  OPN_TWO, SZ_8BIT,  MODRM_0,    "Move imm8 to r/m8")
MNEMONIC(MNEM_MOV_R8_IMM8, "mov",
         PF_NONE, OPCODE(0xB0,0x00), ENC_REG_OPCODE, OPS_R8_IMM8, OPN_TWO, SZ_8BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Move imm8 into r8 (+rb encoding)")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_NF_P(MNEM_MOV_RM16_R16,     "mov", PF_66, 0x89, OPS_RM16_R16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Move r16 to r/m16")
MNEM_RM_NF_P(MNEM_MOV_R16_RM16,     "mov", PF_66, 0x8B, OPS_R16_RM16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Move r/m16 to r16")
MNEM_RM_NF_P(MNEM_MOV_RM16_IMM16,   "mov", PF_66, 0xC7, OPS_RM16_IMM16, OPN_TWO, SZ_16BIT, MODRM_0,    "Move imm16 to r/m16")
MNEMONIC(MNEM_MOV_R16_IMM16, "mov",
         PF_66, OPCODE(0xB8,0x00), ENC_REG_OPCODE, OPS_R16_IMM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Move imm16 into r16 (+rw encoding)")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM_NF(MNEM_MOV_RM32_R32,    "mov", 0x89, OPS_RM32_R32,  OPN_TWO, SZ_32BIT, MODRM_NONE, "Move r32 to r/m32")
MNEM_RM_NF(MNEM_MOV_R32_RM32,    "mov", 0x8B, OPS_R32_RM32,  OPN_TWO, SZ_32BIT, MODRM_NONE, "Move r/m32 to r32")
MNEM_RM_NF(MNEM_MOV_RM32_IMM32,  "mov", 0xC7, OPS_RM32_IMM32,OPN_TWO, SZ_32BIT, MODRM_0,    "Move imm32 to r/m32")
MNEMONIC(MNEM_MOV_R32_IMM32, "mov",
         PF_NONE, OPCODE(0xB8,0x00), ENC_REG_OPCODE, OPS_R32_IMM32, OPN_TWO, SZ_32BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Move imm32 into r32 (+rd encoding)")


/* ════════════════════════════════════════════════════════════════════════════
 *  ADD  (sets OSZAPC flags)
 *
 *  Opcodes: 00/01 = r→r/m, 02/03 = r/m→r, 80 /0 = imm8→r/m8,
 *           81 /0 = imm16/32→r/m, 83 /0 = sign-extended imm8→r/m16/32
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit ────────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_ADD_RM8_R8,      "add", 0x00, OPS_RM8_R8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Add r8 to r/m8")
MNEM_RM(MNEM_ADD_R8_RM8,      "add", 0x02, OPS_R8_RM8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Add r/m8 to r8")
MNEM_RM(MNEM_ADD_RM8_IMM8,    "add", 0x80, OPS_RM8_IMM8,   OPN_TWO, SZ_8BIT,  MODRM_0,    "Add imm8 to r/m8")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_P(MNEM_ADD_RM16_R16,    "add", PF_66, 0x01, OPS_RM16_R16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Add r16 to r/m16")
MNEM_RM_P(MNEM_ADD_R16_RM16,    "add", PF_66, 0x03, OPS_R16_RM16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Add r/m16 to r16")
MNEM_RM_P(MNEM_ADD_RM16_IMM8,   "add", PF_66, 0x83, OPS_RM16_IMM8,  OPN_TWO, SZ_16BIT, MODRM_0,    "Add sign-extended imm8 to r/m16")
MNEM_RM_P(MNEM_ADD_RM16_IMM16,  "add", PF_66, 0x81, OPS_RM16_IMM16, OPN_TWO, SZ_16BIT, MODRM_0,    "Add imm16 to r/m16")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_ADD_RM32_R32,    "add", 0x01, OPS_RM32_R32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Add r32 to r/m32")
MNEM_RM(MNEM_ADD_R32_RM32,    "add", 0x03, OPS_R32_RM32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Add r/m32 to r32")
MNEM_RM(MNEM_ADD_RM32_IMM8,   "add", 0x83, OPS_RM32_IMM8,  OPN_TWO, SZ_32BIT, MODRM_0,    "Add sign-extended imm8 to r/m32")
MNEM_RM(MNEM_ADD_RM32_IMM32,  "add", 0x81, OPS_RM32_IMM32, OPN_TWO, SZ_32BIT, MODRM_0,    "Add imm32 to r/m32")


/* ════════════════════════════════════════════════════════════════════════════
 *  SUB  (sets OSZAPC flags)
 *
 *  Opcodes: 28/29 = r→r/m, 2A/2B = r/m→r, 80 /5 = imm8→r/m8,
 *           81 /5 = imm16/32→r/m, 83 /5 = sign-extended imm8→r/m16/32
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit ────────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_SUB_RM8_R8,      "sub", 0x28, OPS_RM8_R8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Subtract r8 from r/m8")
MNEM_RM(MNEM_SUB_R8_RM8,      "sub", 0x2A, OPS_R8_RM8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Subtract r/m8 from r8")
MNEM_RM(MNEM_SUB_RM8_IMM8,    "sub", 0x80, OPS_RM8_IMM8,   OPN_TWO, SZ_8BIT,  MODRM_5,    "Subtract imm8 from r/m8")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_P(MNEM_SUB_RM16_R16,    "sub", PF_66, 0x29, OPS_RM16_R16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Subtract r16 from r/m16")
MNEM_RM_P(MNEM_SUB_R16_RM16,    "sub", PF_66, 0x2B, OPS_R16_RM16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Subtract r/m16 from r16")
MNEM_RM_P(MNEM_SUB_RM16_IMM8,   "sub", PF_66, 0x83, OPS_RM16_IMM8,  OPN_TWO, SZ_16BIT, MODRM_5,    "Subtract sign-extended imm8 from r/m16")
MNEM_RM_P(MNEM_SUB_RM16_IMM16,  "sub", PF_66, 0x81, OPS_RM16_IMM16, OPN_TWO, SZ_16BIT, MODRM_5,    "Subtract imm16 from r/m16")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_SUB_RM32_R32,    "sub", 0x29, OPS_RM32_R32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Subtract r32 from r/m32")
MNEM_RM(MNEM_SUB_R32_RM32,    "sub", 0x2B, OPS_R32_RM32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Subtract r/m32 from r32")
MNEM_RM(MNEM_SUB_RM32_IMM8,   "sub", 0x83, OPS_RM32_IMM8,  OPN_TWO, SZ_32BIT, MODRM_5,    "Subtract sign-extended imm8 from r/m32")
MNEM_RM(MNEM_SUB_RM32_IMM32,  "sub", 0x81, OPS_RM32_IMM32, OPN_TWO, SZ_32BIT, MODRM_5,    "Subtract imm32 from r/m32")


/* ════════════════════════════════════════════════════════════════════════════
 *  AND  (sets OSZPC, clears OF/CF — FLG_LOGIC)
 *
 *  Opcodes: 20/21 = r→r/m, 22/23 = r/m→r, 80 /4, 81 /4, 83 /4
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit ────────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_AND_RM8_R8,      "and", 0x20, OPS_RM8_R8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Bitwise AND r8 with r/m8")
MNEM_RM(MNEM_AND_R8_RM8,      "and", 0x22, OPS_R8_RM8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Bitwise AND r/m8 with r8")
MNEM_RM(MNEM_AND_RM8_IMM8,    "and", 0x80, OPS_RM8_IMM8,   OPN_TWO, SZ_8BIT,  MODRM_4,    "Bitwise AND imm8 with r/m8")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_P(MNEM_AND_RM16_R16,    "and", PF_66, 0x21, OPS_RM16_R16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Bitwise AND r16 with r/m16")
MNEM_RM_P(MNEM_AND_R16_RM16,    "and", PF_66, 0x23, OPS_R16_RM16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Bitwise AND r/m16 with r16")
MNEM_RM_P(MNEM_AND_RM16_IMM8,   "and", PF_66, 0x83, OPS_RM16_IMM8,  OPN_TWO, SZ_16BIT, MODRM_4,    "Bitwise AND sign-extended imm8 with r/m16")
MNEM_RM_P(MNEM_AND_RM16_IMM16,  "and", PF_66, 0x81, OPS_RM16_IMM16, OPN_TWO, SZ_16BIT, MODRM_4,    "Bitwise AND imm16 with r/m16")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_AND_RM32_R32,    "and", 0x21, OPS_RM32_R32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Bitwise AND r32 with r/m32")
MNEM_RM(MNEM_AND_R32_RM32,    "and", 0x23, OPS_R32_RM32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Bitwise AND r/m32 with r32")
MNEM_RM(MNEM_AND_RM32_IMM8,   "and", 0x83, OPS_RM32_IMM8,  OPN_TWO, SZ_32BIT, MODRM_4,    "Bitwise AND sign-extended imm8 with r/m32")
MNEM_RM(MNEM_AND_RM32_IMM32,  "and", 0x81, OPS_RM32_IMM32, OPN_TWO, SZ_32BIT, MODRM_4,    "Bitwise AND imm32 with r/m32")


/* ════════════════════════════════════════════════════════════════════════════
 *  OR  (sets OSZPC, clears OF/CF — FLG_LOGIC)
 *
 *  Opcodes: 08/09 = r→r/m, 0A/0B = r/m→r, 80 /1, 81 /1, 83 /1
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit ────────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_OR_RM8_R8,       "or",  0x08, OPS_RM8_R8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Bitwise OR r8 with r/m8")
MNEM_RM(MNEM_OR_R8_RM8,       "or",  0x0A, OPS_R8_RM8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Bitwise OR r/m8 with r8")
MNEM_RM(MNEM_OR_RM8_IMM8,     "or",  0x80, OPS_RM8_IMM8,   OPN_TWO, SZ_8BIT,  MODRM_1,    "Bitwise OR imm8 with r/m8")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_P(MNEM_OR_RM16_R16,     "or",  PF_66, 0x09, OPS_RM16_R16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Bitwise OR r16 with r/m16")
MNEM_RM_P(MNEM_OR_R16_RM16,     "or",  PF_66, 0x0B, OPS_R16_RM16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Bitwise OR r/m16 with r16")
MNEM_RM_P(MNEM_OR_RM16_IMM8,    "or",  PF_66, 0x83, OPS_RM16_IMM8,  OPN_TWO, SZ_16BIT, MODRM_1,    "Bitwise OR sign-extended imm8 with r/m16")
MNEM_RM_P(MNEM_OR_RM16_IMM16,   "or",  PF_66, 0x81, OPS_RM16_IMM16, OPN_TWO, SZ_16BIT, MODRM_1,    "Bitwise OR imm16 with r/m16")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_OR_RM32_R32,     "or",  0x09, OPS_RM32_R32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Bitwise OR r32 with r/m32")
MNEM_RM(MNEM_OR_R32_RM32,     "or",  0x0B, OPS_R32_RM32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Bitwise OR r/m32 with r32")
MNEM_RM(MNEM_OR_RM32_IMM8,    "or",  0x83, OPS_RM32_IMM8,  OPN_TWO, SZ_32BIT, MODRM_1,    "Bitwise OR sign-extended imm8 with r/m32")
MNEM_RM(MNEM_OR_RM32_IMM32,   "or",  0x81, OPS_RM32_IMM32, OPN_TWO, SZ_32BIT, MODRM_1,    "Bitwise OR imm32 with r/m32")


/* ════════════════════════════════════════════════════════════════════════════
 *  XOR  (sets OSZPC, clears OF/CF — FLG_LOGIC)
 *
 *  Opcodes: 30/31 = r→r/m, 32/33 = r/m→r, 80 /6, 81 /6, 83 /6
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit ────────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_XOR_RM8_R8,      "xor", 0x30, OPS_RM8_R8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Bitwise XOR r8 with r/m8")
MNEM_RM(MNEM_XOR_R8_RM8,      "xor", 0x32, OPS_R8_RM8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Bitwise XOR r/m8 with r8")
MNEM_RM(MNEM_XOR_RM8_IMM8,    "xor", 0x80, OPS_RM8_IMM8,   OPN_TWO, SZ_8BIT,  MODRM_6,    "Bitwise XOR imm8 with r/m8")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_P(MNEM_XOR_RM16_R16,    "xor", PF_66, 0x31, OPS_RM16_R16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Bitwise XOR r16 with r/m16")
MNEM_RM_P(MNEM_XOR_R16_RM16,    "xor", PF_66, 0x33, OPS_R16_RM16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Bitwise XOR r/m16 with r16")
MNEM_RM_P(MNEM_XOR_RM16_IMM8,   "xor", PF_66, 0x83, OPS_RM16_IMM8,  OPN_TWO, SZ_16BIT, MODRM_6,    "Bitwise XOR sign-extended imm8 with r/m16")
MNEM_RM_P(MNEM_XOR_RM16_IMM16,  "xor", PF_66, 0x81, OPS_RM16_IMM16, OPN_TWO, SZ_16BIT, MODRM_6,    "Bitwise XOR imm16 with r/m16")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_XOR_RM32_R32,    "xor", 0x31, OPS_RM32_R32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Bitwise XOR r32 with r/m32")
MNEM_RM(MNEM_XOR_R32_RM32,    "xor", 0x33, OPS_R32_RM32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Bitwise XOR r/m32 with r32")
MNEM_RM(MNEM_XOR_RM32_IMM8,   "xor", 0x83, OPS_RM32_IMM8,  OPN_TWO, SZ_32BIT, MODRM_6,    "Bitwise XOR sign-extended imm8 with r/m32")
MNEM_RM(MNEM_XOR_RM32_IMM32,  "xor", 0x81, OPS_RM32_IMM32, OPN_TWO, SZ_32BIT, MODRM_6,    "Bitwise XOR imm32 with r/m32")


/* ════════════════════════════════════════════════════════════════════════════
 *  CMP  (like SUB but discards result — sets OSZAPC flags)
 *
 *  Opcodes: 38/39 = r,r/m  3A/3B = r/m,r  80 /7, 81 /7, 83 /7
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit ────────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_CMP_RM8_R8,      "cmp", 0x38, OPS_RM8_R8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Compare r8 with r/m8 (set flags, discard result)")
MNEM_RM(MNEM_CMP_R8_RM8,      "cmp", 0x3A, OPS_R8_RM8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Compare r/m8 with r8 (set flags, discard result)")
MNEM_RM(MNEM_CMP_RM8_IMM8,    "cmp", 0x80, OPS_RM8_IMM8,   OPN_TWO, SZ_8BIT,  MODRM_7,    "Compare imm8 with r/m8")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_P(MNEM_CMP_RM16_R16,    "cmp", PF_66, 0x39, OPS_RM16_R16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Compare r16 with r/m16")
MNEM_RM_P(MNEM_CMP_R16_RM16,    "cmp", PF_66, 0x3B, OPS_R16_RM16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Compare r/m16 with r16")
MNEM_RM_P(MNEM_CMP_RM16_IMM8,   "cmp", PF_66, 0x83, OPS_RM16_IMM8,  OPN_TWO, SZ_16BIT, MODRM_7,    "Compare sign-extended imm8 with r/m16")
MNEM_RM_P(MNEM_CMP_RM16_IMM16,  "cmp", PF_66, 0x81, OPS_RM16_IMM16, OPN_TWO, SZ_16BIT, MODRM_7,    "Compare imm16 with r/m16")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_CMP_RM32_R32,    "cmp", 0x39, OPS_RM32_R32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Compare r32 with r/m32")
MNEM_RM(MNEM_CMP_R32_RM32,    "cmp", 0x3B, OPS_R32_RM32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Compare r/m32 with r32")
MNEM_RM(MNEM_CMP_RM32_IMM8,   "cmp", 0x83, OPS_RM32_IMM8,  OPN_TWO, SZ_32BIT, MODRM_7,    "Compare sign-extended imm8 with r/m32")
MNEM_RM(MNEM_CMP_RM32_IMM32,  "cmp", 0x81, OPS_RM32_IMM32, OPN_TWO, SZ_32BIT, MODRM_7,    "Compare imm32 with r/m32")


/* ════════════════════════════════════════════════════════════════════════════
 *  TEST  (like AND but discards result — sets OSZPC, clears OF/CF)
 *
 *  Opcodes: 84/85 = r/m,r  F6 /0 = r/m8,imm8  F7 /0 = r/m16/32,imm
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit ────────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_TEST_RM8_R8,     "test", 0x84, OPS_RM8_R8,    OPN_TWO, SZ_8BIT,  MODRM_NONE, "Test r/m8 AND r8 (set flags, discard result)")
MNEM_RM(MNEM_TEST_RM8_IMM8,   "test", 0xF6, OPS_RM8_IMM8,  OPN_TWO, SZ_8BIT,  MODRM_0,    "Test r/m8 AND imm8 (set flags, discard result)")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_P(MNEM_TEST_RM16_R16,    "test", PF_66, 0x85, OPS_RM16_R16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Test r/m16 AND r16")
MNEM_RM_P(MNEM_TEST_RM16_IMM16,  "test", PF_66, 0xF7, OPS_RM16_IMM16, OPN_TWO, SZ_16BIT, MODRM_0,    "Test r/m16 AND imm16")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_TEST_RM32_R32,   "test", 0x85, OPS_RM32_R32,  OPN_TWO, SZ_32BIT, MODRM_NONE, "Test r/m32 AND r32 (set flags, discard result)")
MNEM_RM(MNEM_TEST_RM32_IMM32, "test", 0xF7, OPS_RM32_IMM32,OPN_TWO, SZ_32BIT, MODRM_0,    "Test r/m32 AND imm32 (set flags, discard result)")


/* ════════════════════════════════════════════════════════════════════════════
 *  UNARY  (INC, DEC, NOT, NEG on r/m)
 *
 *  Group 4 (FE) = 8-bit INC/DEC, Group 5 (FF) = 16/32-bit INC/DEC
 *  Group 3 (F6) = 8-bit NOT/NEG,  (F7) = 16/32-bit NOT/NEG
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit ────────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_INC_RM8,      "inc",  0xFE, OPS_RM8,  OPN_ONE, SZ_8BIT,  MODRM_0, "Increment r/m8 by 1")
MNEM_RM(MNEM_DEC_RM8,      "dec",  0xFE, OPS_RM8,  OPN_ONE, SZ_8BIT,  MODRM_1, "Decrement r/m8 by 1")
MNEM_RM(MNEM_NOT_RM8,      "not",  0xF6, OPS_RM8,  OPN_ONE, SZ_8BIT,  MODRM_2, "Bitwise NOT r/m8 (one's complement)")
MNEM_RM(MNEM_NEG_RM8,      "neg",  0xF6, OPS_RM8,  OPN_ONE, SZ_8BIT,  MODRM_3, "Negate r/m8 (two's complement)")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_P(MNEM_INC_RM16,     "inc",  PF_66, 0xFF, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_0, "Increment r/m16 by 1")
MNEM_RM_P(MNEM_DEC_RM16,     "dec",  PF_66, 0xFF, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_1, "Decrement r/m16 by 1")
MNEM_RM_P(MNEM_NOT_RM16,     "not",  PF_66, 0xF7, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_2, "Bitwise NOT r/m16 (one's complement)")
MNEM_RM_P(MNEM_NEG_RM16,     "neg",  PF_66, 0xF7, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_3, "Negate r/m16 (two's complement)")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_INC_RM32,     "inc",  0xFF, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_0, "Increment r/m32 by 1")
MNEM_RM(MNEM_DEC_RM32,     "dec",  0xFF, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_1, "Decrement r/m32 by 1")
MNEM_RM(MNEM_NOT_RM32,     "not",  0xF7, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_2, "Bitwise NOT r/m32 (one's complement)")
MNEM_RM(MNEM_NEG_RM32,     "neg",  0xF7, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_3, "Negate r/m32 (two's complement)")


/* ════════════════════════════════════════════════════════════════════════════
 *  MUL / IMUL / DIV / IDIV  (single r/m operand form)
 *
 *  Group 3: F6 = 8-bit, F7 = 16/32-bit
 *  /4 = MUL, /5 = IMUL, /6 = DIV, /7 = IDIV
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit (result in AX) ─────────────────────────────────────────────────── */
MNEM_RM(MNEM_MUL_RM8,      "mul",  0xF6, OPS_RM8,  OPN_ONE, SZ_8BIT,  MODRM_4, "Unsigned multiply AL * r/m8 → AX")
MNEM_RM(MNEM_IMUL_RM8,     "imul", 0xF6, OPS_RM8,  OPN_ONE, SZ_8BIT,  MODRM_5, "Signed multiply AL * r/m8 → AX")
MNEM_RM(MNEM_DIV_RM8,      "div",  0xF6, OPS_RM8,  OPN_ONE, SZ_8BIT,  MODRM_6, "Unsigned divide AX / r/m8 → AL rem AH")
MNEM_RM(MNEM_IDIV_RM8,     "idiv", 0xF6, OPS_RM8,  OPN_ONE, SZ_8BIT,  MODRM_7, "Signed divide AX / r/m8 → AL rem AH")

/* ── 16-bit (result in DX:AX, 66h prefix) ─────────────────────────────────── */
MNEM_RM_P(MNEM_MUL_RM16,     "mul",  PF_66, 0xF7, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_4, "Unsigned multiply AX * r/m16 → DX:AX")
MNEM_RM_P(MNEM_IMUL_RM16,    "imul", PF_66, 0xF7, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_5, "Signed multiply AX * r/m16 → DX:AX")
MNEM_RM_P(MNEM_DIV_RM16,     "div",  PF_66, 0xF7, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_6, "Unsigned divide DX:AX / r/m16 → AX rem DX")
MNEM_RM_P(MNEM_IDIV_RM16,    "idiv", PF_66, 0xF7, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_7, "Signed divide DX:AX / r/m16 → AX rem DX")

/* ── 32-bit (result in EDX:EAX) ───────────────────────────────────────────── */
MNEM_RM(MNEM_MUL_RM32,     "mul",  0xF7, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_4, "Unsigned multiply EAX * r/m32 → EDX:EAX")
MNEM_RM(MNEM_IMUL_RM32,    "imul", 0xF7, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_5, "Signed multiply EAX * r/m32 → EDX:EAX")
MNEM_RM(MNEM_DIV_RM32,     "div",  0xF7, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_6, "Unsigned divide EDX:EAX / r/m32 → EAX rem EDX")
MNEM_RM(MNEM_IDIV_RM32,    "idiv", 0xF7, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_7, "Signed divide EDX:EAX / r/m32 → EAX rem EDX")


/* ════════════════════════════════════════════════════════════════════════════
 *  PUSH / POP / CALL / JMP  (indirect, r/m operand, no flags)
 *
 *  Group 5 (FF): /6 = PUSH, /2 = CALL near, /4 = JMP near
 *  POP (8F /0)
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_NF_P(MNEM_PUSH_RM16,   "push", PF_66, 0xFF, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_6, "Push r/m16 onto stack")
MNEM_RM_NF_P(MNEM_POP_RM16,    "pop",  PF_66, 0x8F, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_0, "Pop stack top into r/m16")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM_NF(MNEM_PUSH_RM32,     "push", 0xFF, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_6, "Push r/m32 onto stack")
MNEM_RM_NF(MNEM_POP_RM32,      "pop",  0x8F, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_0, "Pop stack top into r/m32")
/* Direct near/far calls and jumps MUST appear before indirect r/m forms so
 * that a bare label reference (OP_PTR satisfies OP_IMM) is matched first. */
MNEM_REL(MNEM_JMP_REL32_PRI,  "jmp",   0xE9, RL_REL32, "Near jump (rel32) - priority slot")
MNEM_REL(MNEM_JMP_REL8_PRI,   "jmp",   0xEB, RL_REL8,  "Short jump (rel8) - priority slot")
MNEM_REL(MNEM_CALL_REL32_PRI, "call",  0xE8, RL_REL32, "Near call (rel32) - priority slot")
/* Far jump / call  (opcode 0xEA ptr16:off16 / 0x9A ptr16:off16) */
MNEMONIC(MNEM_JMP_FAR16, "jmp",
         PF_NONE, OPCODE(0xEA,0x00), ENC_IMM, OPS_FAR16, OPN_ONE, SZ_NONE,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_ALL, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Far jump to ptr16:off16")
MNEMONIC(MNEM_CALL_FAR16, "call",
         PF_NONE, OPCODE(0x9A,0x00), ENC_IMM, OPS_FAR16, OPN_ONE, SZ_NONE,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_ALL, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Far call to ptr16:off16")
MNEM_RM_NF_P(MNEM_CALL_RM16,   "call", PF_66, 0xFF, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_2, "Call near procedure at r/m16")
MNEM_RM_NF(MNEM_CALL_RM32,     "call", 0xFF, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_2, "Call near procedure at r/m32")
MNEM_RM_NF_P(MNEM_JMP_RM16,    "jmp",  PF_66, 0xFF, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_4, "Jump to address in r/m16")
MNEM_RM_NF(MNEM_JMP_RM32,      "jmp",  0xFF, OPS_RM32, OPN_ONE, SZ_32BIT, MODRM_4, "Jump to address in r/m32")


/* ════════════════════════════════════════════════════════════════════════════
 *  LEA  (load effective address — no memory access, no flags)
 *  16-bit form uses 66h prefix for r16 destination.
 * ════════════════════════════════════════════════════════════════════════════ */
MNEMONIC(MNEM_LEA, "lea",
         PF_NONE, OPCODE(0x8D,0x00), ENC_MODRM, OPS_R32_RM32, OPN_TWO, SZ_32BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Load effective address of m into r32")
MNEMONIC(MNEM_LEA_R16, "lea",
         PF_66, OPCODE(0x8D,0x00), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Load effective address of m into r16")


/* ════════════════════════════════════════════════════════════════════════════
 *  XCHG  (exchange — no flags)
 *
 *  86 = r/m8,r8   87 = r/m16/32,r16/32
 *  (The +rd form for EAX is in REGISTER-IN-OPCODE section above.)
 * ════════════════════════════════════════════════════════════════════════════ */
MNEM_RM_NF(MNEM_XCHG_RM8_R8,    "xchg", 0x86, OPS_RM8_R8,   OPN_TWO, SZ_8BIT,  MODRM_NONE, "Exchange r/m8 with r8")
MNEM_RM_NF_P(MNEM_XCHG_RM16_R16, "xchg", PF_66, 0x87, OPS_RM16_R16, OPN_TWO, SZ_16BIT, MODRM_NONE, "Exchange r/m16 with r16")
MNEM_RM_NF(MNEM_XCHG_RM32_R32,  "xchg", 0x87, OPS_RM32_R32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Exchange r/m32 with r32")


/* ════════════════════════════════════════════════════════════════════════════
 *  SHIFT / ROTATE
 *
 *  Three sub-forms per operation:
 *    _1  = shift/rotate by 1     (D0 = 8-bit, D1 = 16/32-bit)
 *    _CL = shift/rotate by CL    (D2 = 8-bit, D3 = 16/32-bit)
 *    _I8 = shift/rotate by imm8  (C0 = 8-bit, C1 = 16/32-bit)
 *
 *  ModRM extension digits:
 *    /0 = ROL, /1 = ROR, /4 = SHL, /5 = SHR, /7 = SAR
 *
 *  All shift/rotate instructions modify flags (OSZAPC).
 * ════════════════════════════════════════════════════════════════════════════ */

/* ─── SHL (Shift Left / SAL) ─────────────────────────────────────────────── */

/* 8-bit */
MNEM_RM(MNEM_SHL_RM8_1,   "shl", 0xD0, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_4, "Shift r/m8 left by 1")
MNEM_RM(MNEM_SHL_RM8_CL,  "shl", 0xD2, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_4, "Shift r/m8 left by CL bits")
MNEM_RM(MNEM_SHL_RM8_I8,  "shl", 0xC0, OPS_RM8_IMM8,  OPN_TWO, SZ_8BIT,  MODRM_4, "Shift r/m8 left by imm8 bits")
/* 16-bit */
MNEM_RM_P(MNEM_SHL_RM16_1,  "shl", PF_66, 0xD1, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_4, "Shift r/m16 left by 1")
MNEM_RM_P(MNEM_SHL_RM16_CL, "shl", PF_66, 0xD3, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_4, "Shift r/m16 left by CL bits")
MNEM_RM_P(MNEM_SHL_RM16_I8, "shl", PF_66, 0xC1, OPS_RM16_IMM8, OPN_TWO, SZ_16BIT, MODRM_4, "Shift r/m16 left by imm8 bits")
/* 32-bit */
MNEM_RM(MNEM_SHL_RM32_1,  "shl", 0xD1, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_4, "Shift r/m32 left by 1")
MNEM_RM(MNEM_SHL_RM32_CL, "shl", 0xD3, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_4, "Shift r/m32 left by CL bits")
MNEM_RM(MNEM_SHL_RM32_I8, "shl", 0xC1, OPS_RM32_IMM8, OPN_TWO, SZ_32BIT, MODRM_4, "Shift r/m32 left by imm8 bits")

/* ─── SHR (Shift Right Logical) ──────────────────────────────────────────── */

/* 8-bit */
MNEM_RM(MNEM_SHR_RM8_1,   "shr", 0xD0, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_5, "Shift r/m8 right by 1 (unsigned)")
MNEM_RM(MNEM_SHR_RM8_CL,  "shr", 0xD2, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_5, "Shift r/m8 right by CL bits (unsigned)")
MNEM_RM(MNEM_SHR_RM8_I8,  "shr", 0xC0, OPS_RM8_IMM8,  OPN_TWO, SZ_8BIT,  MODRM_5, "Shift r/m8 right by imm8 bits (unsigned)")
/* 16-bit */
MNEM_RM_P(MNEM_SHR_RM16_1,  "shr", PF_66, 0xD1, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_5, "Shift r/m16 right by 1 (unsigned)")
MNEM_RM_P(MNEM_SHR_RM16_CL, "shr", PF_66, 0xD3, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_5, "Shift r/m16 right by CL bits (unsigned)")
MNEM_RM_P(MNEM_SHR_RM16_I8, "shr", PF_66, 0xC1, OPS_RM16_IMM8, OPN_TWO, SZ_16BIT, MODRM_5, "Shift r/m16 right by imm8 bits (unsigned)")
/* 32-bit */
MNEM_RM(MNEM_SHR_RM32_1,  "shr", 0xD1, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_5, "Shift r/m32 right by 1 (unsigned)")
MNEM_RM(MNEM_SHR_RM32_CL, "shr", 0xD3, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_5, "Shift r/m32 right by CL bits (unsigned)")
MNEM_RM(MNEM_SHR_RM32_I8, "shr", 0xC1, OPS_RM32_IMM8, OPN_TWO, SZ_32BIT, MODRM_5, "Shift r/m32 right by imm8 bits (unsigned)")

/* ─── SAR (Shift Right Arithmetic) ───────────────────────────────────────── */

/* 8-bit */
MNEM_RM(MNEM_SAR_RM8_1,   "sar", 0xD0, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_7, "Shift r/m8 right by 1 (signed, preserves sign)")
MNEM_RM(MNEM_SAR_RM8_CL,  "sar", 0xD2, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_7, "Shift r/m8 right by CL bits (signed)")
MNEM_RM(MNEM_SAR_RM8_I8,  "sar", 0xC0, OPS_RM8_IMM8,  OPN_TWO, SZ_8BIT,  MODRM_7, "Shift r/m8 right by imm8 bits (signed)")
/* 16-bit */
MNEM_RM_P(MNEM_SAR_RM16_1,  "sar", PF_66, 0xD1, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_7, "Shift r/m16 right by 1 (signed)")
MNEM_RM_P(MNEM_SAR_RM16_CL, "sar", PF_66, 0xD3, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_7, "Shift r/m16 right by CL bits (signed)")
MNEM_RM_P(MNEM_SAR_RM16_I8, "sar", PF_66, 0xC1, OPS_RM16_IMM8, OPN_TWO, SZ_16BIT, MODRM_7, "Shift r/m16 right by imm8 bits (signed)")
/* 32-bit */
MNEM_RM(MNEM_SAR_RM32_1,  "sar", 0xD1, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_7, "Shift r/m32 right by 1 (signed, preserves sign)")
MNEM_RM(MNEM_SAR_RM32_CL, "sar", 0xD3, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_7, "Shift r/m32 right by CL bits (signed)")
MNEM_RM(MNEM_SAR_RM32_I8, "sar", 0xC1, OPS_RM32_IMM8, OPN_TWO, SZ_32BIT, MODRM_7, "Shift r/m32 right by imm8 bits (signed)")

/* ─── ROL (Rotate Left) ──────────────────────────────────────────────────── */

/* 8-bit */
MNEM_RM(MNEM_ROL_RM8_1,   "rol", 0xD0, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_0, "Rotate r/m8 left by 1")
MNEM_RM(MNEM_ROL_RM8_CL,  "rol", 0xD2, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_0, "Rotate r/m8 left by CL bits")
MNEM_RM(MNEM_ROL_RM8_I8,  "rol", 0xC0, OPS_RM8_IMM8,  OPN_TWO, SZ_8BIT,  MODRM_0, "Rotate r/m8 left by imm8 bits")
/* 16-bit */
MNEM_RM_P(MNEM_ROL_RM16_1,  "rol", PF_66, 0xD1, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_0, "Rotate r/m16 left by 1")
MNEM_RM_P(MNEM_ROL_RM16_CL, "rol", PF_66, 0xD3, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_0, "Rotate r/m16 left by CL bits")
MNEM_RM_P(MNEM_ROL_RM16_I8, "rol", PF_66, 0xC1, OPS_RM16_IMM8, OPN_TWO, SZ_16BIT, MODRM_0, "Rotate r/m16 left by imm8 bits")
/* 32-bit */
MNEM_RM(MNEM_ROL_RM32_1,  "rol", 0xD1, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_0, "Rotate r/m32 left by 1")
MNEM_RM(MNEM_ROL_RM32_CL, "rol", 0xD3, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_0, "Rotate r/m32 left by CL bits")
MNEM_RM(MNEM_ROL_RM32_I8, "rol", 0xC1, OPS_RM32_IMM8, OPN_TWO, SZ_32BIT, MODRM_0, "Rotate r/m32 left by imm8 bits")

/* ─── ROR (Rotate Right) ─────────────────────────────────────────────────── */

/* 8-bit */
MNEM_RM(MNEM_ROR_RM8_1,   "ror", 0xD0, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_1, "Rotate r/m8 right by 1")
MNEM_RM(MNEM_ROR_RM8_CL,  "ror", 0xD2, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_1, "Rotate r/m8 right by CL bits")
MNEM_RM(MNEM_ROR_RM8_I8,  "ror", 0xC0, OPS_RM8_IMM8,  OPN_TWO, SZ_8BIT,  MODRM_1, "Rotate r/m8 right by imm8 bits")
/* 16-bit */
MNEM_RM_P(MNEM_ROR_RM16_1,  "ror", PF_66, 0xD1, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_1, "Rotate r/m16 right by 1")
MNEM_RM_P(MNEM_ROR_RM16_CL, "ror", PF_66, 0xD3, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_1, "Rotate r/m16 right by CL bits")
MNEM_RM_P(MNEM_ROR_RM16_I8, "ror", PF_66, 0xC1, OPS_RM16_IMM8, OPN_TWO, SZ_16BIT, MODRM_1, "Rotate r/m16 right by imm8 bits")
/* 32-bit */
MNEM_RM(MNEM_ROR_RM32_1,  "ror", 0xD1, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_1, "Rotate r/m32 right by 1")
MNEM_RM(MNEM_ROR_RM32_CL, "ror", 0xD3, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_1, "Rotate r/m32 right by CL bits")
MNEM_RM(MNEM_ROR_RM32_I8, "ror", 0xC1, OPS_RM32_IMM8, OPN_TWO, SZ_32BIT, MODRM_1, "Rotate r/m32 right by imm8 bits")


/* ════════════════════════════════════════════════════════════════════════════
 *  PC-RELATIVE BRANCHES
 *
 *  Short (rel8) and near (rel32) unconditional/conditional jumps.
 *  No size variants — operand is the displacement, not a register.
 * ════════════════════════════════════════════════════════════════════════════ */
/* NOTE: MNEM_JMP_REL32, MNEM_JMP_REL8, MNEM_CALL_REL32 are defined earlier
 * (as _PRI variants) before the indirect r/m forms so bare label references
 * match the relative encoding first.  These aliases remain for completeness. */
MNEM_REL(MNEM_JMP_REL32,  "jmp",   0xE9, RL_REL32, "Near jump (rel32)")
MNEM_REL(MNEM_JMP_REL8,   "jmp",   0xEB, RL_REL8,  "Short jump (rel8)")
MNEM_REL(MNEM_CALL_REL32, "call",  0xE8, RL_REL32, "Near call (rel32)")

/* Conditional jumps — short (rel8) */
MNEM_REL(MNEM_JO_REL8,    "jo",    0x70, RL_REL8,  "Jump short if overflow (OF=1)")
MNEM_REL(MNEM_JNO_REL8,   "jno",   0x71, RL_REL8,  "Jump short if not overflow (OF=0)")
MNEM_REL(MNEM_JB_REL8,    "jb",    0x72, RL_REL8,  "Jump short if below / carry (CF=1)")
MNEM_REL(MNEM_JNB_REL8,   "jnb",   0x73, RL_REL8,  "Jump short if not below / no carry (CF=0)")
MNEM_REL(MNEM_JZ_REL8,    "jz",    0x74, RL_REL8,  "Jump short if zero / equal (ZF=1)")
MNEM_REL(MNEM_JNZ_REL8,   "jnz",   0x75, RL_REL8,  "Jump short if not zero / not equal (ZF=0)")
MNEM_REL(MNEM_JBE_REL8,   "jbe",   0x76, RL_REL8,  "Jump short if below or equal (CF=1 or ZF=1)")
MNEM_REL(MNEM_JNBE_REL8,  "jnbe",  0x77, RL_REL8,  "Jump short if above (CF=0 and ZF=0)")
MNEM_REL(MNEM_JS_REL8,    "js",    0x78, RL_REL8,  "Jump short if sign (SF=1)")
MNEM_REL(MNEM_JNS_REL8,   "jns",   0x79, RL_REL8,  "Jump short if not sign (SF=0)")
MNEM_REL(MNEM_JP_REL8,    "jp",    0x7A, RL_REL8,  "Jump short if parity even (PF=1)")
MNEM_REL(MNEM_JNP_REL8,   "jnp",   0x7B, RL_REL8,  "Jump short if parity odd (PF=0)")
MNEM_REL(MNEM_JL_REL8,    "jl",    0x7C, RL_REL8,  "Jump short if less (SF!=OF)")
MNEM_REL(MNEM_JNL_REL8,   "jnl",   0x7D, RL_REL8,  "Jump short if not less / greater or equal (SF=OF)")
MNEM_REL(MNEM_JLE_REL8,   "jle",   0x7E, RL_REL8,  "Jump short if less or equal (ZF=1 or SF!=OF)")
MNEM_REL(MNEM_JNLE_REL8,  "jnle",  0x7F, RL_REL8,  "Jump short if greater (ZF=0 and SF=OF)")

/* Loop / JCXZ */
MNEM_REL(MNEM_LOOP,        "loop",   0xE2, RL_REL8, "Decrement ECX; jump short if ECX != 0")
MNEM_REL(MNEM_LOOPE,       "loope",  0xE1, RL_REL8, "Decrement ECX; jump short if ECX != 0 and ZF=1")
MNEM_REL(MNEM_LOOPNE,      "loopne", 0xE0, RL_REL8, "Decrement ECX; jump short if ECX != 0 and ZF=0")
MNEM_REL(MNEM_JCXZ,        "jcxz",   0xE3, RL_REL8, "Jump short if CX/ECX = 0")

/* ── Common Jcc aliases (short rel8) ──────────────────────────────────────── */
MNEM_REL(MNEM_JE_REL8,    "je",    0x74, RL_REL8,  "Jump short if equal (ZF=1)")
MNEM_REL(MNEM_JNE_REL8,   "jne",   0x75, RL_REL8,  "Jump short if not equal (ZF=0)")
MNEM_REL(MNEM_JC_REL8,    "jc",    0x72, RL_REL8,  "Jump short if carry (CF=1)")
MNEM_REL(MNEM_JNC_REL8,   "jnc",   0x73, RL_REL8,  "Jump short if not carry (CF=0)")
MNEM_REL(MNEM_JA_REL8,    "ja",    0x77, RL_REL8,  "Jump short if above (CF=0 and ZF=0)")
MNEM_REL(MNEM_JAE_REL8,   "jae",   0x73, RL_REL8,  "Jump short if above or equal (CF=0)")
MNEM_REL(MNEM_JG_REL8,    "jg",    0x7F, RL_REL8,  "Jump short if greater (ZF=0 and SF=OF)")
MNEM_REL(MNEM_JGE_REL8,   "jge",   0x7D, RL_REL8,  "Jump short if greater or equal (SF=OF)")
MNEM_REL(MNEM_JNA_REL8,   "jna",   0x76, RL_REL8,  "Jump short if not above (CF=1 or ZF=1)")
MNEM_REL(MNEM_JNAE_REL8,  "jnae",  0x72, RL_REL8,  "Jump short if not above or equal (CF=1)")
MNEM_REL(MNEM_JNG_REL8,   "jng",   0x7E, RL_REL8,  "Jump short if not greater (ZF=1 or SF!=OF)")
MNEM_REL(MNEM_JNGE_REL8,  "jnge",  0x7C, RL_REL8,  "Jump short if not greater or equal (SF!=OF)")
MNEM_REL(MNEM_JPE_REL8,   "jpe",   0x7A, RL_REL8,  "Jump short if parity even (PF=1)")
MNEM_REL(MNEM_JPO_REL8,   "jpo",   0x7B, RL_REL8,  "Jump short if parity odd (PF=0)")
MNEM_REL(MNEM_LOOPZ,      "loopz",  0xE1, RL_REL8, "Decrement ECX; jump short if ECX != 0 and ZF=1")
MNEM_REL(MNEM_LOOPNZ,     "loopnz", 0xE0, RL_REL8, "Decrement ECX; jump short if ECX != 0 and ZF=0")
MNEM_REL(MNEM_JECXZ,      "jecxz",  0xE3, RL_REL8, "Jump short if ECX = 0")


/* ════════════════════════════════════════════════════════════════════════════
 *  0F-PREFIXED — CONDITIONAL BRANCHES  (near, rel32)
 *  No size variants — the displacement is always 32-bit.
 * ════════════════════════════════════════════════════════════════════════════ */
MNEM_0F_REL(MNEM_JO_REL32,   "jo",   0x80, RL_REL32, "Jump near if overflow (OF=1)")
MNEM_0F_REL(MNEM_JNO_REL32,  "jno",  0x81, RL_REL32, "Jump near if not overflow (OF=0)")
MNEM_0F_REL(MNEM_JB_REL32,   "jb",   0x82, RL_REL32, "Jump near if below / carry (CF=1)")
MNEM_0F_REL(MNEM_JNB_REL32,  "jnb",  0x83, RL_REL32, "Jump near if not below / no carry (CF=0)")
MNEM_0F_REL(MNEM_JZ_REL32,   "jz",   0x84, RL_REL32, "Jump near if zero / equal (ZF=1)")
MNEM_0F_REL(MNEM_JNZ_REL32,  "jnz",  0x85, RL_REL32, "Jump near if not zero / not equal (ZF=0)")
MNEM_0F_REL(MNEM_JBE_REL32,  "jbe",  0x86, RL_REL32, "Jump near if below or equal (CF=1 or ZF=1)")
MNEM_0F_REL(MNEM_JNBE_REL32, "jnbe", 0x87, RL_REL32, "Jump near if above (CF=0 and ZF=0)")
MNEM_0F_REL(MNEM_JS_REL32,   "js",   0x88, RL_REL32, "Jump near if sign (SF=1)")
MNEM_0F_REL(MNEM_JNS_REL32,  "jns",  0x89, RL_REL32, "Jump near if not sign (SF=0)")
MNEM_0F_REL(MNEM_JP_REL32,   "jp",   0x8A, RL_REL32, "Jump near if parity even (PF=1)")
MNEM_0F_REL(MNEM_JNP_REL32,  "jnp",  0x8B, RL_REL32, "Jump near if parity odd (PF=0)")
MNEM_0F_REL(MNEM_JL_REL32,   "jl",   0x8C, RL_REL32, "Jump near if less (SF!=OF)")
MNEM_0F_REL(MNEM_JNL_REL32,  "jnl",  0x8D, RL_REL32, "Jump near if not less / greater or equal (SF=OF)")
MNEM_0F_REL(MNEM_JLE_REL32,  "jle",  0x8E, RL_REL32, "Jump near if less or equal (ZF=1 or SF!=OF)")
MNEM_0F_REL(MNEM_JNLE_REL32, "jnle", 0x8F, RL_REL32, "Jump near if greater (ZF=0 and SF=OF)")

/* ── Common Jcc aliases (near rel32) ──────────────────────────────────────── */
MNEM_0F_REL(MNEM_JE_REL32,   "je",   0x84, RL_REL32, "Jump near if equal (ZF=1)")
MNEM_0F_REL(MNEM_JNE_REL32,  "jne",  0x85, RL_REL32, "Jump near if not equal (ZF=0)")
MNEM_0F_REL(MNEM_JC_REL32,   "jc",   0x82, RL_REL32, "Jump near if carry (CF=1)")
MNEM_0F_REL(MNEM_JNC_REL32,  "jnc",  0x83, RL_REL32, "Jump near if not carry (CF=0)")
MNEM_0F_REL(MNEM_JA_REL32,   "ja",   0x87, RL_REL32, "Jump near if above (CF=0 and ZF=0)")
MNEM_0F_REL(MNEM_JAE_REL32,  "jae",  0x83, RL_REL32, "Jump near if above or equal (CF=0)")
MNEM_0F_REL(MNEM_JG_REL32,   "jg",   0x8F, RL_REL32, "Jump near if greater (ZF=0 and SF=OF)")
MNEM_0F_REL(MNEM_JGE_REL32,  "jge",  0x8D, RL_REL32, "Jump near if greater or equal (SF=OF)")
MNEM_0F_REL(MNEM_JNA_REL32,  "jna",  0x86, RL_REL32, "Jump near if not above (CF=1 or ZF=1)")
MNEM_0F_REL(MNEM_JNAE_REL32, "jnae", 0x82, RL_REL32, "Jump near if not above or equal (CF=1)")
MNEM_0F_REL(MNEM_JNG_REL32,  "jng",  0x8E, RL_REL32, "Jump near if not greater (ZF=1 or SF!=OF)")
MNEM_0F_REL(MNEM_JNGE_REL32, "jnge", 0x8C, RL_REL32, "Jump near if not greater or equal (SF!=OF)")
MNEM_0F_REL(MNEM_JPE_REL32,  "jpe",  0x8A, RL_REL32, "Jump near if parity even (PF=1)")
MNEM_0F_REL(MNEM_JPO_REL32,  "jpo",  0x8B, RL_REL32, "Jump near if parity odd (PF=0)")


/* ════════════════════════════════════════════════════════════════════════════
 *  0F-PREFIXED — MOVZX / MOVSX  (zero/sign extend, no flags)
 *
 *  Source operand is 8-bit or 16-bit; destination is 32-bit (or 16-bit
 *  with 66h prefix for the 8-bit source forms).
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 32-bit destination ───────────────────────────────────────────────────── */
MNEM_0F(MNEM_MOVZX_R32_RM8,  "movzx", 0xB6, OPS_R32_RM32, OPN_TWO, SZ_8BIT,  MODRM_NONE, "Zero-extend r/m8 to r32")
MNEM_0F(MNEM_MOVZX_R32_RM16, "movzx", 0xB7, OPS_R32_RM32, OPN_TWO, SZ_16BIT, MODRM_NONE, "Zero-extend r/m16 to r32")
MNEM_0F(MNEM_MOVSX_R32_RM8,  "movsx", 0xBE, OPS_R32_RM32, OPN_TWO, SZ_8BIT,  MODRM_NONE, "Sign-extend r/m8 to r32")
MNEM_0F(MNEM_MOVSX_R32_RM16, "movsx", 0xBF, OPS_R32_RM32, OPN_TWO, SZ_16BIT, MODRM_NONE, "Sign-extend r/m16 to r32")

/* ── 16-bit destination (66h prefix, 8-bit source only) ───────────────────── */
MNEMONIC(MNEM_MOVZX_R16_RM8, "movzx",
         PF_66, OPCODE(0x0F,0xB6), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_8BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Zero-extend r/m8 to r16")
MNEMONIC(MNEM_MOVSX_R16_RM8, "movsx",
         PF_66, OPCODE(0x0F,0xBE), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_8BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Sign-extend r/m8 to r16")


/* ════════════════════════════════════════════════════════════════════════════
 *  0F-PREFIXED — BIT SCAN  (BSF / BSR)
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEMONIC(MNEM_BSF_R16, "bsf",
         PF_66, OPCODE(0x0F,0xBC), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Bit scan forward r/m16 → r16 (find lowest set bit)")
MNEMONIC(MNEM_BSR_R16, "bsr",
         PF_66, OPCODE(0x0F,0xBD), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Bit scan reverse r/m16 → r16 (find highest set bit)")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_0F(MNEM_BSF, "bsf", 0xBC, OPS_R32_RM32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Bit scan forward r/m32 → r32 (find lowest set bit)")
MNEM_0F(MNEM_BSR, "bsr", 0xBD, OPS_R32_RM32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Bit scan reverse r/m32 → r32 (find highest set bit)")


/* ════════════════════════════════════════════════════════════════════════════
 *  0F-PREFIXED — SYSTEM / MISC
 *  These are fixed-size system instructions with no 8/16/32-bit variants.
 * ════════════════════════════════════════════════════════════════════════════ */

/* CPUID / RDTSC — no ModRM, no operands */
MNEMONIC(MNEM_CPUID, "cpuid",
         PF_NONE, OPCODE(0x0F,0xA2), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_NONE,
         REG_NONE, MODRM_NONE, FALSE, PROC_PENTIUM, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "CPU identification (EAX selects leaf)")
MNEMONIC(MNEM_RDTSC, "rdtsc",
         PF_NONE, OPCODE(0x0F,0x31), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_NONE,
         REG_NONE, MODRM_NONE, FALSE, PROC_PENTIUM, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Read time-stamp counter into EDX:EAX")

/* LGDT / LIDT / SGDT / SIDT — ModRM /digit, memory operand */
MNEM_0F(MNEM_SGDT, "sgdt", 0x01, OPS_RM32, OPN_ONE, SZ_NONE, MODRM_0, "Store GDT register to memory")
MNEM_0F(MNEM_SIDT, "sidt", 0x01, OPS_RM32, OPN_ONE, SZ_NONE, MODRM_1, "Store IDT register to memory")
MNEM_0F(MNEM_LGDT, "lgdt", 0x01, OPS_RM32, OPN_ONE, SZ_NONE, MODRM_2, "Load GDT register from memory")
MNEM_0F(MNEM_LIDT, "lidt", 0x01, OPS_RM32, OPN_ONE, SZ_NONE, MODRM_3, "Load IDT register from memory")


/* ════════════════════════════════════════════════════════════════════════════
 *  0F-PREFIXED — SETcc  (set byte on condition)
 *
 *  Always 8-bit: writes 0 or 1 to an r/m8 operand. No other size variants.
 *  Opcode range: 0F 90..9F
 * ════════════════════════════════════════════════════════════════════════════ */
MNEM_0F(MNEM_SETO,   "seto",   0x90, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if overflow (OF=1)")
MNEM_0F(MNEM_SETNO,  "setno",  0x91, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if not overflow (OF=0)")
MNEM_0F(MNEM_SETB,   "setb",   0x92, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if below / carry (CF=1)")
MNEM_0F(MNEM_SETNB,  "setnb",  0x93, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if not below / no carry (CF=0)")
MNEM_0F(MNEM_SETZ,   "setz",   0x94, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if zero / equal (ZF=1)")
MNEM_0F(MNEM_SETNZ,  "setnz",  0x95, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if not zero / not equal (ZF=0)")
MNEM_0F(MNEM_SETBE,  "setbe",  0x96, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if below or equal (CF=1 or ZF=1)")
MNEM_0F(MNEM_SETNBE, "setnbe", 0x97, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if above (CF=0 and ZF=0)")
MNEM_0F(MNEM_SETS,   "sets",   0x98, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if sign (SF=1)")
MNEM_0F(MNEM_SETNS,  "setns",  0x99, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if not sign (SF=0)")
MNEM_0F(MNEM_SETP,   "setp",   0x9A, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if parity even (PF=1)")
MNEM_0F(MNEM_SETNP,  "setnp",  0x9B, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if parity odd (PF=0)")
MNEM_0F(MNEM_SETL,   "setl",   0x9C, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if less (SF!=OF)")
MNEM_0F(MNEM_SETNL,  "setnl",  0x9D, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if not less / greater or equal (SF=OF)")
MNEM_0F(MNEM_SETLE,  "setle",  0x9E, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if less or equal (ZF=1 or SF!=OF)")
MNEM_0F(MNEM_SETNLE, "setnle", 0x9F, OPS_RM8, OPN_ONE, SZ_8BIT, MODRM_0, "Set byte if greater (ZF=0 and SF=OF)")


/* ════════════════════════════════════════════════════════════════════════════
 *  ADC  (Add with carry — sets OSZAPC flags)
 *
 *  Opcodes: 10/11 = r→r/m, 12/13 = r/m→r, 80 /2 = imm8→r/m8,
 *           81 /2 = imm16/32→r/m, 83 /2 = sign-extended imm8→r/m16/32
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit ────────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_ADC_RM8_R8,      "adc", 0x10, OPS_RM8_R8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Add r8 + CF to r/m8")
MNEM_RM(MNEM_ADC_R8_RM8,      "adc", 0x12, OPS_R8_RM8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Add r/m8 + CF to r8")
MNEM_RM(MNEM_ADC_RM8_IMM8,    "adc", 0x80, OPS_RM8_IMM8,   OPN_TWO, SZ_8BIT,  MODRM_2,    "Add imm8 + CF to r/m8")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_P(MNEM_ADC_RM16_R16,    "adc", PF_66, 0x11, OPS_RM16_R16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Add r16 + CF to r/m16")
MNEM_RM_P(MNEM_ADC_R16_RM16,    "adc", PF_66, 0x13, OPS_R16_RM16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Add r/m16 + CF to r16")
MNEM_RM_P(MNEM_ADC_RM16_IMM8,   "adc", PF_66, 0x83, OPS_RM16_IMM8,  OPN_TWO, SZ_16BIT, MODRM_2,    "Add sign-extended imm8 + CF to r/m16")
MNEM_RM_P(MNEM_ADC_RM16_IMM16,  "adc", PF_66, 0x81, OPS_RM16_IMM16, OPN_TWO, SZ_16BIT, MODRM_2,    "Add imm16 + CF to r/m16")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_ADC_RM32_R32,    "adc", 0x11, OPS_RM32_R32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Add r32 + CF to r/m32")
MNEM_RM(MNEM_ADC_R32_RM32,    "adc", 0x13, OPS_R32_RM32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Add r/m32 + CF to r32")
MNEM_RM(MNEM_ADC_RM32_IMM8,   "adc", 0x83, OPS_RM32_IMM8,  OPN_TWO, SZ_32BIT, MODRM_2,    "Add sign-extended imm8 + CF to r/m32")
MNEM_RM(MNEM_ADC_RM32_IMM32,  "adc", 0x81, OPS_RM32_IMM32, OPN_TWO, SZ_32BIT, MODRM_2,    "Add imm32 + CF to r/m32")


/* ════════════════════════════════════════════════════════════════════════════
 *  SBB  (Subtract with borrow — sets OSZAPC flags)
 *
 *  Opcodes: 18/19 = r→r/m, 1A/1B = r/m→r, 80 /3 = imm8→r/m8,
 *           81 /3 = imm16/32→r/m, 83 /3 = sign-extended imm8→r/m16/32
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── 8-bit ────────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_SBB_RM8_R8,      "sbb", 0x18, OPS_RM8_R8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Subtract r8 + CF from r/m8")
MNEM_RM(MNEM_SBB_R8_RM8,      "sbb", 0x1A, OPS_R8_RM8,     OPN_TWO, SZ_8BIT,  MODRM_NONE, "Subtract r/m8 + CF from r8")
MNEM_RM(MNEM_SBB_RM8_IMM8,    "sbb", 0x80, OPS_RM8_IMM8,   OPN_TWO, SZ_8BIT,  MODRM_3,    "Subtract imm8 + CF from r/m8")

/* ── 16-bit (66h prefix) ──────────────────────────────────────────────────── */
MNEM_RM_P(MNEM_SBB_RM16_R16,    "sbb", PF_66, 0x19, OPS_RM16_R16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Subtract r16 + CF from r/m16")
MNEM_RM_P(MNEM_SBB_R16_RM16,    "sbb", PF_66, 0x1B, OPS_R16_RM16,   OPN_TWO, SZ_16BIT, MODRM_NONE, "Subtract r/m16 + CF from r16")
MNEM_RM_P(MNEM_SBB_RM16_IMM8,   "sbb", PF_66, 0x83, OPS_RM16_IMM8,  OPN_TWO, SZ_16BIT, MODRM_3,    "Subtract sign-extended imm8 + CF from r/m16")
MNEM_RM_P(MNEM_SBB_RM16_IMM16,  "sbb", PF_66, 0x81, OPS_RM16_IMM16, OPN_TWO, SZ_16BIT, MODRM_3,    "Subtract imm16 + CF from r/m16")

/* ── 32-bit ───────────────────────────────────────────────────────────────── */
MNEM_RM(MNEM_SBB_RM32_R32,    "sbb", 0x19, OPS_RM32_R32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Subtract r32 + CF from r/m32")
MNEM_RM(MNEM_SBB_R32_RM32,    "sbb", 0x1B, OPS_R32_RM32,   OPN_TWO, SZ_32BIT, MODRM_NONE, "Subtract r/m32 + CF from r32")
MNEM_RM(MNEM_SBB_RM32_IMM8,   "sbb", 0x83, OPS_RM32_IMM8,  OPN_TWO, SZ_32BIT, MODRM_3,    "Subtract sign-extended imm8 + CF from r/m32")
MNEM_RM(MNEM_SBB_RM32_IMM32,  "sbb", 0x81, OPS_RM32_IMM32, OPN_TWO, SZ_32BIT, MODRM_3,    "Subtract imm32 + CF from r/m32")


/* ════════════════════════════════════════════════════════════════════════════
 *  RCL / RCR  (Rotate through Carry — modifies OF, CF)
 *
 *  Same opcode pattern as ROL/ROR:
 *    D0/D2/C0 = 8-bit, D1/D3/C1 = 16/32-bit
 *  ModRM extension: /2 = RCL, /3 = RCR
 * ════════════════════════════════════════════════════════════════════════════ */

/* ─── RCL (Rotate Left through Carry) ────────────────────────────────────── */

/* 8-bit */
MNEM_RM(MNEM_RCL_RM8_1,   "rcl", 0xD0, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_2, "Rotate r/m8 left through carry by 1")
MNEM_RM(MNEM_RCL_RM8_CL,  "rcl", 0xD2, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_2, "Rotate r/m8 left through carry by CL bits")
MNEM_RM(MNEM_RCL_RM8_I8,  "rcl", 0xC0, OPS_RM8_IMM8,  OPN_TWO, SZ_8BIT,  MODRM_2, "Rotate r/m8 left through carry by imm8 bits")
/* 16-bit */
MNEM_RM_P(MNEM_RCL_RM16_1,  "rcl", PF_66, 0xD1, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_2, "Rotate r/m16 left through carry by 1")
MNEM_RM_P(MNEM_RCL_RM16_CL, "rcl", PF_66, 0xD3, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_2, "Rotate r/m16 left through carry by CL bits")
MNEM_RM_P(MNEM_RCL_RM16_I8, "rcl", PF_66, 0xC1, OPS_RM16_IMM8, OPN_TWO, SZ_16BIT, MODRM_2, "Rotate r/m16 left through carry by imm8 bits")
/* 32-bit */
MNEM_RM(MNEM_RCL_RM32_1,  "rcl", 0xD1, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_2, "Rotate r/m32 left through carry by 1")
MNEM_RM(MNEM_RCL_RM32_CL, "rcl", 0xD3, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_2, "Rotate r/m32 left through carry by CL bits")
MNEM_RM(MNEM_RCL_RM32_I8, "rcl", 0xC1, OPS_RM32_IMM8, OPN_TWO, SZ_32BIT, MODRM_2, "Rotate r/m32 left through carry by imm8 bits")

/* ─── RCR (Rotate Right through Carry) ───────────────────────────────────── */

/* 8-bit */
MNEM_RM(MNEM_RCR_RM8_1,   "rcr", 0xD0, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_3, "Rotate r/m8 right through carry by 1")
MNEM_RM(MNEM_RCR_RM8_CL,  "rcr", 0xD2, OPS_RM8,       OPN_ONE, SZ_8BIT,  MODRM_3, "Rotate r/m8 right through carry by CL bits")
MNEM_RM(MNEM_RCR_RM8_I8,  "rcr", 0xC0, OPS_RM8_IMM8,  OPN_TWO, SZ_8BIT,  MODRM_3, "Rotate r/m8 right through carry by imm8 bits")
/* 16-bit */
MNEM_RM_P(MNEM_RCR_RM16_1,  "rcr", PF_66, 0xD1, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_3, "Rotate r/m16 right through carry by 1")
MNEM_RM_P(MNEM_RCR_RM16_CL, "rcr", PF_66, 0xD3, OPS_RM16,      OPN_ONE, SZ_16BIT, MODRM_3, "Rotate r/m16 right through carry by CL bits")
MNEM_RM_P(MNEM_RCR_RM16_I8, "rcr", PF_66, 0xC1, OPS_RM16_IMM8, OPN_TWO, SZ_16BIT, MODRM_3, "Rotate r/m16 right through carry by imm8 bits")
/* 32-bit */
MNEM_RM(MNEM_RCR_RM32_1,  "rcr", 0xD1, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_3, "Rotate r/m32 right through carry by 1")
MNEM_RM(MNEM_RCR_RM32_CL, "rcr", 0xD3, OPS_RM32,      OPN_ONE, SZ_32BIT, MODRM_3, "Rotate r/m32 right through carry by CL bits")
MNEM_RM(MNEM_RCR_RM32_I8, "rcr", 0xC1, OPS_RM32_IMM8, OPN_TWO, SZ_32BIT, MODRM_3, "Rotate r/m32 right through carry by imm8 bits")


/* ════════════════════════════════════════════════════════════════════════════
 *  STRING INSTRUCTIONS  (no explicit operands — direction flag controls)
 *
 *  Byte forms: single opcode.  Word forms: 66h prefix.  Dword forms: default.
 *  MOVS/STOS/LODS/INS/OUTS: no flags.  CMPS/SCAS: set OSZAPC.
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── MOVS (Move String) ──────────────────────────────────────────────────── */
MNEM_NOOPS(MNEM_MOVSB,  "movsb",  0xA4, "Move byte [DS:ESI] to [ES:EDI], update ESI/EDI")
MNEM_NOOPS(MNEM_MOVSD_S,"movsd",  0xA5, "Move dword [DS:ESI] to [ES:EDI], update ESI/EDI")
MNEMONIC(MNEM_MOVSW, "movsw",
         PF_66, OPCODE(0xA5,0x00), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Move word [DS:ESI] to [ES:EDI], update ESI/EDI")

/* ── CMPS (Compare String) ───────────────────────────────────────────────── */
MNEM_NOOPS(MNEM_CMPSB,  "cmpsb",  0xA6, "Compare byte [DS:ESI] with [ES:EDI], set flags")
MNEM_NOOPS(MNEM_CMPSD_S,"cmpsd",  0xA7, "Compare dword [DS:ESI] with [ES:EDI], set flags")
MNEMONIC(MNEM_CMPSW, "cmpsw",
         PF_66, OPCODE(0xA7,0x00), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Compare word [DS:ESI] with [ES:EDI], set flags")

/* ── STOS (Store String) ─────────────────────────────────────────────────── */
MNEM_NOOPS(MNEM_STOSB,  "stosb",  0xAA, "Store AL to [ES:EDI], update EDI")
MNEM_NOOPS(MNEM_STOSD,  "stosd",  0xAB, "Store EAX to [ES:EDI], update EDI")
MNEMONIC(MNEM_STOSW, "stosw",
         PF_66, OPCODE(0xAB,0x00), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_WRITE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Store AX to [ES:EDI], update EDI")

/* ── LODS (Load String) ──────────────────────────────────────────────────── */
MNEM_NOOPS(MNEM_LODSB,  "lodsb",  0xAC, "Load byte [DS:ESI] into AL, update ESI")
MNEM_NOOPS(MNEM_LODSD,  "lodsd",  0xAD, "Load dword [DS:ESI] into EAX, update ESI")
MNEMONIC(MNEM_LODSW, "lodsw",
         PF_66, OPCODE(0xAD,0x00), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Load word [DS:ESI] into AX, update ESI")

/* ── SCAS (Scan String) ──────────────────────────────────────────────────── */
MNEM_NOOPS(MNEM_SCASB,  "scasb",  0xAE, "Compare AL with [ES:EDI], set flags, update EDI")
MNEM_NOOPS(MNEM_SCASD,  "scasd",  0xAF, "Compare EAX with [ES:EDI], set flags, update EDI")
MNEMONIC(MNEM_SCASW, "scasw",
         PF_66, OPCODE(0xAF,0x00), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Compare AX with [ES:EDI], set flags, update EDI")

/* ── INS (Input String from Port) ────────────────────────────────────────── */
MNEM_NOOPS(MNEM_INSB,   "insb",   0x6C, "Input byte from port DX to [ES:EDI], update EDI")
MNEM_NOOPS(MNEM_INSD,   "insd",   0x6D, "Input dword from port DX to [ES:EDI], update EDI")
MNEMONIC(MNEM_INSW, "insw",
         PF_66, OPCODE(0x6D,0x00), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_80186, ST_DOCUMENTED, MODE_ANY,
         MEM_WRITE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Input word from port DX to [ES:EDI], update EDI")

/* ── OUTS (Output String to Port) ────────────────────────────────────────── */
MNEM_NOOPS(MNEM_OUTSB,  "outsb",  0x6E, "Output byte [DS:ESI] to port DX, update ESI")
MNEM_NOOPS(MNEM_OUTSD,  "outsd",  0x6F, "Output dword [DS:ESI] to port DX, update ESI")
MNEMONIC(MNEM_OUTSW, "outsw",
         PF_66, OPCODE(0x6F,0x00), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_80186, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Output word [DS:ESI] to port DX, update ESI")


/* ════════════════════════════════════════════════════════════════════════════
 *  PREFIX INSTRUCTIONS  (REP, REPE, REPNE, LOCK)
 *
 *  These emit a single prefix byte.  Used before string instructions.
 * ════════════════════════════════════════════════════════════════════════════ */
MNEM_NOOPS(MNEM_REP,    "rep",    0xF3, "Repeat string operation CX/ECX times")
MNEM_NOOPS(MNEM_REPE,   "repe",   0xF3, "Repeat string operation while equal (ZF=1)")
MNEM_NOOPS(MNEM_REPNE,  "repne",  0xF2, "Repeat string operation while not equal (ZF=0)")
MNEM_NOOPS(MNEM_REPZ,   "repz",   0xF3, "Repeat string operation while zero (ZF=1)")
MNEM_NOOPS(MNEM_REPNZ,  "repnz",  0xF2, "Repeat string operation while not zero (ZF=0)")
MNEM_NOOPS(MNEM_LOCK,   "lock",   0xF0, "Assert LOCK# signal for next instruction")


/* ════════════════════════════════════════════════════════════════════════════
 *  IN / OUT  (Port I/O — no flags)
 *
 *  IN:  E4/E5 = AL/EAX,imm8   EC/ED = AL/EAX,DX
 *  OUT: E6/E7 = imm8,AL/EAX   EE/EF = DX,AL/EAX
 *  16-bit forms use 66h prefix on E5/ED/E7/EF.
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── IN from immediate port ───────────────────────────────────────────────── */
MNEM_IMM(MNEM_IN_AL_IMM8,   "in",  0xE4, OPS_IMM8, OPN_ONE, SZ_8BIT,  "Input byte from port imm8 into AL")
MNEM_IMM(MNEM_IN_EAX_IMM8,  "in",  0xE5, OPS_IMM8, OPN_ONE, SZ_32BIT, "Input dword from port imm8 into EAX")
MNEMONIC(MNEM_IN_AX_IMM8, "in",
         PF_66, OPCODE(0xE5,0x00), ENC_IMM, OPS_IMM8, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Input word from port imm8 into AX")

/* ── IN from DX port ──────────────────────────────────────────────────────── */
MNEM_NOOPS(MNEM_IN_AL_DX,   "in",  0xEC, "Input byte from port DX into AL")
MNEM_NOOPS(MNEM_IN_EAX_DX,  "in",  0xED, "Input dword from port DX into EAX")
MNEMONIC(MNEM_IN_AX_DX, "in",
         PF_66, OPCODE(0xED,0x00), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Input word from port DX into AX")

/* ── OUT to immediate port ────────────────────────────────────────────────── */
MNEM_IMM(MNEM_OUT_IMM8_AL,   "out", 0xE6, OPS_IMM8, OPN_ONE, SZ_8BIT,  "Output AL to port imm8")
MNEM_IMM(MNEM_OUT_IMM8_EAX,  "out", 0xE7, OPS_IMM8, OPN_ONE, SZ_32BIT, "Output EAX to port imm8")
MNEMONIC(MNEM_OUT_IMM8_AX, "out",
         PF_66, OPCODE(0xE7,0x00), ENC_IMM, OPS_IMM8, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Output AX to port imm8")

/* ── OUT to DX port ───────────────────────────────────────────────────────── */
MNEM_NOOPS(MNEM_OUT_DX_AL,   "out", 0xEE, "Output AL to port DX")
MNEM_NOOPS(MNEM_OUT_DX_EAX,  "out", 0xEF, "Output EAX to port DX")
MNEMONIC(MNEM_OUT_DX_AX, "out",
         PF_66, OPCODE(0xEF,0x00), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Output AX to port DX")


/* ════════════════════════════════════════════════════════════════════════════
 *  MOV — Segment register forms  (no flags)
 *
 *  8C /r = MOV r/m16, Sreg     8E /r = MOV Sreg, r/m16
 * ════════════════════════════════════════════════════════════════════════════ */
MNEMONIC(MNEM_MOV_RM16_SREG, "mov",
         PF_NONE, OPCODE(0x8C,0x00), ENC_MODRM, OPS_RM16_SEG, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Move segment register to r/m16")
MNEMONIC(MNEM_MOV_SREG_RM16, "mov",
         PF_NONE, OPCODE(0x8E,0x00), ENC_MODRM, OPS_SEG_RM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Move r/m16 to segment register")


/* ════════════════════════════════════════════════════════════════════════════
 *  PUSH / POP — Segment registers  (no flags)
 *
 *  8086: ES(06/07), CS(0E), SS(16/17), DS(1E/1F)
 *  80386: FS(0F A0/A1), GS(0F A8/A9)
 * ════════════════════════════════════════════════════════════════════════════ */
MNEMONIC(MNEM_PUSH_ES, "push",
         PF_NONE, OPCODE(0x06,0x00), ENC_DIRECT, OPS_SEG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Push ES onto stack")
MNEMONIC(MNEM_POP_ES, "pop",
         PF_NONE, OPCODE(0x07,0x00), ENC_DIRECT, OPS_SEG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Pop ES from stack")
MNEMONIC(MNEM_PUSH_CS, "push",
         PF_NONE, OPCODE(0x0E,0x00), ENC_DIRECT, OPS_SEG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Push CS onto stack")
MNEMONIC(MNEM_PUSH_SS, "push",
         PF_NONE, OPCODE(0x16,0x00), ENC_DIRECT, OPS_SEG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Push SS onto stack")
MNEMONIC(MNEM_POP_SS, "pop",
         PF_NONE, OPCODE(0x17,0x00), ENC_DIRECT, OPS_SEG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Pop SS from stack")
MNEMONIC(MNEM_PUSH_DS, "push",
         PF_NONE, OPCODE(0x1E,0x00), ENC_DIRECT, OPS_SEG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Push DS onto stack")
MNEMONIC(MNEM_POP_DS, "pop",
         PF_NONE, OPCODE(0x1F,0x00), ENC_DIRECT, OPS_SEG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Pop DS from stack")

/* ── FS / GS (80386) ──────────────────────────────────────────────────────── */
MNEMONIC(MNEM_PUSH_FS, "push",
         PF_NONE, OPCODE(0x0F,0xA0), ENC_DIRECT, OPS_SEG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Push FS onto stack")
MNEMONIC(MNEM_POP_FS, "pop",
         PF_NONE, OPCODE(0x0F,0xA1), ENC_DIRECT, OPS_SEG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Pop FS from stack")
MNEMONIC(MNEM_PUSH_GS, "push",
         PF_NONE, OPCODE(0x0F,0xA8), ENC_DIRECT, OPS_SEG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Push GS onto stack")
MNEMONIC(MNEM_POP_GS, "pop",
         PF_NONE, OPCODE(0x0F,0xA9), ENC_DIRECT, OPS_SEG, OPN_ONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Pop GS from stack")


/* ════════════════════════════════════════════════════════════════════════════
 *  MISCELLANEOUS  (CWD, ENTER, BOUND, PUSHAD/POPAD/PUSHFD/POPFD)
 * ════════════════════════════════════════════════════════════════════════════ */

/* CWD: sign-extend AX into DX:AX (16-bit form of CDQ) */
MNEMONIC(MNEM_CWD, "cwd",
         PF_66, OPCODE(0x99,0x00), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_ANY, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Sign-extend AX into DX:AX (16-bit)")

/* ENTER imm16, imm8: create procedure stack frame (80186+) */
MNEMONIC(MNEM_ENTER, "enter",
         PF_NONE, OPCODE(0xC8,0x00), ENC_IMM,
         OPS_IMM_IMM, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, FALSE, PROC_80186, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Create stack frame (imm16=frame size, imm8=nesting level)")

/* BOUND: check array index against bounds (80186+) */
MNEMONIC(MNEM_BOUND_R32, "bound",
         PF_NONE, OPCODE(0x62,0x00), ENC_MODRM, OPS_R32_RM32, OPN_TWO, SZ_32BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80186, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Check r32 against array bounds in memory")
MNEMONIC(MNEM_BOUND_R16, "bound",
         PF_66, OPCODE(0x62,0x00), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80186, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Check r16 against array bounds in memory")

/* 32-bit aliases (same opcodes as PUSHA/POPA/PUSHF/POPF in 32-bit mode) */
MNEM_NOOPS(MNEM_PUSHAD, "pushad", 0x60, "Push all 32-bit general-purpose registers")
MNEM_NOOPS(MNEM_POPAD,  "popad",  0x61, "Pop all 32-bit general-purpose registers")
MNEM_NOOPS(MNEM_PUSHFD, "pushfd", 0x9C, "Push 32-bit EFLAGS onto stack")
MNEM_NOOPS(MNEM_POPFD,  "popfd",  0x9D, "Pop 32-bit EFLAGS from stack")


/* ════════════════════════════════════════════════════════════════════════════
 *  LDS / LES  (Load far pointer — 8086, no flags)
 *
 *  LDS r, m16:16/32  →  C5 /r     LES r, m16:16/32  →  C4 /r
 * ════════════════════════════════════════════════════════════════════════════ */
MNEM_RM_NF(MNEM_LDS_R32, "lds", 0xC5, OPS_R32_RM32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Load far pointer into r32 and DS")
MNEM_RM_NF(MNEM_LES_R32, "les", 0xC4, OPS_R32_RM32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Load far pointer into r32 and ES")
MNEM_RM_NF_P(MNEM_LDS_R16, "lds", PF_66, 0xC5, OPS_R16_RM16, OPN_TWO, SZ_16BIT, MODRM_NONE, "Load far pointer into r16 and DS")
MNEM_RM_NF_P(MNEM_LES_R16, "les", PF_66, 0xC4, OPS_R16_RM16, OPN_TWO, SZ_16BIT, MODRM_NONE, "Load far pointer into r16 and ES")


/* ════════════════════════════════════════════════════════════════════════════
 *  80286 SYSTEM INSTRUCTIONS
 *
 *  Protected mode management. All use 0F two-byte opcode prefix.
 * ════════════════════════════════════════════════════════════════════════════ */

/* CLTS: Clear Task-Switched flag in CR0 (0F 06, no ModRM) */
MNEMONIC(MNEM_CLTS, "clts",
         PF_NONE, OPCODE(0x0F,0x06), ENC_DIRECT, OPS_NONE, OPN_NONE, SZ_NONE,
         REG_NONE, MODRM_NONE, FALSE, PROC_80286, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Clear task-switched flag in CR0")

/* LMSW / SMSW: Load/Store Machine Status Word */
MNEM_0F(MNEM_LMSW, "lmsw", 0x01, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_6, "Load machine status word from r/m16")
MNEM_0F(MNEM_SMSW, "smsw", 0x01, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_4, "Store machine status word to r/m16")

/* LLDT / SLDT: Load/Store Local Descriptor Table Register */
MNEM_0F(MNEM_LLDT, "lldt", 0x00, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_2, "Load LDTR from r/m16")
MNEM_0F(MNEM_SLDT, "sldt", 0x00, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_0, "Store LDTR to r/m16")

/* LTR / STR: Load/Store Task Register */
MNEM_0F(MNEM_LTR,  "ltr",  0x00, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_3, "Load task register from r/m16")
MNEM_0F(MNEM_STR,  "str",  0x00, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_1, "Store task register to r/m16")

/* LAR / LSL: Load Access Rights / Load Segment Limit */
MNEM_0F(MNEM_LAR_R32, "lar", 0x02, OPS_R32_RM32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Load access rights byte into r32")
MNEM_0F(MNEM_LSL_R32, "lsl", 0x03, OPS_R32_RM32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Load segment limit into r32")
MNEMONIC(MNEM_LAR_R16, "lar",
         PF_66, OPCODE(0x0F,0x02), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80286, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Load access rights byte into r16")
MNEMONIC(MNEM_LSL_R16, "lsl",
         PF_66, OPCODE(0x0F,0x03), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80286, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Load segment limit into r16")

/* VERR / VERW: Verify segment for reading/writing */
MNEM_0F(MNEM_VERR, "verr", 0x00, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_4, "Verify segment for reading (sets ZF)")
MNEM_0F(MNEM_VERW, "verw", 0x00, OPS_RM16, OPN_ONE, SZ_16BIT, MODRM_5, "Verify segment for writing (sets ZF)")

/* ARPL: Adjust RPL field of segment selector */
MNEM_RM(MNEM_ARPL, "arpl", 0x63, OPS_RM16_R16, OPN_TWO, SZ_16BIT, MODRM_NONE, "Adjust RPL field of selector (sets ZF)")


/* ════════════════════════════════════════════════════════════════════════════
 *  80386 — BIT TEST  (BT, BTS, BTR, BTC)
 *
 *  r/m,r forms: 0F A3/AB/B3/BB (ModRM /r)
 *  r/m,imm8 forms: 0F BA /4.../7
 *  Modifies CF (copies tested bit to CF).
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── BT (Bit Test) ────────────────────────────────────────────────────────── */
MNEM_0F(MNEM_BT_RM32_R32,    "bt",  0xA3, OPS_RM32_R32,  OPN_TWO, SZ_32BIT, MODRM_NONE, "Bit test r/m32 by r32, copy to CF")
MNEM_0F(MNEM_BT_RM32_IMM8,   "bt",  0xBA, OPS_RM32_IMM8, OPN_TWO, SZ_32BIT, MODRM_4,    "Bit test r/m32 by imm8, copy to CF")
MNEMONIC(MNEM_BT_RM16_R16, "bt",
         PF_66, OPCODE(0x0F,0xA3), ENC_MODRM, OPS_RM16_R16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_C, FLG_C, FLG_NONE, FLG_NONE,
         "Bit test r/m16 by r16, copy to CF")
MNEMONIC(MNEM_BT_RM16_IMM8, "bt",
         PF_66, OPCODE(0x0F,0xBA), ENC_MODRM, OPS_RM16_IMM8, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_4, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_C, FLG_C, FLG_NONE, FLG_NONE,
         "Bit test r/m16 by imm8, copy to CF")

/* ── BTS (Bit Test and Set) ───────────────────────────────────────────────── */
MNEM_0F(MNEM_BTS_RM32_R32,   "bts", 0xAB, OPS_RM32_R32,  OPN_TWO, SZ_32BIT, MODRM_NONE, "Bit test and set r/m32 by r32")
MNEM_0F(MNEM_BTS_RM32_IMM8,  "bts", 0xBA, OPS_RM32_IMM8, OPN_TWO, SZ_32BIT, MODRM_5,    "Bit test and set r/m32 by imm8")
MNEMONIC(MNEM_BTS_RM16_R16, "bts",
         PF_66, OPCODE(0x0F,0xAB), ENC_MODRM, OPS_RM16_R16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_LOCK, EX_PREFIX_0F,
         FLG_NONE, FLG_C, FLG_C, FLG_NONE, FLG_NONE,
         "Bit test and set r/m16 by r16")
MNEMONIC(MNEM_BTS_RM16_IMM8, "bts",
         PF_66, OPCODE(0x0F,0xBA), ENC_MODRM, OPS_RM16_IMM8, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_5, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_LOCK, EX_PREFIX_0F,
         FLG_NONE, FLG_C, FLG_C, FLG_NONE, FLG_NONE,
         "Bit test and set r/m16 by imm8")

/* ── BTR (Bit Test and Reset) ─────────────────────────────────────────────── */
MNEM_0F(MNEM_BTR_RM32_R32,   "btr", 0xB3, OPS_RM32_R32,  OPN_TWO, SZ_32BIT, MODRM_NONE, "Bit test and reset r/m32 by r32")
MNEM_0F(MNEM_BTR_RM32_IMM8,  "btr", 0xBA, OPS_RM32_IMM8, OPN_TWO, SZ_32BIT, MODRM_6,    "Bit test and reset r/m32 by imm8")
MNEMONIC(MNEM_BTR_RM16_R16, "btr",
         PF_66, OPCODE(0x0F,0xB3), ENC_MODRM, OPS_RM16_R16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_LOCK, EX_PREFIX_0F,
         FLG_NONE, FLG_C, FLG_C, FLG_NONE, FLG_NONE,
         "Bit test and reset r/m16 by r16")
MNEMONIC(MNEM_BTR_RM16_IMM8, "btr",
         PF_66, OPCODE(0x0F,0xBA), ENC_MODRM, OPS_RM16_IMM8, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_6, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_LOCK, EX_PREFIX_0F,
         FLG_NONE, FLG_C, FLG_C, FLG_NONE, FLG_NONE,
         "Bit test and reset r/m16 by imm8")

/* ── BTC (Bit Test and Complement) ────────────────────────────────────────── */
MNEM_0F(MNEM_BTC_RM32_R32,   "btc", 0xBB, OPS_RM32_R32,  OPN_TWO, SZ_32BIT, MODRM_NONE, "Bit test and complement r/m32 by r32")
MNEM_0F(MNEM_BTC_RM32_IMM8,  "btc", 0xBA, OPS_RM32_IMM8, OPN_TWO, SZ_32BIT, MODRM_7,    "Bit test and complement r/m32 by imm8")
MNEMONIC(MNEM_BTC_RM16_R16, "btc",
         PF_66, OPCODE(0x0F,0xBB), ENC_MODRM, OPS_RM16_R16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_LOCK, EX_PREFIX_0F,
         FLG_NONE, FLG_C, FLG_C, FLG_NONE, FLG_NONE,
         "Bit test and complement r/m16 by r16")
MNEMONIC(MNEM_BTC_RM16_IMM8, "btc",
         PF_66, OPCODE(0x0F,0xBA), ENC_MODRM, OPS_RM16_IMM8, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_7, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_LOCK, EX_PREFIX_0F,
         FLG_NONE, FLG_C, FLG_C, FLG_NONE, FLG_NONE,
         "Bit test and complement r/m16 by imm8")


/* ════════════════════════════════════════════════════════════════════════════
 *  80386 — SHLD / SHRD  (Double-precision shift — sets OSZAPC)
 *
 *  CL forms: two explicit operands (r/m, r), CL implicit.
 *  imm8 forms: three explicit operands (r/m, r, imm8).
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── SHLD (Shift Left Double) ─────────────────────────────────────────────── */
MNEM_0F(MNEM_SHLD_RM32_R32_CL, "shld", 0xA5, OPS_RM32_R32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Shift left double r/m32,r32 by CL")
MNEMONIC(MNEM_SHLD_RM32_R32_I8, "shld",
         PF_NONE, OPCODE(0x0F,0xA4), ENC_MODRM,
         OPS_RM_REG_IMM, OPN_THREE, SZ_32BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Shift left double r/m32,r32 by imm8")
MNEMONIC(MNEM_SHLD_RM16_R16_CL, "shld",
         PF_66, OPCODE(0x0F,0xA5), ENC_MODRM, OPS_RM16_R16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Shift left double r/m16,r16 by CL")
MNEMONIC(MNEM_SHLD_RM16_R16_I8, "shld",
         PF_66, OPCODE(0x0F,0xA4), ENC_MODRM,
         OPS_RM_REG_IMM, OPN_THREE, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Shift left double r/m16,r16 by imm8")

/* ── SHRD (Shift Right Double) ────────────────────────────────────────────── */
MNEM_0F(MNEM_SHRD_RM32_R32_CL, "shrd", 0xAD, OPS_RM32_R32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Shift right double r/m32,r32 by CL")
MNEMONIC(MNEM_SHRD_RM32_R32_I8, "shrd",
         PF_NONE, OPCODE(0x0F,0xAC), ENC_MODRM,
         OPS_RM_REG_IMM, OPN_THREE, SZ_32BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Shift right double r/m32,r32 by imm8")
MNEMONIC(MNEM_SHRD_RM16_R16_CL, "shrd",
         PF_66, OPCODE(0x0F,0xAD), ENC_MODRM, OPS_RM16_R16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Shift right double r/m16,r16 by CL")
MNEMONIC(MNEM_SHRD_RM16_R16_I8, "shrd",
         PF_66, OPCODE(0x0F,0xAC), ENC_MODRM,
         OPS_RM_REG_IMM, OPN_THREE, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_RW, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Shift right double r/m16,r16 by imm8")


/* ════════════════════════════════════════════════════════════════════════════
 *  IMUL — Two-operand (80386) and Three-operand (80186) forms
 *
 *  Two-operand:   0F AF /r  → r = r * r/m  (low half result)
 *  Three-operand: 69 /r id  → r = r/m * imm32
 *                 6B /r ib  → r = r/m * sign-extended imm8
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── Two-operand (80386, 0F AF) ───────────────────────────────────────────── */
MNEM_0F(MNEM_IMUL_R32_RM32_2OP, "imul", 0xAF, OPS_R32_RM32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Signed multiply r32 = r32 * r/m32 (low 32 bits)")
MNEMONIC(MNEM_IMUL_R16_RM16_2OP, "imul",
         PF_66, OPCODE(0x0F,0xAF), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Signed multiply r16 = r16 * r/m16 (low 16 bits)")

/* ── Three-operand (80186, 69/6B) ─────────────────────────────────────────── */
MNEMONIC(MNEM_IMUL_R32_RM32_IMM32, "imul",
         PF_NONE, OPCODE(0x69,0x00), ENC_MODRM,
         OPS_REG_RM_IMM, OPN_THREE, SZ_32BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80186, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Signed multiply r32 = r/m32 * imm32")
MNEMONIC(MNEM_IMUL_R32_RM32_IMM8, "imul",
         PF_NONE, OPCODE(0x6B,0x00), ENC_MODRM,
         OPS_REG_RM_IMM, OPN_THREE, SZ_32BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80186, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Signed multiply r32 = r/m32 * sign-extended imm8")
MNEMONIC(MNEM_IMUL_R16_RM16_IMM16, "imul",
         PF_66, OPCODE(0x69,0x00), ENC_MODRM,
         OPS_REG_RM_IMM, OPN_THREE, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80186, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Signed multiply r16 = r/m16 * imm16")
MNEMONIC(MNEM_IMUL_R16_RM16_IMM8, "imul",
         PF_66, OPCODE(0x6B,0x00), ENC_MODRM,
         OPS_REG_RM_IMM, OPN_THREE, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80186, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_NONE,
         FLG_NONE, FLG_ARITH, FLG_ARITH, FLG_NONE, FLG_NONE,
         "Signed multiply r16 = r/m16 * sign-extended imm8")


/* ════════════════════════════════════════════════════════════════════════════
 *  80386 — LFS / LGS / LSS  (Load far pointer — no flags)
 *
 *  LFS: 0F B4 /r    LGS: 0F B5 /r    LSS: 0F B2 /r
 * ════════════════════════════════════════════════════════════════════════════ */
MNEM_0F(MNEM_LFS_R32, "lfs", 0xB4, OPS_R32_RM32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Load far pointer into r32 and FS")
MNEM_0F(MNEM_LGS_R32, "lgs", 0xB5, OPS_R32_RM32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Load far pointer into r32 and GS")
MNEM_0F(MNEM_LSS_R32, "lss", 0xB2, OPS_R32_RM32, OPN_TWO, SZ_32BIT, MODRM_NONE, "Load far pointer into r32 and SS")
MNEMONIC(MNEM_LFS_R16, "lfs",
         PF_66, OPCODE(0x0F,0xB4), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Load far pointer into r16 and FS")
MNEMONIC(MNEM_LGS_R16, "lgs",
         PF_66, OPCODE(0x0F,0xB5), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Load far pointer into r16 and GS")
MNEMONIC(MNEM_LSS_R16, "lss",
         PF_66, OPCODE(0x0F,0xB2), ENC_MODRM, OPS_R16_RM16, OPN_TWO, SZ_16BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_READ, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Load far pointer into r16 and SS")


/* ════════════════════════════════════════════════════════════════════════════
 *  80386 — MOV to/from Control and Debug registers  (no flags)
 *
 *  0F 20 /r = MOV r32,CRn    0F 22 /r = MOV CRn,r32
 *  0F 21 /r = MOV r32,DRn    0F 23 /r = MOV DRn,r32
 * ════════════════════════════════════════════════════════════════════════════ */
MNEMONIC(MNEM_MOV_R32_CR, "mov",
         PF_NONE, OPCODE(0x0F,0x20), ENC_MODRM, OPS_R32_RM32, OPN_TWO, SZ_32BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Move control register CRn to r32")
MNEMONIC(MNEM_MOV_CR_R32, "mov",
         PF_NONE, OPCODE(0x0F,0x22), ENC_MODRM, OPS_RM32_R32, OPN_TWO, SZ_32BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Move r32 to control register CRn")
MNEMONIC(MNEM_MOV_R32_DR, "mov",
         PF_NONE, OPCODE(0x0F,0x21), ENC_MODRM, OPS_R32_RM32, OPN_TWO, SZ_32BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Move debug register DRn to r32")
MNEMONIC(MNEM_MOV_DR_R32, "mov",
         PF_NONE, OPCODE(0x0F,0x23), ENC_MODRM, OPS_RM32_R32, OPN_TWO, SZ_32BIT,
         REG_NONE, MODRM_NONE, TRUE, PROC_80386, ST_DOCUMENTED, MODE_ANY,
         MEM_NONE, RL_NONE, X_NONE, EX_PREFIX_0F,
         FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE, FLG_NONE,
         "Move r32 to debug register DRn")
