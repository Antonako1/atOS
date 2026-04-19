/*
 * ════════════════════════════════════════════════════════════════════════════
 *  COMPILER/GEN.c  —  x86 32-bit Assembly Code Generator
 *
 *  Emits ASTRAC assembler (ASM) source from the verified AST.
 *
 *  ABI: cdecl
 *   • Arguments pushed right-to-left before CALL; caller pops stack.
 *   • Integer return value in EAX.
 *   • Callee-saved: EBX, ESI, EDI, EBP.
 *   • Stack frame:  PUSH EBP / MOV EBP, ESP  ... LEAVE / RET
 *   • Local variables at [EBP - n], parameters at [EBP + n].
 *
 *  Expression evaluation strategy:
 *   • Every expression leaves its result in EAX.
 *   • Binary ops save left in EBX before evaluating right:
 *       eval left  -> EAX
 *       PUSH EAX
 *       eval right -> EAX
 *       POP  EBX
 *       <op> EAX, EBX   (or reverse depending on op)
 * ════════════════════════════════════════════════════════════════════════════
 */

#include "COMPILER.h"
#include "../ASTRAC.h"

/* ════════════════════════════════════════════════════════════════════════════
 *  CODEGEN STATE
 * ════════════════════════════════════════════════════════════════════════════ */
#define MAX_LOCALS  256
#define MAX_PARAMS  32

typedef struct {
    U8  name[128];
    I32 offset;   /* signed EBP offset: locals < 0, params > 0 */
    U32 size;     /* in bytes */
} LOCAL_VAR;

typedef struct {
    FILE      *out;
    PCOMP_CTX  ctx;
    LOCAL_VAR  locals[MAX_LOCALS];
    U32        local_count;
    I32        stack_size;   /* current local frame size (positive, grows down) */
    U32        label_cnt;    /* unique label counter */
    BOOL       in_func;
} GEN_STATE;

/* ── Emit helpers ─────────────────────────────────────────────────────────── */
/* Direct emit using SPRINTF + FWRITE */
#define EMIT(gs, ...) do {                                                     \
    U8 _buf[BUF_SZ];                                                           \
    U32 _n = SPRINTF(_buf, __VA_ARGS__);                                       \
    FWRITE((gs)->out, _buf, _n);                                               \
} while(0)

static U32 new_label(GEN_STATE *gs) { return gs->ctx->label_counter++; }

/*
Always push loop labels in pairs: first the "continue" target, then the "break" target.
This simplifies handling of nested loops and switch statements, since we don't need to track which label is
*/
static U32 push_loop_label(GEN_STATE *gs, U32 lbl) {
    if (gs->ctx->loop_label_stack_top < 32) {
        gs->ctx->loop_label_stack[gs->ctx->loop_label_stack_top++] = lbl;
        return lbl;
    }
    return 0;
}
static U32 pop_loop_label(GEN_STATE *gs) {
    if (gs->ctx->loop_label_stack_top > 0) return gs->ctx->loop_label_stack[--gs->ctx->loop_label_stack_top];
    return 0;
}

static void push_rodata_string(GEN_STATE *gs, PCNODE str_node, U32 lbl) {
    if (gs->ctx->rodata_string_count < 256) {
        DEBUGM_PRINTF("[COMP GEN] Pushing rodata string literal for label _S%u: \"%s\"\n", lbl, str_node && str_node->txt ? str_node->txt : "");
        RODATA_STR *rs = &gs->ctx->rodata_strings[gs->ctx->rodata_string_count++];
        MEMZERO(rs, sizeof(RODATA_STR));
        rs->node = str_node;
        rs->label = lbl;
    }
}
static RODATA_STR* pop_rodata_string(GEN_STATE *gs) {
    if (gs->ctx->rodata_string_count == 0) {
        DEBUGM_PRINTF("[COMP GEN] pop_rodata_string: no rodata strings to pop\n");
        return NULLPTR;
    }
    RODATA_STR *rs = &gs->ctx->rodata_strings[gs->ctx->rodata_string_count - 1];
    DEBUGM_PRINTF("[COMP GEN] Popping rodata string literal for label _S%u\n", rs->label);
    gs->ctx->rodata_string_count--;
    return rs;
}

static U32 get_continue_loop_label(GEN_STATE *gs) { return gs->ctx->loop_label_stack_top > 0 ? gs->ctx->loop_label_stack[gs->ctx->loop_label_stack_top - 2] : 0; }
static U32 get_break_loop_label(GEN_STATE *gs) { return gs->ctx->loop_label_stack_top > 0 ? gs->ctx->loop_label_stack[gs->ctx->loop_label_stack_top - 1] : 0; }

/* ── Local variable helpers ──────────────────────────────────────────────── */
static LOCAL_VAR *find_local(GEN_STATE *gs, PU8 name) {
    for (U32 i = 0; i < gs->local_count; i++)
        if (STRCMP(gs->locals[i].name, name) == 0)
            return &gs->locals[i];
    return NULLPTR;
}

static I32 alloc_local(GEN_STATE *gs, PU8 name, U32 size) {
    if (gs->local_count >= MAX_LOCALS) return 0;
    gs->stack_size += (I32)size;
    I32 offset = -(I32)gs->stack_size;
    LOCAL_VAR *v = &gs->locals[gs->local_count++];
    STRNCPY(v->name, name, 127); v->name[127] = '\0';
    v->offset = offset;
    v->size   = size;
    return offset;
}

/* ── Expression code generation (result in EAX) ──────────────────────────── */
static VOID gen_expr(GEN_STATE *gs, PCNODE n);

static VOID gen_expr(GEN_STATE *gs, PCNODE n) {
    if (!n) return;
    
    switch (n->ntype) {
        
        case CNODE_INT_LIT:
            EMIT(gs, "\tMOV EAX, %u\n", n->ival);
            break;

        case CNODE_CHAR_LIT:
            EMIT(gs, "\tMOV EAX, %u\n", n->ival);
            break;

        case CNODE_TRUE_LIT:
            EMIT(gs, "\tMOV EAX, 1\n");
            break;

        case CNODE_FALSE_LIT:
        case CNODE_NULLPTR:
            EMIT(gs, "\tMOV EAX, 0\n");
            break;

        case CNODE_IDENT: {
            LOCAL_VAR *v = n->txt ? find_local(gs, n->txt) : NULLPTR;
            if (v) {
                /* Load from stack frame */
                if (v->size == 1)
                    EMIT(gs, "\tMOVZX EAX, BYTE [EBP%+d]\n", v->offset);
                else if (v->size == 2)
                    EMIT(gs, "\tMOVZX EAX, WORD [EBP%+d]\n", v->offset);
                else
                    EMIT(gs, "\tMOV EAX, [EBP%+d]\n", v->offset);
            } else {
                /* Global or external symbol */
                EMIT(gs, "\tMOV EAX, [%s]\n", n->txt ? n->txt : "?");
            }
            break;
        }

        case CNODE_STR_LIT: {
            /* Strings: emit as rodata label, load address */
            U32 lbl = new_label(gs);
            push_rodata_string(gs, n, lbl);
            EMIT(gs, "\tMOV EAX, _S%u\n", lbl);
            break;
        }

        case CNODE_ADDR:
            /* &a — load effective address */
            if (n->child_count > 0 && n->children[0]->ntype == CNODE_IDENT) {
                LOCAL_VAR *v = n->children[0]->txt
                             ? find_local(gs, n->children[0]->txt) : NULLPTR;
                if (v) EMIT(gs, "\tLEA EAX, [EBP%+d]\n", v->offset);
                else   EMIT(gs, "\tMOV EAX, %s\n", n->children[0]->txt ? n->children[0]->txt : "?");
            }
            break;

        case CNODE_DEREF:
            gen_expr(gs, n->child_count > 0 ? n->children[0] : NULLPTR);
            EMIT(gs, "\tMOV EAX, [EAX]\n");
            break;

        case CNODE_UNARY:
            if (n->child_count > 0) gen_expr(gs, n->children[0]);
            switch (n->op) {
                case CTOK_MINUS: EMIT(gs, "\tNEG EAX\n"); break;
                case CTOK_NOT:
                    EMIT(gs, "\tCMP EAX, 0\n");
                    EMIT(gs, "\tSETE AL\n");
                    EMIT(gs, "\tMOVZX EAX, AL\n");
                    break;
                case CTOK_TILDE: EMIT(gs, "\tNOT EAX\n"); break;
                case CTOK_PLUSPLUS:
                    if (n->child_count > 0 && n->children[0]->ntype == CNODE_IDENT) {
                        LOCAL_VAR *v = n->children[0]->txt ? find_local(gs, n->children[0]->txt) : NULLPTR;
                        if (v) EMIT(gs, "\tINC DWORD [EBP%+d]\n\tMOV EAX, [EBP%+d]\n", v->offset, v->offset);
                    }
                    break;
                case CTOK_MINUSMINUS:
                    if (n->child_count > 0 && n->children[0]->ntype == CNODE_IDENT) {
                        LOCAL_VAR *v = n->children[0]->txt ? find_local(gs, n->children[0]->txt) : NULLPTR;
                        if (v) EMIT(gs, "\tDEC DWORD [EBP%+d]\n\tMOV EAX, [EBP%+d]\n", v->offset, v->offset);
                    }
                    break;
                default: break;
            }
            break;

        case CNODE_POSTFIX:
            /* Postfix ++/-- : return old value, then mutate */
            if (n->child_count > 0 && n->children[0]->ntype == CNODE_IDENT) {
                LOCAL_VAR *v = n->children[0]->txt ? find_local(gs, n->children[0]->txt) : NULLPTR;
                if (v) {
                    EMIT(gs, "\tMOV EAX, [EBP%+d]\n", v->offset);
                    if (n->op == CTOK_PLUSPLUS)
                        EMIT(gs, "\tINC DWORD [EBP%+d]\n", v->offset);
                    else
                        EMIT(gs, "\tDEC DWORD [EBP%+d]\n", v->offset);
                }
            }
            break;

        case CNODE_BINARY:
            if (n->child_count < 2) break;
            gen_expr(gs, n->children[0]);
            EMIT(gs, "\tPUSH EAX\n");
            gen_expr(gs, n->children[1]);
            EMIT(gs, "\tPOP EBX\n");
            /* EBX = left, EAX = right */
            switch (n->op) {
                case CTOK_PLUS:    EMIT(gs, "\tADD EAX, EBX\n"); break;
                case CTOK_MINUS:   EMIT(gs, "\tSUB EBX, EAX\n\tMOV EAX, EBX\n"); break;
                case CTOK_STAR:    EMIT(gs, "\tIMUL EAX, EBX\n"); break;
                case CTOK_SLASH:
                    EMIT(gs, "\tMOV ECX, EAX\n\tMOV EAX, EBX\n\tCDQ\n\tIDIV ECX\n"); break;
                case CTOK_PERCENT:
                    EMIT(gs, "\tMOV ECX, EAX\n\tMOV EAX, EBX\n\tCDQ\n\tIDIV ECX\n\tMOV EAX, EDX\n"); break;
                case CTOK_AMP:     EMIT(gs, "\tAND EAX, EBX\n"); break;
                case CTOK_PIPE:    EMIT(gs, "\tOR  EAX, EBX\n"); break;
                case CTOK_CARET:   EMIT(gs, "\tXOR EAX, EBX\n"); break;
                case CTOK_SHL:     EMIT(gs, "\tMOV ECX, EAX\n\tSHL EBX, CL\n\tMOV EAX, EBX\n"); break;
                case CTOK_SHR:     EMIT(gs, "\tMOV ECX, EAX\n\tSHR EBX, CL\n\tMOV EAX, EBX\n"); break;
                /* Relational — result is 0 or 1 in EAX */
                case CTOK_EQ:  EMIT(gs,"\tCMP EBX, EAX\n\tSETE AL\n\tMOVZX EAX,AL\n"); break;
                case CTOK_NEQ: EMIT(gs,"\tCMP EBX, EAX\n\tSETNE AL\n\tMOVZX EAX,AL\n"); break;
                case CTOK_LT:  EMIT(gs,"\tCMP EBX, EAX\n\tSETL AL\n\tMOVZX EAX,AL\n"); break;
                case CTOK_GT:  EMIT(gs,"\tCMP EBX, EAX\n\tSETG AL\n\tMOVZX EAX,AL\n"); break;
                case CTOK_LE:  EMIT(gs,"\tCMP EBX, EAX\n\tSETLE AL\n\tMOVZX EAX,AL\n"); break;
                case CTOK_GE:  EMIT(gs,"\tCMP EBX, EAX\n\tSETGE AL\n\tMOVZX EAX,AL\n"); break;
                case CTOK_AND:
                    EMIT(gs,"\tCMP EBX, 0\n\tSETNE BL\n\tCMP EAX, 0\n\tSETNE AL\n\tAND AL, BL\n\tMOVZX EAX, AL\n");
                    break;
                case CTOK_OR:
                    EMIT(gs,"\tOR EBX, EAX\n\tSETNE AL\n\tMOVZX EAX, AL\n");
                    break;
                default: break;
            }
            break;

        case CNODE_ASSIGN: {
            if (n->child_count < 2) break;
            gen_expr(gs, n->children[1]);   /* value in EAX */
            /* Store to lhs */
            PCNODE lhs = n->children[0];
            if (lhs->ntype == CNODE_IDENT && lhs->txt) {
                LOCAL_VAR *v = find_local(gs, lhs->txt);
                if (v) {
                    if (v->size == 1)
                        EMIT(gs, "\tMOV BYTE [EBP%+d], AL\n", v->offset);
                    else if (v->size == 2)
                        EMIT(gs, "\tMOV WORD [EBP%+d], AX\n", v->offset);
                    else
                        EMIT(gs, "\tMOV [EBP%+d], EAX\n", v->offset);
                } else {
                    EMIT(gs, "\tMOV [%s], EAX\n", lhs->txt);
                }
            } else if (lhs->ntype == CNODE_DEREF && lhs->child_count > 0) {
                /* *ptr = val */
                EMIT(gs, "\tPUSH EAX\n");
                gen_expr(gs, lhs->children[0]);
                EMIT(gs, "\tPOP EBX\n\tMOV [EAX], EBX\n\tMOV EAX, EBX\n");
            }
            break;
        }

        case CNODE_TERNARY:
            if (n->child_count < 3) break;
            {
                U32 else_lbl = new_label(gs);
                U32 end_lbl  = new_label(gs);
                gen_expr(gs, n->children[0]);
                EMIT(gs, "\tCMP EAX, 0\n\tJE .L%u\n", else_lbl);
                gen_expr(gs, n->children[1]);
                EMIT(gs, "\tJMP .L%u\n.L%u:\n", end_lbl, else_lbl);
                gen_expr(gs, n->children[2]);
                EMIT(gs, ".L%u:\n", end_lbl);
            }
            break;

        case CNODE_CALL: {
            if (n->child_count == 0) break;
            if(GET_ARGS()->debug_symbols) {
                EMIT(gs, "\t; gen_expr: function call with %u args at line %u\n", n->child_count - 1, n->line);
            }
            /* Push arguments right-to-left */
            U32 arg_count = n->child_count - 1;
            for (U32 i = arg_count; i > 0; i--)
                if (n->children[i]) {
                    gen_expr(gs, n->children[i]);
                    EMIT(gs, "\tPUSH EAX\n");
                }
            /* Call */
            PCNODE callee = n->children[0];
            if (callee->ntype == CNODE_IDENT && callee->txt)
                EMIT(gs, "\tCALL %s\n", callee->txt);
            else {
                gen_expr(gs, callee);
                EMIT(gs, "\tCALL EAX\n");
            }
            /* Caller cleans stack (cdecl) */
            if (arg_count > 0)
                EMIT(gs, "\tADD ESP, %u\n", arg_count * 4);
            /* Result is in EAX */
            break;
        }

        case CNODE_SIZEOF_TYPE:
            EMIT(gs, "\tMOV EAX, %u\n", COMP_TYPE_SIZE(n->dtype));
            break;

        case CNODE_INDEX:
            if (n->child_count < 2) break;
            gen_expr(gs, n->children[0]);           /* base address */
            EMIT(gs, "\tPUSH EAX\n");
            gen_expr(gs, n->children[1]);           /* index -> EAX */

            // TODO: handle pointer types and get base type size
            // must work for arrays, pointers, and multi-dimensional indexing
            // structs and unions should not be indexable, so we can assume it's an array or pointer
            U32 sz_type = COMP_TYPE_SIZE(n->dtype);
            
            EMIT(gs, "\tIMUL EAX, %u\n", sz_type);
            EMIT(gs, "\tPOP EBX\n\tADD EAX, EBX\n");
            EMIT(gs, "\tMOV EAX, [EAX]\n");
            break;

        case CNODE_CAST:
            if (n->child_count > 0) gen_expr(gs, n->children[0]);
            /* TODO: emit truncation/extension for smaller types */
            break;

        default:
            /* TODO: unhandled expression node */
            EMIT(gs, "\t; TODO: unhandled expr node type %u\n", (U32)n->ntype);
            break;
    }
}

/* ── Statement code generation ───────────────────────────────────────────── */
static VOID gen_stmt(GEN_STATE *gs, PCNODE n);

static VOID gen_stmt(GEN_STATE *gs, PCNODE n) {
    if (!n) return;
    switch (n->ntype) {

        case CNODE_BLOCK:
            // TODO: handle new scope for variables
            if(GET_ARGS()->debug_symbols) {
                EMIT(gs, "\t; gen_stmt: block with %u statements at line %u\n", n->child_count, n->line);
            }
            for (U32 i = 0; i < n->child_count; i++)
                gen_stmt(gs, n->children[i]);
            break;

        case CNODE_VAR_DECL: {
            U32 sz = COMP_TYPE_SIZE(n->dtype);
            if(GET_ARGS()->debug_symbols) {
                EMIT(gs, "\t; gen_stmt: var decl '%s' of size %u at line %u\n", n->txt ? n->txt : "(null)", sz, n->line);
            }
            if (sz == 0) sz = 4;   /* default */
            if (n->txt) alloc_local(gs, n->txt, sz);
            /* If there is an initializer, emit assign code */
            if (n->child_count > 0 && n->txt) {
                gen_expr(gs, n->children[0]);
                LOCAL_VAR *v = find_local(gs, n->txt);
                if (v) {
                    if (sz == 1)      EMIT(gs, "\tMOV BYTE [EBP%+d], AL\n", v->offset);
                    else if (sz == 2) EMIT(gs, "\tMOV WORD [EBP%+d], AX\n", v->offset);
                    else              EMIT(gs, "\tMOV [EBP%+d], EAX\n", v->offset);
                }
            }
            break;
        }

        case CNODE_RETURN:
            if (n->child_count > 0) gen_expr(gs, n->children[0]);
            else EMIT(gs, "\tXOR EAX, EAX\n");
            EMIT(gs, "\tLEAVE\n\tRET\n");
            break;

        case CNODE_EXPR_STMT:
            if (n->child_count > 0) gen_expr(gs, n->children[0]);
            break;

        case CNODE_IF: {
            if (n->child_count < 2) break;
            if(GET_ARGS()->debug_symbols) {
                EMIT(gs, "\t; gen_stmt: if statement at line %u\n", n->line);
            }
            U32 else_lbl = new_label(gs);
            U32 end_lbl  = new_label(gs);
            gen_expr(gs, n->children[0]);               /* condition */
            EMIT(gs, "\tCMP EAX, 0\n\tJE @@L%u\n", else_lbl);
            gen_stmt(gs, n->children[1]);               /* then */
            if (n->child_count > 2) {
                EMIT(gs, "\tJMP @@L%u\n@@L%u:\n", end_lbl, else_lbl);
                gen_stmt(gs, n->children[2]);           /* else */
                EMIT(gs, "@@L%u:\n", end_lbl);
            } else {
                EMIT(gs, "@@L%u:\n", else_lbl);
                (void)end_lbl;
            }
            break;
        }

        case CNODE_WHILE: {
            if (n->child_count < 2) break;
            if(GET_ARGS()->debug_symbols) {
                EMIT(gs, "\t; gen_stmt: while loop at line %u\n", n->line);
            }
            U32 loop_lbl = new_label(gs);
            U32 end_lbl  = new_label(gs);
            push_loop_label(gs, loop_lbl); // cnt
            push_loop_label(gs, end_lbl);  // brk
            EMIT(gs, "@@L%u:\n", loop_lbl);
            gen_expr(gs, n->children[0]);
            EMIT(gs, "\tCMP EAX, 0\n\tJE @@L%u\n", end_lbl);
            gen_stmt(gs, n->children[1]);
            EMIT(gs, "\tJMP @@L%u\n@@L%u:\n", loop_lbl, end_lbl);
            /* pop loop labels (break then continue) */
            pop_loop_label(gs); // brk
            pop_loop_label(gs); // cnt
            break;
        }

        case CNODE_DO_WHILE: {
            if (n->child_count < 2) break;
            if(GET_ARGS()->debug_symbols) {
                EMIT(gs, "\t; gen_stmt: do-while loop at line %u\n", n->line);
            }
            U32 loop_lbl = new_label(gs);
            push_loop_label(gs, loop_lbl); // cnt
            push_loop_label(gs, loop_lbl); // reuse same label for continue and break since condition is at end of loop
            EMIT(gs, "@@L%u:\n", loop_lbl);
            gen_stmt(gs, n->children[0]);
            gen_expr(gs, n->children[1]);
            EMIT(gs, "\tCMP EAX, 0\n\tJNE @@L%u\n", loop_lbl);
            /* pop the two loop labels we pushed */
            pop_loop_label(gs); // brk
            pop_loop_label(gs); // cnt
            break;
        }

        case CNODE_FOR: {
            if (n->child_count < 4) break;
            if(GET_ARGS()->debug_symbols) {
                EMIT(gs, "\t; gen_stmt: for loop init, cond, incr at line %u\n", n->line);
            }
            U32 loop_lbl = new_label(gs);
            U32 end_lbl  = new_label(gs);

            // push loop labels in order: first continue target, then break target
            push_loop_label(gs, loop_lbl); // cnt
            push_loop_label(gs, end_lbl);  // brk

            gen_stmt(gs, n->children[0]);               /* init */
            EMIT(gs, "@@L%u:\n", loop_lbl);
            if (n->children[1]) {
                gen_expr(gs, n->children[1]);           /* cond */
                EMIT(gs, "\tCMP EAX, 0\n\tJE @@L%u\n", end_lbl);
            }
            gen_stmt(gs, n->children[3]);               /* body */
            if (n->children[2]) gen_expr(gs, n->children[2]);  /* incr */
            EMIT(gs, "\tJMP @@L%u\n@@L%u:\n", loop_lbl, end_lbl);

            // after loop, pop labels in reverse order
            pop_loop_label(gs); // brk
            pop_loop_label(gs); // cnt
            break;
        }

        case CNODE_BREAK:
            // Jump to break target label from loop label stack
            EMIT(gs, "\tJMP @@L%u\n", get_break_loop_label(gs)); // brk
            break;

        case CNODE_CONTINUE:
            // Jump to continue target label from loop label stack
            EMIT(gs, "\tJMP @@L%u\n", get_continue_loop_label(gs)); // cnt
            break;

        case CNODE_SWITCH:
        case CNODE_CASE:
        case CNODE_DEFAULT:
            EMIT(gs, "\t; TODO: unimplemented switch/case/default\n");
            break;

        case CNODE_LABEL:
            EMIT(gs, "%s:\n", n->txt ? n->txt : "@@unknown_label");
            for (U32 i = 0; i < n->child_count; i++) gen_stmt(gs, n->children[i]);
            break;

        case CNODE_GOTO:
            EMIT(gs, "\tJMP %s\n", n->txt ? n->txt : "?EMPTY_LABEL");
            break;

        case CNODE_ASM_BLOCK:
            if (GET_ARGS()->debug_symbols) {
                EMIT(gs, "\t; gen_stmt: inline asm block, line %u\n", n->line);
            }
            if (n->txt) EMIT(gs, "%s\n", n->txt);
            break;

        default:
            EMIT(gs, "\t; TODO: unhandled stmt node type %u\n", (U32)n->ntype);
            break;
    }
}

/* ── Function code generation ─────────────────────────────────────────────── */
static VOID gen_function(GEN_STATE *gs, PCNODE func) {
    /* Reset local state */
    gs->local_count = 0;
    gs->stack_size  = 0;
    gs->in_func     = TRUE;

    PU8 fname = func->txt ? func->txt : "_unknown";

    /* First pass: scan body to determine local frame size */
    /* (simplified: we allocate stack at prologue with a placeholder and
     *  backpatch — for now we use a conservative over-allocation) */

    /* ── Prologue ─────────────────────────────────────────────── */
    EMIT(gs, "\n%s:\n", fname);
    EMIT(gs, "\tPUSH EBP\n");
    EMIT(gs, "\tMOV EBP, ESP\n");

    /* Register parameters in local table at positive EBP offsets */
    I32 param_off = 8;   /* [EBP+8] is first arg */
    for (U32 i = 0; i < func->child_count; i++) {
        PCNODE p = func->children[i];
        if (!p || p->ntype != CNODE_PARAM) continue;
        if (p->txt && gs->local_count < MAX_LOCALS) {
            LOCAL_VAR *v = &gs->locals[gs->local_count++];
            STRNCPY(v->name, p->txt, 127); v->name[127] = '\0';
            v->offset = param_off;
            v->size   = COMP_TYPE_SIZE(p->dtype);
            if (v->size == 0) v->size = 4;
            param_off += 4;
        }
    }

    /* Find and process body block */
    for (U32 i = 0; i < func->child_count; i++) {
        PCNODE child = func->children[i];
        if (!child || child->ntype != CNODE_BLOCK) continue;

        /* Reserve stack space: scan for var decls in body */
        U32 frame_sz = 0;
        for (U32 j = 0; j < child->child_count; j++) {
            PCNODE decl = child->children[j];
            if (!decl || decl->ntype != CNODE_VAR_DECL) continue;
            U32 sz = COMP_TYPE_SIZE(decl->dtype);
            if (sz == 0) sz = 4;
            frame_sz += sz;
        }
        if (frame_sz > 0)
            EMIT(gs, "\tSUB ESP, %u\n", frame_sz);

        /* ── Body ───────────────────────────────────────────────── */
        gen_stmt(gs, child);
        break;
    }

    /* ── Epilogue (default return 0 if no explicit return) ─────── */
    EMIT(gs, "\tXOR EAX, EAX\n");
    EMIT(gs, "\tLEAVE\n");
    EMIT(gs, "\tRET\n");

    gs->in_func = FALSE;
}

/* ── Top-level traversal ──────────────────────────────────────────────────── */
static VOID gen_toplevel(GEN_STATE *gs, PCNODE program) {
    EMIT(gs, ".USE32\n");
    EMIT(gs, ".ORG 0x%X\n\n", GET_ARGS()->org);

    /* Phase 1: global variables in .data section */
    BOOL has_globals = FALSE;
    for (U32 i = 0; i < program->child_count; i++) {
        PCNODE n = program->children[i];
        if (!n || n->ntype != CNODE_VAR_DECL) continue;
        if (!has_globals) {
            EMIT(gs, ".DATA\n");
            has_globals = TRUE;
        }
        EMIT(gs, "\t; global variable '%s'\n", n->txt ? n->txt : "(null)");
        U32 sz = COMP_TYPE_SIZE(n->dtype);
        if (sz == 0) sz = 4;

        if(n->child_count > 0 && n->children[0] && n->children[0]->ntype == CNODE_STR_LIT) {
            /* String literal initializer: emit as label with DB */
            EMIT(gs, "%s DB \"%s\", 0\n", n->txt ? n->txt : "_str", n->children[0]->txt ? n->children[0]->txt : "");
            continue;
        }
        PU8 decl_type = (sz == 1) ? "DB" : (sz == 2) ? "DW" : "DD";
        if (n->child_count > 0 && n->children[0] && n->children[0]->ntype == CNODE_INT_LIT)
            EMIT(gs, "%s %s %u\n", n->txt ? n->txt : "_data", decl_type, n->children[0]->ival);
        else
            EMIT(gs, "%s %s 0\n", n->txt ? n->txt : "_data", decl_type);
    }

    /* Phase 2: functions in .code section */
    EMIT(gs, "\n.CODE\n");
    EMIT(gs, "jmp _start\n\n");  /* entry point */

    for (U32 i = 0; i < program->child_count; i++) {
        PCNODE n = program->children[i];
        if (!n || n->ntype != CNODE_FUNC_DECL) continue;
        gen_function(gs, n);
    }

    /* Phase 3: rodata strings at end of .code section */
    if (gs->ctx->rodata_string_count > 0) {
        EMIT(gs, "\n; Read-only string literals\n");
        EMIT(gs, ".RODATA\n");
        U32 rc = gs->ctx->rodata_string_count;
        for (U32 i = 0; i < rc; i++) {
            RODATA_STR *rs = pop_rodata_string(gs);
            if (!rs) break;
            PU8 txt = rs->node && rs->node->txt ? rs->node->txt : "";
            DEBUGM_PRINTF("[COMP GEN] Emitting rodata string label _S%u: \"%s\"\n", rs->label, txt);
            EMIT(gs, "_S%u DB \"%s\", 0\n", rs->label, txt);
        }
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  PUBLIC API
 * ════════════════════════════════════════════════════════════════════════════ */
BOOL COMP_GEN(PCNODE root, PCOMP_CTX ctx) {
    if (!root || !ctx || !ctx->out_asm) return FALSE;
    if(FILE_EXISTS(ctx->out_asm)) {
        if(!FILE_DELETE(ctx->out_asm)) {
            printf("[COMP GEN] Cannot delete existing output '%s'\n", ctx->out_asm);
            return FALSE;
        }
    }
    FILE_CREATE(ctx->out_asm);
    FILE *out = FOPEN(ctx->out_asm, MODE_A | MODE_FAT32);
    if (!out) {
        printf("[COMP GEN] Cannot open output '%s'\n", ctx->out_asm);
        return FALSE;
    }

    /* Emit file header */
    {
        U8 hdr[] = "; Generated by AstraC Compiler\n"
                   "; DO NOT EDIT - auto-generated assembly output\n\n";
        FWRITE(out, hdr, sizeof(hdr) - 1);
    }

    GEN_STATE gs;
    MEMZERO(&gs, sizeof(GEN_STATE));
    gs.out = out;
    gs.ctx = ctx;

    gen_toplevel(&gs, root);

    FCLOSE(out);
    return TRUE;
}
