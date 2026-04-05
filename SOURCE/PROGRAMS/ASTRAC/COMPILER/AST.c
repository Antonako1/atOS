/*
 * ════════════════════════════════════════════════════════════════════════════
 *  COMPILER/AST.c  —  AST Node helpers
 *
 *  All nodes are heap-allocated. Children are stored in a dynamically-grown
 *  array inside each node (CNODE.children).  Nodes form a tree that is
 *  rooted at a CNODE_PROGRAM returned by COMP_PARSE().
 * ════════════════════════════════════════════════════════════════════════════
 */

#include "COMPILER.h"

/* ── Node growth factor ───────────────────────────────────────────────────── */
#define CHILD_INIT_CAP 4

/* ════════════════════════════════════════════════════════════════════════════
 *  CNODE_NEW  —  allocate a blank node
 * ════════════════════════════════════════════════════════════════════════════ */
PCNODE CNODE_NEW(CNODE_TYPE ntype, U32 line, U32 col) {
    PCNODE n = (PCNODE)MAlloc(sizeof(CNODE));
    if (!n) return NULLPTR;
    MEMZERO(n, sizeof(CNODE));
    n->ntype = ntype;
    n->line  = line;
    n->col   = col;
    return n;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  CNODE_ADD_CHILD  —  append a child pointer (grows the child array)
 * ════════════════════════════════════════════════════════════════════════════ */
VOID CNODE_ADD_CHILD(PCNODE parent, PCNODE child) {
    if (!parent) return;

    if (parent->child_count >= parent->child_cap) {
        U32 new_cap = parent->child_cap == 0 ? CHILD_INIT_CAP : parent->child_cap * 2;
        PCNODE *new_children = (PCNODE *)ReAlloc(parent->children,
                                                  new_cap * sizeof(PCNODE));
        if (!new_children) return;   /* OOM — silently drop, caller checks tree integrity */
        parent->children  = new_children;
        parent->child_cap = new_cap;
    }
    parent->children[parent->child_count++] = child;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  Convenience constructors for leaf nodes
 * ════════════════════════════════════════════════════════════════════════════ */

PCNODE CNODE_INT(U32 val, U32 line, U32 col) {
    PCNODE n = CNODE_NEW(CNODE_INT_LIT, line, col);
    if (!n) return NULLPTR;
    n->ival = val;
    /* Give it a U32 type by default — the verifier may narrow it later */
    n->dtype.base = CTYPE_U32;
    return n;
}

PCNODE CNODE_FLOAT(F32 val, U32 line, U32 col) {
    PCNODE n = CNODE_NEW(CNODE_FLOAT_LIT, line, col);
    if (!n) return NULLPTR;
    n->fval      = val;
    n->dtype.base = CTYPE_F32;
    return n;
}

PCNODE CNODE_STR(PU8 txt, U32 line, U32 col) {
    PCNODE n = CNODE_NEW(CNODE_STR_LIT, line, col);
    if (!n) return NULLPTR;
    n->txt        = txt ? STRDUP(txt) : NULLPTR;
    n->dtype.base = CTYPE_U8;
    n->dtype.ptr_depth = 1;   /* PU8 — pointer to char */
    return n;
}

PCNODE CNODE_IDENT_NODE(PU8 name, U32 line, U32 col) {
    PCNODE n = CNODE_NEW(CNODE_IDENT, line, col);
    if (!n) return NULLPTR;
    n->txt = name ? STRDUP(name) : NULLPTR;
    return n;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  DESTROY_CNODE  —  recursively free a node and all its children
 * ════════════════════════════════════════════════════════════════════════════ */
VOID DESTROY_CNODE(PCNODE node) {
    if (!node) return;
    /* Recurse into children */
    for (U32 i = 0; i < node->child_count; i++)
        DESTROY_CNODE(node->children[i]);
    if (node->children) MFree(node->children);
    if (node->txt)      MFree(node->txt);
    if (node->dtype.name) MFree(node->dtype.name);
    MFree(node);
}

VOID DESTROY_CNODE_TREE(PCNODE root) {
    DESTROY_CNODE(root);
}

/* ════════════════════════════════════════════════════════════════════════════
 *  COMP_TYPE_SIZE  —  sizeof in bytes for a given AC type
 * ════════════════════════════════════════════════════════════════════════════ */
U32 COMP_TYPE_SIZE(COMP_TYPE t) {
    /* Pointers are always 4 bytes (32-bit flat model) */
    if (t.ptr_depth > 0) return 4;
    switch (t.base) {
        case CTYPE_U8:    case CTYPE_I8:   case CTYPE_BOOL:  return 1;
        case CTYPE_U16:   case CTYPE_I16:                    return 2;

        case CTYPE_U32:   case CTYPE_I32:
        
        case CTYPE_PU8:   case CTYPE_PU16:
        case CTYPE_PU32:  case CTYPE_PPU8:
        case CTYPE_PPU16: case CTYPE_PPU32:
        case CTYPE_PI8:   case CTYPE_PI16:
        case CTYPE_PI32:  case CTYPE_PPI8:
        case CTYPE_PPI16: case CTYPE_PPI32:
        
        case CTYPE_F32:   case CTYPE_VOIDPTR:                return 4;
        
        case CTYPE_U0:                                       return 0;
        case CTYPE_STRUCT:
        case CTYPE_UNION:
        case CTYPE_ENUM:  return 0;  /* resolved during verification */
        default:          return 0;
    }
}
