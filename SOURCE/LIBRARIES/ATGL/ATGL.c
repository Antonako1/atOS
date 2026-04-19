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
    if (!atgl.initialized) {
        ATGL_INIT();
    };

    VBE_MODEINFO *mode = GET_VBE_MODE();
    atgl.width  = mode ? mode->XResolution : ATGL_SCREEN_WIDTH;
    atgl.height = mode ? mode->YResolution : ATGL_SCREEN_HEIGHT;
    atgl.attrs  = attrs;
    atgl.theme  = ATGL_DEFAULT_THEME();
    atgl.quit   = FALSE;
    atgl.focus  = NULLPTR;
    atgl.next_id = 1;
    atgl.needs_full_clear = TRUE;

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
        atgl.cursor.stride = mode->BytesPerScanLineLinear
                           ? mode->BytesPerScanLineLinear
                           : mode->BytesPerScanLine;
        atgl.width = mode->XResolution;
        atgl.height = mode->YResolution;
    } else {
        atgl.bpp = 4;
        atgl.width = ATGL_SCREEN_WIDTH;
        atgl.height = ATGL_SCREEN_HEIGHT;
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
/*              DIRECT FRAMEBUFFER RENDERING                        */
/* ================================================================ */
/* All helpers write to the per-process framebuffer obtained from
   TCB during ATGL_INIT.  They perform clipping against the screen
   dimensions and support 3-byte and 4-byte pixel formats.          */

static inline VOID fb_write_pixel_at(U8 *fb, U32 offset, VBE_COLOUR c, U32 bpp)
{
    if (bpp == 4) {
        *(U32 *)(fb + offset) = c;
    } else {
        fb[offset]     = (U8)(c & 0xFF);
        fb[offset + 1] = (U8)((c >> 8) & 0xFF);
        fb[offset + 2] = (U8)((c >> 16) & 0xFF);
    }
}

static inline VOID fb_fill_span24(U8 *dst, I32 pixels, VBE_COLOUR colour)
{
    U8 b0 = (U8)(colour & 0xFF);
    U8 b1 = (U8)((colour >> 8) & 0xFF);
    U8 b2 = (U8)((colour >> 16) & 0xFF);
    U32 bytes = (U32)pixels * 3;

    if (b0 == b1 && b1 == b2) {
        MEMSET_OPT(dst, b0, bytes);
        return;
    }

    {
        U8 pattern[192];
        U32 chunk = (U32)sizeof(pattern);
        for (U32 i = 0; i < chunk; i += 3) {
            pattern[i]     = b0;
            pattern[i + 1] = b1;
            pattern[i + 2] = b2;
        }

        while (bytes) {
            U32 copy = (bytes > chunk) ? chunk : bytes;
            MEMCPY_OPT(dst, pattern, copy);
            dst   += copy;
            bytes -= copy;
        }
    }
}

VOID atgl_fb_hline(I32 x, I32 y, I32 w, VBE_COLOUR colour)
{
    if (colour == VBE_SEE_THROUGH) return;
    U8 *fb = (U8 *)atgl.cursor.framebuffer;
    if (!fb) { DRAW_LINE(x, y, x + w - 1, y, colour); return; }

    /* Clip */
    if (y < 0 || (U32)y >= atgl.height || w <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (w <= 0) return;
    if ((U32)(x + w) > atgl.width) w = (I32)atgl.width - x;
    if (w <= 0) return;

    U32 bpp    = atgl.bpp;
    U32 stride = atgl.cursor.stride;
    U32 offset = (U32)y * stride + (U32)x * bpp;

    if (bpp == 4) {
        U32 *row = (U32 *)(fb + offset);
        MEMSET32_OPT(row, colour, (U32)w);
    } else {
        fb_fill_span24(fb + offset, w, colour);
    }
}

VOID atgl_fb_vline(I32 x, I32 y, I32 h, VBE_COLOUR colour)
{
    if (colour == VBE_SEE_THROUGH) return;
    U8 *fb = (U8 *)atgl.cursor.framebuffer;
    if (!fb) { DRAW_LINE(x, y, x, y + h - 1, colour); return; }

    if (x < 0 || (U32)x >= atgl.width || h <= 0) return;
    if (y < 0) { h += y; y = 0; }
    if (h <= 0) return;
    if ((U32)(y + h) > atgl.height) h = (I32)atgl.height - y;
    if (h <= 0) return;

    U32 bpp    = atgl.bpp;
    U32 stride = atgl.cursor.stride;
    U32 base   = (U32)y * stride + (U32)x * bpp;

    if (bpp == 4) {
        for (I32 i = 0; i < h; i++) {
            *(U32 *)(fb + base) = colour;
            base += stride;
        }
    } else {
        U8 b0 = (U8)(colour & 0xFF);
        U8 b1 = (U8)((colour >> 8) & 0xFF);
        U8 b2 = (U8)((colour >> 16) & 0xFF);
        for (I32 i = 0; i < h; i++) {
            fb[base] = b0; fb[base+1] = b1; fb[base+2] = b2;
            base += stride;
        }
    }
}

VOID atgl_fb_fill(I32 x, I32 y, I32 w, I32 h, VBE_COLOUR colour)
{
    if (colour == VBE_SEE_THROUGH) return;
    U8 *fb = (U8 *)atgl.cursor.framebuffer;
    if (!fb) { DRAW_FILLED_RECTANGLE(x, y, w, h, colour); return; }

    /* Clip */
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (w <= 0 || h <= 0) return;
    if ((U32)(x + w) > atgl.width)  w = (I32)atgl.width  - x;
    if ((U32)(y + h) > atgl.height) h = (I32)atgl.height - y;
    if (w <= 0 || h <= 0) return;

    U32 bpp    = atgl.bpp;
    U32 stride = atgl.cursor.stride;

    if (bpp == 4) {
        for (I32 row = 0; row < h; row++) {
            U32 *p = (U32 *)(fb + (U32)(y + row) * stride) + x;
            MEMSET32_OPT(p, colour, (U32)w);
        }
    } else {
        for (I32 row = 0; row < h; row++) {
            U8 *p = fb + (U32)(y + row) * stride + (U32)x * 3;
            fb_fill_span24(p, w, colour);
        }
    }
}

VOID atgl_fb_rect(I32 x, I32 y, I32 w, I32 h, VBE_COLOUR colour)
{
    if (w <= 0 || h <= 0) return;
    atgl_fb_hline(x, y, w, colour);               /* top    */
    atgl_fb_hline(x, y + h - 1, w, colour);       /* bottom */
    atgl_fb_vline(x, y + 1, h - 2, colour);       /* left   */
    atgl_fb_vline(x + w - 1, y + 1, h - 2, colour); /* right */
}

/* ================================================================ */
/*                     DRAWING HELPERS                              */
/* ================================================================ */

VOID ATGL_DRAW_RAISED(I32 x, I32 y, I32 w, I32 h)
{
    VBE_COLOUR hi = atgl.theme.highlight;
    VBE_COLOUR sh = atgl.theme.shadow;
    VBE_COLOUR dk = atgl.theme.dark_shadow;

    /* Outer highlight: top and left */
    atgl_fb_hline(x, y, w, hi);
    atgl_fb_vline(x, y, h, hi);

    /* Outer shadow: bottom and right */
    atgl_fb_vline(x + w - 1, y, h, dk);
    atgl_fb_hline(x, y + h - 1, w, dk);

    /* Inner shadow (1 px inset, bottom-right) */
    if (w > 2 && h > 2) {
        atgl_fb_vline(x + w - 2, y + 1, h - 2, sh);
        atgl_fb_hline(x + 1, y + h - 2, w - 2, sh);
    }
}

VOID ATGL_DRAW_SUNKEN(I32 x, I32 y, I32 w, I32 h)
{
    VBE_COLOUR hi = atgl.theme.highlight;
    VBE_COLOUR sh = atgl.theme.shadow;
    VBE_COLOUR dk = atgl.theme.dark_shadow;

    /* Outer shadow: top and left */
    atgl_fb_hline(x, y, w, sh);
    atgl_fb_vline(x, y, h, sh);

    /* Inner dark (1 px inset, top-left) */
    if (w > 2 && h > 2) {
        atgl_fb_hline(x + 1, y + 1, w - 2, dk);
        atgl_fb_vline(x + 1, y + 1, h - 2, dk);
    }

    /* Outer highlight: bottom and right */
    atgl_fb_vline(x + w - 1, y, h, hi);
    atgl_fb_hline(x, y + h - 1, w, hi);
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
        MEMCPY_OPT(buf, text, max_chars);
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

VOID ATGL_DRAW_PIXEL(U32 x, U32 y, VBE_COLOUR colour) {
    if (atgl.cursor.framebuffer && x < atgl.width && y < atgl.height) {
        fb_write_pixel_at((U8 *)atgl.cursor.framebuffer,
                          y * atgl.cursor.stride + x * atgl.bpp,
                          colour, atgl.bpp);
    } else {
        DRAW_PIXEL(CREATE_VBE_PIXEL_INFO(x, y, colour));
    }
}
VOID ATGL_DRAW_RECTANGLE(U32 x, U32 y, U32 w, U32 h, VBE_COLOUR colour) {
    atgl_fb_rect((I32)x, (I32)y, (I32)w, (I32)h, colour);
}
VOID ATGL_DRAW_FILLED_RECTANGLE(U32 x, U32 y, U32 w, U32 h, VBE_COLOUR colour) {
    atgl_fb_fill((I32)x, (I32)y, (I32)w, (I32)h, colour);
}
VOID ATGL_DRAW_LINE(U32 x1, U32 y1, U32 x2, U32 y2, VBE_COLOUR colour) {
    if (y1 == y2) {
        I32 x = (x1 < x2) ? (I32)x1 : (I32)x2;
        I32 w = (I32)((x1 < x2) ? x2 - x1 : x1 - x2) + 1;
        atgl_fb_hline(x, (I32)y1, w, colour);
    } else if (x1 == x2) {
        I32 y = (y1 < y2) ? (I32)y1 : (I32)y2;
        I32 h = (I32)((y1 < y2) ? y2 - y1 : y1 - y2) + 1;
        atgl_fb_vline((I32)x1, y, h, colour);
    } else {
        DRAW_LINE(x1, y1, x2, y2, colour);
    }
}
VOID ATGL_DRAW_FILLED_ELLIPSE(U32 x, U32 y, U32 rx, U32 ry, VBE_COLOUR colour) {
    DRAW_FILLED_ELLIPSE(x, y, rx, ry, colour);
}


VOID ATGL_DRAW_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour) {
    DRAW_TRIANGLE(x1, y1, x2, y2, x3, y3, colour);
}
VOID ATGL_DRAW_FILLED_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour) {
    DRAW_FILLED_TRIANGLE(x1, y1, x2, y2, x3, y3, colour);
}
