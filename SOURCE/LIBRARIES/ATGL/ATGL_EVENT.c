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

    /* Children in reverse order (topmost / last-added has priority) */
    for (I32 i = (I32)node->child_count - 1; i >= 0; i--) {
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

/* ================================================================ */
/*                   MOUSE CURSOR SAVE/RESTORE                      */
/* ================================================================ */

/* Classic arrow cursor defined as two bitmasks per row (8 wide × 12 tall).
   Bit 7 = leftmost pixel, bit 0 = rightmost pixel.

   Visual layout (B = black outline, W = white fill, . = transparent):
     Row  0: B . . . . . . .
     Row  1: B B . . . . . .
     Row  2: B W B . . . . .
     Row  3: B W W B . . . .
     Row  4: B W W W B . . .
     Row  5: B W W W W B . .
     Row  6: B W W W W W B .
     Row  7: B W W B B B B .
     Row  8: B B W B . . . .
     Row  9: . B B W B . . .
     Row 10: . . . B W B . .
     Row 11: . . . . B . . .                                       */

static const U8 cursor_black[ATGL_CURSOR_HEIGHT] ATTRIB_RODATA = {
    0b10000000, 
    0b11000000, 
    0b10100000, 
    0b10010000, 
    0b10001000, 
    0b10000100,
    0b10000010, 
    0b10011110, 
    0b11010000, 
    0b01101000, 
    0b00010100, 
    0b00001000,
};

static const U8 cursor_white[ATGL_CURSOR_HEIGHT] ATTRIB_RODATA = {
    0b00000000, 
    0b00000000, 
    0b01000000, 
    0b01100000, 
    0b01110000, 
    0b01111000,
    0b01111100, 
    0b01100000, 
    0b00100000, 
    0b00010000, 
    0b00001000, 
    0b00000000,
};

static inline VBE_PIXEL_COLOUR read_pixel(U32 sx, U32 sy) {
    U8 *fb = (U8 *)atgl.cursor.framebuffer;
    U32 offset = sy * atgl.cursor.stride + sx * atgl.bpp;
    if (atgl.bpp == 3) {
        return (VBE_PIXEL_COLOUR)(fb[offset] | (fb[offset+1] << 8) | (fb[offset+2] << 16));
    } else {
        return *(VBE_PIXEL_COLOUR *)(fb + offset);
    }
}

static inline VOID write_pixel(U32 sx, U32 sy, VBE_PIXEL_COLOUR color) {
    U8 *fb = (U8 *)atgl.cursor.framebuffer;
    U32 offset = sy * atgl.cursor.stride + sx * atgl.bpp;
    if (atgl.bpp == 3) {
        fb[offset]   = color & 0xFF;
        fb[offset+1] = (color >> 8) & 0xFF;
        fb[offset+2] = (color >> 16) & 0xFF;
    } else {
        *(VBE_PIXEL_COLOUR *)(fb + offset) = color;
    }
}

/*  Restore the framebuffer pixels that were under the cursor before
    it was drawn.  Call BEFORE any rendering so the scene is clean. */
static VOID atgl_cursor_hide(VOID)
{
    ATGL_CURSOR *c = &atgl.cursor;
    if (!c->has_saved || !c->previous_buffer || !c->framebuffer) return;

    VBE_PIXEL_COLOUR *saved = (VBE_PIXEL_COLOUR *)c->previous_buffer;
    U32 idx = 0;

    for (U32 row = 0; row < c->height; row++) {
        U32 sy = c->y + row;          /* screen-space Y */
        if (sy >= atgl.height) break;  /* clipped off bottom */
        for (U32 col = 0; col < c->width; col++) {
            U32 sx = c->x + col;      /* screen-space X */
            if (sx >= atgl.width) { idx++; continue; }
            write_pixel(sx, sy, saved[idx]);
            idx++;
        }
    }
    c->has_saved = FALSE;
}

/*  Save the screen content at (mx, my) into previous_buffer, then
    draw the arrow cursor on the framebuffer. */
static VOID atgl_cursor_show(I32 mx, I32 my)
{
    ATGL_CURSOR *c = &atgl.cursor;
    if (!c->previous_buffer || !c->framebuffer) return;

    c->prev_x = c->x;
    c->prev_y = c->y;
    c->x = (U32)mx;
    c->y = (U32)my;

    VBE_PIXEL_COLOUR *saved = (VBE_PIXEL_COLOUR *)c->previous_buffer;
    U32 idx = 0;

    /* Step 1 — save the entire bounding box from the framebuffer */
    for (U32 row = 0; row < c->height; row++) {
        U32 sy = c->y + row;
        for (U32 col = 0; col < c->width; col++) {
            U32 sx = c->x + col;
            if (sy < atgl.height && sx < atgl.width)
                saved[idx] = read_pixel(sx, sy);
            else
                saved[idx] = 0;
            idx++;
        }
    }

    /* Step 2 — draw the cursor from bitmap masks */
    for (U32 row = 0; row < ATGL_CURSOR_HEIGHT; row++) {
        U32 sy = c->y + row;
        if (sy >= atgl.height) break;
        U8 black_bits = cursor_black[row];
        U8 white_bits = cursor_white[row];
        for (U32 col = 0; col < ATGL_CURSOR_WIDTH; col++) {
            U32 sx = c->x + col;
            if (sx >= atgl.width) continue;
            U8 mask = 0x80 >> col;
            if (black_bits & mask)
                write_pixel(sx, sy, (VBE_PIXEL_COLOUR)VBE_BLACK);
            else if (white_bits & mask)
                write_pixel(sx, sy, (VBE_PIXEL_COLOUR)VBE_WHITE);
        }
    }

    c->has_saved = TRUE;
}

static VOID render_recursive(PATGL_NODE node, I32 px, I32 py, BOOL force)
{
    if (!node || !node->visible) return;

    /* Compute absolute position from parent origin */
    atgl_compute_abs_rect(node, px, py);

    BOOL render_this = force || node->dirty;

    /* With upward dirty propagation (ATGL_NODE_INVALIDATE), a clean
       node guarantees all descendants are also clean — skip the
       entire subtree in O(1). */
    if (!render_this) return;

    /* Render this node */
    if (node->fn_render) {
        node->fn_render(node);
    }

    /* Render children */
    for (U32 i = 0; i < node->child_count; i++) {
        render_recursive(node->children[i],
                         node->abs_rect.x, node->abs_rect.y, render_this);
    }

    node->dirty = FALSE;
}

VOID ATGL_RENDER_TREE(PATGL_NODE root)
{
    if (!root) return;

    /* 1. Restore framebuffer under the old cursor position so it
          does not interfere with rendering or get "baked in". */
    atgl_cursor_hide();  

    /* 2. Render dirty sub-trees (full clear only on first frame /
          theme change when root is dirty). */
    BOOL force = root->dirty;
    if (force) {
        CLEAR_SCREEN_COLOUR(atgl.theme.bg);
    }
    render_recursive(root, 0, 0, force);

    /* 3. Save the screen content at the new cursor position and draw
          the cursor on top.  This uses direct framebuffer access —
          no syscalls needed for the cursor itself. */
    atgl_cursor_show(atgl.last_mouse_x, atgl.last_mouse_y);

    /* 4. Push to display */
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
        ATGL_NODE_INVALIDATE(atgl.focus);
    }

    atgl.focus = node;

    if (node) {
        node->focused = TRUE;
        ATGL_NODE_INVALIDATE(node);
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
