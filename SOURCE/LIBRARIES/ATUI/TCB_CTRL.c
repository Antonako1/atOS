
#define ATUI_INTERNALS
#define __RTOS__
#include <DRIVERS/VESA/VBE.h>
#undef __RTOS__
#include <LIBRARIES/ATUI/ATUI_TYPES.h>
#include <LIBRARIES/ATUI/ATUI.h>
#include <STD/MEM.h>
#include <STD/BINARY.h>
#include <STD/GRAPHICS.h>

typedef struct {
    U8  *fb;
    U32 bpp;
    U32 stride;
    U32 width;
    U32 height;
} ATUI_FB_CTX;

static inline VOID atui_fb_init(ATUI_DISPLAY *d, ATUI_FB_CTX *ctx)
{
    VBE_MODEINFO *mode = GET_VBE_MODE();

    ctx->fb     = (d && d->t && d->t->framebuffer_virt) ? (U8 *)d->t->framebuffer_virt : NULLPTR;
    ctx->bpp    = mode ? (mode->BitsPerPixel + 7) / 8 : 0;
    ctx->stride = mode ? (mode->BytesPerScanLineLinear ? mode->BytesPerScanLineLinear
                                                        : mode->BytesPerScanLine)
                       : 0;
    ctx->width  = mode ? mode->XResolution : 0;
    ctx->height = mode ? mode->YResolution : 0;

    if (!ctx->fb || !ctx->stride || !ctx->bpp) {
        ctx->fb = NULLPTR;
    }
}

static inline VOID atui_fb_fill_span24(U8 *dst, U32 pixels, VBE_COLOUR colour)
{
    U8 b0 = (U8)(colour & 0xFF);
    U8 b1 = (U8)((colour >> 8) & 0xFF);
    U8 b2 = (U8)((colour >> 16) & 0xFF);
    U32 bytes = pixels * 3;

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

static inline VOID atui_fb_hspan(ATUI_FB_CTX *ctx, U32 x, U32 y, U32 w, VBE_COLOUR colour)
{
    if (!w) return;

    if (!ctx->fb || y >= ctx->height || x >= ctx->width) {
        DRAW_FILLED_RECTANGLE(x, y, w, 1, colour);
        return;
    }

    if (x + w > ctx->width) w = ctx->width - x;
    if (!w) return;

    U8 *row = ctx->fb + y * ctx->stride + x * ctx->bpp;

    if (ctx->bpp == 4) {
        MEMSET32_OPT((U32 *)row, colour, w);
    } else if (ctx->bpp == 3) {
        atui_fb_fill_span24(row, w, colour);
    } else {
        DRAW_FILLED_RECTANGLE(x, y, w, 1, colour);
    }
}

static inline VOID atui_fb_fill_rect(ATUI_FB_CTX *ctx, U32 x, U32 y, U32 w, U32 h, VBE_COLOUR colour)
{
    if (!w || !h) return;

    if (!ctx->fb || x >= ctx->width || y >= ctx->height) {
        DRAW_FILLED_RECTANGLE(x, y, w, h, colour);
        return;
    }

    if (x + w > ctx->width)  w = ctx->width - x;
    if (y + h > ctx->height) h = ctx->height - y;
    if (!w || !h) return;

    if (ctx->bpp == 4) {
        for (U32 row = 0; row < h; row++) {
            U32 *dst = (U32 *)(ctx->fb + (y + row) * ctx->stride) + x;
            MEMSET32_OPT(dst, colour, w);
        }
    } else if (ctx->bpp == 3) {
        for (U32 row = 0; row < h; row++) {
            U8 *dst = ctx->fb + (y + row) * ctx->stride + x * 3;
            atui_fb_fill_span24(dst, w, colour);
        }
    } else {
        DRAW_FILLED_RECTANGLE(x, y, w, h, colour);
    }
}

VOID ATUI_COPY_TO_TCB_FRAMEBUFFER() {
    ATUI_DISPLAY *d = GET_DISPLAY();
    if (!d || !d->t) return;

    ATUI_FB_CTX fb;
    atui_fb_init(d, &fb);

    const U32 cols = d->cols;
    const U32 rows = d->rows;

    const U32 char_w = d->font.char_width;   
    const U32 char_h = d->font.char_height;
    const U32 font_sz = d->font.file->sz;
    U8 *font_data = d->font.file->data;
    const BOOL cursor_visible = IS_FLAG_SET(d->cnf, ATUIC_CURSOR_VISIBLE);
    const BOOL insert_mode = IS_FLAG_SET(d->cnf, ATUIC_INSERT_MODE);
    const U32 cursor_row = d->cursor.row;
    const U32 cursor_col = d->cursor.col;

    for (U32 y = 0; y < rows; y++) {
        U32 row_base = y * cols;
        U32 y_px = y * char_h;
        CHAR_CELL *row_cells = &d->back_buffer[row_base];
        BOOL8 *row_dirty = &d->dirty[row_base];

        U32 x = 0;

        while (x < cols) {
            // skip clean cells
            if (!row_dirty[x]) {
                x++;
                continue;
            }

            // start run — batch consecutive dirty cells with same bg AND same attrs
            U32 run_start = x;
            CHAR_CELL *first = &row_cells[run_start];

            while (x < cols && row_dirty[x]) {
                CHAR_CELL *cur = &row_cells[x];
                if (x > run_start && (cur->bg != first->bg || cur->attrs != first->attrs))
                    break;
                x++;
            }

            U32 run_len = x - run_start;
            U32 run_px = run_start * char_w;
            CHAR_CELL *run_cell = first;
            U8 run_attrs = run_cell->attrs;

            // Resolve effective bg for the run (handle REVERSE at run level)
            VBE_COLOUR run_bg = run_cell->bg;
            if (run_attrs & ATUI_A_REVERSE)
                run_bg = run_cell->fg;

            // 1. Clear background in one go
            atui_fb_fill_rect(&fb, run_px, y_px, run_len * char_w, char_h, run_bg);

            // 1.5 Draw Cursor Shape explicitly
            if (cursor_visible && y == cursor_row) {
                if (cursor_col >= run_start && cursor_col < run_start + run_len) {
                    U32 cursor_x_px = cursor_col * char_w;
                    CHAR_CELL *cursor_cell = &row_cells[cursor_col];
                    
                    if (insert_mode) {
                        atui_fb_fill_rect(&fb, cursor_x_px, y_px,
                                          char_w, char_h, cursor_cell->fg);
                    } else {
                        atui_fb_fill_rect(&fb, cursor_x_px, y_px + char_h - 2,
                                          char_w, 2, cursor_cell->fg);
                    }
                }
            }

            // 2. Draw glyphs — batch foreground pixel runs per scanline
            for (U32 gy = 0; gy < char_h; gy++) {

                U32 py = y_px + gy;

                BOOL in_run = FALSE;
                U32 run_px_start = 0;
                VBE_COLOUR run_color = 0;

                for (U32 cx = 0; cx < run_len; cx++) {
                    CHAR_CELL *cell = &row_cells[run_start + cx];

                    BOOL is_cursor = cursor_visible && y == cursor_row &&
                                     (run_start + cx) == cursor_col;

                    VBE_COLOUR fg = cell->fg;
                    U8 attrs = cell->attrs;

                    if (attrs & ATUI_A_REVERSE) {
                        fg = cell->bg;
                    }
                    if (attrs & ATUI_A_DIM) {
                        fg = ((fg >> 1) & 0x7F7F7F);
                    }
                    if (is_cursor && insert_mode) {
                        fg = cell->bg;
                    }

                    U8 ch = cell->c;
                    U32 glyph_index = ch * char_h;

                    U8 bits = 0;
                    if (glyph_index + gy < font_sz) {
                        bits = font_data[glyph_index + gy];
                    }

                    if (attrs & ATUI_A_BOLD) {
                        bits |= (bits >> 1);
                    }

                    U32 cell_px_base = run_px + cx * char_w;
                    for (U32 gx = 0; gx < char_w; gx++) {
                        BOOL bit = bits & (1 << (7 - gx));
                        U32 px = cell_px_base + gx;

                        if (bit) {
                            if (!in_run) {
                                in_run = TRUE;
                                run_px_start = px;
                                run_color = fg;
                            } else if (run_color != fg) {
                                // Flush and start new color run
                                atui_fb_hspan(&fb, run_px_start, py,
                                              px - run_px_start, run_color);
                                run_px_start = px;
                                run_color = fg;
                            }
                        } else {
                            if (in_run) {
                                atui_fb_hspan(&fb, run_px_start, py,
                                              px - run_px_start, run_color);
                                in_run = FALSE;
                            }
                        }
                    }
                }

                if (in_run) {
                    U32 end_px = (run_start + run_len) * char_w;
                    atui_fb_hspan(&fb, run_px_start, py,
                                  end_px - run_px_start, run_color);
                }
            }

            // 3. Draw UNDERLINE attribute (1px line at bottom of cell)
            for (U32 cx = 0; cx < run_len; cx++) {
                CHAR_CELL *cell = &row_cells[run_start + cx];
                if (cell->attrs & ATUI_A_UNDERLINE) {
                    VBE_COLOUR ufg = cell->fg;
                    if (cell->attrs & ATUI_A_REVERSE) ufg = cell->bg;
                    if (cell->attrs & ATUI_A_DIM) ufg = ((ufg >> 1) & 0x7F7F7F);
                    atui_fb_hspan(&fb, run_px + cx * char_w,
                                  y_px + char_h - 1, char_w, ufg);
                }
            }

            // 4. mark clean
            MEMSET_OPT(row_dirty + run_start, FALSE, run_len);
        }
    }
}