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
    REG_NONE = -1,

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


typedef enum _ASM_SYMBOLS {
    SYM_NONE = 0,

    /* Delimiters */
    SYM_COMMA,      // ,
    SYM_COLON,      // :
    SYM_SEMICOLON,  // ; (should already be removed by preprocessor)
    
    /* Brackets for memory addressing */
    SYM_LBRACKET,   // [
    SYM_RBRACKET,   // ]
    SYM_LPAREN,     // (
    SYM_RPAREN,     // )

    /* Arithmetic / Logical Operators */
    SYM_PLUS,       // +
    SYM_MINUS,      // -
    SYM_ASTERISK,   // *
    SYM_SLASH,      // /
    SYM_PERCENT,    // %

    /* Bitwise Operators */
    SYM_AND,        // &
    SYM_OR,         // |
    SYM_XOR,        // ^
    SYM_TILDE,      // ~
    SYM_SHL,        // <<
    SYM_SHR,        // >>

    /* Assignment / Special */
    SYM_EQUALS,     // =

    /* Others */
    SYM_DOT,        // .
    SYM_AT,         // @
    SYM_DOLLAR,     // $ (current instruction pointer)
    
    SYM_AMOUNT
} ASM_SYMBOLS;

typedef enum _ASM_VAR_TYPE {
    TYPE_NONE,
    
    TYPE_BYTE,  // 8-bit
    TYPE_WORD,  // 16-bit
    TYPE_DWORD, // 32-bit
    TYPE_FLOAT, // 32-bit
    TYPE_PTR,   // 16-bit or 32-bit depends on .use case

    TYPE_AMOUNT,
} ASM_VAR_TYPE;

typedef enum _ASM_TOKEN_TYPE {
    TOK_NONE,
    TOK_EOL,
    TOK_EOF,
    TOK_LABEL,       // e.g. "main:"
    TOK_LOCAL_LABEL, // e.g. "@@1, @F, @b"
    TOK_MNEMONIC,    // e.g. "mov", "add", "jmp"
    TOK_REGISTER,    // e.g. "eax", "r8", "al"
    TOK_NUMBER,      // e.g. "123", "0xFF", "0b101", 'S'
    TOK_STRING,      // e.g. "Hello"
    TOK_SYMBOL,      // ',', '[', ']', '+', '-', etc
    TOK_IDENT_VAR,   // DD, DB, BYTE, WORD etc
    TOK_IDENTIFIER,  // variables, constants, macros. only these a case-sensitive
    TOK_DIRECTIVE,   // .section, .data, .text, etc
    TOK_AMOUNT,
} ASM_TOKEN_TYPE;

typedef enum _ASM_DIRECTIVE {
    DIR_NONE,

    /*
    We only have .DATA and .RODATA because BSS is unreliable
    */
    DIR_DATA,
    DIR_RODATA,
    
    DIR_CODE,
    DIR_CODE_TYPE_32,
    DIR_CODE_TYPE_16,
    DIR_AMOUNT,
} ASM_DIRECTIVE;

typedef enum _ASM_LABEL {
    LABEL_NONE,
} ASM_LABEL;

typedef struct {
    ASM_TOKEN_TYPE type;
    PU8 txt;
    U32 line;
    U32 col;
    union {
        U32 num;
        ASM_REGS reg;
        U32 mnemonic;
        ASM_SYMBOLS symbol;
        ASM_DIRECTIVE directive;
        ASM_VAR_TYPE var_type;
    };
} ASM_TOK, *PASM_TOK;

#define MAX_TOKENS 1024

typedef struct {
    U32 len;
    PASM_TOK toks[MAX_TOKENS];
} ASM_TOK_ARRAY;

typedef struct _ASM_VAR {
    ASM_VAR_TYPE var_type;
    PU8 name; // Name of variable
    PU8 start_value;

    BOOL is_list; // If true, value will be tokenized by ',' char
    U32 list_len;

} ASM_VAR, *PASM_VAR;

#define MAX_VARS 100
typedef struct _ASM_VAR_ARR {
    PASM_VAR vars[MAX_VARS];
    U32 len;
} ASM_VAR_ARR;

typedef enum {
    // Top-level structure types
    SYNT_NODE_INVALID,

    // Code section
    SYNT_NODE_INSTRUCTION, // e.g., ADD AX, 5
    SYNT_NODE_LABEL,       // e.g., main_loop:
    SYNT_NODE_LOCAL_LABEL, // e,g., @@1:
    SYNT_NODE_RAW_NUM,     // Raw number inserted

    // Data sections
    SYNT_NODE_DIRECTIVE,   // e.g., DB test 10


} ASM_NODE_NODE_TYPE;

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
    ST_MARGINALLY,
    ST_UNDOCUMENTED,
} ASM_DOC_STATUS;

typedef enum {
    MODE_REAL,
    MODE_PROTECTED,
    MODE_V86,
    MODE_ANY = MODE_REAL | MODE_PROTECTED | MODE_V86,
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


#define OPS_PTR { OP_PTR, OP_NONE, OP_NONE, OP_NONE }
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

//
// ─── MNEMONIC TABLE STRUCTURE ─────────────────────────────────────────────────
//
typedef struct {
    U32                mnemonic;       // enum value from ASM_MNEMONIC
    const PU8          name;           // "add"
    ASM_PREFIX         prefix;         // 0x66, 0xF2, etc.
    ASM_OPCODE_PREFIX  opcode_prefix;  // 0F escape
    U8                 opcode[2];      // primary/secondary opcode bytes
    ASM_ENCODING_TYPE  encoding;       // direct/modrm/imm/etc.
    ASM_OPERAND_TYPE   operand[4];     // operand types
    ASM_OPERAND_COUNT  operand_count;  // operand count
    ASM_OPERAND_SIZE   size;           // operand size (8/16/32)
    ASM_REGS reg_fixed;      // fixed register (if any)
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

typedef struct ASM_NODE_MEM {
    ASM_REGS base_reg;    // RF_EAX (Base register)
    ASM_REGS index_reg;   // RF_EBX (Index register)
    S32 scale;                      // 1, 2, 4, or 8 (Scale factor)
    S32 displacement;               // Constant offset
    ASM_REGS segment;     // RF_CS, RF_DS, etc. (Segment override, if present)
    PU8 symbol_name;                // If the displacement is a label/symbol (e.g., [var_a])
} ASM_NODE_MEM;

typedef struct {
    ASM_OPERAND_TYPE    type; // OP_REG, OP_IMM, OP_MEM, etc. (from your original code)
    ASM_OPERAND_SIZE    size; // SZ_8BIT, SZ_16BIT, SZ_32BIT (from your original code)
    
    // Tagged union for operand details
    union {
        // OP_REG
        ASM_REGS                reg;        // If type is OP_REG (e.g., ASM_REGS_AX)

        // OP_IMM
        U32                     immediate;  // If type is OP_IMM (handles all sizes)

        // OP_MEM (requires a dedicated memory structure)
        struct ASM_NODE_MEM* mem_ref;    // If type is OP_MEM
    };
} ASM_OPERAND_SYNT;


typedef struct ASM_NODE {
    ASM_NODE_NODE_TYPE  node_type;
    U32                 line;
    U32                 col;
    struct ASM_NODE* next; // Link to the next statement/instruction

    // Details specific to the node_type
    union {
        // SYNT_NODE_INSTRUCTION
        struct {
            const ASM_MNEMONIC_TABLE *mnem;
        } instruction;

        // SYNT_NODE_LABEL
        struct {
            PU8                     name;
            // Additional fields for public/external status
        } label;
        
        // SYNT_NODE_DIRECTIVE (e.g., DB, EQU, SECTION)
        struct {
            ASM_DIRECTIVE           directive_type;
            // Union for directive arguments (data list for DB, value for EQU, etc.)
            // Example:
            struct {
                U32 element_size;
                // Pointer to an array of immediate values or strings
            } data_def;
        } directive;
    };
} ASM_NODE, *PASM_NODE;

#define MAX_NODES 1024
typedef struct {
    U32 len;
    PASM_NODE toks[MAX_NODES];
} ASM_AST_ARRAY;

typedef struct {
    PU8 value;
    U32 enum_val;
} KEYWORD;

const KEYWORD *get_registers();
const KEYWORD *get_directives();
const KEYWORD *get_symbols();
const KEYWORD *get_variable_types();


BOOLEAN START_ASSEMBLING();

VOID DESTROY_ASM_INFO(PASM_INFO info);

PASM_INFO PREPROCESS_ASM();

ASM_TOK_ARRAY *LEX(PASM_INFO info);
VOID DESTROY_TOK_ARR(ASM_TOK_ARRAY *toks);

ASM_AST_ARRAY *ASM_AST(ASM_TOK_ARRAY *toks, PASM_INFO info);
VOID DESTROY_AST_ARR(ASM_AST_ARRAY *ast);

BOOLEAN VERIFY_AST(ASM_AST_ARRAY ast, PASM_INFO info);
BOOLEAN OPTIMIZE(ASM_AST_ARRAY ast, PASM_INFO info);
BOOLEAN GEN_BINARY(ASM_AST_ARRAY ast, PASM_INFO info);

#endif // ASSEMBLER_H