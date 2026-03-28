/*+++
    LIBRARIES/ATGL/ATGL.c - Screen management, initialisation, and drawing helpers

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#define __RTOS__
#include <STD/MEM.h>
#include <STD/STRING.h>
#include <STD/PROC_COM.h>
#include <STD/DEBUG.h>
#include <LIBRARIES/ATGL/ATGL_INTERNAL.h>
#include <STD/GRAPHICS.h>
#include <DRIVERS/VESA/VBE.h>
#undef __RTOS__
/* ================================================================ */
/*                         GLOBAL STATE                             */
/* ================================================================ */

ATGL_STATE atgl ATTRIB_DATA = { 0 };

/* ================================================================ */
/*                      SCREEN MANAGEMENT                           */
/* ================================================================ */

VOID ATGL_CREATE_SCREEN(ATGL_SCREEN_ATTRIBS attrs)
{
    if (atgl.initialized) return;

    VBE_MODEINFO *mode = GET_VBE_MODE();
    atgl.width  = mode ? mode->XResolution : ATGL_SCREEN_WIDTH;
    atgl.height = mode ? mode->YResolution : ATGL_SCREEN_HEIGHT;
    atgl.attrs  = attrs;
    atgl.theme  = ATGL_DEFAULT_THEME();
    atgl.quit   = FALSE;
    atgl.focus  = NULLPTR;
    atgl.next_id = 1;

    /* Create root node spanning the entire screen */
    atgl.root = ATGL_NODE_CREATE(
        ATGL_NODE_ROOT, NULLPTR,
        (ATGL_RECT){ 0, 0, (I32)atgl.width, (I32)atgl.height }
    );
    if (!atgl.root) return;

    atgl.root->fg      = atgl.theme.fg;
    atgl.root->bg      = atgl.theme.bg;
    atgl.root->visible = TRUE;
    atgl.root->enabled = TRUE;

    /* Clear screen unless SA_NO_CLEAR requested */
    if (!(attrs & ATGL_SA_NO_CLEAR)) {
        CLEAR_SCREEN_COLOUR(atgl.theme.bg);
    }

    /* Reset event polling state */
    atgl.last_kb_seq     = 0;
    atgl.last_ms_seq     = 0;
    atgl.last_mouse_x    = 0;
    atgl.last_mouse_y    = 0;
    atgl.last_btn_left   = FALSE;
    atgl.last_btn_right  = FALSE;
    atgl.last_btn_middle = FALSE;
}

VOID ATGL_INIT(VOID)
{
    atgl.initialized = TRUE;

    /* ---- Initialise mouse cursor save/restore ---- */
    atgl.cursor.width     = ATGL_CURSOR_WIDTH;  // 8
    atgl.cursor.height    = ATGL_CURSOR_HEIGHT; // 12
    atgl.cursor.x         = 0;
    atgl.cursor.y         = 0;
    atgl.cursor.has_saved = FALSE;
    VBE_MODEINFO *mode = GET_VBE_MODE();
    if (mode) {
        atgl.bpp = (mode->BitsPerPixel + 7) / 8;
        DEBUG_PRINTF("[ATGL] VBE mode detected: %dx%d, %d bpp, BytesPerScanLine: %d\n",
                     mode->XResolution, mode->YResolution, mode->BitsPerPixel, mode->BytesPerScanLine);
        atgl.cursor.stride = mode->BytesPerScanLine;
    } else {
        atgl.cursor.stride = atgl.width * atgl.bpp; // fallback 
    }
    

    /* Get the process framebuffer pointer from TCB */
    TCB *tcb = GET_CURRENT_TCB();
    if (tcb && tcb->framebuffer_virt) {
        atgl.cursor.framebuffer = (VBE_PIXEL_COLOUR *)tcb->framebuffer_virt;
            /* Always allocate buffer for 8x12 cursor */
            atgl.cursor.previous_buffer = MAlloc(
                atgl.cursor.width * atgl.cursor.height * sizeof(VBE_PIXEL_COLOUR));
    } else {
        atgl.cursor.framebuffer     = NULLPTR;
        atgl.cursor.previous_buffer = NULLPTR;
    }

    DEBUG_PRINTF("[ATGL] Initialized (%dx%d)\n", atgl.width, atgl.height);
}

VOID ATGL_DESTROY_SCREEN(VOID)
{
    if (!atgl.initialized) return;

    if (atgl.root) {
        ATGL_NODE_DESTROY(atgl.root);
        atgl.root = NULLPTR;
    }

    /* Free cursor save buffer */
    if (atgl.cursor.previous_buffer) {
        MFree(atgl.cursor.previous_buffer);
        atgl.cursor.previous_buffer = NULLPTR;
    }
    atgl.cursor.framebuffer = NULLPTR;
    atgl.cursor.has_saved   = FALSE;

    atgl.initialized = FALSE;
    atgl.focus = NULLPTR;
    DEBUG_PRINTF("[ATGL] Screen destroyed\n");
}

PATGL_NODE ATGL_GET_SCREEN_ROOT_NODE(VOID) { return atgl.root;   }
U32        ATGL_GET_SCREEN_WIDTH(VOID)     { return atgl.width;  }
U32        ATGL_GET_SCREEN_HEIGHT(VOID)    { return atgl.height; }

/* ================================================================ */
/*                            QUIT                                  */
/* ================================================================ */

VOID ATGL_QUIT(VOID)        { atgl.quit = TRUE;  }
BOOL ATGL_SHOULD_QUIT(VOID) { return atgl.quit;  }

/* ================================================================ */
/*                     DRAWING HELPERS                              */
/* ================================================================ */

VOID ATGL_DRAW_RAISED(I32 x, I32 y, I32 w, I32 h)
{
    VBE_COLOUR hi = atgl.theme.highlight;
    VBE_COLOUR sh = atgl.theme.shadow;
    VBE_COLOUR dk = atgl.theme.dark_shadow;

    /* Outer highlight: top and left */
    DRAW_LINE(x, y, x + w - 1, y, hi);
    DRAW_LINE(x, y, x, y + h - 1, hi);

    /* Outer shadow: bottom and right */
    DRAW_LINE(x + w - 1, y, x + w - 1, y + h - 1, dk);
    DRAW_LINE(x, y + h - 1, x + w - 1, y + h - 1, dk);

    /* Inner shadow (1 px inset, bottom-right) */
    if (w > 2 && h > 2) {
        DRAW_LINE(x + w - 2, y + 1, x + w - 2, y + h - 2, sh);
        DRAW_LINE(x + 1, y + h - 2, x + w - 2, y + h - 2, sh);
    }
}

VOID ATGL_DRAW_SUNKEN(I32 x, I32 y, I32 w, I32 h)
{
    VBE_COLOUR hi = atgl.theme.highlight;
    VBE_COLOUR sh = atgl.theme.shadow;
    VBE_COLOUR dk = atgl.theme.dark_shadow;

    /* Outer shadow: top and left */
    DRAW_LINE(x, y, x + w - 1, y, sh);
    DRAW_LINE(x, y, x, y + h - 1, sh);

    /* Inner dark (1 px inset, top-left) */
    if (w > 2 && h > 2) {
        DRAW_LINE(x + 1, y + 1, x + w - 2, y + 1, dk);
        DRAW_LINE(x + 1, y + 1, x + 1, y + h - 2, dk);
    }

    /* Outer highlight: bottom and right */
    DRAW_LINE(x + w - 1, y, x + w - 1, y + h - 1, hi);
    DRAW_LINE(x, y + h - 1, x + w - 1, y + h - 1, hi);
}

VOID ATGL_DRAW_TEXT(I32 x, I32 y, PU8 text, VBE_COLOUR fg, VBE_COLOUR bg)
{
    if (!text) return;
    DRAW_8x8_STRING(x, y, text, fg, bg);
}

VOID ATGL_DRAW_TEXT_CLIPPED(I32 x, I32 y, I32 max_w, PU8 text,
                            VBE_COLOUR fg, VBE_COLOUR bg)
{
    if (!text) return;

    U32 char_w    = atgl.theme.char_w;
    U32 max_chars = (U32)max_w / char_w;
    U32 len       = STRLEN(text);

    if (len <= max_chars) {
        /* Whole string fits — single batched syscall */
        DRAW_8x8_STRING(x, y, text, fg, bg);
    } else {
        /* Truncate into a stack buffer and draw in one call instead of
           issuing a per-character syscall for every visible glyph. */
        CHAR buf[ATGL_NODE_MAX_TEXT];
        if (max_chars >= ATGL_NODE_MAX_TEXT) max_chars = ATGL_NODE_MAX_TEXT - 1;
        MEMCPY(buf, text, max_chars);
        buf[max_chars] = '\0';
        DRAW_8x8_STRING(x, y, (PU8)buf, fg, bg);
    }
}

/* ================================================================ */
/*                      LAYOUT HELPERS                              */
/* ================================================================ */

VOID ATGL_LAYOUT_APPLY(PATGL_NODE panel)
{
    if (!panel || panel->type != ATGL_NODE_PANEL) return;

    ATGL_PANEL_DATA *pd = &panel->data.panel;
    I32 pad   = (I32)pd->padding;
    I32 space = (I32)pd->spacing;
    BOOL is_vert = (pd->layout == ATGL_LAYOUT_VERTICAL);
    BOOL is_horz = (pd->layout == ATGL_LAYOUT_HORIZONTAL);
    I32 offset = pad;

    for (U32 i = 0; i < panel->child_count; i++) {
        PATGL_NODE child = panel->children[i];
        if (!child->visible) continue;

        if (is_vert) {
            child->rect.x = pad;
            child->rect.y = offset;
            offset += child->rect.h + space;
        } else if (is_horz) {
            child->rect.x = offset;
            child->rect.y = pad;
            offset += child->rect.w + space;
        }
        ATGL_NODE_INVALIDATE(child);
    }
}

VOID ATGL_CENTER_IN_PARENT(PATGL_NODE node)
{
    if (!node || !node->parent) return;
    PATGL_NODE p = node->parent;
    node->rect.x = (p->rect.w - node->rect.w) / 2;
    node->rect.y = (p->rect.h - node->rect.h) / 2;
    ATGL_NODE_INVALIDATE(node);
}

VOID ATGL_CENTER_IN_SCREEN(PATGL_NODE node)
{
    if (!node) return;
    node->rect.x = ((I32)atgl.width  - node->rect.w) / 2;
    node->rect.y = ((I32)atgl.height - node->rect.h) / 2;
    ATGL_NODE_INVALIDATE(node);
}
