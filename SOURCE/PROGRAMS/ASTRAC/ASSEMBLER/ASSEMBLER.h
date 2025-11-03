#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <STD/TYPEDEF.h>
#include <STD/FS_DISK.h>
#include <STD/MEM.h>
#include <STD/IO.h>
#include <STD/STRING.h>
#include <STD/DEBUG.h>
#include <PROGRAMS/ASTRAC/COMPILER/COMPILER.h>
#include <PROGRAMS/ASTRAC/ASTRAC.h>

typedef struct {
    PU8 *tmp_files;
    U32 tmp_file_count;
} ASM_INFO, *PASM_INFO;


typedef enum _ASM_REGS {
    REG_NONE = 0,

    /* == General Purpose Registers (32-bit) == */
    REG_EAX, REG_EBX, REG_ECX, REG_EDX,
    REG_ESI, REG_EDI, REG_EBP, REG_ESP,

    /* == General Purpose Registers (16-bit) == */
    REG_AX, REG_BX, REG_CX, REG_DX,
    REG_SI, REG_DI, REG_BP, REG_SP,

    /* == General Purpose Registers (8-bit) == */
    REG_AL, REG_AH, REG_BL, REG_BH,
    REG_CL, REG_CH, REG_DL, REG_DH,

    /* == Instruction Pointer == */
    REG_IP,  // 16-bit IP (real mode)
    REG_EIP, // 32-bit instruction pointer

    /* == Segment Registers == */
    REG_CS, REG_DS, REG_ES, REG_FS, REG_GS, REG_SS,

    /* == Control Registers == */
    REG_CR0, REG_CR2, REG_CR3, REG_CR4,

    /* == Debug Registers == */
    REG_DR0, REG_DR1, REG_DR2, REG_DR3, REG_DR6, REG_DR7,

    /* == x87 Floating Point Registers == */
    REG_ST0, REG_ST1, REG_ST2, REG_ST3,
    REG_ST4, REG_ST5, REG_ST6, REG_ST7,

    /* == MMX Registers == */
    REG_MM0, REG_MM1, REG_MM2, REG_MM3,
    REG_MM4, REG_MM5, REG_MM6, REG_MM7,

    /* == SSE Registers (Pentium III and later, still i386-compatible) == */
    REG_XMM0, REG_XMM1, REG_XMM2, REG_XMM3,
    REG_XMM4, REG_XMM5, REG_XMM6, REG_XMM7,
} ASM_REGS;

#define MNEMONIC(name, mnemonic, opcode, operands) mnemonic,
typedef enum _ASM_MNEMONIC {
    #include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONICS.h>
    MNEM_MAX
} ASM_MNEMONIC;
#undef MNEMONIC

typedef struct {
    const char* name;
    ASM_MNEMONIC mnemonic;
    U8 opcode;
    U8 operand_count;
} ASM_MNEMONIC_TABLE;

#define MNEMONIC(name, mnemonic, opcode, operands) { name, mnemonic, opcode, operands },
ASM_MNEMONIC_TABLE asm_mnemonics[] = {
    #include <PROGRAMS/ASTRAC/ASSEMBLER/MNEMONICS.h>
};
#undef MNEMONIC

typedef enum _ASM_TOKEN_TYPE {
    TOK_NONE,
    TOK_EOF,
    TOK_LABEL,       // e.g. "main:"
    TOK_LOCAL_LABEL, // e.g. "@@1, @F, @b"
    TOK_MNEMONIC,    // e.g. "mov", "add", "jmp"
    TOK_REGISTER,    // e.g. "eax", "r8", "al"
    TOK_NUMBER,      // e.g. "123", "0xFF", "0b101", 'S'
    TOK_STRING,      // e.g. "Hello"
    TOK_SYMBOL,      // ',', '[', ']', '+', '-', etc
    TOK_IDENTIFIER,  // variables, constants, macros. only these a case-sensitive
    TOK_DIRECTIVE,   // .section, .data, .text, etc
    TOK_AMOUNT,
} ASM_TOKEN_TYPE;

typedef enum _ASM_DIRECTIVE {
    DIR_SECTION,
    DIR_DATA,
    DIR_RODATA,
    DIR_CODE,
    DIR_BSS,
    DIR_TEXT,
} ASM_DIRECTIVE;

typedef struct {
    ASM_TOKEN_TYPE type;
    PU8 txt;
    U32 line;
    U32 col;
    union {
        U32 num;
        ASM_REGS reg;
        ASM_MNEMONIC mnemonic;
        ASM_DIRECTIVE directive;
    };
} ASM_TOK, *PASM_TOK;

#define MAX_TOKENS 1024

typedef struct {
    U32 len;
    PASM_TOK* toks;
} ASM_TOK_ARRAY;

typedef struct {
    U32 type;
    PU8 txt;
    U32 line;
    U32 col;
    struct ASM_SYNT *next;
} ASM_SYNT, *PASM_SYNT;

typedef struct {
    U32 len;
    PASM_SYNT* toks;
} ASM_AST_ARRAY;



BOOLEAN START_ASSEMBLING();

VOID DESTROY_ASM_INFO(PASM_INFO info);

PASM_INFO PREPROCESS_ASM();

ASM_TOK_ARRAY *LEX(PASM_INFO info);
VOID DESTROY_TOK_ARR(ASM_TOK_ARRAY *toks);

ASM_AST_ARRAY *ASM_AST(ASM_TOK_ARRAY toks, PASM_INFO info);
VOID DESTROY_AST_ARR(ASM_AST_ARRAY *ast);

BOOLEAN VERIFY_AST(ASM_AST_ARRAY ast, PASM_INFO info);
BOOLEAN OPTIMIZE(ASM_AST_ARRAY ast, PASM_INFO info);
BOOLEAN GEN_BINARY(ASM_AST_ARRAY ast, PASM_INFO info);

#endif // ASSEMBLER_H