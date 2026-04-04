#include "ASSEMBLER.h"

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  CLEANUP HELPERS
 * ════════════════════════════════════════════════════════════════════════════
 */

VOID DESTROY_ASM_INFO(PASM_INFO info) {
    if (!info) return;
    for (U32 i = 0; i < info->tmp_file_count; i++) {
        MFree(info->tmp_files[i]);
    }
    MFree(info);
}

VOID DESTROY_TOK_ARR(ASM_TOK_ARRAY *toks) {
    if (!toks) return;
    for (U32 i = 0; i < toks->len; i++) {
        PASM_TOK tok = toks->toks[i];
        if (tok && tok->txt) MFree(tok->txt);
        if (tok) MFree(tok);
    }
    MFree(toks);
}

/* DESTROY_AST_ARR is implemented in AST.c (deep-frees all nodes) */

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  DEBUG: DUMP TOKENS TO DISK
 * ════════════════════════════════════════════════════════════════════════════
 */

BOOL WRITE_TOKS_TO_DISK(PASM_INFO info, ASM_TOK_ARRAY *toks) {
    FILE *file = FOPEN("/TMP/ASM_TOK_DUMP.TXT", MODE_W);
    if (!file) return FALSE;

    U32 char_buf_sz = sizeof(U8) * 30;
    U32 sz  = (toks->len * sizeof(ASM_TOK)) + (toks->len * char_buf_sz);
    VOIDPTR buf = MAlloc(sz);
    if (!buf) return FALSE;

    U32 ptr = 0;
    for (U32 i = 0; i < toks->len; i++) {
        U32 sz2 = 0;
        MEMCPY_OPT(buf + ptr, toks->toks[i], sz2);
        ptr += sizeof(ASM_TOK) + char_buf_sz;
    }

    FWRITE(file, buf, sz);
    MFree(buf);
    return TRUE;
}

/*
 * ════════════════════════════════════════════════════════════════════════════
 *  ASSEMBLER PIPELINE
 * ════════════════════════════════════════════════════════════════════════════
 *
 *  The assembler runs these stages in order:
 *
 *      1. PREPROCESS  — resolve #include / #define / #ifdef, produce tmp files
 *      2. LEX         — tokenise the preprocessed source
 *      3. AST         — build an abstract syntax tree from the token stream
 *      4. VERIFY      — semantic checks on the AST  (TODO)
 *      5. OPTIMIZE    — peephole / constant-fold     (TODO)
 *      6. GEN_BINARY  — emit machine code             (TODO)
 *
 *  Each stage returns NULL / FALSE on failure, causing an early exit with
 *  the appropriate ASTRAC_ERR_* code.
 */

ASTRAC_RESULT START_ASSEMBLING() {
    ASTRAC_ARGS  *cfg     = GET_ARGS();
    PASM_INFO     info    = NULLPTR;
    ASM_TOK_ARRAY *tok_arr = NULLPTR;
    ASM_AST_ARRAY *ast_arr = NULLPTR;
    ASTRAC_RESULT  result  = ASTRAC_OK;

    /* ── Stage 1: Preprocess ────────────────────────────────────── */
    if (cfg->verbose) printf("[ASM] Stage 1/6: Preprocessing...\n");

    info = PREPROCESS_ASM();
    if (!info || info->tmp_file_count == 0) {
        printf("[ASM] Preprocessing failed — no output files produced.\n");
        result = ASTRAC_ERR_PREPROCESS;
        goto cleanup;
    }

    /* ── Stage 2: Lex ───────────────────────────────────────────── */
    if (cfg->verbose) printf("[ASM] Stage 2/6: Lexing...\n");

    tok_arr = LEX(info);
    if (!tok_arr || tok_arr->len == 0) {
        printf("[ASM] Lexer produced no tokens.\n");
        result = ASTRAC_ERR_LEX;
        goto cleanup;
    }

    if (cfg->verbose) printf("[ASM]   -> %u tokens\n", tok_arr->len);

    /* ── Stage 3: AST ───────────────────────────────────────────── */
    if (cfg->verbose) printf("[ASM] Stage 3/6: Building AST...\n");

    ast_arr = ASM_BUILD_AST(tok_arr);
    if (!ast_arr) {
        printf("[ASM] AST construction failed.\n");
        result = ASTRAC_ERR_AST;
        goto cleanup;
    }

    if (cfg->verbose) printf("[ASM]   -> %u nodes\n", ast_arr->len);

    /* ── Stage 4: Verify  ─────────────────────────────────── */
    if (cfg->verbose) printf("[ASM] Stage 4/6: Verifying AST...\n");
    if (!VERIFY_AST(ast_arr, info)) {
        printf("[ASM] AST verification failed.\n");
        result = ASTRAC_ERR_VERIFY;
        goto cleanup;
    }

    /* ── Stage 5: Optimize (TODO) ───────────────────────────────── */
    if (cfg->verbose) printf("[ASM] Stage 5/6: Optimizing... (stub)\n");
    /*
    if (!OPTIMIZE(ast_arr, info)) {
        printf("[ASM] Optimization pass failed.\n");
        result = ASTRAC_ERR_OPTIMIZE;
        goto cleanup;
    }
    */

    /* ── Stage 6: Generate binary ────────────────────────── */
    if (cfg->verbose) printf("[ASM] Stage 6/6: Generating binary...\n");
    if (!GEN_BINARY(ast_arr, info)) {
        printf("[ASM] Code generation failed.\n");
        result = ASTRAC_ERR_CODEGEN;
        goto cleanup;
    }

    if (!cfg->quiet) printf("[ASM] Assembly complete.\n");

cleanup:
    DESTROY_AST_ARR(ast_arr);
    DESTROY_TOK_ARR(tok_arr);
    DESTROY_ASM_INFO(info);
    return result;
}