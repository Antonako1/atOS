/*+++
    LIBRARIES/ATGL/ATGL_EVENT.c - Event polling, dispatch, focus, and rendering

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#include <LIBRARIES/ATGL/ATGL_INTERNAL.h>
#include <STD/GRAPHICS.h>
#include <STD/IO.h>
#include <STD/PROC_COM.h>
#include <STD/MEM.h>
#include <STD/DEBUG.h>

/* ================================================================ */
/*                       EVENT POLLING                              */
/* ================================================================ */

BOOL ATGL_POLL_EVENTS(ATGL_EVENT *ev)
{
    if (!ev) return FALSE;
    MEMSET(ev, 0, sizeof(ATGL_EVENT));

    /* ---- Keyboard ---- */
    PS2_KB_DATA *kb = kb_poll();
    if (kb && kb->seq != atgl.last_kb_seq) {
        atgl.last_kb_seq = kb->seq;

        ev->type = kb->cur.pressed ? ATGL_EVT_KEY_DOWN : ATGL_EVT_KEY_UP;
        ev->key.keycode = (U32)kb->cur.keycode;

        /* Map scan code to ASCII, handle shift for letters */
        U8 ch = keypress_to_char((U32)kb->cur.keycode);
        if (kb->mods.shift && ch >= 'a' && ch <= 'z') {
            ch = ch - 'a' + 'A';
        }
        ev->key.ch    = ch;
        ev->key.shift = kb->mods.shift;
        ev->key.ctrl  = kb->mods.ctrl;
        ev->key.alt   = kb->mods.alt;
        ev->handled   = FALSE;
        return TRUE;
    }

    /* ---- Mouse ---- */
    PS2_MOUSE_DATA *ms = mouse_poll();
    if (ms && ms->seq != atgl.last_ms_seq) {
        atgl.last_ms_seq = ms->seq;
        BOOL bl = ms->cur.mouse1;
        BOOL br = ms->cur.mouse2;
        BOOL bm = ms->cur.mouse3;
        I32  mx = (I32)ms->cur.x;
        I32  my = (I32)ms->cur.y;

        /* Determine event type */
        ev->type = ATGL_EVT_MOUSE_MOVE;        /* default */

        if (bl && !atgl.last_btn_left)  ev->type = ATGL_EVT_MOUSE_DOWN;
        if (!bl && atgl.last_btn_left)  ev->type = ATGL_EVT_MOUSE_UP;
        if (br && !atgl.last_btn_right) ev->type = ATGL_EVT_MOUSE_DOWN;

        ev->mouse.x          = mx;
        ev->mouse.y          = my;
        ev->mouse.btn_left   = bl;
        ev->mouse.btn_right  = br;
        ev->mouse.btn_middle = bm;
        ev->handled          = FALSE;

        atgl.last_mouse_x    = mx;
        atgl.last_mouse_y    = my;
        atgl.last_btn_left   = bl;
        atgl.last_btn_right  = br;
        atgl.last_btn_middle = bm;
        return TRUE;
    }
    return FALSE;
}

/* ================================================================ */
/*                      EVENT DISPATCH                              */
/* ================================================================ */

static VOID dispatch_recursive(PATGL_NODE node, ATGL_EVENT *ev)
{
    if (!node || !node->visible || !node->enabled || ev->handled) return;

    /* Children first (depth-first, front nodes have priority) */
    for (U32 i = 0; i < node->child_count; i++) {
        dispatch_recursive(node->children[i], ev);
        if (ev->handled) return;
    }

    /* This node */
    if (node->fn_event) {
        node->fn_event(node, ev);
    }
}

VOID ATGL_DISPATCH_EVENT(PATGL_NODE root, ATGL_EVENT *ev)
{
    if (!root || !ev) return;

    /* ---- Global: Tab cycles focus ---- */
    if (ev->type == ATGL_EVT_KEY_DOWN && ev->key.keycode == KEY_TAB) {
        if (ev->key.shift)
            ATGL_PREV_FOCUS();
        else
            ATGL_NEXT_FOCUS();
        ev->handled = TRUE;
        return;
    }

    /* ---- Key events → focused node first ---- */
    if (ev->type == ATGL_EVT_KEY_DOWN || ev->type == ATGL_EVT_KEY_UP) {
        if (atgl.focus && atgl.focus->fn_event) {
            atgl.focus->fn_event(atgl.focus, ev);
            if (ev->handled) return;
        }
    }

    /* ---- Mouse down / click → hit-test & set focus ---- */
    if (ev->type == ATGL_EVT_MOUSE_DOWN) {
        PATGL_NODE hit = atgl_hit_test_recursive(root,
                                                 ev->mouse.x,
                                                 ev->mouse.y);
        if (hit && hit->focusable) {
            ATGL_SET_FOCUS(hit);
        }
    }

    /* ---- Mouse up → dispatch + synthesise click ---- */
    if (ev->type == ATGL_EVT_MOUSE_UP) {
        dispatch_recursive(root, ev);
        if (!ev->handled) {
            ATGL_EVENT click = *ev;
            click.type    = ATGL_EVT_MOUSE_CLICK;
            click.handled = FALSE;
            dispatch_recursive(root, &click);
        }
        return;
    }

    /* ---- Everything else ---- */
    dispatch_recursive(root, ev);
}

/* ================================================================ */
/*                        RENDERING                                 */
/* ================================================================ */

static VOID draw_mouse_cursor(I32 x, I32 y)
{
    /* Define cursor dimensions */
    const I32 cursor_width = 8;
    const I32 cursor_height = 12;

    /* Draw a simple arrow cursor */
    for (I32 i = 0; i < cursor_height; i++) {
        for (I32 j = 0; j <= i && j < cursor_width; j++) {
            VBE_PIXEL_INFO pixel = CREATE_VBE_PIXEL_INFO(x + j, y + i, atgl.theme.fg);
            DRAW_PIXEL(pixel); /* Draw pixel in foreground color */
        }
    }
}

static VOID render_recursive(PATGL_NODE node, I32 px, I32 py)
{
    if (!node || !node->visible) return;

    /* Compute absolute position from parent origin */
    atgl_compute_abs_rect(node, px, py);

    /* Render this node */
    if (node->fn_render) {
        node->fn_render(node);
    }

    /* Render children */
    for (U32 i = 0; i < node->child_count; i++) {
        render_recursive(node->children[i],
                         node->abs_rect.x, node->abs_rect.y);
    }

    node->dirty = FALSE;
}

VOID ATGL_RENDER_TREE(PATGL_NODE root)
{
    if (!root) return;

    /* Clear screen with theme background */
    CLEAR_SCREEN_COLOUR(atgl.theme.bg);
    
    /* Recursive render from (0, 0) */
    render_recursive(root, 0, 0);
    
    /* Draw mouse cursor on top of everything */
    draw_mouse_cursor(atgl.last_mouse_x, atgl.last_mouse_y);
    
    /* Push to display */
    FLUSH_VRAM();
}

/* ================================================================ */
/*                     FOCUS MANAGEMENT                             */
/* ================================================================ */

#define ATGL_MAX_FOCUSABLE 128

VOID ATGL_SET_FOCUS(PATGL_NODE node)
{
    if (atgl.focus == node) return;

    if (atgl.focus) {
        atgl.focus->focused = FALSE;
        atgl.focus->dirty   = TRUE;
    }

    atgl.focus = node;

    if (node) {
        node->focused = TRUE;
        node->dirty   = TRUE;
    }
}

PATGL_NODE ATGL_GET_FOCUS(VOID)
{
    return atgl.focus;
}

VOID ATGL_NEXT_FOCUS(VOID)
{
    if (!atgl.root) return;

    PATGL_NODE list[ATGL_MAX_FOCUSABLE];
    U32 count = 0;
    atgl_collect_focusable(atgl.root, list, &count, ATGL_MAX_FOCUSABLE);
    if (count == 0) return;

    if (!atgl.focus) {
        ATGL_SET_FOCUS(list[0]);
        return;
    }

    for (U32 i = 0; i < count; i++) {
        if (list[i] == atgl.focus) {
            ATGL_SET_FOCUS(list[(i + 1) % count]);
            return;
        }
    }
    ATGL_SET_FOCUS(list[0]);
}

VOID ATGL_PREV_FOCUS(VOID)
{
    if (!atgl.root) return;

    PATGL_NODE list[ATGL_MAX_FOCUSABLE];
    U32 count = 0;
    atgl_collect_focusable(atgl.root, list, &count, ATGL_MAX_FOCUSABLE);
    if (count == 0) return;

    if (!atgl.focus) {
        ATGL_SET_FOCUS(list[count - 1]);
        return;
    }

    for (U32 i = 0; i < count; i++) {
        if (list[i] == atgl.focus) {
            ATGL_SET_FOCUS(list[i == 0 ? count - 1 : i - 1]);
            return;
        }
    }
    ATGL_SET_FOCUS(list[count - 1]);
}
