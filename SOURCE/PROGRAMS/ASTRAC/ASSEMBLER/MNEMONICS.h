#ifndef MNEMONICS_H
#define MNEMONICS_H
#include <STD/TYPEDEF.h>


//
// ─── ENCODING TYPE ─────────────────────────────────────────────────────────────
//
typedef enum {
    ENC_DIRECT,        // opcode only (e.g. push cs)
    ENC_REG_OPCODE,    // opcode + reg encoded in opcode low bits
    ENC_MODRM,         // uses ModR/M byte
    ENC_IMM,           // immediate follows
} ASM_ENCODING_TYPE;

//
// ─── OPERAND TYPE ─────────────────────────────────────────────────────────────
//
typedef enum {
    OP_NONE,
    OP_REG,
    OP_MEM,
    OP_IMM,
    OP_SEG,
    OP_PTR,
    OP_MOFFS,
} ASM_OPERAND_TYPE;

//
// ─── OPERAND SIZE (16- and 32-bit variants only) ──────────────────────────────
//
typedef enum {
    SZ_NONE = 0,
    SZ_8BIT = 8,
    SZ_16BIT = 16,
    SZ_32BIT = 32,
} ASM_OPERAND_SIZE;

//
// ─── OPERAND COUNT ────────────────────────────────────────────────────────────
//
typedef enum {
    OPN_NONE  = 0,
    OPN_ONE   = 1,
    OPN_TWO   = 2,
    OPN_THREE = 3,
    OPN_FOUR  = 4,
} ASM_OPERAND_COUNT;

//
// ─── FIXED REGISTER ───────────────────────────────────────────────────────────
//
typedef enum {
    RF_NONE = -1,
    RF_AX, RF_BX, RF_CX, RF_DX,
    RF_SI, RF_DI, RF_BP, RF_SP,
    RF_CS, RF_DS, RF_ES, RF_SS, RF_FS, RF_GS,
} ASM_FIXED_REGISTER;

//
// ─── MODRM EXTENSION (/digit) ─────────────────────────────────────────────────
//
typedef enum {
    MODRM_NONE = -1,
    MODRM_0 = 0,
    MODRM_1,
    MODRM_2,
    MODRM_3,
    MODRM_4,
    MODRM_5,
    MODRM_6,
    MODRM_7,
} ASM_MODRM_EXTENSION;

//
// ─── PREFIX FIELDS (pf / 0F / po / so) ────────────────────────────────────────
// pf = prefix byte (e.g., 0x66 or 0xF2)
// 0F = secondary opcode prefix (true if instruction uses 0F)
// po = primary opcode
// so = secondary opcode (for multi-byte ops)
//
typedef enum {
    PF_NONE = 0x00,
    PF_66   = 0x66,
    PF_F2   = 0xF2,
    PF_F3   = 0xF3,
} ASM_PREFIX;

typedef enum {
    PFX_NONE = 0,
    PFX_0F   = 1,
} ASM_OPCODE_PREFIX;

//
// ─── PROCESSOR AND MODE ENUMS ─────────────────────────────────────────────────
//
typedef enum {
    PROC_ANY,
    PROC_8086,
    PROC_80186,
    PROC_80286,
    PROC_80386,
    PROC_PENTIUM,
} ASM_PROCESSOR;

typedef enum {
    ST_DOCUMENTED,
    ST_UNDOCUMENTED,
} ASM_DOC_STATUS;

typedef enum {
    MODE_REAL,
    MODE_PROTECTED,
    MODE_V86,
} ASM_CPU_MODE;

typedef enum {
    MEM_NONE,
    MEM_READ,
    MEM_WRITE,
    MEM_RW,
} ASM_MEMORY_ACCESS;

typedef enum {
    RL_NONE,
    RL_REL8,
    RL_REL16,
    RL_REL32,
} ASM_RELTYPE;

typedef enum {
    X_NONE,
    X_LOCK,
    X_FPU_PUSH,
    X_FPU_POP,
} ASM_LOCK_FPU_TYPE;

typedef enum {
    EX_NONE,
    EX_IMPLICIT,
    EX_PREFIX_0F,
} ASM_OPCODE_EXTENSION;

//
// ─── FLAGS (tested / modified / defined / undefined / f-values) ───────────────
//
typedef enum {
    FLG_NONE = 0,
    FLG_O = 1 << 0,
    FLG_S = 1 << 1,
    FLG_Z = 1 << 2,
    FLG_A = 1 << 3,
    FLG_P = 1 << 4,
    FLG_C = 1 << 5,
} ASM_FLAGS;
#define FLG_ARITH (FLG_O | FLG_S | FLG_Z | FLG_A | FLG_P | FLG_C)
#define FLG_ALL FLG_ARITH
#define FLG_LOGIC (FLG_S | FLG_Z | FLG_P) // O, C cleared; A undefined
#define FLG_ARITH_NO_C (FLG_O | FLG_S | FLG_Z | FLG_A | FLG_P) // For INC/DEC



#define OPS_NONE  { OP_NONE, OP_NONE, OP_NONE, OP_NONE }
#define OPS_RM8_R8 { OP_MEM, OP_REG, OP_NONE, OP_NONE }
#define OPS_RM16_R16 { OP_MEM, OP_REG, OP_NONE, OP_NONE }
#define OPS_R8_RM8 { OP_REG, OP_MEM, OP_NONE, OP_NONE }
#define OPS_R16_RM16 { OP_REG, OP_MEM, OP_NONE, OP_NONE }
#define OPS_REG_IMM8 { OP_REG, OP_IMM, OP_NONE, OP_NONE }
#define OPS_REG_IMM16 { OP_REG, OP_IMM, OP_NONE, OP_NONE }
#define OPS_REG_IMM32 { OP_REG, OP_IMM, OP_NONE, OP_NONE }
#define OPS_RM8_IMM8 { OP_MEM, OP_IMM, OP_NONE, OP_NONE }
#define OPS_RM16_IMM16 { OP_MEM, OP_IMM, OP_NONE, OP_NONE }
#define OPS_RM32_IMM32 { OP_MEM, OP_IMM, OP_NONE, OP_NONE }
#define OPS_SEG { OP_SEG, OP_NONE, OP_NONE, OP_NONE }
#define OPS_REG { OP_REG, OP_NONE, OP_NONE, OP_NONE }
#define OPS_REL8 { OP_IMM, OP_NONE, OP_NONE, OP_NONE }
#define OPS_REL16 { OP_IMM, OP_NONE, OP_NONE, OP_NONE }
#define OPS_REL32 { OP_IMM, OP_NONE, OP_NONE, OP_NONE }
#define OPS_RM16 { OP_MEM, OP_NONE, OP_NONE, OP_NONE }
#define OPS_RM32 { OP_MEM, OP_NONE, OP_NONE, OP_NONE }
#define OPS_RM16_SEG { OP_MEM, OP_SEG, OP_NONE, OP_NONE }
#define OPS_RM32_SEG { OP_MEM, OP_SEG, OP_NONE, OP_NONE }
#define OPS_SEG_RM16 { OP_SEG, OP_MEM, OP_NONE, OP_NONE }
#define OPS_SEG_RM32 { OP_SEG, OP_MEM, OP_NONE, OP_NONE }

// Direct address MOVS
#define OPS_REG_MOFFS8 { OP_REG, OP_MOFFS, OP_NONE, OP_NONE }
#define OPS_REG_MOFFS16 { OP_REG, OP_MOFFS, OP_NONE, OP_NONE }
#define OPS_REG_MOFFS32 { OP_REG, OP_MOFFS, OP_NONE, OP_NONE }
#define OPS_MOFFS8_REG { OP_MOFFS, OP_REG, OP_NONE, OP_NONE }
#define OPS_MOFFS16_REG { OP_MOFFS, OP_REG, OP_NONE, OP_NONE }
#define OPS_MOFFS32_REG { OP_MOFFS, OP_REG, OP_NONE, OP_NONE }



/**
 * @brief Defines a mnemonic entry for standard (non-0F-prefixed) x86 instructions.
 *
 * @param mnemonic     Enum identifier for the mnemonic (e.g. MNEM_ADD).
 * @param name         String name of the instruction (e.g. "add").
 * @param prefix       Instruction prefix (e.g. 0x66, PF_F3).
 * @param opcode       Primary opcode byte(s) of the instruction.
 * @param encoding     Encoding type (e.g. ENC_MODRM, ENC_IMM, ENC_DIRECT).
 * @param operand      Array of operand types (up to 4 operands).
 * @param operand_count Number of operands (0–4).
 * @param size         Operand size (e.g. SZ_8BIT, SZ_16BIT, SZ_32BIT).
 * @param reg_fixed    Fixed register for instruction (e.g. RF_AX or RF_NONE).
 * @param modrm_ext    ModR/M extension digit (/0–/7 or MODRM_NONE).
 * @param has_modrm    Whether the instruction uses a ModR/M byte.
 * @param processor    CPU generation (e.g. PROC_8086, PROC_80386, PROC_PENTIUM).
 * @param status       Documentation status (e.g. ST_DOCUMENTED, ST_UNDOCUMENTED).
 * @param mode         CPU mode (e.g. MODE_REAL, MODE_PROTECTED, MODE_V86).
 * @param memory       Memory access type (e.g. MEM_READ, MEM_WRITE, MEM_RW).
 * @param rel_type     Relative addressing type (e.g. RL_NONE, RL_REL8).
 * @param lock_type    Lock/FPU category (e.g. X_NONE, X_LOCK, X_FPU_PUSH).
 * @param opcode_ext   Opcode extension (e.g. EX_NONE, EX_IMPLICIT).
 * @param tested_f     Flags tested by the instruction.
 * @param modif_f      Flags modified by the instruction.
 * @param def_f        Flags defined by the instruction.
 * @param undef_f      Flags undefined by the instruction.
 * @param flags_val    Resulting flag values after execution.
 * @param description  Human-readable description or notes about the instruction.
 */

#define MNEMONIC(mnemonic, name, prefix, opcode, encoding, operand, operand_count, size, reg_fixed, modrm_ext, has_modrm, processor, status, mode, memory, rel_type, lock_type, opcode_ext, tested_f, modif_f, def_f, undef_f, flags_val, description) mnemonic,
typedef enum _ASM_MNEMONIC {
    #include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONIC_LIST.h>
} ASM_MNEMONIC;
#undef MNEMONIC

//
// ─── MNEMONIC TABLE STRUCTURE ─────────────────────────────────────────────────
//
typedef struct {
    ASM_MNEMONIC       mnemonic;       // enumeration value (MNEM_ADD, etc.)
    const PU8          name;           // "add"
    ASM_PREFIX         prefix;         // 0x66, 0xF2, etc.
    ASM_OPCODE_PREFIX  opcode_prefix;  // 0F escape
    U8                 opcode[2];      // primary/secondary opcode bytes
    ASM_ENCODING_TYPE  encoding;       // direct/modrm/imm/etc.
    ASM_OPERAND_TYPE   operand[4];     // operand types
    ASM_OPERAND_COUNT  operand_count;  // operand count
    ASM_OPERAND_SIZE   size;           // operand size (8/16/32)
    ASM_FIXED_REGISTER reg_fixed;      // fixed register (if any)
    ASM_MODRM_EXTENSION modrm_ext;     // /digit field
    BOOL               has_modrm;      // uses ModR/M?
    ASM_PROCESSOR      processor;      // processor generation
    ASM_DOC_STATUS     status;         // documented/undocumented
    ASM_CPU_MODE       mode;           // real/protected/V86
    ASM_MEMORY_ACCESS  memory;         // memory access type
    ASM_RELTYPE        rel_type;       // relative type (for jumps)
    ASM_LOCK_FPU_TYPE  lock_type;      // lock/fpu push/pop
    ASM_OPCODE_EXTENSION opcode_ext;   // implicit or prefix extension
    ASM_FLAGS          tested_flags;   // tested flags
    ASM_FLAGS          modified_flags; // modified flags
    ASM_FLAGS          defined_flags;  // defined flags
    ASM_FLAGS          undefined_flags;// undefined flags
    ASM_FLAGS          flags_value;    // resulting flag values
    PU8          description;    // description/notes
} ASM_MNEMONIC_TABLE;

//
// ─── TABLE INITIALIZER MACROS ─────────────────────────────────────────────────
//

/**
 * @brief Defines a mnemonic entry for standard (non-0F-prefixed) x86 instructions.
 *
 * @param mnemonic     Enum identifier for the mnemonic (e.g. MNEM_ADD).
 * @param name         String name of the instruction (e.g. "add").
 * @param prefix       Instruction prefix (e.g. 0x66, PF_F3).
 * @param opcode       Primary opcode byte(s) of the instruction.
 * @param encoding     Encoding type (e.g. ENC_MODRM, ENC_IMM, ENC_DIRECT).
 * @param operand      Array of operand types (up to 4 operands).
 * @param operand_count Number of operands (0–4).
 * @param size         Operand size (e.g. SZ_8BIT, SZ_16BIT, SZ_32BIT).
 * @param reg_fixed    Fixed register for instruction (e.g. RF_AX or RF_NONE).
 * @param modrm_ext    ModR/M extension digit (/0–/7 or MODRM_NONE).
 * @param has_modrm    Whether the instruction uses a ModR/M byte.
 * @param processor    CPU generation (e.g. PROC_8086, PROC_80386, PROC_PENTIUM).
 * @param status       Documentation status (e.g. ST_DOCUMENTED, ST_UNDOCUMENTED).
 * @param mode         CPU mode (e.g. MODE_REAL, MODE_PROTECTED, MODE_V86).
 * @param memory       Memory access type (e.g. MEM_READ, MEM_WRITE, MEM_RW).
 * @param rel_type     Relative addressing type (e.g. RL_NONE, RL_REL8).
 * @param lock_type    Lock/FPU category (e.g. X_NONE, X_LOCK, X_FPU_PUSH).
 * @param opcode_ext   Opcode extension (e.g. EX_NONE, EX_IMPLICIT).
 * @param tested_f     Flags tested by the instruction.
 * @param modif_f      Flags modified by the instruction.
 * @param def_f        Flags defined by the instruction.
 * @param undef_f      Flags undefined by the instruction.
 * @param flags_val    Resulting flag values after execution.
 * @param description  Human-readable description or notes about the instruction.
 */
#define MNEMONIC(mnemonic, name, prefix, opcode, encoding, operand, operand_count, size, reg_fixed, modrm_ext, has_modrm, processor, status, mode, memory, rel_type, lock_type, opcode_ext, tested_f, modif_f, def_f, undef_f, flags_val, description) \
    { mnemonic, name, prefix, PFX_NONE, opcode, encoding, operand, operand_count, size, reg_fixed, modrm_ext, has_modrm, processor, status, mode, memory, rel_type, lock_type, opcode_ext, tested_f, modif_f, def_f, undef_f, flags_val, description },
    
/**
 * @brief Defines a mnemonic entry for 0F-prefixed (two-byte opcode) x86 instructions.
 *
 * @param mnemonic     Enum identifier for the mnemonic (e.g. MNEM_JE, MNEM_MOVZX).
 * @param name         String name of the instruction (e.g. "movzx").
 * @param prefix       Instruction prefix (e.g. 0x66, PF_F3).
 * @param opcode       Secondary opcode byte(s) following the 0F escape.
 * @param encoding     Encoding type (e.g. ENC_MODRM, ENC_IMM, ENC_DIRECT).
 * @param operand      Array of operand types (up to 4 operands).
 * @param operand_count Number of operands (0–4).
 * @param size         Operand size (e.g. SZ_8BIT, SZ_16BIT, SZ_32BIT).
 * @param reg_fixed    Fixed register for instruction (e.g. RF_AX or RF_NONE).
 * @param modrm_ext    ModR/M extension digit (/0–/7 or MODRM_NONE).
 * @param has_modrm    Whether the instruction uses a ModR/M byte.
 * @param processor    CPU generation (e.g. PROC_8086, PROC_80386, PROC_PENTIUM).
 * @param status       Documentation status (e.g. ST_DOCUMENTED, ST_UNDOCUMENTED).
 * @param mode         CPU mode (e.g. MODE_REAL, MODE_PROTECTED, MODE_V86).
 * @param memory       Memory access type (e.g. MEM_READ, MEM_WRITE, MEM_RW).
 * @param rel_type     Relative addressing type (e.g. RL_NONE, RL_REL8).
 * @param lock_type    Lock/FPU category (e.g. X_NONE, X_LOCK, X_FPU_PUSH).
 * @param opcode_ext   Opcode extension (e.g. EX_NONE, EX_PREFIX_0F).
 * @param tested_f     Flags tested by the instruction.
 * @param modif_f      Flags modified by the instruction.
 * @param def_f        Flags defined by the instruction.
 * @param undef_f      Flags undefined by the instruction.
 * @param flags_val    Resulting flag values after execution.
 * @param description  Human-readable description or notes about the instruction.
 */
#define MNEMONIC_0F(mnemonic, name, prefix, opcode, encoding, operand, operand_count, size, reg_fixed, modrm_ext, has_modrm, processor, status, mode, memory, rel_type, lock_type, opcode_ext, tested_f, modif_f, def_f, undef_f, flags_val, description) \
    { mnemonic, name, prefix, PFX_0F, opcode, encoding, operand, operand_count, size, reg_fixed, modrm_ext, has_modrm, processor, status, mode, memory, rel_type, lock_type, opcode_ext, tested_f, modif_f, def_f, undef_f, flags_val, description },

//
// ─── MNEMONIC TABLE ───────────────────────────────────────────────────────────
//
static const ASM_MNEMONIC_TABLE asm_mnemonics[] = {
    #include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONIC_LIST.h>
};
#undef MNEMONIC
#undef MNEMONIC_0F

#endif // MNEMONICS_H
