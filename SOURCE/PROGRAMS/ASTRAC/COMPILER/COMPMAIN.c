/*
 * ════════════════════════════════════════════════════════════════════════════
 *  COMPILER/COMPMAIN.c  —  AC Compiler Pipeline
 *
 *  Orchestrates the full C compilation pipeline:
 *
 *   1. PREPROCESS_C  — resolve #include / #define / macros (SHARED/PREPROCESS.c)
 *   2. COMP_LEX      — tokenise the preprocessed AC source   (LEX.c)
 *   3. COMP_PARSE    — build AST                             (PARSER.c)
 *   4. COMP_VERIFY   — semantic checks                       (VERIFY_AST.c)
 *   5. COMP_GEN      — emit x86 32-bit assembly              (GEN.c)
 *   6. (optional)    — hand off to the assembler if -B or BUILD mode
 *
 *  If -S (emit_asm_only) the pipeline stops after step 5.
 *  If -E (PREPROCESS_ONLY) the pipeline stops after step 1.
 * ════════════════════════════════════════════════════════════════════════════
 */

#include "COMPILER.h"
#include "../ASTRAC.h"
#include "../ASSEMBLER/ASSEMBLER.h"   /* for PASM_INFO, PREPROCESS_C, DESTROY_ASM_INFO */

/* ════════════════════════════════════════════════════════════════════════════
 *  CLEANUP HELPERS
 * ════════════════════════════════════════════════════════════════════════════ */
static VOID destroy_comp_ctx(PCOMP_CTX ctx) {
    if (!ctx) return;
    if (ctx->tmp_src) MFree(ctx->tmp_src);
    if (ctx->out_asm) MFree(ctx->out_asm);
    MFree(ctx);
}

/* ════════════════════════════════════════════════════════════════════════════
 *  START_COMPILER  —  entry point called from ASTRAC.c
 * ════════════════════════════════════════════════════════════════════════════ */
U32 START_COMPILER() {
    ASTRAC_ARGS *cfg = GET_ARGS();

    /*
     * ── Stage 1: Preprocess ────────────────────────────────────────────────
     *  Resolves #include / #define / conditionals.
     *  Produces one temp file per input file in /TMP/.
     */
    if (cfg->verbose) printf("[COMP] Stage 1/5: Preprocessing...\n");
    DEBUGM_PRINTF("[COMP] Preprocessor mode: %s\n", (cfg->build_type == PREPROCESS_ONLY) ? "PREPROCESS_ONLY" : "C_PREPROCESSOR");

    PASM_INFO pp = PREPROCESS_C();
    if (!pp || pp->tmp_file_count == 0) {
        printf("[COMP] Preprocessing failed — no output produced.\n");
        if (pp) DESTROY_ASM_INFO(pp);
        return ASTRAC_ERR_PREPROCESS;
    }
    DEBUGM_PRINTF("[COMP] Preprocessing produced %u temp file(s).\n", pp->tmp_file_count);

    /* If -E (preprocess only): copy temp files to output and stop. */
    if (cfg->build_type == PREPROCESS_ONLY) {
        if (!cfg->quiet) printf("[COMP] Preprocessing complete: %s\n", pp->tmp_files[0]);
        DESTROY_ASM_INFO(pp);
        return ASTRAC_OK;
    }
    DEBUGM_PRINTF("[COMP] Preprocessed file: %s\n", pp->tmp_files[0]);

    /*
     * Process each preprocessed file independently.
     * (AC spec: one source file per compilation unit.)
     */
    ASTRAC_RESULT result = ASTRAC_OK;

    for (U32 f = 0; f < pp->tmp_file_count; f++) {

        /* Build COMP_CTX for this file */
        PCOMP_CTX ctx = (PCOMP_CTX)MAlloc(sizeof(COMP_CTX));
        if (!ctx) {
            printf("[COMP] Out of memory\n");
            DESTROY_ASM_INFO(pp);
            return ASTRAC_ERR_INTERNAL;
        }
        MEMZERO(ctx, sizeof(COMP_CTX));
        ctx->tmp_src = STRDUP(pp->tmp_files[f]);
        ctx->out_asm = STRDUP(cfg->outfile);

        DEBUGM_PRINTF("[COMP] Processing file: %s\n", ctx->tmp_src);

        /*
         * ── Stage 2: Lex ──────────────────────────────────────────────────
         */
        if (cfg->verbose) printf("[COMP] Stage 2/5: Lexing '%s'...\n", ctx->tmp_src);

        PCOMP_TOK_ARRAY toks = COMP_LEX(ctx);
        if (!toks || toks->len == 0) {
            printf("[COMP] Lexer produced no tokens.\n");
            destroy_comp_ctx(ctx);
            result = ASTRAC_ERR_LEX;
            break;
        }
        DEBUGM_PRINTF("[COMP] Lexer produced %u token(s).\n", toks->len);

        if (cfg->verbose) printf("[COMP]   -> %u token(s)\n", toks->len);

        /*
         * ── Stage 3: Parse ────────────────────────────────────────────────
         */
        if (cfg->verbose) printf("[COMP] Stage 3/5: Parsing...\n");
        DEBUGM_PRINTF("[COMP] Parser input: %u tokens\n", toks->len);
        PCNODE ast = COMP_PARSE(toks, ctx);

        /* Tokens no longer needed after parsing */
        DESTROY_COMP_TOK_ARRAY(toks);
        toks = NULLPTR;

        DEBUGM_PRINTF("[COMP] Parser output: AST root node at %p\n", ast);

        if (!ast) {
            printf("[COMP] Parsing failed.\n");
            destroy_comp_ctx(ctx);
            result = ASTRAC_ERR_AST;
            break;
        }

        /*
         * ── Stage 4: Verify ───────────────────────────────────────────────
         */
        if (cfg->verbose) printf("[COMP] Stage 4/5: Verifying AST...\n");
        DEBUGM_PRINTF("[COMP] Verifying AST at %p\n", ast);

        if (!COMP_VERIFY(ast, ctx)) {
            printf("[COMP] Semantic verification failed.\n");
            DESTROY_CNODE_TREE(ast);
            destroy_comp_ctx(ctx);
            result = ASTRAC_ERR_VERIFY;
            break;
        }

        DEBUGM_PRINTF("[COMP] AST verification passed with %u warning(s) and %u error(s).\n", ctx->warnings, ctx->errors);
        /* Check warnings-as-errors */
        if (cfg->warnings_as_errors && ctx->warnings > 0) {
            printf("[COMP] %u warning(s) treated as error(s).\n", ctx->warnings);
            DESTROY_CNODE_TREE(ast);
            destroy_comp_ctx(ctx);
            result = ASTRAC_ERR_VERIFY;
            break;
        }

        /*
         * ── Stage 5: Code Generation ──────────────────────────────────────
         *  If -S: emit .ASM only and stop.
         *  If -B: emit .ASM then hand off to assembler (START_ASSEMBLING).
         */
        if (cfg->verbose) printf("[COMP] Stage 5/5: Generating assembly '%s'...\n", ctx->out_asm);
        DEBUGM_PRINTF("[COMP] Generating assembly to '%s' from AST at %p\n", ctx->out_asm, ast);
        if (!COMP_GEN(ast, ctx)) {
            printf("[COMP] Code generation failed.\n");
            DESTROY_CNODE_TREE(ast);
            destroy_comp_ctx(ctx);
            result = ASTRAC_ERR_CODEGEN;
            break;
        }

        DESTROY_CNODE_TREE(ast);
        ast = NULLPTR;

        if (!cfg->quiet && cfg->emit_asm_only)
            printf("[COMP] Assembly written to '%s'.\n", ctx->out_asm);
        DEBUGM_PRINTF("[COMP] Assembly generation complete for '%s'.\n", ctx->out_asm);
        destroy_comp_ctx(ctx);
    }

    DESTROY_ASM_INFO(pp);

    if (result != ASTRAC_OK) return result;

    /*
     * ── Stage 6 (BUILD mode only): Hand off to assembler ──────────────────
     *  If the user passed -B (compile + assemble), now assemble the emitted
     *  .ASM file.  We do this by calling START_ASSEMBLING which reads
     *  cfg->input_files[].  We temporarily swap in the .ASM output as input.
     *  NOTE: START_ASSEMBLING will call PREPROCESS_ASM on cfg->input_files[],
     *  so we update the first input file to point to the generated .ASM.
     */
    if ((cfg->build_type & ASSEMBLE) && !cfg->emit_asm_only) {
        if (cfg->verbose) printf("[COMP] Handing off to assembler...\n");
        /* Point the assembler at the freshly generated .ASM */
        cfg->input_files[0] = cfg->outfile;
        cfg->input_file_count = 1;
        /* Derive a .BIN output name */
        PU8 bin_out = STRDUP(cfg->outfile);
        PU8 dot = STRRCHR(bin_out, '.');
        if (dot) *dot = '\0';
        STRCAT(bin_out, ".BIN");
        cfg->outfile = bin_out;
        cfg->got_outfile = TRUE;
        return (U32)START_ASSEMBLING();
    }

    if (!cfg->quiet) printf("[COMP] Compilation complete.\n");
    return ASTRAC_OK;
}
