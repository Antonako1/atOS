#ifndef MNEMONICS_H
#define MNEMONICS_H
#include <STD/TYPEDEF.h>

#define MNEMONIC(name, mnemonic, opcode, operands) mnemonic,
typedef enum _ASM_MNEMONIC {
    #include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONIC_LIST.h>
} ASM_MNEMONIC;
#undef MNEMONIC

typedef struct {
    const char* name;
    ASM_MNEMONIC mnemonic;
    U8 opcode[4];
    U8 operand_count;
} ASM_MNEMONIC_TABLE;

#define MNEMONIC(name, mnemonic, opcode, operands) { name, mnemonic, opcode, operands },
ASM_MNEMONIC_TABLE asm_mnemonics[] = {
    #include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONIC_LIST.h>
};
#undef MNEMONIC
#endif // MNEMONICS_H