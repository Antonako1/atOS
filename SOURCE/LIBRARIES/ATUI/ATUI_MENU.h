#ifndef ATUI_MENU_H
#define ATUI_MENU_H

#include <LIBRARIES/ATUI/ATUI.h>

/* ================================================================ */
/*  ATUI Menu System                                                */
/*                                                                  */
/*  Provides a standalone ncurses-style menu library with support    */
/*  for single/multi-column layout, multi-select, scrolling,        */
/*  and optional cascading submenus.                                */
/*                                                                  */
/*  Usage:                                                          */
/*      ATUI_MENU_ITEM items[3];                                    */
/*      ATUI_MENU_ITEM_INIT(&items[0], "Open", "Open a file");     */
/*      ATUI_MENU_ITEM_INIT(&items[1], "Save", "Save current");    */
/*      ATUI_MENU_ITEM_INIT(&items[2], "Quit", "Exit the app");    */
/*      ATUI_MENU *menu = ATUI_NEW_MENU(items, 3);                 */
/*      ATUI_WINDOW *mwin = ATUI_NEWWIN(10, 20, 2, 2);             */
/*      ATUI_SET_MENU_WIN(menu, mwin);                              */
/*      ATUI_POST_MENU(menu);                                       */
/*      // ... event loop with ATUI_MENU_DRIVER ...                 */
/* ================================================================ */

/* ================= MENU REQUEST CODES ================= */

#define ATUI_REQ_UP          1
#define ATUI_REQ_DOWN        2
#define ATUI_REQ_LEFT        3
#define ATUI_REQ_RIGHT       4
#define ATUI_REQ_FIRST       5
#define ATUI_REQ_LAST        6
#define ATUI_REQ_SCROLL_UP   7
#define ATUI_REQ_SCROLL_DOWN 8
#define ATUI_REQ_TOGGLE      9
#define ATUI_REQ_CLEAR       10
#define ATUI_REQ_SELECT      11

/* ================= MENU ITEM ================= */

#define ATUI_MENU_ITEM_TEXT_LEN  64
#define ATUI_MENU_ITEM_DESC_LEN  128

typedef struct _ATUI_MENU_ITEM {
    CHAR text[ATUI_MENU_ITEM_TEXT_LEN];
    CHAR description[ATUI_MENU_ITEM_DESC_LEN];
    BOOL enabled;
    BOOL selected;          // for multi-select mode
    U32 index;
    VOID (*on_select)(struct _ATUI_MENU_ITEM *item);
    struct _ATUI_MENU *submenu;  // optional cascading submenu
} ATUI_MENU_ITEM;

/* ================= MENU ================= */

typedef struct _ATUI_MENU {
    ATUI_MENU_ITEM items[ATUI_MAX_MENU_ITEMS];
    U32 item_count;
    U32 current;            // currently highlighted item index
    U32 top_row;            // first visible row (scrolling)
    U32 format_rows;        // visible rows in display area
    U32 format_cols;        // number of columns for multi-column layout
    CHAR mark;              // character prefixed to selected items (default '*')
    CHAR unmark;            // character prefixed to unselected items (default ' ')
    BOOL multi_select;      // allow toggling multiple items
    ATUI_WINDOW *win;       // main window for the menu
    ATUI_WINDOW *sub_win;   // scrollable sub-window within the menu window
    BOOL posted;            // is the menu currently drawn?
    VBE_COLOUR item_fg;
    VBE_COLOUR item_bg;
    VBE_COLOUR highlight_fg;
    VBE_COLOUR highlight_bg;
} ATUI_MENU;

/* ================= CONVENIENCE MACRO ================= */

#define ATUI_MENU_ITEM_INIT(item_ptr, t, d) do { \
    MEMZERO((item_ptr), sizeof(ATUI_MENU_ITEM));  \
    STRNCPY((item_ptr)->text, (t), ATUI_MENU_ITEM_TEXT_LEN - 1); \
    STRNCPY((item_ptr)->description, (d), ATUI_MENU_ITEM_DESC_LEN - 1); \
    (item_ptr)->enabled = TRUE; \
} while(0)

/* ================= API ================= */

/// Create a new menu from an array of items.
ATUI_MENU* ATUI_NEW_MENU(ATUI_MENU_ITEM *items, U32 count);

/// Free the menu (does NOT destroy windows).
VOID ATUI_FREE_MENU(ATUI_MENU *menu);

/// Set the main window to render the menu in.
VOID ATUI_SET_MENU_WIN(ATUI_MENU *menu, ATUI_WINDOW *win);

/// Set a sub-window (scrollable area within the main window).
VOID ATUI_SET_MENU_SUB(ATUI_MENU *menu, ATUI_WINDOW *sub);

/// Render the menu into its window.
VOID ATUI_POST_MENU(ATUI_MENU *menu);

/// Clear the menu from its window.
VOID ATUI_UNPOST_MENU(ATUI_MENU *menu);

/// Process a navigation/selection request. Returns 0 on success.
U32 ATUI_MENU_DRIVER(ATUI_MENU *menu, U32 request);

/// Get the currently highlighted item.
ATUI_MENU_ITEM* ATUI_CURRENT_ITEM(ATUI_MENU *menu);

/// Set multi-column layout: rows visible × columns.
VOID ATUI_SET_MENU_FORMAT(ATUI_MENU *menu, U32 rows, U32 cols);

/// Set the selection mark character.
VOID ATUI_SET_MENU_MARK(ATUI_MENU *menu, CHAR mark);

/// Set the unselection mark character.
VOID ATUI_SET_MENU_UNMARK(ATUI_MENU *menu, CHAR unmark);

/// Enable/disable multi-select mode.
VOID ATUI_SET_MENU_MULTI(ATUI_MENU *menu, BOOL multi);

/// Set menu colors.
VOID ATUI_SET_MENU_COLORS(ATUI_MENU *menu,
                           VBE_COLOUR item_fg, VBE_COLOUR item_bg,
                           VBE_COLOUR highlight_fg, VBE_COLOUR highlight_bg);

/// Get the index of the current item.
U32 ATUI_MENU_CURRENT_INDEX(ATUI_MENU *menu);

/// Get menu item by index.
ATUI_MENU_ITEM* ATUI_MENU_ITEM_AT(ATUI_MENU *menu, U32 index);

#endif // ATUI_MENU_H
