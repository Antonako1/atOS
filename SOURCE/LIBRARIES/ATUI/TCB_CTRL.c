
#define ATUI_INTERNALS
#include <LIBRARIES/ATUI/ATUI_TYPES.h>
#include <LIBRARIES/ATUI/ATUI.h>
#include <STD/BINARY.h>
#include <STD/GRAPHICS.h>

VOID ATUI_COPY_TO_TCB_FRAMEBUFFER() {
    ATUI_DISPLAY *d = GET_DISPLAY();
    if (!d || !d->t) return;

    const U32 cols = d->cols;
    const U32 rows = d->rows;

    const U32 char_w = d->font.char_width;   
    const U32 char_h = d->font.char_height;  

    for (U32 y = 0; y < rows; y++) {

        U32 x = 0;

        while (x < cols) {
            U32 idx = y * cols + x;

            // skip clean cells
            if (!d->dirty[idx]) {
                x++;
                continue;
            }

            // start run — batch consecutive dirty cells with same bg AND same attrs
            U32 run_start = x;

            while (x < cols && d->dirty[y * cols + x]) {
                if (x > run_start) {
                    CHAR_CELL *first = &d->back_buffer[y * cols + run_start];
                    CHAR_CELL *cur   = &d->back_buffer[y * cols + x];
                    if (cur->bg != first->bg || cur->attrs != first->attrs)
                        break;
                }
                x++;
            }

            U32 run_len = x - run_start;
            CHAR_CELL *run_cell = &d->back_buffer[y * cols + run_start];
            U8 run_attrs = run_cell->attrs;

            // Resolve effective bg for the run (handle REVERSE at run level)
            VBE_COLOUR run_bg = run_cell->bg;
            if (run_attrs & ATUI_A_REVERSE)
                run_bg = run_cell->fg;

            // 1. Clear background in one go
            DRAW_FILLED_RECTANGLE(
                run_start * char_w,
                y * char_h,
                (run_start + run_len) * char_w,
                (y + 1) * char_h,
                run_bg
            );

            // 1.5 Draw Cursor Shape explicitly
            if (IS_FLAG_SET(d->cnf, ATUIC_CURSOR_VISIBLE) && y == d->cursor.row) {
                if (d->cursor.col >= run_start && d->cursor.col < run_start + run_len) {
                    U32 cell_idx = y * cols + d->cursor.col;
                    
                    if (IS_FLAG_SET(d->cnf, ATUIC_INSERT_MODE)) {
                        // Block Cursor: Fill the whole cell with Foreground color
                        DRAW_FILLED_RECTANGLE(
                            d->cursor.col * char_w,
                            y * char_h,
                            (d->cursor.col + 1) * char_w,
                            (y + 1) * char_h,
                            d->back_buffer[cell_idx].fg
                        );
                    } else {
                        // Underline Cursor: Draw a 2-pixel line at the bottom
                        DRAW_FILLED_RECTANGLE(
                            d->cursor.col * char_w,
                            y * char_h + char_h - 2, 
                            (d->cursor.col + 1) * char_w,
                            y * char_h + char_h,
                            d->back_buffer[cell_idx].fg
                        );
                    }
                }
            }

            // 2. Draw glyphs (batched per scanline)
            for (U32 gy = 0; gy < char_h; gy++) {

                BOOL in_run = FALSE;
                U32 run_px_start = 0;
                VBE_COLOUR run_color = 0;

                for (U32 cx = 0; cx < run_len; cx++) {
                    U32 cell_idx = y * cols + (run_start + cx);
                    CHAR_CELL *cell = &d->back_buffer[cell_idx];

                    BOOL is_cursor =
                        IS_FLAG_SET(d->cnf, ATUIC_CURSOR_VISIBLE) &&
                        y == d->cursor.row &&
                        (run_start + cx) == d->cursor.col;

                    // Resolve fg/bg with attribute handling
                    VBE_COLOUR fg = cell->fg;
                    VBE_COLOUR bg = cell->bg;
                    U8 attrs = cell->attrs;

                    // REVERSE: swap fg and bg
                    if (attrs & ATUI_A_REVERSE) {
                        VBE_COLOUR tmp = fg;
                        fg = bg;
                        bg = tmp;
                    }

                    // DIM: halve the fg RGB components
                    if (attrs & ATUI_A_DIM) {
                        fg = ((fg >> 1) & 0x7F7F7F);
                    }

                    // If it's a block cursor, invert the text to draw into the block
                    if (is_cursor && IS_FLAG_SET(d->cnf, ATUIC_INSERT_MODE)) {
                        fg = bg;
                    }

                    U8 ch = cell->c;
                    U32 glyph_index = ch * char_h;

                    if (glyph_index + gy >= d->font.file->sz)
                        continue;

                    U8 *glyph = d->font.file->data + glyph_index;
                    U8 bits = glyph[gy];

                    // BOLD: widen each glyph pixel rightward (classic CRT bold)
                    if (attrs & ATUI_A_BOLD) {
                        bits |= (bits >> 1);
                    }

                    for (U32 gx = 0; gx < char_w; gx++) {
                        BOOL bit = bits & (1 << (7 - gx));

                        U32 px = (run_start + cx) * char_w + gx;
                        U32 py = y * char_h + gy;

                        if (bit) {
                            if (!in_run) {
                                in_run = TRUE;
                                run_px_start = px;
                                run_color = fg;
                            } else if (run_color != fg) {
                                DRAW_LINE(run_px_start, py, px - 1, py, run_color);
                                run_px_start = px;
                                run_color = fg;
                            }
                        } else {
                            if (in_run) {
                                DRAW_LINE(run_px_start, py, px - 1, py, run_color);
                                in_run = FALSE;
                            }
                        }
                    }
                }

                if (in_run) {
                    U32 end_px = (run_start + run_len) * char_w - 1;
                    DRAW_LINE(run_px_start, y * char_h + gy, end_px, y * char_h + gy, run_color);
                }
            }

            // 3. Draw UNDERLINE attribute (1px line at bottom of cell)
            for (U32 cx = 0; cx < run_len; cx++) {
                U32 cell_idx = y * cols + (run_start + cx);
                CHAR_CELL *cell = &d->back_buffer[cell_idx];
                if (cell->attrs & ATUI_A_UNDERLINE) {
                    VBE_COLOUR ufg = cell->fg;
                    if (cell->attrs & ATUI_A_REVERSE) ufg = cell->bg;
                    if (cell->attrs & ATUI_A_DIM) ufg = ((ufg >> 1) & 0x7F7F7F);
                    DRAW_FILLED_RECTANGLE(
                        (run_start + cx) * char_w,
                        y * char_h + char_h - 1,
                        (run_start + cx + 1) * char_w,
                        y * char_h + char_h,
                        ufg
                    );
                }
            }

            // 4. mark clean
            for (U32 cx = 0; cx < run_len; cx++) {
                d->dirty[y * cols + (run_start + cx)] = FALSE;
            }
        }
    }
}