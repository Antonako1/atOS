/*
 * ════════════════════════════════════════════════════════════════════════════
 *  COMPILER/VERIFY_AST.c  —  Semantic Verification Pass
 *
 *  Walks the AST produced by COMP_PARSE() and performs semantic checks:
 *   - Type compatibility for assignments / expressions
 *   - Undefined identifier detection (via a simple symbol table)
 *   - Function call arity checks
 *   - Return-type consistency
 *
 *  Current state: foundational pass — well-formed tree check,
 *  symbol table structure, and type-size annotations are implemented.
 *  Full type-inference and error-recovery are marked TODO.
 * ════════════════════════════════════════════════════════════════════════════
 */

#include "COMPILER.h"
#include "../ASTRAC.h"

/* ════════════════════════════════════════════════════════════════════════════
 *  SYMBOL TABLE  (single flat scope chain — stack of scopes)
 * ════════════════════════════════════════════════════════════════════════════ */
#define MAX_SYMS_PER_SCOPE  256
#define MAX_SCOPES          64

typedef struct {
    U8        name[128];
    COMP_TYPE dtype;
    BOOL      is_func;
    U32       param_count;   /* for functions */
} SYM_ENTRY;

typedef struct {
    SYM_ENTRY syms[MAX_SYMS_PER_SCOPE];
    U32       len;
} SCOPE;

typedef struct {
    SCOPE  scopes[MAX_SCOPES];
    U32    depth;
    COMP_TYPE current_func_ret;   /* return type of the enclosing function */
    PCOMP_CTX ctx;
} VERIFY_STATE;

static VOID scope_push(VERIFY_STATE *vs) {
    if (vs->depth < MAX_SCOPES) {
        MEMZERO(&vs->scopes[vs->depth], sizeof(SCOPE));
        vs->depth++;
    }
}

static VOID scope_pop(VERIFY_STATE *vs) {
    if (vs->depth > 0) vs->depth--;
}

static BOOL scope_define(VERIFY_STATE *vs, PU8 name, COMP_TYPE t, BOOL is_func, U32 params) {
    if (vs->depth == 0) return FALSE;
    SCOPE *s = &vs->scopes[vs->depth - 1];
    if (s->len >= MAX_SYMS_PER_SCOPE) return FALSE;
    SYM_ENTRY *e = &s->syms[s->len++];
    STRNCPY(e->name, name, 127); e->name[127] = '\0';
    e->dtype       = t;
    e->is_func     = is_func;
    e->param_count = params;
    return TRUE;
}

static SYM_ENTRY *scope_lookup(VERIFY_STATE *vs, PU8 name) {
    /* Search from innermost scope outward */
    U32 i = vs->depth;
    while (i > 0) {
        i--;
        SCOPE *s = &vs->scopes[i];
        for (U32 j = 0; j < s->len; j++)
            if (STRCMP(s->syms[j].name, name) == 0)
                return &s->syms[j];
    }
    return NULLPTR;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  VERIFY HELPERS
 * ════════════════════════════════════════════════════════════════════════════ */
#define VERR(vs, node, ...) do { \
    printf("[COMP VERIFY] %u:%u: ", (node)->line, (node)->col); \
    printf(__VA_ARGS__); printf("\n");                           \
    (vs)->ctx->errors++;                                         \
} while(0)

#define VWARN(vs, node, ...) do { \
    printf("[COMP VERIFY] %u:%u: warning: ", (node)->line, (node)->col); \
    printf(__VA_ARGS__); printf("\n");                                    \
    (vs)->ctx->warnings++;                                                \
} while(0)

/* ════════════════════════════════════════════════════════════════════════════
 *  RECURSIVE TREE WALKER
 * ════════════════════════════════════════════════════════════════════════════ */
static VOID verify_node(VERIFY_STATE *vs, PCNODE n);

static VOID verify_children(VERIFY_STATE *vs, PCNODE n) {
    for (U32 i = 0; i < n->child_count; i++)
        if (n->children[i])
            verify_node(vs, n->children[i]);
}

static VOID verify_node(VERIFY_STATE *vs, PCNODE n) {
    if (!n) return;
    DEBUGM_PRINTF("[COMP VERIFY] Verifying node type %u, txt '%s', line %u\n",
                  n->ntype, n->txt ? n->txt : "(null)", n->line);
    switch (n->ntype) {

        /* ── Program: global scope ──────────────────────────────── */
        case CNODE_PROGRAM:
            scope_push(vs);
            verify_children(vs, n);
            scope_pop(vs);
            break;

        /* ── Function declaration ───────────────────────────────── */
        case CNODE_FUNC_DECL: {
            /* Register function name in current scope */
            U32 params = 0;
            for (U32 i = 0; i < n->child_count; i++)
                if (n->children[i] && n->children[i]->ntype == CNODE_PARAM) params++;
            if (n->txt) scope_define(vs, n->txt, n->dtype, TRUE, params);

            /* New scope for body */
            COMP_TYPE saved_ret = vs->current_func_ret;
            vs->current_func_ret = n->dtype;
            scope_push(vs);

            /* Register parameters */
            for (U32 i = 0; i < n->child_count; i++) {
                PCNODE child = n->children[i];
                if (child && child->ntype == CNODE_PARAM && child->txt)
                    scope_define(vs, child->txt, child->dtype, FALSE, 0);
            }
            /* Verify body (last child that is a CNODE_BLOCK) */
            for (U32 i = 0; i < n->child_count; i++) {
                PCNODE child = n->children[i];
                if (child && child->ntype == CNODE_BLOCK)
                    verify_node(vs, child);
            }
            scope_pop(vs);
            vs->current_func_ret = saved_ret;
            break;
        }

        /* ── Variable declaration ───────────────────────────────── */
        case CNODE_VAR_DECL:
            if (n->txt) scope_define(vs, n->txt, n->dtype, FALSE, 0);
            verify_children(vs, n);
            break;

        /* ── Block: new scope ───────────────────────────────────── */
        case CNODE_BLOCK:
            scope_push(vs);
            verify_children(vs, n);
            scope_pop(vs);
            break;

        /* ── Identifier: check it is declared ──────────────────── */
        case CNODE_IDENT:
            if (n->txt) {
                SYM_ENTRY *e = scope_lookup(vs, n->txt);
                if (!e) {
                    VWARN(vs, n, "undeclared identifier '%s'", n->txt);
                } else {
                    n->dtype = e->dtype;
                }
            }
            break;

        /* ── Return: check return type ──────────────────────────── */
        case CNODE_RETURN:
            verify_children(vs, n);
            /* TODO: check that expression type is compatible with
             * vs->current_func_ret using COMP_TYPE_SIZE matching. */
            break;

        /* ── Function call ──────────────────────────────────────── */
        case CNODE_CALL:
            verify_children(vs, n);
            /* TODO: arity check using symbol table */
            break;

        /* ── Binary / unary / assign ────────────────────────────── */
        case CNODE_BINARY:
        case CNODE_UNARY:
        case CNODE_POSTFIX:
        case CNODE_ASSIGN:
            verify_children(vs, n);
            /* TODO: type-compatibility checks */
            break;

        /* ── All other nodes: just recurse ─────────────────────── */
        default:
            verify_children(vs, n);
            break;
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  PUBLIC API
 * ════════════════════════════════════════════════════════════════════════════ */
BOOL COMP_VERIFY(PCNODE root, PCOMP_CTX ctx) {
    if (!root || !ctx) return FALSE;

    DEBUGM_PRINTF("[COMP VERIFY] Starting AST verification for root node at %p\n", root);
    VERIFY_STATE vs;
    MEMZERO(&vs, sizeof(VERIFY_STATE));
    vs.ctx = ctx;
    DEBUGM_PRINTF("[COMP VERIFY] Initialized VERIFY_STATE at %p\n", &vs);

    DEBUGM_PRINTF("[COMP VERIFY] Initial symbol table depth: %u\n", vs.depth);
    verify_node(&vs, root);

    DEBUGM_PRINTF("[COMP VERIFY] Completed AST verification with %u error(s) and %u warning(s)\n",
                  ctx->errors, ctx->warnings);
    if (ctx->errors > 0) {
        printf("[COMP VERIFY] %u error(s), %u warning(s)\n", ctx->errors, ctx->warnings);
        return FALSE;
    }
    if (ctx->warnings > 0)
        printf("[COMP VERIFY] %u warning(s)\n", ctx->warnings);

    return TRUE;
}
