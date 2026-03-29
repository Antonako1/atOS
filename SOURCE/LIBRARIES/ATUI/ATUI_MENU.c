/*
 * ATUI_MENU.c  —  Standalone menu system
 *
 * Provides ncurses-compatible menu operations with support for
 * single/multi-column layout, scrolling, multi-select, and
 * configurable mark characters.
 */

#include <LIBRARIES/ATUI/ATUI_TYPES.h>
#include <LIBRARIES/ATUI/ATUI.h>
#include <LIBRARIES/ATUI/ATUI_MENU.h>
#include <STD/MEM.h>
#include <STD/STRING.h>

/* ================================================================ */
/*  create / destroy                                                */
/* ================================================================ */

ATUI_MENU* ATUI_NEW_MENU(ATUI_MENU_ITEM *items, U32 count) {
    if (!items || count == 0) return NULLPTR;
    if (count > ATUI_MAX_MENU_ITEMS) count = ATUI_MAX_MENU_ITEMS;

    ATUI_MENU *menu = MAlloc(sizeof(ATUI_MENU));
    if (!menu) return NULLPTR;
    MEMZERO(menu, sizeof(ATUI_MENU));

    for (U32 i = 0; i < count; i++) {
        menu->items[i] = items[i];
        menu->items[i].index = i;
    }
    menu->item_count = count;
    menu->current = 0;
    menu->top_row = 0;
    menu->format_rows = 0; // 0 = auto from window size
    menu->format_cols = 1;
    menu->mark = '*';
    menu->unmark = ' ';
    menu->multi_select = FALSE;
    menu->posted = FALSE;

    // Default colors
    menu->item_fg = VBE_WHITE;
    menu->item_bg = VBE_BLACK;
    menu->highlight_fg = VBE_BLACK;
    menu->highlight_bg = VBE_WHITE;

    return menu;
}

VOID ATUI_FREE_MENU(ATUI_MENU *menu) {
    if (menu) MFree(menu);
}

/* ================================================================ */
/*  configuration                                                   */
/* ================================================================ */

VOID ATUI_SET_MENU_WIN(ATUI_MENU *menu, ATUI_WINDOW *win) {
    if (menu) menu->win = win;
}

VOID ATUI_SET_MENU_SUB(ATUI_MENU *menu, ATUI_WINDOW *sub) {
    if (menu) menu->sub_win = sub;
}

VOID ATUI_SET_MENU_FORMAT(ATUI_MENU *menu, U32 rows, U32 cols) {
    if (!menu) return;
    menu->format_rows = rows;
    if (cols == 0) cols = 1;
    menu->format_cols = cols;
}

VOID ATUI_SET_MENU_MARK(ATUI_MENU *menu, CHAR mark) {
    if (menu) menu->mark = mark;
}

VOID ATUI_SET_MENU_UNMARK(ATUI_MENU *menu, CHAR unmark) {
    if (menu) menu->unmark = unmark;
}

VOID ATUI_SET_MENU_MULTI(ATUI_MENU *menu, BOOL multi) {
    if (menu) menu->multi_select = multi;
}

VOID ATUI_SET_MENU_COLORS(ATUI_MENU *menu,
                           VBE_COLOUR item_fg, VBE_COLOUR item_bg,
                           VBE_COLOUR highlight_fg, VBE_COLOUR highlight_bg) {
    if (!menu) return;
    menu->item_fg = item_fg;
    menu->item_bg = item_bg;
    menu->highlight_fg = highlight_fg;
    menu->highlight_bg = highlight_bg;
}

ATUI_MENU_ITEM* ATUI_CURRENT_ITEM(ATUI_MENU *menu) {
    if (!menu || menu->current >= menu->item_count) return NULLPTR;
    return &menu->items[menu->current];
}

U32 ATUI_MENU_CURRENT_INDEX(ATUI_MENU *menu) {
    if (!menu) return 0;
    return menu->current;
}

ATUI_MENU_ITEM* ATUI_MENU_ITEM_AT(ATUI_MENU *menu, U32 index) {
    if (!menu || index >= menu->item_count) return NULLPTR;
    return &menu->items[index];
}

/* ================================================================ */
/*  rendering                                                       */
/* ================================================================ */

static VOID menu_render(ATUI_MENU *menu) {
    ATUI_WINDOW *win = menu->sub_win ? menu->sub_win : menu->win;
    if (!win) return;

    ATUI_WCLEAR(win);
    if (win->boxed) ATUI_WBOX(win);

    // Determine visible area
    U32 iy = win->boxed ? 1 : 0;
    U32 ix = win->boxed ? 1 : 0;
    U32 ih = win->h > 2 * iy ? win->h - 2 * iy : win->h;
    U32 iw = win->w > 2 * ix ? win->w - 2 * ix : win->w;

    U32 vis_rows = menu->format_rows > 0 ? menu->format_rows : ih;
    if (vis_rows > ih) vis_rows = ih;

    U32 cols = menu->format_cols;
    if (cols == 0) cols = 1;
    U32 col_width = iw / cols;
    if (col_width < 2) col_width = 2;

    // Ensure current item is visible (adjust top_row)
    U32 items_per_page = vis_rows * cols;
    if (menu->current < menu->top_row)
        menu->top_row = menu->current;
    if (menu->current >= menu->top_row + items_per_page)
        menu->top_row = menu->current - items_per_page + 1;

    // Render items
    for (U32 vi = 0; vi < items_per_page; vi++) {
        U32 item_idx = menu->top_row + vi;
        if (item_idx >= menu->item_count) break;

        ATUI_MENU_ITEM *item = &menu->items[item_idx];

        // Position in multi-column layout (column-major)
        U32 col_num = vi / vis_rows;
        U32 row_num = vi % vis_rows;

        U32 row = iy + row_num;
        U32 col = ix + col_num * col_width;

        BOOL is_current = (item_idx == menu->current);
        VBE_COLOUR fg = is_current ? menu->highlight_fg : menu->item_fg;
        VBE_COLOUR bg = is_current ? menu->highlight_bg : menu->item_bg;

        if (!item->enabled) {
            fg = VBE_GRAY;  // dimmed for disabled items
        }

        // Mark character
        CHAR mark = item->selected ? menu->mark : menu->unmark;

        // Build item text: "M text" where M is the mark
        U32 max_text = col_width > 2 ? col_width - 2 : 0;

        // Write mark
        if (col < ix + iw) {
            U32 idx = row * win->w + col;
            win->buffer[idx].c = mark;
            win->buffer[idx].fg = fg;
            win->buffer[idx].bg = bg;
            win->buffer[idx].attrs = is_current ? ATUI_A_STANDOUT : ATUI_A_NORMAL;
            win->dirty[idx] = TRUE;
        }

        // Write text
        U32 tlen = STRLEN(item->text);
        if (tlen > max_text) tlen = max_text;
        for (U32 i = 0; i < max_text; i++) {
            CHAR c = (i < tlen) ? item->text[i] : ' ';
            U32 cx = col + 1 + i;
            if (cx >= ix + iw) break;
            U32 idx = row * win->w + cx;
            win->buffer[idx].c = c;
            win->buffer[idx].fg = fg;
            win->buffer[idx].bg = bg;
            win->buffer[idx].attrs = is_current ? ATUI_A_STANDOUT : ATUI_A_NORMAL;
            win->dirty[idx] = TRUE;
        }

        // Trailing space to fill column
        U32 fill_x = col + 1 + max_text;
        if (fill_x < ix + iw && fill_x < col + col_width) {
            U32 idx = row * win->w + fill_x;
            win->buffer[idx].c = ' ';
            win->buffer[idx].fg = fg;
            win->buffer[idx].bg = bg;
            win->buffer[idx].attrs = ATUI_A_NORMAL;
            win->dirty[idx] = TRUE;
        }
    }

    // Show scroll indicators if needed
    if (menu->top_row > 0 && ih > 0) {
        // Up arrow at top-right
        U32 idx = iy * win->w + (ix + iw - 1);
        win->buffer[idx].c = (CHAR)ATUI_ARROW_UP;
        win->buffer[idx].fg = menu->item_fg;
        win->buffer[idx].bg = menu->item_bg;
        win->buffer[idx].attrs = ATUI_A_NORMAL;
        win->dirty[idx] = TRUE;
    }
    if (menu->top_row + items_per_page < menu->item_count && ih > 0) {
        // Down arrow at bottom-right
        U32 last_row = iy + vis_rows - 1;
        U32 idx = last_row * win->w + (ix + iw - 1);
        win->buffer[idx].c = (CHAR)ATUI_ARROW_DOWN;
        win->buffer[idx].fg = menu->item_fg;
        win->buffer[idx].bg = menu->item_bg;
        win->buffer[idx].attrs = ATUI_A_NORMAL;
        win->dirty[idx] = TRUE;
    }
}

/* ================================================================ */
/*  post / unpost                                                   */
/* ================================================================ */

VOID ATUI_POST_MENU(ATUI_MENU *menu) {
    if (!menu || !menu->win) return;
    menu->posted = TRUE;
    menu_render(menu);
    ATUI_WREFRESH(menu->sub_win ? menu->sub_win : menu->win);
    if (menu->sub_win && menu->win != menu->sub_win) {
        ATUI_WREFRESH(menu->win);
    }
}

VOID ATUI_UNPOST_MENU(ATUI_MENU *menu) {
    if (!menu) return;
    menu->posted = FALSE;
    if (menu->sub_win) ATUI_WCLEAR(menu->sub_win);
    if (menu->win) ATUI_WCLEAR(menu->win);
}

/* ================================================================ */
/*  menu_driver — handle navigation requests                        */
/* ================================================================ */

U32 ATUI_MENU_DRIVER(ATUI_MENU *menu, U32 request) {
    if (!menu || menu->item_count == 0) return 1;

    U32 vis_rows = menu->format_rows;
    if (vis_rows == 0) {
        ATUI_WINDOW *win = menu->sub_win ? menu->sub_win : menu->win;
        if (win) {
            U32 iy = win->boxed ? 1 : 0;
            vis_rows = win->h > 2 * iy ? win->h - 2 * iy : 1;
        } else {
            vis_rows = menu->item_count;
        }
    }
    U32 cols = menu->format_cols > 0 ? menu->format_cols : 1;

    switch (request) {
        case ATUI_REQ_DOWN: {
            // Find next enabled item
            U32 next = menu->current;
            for (U32 i = 0; i < menu->item_count - 1; i++) {
                next++;
                if (next >= menu->item_count) break;
                if (menu->items[next].enabled) {
                    menu->current = next;
                    break;
                }
            }
            break;
        }

        case ATUI_REQ_UP: {
            U32 prev = menu->current;
            for (U32 i = 0; i < menu->item_count; i++) {
                if (prev == 0) break;
                prev--;
                if (menu->items[prev].enabled) {
                    menu->current = prev;
                    break;
                }
            }
            break;
        }

        case ATUI_REQ_RIGHT: {
            // Move to next column in multi-column layout
            U32 next = menu->current + vis_rows;
            if (next < menu->item_count && menu->items[next].enabled)
                menu->current = next;
            break;
        }

        case ATUI_REQ_LEFT: {
            // Move to previous column
            if (menu->current >= vis_rows) {
                U32 prev = menu->current - vis_rows;
                if (menu->items[prev].enabled)
                    menu->current = prev;
            }
            break;
        }

        case ATUI_REQ_FIRST: {
            for (U32 i = 0; i < menu->item_count; i++) {
                if (menu->items[i].enabled) {
                    menu->current = i;
                    break;
                }
            }
            break;
        }

        case ATUI_REQ_LAST: {
            for (I32 i = (I32)menu->item_count - 1; i >= 0; i--) {
                if (menu->items[i].enabled) {
                    menu->current = (U32)i;
                    break;
                }
            }
            break;
        }

        case ATUI_REQ_SCROLL_DOWN: {
            U32 page = vis_rows * cols;
            U32 next = menu->current + page;
            if (next >= menu->item_count)
                next = menu->item_count - 1;
            menu->current = next;
            break;
        }

        case ATUI_REQ_SCROLL_UP: {
            U32 page = vis_rows * cols;
            if (menu->current >= page)
                menu->current -= page;
            else
                menu->current = 0;
            break;
        }

        case ATUI_REQ_TOGGLE: {
            if (menu->multi_select) {
                menu->items[menu->current].selected =
                    !menu->items[menu->current].selected;
            } else {
                // Single select: deselect all, select current
                for (U32 i = 0; i < menu->item_count; i++)
                    menu->items[i].selected = FALSE;
                menu->items[menu->current].selected = TRUE;
            }
            break;
        }

        case ATUI_REQ_CLEAR: {
            for (U32 i = 0; i < menu->item_count; i++)
                menu->items[i].selected = FALSE;
            break;
        }

        case ATUI_REQ_SELECT: {
            ATUI_MENU_ITEM *item = &menu->items[menu->current];
            if (!menu->multi_select) {
                for (U32 i = 0; i < menu->item_count; i++)
                    menu->items[i].selected = FALSE;
            }
            item->selected = TRUE;
            if (item->on_select) item->on_select(item);
            break;
        }

        default:
            return 1; // unknown request
    }

    // Re-render after navigation
    if (menu->posted) {
        menu_render(menu);
        ATUI_WINDOW *rwin = menu->sub_win ? menu->sub_win : menu->win;
        if (rwin) ATUI_WREFRESH(rwin);
    }

    return 0;
}
