/*
 * ATUI_WIDGETS.c  —  Widget rendering, input handling, focus & form system
 *
 * Provides: buttons, labels, textboxes, checkboxes, radio buttons,
 *           dropdowns, progress bars, separators, and a form abstraction.
 *
 * The entry point ATUI_RENDER_WIDGETS() is called from ATUI_WREFRESH()
 * via extern linkage in ATUI_WINDOW.c.
 */

#include <LIBRARIES/ATUI/ATUI_TYPES.h>
#include <LIBRARIES/ATUI/ATUI.h>
#include <STD/MEM.h>
#include <STD/STRING.h>
#include <STD/PROC_COM.h>
#include <DRIVERS/PS2/KEYBOARD_MOUSE.h>

#define PIT_TICKS_PER_SCROLL_STEP 10 // ~50 ms per marquee scroll step

/* ================================================================ */
/*  internal helpers                                                */
/* ================================================================ */

/// Write a character into the window back-buffer (duplicates the
/// static helper in ATUI_WINDOW.c because we need it here too).
static VOID wput(ATUI_WINDOW *win, U32 row, U32 col,
                 CHAR c, VBE_COLOUR fg, VBE_COLOUR bg) {
    if (row >= win->h || col >= win->w) return;
    U32 idx = row * win->w + col;
    win->buffer[idx].c  = c;
    win->buffer[idx].fg = fg;
    win->buffer[idx].bg = bg;
    win->dirty[idx]     = TRUE;
}

/// Compute interior origin and size (skip border if boxed).
static VOID winterior(ATUI_WINDOW *win, U32 *iy, U32 *ix, U32 *ih, U32 *iw) {
    if (win->boxed) {
        *iy = 1; *ix = 1;
        *ih = win->h > 2 ? win->h - 2 : 0;
        *iw = win->w > 2 ? win->w - 2 : 0;
    } else {
        *iy = 0; *ix = 0;
        *ih = win->h; *iw = win->w;
    }
}

/// Can this widget type receive focus?
static BOOL widget_is_focusable(ATUI_WIDGET_TYPE t) {
    return (t == ATUI_WIDGET_BUTTON   ||
            t == ATUI_WIDGET_TEXTBOX  ||
            t == ATUI_WIDGET_CHECKBOX ||
            t == ATUI_WIDGET_RADIO    ||
            t == ATUI_WIDGET_DROPDOWN);
}

/// Allocate the next free widget slot in a window. Returns NULLPTR if full.
static ATUI_WIDGET* widget_alloc(ATUI_WINDOW *win) {
    if (!win || win->widget_count >= ATUI_MAX_WIDGETS) return NULLPTR;
    ATUI_WIDGET *w = &win->widgets[win->widget_count++];
    MEMZERO(w, sizeof(ATUI_WIDGET));
    w->visible   = TRUE;
    w->enabled   = TRUE;
    w->fg        = win->fg;
    w->bg        = win->bg;
    w->focus_fg  = VBE_BLACK;
    w->focus_bg  = VBE_WHITE;
    return w;
}

/* ================================================================ */
/*  individual widget renderers                                     */
/* ================================================================ */

static VOID render_button(ATUI_WINDOW *win, ATUI_WIDGET *w, U32 iy, U32 ix) {
    VBE_COLOUR fg = w->focused ? w->focus_fg : w->fg;
    VBE_COLOUR bg = w->focused ? w->focus_bg : w->bg;

    U32 row = iy + w->y;
    U32 col = ix + w->x;

    wput(win, row, col, '[', fg, bg);
    col++;
    for (U32 i = 0; w->text[i] && i < 62; i++) {
        wput(win, row, col++, w->text[i], fg, bg);
    }
    wput(win, row, col, ']', fg, bg);
}

static VOID render_label(ATUI_WINDOW *win, ATUI_WIDGET *w, U32 iy, U32 ix, U32 iw) {
    U32 row  = iy + w->y;
    U32 col  = ix + w->x;
    U32 tlen = STRLEN(w->text);
    // available columns from this widget's x to the interior right edge
    U32 avail = (w->x < iw) ? iw - w->x : 0;

    if (tlen == 0 || avail == 0) return;

    if (tlen <= avail) {
        // Text fits — render without scrolling
        for (U32 i = 0; i < tlen; i++)
            wput(win, row, col + i, w->text[i], w->fg, w->bg);
    } else {
        // Text overflows — marquee scroll driven by PIT ticks
        ATUI_LABEL *lbl = &w->data.label;
        U32 step = lbl->ticks_per_step > 0 ? lbl->ticks_per_step : PIT_TICKS_PER_SCROLL_STEP;
        U32 now  = GET_PIT_TICKS();
        if (now - lbl->last_tick >= step) {
            lbl->scroll_offset = (lbl->scroll_offset + 1) % tlen;
            lbl->last_tick = now;
        }
        // Render avail chars wrapping around the text cyclically,
        // with a 2-space gap between repetitions
        for (U32 i = 0; i < avail; i++) {
            U32 pos = (lbl->scroll_offset + i) % (tlen + 2);
            CHAR c = (pos < tlen) ? w->text[pos] : ' ';
            wput(win, row, col + i, c, w->fg, w->bg);
        }
    }
}

static VOID render_textbox(ATUI_WINDOW *win, ATUI_WIDGET *w, U32 iy, U32 ix) {
    ATUI_TEXTBOX *tb = &w->data.textbox;
    VBE_COLOUR fg = w->focused ? w->focus_fg : w->fg;
    VBE_COLOUR bg = w->focused ? w->focus_bg : w->bg;

    U32 row = iy + w->y;
    U32 col = ix + w->x;
    U32 vis = tb->visible_width;

    wput(win, row, col, '[', fg, bg);
    col++;

    BOOL show_placeholder = (tb->len == 0 && !w->focused && tb->placeholder[0]);

    for (U32 i = 0; i < vis; i++) {
        CHAR c = ' ';
        if (show_placeholder) {
            c = (i < STRLEN(tb->placeholder)) ? tb->placeholder[i] : ' ';
        } else {
            U32 buf_pos = tb->scroll_offset + i;
            if (buf_pos < tb->len) {
                c = tb->password_mode ? '*' : tb->buffer[buf_pos];
            }
        }
        // Cursor highlight
        VBE_COLOUR cell_fg = fg;
        VBE_COLOUR cell_bg = bg;
        if (w->focused && (tb->scroll_offset + i) == tb->cursor) {
            cell_fg = bg;
            cell_bg = fg;
        }
        wput(win, row, col + i, c, cell_fg, cell_bg);
    }
    wput(win, row, col + vis, ']', fg, bg);
}

static VOID render_checkbox(ATUI_WINDOW *win, ATUI_WIDGET *w, U32 iy, U32 ix) {
    VBE_COLOUR fg = w->focused ? w->focus_fg : w->fg;
    VBE_COLOUR bg = w->focused ? w->focus_bg : w->bg;

    U32 row = iy + w->y;
    U32 col = ix + w->x;

    wput(win, row, col,   '[', fg, bg);
    wput(win, row, col+1, w->data.checkbox.checked ? (CHAR)ATUI_CHECK_MARK : ' ', fg, bg);
    wput(win, row, col+2, ']', fg, bg);
    col += 4;
    for (U32 i = 0; w->text[i] && i < 60; i++) {
        wput(win, row, col++, w->text[i], fg, bg);
    }
}

static VOID render_radio(ATUI_WINDOW *win, ATUI_WIDGET *w, U32 iy, U32 ix) {
    VBE_COLOUR fg = w->focused ? w->focus_fg : w->fg;
    VBE_COLOUR bg = w->focused ? w->focus_bg : w->bg;

    U32 row = iy + w->y;
    U32 col = ix + w->x;

    wput(win, row, col,   '(', fg, bg);
    wput(win, row, col+1, w->data.radio.selected ? (CHAR)ATUI_BULLET : ' ', fg, bg);
    wput(win, row, col+2, ')', fg, bg);
    col += 4;
    for (U32 i = 0; w->text[i] && i < 60; i++) {
        wput(win, row, col++, w->text[i], fg, bg);
    }
}

static VOID render_dropdown(ATUI_WINDOW *win, ATUI_WIDGET *w, U32 iy, U32 ix) {
    ATUI_DROPDOWN *dd = &w->data.dropdown;
    VBE_COLOUR fg = w->focused ? w->focus_fg : w->fg;
    VBE_COLOUR bg = w->focused ? w->focus_bg : w->bg;

    U32 row = iy + w->y;
    U32 col = ix + w->x;
    U32 vis = w->w > 3 ? w->w - 3 : 1;

    // Header: [selectedText v]
    wput(win, row, col, '[', fg, bg);
    col++;
    CHAR *sel_text = (dd->selected < dd->item_count) ? dd->items[dd->selected] : (CHAR*)"";
    for (U32 i = 0; i < vis; i++) {
        CHAR c = (i < STRLEN(sel_text)) ? sel_text[i] : ' ';
        wput(win, row, col + i, c, fg, bg);
    }
    wput(win, row, col + vis, (CHAR)ATUI_ARROW_DOWN, fg, bg);
    wput(win, row, col + vis + 1, ']', fg, bg);

    // Open list
    if (dd->open) {
        U32 max_vis = dd->max_visible;
        if (max_vis == 0) max_vis = dd->item_count;
        if (max_vis > dd->item_count - dd->scroll_offset)
            max_vis = dd->item_count - dd->scroll_offset;

        for (U32 i = 0; i < max_vis; i++) {
            U32 item_idx = dd->scroll_offset + i;
            U32 list_row = row + 1 + i;
            U32 list_col = ix + w->x;

            VBE_COLOUR ifg = (item_idx == dd->selected) ? bg : fg;
            VBE_COLOUR ibg = (item_idx == dd->selected) ? fg : bg;

            wput(win, list_row, list_col, ' ', ifg, ibg);
            for (U32 j = 0; j < vis + 2; j++) {
                CHAR c = (j < STRLEN(dd->items[item_idx])) ? dd->items[item_idx][j] : ' ';
                wput(win, list_row, list_col + 1 + j, c, ifg, ibg);
            }
            wput(win, list_row, list_col + vis + 3, ' ', ifg, ibg);
        }
    }
}

static VOID render_progressbar(ATUI_WINDOW *win, ATUI_WIDGET *w, U32 iy, U32 ix) {
    ATUI_PROGRESSBAR *pb = &w->data.progress;
    U32 row = iy + w->y;
    U32 col = ix + w->x;
    U32 bar_w = w->w;

    U32 max_v = pb->max > 0 ? pb->max : 100;
    U32 filled = (pb->value * bar_w) / max_v;
    if (filled > bar_w) filled = bar_w;

    for (U32 i = 0; i < bar_w; i++) {
        CHAR c = (i < filled) ? (CHAR)ATUI_BLOCK_FULL : (CHAR)ATUI_SHADE_LIGHT;
        wput(win, row, col + i, c, w->fg, w->bg);
    }

    if (pb->show_percent) {
        CHAR pct[8];
        U32 percent = (pb->value * 100) / max_v;
        SPRINTF(pct, "%d%%", percent);
        U32 plen = STRLEN(pct);
        U32 start = (bar_w > plen) ? (bar_w - plen) / 2 : 0;
        for (U32 i = 0; i < plen && (start + i) < bar_w; i++) {
            wput(win, row, col + start + i, pct[i], w->bg, w->fg);
        }
    }
}

static VOID render_separator(ATUI_WINDOW *win, ATUI_WIDGET *w, U32 iy, U32 ix) {
    U32 row = iy + w->y;
    U32 col = ix + w->x;
    for (U32 i = 0; i < w->w; i++) {
        wput(win, row, col + i, (CHAR)ATUI_BOX_H, w->fg, w->bg);
    }
}

/* ================================================================ */
/*  render all widgets into a window                                */
/* ================================================================ */

static VOID render_all_widgets(ATUI_WINDOW *win) {
    U32 iy, ix, ih, iw;
    winterior(win, &iy, &ix, &ih, &iw);

    for (U32 i = 0; i < win->widget_count; i++) {
        ATUI_WIDGET *w = &win->widgets[i];
        if (!w->visible) continue;

        switch (w->type) {
            case ATUI_WIDGET_BUTTON:      render_button(win, w, iy, ix);         break;
            case ATUI_WIDGET_LABEL:       render_label(win, w, iy, ix, iw);      break;
            case ATUI_WIDGET_TEXTBOX:     render_textbox(win, w, iy, ix);     break;
            case ATUI_WIDGET_CHECKBOX:    render_checkbox(win, w, iy, ix);    break;
            case ATUI_WIDGET_RADIO:       render_radio(win, w, iy, ix);       break;
            case ATUI_WIDGET_DROPDOWN:    render_dropdown(win, w, iy, ix);    break;
            case ATUI_WIDGET_PROGRESSBAR: render_progressbar(win, w, iy, ix); break;
            case ATUI_WIDGET_SEPARATOR:   render_separator(win, w, iy, ix);   break;
            default: break;
        }
    }
}

/* ================================================================ */
/*  public entry point (called from ATUI_WINDOW.c via extern)       */
/* ================================================================ */

VOID ATUI_RENDER_WIDGETS(ATUI_WINDOW *win) {
    if (!win || win->widget_count == 0) return;
    render_all_widgets(win);
}

/* ================================================================ */
/*  widget constructors                                             */
/* ================================================================ */

ATUI_WIDGET* ATUI_WADD_BUTTON(
    ATUI_WINDOW *win, U32 y, U32 x,
    const CHAR *text, VOID (*on_press)(ATUI_WIDGET*))
{
    ATUI_WIDGET *w = widget_alloc(win);
    if (!w) return NULLPTR;
    w->type = ATUI_WIDGET_BUTTON;
    w->x = x; w->y = y;
    STRNCPY(w->text, text, ATUI_WIDGET_MAX_TEXT - 1);
    w->w = STRLEN(w->text) + 2; // [Text]
    w->h = 1;
    w->on_press = on_press;
    // first focusable widget gets focus
    if (win->widget_count == 1 || !widget_is_focusable(win->widgets[win->focused_widget].type)) {
        win->focused_widget = win->widget_count - 1;
        w->focused = TRUE;
    }
    return w;
}

ATUI_WIDGET* ATUI_WADD_LABEL(
    ATUI_WINDOW *win, U32 y, U32 x, const CHAR *text)
{
    ATUI_WIDGET *w = widget_alloc(win);
    if (!w) return NULLPTR;
    w->type = ATUI_WIDGET_LABEL;
    w->x = x; w->y = y;
    STRNCPY(w->text, text, ATUI_WIDGET_MAX_TEXT - 1);
    w->w = STRLEN(w->text);
    w->h = 1;
    w->data.label.scroll_offset  = 0;
    w->data.label.last_tick      = GET_PIT_TICKS();
    w->data.label.ticks_per_step = PIT_TICKS_PER_SCROLL_STEP; // ~50 ms per step
    return w;
}

ATUI_WIDGET* ATUI_WADD_TEXTBOX(
    ATUI_WINDOW *win, U32 y, U32 x, U32 width,
    const CHAR *placeholder, VOID (*on_change)(ATUI_WIDGET*))
{
    ATUI_WIDGET *w = widget_alloc(win);
    if (!w) return NULLPTR;
    w->type = ATUI_WIDGET_TEXTBOX;
    w->x = x; w->y = y;
    w->w = width + 2; // [.......]
    w->h = 1;
    w->data.textbox.visible_width = width;
    w->data.textbox.password_mode = FALSE;
    if (placeholder) STRNCPY(w->data.textbox.placeholder, placeholder, ATUI_WIDGET_MAX_TEXT - 1);
    w->on_change = on_change;
    if (win->widget_count == 1 || !widget_is_focusable(win->widgets[win->focused_widget].type)) {
        win->focused_widget = win->widget_count - 1;
        w->focused = TRUE;
    }
    return w;
}

ATUI_WIDGET* ATUI_WADD_PASSWORD(
    ATUI_WINDOW *win, U32 y, U32 x, U32 width,
    const CHAR *placeholder, VOID (*on_change)(ATUI_WIDGET*))
{
    ATUI_WIDGET *w = ATUI_WADD_TEXTBOX(win, y, x, width, placeholder, on_change);
    if (w) w->data.textbox.password_mode = TRUE;
    return w;
}

ATUI_WIDGET* ATUI_WADD_CHECKBOX(
    ATUI_WINDOW *win, U32 y, U32 x,
    const CHAR *text, BOOL initial, VOID (*on_change)(ATUI_WIDGET*))
{
    ATUI_WIDGET *w = widget_alloc(win);
    if (!w) return NULLPTR;
    w->type = ATUI_WIDGET_CHECKBOX;
    w->x = x; w->y = y;
    STRNCPY(w->text, text, ATUI_WIDGET_MAX_TEXT - 1);
    w->w = STRLEN(text) + 4; // [x] Text
    w->h = 1;
    w->data.checkbox.checked = initial;
    w->on_change = on_change;
    if (win->widget_count == 1 || !widget_is_focusable(win->widgets[win->focused_widget].type)) {
        win->focused_widget = win->widget_count - 1;
        w->focused = TRUE;
    }
    return w;
}

ATUI_WIDGET* ATUI_WADD_RADIO(
    ATUI_WINDOW *win, U32 y, U32 x,
    const CHAR *text, U32 group_id, BOOL selected,
    VOID (*on_change)(ATUI_WIDGET*))
{
    ATUI_WIDGET *w = widget_alloc(win);
    if (!w) return NULLPTR;
    w->type = ATUI_WIDGET_RADIO;
    w->x = x; w->y = y;
    STRNCPY(w->text, text, ATUI_WIDGET_MAX_TEXT - 1);
    w->w = STRLEN(w->text) + 4; // (o) Text
    w->h = 1;
    w->data.radio.group_id = group_id;
    w->data.radio.selected = selected;
    w->on_change = on_change;
    if (win->widget_count == 1 || !widget_is_focusable(win->widgets[win->focused_widget].type)) {
        win->focused_widget = win->widget_count - 1;
        w->focused = TRUE;
    }
    return w;
}

ATUI_WIDGET* ATUI_WADD_DROPDOWN(
    ATUI_WINDOW *win, U32 y, U32 x, U32 width,
    VOID (*on_change)(ATUI_WIDGET*))
{
    ATUI_WIDGET *w = widget_alloc(win);
    if (!w) return NULLPTR;
    w->type = ATUI_WIDGET_DROPDOWN;
    w->x = x; w->y = y;
    w->w = width + 3; // [text  v]
    w->h = 1;
    w->data.dropdown.max_visible = 5;
    w->on_change = on_change;
    if (win->widget_count == 1 || !widget_is_focusable(win->widgets[win->focused_widget].type)) {
        win->focused_widget = win->widget_count - 1;
        w->focused = TRUE;
    }
    return w;
}

VOID ATUI_DROPDOWN_ADD_ITEM(ATUI_WIDGET *dropdown, const CHAR *item) {
    if (!dropdown || dropdown->type != ATUI_WIDGET_DROPDOWN) return;
    ATUI_DROPDOWN *dd = &dropdown->data.dropdown;
    if (dd->item_count >= 16) return;
    STRNCPY(dd->items[dd->item_count], item, 31);
    dd->item_count++;
}

ATUI_WIDGET* ATUI_WADD_PROGRESSBAR(
    ATUI_WINDOW *win, U32 y, U32 x, U32 width, U32 max_val)
{
    ATUI_WIDGET *w = widget_alloc(win);
    if (!w) return NULLPTR;
    w->type = ATUI_WIDGET_PROGRESSBAR;
    w->x = x; w->y = y;
    w->w = width;
    w->h = 1;
    w->data.progress.max = max_val > 0 ? max_val : 100;
    w->data.progress.value = 0;
    w->data.progress.show_percent = TRUE;
    return w;
}

VOID ATUI_PROGRESSBAR_SET(ATUI_WIDGET *bar, U32 value) {
    if (!bar || bar->type != ATUI_WIDGET_PROGRESSBAR) return;
    bar->data.progress.value = value;
}

ATUI_WIDGET* ATUI_WADD_SEPARATOR(
    ATUI_WINDOW *win, U32 y, U32 x, U32 width)
{
    ATUI_WIDGET *w = widget_alloc(win);
    if (!w) return NULLPTR;
    w->type = ATUI_WIDGET_SEPARATOR;
    w->x = x; w->y = y;
    w->w = width;
    w->h = 1;
    return w;
}

/* ================================================================ */
/*  focus management                                                */
/* ================================================================ */

VOID ATUI_WSET_FOCUS(ATUI_WINDOW *win, U32 index) {
    if (!win || index >= win->widget_count) return;
    // Clear old focus
    if (win->focused_widget < win->widget_count)
        win->widgets[win->focused_widget].focused = FALSE;
    win->focused_widget = index;
    win->widgets[index].focused = TRUE;
}

ATUI_WIDGET* ATUI_WGET_FOCUS(ATUI_WINDOW *win) {
    if (!win || win->widget_count == 0) return NULLPTR;
    if (win->focused_widget >= win->widget_count) return NULLPTR;
    return &win->widgets[win->focused_widget];
}

VOID ATUI_WNEXT_FOCUS(ATUI_WINDOW *win) {
    if (!win || win->widget_count == 0) return;

    // Unfocus current
    if (win->focused_widget < win->widget_count)
        win->widgets[win->focused_widget].focused = FALSE;

    U32 start = win->focused_widget;
    U32 idx = start;
    for (U32 i = 0; i < win->widget_count; i++) {
        idx = (idx + 1) % win->widget_count;
        ATUI_WIDGET *w = &win->widgets[idx];
        if (w->visible && w->enabled && widget_is_focusable(w->type)) {
            // Close any open dropdown on the previous widget
            if (win->widgets[start].type == ATUI_WIDGET_DROPDOWN)
                win->widgets[start].data.dropdown.open = FALSE;
            win->focused_widget = idx;
            w->focused = TRUE;
            return;
        }
    }
    // Nothing focusable — re-focus original
    win->widgets[start].focused = TRUE;
}

VOID ATUI_WPREV_FOCUS(ATUI_WINDOW *win) {
    if (!win || win->widget_count == 0) return;

    if (win->focused_widget < win->widget_count)
        win->widgets[win->focused_widget].focused = FALSE;

    U32 start = win->focused_widget;
    U32 idx = start;
    for (U32 i = 0; i < win->widget_count; i++) {
        idx = (idx == 0) ? win->widget_count - 1 : idx - 1;
        ATUI_WIDGET *w = &win->widgets[idx];
        if (w->visible && w->enabled && widget_is_focusable(w->type)) {
            if (win->widgets[start].type == ATUI_WIDGET_DROPDOWN)
                win->widgets[start].data.dropdown.open = FALSE;
            win->focused_widget = idx;
            w->focused = TRUE;
            return;
        }
    }
    win->widgets[start].focused = TRUE;
}

/* ================================================================ */
/*  getters                                                         */
/* ================================================================ */

CHAR* ATUI_TEXTBOX_GET_TEXT(ATUI_WIDGET *w) {
    if (!w || w->type != ATUI_WIDGET_TEXTBOX) return NULLPTR;
    return w->data.textbox.buffer;
}

BOOL ATUI_CHECKBOX_GET(ATUI_WIDGET *w) {
    if (!w || w->type != ATUI_WIDGET_CHECKBOX) return FALSE;
    return w->data.checkbox.checked;
}

U32 ATUI_DROPDOWN_GET_SELECTED(ATUI_WIDGET *w) {
    if (!w || w->type != ATUI_WIDGET_DROPDOWN) return 0;
    return w->data.dropdown.selected;
}

CHAR* ATUI_DROPDOWN_GET_TEXT(ATUI_WIDGET *w) {
    if (!w || w->type != ATUI_WIDGET_DROPDOWN) return NULLPTR;
    ATUI_DROPDOWN *dd = &w->data.dropdown;
    if (dd->selected >= dd->item_count) return NULLPTR;
    return dd->items[dd->selected];
}

U32 ATUI_RADIO_GET_SELECTED(ATUI_WINDOW *win, U32 group_id) {
    if (!win) return 0;
    for (U32 i = 0; i < win->widget_count; i++) {
        ATUI_WIDGET *w = &win->widgets[i];
        if (w->type == ATUI_WIDGET_RADIO &&
            w->data.radio.group_id == group_id &&
            w->data.radio.selected)
            return i;
    }
    return 0;
}

/* ================================================================ */
/*  widget property setters                                         */
/* ================================================================ */

VOID ATUI_WIDGET_SET_VISIBLE(ATUI_WIDGET *w, BOOL visible) {
    if (w) w->visible = visible;
}

VOID ATUI_WIDGET_SET_ENABLED(ATUI_WIDGET *w, BOOL enabled) {
    if (w) w->enabled = enabled;
}

VOID ATUI_WIDGET_SET_TEXT(ATUI_WIDGET *w, const CHAR *text) {
    if (!w || !text) return;
    STRNCPY(w->text, text, ATUI_WIDGET_MAX_TEXT - 1);
    // Recalculate width for simple types
    switch (w->type) {
        case ATUI_WIDGET_BUTTON:
            w->w = STRLEN(w->text) + 2;
            break;
        case ATUI_WIDGET_LABEL:
            w->w = STRLEN(w->text);
            break;
        case ATUI_WIDGET_CHECKBOX:
            w->w = STRLEN(w->text) + 4;
            break;
        case ATUI_WIDGET_RADIO:
            w->w = STRLEN(w->text) + 4;
            break;
        default:
            break;
    }
}

/* ================================================================ */
/*  per-widget key handlers                                         */
/* ================================================================ */

static VOID textbox_handle_key(ATUI_WIDGET *w, U32 key) {
    ATUI_TEXTBOX *tb = &w->data.textbox;

    if (key >= 32 && key < 256 && key != 127) {
        // Printable character
        if (tb->len < 127) {
            // Shift chars right from cursor
            for (U32 i = tb->len; i > tb->cursor; i--)
                tb->buffer[i] = tb->buffer[i - 1];
            tb->buffer[tb->cursor] = (CHAR)key;
            tb->len++;
            tb->cursor++;
            tb->buffer[tb->len] = '\0';
            // Auto-scroll
            if (tb->cursor > tb->scroll_offset + tb->visible_width)
                tb->scroll_offset = tb->cursor - tb->visible_width;
            if (w->on_change) w->on_change(w);
        }
    } else if (key == 8 || key == 127) {
        // Backspace
        if (tb->cursor > 0) {
            for (U32 i = tb->cursor - 1; i < tb->len - 1; i++)
                tb->buffer[i] = tb->buffer[i + 1];
            tb->len--;
            tb->cursor--;
            tb->buffer[tb->len] = '\0';
            if (tb->scroll_offset > 0 && tb->cursor < tb->scroll_offset)
                tb->scroll_offset = tb->cursor;
            if (w->on_change) w->on_change(w);
        }
    } else if (key == KEY_ARROW_LEFT) {
        if (tb->cursor > 0) {
            tb->cursor--;
            if (tb->cursor < tb->scroll_offset)
                tb->scroll_offset = tb->cursor;
        }
    } else if (key == KEY_ARROW_RIGHT) {
        if (tb->cursor < tb->len) {
            tb->cursor++;
            if (tb->cursor > tb->scroll_offset + tb->visible_width)
                tb->scroll_offset = tb->cursor - tb->visible_width;
        }
    } else if (key == KEY_HOME) {
        tb->cursor = 0;
        tb->scroll_offset = 0;
    } else if (key == KEY_END) {
        tb->cursor = tb->len;
        if (tb->cursor > tb->visible_width)
            tb->scroll_offset = tb->cursor - tb->visible_width;
    } else if (key == KEY_DELETE) {
        if (tb->cursor < tb->len) {
            for (U32 i = tb->cursor; i < tb->len - 1; i++)
                tb->buffer[i] = tb->buffer[i + 1];
            tb->len--;
            tb->buffer[tb->len] = '\0';
            if (w->on_change) w->on_change(w);
        }
    }
}

static VOID dropdown_handle_key(ATUI_WIDGET *w, U32 key) {
    ATUI_DROPDOWN *dd = &w->data.dropdown;

    if (key == KEY_ENTER || key == ' ') {
        if (!dd->open) {
            dd->open = TRUE;
        } else {
            dd->open = FALSE;
            if (w->on_change) w->on_change(w);
        }
    } else if (dd->open) {
        if (key == KEY_ARROW_UP && dd->selected > 0) {
            dd->selected--;
            if (dd->selected < dd->scroll_offset)
                dd->scroll_offset = dd->selected;
        } else if (key == KEY_ARROW_DOWN && dd->selected + 1 < dd->item_count) {
            dd->selected++;
            if (dd->selected >= dd->scroll_offset + dd->max_visible)
                dd->scroll_offset = dd->selected - dd->max_visible + 1;
        } else if (key == KEY_ESC) {
            dd->open = FALSE;
        }
    } else {
        if (key == KEY_ARROW_UP && dd->selected > 0)
            dd->selected--;
        else if (key == KEY_ARROW_DOWN && dd->selected + 1 < dd->item_count)
            dd->selected++;
    }
}

static VOID radio_select(ATUI_WINDOW *win, ATUI_WIDGET *w) {
    U32 gid = w->data.radio.group_id;
    for (U32 i = 0; i < win->widget_count; i++) {
        ATUI_WIDGET *other = &win->widgets[i];
        if (other->type == ATUI_WIDGET_RADIO &&
            other->data.radio.group_id == gid)
            other->data.radio.selected = FALSE;
    }
    w->data.radio.selected = TRUE;
    if (w->on_change) w->on_change(w);
}

/* ================================================================ */
/*  main key dispatch for windows                                   */
/* ================================================================ */

VOID ATUI_WINDOW_HANDLE_KEY(ATUI_WINDOW *win, U32 key) {
    if (!win || win->widget_count == 0) return;

    // Tab → next focus
    if (key == '\t' || key == KEY_TAB) {
        ATUI_WNEXT_FOCUS(win);
        return;
    }

    ATUI_WIDGET *w = ATUI_WGET_FOCUS(win);
    if (!w || !w->enabled) return;

    switch (w->type) {
        case ATUI_WIDGET_BUTTON:
            // Enter / Space — activate
            if (key == KEY_ENTER || key == ' ') {
                if (w->on_press) w->on_press(w);
            }
            // Arrow keys move focus between buttons / focusable widgets
            else if (key == KEY_ARROW_LEFT || key == KEY_ARROW_UP) {
                ATUI_WPREV_FOCUS(win);
            }
            else if (key == KEY_ARROW_RIGHT || key == KEY_ARROW_DOWN) {
                ATUI_WNEXT_FOCUS(win);
            }
            break;

        case ATUI_WIDGET_TEXTBOX:
            // Enter in a textbox advances focus to the next widget
            if (key == KEY_ENTER) {
                ATUI_WNEXT_FOCUS(win);
            } else {
                textbox_handle_key(w, key);
            }
            break;

        case ATUI_WIDGET_CHECKBOX:
            if (key == KEY_ENTER || key == ' ') {
                w->data.checkbox.checked = !w->data.checkbox.checked;
                if (w->on_change) w->on_change(w);
            }
            else if (key == KEY_ARROW_LEFT || key == KEY_ARROW_UP)  ATUI_WPREV_FOCUS(win);
            else if (key == KEY_ARROW_RIGHT || key == KEY_ARROW_DOWN) ATUI_WNEXT_FOCUS(win);
            break;

        case ATUI_WIDGET_RADIO:
            if (key == KEY_ENTER || key == ' ') {
                radio_select(win, w);
            }
            else if (key == KEY_ARROW_LEFT || key == KEY_ARROW_UP)  ATUI_WPREV_FOCUS(win);
            else if (key == KEY_ARROW_RIGHT || key == KEY_ARROW_DOWN) ATUI_WNEXT_FOCUS(win);
            break;

        case ATUI_WIDGET_DROPDOWN:
            dropdown_handle_key(w, key);
            break;

        default:
            break;
    }
}

/* ================================================================ */
/*  form system                                                     */
/* ================================================================ */

ATUI_FORM* ATUI_FORM_CREATE(ATUI_WINDOW *win, VOID (*on_submit)(ATUI_FORM*)) {
    if (!win) return NULLPTR;
    ATUI_FORM *form = MAlloc(sizeof(ATUI_FORM));
    if (!form) return NULLPTR;
    MEMZERO(form, sizeof(ATUI_FORM));
    form->win = win;
    form->on_submit = on_submit;
    return form;
}

VOID ATUI_FORM_ADD_FIELD(ATUI_FORM *form, ATUI_WIDGET *w) {
    if (!form || !w || form->field_count >= ATUI_MAX_FORM_FIELDS) return;
    // Find widget index in the window
    ATUI_WINDOW *win = form->win;
    for (U32 i = 0; i < win->widget_count; i++) {
        if (&win->widgets[i] == w) {
            form->field_indices[form->field_count++] = i;
            break;
        }
    }
}

static VOID form_set_field(ATUI_FORM *form, U32 field_idx) {
    ATUI_WINDOW *win = form->win;
    // Unfocus old
    U32 old = form->field_indices[form->current_field];
    win->widgets[old].focused = FALSE;
    // Focus new
    form->current_field = field_idx;
    U32 new_idx = form->field_indices[field_idx];
    win->widgets[new_idx].focused = TRUE;
    win->focused_widget = new_idx;
}

VOID ATUI_FORM_HANDLE_KEY(ATUI_FORM *form, U32 key, BOOL shift) {
    if (!form || !form->win || form->field_count == 0) return;

    ATUI_WINDOW *win = form->win;
    ATUI_WIDGET *w = &win->widgets[form->field_indices[form->current_field]];

    // Tab / Shift-Tab — cycle fields (shift flag from caller)
    if (key == '\t' || key == KEY_TAB) {
        U32 next;
        if (shift) {
            next = (form->current_field == 0)
                ? form->field_count - 1
                : form->current_field - 1;
        } else {
            next = (form->current_field + 1) % form->field_count;
        }
        form_set_field(form, next);
        return;
    }

    // Enter — activate or advance
    if (key == KEY_ENTER) {
        if (w->type == ATUI_WIDGET_BUTTON) {
            // Activate button and optionally submit form
            if (w->on_press) w->on_press(w);
            if (form->on_submit) {
                form->submitted = TRUE;
                form->on_submit(form);
            }
            return;
        }
        // For checkboxes toggle and advance; for textboxes just advance
        if (w->type == ATUI_WIDGET_CHECKBOX) {
            w->data.checkbox.checked = !w->data.checkbox.checked;
            if (w->on_change) w->on_change(w);
        } else if (w->type == ATUI_WIDGET_RADIO) {
            radio_select(win, w);
        }
        // Advance to next field
        form_set_field(form, (form->current_field + 1) % form->field_count);
        return;
    }

    // Arrow keys between fields (useful for button rows)
    if (key == KEY_ARROW_RIGHT || key == KEY_ARROW_DOWN) {
        form_set_field(form, (form->current_field + 1) % form->field_count);
        return;
    }
    if (key == KEY_ARROW_LEFT || key == KEY_ARROW_UP) {
        U32 prev = (form->current_field == 0)
            ? form->field_count - 1 : form->current_field - 1;
        form_set_field(form, prev);
        return;
    }

    // Everything else goes to the focused widget's handler
    ATUI_WINDOW_HANDLE_KEY(win, key);
}

VOID ATUI_FORM_DESTROY(ATUI_FORM *form) {
    if (form) MFree(form);
}

/* ================================================================ */
/*  Ready-made dialogs and widgets (modal)                          */
/* ================================================================ */

#include <LIBRARIES/ATUI/ATUI_WIDGETS.h>

INT ATUI_MSGBOX_OK(const CHAR *title, const CHAR *message) {
    ATUI_WINDOW *win = ATUI_NEWWIN(7, 40, 8, 20);
    win->boxed = TRUE;
    ATUI_WBOX(win);
    ATUI_WSET_TITLE(win, title ? title : "Message");
    ATUI_WADD_LABEL(win, 2, 3, message);   // widget 0 — label
    ATUI_WADD_BUTTON(win, 4, 16, "OK", NULLPTR); // widget 1 — button
    ATUI_WSET_FOCUS(win, 1); // focus the button
    ATUI_WREFRESH(win);
    while (1) {
        PS2_KB_DATA *kp = ATUI_GETCH();
        if (!kp) continue;
        U32 code = kp->cur.keycode;
        if (code == KEY_ENTER || code == ' ' || code == KEY_ESC) break;
        ATUI_WINDOW_HANDLE_KEY(win, code);
        ATUI_WREFRESH(win);
    }
    ATUI_DELWIN(win);
    return 1;
}

INT ATUI_MSGBOX_YESNO(const CHAR *title, const CHAR *message) {
    ATUI_WINDOW *win = ATUI_NEWWIN(7, 44, 8, 18);
    win->boxed = TRUE;
    ATUI_WBOX(win);
    ATUI_WSET_TITLE(win, title ? title : "Confirm");
    ATUI_WADD_LABEL(win, 2, 3, message);         // widget 0 — label
    ATUI_WADD_BUTTON(win, 4, 10, "Yes", NULLPTR); // widget 1
    ATUI_WADD_BUTTON(win, 4, 24, "No",  NULLPTR); // widget 2
    ATUI_WSET_FOCUS(win, 1); // start on Yes
    ATUI_WREFRESH(win);
    while (1) {
        PS2_KB_DATA *kp = ATUI_GETCH_NB();
        ATUI_WREFRESH(win);
        if (!kp) continue;
        U32 code = kp->cur.keycode;
        if (code == KEY_ENTER || code == ' ') break;
        if (code == KEY_ESC) { ATUI_WSET_FOCUS(win, 2); break; } // ESC = No
        ATUI_WINDOW_HANDLE_KEY(win, code);
        ATUI_WREFRESH(win);
    }
    U32 focused = win->focused_widget; // 1=Yes, 2=No
    ATUI_DELWIN(win);
    return (focused == 1) ? 1 : 0;
}

INT ATUI_MSGBOX_INPUT(const CHAR *title, const CHAR *message, CHAR *buf, U32 buflen) {
    ATUI_WINDOW *win = ATUI_NEWWIN(9, 48, 7, 16);
    win->boxed = TRUE;
    ATUI_WBOX(win);
    ATUI_WSET_TITLE(win, title ? title : "Input");
    ATUI_WADD_LABEL(win, 2, 3, message);               // widget 0 — label
    ATUI_WIDGET *tb = ATUI_WADD_TEXTBOX(win, 4, 3, 38, "", NULLPTR); // widget 1
    ATUI_WADD_BUTTON(win, 6, 12, "OK",     NULLPTR);   // widget 2
    ATUI_WADD_BUTTON(win, 6, 26, "Cancel", NULLPTR);   // widget 3
    ATUI_WSET_FOCUS(win, 1); // start in textbox
    ATUI_WREFRESH(win);
    BOOL cancelled = FALSE;
    while (1) {
        PS2_KB_DATA *kp = ATUI_GETCH();
        if (!kp) continue;
        U32 code = kp->cur.keycode;
        if (code == KEY_ESC) { cancelled = TRUE; break; }
        if (code == KEY_ENTER) {
            U32 f = win->focused_widget;
            if (f == 2) break;         // OK
            if (f == 3) { cancelled = TRUE; break; } // Cancel
            // textbox or other: advance to next
        }
        ATUI_WINDOW_HANDLE_KEY(win, code);
        ATUI_WREFRESH(win);
    }
    if (!cancelled)
        STRNCPY(buf, ATUI_TEXTBOX_GET_TEXT(tb), buflen - 1);
    else
        buf[0] = '\0';
    ATUI_DELWIN(win);
    return cancelled ? 0 : 1;
}

INT ATUI_MSGBOX_LIST(const CHAR *title, const CHAR *message, const CHAR **items, U32 item_count) {
    ATUI_WINDOW *win = ATUI_NEWWIN(10 + item_count, 48, 6, 16);
    win->boxed = TRUE;
    ATUI_WBOX(win);
    ATUI_WSET_TITLE(win, title ? title : "Select");
    ATUI_WADD_LABEL(win, 2, 3, message);
    U32 y = 4;
    U32 sel = 0;
    for (U32 i = 0; i < item_count; i++) {
        ATUI_WADD_LABEL(win, y + i, 6, items[i]);
    }
    // widget 0 = message label, widgets 1..item_count = item labels
    // Pre-highlight first item
    if (item_count > 0) {
        win->widgets[1].fg = VBE_WHITE;
        win->widgets[1].bg = VBE_BLUE;
    }
    ATUI_WREFRESH(win);
    while (1) {
        PS2_KB_DATA *kp = ATUI_GETCH();
        if (!kp) continue;
        U32 code = kp->cur.keycode;
        if (code == KEY_ENTER) break;
        if (code == KEY_ARROW_UP   && sel > 0)              sel--;
        if (code == KEY_ARROW_DOWN && sel + 1 < item_count) sel++;
        if (code == KEY_TAB)                                 sel = (sel + 1) % item_count;
        if (code == KEY_ESC) { sel = (U32)-1; break; }
        // Update highlight — items start at widget index 1
        for (U32 i = 0; i < item_count; i++) {
            win->widgets[1 + i].fg = (i == sel) ? VBE_WHITE : VBE_GRAY;
            win->widgets[1 + i].bg = (i == sel) ? VBE_BLUE  : VBE_BLACK;
        }
        ATUI_WREFRESH(win);
    }
    ATUI_DELWIN(win);
    return sel;
}

VOID ATUI_MSGBOX_PROGRESS(const CHAR *title, const CHAR *message, U32 *progress, U32 max) {
    ATUI_WINDOW *win = ATUI_NEWWIN(8, 48, 8, 16);
    win->boxed = TRUE;
    ATUI_WBOX(win);
    ATUI_WSET_TITLE(win, title ? title : "Progress");
    ATUI_WADD_LABEL(win, 2, 3, message);
    ATUI_WIDGET *bar = ATUI_WADD_PROGRESSBAR(win, 4, 3, 40, max);
    ATUI_WREFRESH(win);
    while (*progress < max) {
        ATUI_PROGRESSBAR_SET(bar, *progress);
        ATUI_WREFRESH(win);
        for (volatile int d = 0; d < 100000; d++); // crude delay
    }
    ATUI_DELWIN(win);
}

INT ATUI_COLOR_PICKER(const CHAR *title, VBE_COLOUR *out_color) {
    static const CHAR *colors[] = { "Black", "Blue", "Green", "Cyan", "Red", "Magenta", "Yellow", "White", "Gray", "Lime", "Aqua", "Crimson", "Orange" };
    static const VBE_COLOUR vbe_colors[] = { VBE_BLACK, VBE_BLUE, VBE_GREEN, VBE_CYAN, VBE_RED, VBE_MAGENTA, VBE_YELLOW, VBE_WHITE, VBE_GRAY, VBE_LIME, VBE_AQUA, VBE_CRIMSON, VBE_ORANGE };
    INT sel = ATUI_MSGBOX_LIST(title ? title : "Pick Color", "Select a color:", colors, 13);
    if (sel >= 0 && out_color) *out_color = vbe_colors[sel];
    return sel;
}

INT ATUI_FILE_PICKER(const CHAR *title, CHAR *buf, U32 buflen) {
    // Placeholder: just input box for now
    return ATUI_MSGBOX_INPUT(title ? title : "Pick File", "Enter file path:", buf, buflen);
}

// Additional ready-made dialogs
INT ATUI_MSGBOX_ERROR(const CHAR *title, const CHAR *message) {
    return ATUI_MSGBOX_OK(title ? title : "Error", message);
}
INT ATUI_MSGBOX_INFO(const CHAR *title, const CHAR *message) {
    return ATUI_MSGBOX_OK(title ? title : "Info", message);
}
INT ATUI_MSGBOX_WARN(const CHAR *title, const CHAR *message) {
    return ATUI_MSGBOX_OK(title ? title : "Warning", message);
}
INT ATUI_MSGBOX_CONFIRM(const CHAR *title, const CHAR *message) {
    return ATUI_MSGBOX_YESNO(title ? title : "Confirm", message);
}
// Spinbox: integer up/down selector
typedef struct { INT value, min, max, step; } SPINBOX_STATE;
INT ATUI_SPINBOX(const CHAR *title, const CHAR *message, INT min, INT max, INT step, INT *out_val) {
    INT val = min;
    ATUI_WINDOW *win = ATUI_NEWWIN(8, 40, 8, 20);
    win->boxed = TRUE;
    ATUI_WBOX(win);
    ATUI_WSET_TITLE(win, title ? title : "Select Value");
    ATUI_WADD_LABEL(win, 2, 3, message);
    ATUI_WIDGET *lbl = ATUI_WADD_LABEL(win, 4, 16, "");
    ATUI_WIDGET *ok = ATUI_WADD_BUTTON(win, 6, 16, "OK", NULLPTR);
    ATUI_WSET_FOCUS(win, 1);
    while (1) {
        CHAR buf[16];
        SPRINTF(buf, "%d", val);
        ATUI_WIDGET_SET_TEXT(lbl, buf);
        ATUI_WREFRESH(win);
        PS2_KB_DATA *kp = ATUI_GETCH();
        if (!kp) continue;
        U32 code = kp->cur.keycode;
        if (code == KEY_ENTER) break;
        if (code == KEY_ARROW_UP && val + step <= max) val += step;
        if (code == KEY_ARROW_DOWN && val - step >= min) val -= step;
        if (code == KEY_ESC) { val = min; break; }
    }
    if (out_val) *out_val = val;
    ATUI_DELWIN(win);
    return val;
}
// Slider: value selection with left/right
INT ATUI_SLIDER(const CHAR *title, const CHAR *message, INT min, INT max, INT *out_val) {
    INT val = min;
    ATUI_WINDOW *win = ATUI_NEWWIN(8, 48, 8, 16);
    win->boxed = TRUE;
    ATUI_WBOX(win);
    ATUI_WSET_TITLE(win, title ? title : "Slider");
    ATUI_WADD_LABEL(win, 2, 3, message);
    ATUI_WIDGET *bar = ATUI_WADD_PROGRESSBAR(win, 4, 3, 40, max - min);
    ATUI_WIDGET *ok = ATUI_WADD_BUTTON(win, 6, 20, "OK", NULLPTR);
    ATUI_WSET_FOCUS(win, 1);
    while (1) {
        ATUI_PROGRESSBAR_SET(bar, val - min);
        ATUI_WREFRESH(win);
        PS2_KB_DATA *kp = ATUI_GETCH();
        if (!kp) continue;
        U32 code = kp->cur.keycode;
        if (code == KEY_ENTER) break;
        if (code == KEY_ARROW_LEFT && val > min) val--;
        if (code == KEY_ARROW_RIGHT && val < max) val++;
        if (code == KEY_ESC) { val = min; break; }
    }
    if (out_val) *out_val = val;
    ATUI_DELWIN(win);
    return val;
}
// Keybind picker: waits for a key and returns its code
INT ATUI_KEYBIND_PICKER(const CHAR *title, U32 *out_key) {
    ATUI_WINDOW *win = ATUI_NEWWIN(6, 40, 10, 20);
    win->boxed = TRUE;
    ATUI_WBOX(win);
    ATUI_WSET_TITLE(win, title ? title : "Press a Key");
    ATUI_WADD_LABEL(win, 2, 3, "Press any key to bind...");
    ATUI_WREFRESH(win);
    PS2_KB_DATA *kp = ATUI_GETCH();
    U32 code = kp ? kp->cur.keycode : 0;
    if (out_key) *out_key = code;
    ATUI_DELWIN(win);
    return code;
}
// Font picker: placeholder, just input for now
INT ATUI_FONT_PICKER(const CHAR *title, CHAR *buf, U32 buflen) {
    return ATUI_MSGBOX_INPUT(title ? title : "Pick Font", "Enter font path:", buf, buflen);
}
// ATUI_FORM_YESNO — alias used by programs that want a Yes/No prompt
INT ATUI_FORM_YESNO(const CHAR *question) {
    return ATUI_MSGBOX_YESNO("Confirm", question);
}

// About box
VOID ATUI_ABOUT_BOX(const CHAR *app, const CHAR *author, const CHAR *desc) {
    ATUI_WINDOW *win = ATUI_NEWWIN(10, 48, 6, 16);
    win->boxed = TRUE;
    ATUI_WBOX(win);
    ATUI_WSET_TITLE(win, app ? app : "About");
    ATUI_WADD_LABEL(win, 2, 3, desc ? desc : "No description.");
    ATUI_WADD_LABEL(win, 4, 3, author ? author : "");
    ATUI_WADD_LABEL(win, 6, 3, "Powered by ATUI");
    ATUI_WIDGET *ok = ATUI_WADD_BUTTON(win, 8, 20, "OK", NULLPTR);
    ATUI_WSET_FOCUS(win, 3);
    ATUI_WREFRESH(win);
    while (1) {
        PS2_KB_DATA *kp = ATUI_GETCH();
        if (!kp) continue;
        U32 code = kp->cur.keycode;
        if (code == KEY_ENTER || code == ' ' || code == KEY_ESC) break;
        ATUI_WINDOW_HANDLE_KEY(win, code);
        ATUI_WREFRESH(win);
    }
    ATUI_DELWIN(win);
}
