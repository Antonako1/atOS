/*
 * TOUTPUT.c — ATUI-based terminal output for TSHELL
 *
 * Replaces VOUTPUT.c from the legacy shell.
 * Provides a 512-line scrollback ring buffer with per-character color
 * (via ANSI escape sequences), line editing, command history, and
 * cursor management — all rendered through an ATUI scrolling window.
 */

#include "TSHELL.h"
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/PROC_COM.h>
#include <STD/DEBUG.h>

/* ---- Module-global state ---- */
static TSHELL_INSTANCE *shndl ATTRIB_DATA = NULLPTR;
static TERM_OUTPUT     *tout  ATTRIB_DATA = NULLPTR;

/* ---- Visible rows/cols (== window interior) ---- */
static U32 vis_rows ATTRIB_DATA = 0;
static U32 vis_cols ATTRIB_DATA = 0;

/* ---- Output batching: when > 0, defer render_visible calls ---- */
static U32 batch_depth ATTRIB_DATA = 0;
static BOOL batch_dirty ATTRIB_DATA = FALSE;

U32 get_vis_rows() {
    return vis_rows;
}

/* ================================================================== */
/*  Initialization                                                     */
/* ================================================================== */

VOID INIT_TOUTPUT(TSHELL_INSTANCE *sh, ATUI_WINDOW *win) {
    ATUI_CLEAR();
    shndl = sh;
    tout  = sh->tout;

    tout->win = win;

    /* Default colors */
    tout->default_fg = VBE_WHITE;
    tout->default_bg = VBE_BLACK;
    tout->fg         = tout->default_fg;
    tout->bg         = tout->default_bg;

    /* Scrollback */
    MEMZERO(&tout->scrollback, sizeof(SCROLLBACK_BUFFER));
    tout->scrollback.write_row    = 0;
    tout->scrollback.total_lines  = 0;
    tout->scrollback.scroll_offset = 0;

    /* Cursor */
    tout->cursor_col = 0;
    tout->cursor_row = 0;
    tout->prompt_length = 0;
    tout->prompt_row    = 0;

    /* Line editing */
    MEMZERO(tout->current_line, CUR_LINE_MAX_LENGTH);
    tout->edit_pos      = 0;
    tout->history_count = 0;
    tout->history_index = 0;
    MEMZERO(tout->yank_buffer, CUR_LINE_MAX_LENGTH);

    /* ANSI parser */
    ANSI_RESET(tout);

    /* Cursor blink */
    tout->cursor_visible    = TRUE;
    tout->cursor_blink_tick = 0;

    /* Window dimensions */
    vis_cols = win->w;
    vis_rows = win->h;

    /* Set window colors and enable scrolling */
    ATUI_WSET_COLOR(win, tout->default_fg, tout->default_bg);
    win->scroll = TRUE;
    win->boxed  = FALSE;

    /* Fill scrollback with spaces */
    for (U32 r = 0; r < SCROLLBACK_LINES; r++) {
        for (U32 c = 0; c < AMOUNT_OF_COLS; c++) {
            tout->scrollback.cells[r][c].c  = ' ';
            tout->scrollback.cells[r][c].fg = tout->default_fg;
            tout->scrollback.cells[r][c].bg = tout->default_bg;
        }
    }
}

/* ================================================================== */
/*  Scrollback helpers                                                 */
/* ================================================================== */

/* Advance to the next scrollback row (allocating a new blank line) */
static VOID sb_newline(VOID) {
    tout->scrollback.write_row = (tout->scrollback.write_row + 1) % SCROLLBACK_LINES;
    tout->scrollback.total_lines++;

    /* Clear the new row */
    U32 wr = tout->scrollback.write_row;
    for (U32 c = 0; c < AMOUNT_OF_COLS; c++) {
        tout->scrollback.cells[wr][c].c  = ' ';
        tout->scrollback.cells[wr][c].fg = tout->fg;
        tout->scrollback.cells[wr][c].bg = tout->bg;
    }
    tout->cursor_col = 0;
}

/* Write one printable character into the scrollback at cursor position */
static VOID sb_putc(CHAR ch) {
    U32 wr = tout->scrollback.write_row;
    U32 col = tout->cursor_col;

    if (col >= vis_cols) {
        sb_newline();
        wr  = tout->scrollback.write_row;
        col = 0;
    }

    tout->scrollback.cells[wr][col].c  = ch;
    tout->scrollback.cells[wr][col].fg = tout->fg;
    tout->scrollback.cells[wr][col].bg = tout->bg;
    tout->cursor_col = col + 1;
}

/* ================================================================== */
/*  Rendering — copy scrollback into ATUI window                       */
/* ================================================================== */

static VOID render_visible(VOID) {
    ATUI_WINDOW *win = tout->win;

    /* How many content lines we actually have */
    U32 total = tout->scrollback.total_lines + 1; /* +1 for current write row */
    if (total > SCROLLBACK_LINES) total = SCROLLBACK_LINES;

    U32 offset = tout->scrollback.scroll_offset;

    /* Bottom row in scrollback = write_row */
    /* The visible region spans [bottom - offset - visible_rows + 1 .. bottom - offset] */
    /* Clamp so we don't go negative */
    I32 bottom_sb = (I32)tout->scrollback.write_row - (I32)offset;

    /* Move window cursor to (0,0) and redraw full screen */
    ATUI_WMOVE(win, 0, 0);

    /* Cursor overlay coordinates (prompt_row in scrollback, column) */
    U32 cur_sb_row = tout->prompt_row;
    U32 cur_col    = tout->prompt_length + tout->edit_pos;
    if (cur_col >= vis_cols) cur_col = vis_cols - 1;

    for (U32 vr = 0; vr < vis_rows; vr++) {
        I32 sb_idx = bottom_sb - (I32)(vis_rows - 1) + (I32)vr;

        /* Wrap around the ring buffer */
        while (sb_idx < 0) sb_idx += SCROLLBACK_LINES;
        sb_idx = sb_idx % SCROLLBACK_LINES;

        for (U32 vc = 0; vc < vis_cols; vc++) {
            SCROLLBACK_CELL *cell = &tout->scrollback.cells[sb_idx][vc];
            VBE_COLOUR fg = cell->fg;
            VBE_COLOUR bg = cell->bg;

            /* Selection / Cursor overlay */
            BOOL is_selected = FALSE;
            if ((U32)sb_idx == cur_sb_row) {
                if (tout->selection_active) {
                    U32 s_start = tout->selection_start;
                    U32 s_end   = tout->edit_pos;
                    if (s_start > s_end) { U32 t = s_start; s_start = s_end; s_end = t; }
                    
                    U32 col_idx = vc - tout->prompt_length;
                    if (vc >= tout->prompt_length && col_idx >= s_start && col_idx < s_end) {
                        is_selected = TRUE;
                    }
                }
                
                if (vc == cur_col && tout->cursor_visible) {
                    is_selected = !is_selected; /* Invert if cursor is on it */
                }
            }

            if (is_selected) {
                VBE_COLOUR tmp = fg;
                fg = bg;
                bg = tmp;
            }

            ATUI_WSET_COLOR(win, fg, bg);
            ATUI_WMOVE(win, vr, vc);
            ATUI_WADDCH(win, cell->c);
        }
    }

    ATUI_WREFRESH(win);
}

/* ================================================================== */
/*  Output batching — suppress renders during bulk output              */
/* ================================================================== */

VOID TPUT_BEGIN(VOID) {
    batch_depth++;
}

VOID TPUT_END(VOID) {
    if (batch_depth > 0) batch_depth--;
    if (batch_depth == 0 && batch_dirty) {
        batch_dirty = FALSE;
        render_visible();
    }
}

/* Wrapper: call render_visible only if not batching */
static VOID maybe_render(VOID) {
    if (batch_depth > 0) {
        batch_dirty = TRUE;
    } else {
        render_visible();
    }
}

VOID TPUT_REFRESH(VOID) {
    render_visible();
}

/* ================================================================== */
/*  Core output API                                                    */
/* ================================================================== */

VOID TPUT_CHAR(CHAR c) {
    /* Feed through ANSI parser first */
    if (ANSI_FEED(c, tout)) return;

    /* Handle control characters */
    if (c == '\n') {
        sb_newline();
        if (tout->scrollback.scroll_offset > 0)
            tout->scrollback.scroll_offset = 0; /* snap to bottom on output */
        maybe_render();
        return;
    }
    if (c == '\r') {
        tout->cursor_col = 0;
        return;
    }
    if (c == '\t') {
        U32 spaces = 4 - (tout->cursor_col % 4);
        for (U32 i = 0; i < spaces; i++) sb_putc(' ');
        return;
    }
    if (c == '\b') {
        if (tout->cursor_col > 0) tout->cursor_col--;
        return;
    }

    sb_putc(c);
}

VOID TPUT_STRING(PU8 str) {
    if (!str) return;

    /* Temporarily suppress per-character renders */
    batch_depth++;

    while (*str) {
        CHAR c = *str++;
        if (ANSI_FEED(c, tout)) continue;

        if (c == '\n') {
            sb_newline();
            if (tout->scrollback.scroll_offset > 0)
                tout->scrollback.scroll_offset = 0;
            continue;
        }
        if (c == '\r') { tout->cursor_col = 0; continue; }
        if (c == '\t') {
            U32 spaces = 4 - (tout->cursor_col % 4);
            for (U32 i = 0; i < spaces; i++) sb_putc(' ');
            continue;
        }
        if (c == '\b') {
            if (tout->cursor_col > 0) tout->cursor_col--;
            continue;
        }
        sb_putc(c);
    }

    batch_depth--;
    /* Single render after the entire string is processed */
    if (batch_depth == 0 && tout->scrollback.scroll_offset == 0)
        render_visible();
}

VOID TPUT_DEC(U32 n) {
    U8 buf[16];
    ITOA_U(n, buf, 10);
    TPUT_STRING(buf);
}

VOID TPUT_HEX(U32 n) {
    U8 buf[16];
    ITOA_U(n, buf, 16);
    TPUT_STRING(buf);
}

VOID TPUT_BIN(U32 n) {
    U8 buf[40];
    ITOA_U(n, buf, 2);
    TPUT_STRING(buf);
}

VOID TPUT_NEWLINE(VOID) {
    sb_newline();
    if (tout->scrollback.scroll_offset > 0)
        tout->scrollback.scroll_offset = 0;
    maybe_render();
}

/* ================================================================== */
/*  Color control                                                      */
/* ================================================================== */

VOID TPUT_SET_FG(VBE_COLOUR c) { tout->fg = c; }
VOID TPUT_SET_BG(VBE_COLOUR c) { tout->bg = c; }

/* ================================================================== */
/*  Scrollback navigation                                              */
/* ================================================================== */

VOID TPUT_SCROLL_UP(U32 n) {
    U32 max_off = 0;
    if (tout->scrollback.total_lines + 1 > vis_rows)
        max_off = tout->scrollback.total_lines + 1 - vis_rows;
    if (max_off > SCROLLBACK_LINES - vis_rows)
        max_off = SCROLLBACK_LINES - vis_rows;

    tout->scrollback.scroll_offset += n;
    if (tout->scrollback.scroll_offset > max_off)
        tout->scrollback.scroll_offset = max_off;
    render_visible();
}

VOID TPUT_SCROLL_DOWN(U32 n) {
    if (n >= tout->scrollback.scroll_offset)
        tout->scrollback.scroll_offset = 0;
    else
        tout->scrollback.scroll_offset -= n;
    render_visible();
}

VOID TPUT_SCROLL_TO_BOTTOM(VOID) {
    tout->scrollback.scroll_offset = 0;
    render_visible();
}

/* ================================================================== */
/*  Clear / Redraw                                                     */
/* ================================================================== */

VOID TPUT_CLS(VOID) {
    MEMZERO(&tout->scrollback, sizeof(SCROLLBACK_BUFFER));
    for (U32 r = 0; r < SCROLLBACK_LINES; r++) {
        for (U32 c = 0; c < AMOUNT_OF_COLS; c++) {
            tout->scrollback.cells[r][c].c  = ' ';
            tout->scrollback.cells[r][c].fg = tout->fg;
            tout->scrollback.cells[r][c].bg = tout->bg;
        }
    }
    tout->cursor_col = 0;
    tout->scrollback.write_row   = 0;
    tout->scrollback.total_lines = 0;
    tout->scrollback.scroll_offset = 0;

    MEMZERO(tout->current_line, CUR_LINE_MAX_LENGTH);
    tout->edit_pos      = 0;
    tout->history_index = 0;

    ATUI_WCLEAR(tout->win);
    render_visible();
}

VOID TPUT_REDRAW_CONSOLE(VOID) {
    render_visible();
}

/* ================================================================== */
/*  Prompt                                                              */
/* ================================================================== */

VOID PUT_SHELL_START(VOID) {
    TPUT_STRING(GET_PATH());
    TPUT_STRING((PU8)PATH_END_CHAR);

    tout->prompt_length = tout->cursor_col;
    tout->prompt_row    = tout->scrollback.write_row;
    tout->edit_pos      = 0;
    MEMZERO(tout->current_line, CUR_LINE_MAX_LENGTH);

    render_visible();
}

U8 *GET_CURRENT_LINE(VOID) {
    return tout->current_line;
}

/* ================================================================== */
/*  Command History                                                     */
/* ================================================================== */

static VOID push_history(PU8 line) {
    if (!line) return;
    U32 len = STRNLEN(line, CUR_LINE_MAX_LENGTH);
    if (len < 1) return;

    PU8 new_line = (PU8)MAlloc(len + 1);
    if (!new_line) return;
    MEMCPY(new_line, line, len);
    new_line[len] = '\0';

    if (tout->history_count < CMD_LINE_HISTORY) {
        tout->line_history[tout->history_count++] = new_line;
    } else {
        MFree(tout->line_history[0]);
        for (U32 i = 1; i < CMD_LINE_HISTORY; i++)
            tout->line_history[i - 1] = tout->line_history[i];
        tout->line_history[CMD_LINE_HISTORY - 1] = new_line;
    }
}

/* ================================================================== */
/*  Redraw current command line in scrollback                           */
/* ================================================================== */

VOID REDRAW_CURRENT_LINE(VOID) {
    /* The prompt row is where the prompt was written */
    U32 pr = tout->prompt_row;
    U32 pl = tout->prompt_length;

    /* Clear from prompt_length to end of row */
    for (U32 c = pl; c < vis_cols; c++) {
        tout->scrollback.cells[pr][c].c  = ' ';
        tout->scrollback.cells[pr][c].fg = tout->fg;
        tout->scrollback.cells[pr][c].bg = tout->bg;
    }

    /* Write current_line into scrollback starting at prompt_length */
    U32 len = STRLEN(tout->current_line);
    if (len > vis_cols - pl) len = vis_cols - pl;
    for (U32 i = 0; i < len; i++) {
        tout->scrollback.cells[pr][pl + i].c  = tout->current_line[i];
        tout->scrollback.cells[pr][pl + i].fg = tout->fg;
        tout->scrollback.cells[pr][pl + i].bg = tout->bg;
    }

    /* Update cursor position */
    tout->cursor_col = pl + tout->edit_pos;

    /* Reset cursor to visible on edit (restart blink timer) */
    tout->cursor_visible    = TRUE;
    tout->cursor_blink_tick = GET_PIT_TICKS();

    render_visible();
}

/* ================================================================== */
/*  Cursor blink (simplified — always visible in TSHELL)               */
/* ================================================================== */

VOID BLINK_CURSOR(VOID) {
    /* Toggle cursor visibility every 500 ms.
       The actual inversion is done as an overlay in render_visible(),
       so we never corrupt scrollback cells. */
    U32 now = GET_PIT_TICKS();
    if (now - tout->cursor_blink_tick < 500) return;
    tout->cursor_blink_tick = now;
    tout->cursor_visible = !tout->cursor_visible;
    render_visible();
}

/* ================================================================== */
/*  Line Editing Handlers                                               */
/* ================================================================== */

VOID HANDLE_LE_ENTER(VOID) {
    U32 len = STRNLEN(tout->current_line, CUR_LINE_MAX_LENGTH);
    tout->current_line[len] = '\0';
    tout->selection_active = FALSE;

    push_history(tout->current_line);

    BATSH_SET_MODE(FALSE);
    PARSE_BATSH_INPUT(tout->current_line, NULL);
    if (STRLEN(tout->current_line) == 0)
        TPUT_NEWLINE();

    MEMZERO(tout->current_line, CUR_LINE_MAX_LENGTH);
    tout->edit_pos = 0;
    tout->cursor_col = 0;

    PUT_SHELL_START();
    tout->history_index = 0;
}

VOID HANDLE_LE_BACKSPACE(VOID) {
    if (tout->edit_pos == 0) return;

    if (tout->selection_active) {
        U32 s_start = tout->selection_start;
        U32 s_end   = tout->edit_pos;
        if (s_start > s_end) { U32 t = s_start; s_start = s_end; s_end = t; }
        
        U32 len = STRLEN(tout->current_line);
        U32 count = s_end - s_start;
        for (U32 i = s_start; i <= len - count; i++)
            tout->current_line[i] = tout->current_line[i + count];
        
        tout->edit_pos = s_start;
        tout->selection_active = FALSE;
    } else {
        tout->edit_pos--;
        U32 len = STRLEN(tout->current_line);
        for (U32 i = tout->edit_pos; i < len; i++)
            tout->current_line[i] = tout->current_line[i + 1];
    }
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_CTRL_BACKSPACE(VOID) {
    if (tout->edit_pos == 0) return;
    U32 end = tout->edit_pos;
    while (tout->edit_pos > 0 && tout->current_line[tout->edit_pos - 1] == ' ')
        tout->edit_pos--;
    while (tout->edit_pos > 0 && tout->current_line[tout->edit_pos - 1] != ' ')
        tout->edit_pos--;
    
    U32 start = tout->edit_pos;
    U32 count = end - start;
    U32 len = STRLEN(tout->current_line);
    for (U32 i = start; i <= len - count; i++)
        tout->current_line[i] = tout->current_line[i + count];
    
    tout->selection_active = FALSE;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_DELETE(VOID) {
    U32 len = STRLEN(tout->current_line);
    if (tout->selection_active) {
        U32 s_start = tout->selection_start;
        U32 s_end   = tout->edit_pos;
        if (s_start > s_end) { U32 t = s_start; s_start = s_end; s_end = t; }
        
        U32 count = s_end - s_start;
        for (U32 i = s_start; i <= len - count; i++)
            tout->current_line[i] = tout->current_line[i + count];
        
        tout->edit_pos = s_start;
        tout->selection_active = FALSE;
    } else {
        if (tout->edit_pos >= len) return;
        for (U32 i = tout->edit_pos; i < len; i++)
            tout->current_line[i] = tout->current_line[i + 1];
    }
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_CTRL_DELETE(VOID) {
    U32 start = tout->edit_pos;
    U32 len = STRLEN(tout->current_line);
    if (start >= len) return;

    U32 end = start;
    while (end < len && tout->current_line[end] != ' ')
        end++;
    while (end < len && tout->current_line[end] == ' ')
        end++;
    
    U32 count = end - start;
    for (U32 i = start; i <= len - count; i++)
        tout->current_line[i] = tout->current_line[i + count];
    
    tout->selection_active = FALSE;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_ARROW_LEFT(VOID) {
    if (tout->selection_active) {
        U32 s_start = tout->selection_start;
        U32 s_end   = tout->edit_pos;
        tout->edit_pos = (s_start < s_end) ? s_start : s_end;
        tout->selection_active = FALSE;
    } else {
        if (tout->edit_pos == 0) return;
        tout->edit_pos--;
    }
    tout->cursor_col = tout->prompt_length + tout->edit_pos;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_SHIFT_ARROW_LEFT(VOID) {
    if (tout->edit_pos == 0) return;
    if (!tout->selection_active) {
        tout->selection_active = TRUE;
        tout->selection_start = tout->edit_pos;
    }
    tout->edit_pos--;
    tout->cursor_col = tout->prompt_length + tout->edit_pos;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_ARROW_RIGHT(VOID) {
    U32 len = STRLEN(tout->current_line);
    if (tout->selection_active) {
        U32 s_start = tout->selection_start;
        U32 s_end   = tout->edit_pos;
        tout->edit_pos = (s_start > s_end) ? s_start : s_end;
        tout->selection_active = FALSE;
    } else {
        if (tout->edit_pos >= len) return;
        tout->edit_pos++;
    }
    tout->cursor_col = tout->prompt_length + tout->edit_pos;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_SHIFT_ARROW_RIGHT(VOID) {
    U32 len = STRLEN(tout->current_line);
    if (tout->edit_pos >= len) return;
    if (!tout->selection_active) {
        tout->selection_active = TRUE;
        tout->selection_start = tout->edit_pos;
    }
    tout->edit_pos++;
    tout->cursor_col = tout->prompt_length + tout->edit_pos;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_ARROW_UP(VOID) {
    if (tout->history_count == 0) return;

    if (tout->history_index < tout->history_count) tout->history_index++;
    else tout->history_index = tout->history_count;

    const U8 *line = tout->line_history[tout->history_count - tout->history_index];
    STRNCPY(tout->current_line, line, CUR_LINE_MAX_LENGTH);
    tout->edit_pos = STRLEN(tout->current_line);

    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_ARROW_DOWN(VOID) {
    if (tout->history_count == 0 || tout->history_index == 0) return;

    tout->history_index--;

    if (tout->history_index == 0) {
        MEMZERO(tout->current_line, CUR_LINE_MAX_LENGTH);
        tout->edit_pos = 0;
    } else {
        const U8 *line = tout->line_history[tout->history_count - tout->history_index];
        STRNCPY(tout->current_line, line, CUR_LINE_MAX_LENGTH);
        tout->edit_pos = STRLEN(tout->current_line);
    }

    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_DEFAULT(KEYPRESS *kp, MODIFIERS *mod) {
    (VOID)mod;

    U8 c = keypress_to_char(kp->keycode);
    if (!c) return;

    U32 len = STRLEN(tout->current_line);
    if (tout->prompt_length + len >= vis_cols - 1) return;
    if (len >= CUR_LINE_MAX_LENGTH - 1) return;

    if (tout->selection_active) {
        U32 s_start = tout->selection_start;
        U32 s_end   = tout->edit_pos;
        if (s_start > s_end) { U32 t = s_start; s_start = s_end; s_end = t; }
        
        U32 count = s_end - s_start;
        for (U32 i = s_start; i <= len - count; i++)
            tout->current_line[i] = tout->current_line[i + count];
        
        tout->edit_pos = s_start;
        tout->selection_active = FALSE;
        len = STRLEN(tout->current_line); /* Re-calculate length after deletion */
    }

    /* Shift right for insertion */
    for (U32 i = len + 1; i > tout->edit_pos; i--)
        tout->current_line[i] = tout->current_line[i - 1];

    tout->current_line[tout->edit_pos] = c;
    tout->edit_pos++;

    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_HOME(VOID) {
    tout->edit_pos = 0;
    tout->cursor_col = tout->prompt_length;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_END(VOID) {
    tout->edit_pos = STRLEN(tout->current_line);
    tout->cursor_col = tout->prompt_length + tout->edit_pos;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_CTRL_LEFT(VOID) {
    if (tout->edit_pos == 0) return;
    tout->selection_active = FALSE;

    while (tout->edit_pos > 0 && tout->current_line[tout->edit_pos - 1] == ' ')
        tout->edit_pos--;
    while (tout->edit_pos > 0 && tout->current_line[tout->edit_pos - 1] != ' ')
        tout->edit_pos--;

    tout->cursor_col = tout->prompt_length + tout->edit_pos;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_CTRL_RIGHT(VOID) {
    U32 len = STRLEN(tout->current_line);
    if (tout->edit_pos >= len) return;
    tout->selection_active = FALSE;

    while (tout->edit_pos < len && tout->current_line[tout->edit_pos] != ' ')
        tout->edit_pos++;
    while (tout->edit_pos < len && tout->current_line[tout->edit_pos] == ' ')
        tout->edit_pos++;

    tout->cursor_col = tout->prompt_length + tout->edit_pos;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_CTRL_A(VOID) { HANDLE_LE_HOME(); }
VOID HANDLE_LE_CTRL_E(VOID) { HANDLE_LE_END(); }

VOID HANDLE_LE_CTRL_U(VOID) {
    if (tout->edit_pos == 0) return;

    STRNCPY(tout->yank_buffer, tout->current_line, tout->edit_pos);
    tout->yank_buffer[tout->edit_pos] = '\0';

    U32 len = STRLEN(tout->current_line);
    for (U32 i = 0; i <= len - tout->edit_pos; i++)
        tout->current_line[i] = tout->current_line[i + tout->edit_pos];

    tout->edit_pos = 0;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_CTRL_K(VOID) {
    U32 len = STRLEN(tout->current_line);
    if (tout->edit_pos >= len) return;

    STRCPY(tout->yank_buffer, &tout->current_line[tout->edit_pos]);
    tout->current_line[tout->edit_pos] = '\0';

    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_CTRL_W(VOID) {
    if (tout->edit_pos == 0) return;

    while (tout->edit_pos > 0 && tout->current_line[tout->edit_pos - 1] == ' ')
        tout->edit_pos--;

    U32 end = tout->edit_pos;
    while (tout->edit_pos > 0 && tout->current_line[tout->edit_pos - 1] != ' ')
        tout->edit_pos--;

    U32 start = tout->edit_pos;
    U32 len = STRLEN(tout->current_line);

    STRNCPY(tout->yank_buffer, &tout->current_line[start], end - start);
    tout->yank_buffer[end - start] = '\0';

    for (U32 i = start; i <= len - (end - start); i++)
        tout->current_line[i] = tout->current_line[i + (end - start)];

    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_CTRL_Y(VOID) {
    if (tout->yank_buffer[0] == '\0') return;
    U32 len = STRLEN(tout->current_line);
    U32 yank_len = STRLEN(tout->yank_buffer);

    if (len + yank_len >= CUR_LINE_MAX_LENGTH - 1) return;

    for (U32 i = len + yank_len; i > tout->edit_pos; i--)
        tout->current_line[i] = tout->current_line[i - yank_len];

    for (U32 i = 0; i < yank_len; i++)
        tout->current_line[tout->edit_pos + i] = tout->yank_buffer[i];

    tout->edit_pos += yank_len;
    REDRAW_CURRENT_LINE();
}

VOID HANDLE_LE_CTRL_L(VOID) {
    TPUT_CLS();
    PUT_SHELL_START();
}

VOID HANDLE_CTRL_C(VOID) {
    TSHELL_INSTANCE *sh = GET_SHNDL();
    U32 pid = sh->focused_pid;
    U32 self_pid = sh->self_pid;
    if (pid != self_pid) {
        END_PROC_SHELL(pid, U16_MAX / 2, TRUE);
    }

    TPUT_CHAR('^');
    TPUT_CHAR('C');
    TPUT_NEWLINE();

    PUT_SHELL_START();
}
