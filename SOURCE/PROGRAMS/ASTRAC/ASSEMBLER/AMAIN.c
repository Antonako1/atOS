#include "ASSEMBLER.h"

VOID DESTROY_ASM_INFO(PASM_INFO info) {
    for(U32 o = 0; o < info->tmp_file_count; o++) {
        MFree(info->tmp_files[0]);
    }
}
VOID DESTROY_TOK_ARR(ASM_TOK_ARRAY toks) {

}
VOID DESTROY_AST_ARR(ASM_AST_ARRAY ast) {

}

BOOL WRITE_TOKS_TO_DISK(PASM_INFO info) {

}

BOOLEAN START_ASSEMBLING() {
    PASM_INFO info;
    ASM_TOK_ARRAY tok_arr;
    ASM_AST_ARRAY ast_arr;
    info = PREPROCESS_ASM();
    if(!info) goto err;
    DEBUG_PUTS_LN("Info is not bogus");

    BOOL res = 0;
    goto cleanup;
    err:
    res = 1;
    cleanup:
    DESTROY_ASM_INFO(info);
    DESTROY_TOK_ARR(tok_arr);
    DESTROY_AST_ARR(ast_arr);
    return res;
}