#ifndef ATUI_PANEL_H
#define ATUI_PANEL_H

#include <LIBRARIES/ATUI/ATUI.h>

/* ================================================================ */
/*  ATUI Panel System                                               */
/*                                                                  */
/*  Provides Z-ordered overlapping window management, matching the  */
/*  ncurses panel library semantics.  Panels wrap ATUI_WINDOWs and  */
/*  are maintained in a doubly-linked deck ordered bottom-to-top.   */
/*                                                                  */
/*  Usage:                                                          */
/*      ATUI_WINDOW *w1 = ATUI_NEWWIN(10, 40, 5, 10);              */
/*      ATUI_PANEL  *p1 = ATUI_NEW_PANEL(w1);                      */
/*      ATUI_WINDOW *w2 = ATUI_NEWWIN(8, 30, 8, 20);               */
/*      ATUI_PANEL  *p2 = ATUI_NEW_PANEL(w2);                      */
/*      // ... write to windows ...                                 */
/*      ATUI_UPDATE_PANELS();   // composites all visible panels    */
/*      ATUI_REFRESH();         // rasterizes to framebuffer        */
/* ================================================================ */

typedef struct _ATUI_PANEL {
    ATUI_WINDOW *win;
    BOOL visible;
    struct _ATUI_PANEL *above;
    struct _ATUI_PANEL *below;
} ATUI_PANEL;

/// Create a new panel wrapping the given window. Inserts at top of deck.
ATUI_PANEL* ATUI_NEW_PANEL(ATUI_WINDOW *win);

/// Remove the panel from the deck and free the panel struct.
/// Does NOT destroy the underlying window.
VOID ATUI_DEL_PANEL(ATUI_PANEL *pan);

/// Make the panel visible and move it to the top of the deck.
VOID ATUI_SHOW_PANEL(ATUI_PANEL *pan);

/// Hide the panel (excluded from compositing).
VOID ATUI_HIDE_PANEL(ATUI_PANEL *pan);

/// Move the panel to the top of the deck.
VOID ATUI_TOP_PANEL(ATUI_PANEL *pan);

/// Move the panel to the bottom of the deck.
VOID ATUI_BOTTOM_PANEL(ATUI_PANEL *pan);

/// Return the window wrapped by this panel.
ATUI_WINDOW* ATUI_PANEL_WINDOW(ATUI_PANEL *pan);

/// Return the panel above this one (NULL if top).
ATUI_PANEL* ATUI_PANEL_ABOVE(ATUI_PANEL *pan);

/// Return the panel below this one (NULL if bottom).
ATUI_PANEL* ATUI_PANEL_BELOW(ATUI_PANEL *pan);

/// Composite all visible panels bottom-to-top into the main display,
/// handling overlaps correctly, then rasterize.
VOID ATUI_UPDATE_PANELS();

/// Replace the window wrapped by a panel.
VOID ATUI_REPLACE_PANEL(ATUI_PANEL *pan, ATUI_WINDOW *win);

/// Move the panel's window to a new screen position.
VOID ATUI_MOVE_PANEL(ATUI_PANEL *pan, U32 y, U32 x);

#endif // ATUI_PANEL_H
