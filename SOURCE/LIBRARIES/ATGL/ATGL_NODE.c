/*+++
    LIBRARIES/ATGL/ATGL_NODE.c - Node tree operations

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#include <LIBRARIES/ATGL/ATGL_INTERNAL.h>
#include <STD/MEM.h>
#include <STD/STRING.h>

/* ================================================================ */
/*                       CREATE / DESTROY                           */
/* ================================================================ */

PATGL_NODE ATGL_NODE_CREATE(ATGL_NODE_TYPE type, PATGL_NODE parent,
                            ATGL_RECT rect)
{
    PATGL_NODE node = (PATGL_NODE)CAlloc(1, sizeof(ATGL_NODE));
    if (!node) return NULLPTR;

    node->type      = type;
    node->id        = atgl.next_id++;
    node->rect      = rect;
    node->abs_rect  = rect;
    node->visible   = TRUE;
    node->enabled   = TRUE;
    node->focused   = FALSE;
    node->dirty     = TRUE;
    node->focusable = FALSE;
    node->fg        = VBE_BLACK;
    node->bg        = VBE_SEE_THROUGH;
    node->parent    = NULLPTR;
    node->child_count = 0;
    node->fn_render  = NULLPTR;
    node->fn_event   = NULLPTR;
    node->fn_destroy = NULLPTR;
    node->on_click   = NULLPTR;
    node->on_change  = NULLPTR;

    MEMSET(node->text, 0, ATGL_NODE_MAX_TEXT);

    if (parent) {
        ATGL_NODE_ADD_CHILD(parent, node);
    }

    return node;
}

BOOL ATGL_NODE_ADD_CHILD(PATGL_NODE parent, PATGL_NODE child)
{
    if (!parent || !child) return FALSE;
    if (parent->child_count >= ATGL_MAX_CHILDREN) return FALSE;

    child->parent = parent;
    parent->children[parent->child_count++] = child;
    parent->dirty = TRUE;
    return TRUE;
}

BOOL ATGL_NODE_REMOVE_CHILD(PATGL_NODE parent, PATGL_NODE child)
{
    if (!parent || !child) return FALSE;

    for (U32 i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            for (U32 j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->children[--parent->child_count] = NULLPTR;
            child->parent = NULLPTR;
            parent->dirty = TRUE;
            return TRUE;
        }
    }
    return FALSE;
}

VOID ATGL_NODE_DESTROY(PATGL_NODE node)
{
    if (!node) return;

    /* Destroy children first (depth-first) */
    for (U32 i = 0; i < node->child_count; i++) {
        ATGL_NODE_DESTROY(node->children[i]);
        node->children[i] = NULLPTR;
    }
    node->child_count = 0;

    /* Call widget-specific destructor */
    if (node->fn_destroy) {
        node->fn_destroy(node);
    }

    /* Remove from parent */
    if (node->parent) {
        ATGL_NODE_REMOVE_CHILD(node->parent, node);
    }

    /* Clear focus if this node owned it */
    if (atgl.focus == node) {
        atgl.focus = NULLPTR;
    }

    MFree(node);
}

/* ================================================================ */
/*                         SETTERS                                  */
/* ================================================================ */

VOID ATGL_NODE_SET_TEXT(PATGL_NODE node, PU8 text)
{
    if (!node) return;

    if (text) {
        U32 len = STRLEN(text);
        if (len >= ATGL_NODE_MAX_TEXT) len = ATGL_NODE_MAX_TEXT - 1;
        MEMCPY(node->text, text, len);
        node->text[len] = '\0';
    } else {
        node->text[0] = '\0';
    }
    node->dirty = TRUE;
}

VOID ATGL_NODE_SET_VISIBLE(PATGL_NODE node, BOOL visible)
{
    if (!node) return;
    node->visible = visible;
    node->dirty   = TRUE;
    if (node->parent) node->parent->dirty = TRUE;
}

VOID ATGL_NODE_SET_ENABLED(PATGL_NODE node, BOOL enabled)
{
    if (!node) return;
    node->enabled = enabled;
    node->dirty   = TRUE;
}

VOID ATGL_NODE_SET_RECT(PATGL_NODE node, ATGL_RECT rect)
{
    if (!node) return;
    node->rect  = rect;
    node->dirty = TRUE;
    if (node->parent) node->parent->dirty = TRUE;
}

VOID ATGL_NODE_SET_COLORS(PATGL_NODE node, VBE_COLOUR fg, VBE_COLOUR bg)
{
    if (!node) return;
    node->fg    = fg;
    node->bg    = bg;
    node->dirty = TRUE;
}

VOID ATGL_NODE_INVALIDATE(PATGL_NODE node)
{
    if (!node) return;
    node->dirty = TRUE;
}

/* ================================================================ */
/*                         QUERIES                                  */
/* ================================================================ */

PATGL_NODE ATGL_NODE_FIND_BY_ID(PATGL_NODE root, U32 id)
{
    if (!root) return NULLPTR;
    if (root->id == id) return root;

    for (U32 i = 0; i < root->child_count; i++) {
        PATGL_NODE found = ATGL_NODE_FIND_BY_ID(root->children[i], id);
        if (found) return found;
    }
    return NULLPTR;
}

/* ================================================================ */
/*                    INTERNAL HELPERS                              */
/* ================================================================ */

VOID atgl_compute_abs_rect(PATGL_NODE node, I32 parent_x, I32 parent_y)
{
    if (!node) return;
    node->abs_rect.x = parent_x + node->rect.x;
    node->abs_rect.y = parent_y + node->rect.y;
    node->abs_rect.w = node->rect.w;
    node->abs_rect.h = node->rect.h;
}

PATGL_NODE atgl_hit_test_recursive(PATGL_NODE node, I32 x, I32 y)
{
    if (!node || !node->visible || !node->enabled) return NULLPTR;
    if (!ATGL_RECT_CONTAINS(&node->abs_rect, x, y)) return NULLPTR;

    /* Check children in reverse order (topmost first) */
    for (I32 i = (I32)node->child_count - 1; i >= 0; i--) {
        PATGL_NODE hit = atgl_hit_test_recursive(node->children[i], x, y);
        if (hit) return hit;
    }

    /* Return this node if focusable or has an event handler */
    if (node->focusable || node->fn_event) return node;
    return NULLPTR;
}

VOID atgl_collect_focusable(PATGL_NODE node,
                            PATGL_NODE *out, U32 *count, U32 max)
{
    if (!node || !node->visible || !node->enabled || *count >= max) return;

    if (node->focusable) {
        out[(*count)++] = node;
    }

    for (U32 i = 0; i < node->child_count; i++) {
        atgl_collect_focusable(node->children[i], out, count, max);
    }
}
