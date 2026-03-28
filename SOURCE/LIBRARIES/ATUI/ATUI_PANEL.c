/*
 * ATUI_PANEL.c  —  Z-ordered panel management system
 *
 * Provides ncurses-compatible panel operations for managing overlapping
 * windows with proper Z-ordering.
 *
 * The panel deck is a doubly-linked list ordered bottom-to-top.
 * ATUI_UPDATE_PANELS() composites all visible panels in order.
 */

#define ATUI_INTERNALS
#include <LIBRARIES/ATUI/ATUI_TYPES.h>
#include <LIBRARIES/ATUI/ATUI.h>
#include <LIBRARIES/ATUI/ATUI_PANEL.h>
#include <STD/MEM.h>

/* ================================================================ */
/*  panel deck — doubly-linked list                                 */
/* ================================================================ */

static ATUI_PANEL *panel_bottom ATTRIB_DATA = { NULLPTR };
static ATUI_PANEL *panel_top    ATTRIB_DATA = { NULLPTR };

/// Unlink a panel from the deck (does not free it)
static VOID deck_unlink(ATUI_PANEL *pan) {
    if (!pan) return;

    if (pan->below)
        pan->below->above = pan->above;
    else
        panel_bottom = pan->above;

    if (pan->above)
        pan->above->below = pan->below;
    else
        panel_top = pan->below;

    pan->above = NULLPTR;
    pan->below = NULLPTR;
}

/// Link a panel at the top of the deck
static VOID deck_push_top(ATUI_PANEL *pan) {
    if (!pan) return;

    pan->below = panel_top;
    pan->above = NULLPTR;

    if (panel_top)
        panel_top->above = pan;
    else
        panel_bottom = pan;

    panel_top = pan;
}

/// Link a panel at the bottom of the deck
static VOID deck_push_bottom(ATUI_PANEL *pan) {
    if (!pan) return;

    pan->above = panel_bottom;
    pan->below = NULLPTR;

    if (panel_bottom)
        panel_bottom->below = pan;
    else
        panel_top = pan;

    panel_bottom = pan;
}

/* ================================================================ */
/*  public API                                                      */
/* ================================================================ */

ATUI_PANEL* ATUI_NEW_PANEL(ATUI_WINDOW *win) {
    if (!win) return NULLPTR;

    ATUI_PANEL *pan = MAlloc(sizeof(ATUI_PANEL));
    if (!pan) return NULLPTR;

    pan->win = win;
    pan->visible = TRUE;
    pan->above = NULLPTR;
    pan->below = NULLPTR;

    deck_push_top(pan);
    return pan;
}

VOID ATUI_DEL_PANEL(ATUI_PANEL *pan) {
    if (!pan) return;
    deck_unlink(pan);
    MFree(pan);
}

VOID ATUI_SHOW_PANEL(ATUI_PANEL *pan) {
    if (!pan) return;
    pan->visible = TRUE;
    // Move to top
    deck_unlink(pan);
    deck_push_top(pan);
}

VOID ATUI_HIDE_PANEL(ATUI_PANEL *pan) {
    if (!pan) return;
    pan->visible = FALSE;
}

VOID ATUI_TOP_PANEL(ATUI_PANEL *pan) {
    if (!pan) return;
    deck_unlink(pan);
    deck_push_top(pan);
}

VOID ATUI_BOTTOM_PANEL(ATUI_PANEL *pan) {
    if (!pan) return;
    deck_unlink(pan);
    deck_push_bottom(pan);
}

ATUI_WINDOW* ATUI_PANEL_WINDOW(ATUI_PANEL *pan) {
    if (!pan) return NULLPTR;
    return pan->win;
}

ATUI_PANEL* ATUI_PANEL_ABOVE(ATUI_PANEL *pan) {
    if (!pan) return NULLPTR;
    return pan->above;
}

ATUI_PANEL* ATUI_PANEL_BELOW(ATUI_PANEL *pan) {
    if (!pan) return NULLPTR;
    return pan->below;
}

VOID ATUI_REPLACE_PANEL(ATUI_PANEL *pan, ATUI_WINDOW *win) {
    if (!pan || !win) return;
    pan->win = win;
}

VOID ATUI_MOVE_PANEL(ATUI_PANEL *pan, U32 y, U32 x) {
    if (!pan || !pan->win) return;
    pan->win->y = y;
    pan->win->x = x;
    pan->win->needs_full_redraw = TRUE;
}

/* ================================================================ */
/*  ATUI_UPDATE_PANELS — composite all visible panels               */
/* ================================================================ */

VOID ATUI_UPDATE_PANELS() {
    ATUI_DISPLAY *d = GET_DISPLAY();
    if (!d) return;

    // Defined in ATUI_WIDGETS.c
    extern VOID ATUI_RENDER_WIDGETS(ATUI_WINDOW *win);

    // Step 1: Clear the main display
    for (U32 i = 0; i < d->cols * d->rows; i++) {
        d->back_buffer[i].c = ' ';
        d->back_buffer[i].fg = d->fg;
        d->back_buffer[i].bg = d->bg;
        d->back_buffer[i].attrs = ATUI_A_NORMAL;
        d->dirty[i] = TRUE;
    }

    // Step 2: Composite panels bottom-to-top
    // Walk from bottom to top — later panels overwrite earlier ones
    ATUI_PANEL *pan = panel_bottom;
    while (pan) {
        if (pan->visible && pan->win) {
            ATUI_WINDOW *win = pan->win;

            // Render widgets into the window buffer
            ATUI_RENDER_WIDGETS(win);

            // Composite window cells into main display
            for (U32 r = 0; r < win->h; r++) {
                U32 dr = win->y + r;
                if (dr >= d->rows) break;
                for (U32 c = 0; c < win->w; c++) {
                    U32 dc = win->x + c;
                    if (dc >= d->cols) break;

                    U32 widx = r * win->w + c;
                    U32 didx = dr * d->cols + dc;

                    d->back_buffer[didx] = win->buffer[widx];
                    d->dirty[didx] = TRUE;

                    // Update window front buffer
                    win->front_buffer[widx] = win->buffer[widx];
                    win->dirty[widx] = FALSE;
                }
            }
        }
        pan = pan->above;
    }

    // Step 3: Rasterize to framebuffer
    ATUI_COPY_TO_TCB_FRAMEBUFFER();
}
