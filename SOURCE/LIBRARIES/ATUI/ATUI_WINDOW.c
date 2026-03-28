#define ATUI_INTERNALS
#include <LIBRARIES/ATUI/ATUI_TYPES.h>
#include <LIBRARIES/ATUI/ATUI.h>
#include <STD/MEM.h>
#include <STD/STRING.h>
#include <STD/BINARY.h>
#include <STD/PROC_COM.h>

// Defined in ATUI_WIDGETS.c — renders all widgets into the window buffer
extern VOID ATUI_RENDER_WIDGETS(ATUI_WINDOW *win);

/* ================================================================ */
/*  helpers                                                         */
/* ================================================================ */

/// Put a character into the window's back-buffer at (row, col)
static VOID wbuf_put(ATUI_WINDOW *win, U32 row, U32 col,
                     CHAR c, VBE_COLOUR fg, VBE_COLOUR bg) {
    if (row >= win->h || col >= win->w) return;
    U32 idx = row * win->w + col;
    win->buffer[idx].c  = c;
    win->buffer[idx].fg = fg;
    win->buffer[idx].bg = bg;
    win->dirty[idx]     = TRUE;
}

/// Get the interior origin (skip border if boxed)
static VOID wbuf_interior(ATUI_WINDOW *win, U32 *iy, U32 *ix, U32 *ih, U32 *iw) {
    if (win->boxed) {
        *iy = 1; *ix = 1;
        *ih = win->h > 2 ? win->h - 2 : 0;
        *iw = win->w > 2 ? win->w - 2 : 0;
    } else {
        *iy = 0; *ix = 0;
        *ih = win->h; *iw = win->w;
    }
}

/* ================================================================ */
/*  window lifecycle                                                */
/* ================================================================ */

ATUI_WINDOW* ATUI_NEWWIN(U32 h, U32 w, U32 y, U32 x) {
    if (h == 0 || w == 0) return NULLPTR;

    ATUI_WINDOW *win = MAlloc(sizeof(ATUI_WINDOW));
    if (!win) return NULLPTR;
    MEMZERO(win, sizeof(ATUI_WINDOW));

    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;

    win->cursor_x = 0;
    win->cursor_y = 0;

    ATUI_DISPLAY *d = GET_DISPLAY();
    win->fg = d ? d->fg : VBE_WHITE;
    win->bg = d ? d->bg : VBE_BLACK;

    win->border_style = ATUI_BORDER_SINGLE;
    win->needs_full_redraw = TRUE;

    U32 cells = w * h;

    win->buffer = MAlloc(cells * sizeof(CHAR_CELL));
    if (!win->buffer) { MFree(win); return NULLPTR; }

    win->front_buffer = MAlloc(cells * sizeof(CHAR_CELL));
    if (!win->front_buffer) { MFree(win->buffer); MFree(win); return NULLPTR; }

    win->dirty = MAlloc(cells * sizeof(BOOL8));
    if (!win->dirty) { MFree(win->front_buffer); MFree(win->buffer); MFree(win); return NULLPTR; }

    // initialise buffers to spaces
    for (U32 i = 0; i < cells; i++) {
        win->buffer[i].c  = ' ';
        win->buffer[i].fg = win->fg;
        win->buffer[i].bg = win->bg;
        win->front_buffer[i].c  = 0; // force first diff
        win->front_buffer[i].fg = 0;
        win->front_buffer[i].bg = 0;
        win->dirty[i] = TRUE;
    }

    win->widget_count   = 0;
    win->focused_widget = 0;

    return win;
}

VOID ATUI_DELWIN(ATUI_WINDOW *win) {
    if (!win) return;

    ATUI_DISPLAY *d = GET_DISPLAY();
    if (d) {
        // restore the region underneath the window in the main display
        for (U32 r = 0; r < win->h; r++) {
            U32 dr = win->y + r;
            if (dr >= d->rows) break;
            for (U32 c = 0; c < win->w; c++) {
                U32 dc = win->x + c;
                if (dc >= d->cols) break;
                U32 didx = dr * d->cols + dc;
                d->dirty[didx] = TRUE;
            }
        }
    }

    if (win->buffer)       MFree(win->buffer);
    if (win->front_buffer) MFree(win->front_buffer);
    if (win->dirty)        MFree(win->dirty);
    MFree(win);
}

/* ================================================================ */
/*  refresh — composite window into main display                    */
/* ================================================================ */

VOID ATUI_WREFRESH(ATUI_WINDOW *win) {
    if (!win) return;
    ATUI_DISPLAY *d = GET_DISPLAY();
    if (!d) return;

    // render widgets into the window buffer before compositing
    ATUI_RENDER_WIDGETS(win);

    for (U32 r = 0; r < win->h; r++) {
        U32 dr = win->y + r;
        if (dr >= d->rows) break;
        for (U32 c = 0; c < win->w; c++) {
            U32 dc = win->x + c;
            if (dc >= d->cols) break;

            U32 widx = r * win->w + c;
            U32 didx = dr * d->cols + dc;

            CHAR_CELL *src = &win->buffer[widx];
            CHAR_CELL *dst_front = &win->front_buffer[widx];

            // only copy changed cells
            if (win->dirty[widx] ||
                src->c  != dst_front->c ||
                src->fg != dst_front->fg ||
                src->bg != dst_front->bg)
            {
                d->back_buffer[didx] = *src;
                d->dirty[didx]       = TRUE;
                *dst_front           = *src;
                win->dirty[widx]     = FALSE;
            }
        }
    }

    ATUI_COPY_TO_TCB_FRAMEBUFFER();
}

/* ================================================================ */
/*  clear                                                           */
/* ================================================================ */

VOID ATUI_WCLEAR(ATUI_WINDOW *win) {
    if (!win) return;
    U32 cells = win->w * win->h;
    for (U32 i = 0; i < cells; i++) {
        win->buffer[i].c  = ' ';
        win->buffer[i].fg = win->fg;
        win->buffer[i].bg = win->bg;
        win->dirty[i]     = TRUE;
    }
    win->cursor_x = 0;
    win->cursor_y = 0;
    win->needs_full_redraw = TRUE;
}

/* ================================================================ */
/*  box / border drawing                                            */
/* ================================================================ */

VOID ATUI_WBOX(ATUI_WINDOW *win) {
    if (!win) return;
    ATUI_WBOX_STYLED(win, win->border_style);
}

VOID ATUI_WBOX_STYLED(ATUI_WINDOW *win, ATUI_BORDER_STYLE style) {
    if (!win || win->w < 2 || win->h < 2) return;

    win->boxed = TRUE;
    win->border_style = style;

    CHAR h_ch, v_ch, tl, tr, bl, br;

    if (style == ATUI_BORDER_DOUBLE) {
        h_ch = ATUI_BOX_DH;  v_ch = ATUI_BOX_DV;
        tl   = ATUI_BOX_DTL; tr   = ATUI_BOX_DTR;
        bl   = ATUI_BOX_DBL; br   = ATUI_BOX_DBR;
    } else {
        h_ch = ATUI_BOX_H;   v_ch = ATUI_BOX_V;
        tl   = ATUI_BOX_TL;  tr   = ATUI_BOX_TR;
        bl   = ATUI_BOX_BL;  br   = ATUI_BOX_BR;
    }

    VBE_COLOUR fg = win->fg;
    VBE_COLOUR bg = win->bg;

    // corners
    wbuf_put(win, 0,          0,          tl, fg, bg);
    wbuf_put(win, 0,          win->w - 1, tr, fg, bg);
    wbuf_put(win, win->h - 1, 0,          bl, fg, bg);
    wbuf_put(win, win->h - 1, win->w - 1, br, fg, bg);

    // top + bottom
    for (U32 c = 1; c < win->w - 1; c++) {
        wbuf_put(win, 0,          c, h_ch, fg, bg);
        wbuf_put(win, win->h - 1, c, h_ch, fg, bg);
    }

    // left + right
    for (U32 r = 1; r < win->h - 1; r++) {
        wbuf_put(win, r, 0,          v_ch, fg, bg);
        wbuf_put(win, r, win->w - 1, v_ch, fg, bg);
    }

    // draw title centered in top border
    if (win->title[0]) {
        U32 tlen = STRLEN(win->title);
        U32 inner = win->w - 2; // border chars
        if (tlen > inner - 2) tlen = inner - 2; // leave space
        U32 tx = 1 + (inner - tlen) / 2;
        for (U32 i = 0; i < tlen; i++) {
            wbuf_put(win, 0, tx + i, win->title[i], fg, bg);
        }
    }
}

VOID ATUI_WSET_TITLE(ATUI_WINDOW *win, const CHAR *title) {
    if (!win) return;
    if (title) {
        STRNCPY(win->title, title, 63);
        win->title[63] = 0;
    } else {
        win->title[0] = 0;
    }
}

/* ================================================================ */
/*  scroll                                                          */
/* ================================================================ */

VOID ATUI_WSCROLL(ATUI_WINDOW *win) {
    if (!win) return;

    U32 iy, ix, ih, iw;
    wbuf_interior(win, &iy, &ix, &ih, &iw);
    if (ih == 0 || iw == 0) return;

    // shift rows up by 1 inside interior
    for (U32 r = 0; r < ih - 1; r++) {
        for (U32 c = 0; c < iw; c++) {
            U32 dst = (iy + r)     * win->w + (ix + c);
            U32 src = (iy + r + 1) * win->w + (ix + c);
            win->buffer[dst] = win->buffer[src];
            win->dirty[dst]  = TRUE;
        }
    }

    // clear last interior row
    U32 last_row = iy + ih - 1;
    for (U32 c = 0; c < iw; c++) {
        U32 idx = last_row * win->w + (ix + c);
        win->buffer[idx].c  = ' ';
        win->buffer[idx].fg = win->fg;
        win->buffer[idx].bg = win->bg;
        win->dirty[idx]     = TRUE;
    }
}

/* ================================================================ */
/*  cursor                                                          */
/* ================================================================ */

VOID ATUI_WMOVE(ATUI_WINDOW *win, U32 y, U32 x) {
    if (!win) return;

    U32 iy, ix, ih, iw;
    wbuf_interior(win, &iy, &ix, &ih, &iw);

    if (y >= ih) y = ih ? ih - 1 : 0;
    if (x >= iw) x = iw ? iw - 1 : 0;

    win->cursor_y = y;
    win->cursor_x = x;
}

VOID ATUI_WGETYX(ATUI_WINDOW *win, U32 *y, U32 *x) {
    if (!win) return;
    if (y) *y = win->cursor_y;
    if (x) *x = win->cursor_x;
}

/* ================================================================ */
/*  character / string output                                       */
/* ================================================================ */

VOID ATUI_WADDCH(ATUI_WINDOW *win, CHAR c) {
    if (!win) return;

    U32 iy, ix, ih, iw;
    wbuf_interior(win, &iy, &ix, &ih, &iw);
    if (ih == 0 || iw == 0) return;

    U32 cy = win->cursor_y;
    U32 cx = win->cursor_x;

    if (c == '\n') {
        cx = 0;
        cy++;
        if (cy >= ih) {
            if (win->scroll) {
                ATUI_WSCROLL(win);
                cy = ih - 1;
            } else {
                cy = ih - 1;
            }
        }
        win->cursor_y = cy;
        win->cursor_x = cx;
        return;
    }

    if (c == '\r') {
        win->cursor_x = 0;
        return;
    }

    if (c == '\b') {
        if (cx > 0) {
            cx--;
        } else if (cy > 0) {
            cy--;
            cx = iw - 1;
        }
        wbuf_put(win, iy + cy, ix + cx, ' ', win->fg, win->bg);
        win->cursor_y = cy;
        win->cursor_x = cx;
        return;
    }

    wbuf_put(win, iy + cy, ix + cx, c, win->fg, win->bg);

    cx++;
    if (cx >= iw) {
        cx = 0;
        cy++;
        if (cy >= ih) {
            if (win->scroll) {
                ATUI_WSCROLL(win);
                cy = ih - 1;
            } else {
                cy = ih - 1;
            }
        }
    }
    win->cursor_y = cy;
    win->cursor_x = cx;
}

VOID ATUI_WADDSTR(ATUI_WINDOW *win, const CHAR *str) {
    if (!win || !str) return;
    while (*str) {
        ATUI_WADDCH(win, *str);
        str++;
    }
}

/// callback context for VFORMAT
typedef struct {
    ATUI_WINDOW *win;
} ATUI_WPRINT_CTX;

static VOID wprint_putch(CHAR c, VOID *ctx) {
    ATUI_WPRINT_CTX *p = (ATUI_WPRINT_CTX *)ctx;
    ATUI_WADDCH(p->win, c);
}

VOID ATUI_WPRINTW(ATUI_WINDOW *win, const CHAR *fmt, ...) {
    if (!win || !fmt) return;
    va_list args;
    va_start(args, fmt);
    ATUI_WPRINT_CTX ctx = { .win = win };
    VFORMAT(wprint_putch, &ctx, (CHAR *)fmt, args);
    va_end(args);
}

VOID ATUI_WMVADDSTR(ATUI_WINDOW *win, U32 y, U32 x, const CHAR *str) {
    if (!win || !str) return;
    ATUI_WMOVE(win, y, x);
    ATUI_WADDSTR(win, str);
}

VOID ATUI_WMVPRINTW(ATUI_WINDOW *win, U32 y, U32 x, const CHAR *fmt, ...) {
    if (!win || !fmt) return;
    ATUI_WMOVE(win, y, x);
    va_list args;
    va_start(args, fmt);
    ATUI_WPRINT_CTX ctx = { .win = win };
    VFORMAT(wprint_putch, &ctx, (CHAR *)fmt, args);
    va_end(args);
}

/* ================================================================ */
/*  colors                                                          */
/* ================================================================ */

VOID ATUI_WSET_COLOR(ATUI_WINDOW *win, VBE_COLOUR fg, VBE_COLOUR bg) {
    if (!win) return;
    win->fg = fg;
    win->bg = bg;
}

/* ================================================================ */
/*  line drawing helpers                                            */
/* ================================================================ */

VOID ATUI_WHLINE(ATUI_WINDOW *win, U32 y, U32 x, U32 len, CHAR ch) {
    if (!win) return;
    U32 iy, ix, ih, iw;
    wbuf_interior(win, &iy, &ix, &ih, &iw);
    for (U32 i = 0; i < len && (x + i) < iw; i++) {
        wbuf_put(win, iy + y, ix + x + i, ch, win->fg, win->bg);
    }
}

VOID ATUI_WVLINE(ATUI_WINDOW *win, U32 y, U32 x, U32 len, CHAR ch) {
    if (!win) return;
    U32 iy, ix, ih, iw;
    wbuf_interior(win, &iy, &ix, &ih, &iw);
    for (U32 i = 0; i < len && (y + i) < ih; i++) {
        wbuf_put(win, iy + y + i, ix + x, ch, win->fg, win->bg);
    }
}

/* ================================================================ */
/*  mouse hit-testing                                               */
/* ================================================================ */

/// Check if mouse cell coordinates overlap the window
static BOOL mouse_in_window(ATUI_WINDOW *win, U32 mcol, U32 mrow) {
    return (mcol >= win->x && mcol < win->x + win->w &&
            mrow >= win->y && mrow < win->y + win->h);
}

/// Check if mouse cell coordinates overlap a widget (in window-local coords)
static BOOL mouse_in_widget(ATUI_WINDOW *win, ATUI_WIDGET *w, U32 mcol, U32 mrow) {
    // Convert screen coords to window-interior coords
    U32 iy = win->boxed ? 1 : 0;
    U32 ix = win->boxed ? 1 : 0;

    U32 wabs_y = win->y + iy + w->y;
    U32 wabs_x = win->x + ix + w->x;

    return (mcol >= wabs_x && mcol < wabs_x + w->w &&
            mrow >= wabs_y && mrow < wabs_y + w->h);
}

ATUI_WIDGET* ATUI_WINDOW_HANDLE_MOUSE(ATUI_WINDOW *win, ATUI_MOUSE *ms) {
    if (!win || !ms) return NULLPTR;

    // Only process on click
    if (!ms->clicked) return NULLPTR;

    // Check if click is inside the window
    if (!mouse_in_window(win, ms->cell_col, ms->cell_row))
        return NULLPTR;

    // Test each widget for hit
    for (U32 i = 0; i < win->widget_count; i++) {
        ATUI_WIDGET *w = &win->widgets[i];
        if (!w->visible || !w->enabled) continue;

        if (mouse_in_widget(win, w, ms->cell_col, ms->cell_row)) {
            // Focus this widget
            if (win->focused_widget < win->widget_count)
                win->widgets[win->focused_widget].focused = FALSE;
            win->focused_widget = i;
            w->focused = TRUE;

            // Activate depending on type
            switch (w->type) {
                case ATUI_WIDGET_BUTTON:
                    if (w->on_press) w->on_press(w);
                    break;

                case ATUI_WIDGET_CHECKBOX:
                    w->data.checkbox.checked = !w->data.checkbox.checked;
                    if (w->on_change) w->on_change(w);
                    break;

                case ATUI_WIDGET_RADIO: {
                    // Deselect all in group, select this
                    U32 gid = w->data.radio.group_id;
                    for (U32 j = 0; j < win->widget_count; j++) {
                        ATUI_WIDGET *other = &win->widgets[j];
                        if (other->type == ATUI_WIDGET_RADIO &&
                            other->data.radio.group_id == gid)
                            other->data.radio.selected = FALSE;
                    }
                    w->data.radio.selected = TRUE;
                    if (w->on_change) w->on_change(w);
                    break;
                }

                case ATUI_WIDGET_DROPDOWN: {
                    ATUI_DROPDOWN *dd = &w->data.dropdown;
                    if (!dd->open) {
                        dd->open = TRUE;
                    } else {
                        // Calculate which item was clicked
                        U32 iy_off = win->boxed ? 1 : 0;
                        U32 item_row = ms->cell_row - (win->y + iy_off + w->y + 1);
                        U32 item_idx = dd->scroll_offset + item_row;
                        if (item_idx < dd->item_count) {
                            dd->selected = item_idx;
                            dd->open = FALSE;
                            if (w->on_change) w->on_change(w);
                        } else {
                            dd->open = FALSE;
                        }
                    }
                    break;
                }

                case ATUI_WIDGET_TEXTBOX: {
                    // Place cursor at click position within the textbox
                    U32 ix_off = win->boxed ? 1 : 0;
                    U32 click_x = ms->cell_col - (win->x + ix_off + w->x + 1); // +1 for bracket
                    ATUI_TEXTBOX *tb = &w->data.textbox;
                    U32 new_cursor = tb->scroll_offset + click_x;
                    if (new_cursor > tb->len) new_cursor = tb->len;
                    tb->cursor = new_cursor;
                    break;
                }

                default:
                    break;
            }

            return w;
        }
    }

    return NULLPTR;
}

/* ================================================================ */
/*  animated roll-in                                               */
/* ================================================================ */

/// Typewriter roll-in: writes each character of str one at a time into the
/// window at interior position (y, x), waiting ticks_per_char PIT ticks
/// between each character, then calling ATUI_WREFRESH so the user sees
/// the text building up in real time.
///
/// ticks_per_char: PIT runs at ~1000 Hz, so 20 ≈ 20 ms per character.
///                 Pass 0 to print instantly (same as ATUI_WMVADDSTR).
VOID ATUI_WROLLSTR(ATUI_WINDOW *win, U32 y, U32 x, const CHAR *str, U32 ticks_per_char) {
    if (!win || !str) return;

    U32 iy, ix, ih, iw;
    wbuf_interior(win, &iy, &ix, &ih, &iw);

    U32 col = x;
    for (U32 i = 0; str[i] != '\0' && col < iw; i++, col++) {
        wbuf_put(win, iy + y, ix + col, str[i], win->fg, win->bg);
        ATUI_WREFRESH(win);

        if (ticks_per_char > 0) {
            U32 deadline = GET_PIT_TICKS() + ticks_per_char;
            while (GET_PIT_TICKS() < deadline);
        }
    }
}

/// Move to (y, x) inside win, then roll-in str.
VOID ATUI_WMVROLLSTR(ATUI_WINDOW *win, U32 y, U32 x, const CHAR *str, U32 ticks_per_char) {
    ATUI_WMOVE(win, y, x);
    ATUI_WROLLSTR(win, y, x, str, ticks_per_char);
}

/// Roll every line of a NULL-terminated string array into the window,
/// one line per row starting at (start_y, x).
VOID ATUI_WROLLLINES(ATUI_WINDOW *win, U32 start_y, U32 x,
                     const CHAR **lines, U32 line_count,
                     U32 ticks_per_char, U32 ticks_between_lines) {
    if (!win || !lines) return;
    for (U32 i = 0; i < line_count; i++) {
        if (!lines[i]) continue;
        ATUI_WROLLSTR(win, start_y + i, x, lines[i], ticks_per_char);
        // Optional pause between lines
        if (ticks_between_lines > 0) {
            U32 deadline = GET_PIT_TICKS() + ticks_between_lines;
            while (GET_PIT_TICKS() < deadline);
        }
    }
}
