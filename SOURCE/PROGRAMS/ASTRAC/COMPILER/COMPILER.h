#ifndef COMPILER_H
#define COMPILER_H

#include <STD/TYPEDEF.h>
#include <STD/FS_DISK.h>
#include <STD/MEM.h>
#include <STD/IO.h>
#include <STD/STRING.h>

/* ════════════════════════════════════════════════════════════════════════════
 *  TOKEN TYPES
 *  The lexer normalises all identifiers and keywords to UPPER CASE.
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _COMP_TOK_TYPE {
    /* Sentinels */
    CTOK_EOF = 0,
    CTOK_ERROR,

    /* Literals */
    CTOK_INT_LIT,       /* 123 / 0xFF / 0b101          */
    CTOK_FLOAT_LIT,     /* 3.14                         */
    CTOK_STR_LIT,       /* "hello"                      */
    CTOK_CHAR_LIT,      /* 'A'                          */

    /* Identifier (anything that is not a keyword) */
    CTOK_IDENT,

    /* ── Keywords: primitive types ─────────────────────────────── */
    CTOK_KW_U8,
    CTOK_KW_I8,
    CTOK_KW_U16,
    CTOK_KW_I16,
    CTOK_KW_U32,
    CTOK_KW_I32,

    CTOK_KW_PU8,      /* Pointer shorthand: P<TYPE> (e.g. PU8 = pointer to U8) */
    CTOK_KW_PU16,     /* Pointer shorthand: P<TYPE> (e.g. PU16 = pointer to U16) */
    CTOK_KW_PU32,     /* Pointer shorthand: P<TYPE> (e.g. PU32 = pointer to U32) */
    CTOK_KW_PPU8,     /* Pointer shorthand: PP<TYPE> (e.g. PPU8 = pointer to pointer to U8) */
    CTOK_KW_PPU16,    /* Pointer shorthand: PP<TYPE> (e.g. PPU16 = pointer to pointer to U16) */
    CTOK_KW_PPU32,    /* Pointer shorthand: PP<TYPE> (e.g. PPU32 = pointer to pointer to U32) */

    CTOK_KW_PI8,      /* Pointer shorthand: P<TYPE> (e.g. PI8 = pointer to I8) */
    CTOK_KW_PI16,     /* Pointer shorthand: P<TYPE> (e.g. PI16 = pointer to I16) */
    CTOK_KW_PI32,     /* Pointer shorthand: P<TYPE> (e.g. PI32 = pointer to I32) */
    CTOK_KW_PPI8,     /* Pointer shorthand: PP<TYPE> (e.g. PPI8 = pointer to pointer to I8) */
    CTOK_KW_PPI16,    /* Pointer shorthand: PP<TYPE> (e.g. PPI16 = pointer to pointer to I16) */
    CTOK_KW_PPI32,    /* Pointer shorthand: PP<TYPE> (e.g. PPI32 = pointer to pointer to I32) */
    
    CTOK_KW_F32,
    CTOK_KW_U0,         /* void                         */
    CTOK_KW_BOOL,
    CTOK_KW_TRUE,
    CTOK_KW_FALSE,
    CTOK_KW_NULLPTR,
    CTOK_KW_VOIDPTR,    /* untyped pointer (void*)      */

    /* ── Keywords: control flow ─────────────────────────────────── */
    CTOK_KW_IF,
    CTOK_KW_ELSE,
    CTOK_KW_FOR,
    CTOK_KW_WHILE,
    CTOK_KW_DO,
    CTOK_KW_RETURN,
    CTOK_KW_BREAK,
    CTOK_KW_CONTINUE,
    CTOK_KW_SWITCH,
    CTOK_KW_CASE,
    CTOK_KW_DEFAULT,
    CTOK_KW_GOTO,

    /* ── Keywords: declarations ─────────────────────────────────── */
    CTOK_KW_STRUCT,
    CTOK_KW_UNION,
    CTOK_KW_ENUM,
    CTOK_KW_SIZEOF,

    /* Inline assembly block */
    CTOK_KW_ASM,

    /* ── Arithmetic operators ───────────────────────────────────── */
    CTOK_PLUS,          /* +   */
    CTOK_MINUS,         /* -   */
    CTOK_STAR,          /* *   */
    CTOK_SLASH,         /* /   */
    CTOK_PERCENT,       /* %   */
    CTOK_PLUSPLUS,      /* ++  */
    CTOK_MINUSMINUS,    /* --  */

    /* ── Relational operators ───────────────────────────────────── */
    CTOK_EQ,            /* ==  */
    CTOK_NEQ,           /* !=  */
    CTOK_LT,            /* <   */
    CTOK_GT,            /* >   */
    CTOK_LE,            /* <=  */
    CTOK_GE,            /* >=  */

    /* ── Logical operators ──────────────────────────────────────── */
    CTOK_AND,           /* &&  */
    CTOK_OR,            /* ||  */
    CTOK_NOT,           /* !   */

    /* ── Bitwise operators ──────────────────────────────────────── */
    CTOK_AMP,           /* &   */
    CTOK_PIPE,          /* |   */
    CTOK_CARET,         /* ^   */
    CTOK_TILDE,         /* ~   */
    CTOK_SHL,           /* <<  */
    CTOK_SHR,           /* >>  */

    /* ── Assignment operators ───────────────────────────────────── */
    CTOK_ASSIGN,        /* =   */
    CTOK_PLUS_ASSIGN,   /* +=  */
    CTOK_MINUS_ASSIGN,  /* -=  */
    CTOK_STAR_ASSIGN,   /* *=  */
    CTOK_SLASH_ASSIGN,  /* /=  */
    CTOK_PCT_ASSIGN,    /* %=  */
    CTOK_AMP_ASSIGN,    /* &=  */
    CTOK_PIPE_ASSIGN,   /* |=  */
    CTOK_CARET_ASSIGN,  /* ^=  */
    CTOK_SHL_ASSIGN,    /* <<= */
    CTOK_SHR_ASSIGN,    /* >>= */

    /* ── Member / pointer accessors ─────────────────────────────── */
    CTOK_ARROW,         /* ->  */
    CTOK_DOT,           /* .   */

    /* ── Miscellaneous operators ─────────────────────────────────── */
    CTOK_QUESTION,      /* ?   */
    CTOK_COLON,         /* :   */

    /* ── Delimiters ─────────────────────────────────────────────── */
    CTOK_LPAREN,        /* (   */
    CTOK_RPAREN,        /* )   */
    CTOK_LBRACE,        /* {   */
    CTOK_RBRACE,        /* }   */
    CTOK_LBRACKET,      /* [   */
    CTOK_RBRACKET,      /* ]   */
    CTOK_SEMICOLON,     /* ;   */
    CTOK_COMMA,         /* ,   */
} COMP_TOK_TYPE;

/* ════════════════════════════════════════════════════════════════════════════
 *  TOKEN
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct _COMP_TOK {
    COMP_TOK_TYPE type;
    PU8  txt;       /* heap-allocated text (ident/literal raw text, NUL-terminated) */
    U32  line;      /* source line  (1-based) */
    U32  col;       /* source column (1-based) */
    union {
        U32  ival;  /* integer literal value  */
        F32  fval;  /* float literal value    */
    };
} COMP_TOK, *PCOMP_TOK;

typedef struct {
    PCOMP_TOK *toks;
    U32 len;
    U32 cap;
} COMP_TOK_ARRAY, *PCOMP_TOK_ARRAY;

/* ════════════════════════════════════════════════════════════════════════════
 *  TYPE SYSTEM
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _COMP_BASE_TYPE {
    CTYPE_NONE = 0,
    CTYPE_U8,
    CTYPE_I8,
    CTYPE_U16,
    CTYPE_I16,
    CTYPE_U32,
    CTYPE_I32,
    CTYPE_F32,
    CTYPE_U0,       /* void                       */
    CTYPE_PU8,
    CTYPE_PU16,
    CTYPE_PU32,
    CTYPE_PPU8,
    CTYPE_PPU16,
    CTYPE_PPU32,
    CTYPE_PI8,
    CTYPE_PI16,
    CTYPE_PI32,
    CTYPE_PPI8,
    CTYPE_PPI16,
    CTYPE_PPI32,
    CTYPE_BOOL,
    CTYPE_VOIDPTR,  /* untyped pointer            */
    CTYPE_STRUCT,   /* named struct (see txt)     */
    CTYPE_ENUM,     /* named enum (see txt)       */
    CTYPE_UNION,    /* named union (see txt)      */
} COMP_BASE_TYPE;

typedef struct _COMP_TYPE {
    COMP_BASE_TYPE base;
    U8  ptr_depth;  /* 0 = value; 1 = P<T>; 2 = PP<T>  */
    PU8 name;       /* struct/union/enum name, or NULL   */
} COMP_TYPE;

/* Convenience: size in bytes of a base type (ptr_depth always = 4 bytes) */
U32 COMP_TYPE_SIZE(COMP_TYPE t);

/* ════════════════════════════════════════════════════════════════════════════
 *  AST NODE TYPES
 * ════════════════════════════════════════════════════════════════════════════ */
typedef enum _CNODE_TYPE {
    /* Top-level */
    CNODE_PROGRAM,          /* Translation unit: children = declarations        */

    /* Declarations */
    CNODE_FUNC_DECL,        /* Function (name in txt, type = return type)       */
    CNODE_VAR_DECL,         /* Variable declaration (name in txt)               */
    CNODE_PARAM,            /* Single parameter (name in txt)                   */
    CNODE_STRUCT_DECL,      /* struct definition (name in txt)                  */
    CNODE_UNION_DECL,       /* union definition  (name in txt)                  */
    CNODE_ENUM_DECL,        /* enum  definition  (name in txt)                  */
    CNODE_ENUM_FIELD,       /* enumerator (name in txt, ival = value)           */

    /* Statements */
    CNODE_BLOCK,            /* { stmt* }                                        */
    CNODE_RETURN,           /* return expr?                                     */
    CNODE_IF,               /* children[0]=cond, [1]=then, [2]=else (or NULL)  */
    CNODE_FOR,              /* [0]=init, [1]=cond, [2]=incr, [3]=body          */
    CNODE_WHILE,            /* [0]=cond, [1]=body                              */
    CNODE_DO_WHILE,         /* [0]=body, [1]=cond                              */
    CNODE_BREAK,
    CNODE_CONTINUE,
    CNODE_SWITCH,           /* [0]=expr, [1..n]=case/default                   */
    CNODE_CASE,             /* [0]=value expr, [1..n]=stmts                    */
    CNODE_DEFAULT,          /* [0..n]=stmts                                    */
    CNODE_GOTO,             /* txt = label name                                */
    CNODE_LABEL,            /* txt = label name, [0]=next stmt                 */
    CNODE_EXPR_STMT,        /* [0]=expr                                        */
    CNODE_ASM_BLOCK,        /* txt = raw asm text                              */

    /* Expressions */
    CNODE_BINARY,           /* [0]=left, [1]=right; op = operator token type   */
    CNODE_UNARY,            /* [0]=operand; op = operator token type           */
    CNODE_POSTFIX,          /* [0]=operand; op = ++ or --                      */
    CNODE_TERNARY,          /* [0]=cond, [1]=then, [2]=else                    */
    CNODE_CALL,             /* [0]=callee, [1..n]=args                         */
    CNODE_INDEX,            /* [0]=array, [1]=index                            */
    CNODE_MEMBER,           /* [0]=object, txt=field name  (a.b)               */
    CNODE_ARROW_EXPR,       /* [0]=pointer, txt=field name (a->b)              */
    CNODE_ASSIGN,           /* [0]=lhs, [1]=rhs; op = assignment op type       */
    CNODE_CAST,             /* [0]=expr; dtype = target type                   */
    CNODE_SIZEOF_TYPE,      /* dtype = type being sizeof'd                     */
    CNODE_SIZEOF_EXPR,      /* [0]=expr being sizeof'd                         */
    CNODE_ADDR,             /* [0]=expr   (&a)                                 */
    CNODE_DEREF,            /* [0]=expr   (*a)                                 */

    /* Leaf nodes */
    CNODE_INT_LIT,          /* ival                                            */
    CNODE_FLOAT_LIT,        /* fval                                            */
    CNODE_STR_LIT,          /* txt (decoded string content)                    */
    CNODE_CHAR_LIT,         /* ival                                            */
    CNODE_IDENT,            /* txt = identifier name (normalised upper-case)   */
    CNODE_NULLPTR,
    CNODE_TRUE_LIT,
    CNODE_FALSE_LIT,
} CNODE_TYPE;

/* ════════════════════════════════════════════════════════════════════════════
 *  AST NODE
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct _CNODE {
    CNODE_TYPE     ntype;           /* node kind                        */
    COMP_TYPE      dtype;           /* expression / declaration type    */
    PU8            txt;             /* heap text payload (may be NULL)  */
    U32            line;
    U32            col;
    COMP_TOK_TYPE  op;              /* operator (binary/unary/assign)   */
    union {
        U32  ival;
        F32  fval;
    };
    struct _CNODE **children;       /* heap array of child pointers     */
    U32  child_count;
    U32  child_cap;
} CNODE, *PCNODE;

/* ════════════════════════════════════════════════════════════════════════════
 *  AST ARRAY  (top-level declarations)
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct {
    PCNODE *nodes;
    U32 len;
    U32 cap;
} CNODE_ARRAY, *PCNODE_ARRAY;


typedef struct {
    U32 label; // label number for the string literal
    PCNODE node; // pointer to the original CNODE_STR_LIT node containing the string literal (for reference during .rodata emission)
} RODATA_STR;

/* ════════════════════════════════════════════════════════════════════════════
 *  COMPILER CONTEXT
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct _COMP_CTX {
    PU8  tmp_src;           /* path: preprocessed .AC source temp file */
    PU8  out_asm;           /* path: output .ASM file                   */
    
    U32  label_counter;     /* auto-increment for generated labels      */

    U32  loop_label_stack[32];  /* for break/continue target labels */
    U32  loop_label_stack_top;          /* top of the loop label stack */


    RODATA_STR rodata_strings[256]; // array of string literal nodes for .rodata emission, indexed by label number
    U32 rodata_string_count; // count of string literals collected for .rodata section

    U32  errors;            /* error count                              */
    U32  warnings;          /* warning count                            */
} COMP_CTX, *PCOMP_CTX;

/* ════════════════════════════════════════════════════════════════════════════
 *  FUNCTION DECLARATIONS
 * ════════════════════════════════════════════════════════════════════════════ */

/* Entry point called by ASTRAC.c */
U32 START_COMPILER();

/* — Lexer — */
PCOMP_TOK_ARRAY COMP_LEX(PCOMP_CTX ctx);
VOID            DESTROY_COMP_TOK_ARRAY(PCOMP_TOK_ARRAY toks);

/* — Parser — */
PCNODE COMP_PARSE(PCOMP_TOK_ARRAY toks, PCOMP_CTX ctx);
VOID   DESTROY_CNODE(PCNODE node);
VOID   DESTROY_CNODE_TREE(PCNODE root);

/* — AST helpers (AST.c) — */
PCNODE    CNODE_NEW(CNODE_TYPE ntype, U32 line, U32 col);
VOID      CNODE_ADD_CHILD(PCNODE parent, PCNODE child);
PCNODE    CNODE_INT(U32 val, U32 line, U32 col);
PCNODE    CNODE_FLOAT(F32 val, U32 line, U32 col);
PCNODE    CNODE_STR(PU8 txt, U32 line, U32 col);
PCNODE    CNODE_IDENT_NODE(PU8 name, U32 line, U32 col);

/* — AST Verification (VERIFY_AST.c) — */
BOOL COMP_VERIFY(PCNODE root, PCOMP_CTX ctx);

/* — Code Generation (GEN.c) — */
BOOL COMP_GEN(PCNODE root, PCOMP_CTX ctx);

#endif /* COMPILER_H */