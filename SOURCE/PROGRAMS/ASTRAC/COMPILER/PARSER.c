/*
 * ════════════════════════════════════════════════════════════════════════════
 *  COMPILER/PARSER.c  —  AC Language Recursive-Descent Parser
 *
 *  Consumes a COMP_TOK_ARRAY (produced by LEX.c) and returns a PCNODE tree
 *  rooted at CNODE_PROGRAM.
 *
 *  Grammar overview (simplified):
 *
 *    program      ::= (decl)*
 *    decl         ::= func_decl | var_decl | struct_decl | enum_decl
 *    func_decl    ::= type IDENT '(' params ')' block
 *    var_decl     ::= type IDENT ['=' expr] ';'
 *    block        ::= '{' (var_decl)* (stmt)* '}'
 *    stmt         ::= block | if_stmt | for_stmt | while_stmt |
 *                     do_stmt | switch_stmt | return_stmt |
 *                     break_stmt | continue_stmt | goto_stmt |
 *                     label_stmt | asm_block | expr_stmt
 *    expr         ::= assignment_expr (',' assignment_expr)*
 *    ...          (standard C precedence levels below)
 * ════════════════════════════════════════════════════════════════════════════
 */

#include "COMPILER.h"
#include "../ASTRAC.h"

/* ════════════════════════════════════════════════════════════════════════════
 *  PARSER STATE
 * ════════════════════════════════════════════════════════════════════════════ */
typedef struct {
    PCOMP_TOK_ARRAY toks;
    U32             pos;
    PCOMP_CTX       ctx;
} PSTATE;

/* ── tok / advance helpers ─────────────────────────────────────────────────── */

static PCOMP_TOK ps_peek(PSTATE *p) {
    if (p->pos >= p->toks->len) return p->toks->toks[p->toks->len - 1]; /* EOF */
    return p->toks->toks[p->pos];
}

static PCOMP_TOK ps_peek2(PSTATE *p) {
    U32 next = p->pos + 1;
    if (next >= p->toks->len) return p->toks->toks[p->toks->len - 1];
    return p->toks->toks[next];
}

static PCOMP_TOK ps_advance(PSTATE *p) {
    PCOMP_TOK t = ps_peek(p);
    if (t->type != CTOK_EOF) p->pos++;
    return t;
}

static BOOL ps_check(PSTATE *p, COMP_TOK_TYPE t) {
    return ps_peek(p)->type == t;
}

static BOOL ps_match(PSTATE *p, COMP_TOK_TYPE t) {
    if (!ps_check(p, t)) return FALSE;
    ps_advance(p);
    return TRUE;
}

/* eat: consume expected token or emit error and return FALSE */
static BOOL ps_eat(PSTATE *p, COMP_TOK_TYPE t, PU8 ctx_msg) {
    if (ps_check(p, t)) { ps_advance(p); return TRUE; }
    PCOMP_TOK got = ps_peek(p);
    printf("[COMP PARSE] %u:%u: expected %s, got '%s'\n",
           got->line, got->col, ctx_msg, got->txt ? got->txt : "?");
    p->ctx->errors++;
    return FALSE;
}

/* Synchronize to next ';' or '}' on parse error */
static VOID ps_sync(PSTATE *p) {
    while (!ps_check(p, CTOK_EOF)) {
        COMP_TOK_TYPE t = ps_peek(p)->type;
        if (t == CTOK_SEMICOLON || t == CTOK_RBRACE) { ps_advance(p); return; }
        ps_advance(p);
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  TYPE PARSER
 * ════════════════════════════════════════════════════════════════════════════ */

/* Returns TRUE if the current token can begin a type specifier */
static BOOL tok_is_type(PSTATE *p) {
    COMP_TOK_TYPE t = ps_peek(p)->type;
    switch (t) {
        case CTOK_KW_U8: case CTOK_KW_I8:
        case CTOK_KW_U16: case CTOK_KW_I16:
        case CTOK_KW_U32: case CTOK_KW_I32:
        case CTOK_KW_F32:
        case CTOK_KW_U0:
        case CTOK_KW_BOOL:
        case CTOK_KW_VOIDPTR:
        case CTOK_KW_STRUCT: case CTOK_KW_UNION: case CTOK_KW_ENUM:
            return TRUE;
        case CTOK_IDENT: {
            /* P<TYPE> or PP<TYPE> pointer shorthand — ident starts with 'P' */
            PU8 txt = ps_peek(p)->txt;
            if (txt && txt[0] == 'P') return TRUE;
            return FALSE;
        }
        default: return FALSE;
    }
}

/*
 * Resolve a P<TYPE> / PP<TYPE> identifier to a COMP_TYPE.
 * E.g. "PU8" -> {CTYPE_U8, 1}, "PPU16" -> {CTYPE_U16, 2}, "PPU32" -> ...
 * Returns FALSE if the identifier doesn't match this pattern.
 */
static BOOL try_resolve_ptr_ident(PU8 txt, COMP_TYPE *out) {
    if (!txt || txt[0] != 'P') return FALSE;

    U8 depth = 0;
    U32 i = 0;
    while (txt[i] == 'P') { depth++; i++; }

    PU8 base = txt + i;
    COMP_BASE_TYPE bt = CTYPE_NONE;

    if      (STRCMP(base, "U8")      == 0) bt = CTYPE_U8;
    else if (STRCMP(base, "I8")      == 0) bt = CTYPE_I8;
    else if (STRCMP(base, "U16")     == 0) bt = CTYPE_U16;
    else if (STRCMP(base, "I16")     == 0) bt = CTYPE_I16;
    else if (STRCMP(base, "U32")     == 0) bt = CTYPE_U32;
    else if (STRCMP(base, "I32")     == 0) bt = CTYPE_I32;
    else if (STRCMP(base, "F32")     == 0) bt = CTYPE_F32;
    else if (STRCMP(base, "BOOL")    == 0) bt = CTYPE_BOOL;
    else if (STRCMP(base, "VOIDPTR") == 0) bt = CTYPE_VOIDPTR;
    else if (STRCMP(base, "U0")      == 0) bt = CTYPE_U0;
    else return FALSE;

    out->base      = bt;
    out->ptr_depth = depth;
    out->name      = NULLPTR;
    return TRUE;
}

/* parse_type: fill *out with the type at the current token.
 * Returns TRUE on success and advances past the type tokens. */
static BOOL parse_type(PSTATE *p, COMP_TYPE *out) {
    MEMZERO(out, sizeof(COMP_TYPE));
    PCOMP_TOK tok = ps_peek(p);

    switch (tok->type) {
        case CTOK_KW_U8:      ps_advance(p); out->base = CTYPE_U8;      return TRUE;
        case CTOK_KW_I8:      ps_advance(p); out->base = CTYPE_I8;      return TRUE;
        case CTOK_KW_U16:     ps_advance(p); out->base = CTYPE_U16;     return TRUE;
        case CTOK_KW_I16:     ps_advance(p); out->base = CTYPE_I16;     return TRUE;
        case CTOK_KW_U32:     ps_advance(p); out->base = CTYPE_U32;     return TRUE;
        case CTOK_KW_I32:     ps_advance(p); out->base = CTYPE_I32;     return TRUE;
        case CTOK_KW_F32:     ps_advance(p); out->base = CTYPE_F32;     return TRUE;
        case CTOK_KW_U0:      ps_advance(p); out->base = CTYPE_U0;      return TRUE;
        case CTOK_KW_BOOL:    ps_advance(p); out->base = CTYPE_BOOL;    return TRUE;
        case CTOK_KW_VOIDPTR: ps_advance(p); out->base = CTYPE_VOIDPTR; return TRUE;
        case CTOK_KW_PU8:     ps_advance(p); out->base = CTYPE_PU8;     return TRUE;
        case CTOK_KW_PU16:    ps_advance(p); out->base = CTYPE_PU16;    return TRUE;
        case CTOK_KW_PU32:    ps_advance(p); out->base = CTYPE_PU32;    return TRUE;
        case CTOK_KW_PPU8:    ps_advance(p); out->base = CTYPE_PPU8;    return TRUE;
        case CTOK_KW_PPU16:   ps_advance(p); out->base = CTYPE_PPU16;   return TRUE;
        case CTOK_KW_PPU32:   ps_advance(p); out->base = CTYPE_PPU32;   return TRUE;
        case CTOK_KW_PI8:     ps_advance(p); out->base = CTYPE_PI8;     return TRUE;
        case CTOK_KW_PI16:    ps_advance(p); out->base = CTYPE_PI16;    return TRUE;
        case CTOK_KW_PI32:    ps_advance(p); out->base = CTYPE_PI32;    return TRUE;
        case CTOK_KW_PPI8:    ps_advance(p); out->base = CTYPE_PPI8;    return TRUE;
        case CTOK_KW_PPI16:   ps_advance(p); out->base = CTYPE_PPI16;   return TRUE;
        case CTOK_KW_PPI32:   ps_advance(p); out->base = CTYPE_PPI32;   return TRUE;
        case CTOK_KW_STRUCT:
        case CTOK_KW_UNION:
        case CTOK_KW_ENUM: {
            COMP_BASE_TYPE bt = (tok->type == CTOK_KW_STRUCT) ? CTYPE_STRUCT
                              : (tok->type == CTOK_KW_UNION)  ? CTYPE_UNION
                                                               : CTYPE_ENUM;
            ps_advance(p);
            PCOMP_TOK name_tok = ps_peek(p);
            if (name_tok->type != CTOK_IDENT) {
                printf("[COMP PARSE] %u:%u: expected struct/union/enum name\n",
                       name_tok->line, name_tok->col);
                p->ctx->errors++;
                return FALSE;
            }
            out->base = bt;
            out->name = STRDUP(name_tok->txt);
            ps_advance(p);
            return TRUE;
        }

        case CTOK_IDENT: {
            COMP_TYPE resolved;
            if (try_resolve_ptr_ident(tok->txt, &resolved)) {
                *out = resolved;
                ps_advance(p);
                return TRUE;
            }
            /* Could be a typedef name — accept and mark as struct placeholder */
            out->base = CTYPE_STRUCT;
            out->name = STRDUP(tok->txt);
            ps_advance(p);
            return TRUE;
        }

        default:
            printf("[COMP PARSE] %u:%u: expected type specifier, got '%s'\n",
                   tok->line, tok->col, tok->txt ? tok->txt : "?");
            p->ctx->errors++;
            return FALSE;
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  FORWARD DECLARATIONS
 * ════════════════════════════════════════════════════════════════════════════ */
static PCNODE parse_expr(PSTATE *p);
static PCNODE parse_stmt(PSTATE *p);
static PCNODE parse_block(PSTATE *p);
static PCNODE parse_var_decl(PSTATE *p);

/* ════════════════════════════════════════════════════════════════════════════
 *  EXPRESSION PARSER  (precedence: lowest -> highest)
 * ════════════════════════════════════════════════════════════════════════════ */

/* ── primary ───────────────────────────────────────────────────────────────── */
static PCNODE parse_primary(PSTATE *p) {
    PCOMP_TOK tok = ps_peek(p);

    /* Integer literal */
    if (tok->type == CTOK_INT_LIT) {
        ps_advance(p);
        return CNODE_INT(tok->ival, tok->line, tok->col);
    }

    /* Float literal */
    if (tok->type == CTOK_FLOAT_LIT) {
        ps_advance(p);
        return CNODE_FLOAT(tok->fval, tok->line, tok->col);
    }

    /* String literal */
    if (tok->type == CTOK_STR_LIT) {
        ps_advance(p);
        return CNODE_STR(tok->txt, tok->line, tok->col);
    }

    /* Char literal */
    if (tok->type == CTOK_CHAR_LIT) {
        ps_advance(p);
        PCNODE n = CNODE_NEW(CNODE_CHAR_LIT, tok->line, tok->col);
        if (!n) return NULLPTR;
        n->ival = tok->ival;
        n->dtype.base = CTYPE_U8;
        return n;
    }

    /* TRUE / FALSE */
    if (tok->type == CTOK_KW_TRUE) {
        ps_advance(p);
        PCNODE n = CNODE_NEW(CNODE_TRUE_LIT, tok->line, tok->col);
        if (!n) return NULLPTR;
        n->ival = 1; n->dtype.base = CTYPE_BOOL;
        return n;
    }
    if (tok->type == CTOK_KW_FALSE) {
        ps_advance(p);
        PCNODE n = CNODE_NEW(CNODE_FALSE_LIT, tok->line, tok->col);
        if (!n) return NULLPTR;
        n->ival = 0; n->dtype.base = CTYPE_BOOL;
        return n;
    }

    /* NULLPTR */
    if (tok->type == CTOK_KW_NULLPTR) {
        ps_advance(p);
        PCNODE n = CNODE_NEW(CNODE_NULLPTR, tok->line, tok->col);
        if (!n) return NULLPTR;
        n->dtype.base = CTYPE_VOIDPTR;
        return n;
    }

    /* Identifier */
    if (tok->type == CTOK_IDENT) {
        ps_advance(p);
        return CNODE_IDENT_NODE(tok->txt, tok->line, tok->col);
    }

    /* sizeof(type) or sizeof(expr) */
    if (tok->type == CTOK_KW_SIZEOF) {
        U32 line = tok->line, col = tok->col;
        ps_advance(p);
        if (!ps_eat(p, CTOK_LPAREN, "'(' after sizeof")) return NULLPTR;

        if (tok_is_type(p)) {
            COMP_TYPE ct;
            if (!parse_type(p, &ct)) return NULLPTR;
            if (!ps_eat(p, CTOK_RPAREN, "')' after sizeof type")) return NULLPTR;
            PCNODE n = CNODE_NEW(CNODE_SIZEOF_TYPE, line, col);
            if (!n) return NULLPTR;
            n->dtype = ct;
            return n;
        } else {
            PCNODE inner = parse_expr(p);
            if (!inner) return NULLPTR;
            if (!ps_eat(p, CTOK_RPAREN, "')' after sizeof expr")) {
                DESTROY_CNODE(inner); return NULLPTR;
            }
            PCNODE n = CNODE_NEW(CNODE_SIZEOF_EXPR, line, col);
            if (!n) { DESTROY_CNODE(inner); return NULLPTR; }
            CNODE_ADD_CHILD(n, inner);
            return n;
        }
    }

    /* Parenthesised expression or cast: '(' type ')' expr */
    if (tok->type == CTOK_LPAREN) {
        ps_advance(p);
        /* Peek ahead: if this looks like a cast "(type)" ... */
        if (tok_is_type(p)) {
            COMP_TYPE ct;
            U32 saved = p->pos;
            if (parse_type(p, &ct) && ps_check(p, CTOK_RPAREN)) {
                ps_advance(p);  /* consume ')' */
                PCNODE inner = parse_primary(p);  /* only unary priority for casts */
                if (!inner) return NULLPTR;
                PCNODE n = CNODE_NEW(CNODE_CAST, tok->line, tok->col);
                if (!n) { DESTROY_CNODE(inner); return NULLPTR; }
                n->dtype = ct;
                CNODE_ADD_CHILD(n, inner);
                return n;
            }
            /* Not a cast — restore and parse as grouped expr */
            p->pos = saved;
            if (ct.name) MFree(ct.name);
        }
        PCNODE inner = parse_expr(p);
        if (!inner) return NULLPTR;
        if (!ps_eat(p, CTOK_RPAREN, "')' in expression")) {
            DESTROY_CNODE(inner); return NULLPTR;
        }
        return inner;
    }

    printf("[COMP PARSE] %u:%u: unexpected token '%s' in expression\n",
           tok->line, tok->col, tok->txt ? tok->txt : "?");
    p->ctx->errors++;
    return NULLPTR;
}

/* ── postfix: call, index, member, arrow, ++/-- ────────────────────────────── */
static PCNODE parse_postfix(PSTATE *p) {
    PCNODE left = parse_primary(p);
    if (!left) return NULLPTR;

    for (;;) {
        PCOMP_TOK tok = ps_peek(p);

        /* Function call: a( args ) */
        if (tok->type == CTOK_LPAREN) {
            ps_advance(p);
            PCNODE call = CNODE_NEW(CNODE_CALL, tok->line, tok->col);
            if (!call) { DESTROY_CNODE(left); return NULLPTR; }
            CNODE_ADD_CHILD(call, left);

            if (!ps_check(p, CTOK_RPAREN)) {
                do {
                    PCNODE arg = parse_expr(p);
                    if (!arg) { DESTROY_CNODE(call); return NULLPTR; }
                    CNODE_ADD_CHILD(call, arg);
                } while (ps_match(p, CTOK_COMMA));
            }
            if (!ps_eat(p, CTOK_RPAREN, "')' in function call")) {
                DESTROY_CNODE(call); return NULLPTR;
            }
            left = call;
            continue;
        }

        /* Index: a[n] */
        if (tok->type == CTOK_LBRACKET) {
            ps_advance(p);
            PCNODE idx = parse_expr(p);
            if (!idx) { DESTROY_CNODE(left); return NULLPTR; }
            if (!ps_eat(p, CTOK_RBRACKET, "']'")) { DESTROY_CNODE(left); DESTROY_CNODE(idx); return NULLPTR; }
            PCNODE n = CNODE_NEW(CNODE_INDEX, tok->line, tok->col);
            if (!n) { DESTROY_CNODE(left); DESTROY_CNODE(idx); return NULLPTR; }
            CNODE_ADD_CHILD(n, left); CNODE_ADD_CHILD(n, idx);
            left = n; continue;
        }

        /* Member a.b */
        if (tok->type == CTOK_DOT) {
            ps_advance(p);
            PCOMP_TOK field = ps_peek(p);
            if (field->type != CTOK_IDENT) {
                printf("[COMP PARSE] %u:%u: expected field name after '.'\n", field->line, field->col);
                p->ctx->errors++; DESTROY_CNODE(left); return NULLPTR;
            }
            ps_advance(p);
            PCNODE n = CNODE_NEW(CNODE_MEMBER, tok->line, tok->col);
            if (!n) { DESTROY_CNODE(left); return NULLPTR; }
            n->txt = STRDUP(field->txt);
            CNODE_ADD_CHILD(n, left);
            left = n; continue;
        }

        /* Arrow a->b */
        if (tok->type == CTOK_ARROW) {
            ps_advance(p);
            PCOMP_TOK field = ps_peek(p);
            if (field->type != CTOK_IDENT) {
                printf("[COMP PARSE] %u:%u: expected field name after '->'\n", field->line, field->col);
                p->ctx->errors++; DESTROY_CNODE(left); return NULLPTR;
            }
            ps_advance(p);
            PCNODE n = CNODE_NEW(CNODE_ARROW_EXPR, tok->line, tok->col);
            if (!n) { DESTROY_CNODE(left); return NULLPTR; }
            n->txt = STRDUP(field->txt);
            CNODE_ADD_CHILD(n, left);
            left = n; continue;
        }

        /* Postfix ++ / -- */
        if (tok->type == CTOK_PLUSPLUS || tok->type == CTOK_MINUSMINUS) {
            ps_advance(p);
            PCNODE n = CNODE_NEW(CNODE_POSTFIX, tok->line, tok->col);
            if (!n) { DESTROY_CNODE(left); return NULLPTR; }
            n->op = tok->type;
            CNODE_ADD_CHILD(n, left);
            left = n; continue;
        }

        break;
    }
    return left;
}

/* ── unary ──────────────────────────────────────────────────────────────────── */
static PCNODE parse_unary(PSTATE *p) {
    PCOMP_TOK tok = ps_peek(p);

    /* Prefix ++ / -- */
    if (tok->type == CTOK_PLUSPLUS || tok->type == CTOK_MINUSMINUS) {
        ps_advance(p);
        PCNODE operand = parse_unary(p);
        if (!operand) return NULLPTR;
        PCNODE n = CNODE_NEW(CNODE_UNARY, tok->line, tok->col);
        if (!n) { DESTROY_CNODE(operand); return NULLPTR; }
        n->op = tok->type; CNODE_ADD_CHILD(n, operand);
        return n;
    }

    /* Unary: + - ! ~ * & */
    if (tok->type == CTOK_PLUS  || tok->type == CTOK_MINUS  ||
        tok->type == CTOK_NOT   || tok->type == CTOK_TILDE) {
        ps_advance(p);
        PCNODE operand = parse_unary(p);
        if (!operand) return NULLPTR;
        PCNODE n = CNODE_NEW(CNODE_UNARY, tok->line, tok->col);
        if (!n) { DESTROY_CNODE(operand); return NULLPTR; }
        n->op = tok->type; CNODE_ADD_CHILD(n, operand);
        return n;
    }

    /* Dereference * */
    if (tok->type == CTOK_STAR) {
        ps_advance(p);
        PCNODE operand = parse_unary(p);
        if (!operand) return NULLPTR;
        PCNODE n = CNODE_NEW(CNODE_DEREF, tok->line, tok->col);
        if (!n) { DESTROY_CNODE(operand); return NULLPTR; }
        CNODE_ADD_CHILD(n, operand); return n;
    }

    /* Address-of & */
    if (tok->type == CTOK_AMP) {
        ps_advance(p);
        PCNODE operand = parse_unary(p);
        if (!operand) return NULLPTR;
        PCNODE n = CNODE_NEW(CNODE_ADDR, tok->line, tok->col);
        if (!n) { DESTROY_CNODE(operand); return NULLPTR; }
        CNODE_ADD_CHILD(n, operand); return n;
    }

    return parse_postfix(p);
}

/* ── binary expressions: standard C precedence levels ──────────────────────── */

#define BINARY_LEVEL(name, next, ...)                                           \
static PCNODE name(PSTATE *p) {                                                 \
    PCNODE left = next(p);                                                      \
    if (!left) return NULLPTR;                                                  \
    for (;;) {                                                                  \
        PCOMP_TOK tok = ps_peek(p);                                             \
        COMP_TOK_TYPE t = tok->type;                                            \
        BOOL match = FALSE;                                                     \
        COMP_TOK_TYPE ops[] = { __VA_ARGS__ };                                  \
        U32 nops = sizeof(ops) / sizeof(ops[0]);                                \
        for (U32 k = 0; k < nops; k++) if (t == ops[k]) { match = TRUE; break; } \
        if (!match) break;                                                      \
        ps_advance(p);                                                          \
        PCNODE right = next(p);                                                 \
        if (!right) { DESTROY_CNODE(left); return NULLPTR; }                   \
        PCNODE n = CNODE_NEW(CNODE_BINARY, tok->line, tok->col);                \
        if (!n) { DESTROY_CNODE(left); DESTROY_CNODE(right); return NULLPTR; } \
        n->op = t;                                                              \
        CNODE_ADD_CHILD(n, left); CNODE_ADD_CHILD(n, right);                   \
        left = n;                                                               \
    }                                                                           \
    return left;                                                                \
}

BINARY_LEVEL(parse_mul,    parse_unary,   CTOK_STAR, CTOK_SLASH, CTOK_PERCENT)
BINARY_LEVEL(parse_add,    parse_mul,     CTOK_PLUS, CTOK_MINUS)
BINARY_LEVEL(parse_shift,  parse_add,     CTOK_SHL, CTOK_SHR)
BINARY_LEVEL(parse_rel,    parse_shift,   CTOK_LT, CTOK_GT, CTOK_LE, CTOK_GE)
BINARY_LEVEL(parse_eq,     parse_rel,     CTOK_EQ, CTOK_NEQ)
BINARY_LEVEL(parse_bitand, parse_eq,      CTOK_AMP)
BINARY_LEVEL(parse_bitxor, parse_bitand,  CTOK_CARET)
BINARY_LEVEL(parse_bitor,  parse_bitxor,  CTOK_PIPE)
BINARY_LEVEL(parse_logand, parse_bitor,   CTOK_AND)
BINARY_LEVEL(parse_logor,  parse_logand,  CTOK_OR)

/* ── ternary: cond ? then : else ────────────────────────────────────────────── */
static PCNODE parse_ternary(PSTATE *p) {
    PCNODE cond = parse_logor(p);
    if (!cond) return NULLPTR;

    if (!ps_match(p, CTOK_QUESTION)) return cond;

    PCNODE then_e = parse_expr(p);
    if (!then_e) { DESTROY_CNODE(cond); return NULLPTR; }
    if (!ps_eat(p, CTOK_COLON, "':' in ternary")) { DESTROY_CNODE(cond); DESTROY_CNODE(then_e); return NULLPTR; }
    PCNODE else_e = parse_ternary(p);
    if (!else_e) { DESTROY_CNODE(cond); DESTROY_CNODE(then_e); return NULLPTR; }

    PCNODE n = CNODE_NEW(CNODE_TERNARY, cond->line, cond->col);
    if (!n) { DESTROY_CNODE(cond); DESTROY_CNODE(then_e); DESTROY_CNODE(else_e); return NULLPTR; }
    CNODE_ADD_CHILD(n, cond); CNODE_ADD_CHILD(n, then_e); CNODE_ADD_CHILD(n, else_e);
    return n;
}

/* ── assignment ─────────────────────────────────────────────────────────────── */
static BOOL is_assign_op(COMP_TOK_TYPE t) {
    return t == CTOK_ASSIGN     || t == CTOK_PLUS_ASSIGN  || t == CTOK_MINUS_ASSIGN ||
           t == CTOK_STAR_ASSIGN|| t == CTOK_SLASH_ASSIGN || t == CTOK_PCT_ASSIGN   ||
           t == CTOK_AMP_ASSIGN || t == CTOK_PIPE_ASSIGN  || t == CTOK_CARET_ASSIGN ||
           t == CTOK_SHL_ASSIGN || t == CTOK_SHR_ASSIGN;
}

static PCNODE parse_assign(PSTATE *p) {
    PCNODE left = parse_ternary(p);
    if (!left) return NULLPTR;

    PCOMP_TOK tok = ps_peek(p);
    if (is_assign_op(tok->type)) {
        ps_advance(p);
        PCNODE right = parse_assign(p);   /* right-associative */
        if (!right) { DESTROY_CNODE(left); return NULLPTR; }
        PCNODE n = CNODE_NEW(CNODE_ASSIGN, tok->line, tok->col);
        if (!n) { DESTROY_CNODE(left); DESTROY_CNODE(right); return NULLPTR; }
        n->op = tok->type;
        CNODE_ADD_CHILD(n, left); CNODE_ADD_CHILD(n, right);
        return n;
    }
    return left;
}

/* ── top-level expression (comma-separated in some contexts) ────────────────── */
static PCNODE parse_expr(PSTATE *p) {
    return parse_assign(p);
}

/* ════════════════════════════════════════════════════════════════════════════
 *  STATEMENT PARSER
 * ════════════════════════════════════════════════════════════════════════════ */

static PCNODE parse_if(PSTATE *p) {
    PCOMP_TOK tok = ps_advance(p);   /* consume 'IF' */
    if (!ps_eat(p, CTOK_LPAREN, "'(' after if")) return NULLPTR;
    PCNODE cond = parse_expr(p);
    if (!cond) return NULLPTR;
    if (!ps_eat(p, CTOK_RPAREN, "')' after if condition")) { DESTROY_CNODE(cond); return NULLPTR; }
    PCNODE then_s = parse_stmt(p);
    if (!then_s) { DESTROY_CNODE(cond); return NULLPTR; }

    PCNODE n = CNODE_NEW(CNODE_IF, tok->line, tok->col);
    if (!n) { DESTROY_CNODE(cond); DESTROY_CNODE(then_s); return NULLPTR; }
    CNODE_ADD_CHILD(n, cond); CNODE_ADD_CHILD(n, then_s);

    if (ps_match(p, CTOK_KW_ELSE)) {
        PCNODE else_s = parse_stmt(p);
        if (!else_s) { DESTROY_CNODE(n); return NULLPTR; }
        CNODE_ADD_CHILD(n, else_s);
    }
    return n;
}

static PCNODE parse_for(PSTATE *p) {
    PCOMP_TOK tok = ps_advance(p);   /* consume 'FOR' */
    if (!ps_eat(p, CTOK_LPAREN, "'(' after for")) return NULLPTR;

    PCNODE init = NULLPTR, cond = NULLPTR, incr = NULLPTR;

    /* init: var decl or expr or empty */
    if (!ps_check(p, CTOK_SEMICOLON)) {
        if (tok_is_type(p))
            init = parse_var_decl(p);     /* includes the ';' */
        else {
            init = parse_expr(p);
            if (!ps_eat(p, CTOK_SEMICOLON, "';' in for init")) { DESTROY_CNODE(init); return NULLPTR; }
        }
    } else ps_advance(p);

    /* cond */
    if (!ps_check(p, CTOK_SEMICOLON)) cond = parse_expr(p);
    if (!ps_eat(p, CTOK_SEMICOLON, "';' in for condition")) { DESTROY_CNODE(init); DESTROY_CNODE(cond); return NULLPTR; }

    /* incr */
    if (!ps_check(p, CTOK_RPAREN)) incr = parse_expr(p);
    if (!ps_eat(p, CTOK_RPAREN, "')' in for")) {
        DESTROY_CNODE(init); DESTROY_CNODE(cond); DESTROY_CNODE(incr); return NULLPTR;
    }

    PCNODE body = parse_stmt(p);
    if (!body) { DESTROY_CNODE(init); DESTROY_CNODE(cond); DESTROY_CNODE(incr); return NULLPTR; }

    PCNODE n = CNODE_NEW(CNODE_FOR, tok->line, tok->col);
    if (!n) { DESTROY_CNODE(init); DESTROY_CNODE(cond); DESTROY_CNODE(incr); DESTROY_CNODE(body); return NULLPTR; }
    CNODE_ADD_CHILD(n, init); CNODE_ADD_CHILD(n, cond);
    CNODE_ADD_CHILD(n, incr); CNODE_ADD_CHILD(n, body);
    return n;
}

static PCNODE parse_while(PSTATE *p) {
    PCOMP_TOK tok = ps_advance(p);
    if (!ps_eat(p, CTOK_LPAREN, "'(' after while")) return NULLPTR;
    PCNODE cond = parse_expr(p);
    if (!cond) return NULLPTR;
    if (!ps_eat(p, CTOK_RPAREN, "')' after while condition")) { DESTROY_CNODE(cond); return NULLPTR; }
    PCNODE body = parse_stmt(p);
    if (!body) { DESTROY_CNODE(cond); return NULLPTR; }
    PCNODE n = CNODE_NEW(CNODE_WHILE, tok->line, tok->col);
    if (!n) { DESTROY_CNODE(cond); DESTROY_CNODE(body); return NULLPTR; }
    CNODE_ADD_CHILD(n, cond); CNODE_ADD_CHILD(n, body);
    return n;
}

static PCNODE parse_do_while(PSTATE *p) {
    PCOMP_TOK tok = ps_advance(p);
    PCNODE body = parse_stmt(p);
    if (!body) return NULLPTR;
    if (!ps_eat(p, CTOK_KW_WHILE, "'while' after do block")) { DESTROY_CNODE(body); return NULLPTR; }
    if (!ps_eat(p, CTOK_LPAREN,   "'(' after do-while"))      { DESTROY_CNODE(body); return NULLPTR; }
    PCNODE cond = parse_expr(p);
    if (!cond) { DESTROY_CNODE(body); return NULLPTR; }
    if (!ps_eat(p, CTOK_RPAREN,   "')' after do-while"))      { DESTROY_CNODE(body); DESTROY_CNODE(cond); return NULLPTR; }
    if (!ps_eat(p, CTOK_SEMICOLON,"';' after do-while"))       { DESTROY_CNODE(body); DESTROY_CNODE(cond); return NULLPTR; }
    PCNODE n = CNODE_NEW(CNODE_DO_WHILE, tok->line, tok->col);
    if (!n) { DESTROY_CNODE(body); DESTROY_CNODE(cond); return NULLPTR; }
    CNODE_ADD_CHILD(n, body); CNODE_ADD_CHILD(n, cond);
    return n;
}

static PCNODE parse_switch(PSTATE *p) {
    PCOMP_TOK tok = ps_advance(p);
    if (!ps_eat(p, CTOK_LPAREN, "'(' after switch")) return NULLPTR;
    PCNODE expr = parse_expr(p);
    if (!expr) return NULLPTR;
    if (!ps_eat(p, CTOK_RPAREN, "')' after switch expr")) { DESTROY_CNODE(expr); return NULLPTR; }
    if (!ps_eat(p, CTOK_LBRACE, "'{' after switch ()"))   { DESTROY_CNODE(expr); return NULLPTR; }

    PCNODE sw = CNODE_NEW(CNODE_SWITCH, tok->line, tok->col);
    if (!sw) { DESTROY_CNODE(expr); return NULLPTR; }
    CNODE_ADD_CHILD(sw, expr);

    while (!ps_check(p, CTOK_RBRACE) && !ps_check(p, CTOK_EOF)) {
        PCOMP_TOK ct = ps_peek(p);
        PCNODE clause = NULLPTR;

        if (ct->type == CTOK_KW_CASE) {
            ps_advance(p);
            /*  case 1...n: range syntax (future) — for now parse a single expr */
            PCNODE val = parse_expr(p);
            if (!val) { DESTROY_CNODE(sw); return NULLPTR; }
            if (!ps_eat(p, CTOK_COLON, "':' after case value")) { DESTROY_CNODE(val); DESTROY_CNODE(sw); return NULLPTR; }
            clause = CNODE_NEW(CNODE_CASE, ct->line, ct->col);
            if (!clause) { DESTROY_CNODE(val); DESTROY_CNODE(sw); return NULLPTR; }
            CNODE_ADD_CHILD(clause, val);
        } else if (ct->type == CTOK_KW_DEFAULT) {
            ps_advance(p);
            if (!ps_eat(p, CTOK_COLON, "':' after default")) { DESTROY_CNODE(sw); return NULLPTR; }
            clause = CNODE_NEW(CNODE_DEFAULT, ct->line, ct->col);
            if (!clause) { DESTROY_CNODE(sw); return NULLPTR; }
        } else {
            /* Statement inside case body */
            PCNODE s = parse_stmt(p);
            if (!s) { DESTROY_CNODE(sw); return NULLPTR; }
            /* Attach to last clause if any, else top-level */
            if (sw->child_count > 1) {
                PCNODE last = sw->children[sw->child_count - 1];
                CNODE_ADD_CHILD(last, s);
            } else {
                CNODE_ADD_CHILD(sw, s);
            }
            continue;
        }
        CNODE_ADD_CHILD(sw, clause);
    }
    ps_eat(p, CTOK_RBRACE, "'}' to close switch");
    return sw;
}

static PCNODE parse_return(PSTATE *p) {
    PCOMP_TOK tok = ps_advance(p);
    PCNODE n = CNODE_NEW(CNODE_RETURN, tok->line, tok->col);
    if (!n) return NULLPTR;
    if (!ps_check(p, CTOK_SEMICOLON)) {
        PCNODE val = parse_expr(p);
        if (!val) { DESTROY_CNODE(n); return NULLPTR; }
        CNODE_ADD_CHILD(n, val);
    }
    ps_eat(p, CTOK_SEMICOLON, "';' after return");
    return n;
}

static PCNODE parse_goto(PSTATE *p) {
    PCOMP_TOK tok = ps_advance(p);
    PCOMP_TOK lbl = ps_peek(p);
    if (lbl->type != CTOK_IDENT) {
        printf("[COMP PARSE] %u:%u: expected label after goto\n", lbl->line, lbl->col);
        p->ctx->errors++; return NULLPTR;
    }
    ps_advance(p);
    ps_eat(p, CTOK_SEMICOLON, "';' after goto label");
    PCNODE n = CNODE_NEW(CNODE_GOTO, tok->line, tok->col);
    if (!n) return NULLPTR;
    n->txt = STRDUP(lbl->txt);
    return n;
}

/* Inline ASM{} block: consume everything between the braces as raw text */
static PCNODE parse_asm_block(PSTATE *p) {
    PCOMP_TOK tok = ps_advance(p);  /* consume 'ASM' keyword */
    if (!ps_eat(p, CTOK_LBRACE, "'{' after ASM")) return NULLPTR;

    U8 raw[BUF_SZ];
    U32 rlen = 0;
    raw[0] = '\0';
    U32 depth = 1;

    while (!ps_check(p, CTOK_EOF) && depth > 0) {
        PCOMP_TOK t = ps_advance(p);
        if (t->type == CTOK_LBRACE) { depth++; }
        else if (t->type == CTOK_RBRACE) { depth--; if (depth == 0) break; }
        if (t->txt && rlen + STRLEN(t->txt) + 2 < BUF_SZ) {
            STRCAT((U8*)raw, t->txt);
            STRCAT((U8*)raw, " ");
            rlen += STRLEN(t->txt) + 1;
        }
    }

    PCNODE n = CNODE_NEW(CNODE_ASM_BLOCK, tok->line, tok->col);
    if (!n) return NULLPTR;
    n->txt = STRDUP(raw);
    return n;
}

/* Variable declaration: type IDENT ['=' expr] ';'
 * Called from both block (local) and top-level (global). */
static PCNODE parse_var_decl(PSTATE *p) {
    COMP_TYPE t;
    if (!parse_type(p, &t)) return NULLPTR;

    PCOMP_TOK name_tok = ps_peek(p);
    if (name_tok->type != CTOK_IDENT) {
        printf("[COMP PARSE] %u:%u: expected variable name\n", name_tok->line, name_tok->col);
        p->ctx->errors++; if (t.name) MFree(t.name); return NULLPTR;
    }
    ps_advance(p);

    PCNODE n = CNODE_NEW(CNODE_VAR_DECL, name_tok->line, name_tok->col);
    if (!n) { if (t.name) MFree(t.name); return NULLPTR; }
    n->dtype = t;
    n->txt   = STRDUP(name_tok->txt);

    /* Optional initializer */
    if (ps_match(p, CTOK_ASSIGN)) {
        PCNODE init = parse_expr(p);
        if (!init) { DESTROY_CNODE(n); return NULLPTR; }
        CNODE_ADD_CHILD(n, init);
    }

    if (!ps_eat(p, CTOK_SEMICOLON, "';' after variable declaration")) {
        DESTROY_CNODE(n); return NULLPTR;
    }
    return n;
}

/* Block: '{' var_decl* stmt* '}' */
static PCNODE parse_block(PSTATE *p) {
    PCOMP_TOK open = ps_peek(p);
    if (!ps_eat(p, CTOK_LBRACE, "'{'")) return NULLPTR;

    PCNODE block = CNODE_NEW(CNODE_BLOCK, open->line, open->col);
    if (!block) return NULLPTR;

    /* Phase 1: variable declarations */
    while (!ps_check(p, CTOK_RBRACE) && !ps_check(p, CTOK_EOF) && tok_is_type(p)) {
        /* Peek two ahead to distinguish "type name ;" from "type name (" (func call not possible here) */
        PCNODE decl = parse_var_decl(p);
        if (!decl) { DESTROY_CNODE(block); return NULLPTR; }
        CNODE_ADD_CHILD(block, decl);
    }

    /* Phase 2: statements */
    while (!ps_check(p, CTOK_RBRACE) && !ps_check(p, CTOK_EOF)) {
        PCNODE stmt = parse_stmt(p);
        if (!stmt) { DESTROY_CNODE(block); return NULLPTR; }
        CNODE_ADD_CHILD(block, stmt);
    }

    if (!ps_eat(p, CTOK_RBRACE, "'}'")) { DESTROY_CNODE(block); return NULLPTR; }
    return block;
}

/* Statement dispatcher */
static PCNODE parse_stmt(PSTATE *p) {
    PCOMP_TOK tok = ps_peek(p);

    switch (tok->type) {
        case CTOK_LBRACE:       return parse_block(p);
        case CTOK_KW_IF:        return parse_if(p);
        case CTOK_KW_FOR:       return parse_for(p);
        case CTOK_KW_WHILE:     return parse_while(p);
        case CTOK_KW_DO:        return parse_do_while(p);
        case CTOK_KW_SWITCH:    return parse_switch(p);
        case CTOK_KW_RETURN:    return parse_return(p);
        case CTOK_KW_GOTO:      return parse_goto(p);
        case CTOK_KW_ASM:       return parse_asm_block(p);
        case CTOK_KW_BREAK: {
            ps_advance(p);
            ps_eat(p, CTOK_SEMICOLON, "';' after break");
            return CNODE_NEW(CNODE_BREAK, tok->line, tok->col);
        }
        case CTOK_KW_CONTINUE: {
            ps_advance(p);
            ps_eat(p, CTOK_SEMICOLON, "';' after continue");
            return CNODE_NEW(CNODE_CONTINUE, tok->line, tok->col);
        }
        case CTOK_SEMICOLON: {
            ps_advance(p);
            return CNODE_NEW(CNODE_EXPR_STMT, tok->line, tok->col);   /* empty stmt */
        }
        case CTOK_IDENT: {
            /* Label: IDENT ':' */
            if (ps_peek2(p)->type == CTOK_COLON) {
                ps_advance(p); ps_advance(p);   /* name + ':' */
                PCNODE n = CNODE_NEW(CNODE_LABEL, tok->line, tok->col);
                if (!n) return NULLPTR;
                n->txt = STRDUP(tok->txt);
                PCNODE next = parse_stmt(p);
                if (next) CNODE_ADD_CHILD(n, next);
                return n;
            }
            /* Fall through to expression-statement */
        }
        default: {
            /* Expression statement */
            PCNODE expr = parse_expr(p);
            if (!expr) { ps_sync(p); return NULLPTR; }
            ps_eat(p, CTOK_SEMICOLON, "';' after expression");
            PCNODE n = CNODE_NEW(CNODE_EXPR_STMT, tok->line, tok->col);
            if (!n) { DESTROY_CNODE(expr); return NULLPTR; }
            CNODE_ADD_CHILD(n, expr);
            return n;
        }
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  PARAMETER LIST
 * ════════════════════════════════════════════════════════════════════════════ */
/* Returns a CNODE_FUNC_DECL node after filling its children with params then body.
 * Called AFTER the function name has been consumed. */
static BOOL parse_param_list(PSTATE *p, PCNODE func) {
    if (!ps_eat(p, CTOK_LPAREN, "'(' in function declaration")) return FALSE;

    while (!ps_check(p, CTOK_RPAREN) && !ps_check(p, CTOK_EOF)) {
        COMP_TYPE pt;
        if (!parse_type(p, &pt)) return FALSE;

        PCOMP_TOK pname = ps_peek(p);
        PU8 param_name = NULLPTR;
        if (pname->type == CTOK_IDENT) {
            param_name = pname->txt;
            ps_advance(p);
        }

        PCNODE param = CNODE_NEW(CNODE_PARAM, pname->line, pname->col);
        if (!param) return FALSE;
        param->dtype = pt;
        param->txt   = param_name ? STRDUP(param_name) : NULLPTR;
        CNODE_ADD_CHILD(func, param);

        if (!ps_match(p, CTOK_COMMA)) break;
    }
    return ps_eat(p, CTOK_RPAREN, "')' after parameters");
}

/* ════════════════════════════════════════════════════════════════════════════
 *  TOP-LEVEL DECLARATION PARSER
 * ════════════════════════════════════════════════════════════════════════════ */
static PCNODE parse_toplevel_decl(PSTATE *p) {
    /* Struct / union / enum definitions */
    if (ps_check(p, CTOK_KW_STRUCT) || ps_check(p, CTOK_KW_UNION) || ps_check(p, CTOK_KW_ENUM)) {
        /* TODO: implement struct/union/enum body parsing */
        PCOMP_TOK kw = ps_advance(p);
        PCOMP_TOK nm = ps_peek(p);
        PU8 tag_name = NULLPTR;
        if (nm->type == CTOK_IDENT) { tag_name = nm->txt; ps_advance(p); }

        CNODE_TYPE nt = (kw->type == CTOK_KW_STRUCT) ? CNODE_STRUCT_DECL
                      : (kw->type == CTOK_KW_UNION)  ? CNODE_UNION_DECL
                                                      : CNODE_ENUM_DECL;
        PCNODE n = CNODE_NEW(nt, kw->line, kw->col);
        if (!n) return NULLPTR;
        n->txt = tag_name ? STRDUP(tag_name) : NULLPTR;
        /* Skip over body { ... } */
        if (ps_check(p, CTOK_LBRACE)) {
            U32 depth = 1; ps_advance(p);
            while (!ps_check(p, CTOK_EOF) && depth > 0) {
                if (ps_peek(p)->type == CTOK_LBRACE) depth++;
                else if (ps_peek(p)->type == CTOK_RBRACE) depth--;
                ps_advance(p);
            }
        }
        ps_match(p, CTOK_SEMICOLON);
        return n;
    }

    /* Everything else: read type + name, then decide */
    COMP_TYPE t;
    if (!parse_type(p, &t)) return NULLPTR;

    PCOMP_TOK name_tok = ps_peek(p);
    if (name_tok->type != CTOK_IDENT) {
        printf("[COMP PARSE] %u:%u: expected declaration name\n", name_tok->line, name_tok->col);
        p->ctx->errors++; if (t.name) MFree(t.name); return NULLPTR;
    }
    ps_advance(p);

    /* Function: type name ( ... ) { ... } */
    if (ps_check(p, CTOK_LPAREN)) {
        PCNODE func = CNODE_NEW(CNODE_FUNC_DECL, name_tok->line, name_tok->col);
        if (!func) { if (t.name) MFree(t.name); return NULLPTR; }
        func->dtype = t;
        func->txt   = STRDUP(name_tok->txt);

        if (!parse_param_list(p, func)) { DESTROY_CNODE(func); return NULLPTR; }

        if (ps_check(p, CTOK_SEMICOLON)) {
            /* Forward declaration */
            ps_advance(p);
        } else {
            PCNODE body = parse_block(p);
            if (!body) { DESTROY_CNODE(func); return NULLPTR; }
            CNODE_ADD_CHILD(func, body);
        }
        return func;
    }

    /* Global variable */
    PCNODE var = CNODE_NEW(CNODE_VAR_DECL, name_tok->line, name_tok->col);
    if (!var) { if (t.name) MFree(t.name); return NULLPTR; }
    var->dtype = t;
    var->txt   = STRDUP(name_tok->txt);

    if (ps_match(p, CTOK_ASSIGN)) {
        PCNODE init = parse_expr(p);
        if (!init) { DESTROY_CNODE(var); return NULLPTR; }
        CNODE_ADD_CHILD(var, init);
    }
    ps_eat(p, CTOK_SEMICOLON, "';' after global variable");
    return var;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  PUBLIC API
 * ════════════════════════════════════════════════════════════════════════════ */
PCNODE COMP_PARSE(PCOMP_TOK_ARRAY toks, PCOMP_CTX ctx) {
    PSTATE p;
    p.toks = toks;
    p.pos  = 0;
    p.ctx  = ctx;

    PCNODE program = CNODE_NEW(CNODE_PROGRAM, 0, 0);
    if (!program) return NULLPTR;

    while (!ps_check(&p, CTOK_EOF)) {
        PCNODE decl = parse_toplevel_decl(&p);
        if (!decl) {
            /* Error recovery: skip to next sensible boundary */
            ps_sync(&p);
            continue;
        }
        CNODE_ADD_CHILD(program, decl);
    }

    if (ctx->errors > 0) {
        printf("[COMP PARSE] Parsing finished with %u error(s).\n", ctx->errors);
        DESTROY_CNODE_TREE(program);
        return NULLPTR;
    }
    return program;
}
