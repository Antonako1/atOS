#include "ASSEMBLER.h"

VOID DESTROY_ASM_INFO(PASM_INFO info) {
    for(U32 o = 0; o < info->tmp_file_count; o++) {
        MFree(info->tmp_files[0]);
    }
}
VOID DESTROY_TOK_ARR(ASM_TOK_ARRAY *toks) {
    for(U32 i = 0; i < toks->len; i++) {
        PASM_TOK tok = toks->toks[i];
        if(tok->txt) MFree(tok->txt);
    }
    MFree(toks);
}
VOID DESTROY_AST_ARR(ASM_AST_ARRAY *ast) {

}

BOOL WRITE_TOKS_TO_DISK(PASM_INFO info) {

}

BOOLEAN START_ASSEMBLING() {
    PASM_INFO info;
    ASM_TOK_ARRAY *tok_arr;
    ASM_AST_ARRAY *ast_arr;
    info = PREPROCESS_ASM();
    if(info->tmp_file_count == 0) {
        printf("[ASM PP] No tmp files to continue with\n");
        goto err;
    }
    tok_arr = LEX(info);
    if(!tok_arr) {
        printf("[ASM LEX] Nothing lexed\n");
        goto err;
    }

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