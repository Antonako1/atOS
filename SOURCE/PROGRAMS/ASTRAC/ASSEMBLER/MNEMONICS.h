#ifndef MNEMONICS_H
#define MNEMONICS_H
#include <STD/TYPEDEF.h>
#include <PROGRAMS/ASTRAC/ASSEMBLER/ASSEMBLER.h>

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
