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

typedef struct {
    U32 type;
    PU8 txt;
} ASM_TOK, *PASM_TOK;

typedef struct {
    U32 len;
    PASM_TOK* toks;
} ASM_TOK_ARRAY;

typedef struct {
    U32 type;
    PU8 txt;

    struct ASM_SYNT *next;
} ASM_SYNT, *PASM_SYNT;

typedef struct {
    U32 len;
    PASM_SYNT* toks;
} ASM_AST_ARRAY;

BOOLEAN START_ASSEMBLING();

BOOL WRITE_PASM_INFO(PASM_INFO info);
VOID DESTROY_ASM_INFO(PASM_INFO info);

PASM_INFO PREPROCESS_ASM();

ASM_TOK_ARRAY LEX(PASM_INFO info);
VOID DESTROY_TOK_ARR(ASM_TOK_ARRAY toks);

ASM_AST_ARRAY ASM_AST(ASM_TOK_ARRAY toks, PASM_INFO info);
VOID DESTROY_AST_ARR(ASM_AST_ARRAY ast);

BOOLEAN VERIFY_AST(ASM_AST_ARRAY ast, PASM_INFO info);
BOOLEAN OPTIMIZE(ASM_AST_ARRAY ast, PASM_INFO info);
BOOLEAN GEN_BINARY(ASM_AST_ARRAY ast, PASM_INFO info);

#endif // ASSEMBLER_H