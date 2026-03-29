#ifndef ATUI_H
#define ATUI_H

#include <STD/TYPEDEF.h>
#include <DRIVERS/PS2/KEYBOARD_MOUSE.h>
#include <LIBRARIES/ATUI/ATUI_TYPES.h>

/* ================= INIT / SHUTDOWN ================= */

BOOL ATUI_INITIALIZE(PU8 fnt, ATUI_CONF cnf);
VOID ATUI_DESTROY();
VOID ATUI_REFRESH();

/* ================= SCREEN CONTROL ================= */

VOID ATUI_CLEAR();
VOID ATUI_CLRTOEOL();
VOID ATUI_CLRTOBOT();

/* ================= CURSOR ================= */

VOID ATUI_MOVE(U32 y, U32 x);
VOID ATUI_GETYX(U32 *y, U32 *x);
VOID ATUI_CURSOR_SET(BOOL visible);

/// Set cursor visibility: 0=invisible, 1=normal (underline), 2=very visible (block)
VOID ATUI_CURS_SET(U32 visibility);

/* ================= OUTPUT ================= */

VOID ATUI_ADDCH(CHAR c);
VOID ATUI_ADDSTR(CHAR *str);
VOID ATUI_PRINTW(CHAR *fmt, ...);
VOID ATUI_MVPRINTW(U32 y, U32 x, CHAR *fmt, ...);
VOID ATUI_MVADDSTR(U32 y, U32 x, CHAR *str);

/* ================= COLORS ================= */

VOID ATUI_SET_FG(VBE_COLOUR color);
VOID ATUI_SET_BG(VBE_COLOUR color);
VOID ATUI_SET_COLOR(VBE_COLOUR fg, VBE_COLOUR bg);

/* ================= ATTRIBUTES ================= */

/// OR attrs into the current attribute state
VOID ATUI_ATTRON(U8 attrs);

/// Clear attrs from the current attribute state
VOID ATUI_ATTROFF(U8 attrs);

/// Replace the current attribute state
VOID ATUI_ATTRSET(U8 attrs);

/// Get the current attribute state
U8 ATUI_ATTRGET();

/* ================= BACKGROUND CHARACTER ================= */

/// Set the background cell used for clearing
VOID ATUI_BKGD(CHAR_CELL cell);

/// Get the current background cell
CHAR_CELL ATUI_GETBKGD();

/* ================= READ-BACK (INCH / INSTR) ================= */

/// Return the CHAR_CELL at the current cursor position
CHAR_CELL ATUI_INCH();

/// Return the CHAR_CELL at (y, x)
CHAR_CELL ATUI_MVINCH(U32 y, U32 x);

/// Read characters from cursor to end of row into buf (max maxlen-1 characters)
U32 ATUI_INSTR(CHAR *buf, U32 maxlen);

/// Move to (y, x) then read characters into buf
U32 ATUI_MVINSTR(U32 y, U32 x, CHAR *buf, U32 maxlen);

/* ================= TOUCH ================= */

/// Mark all display cells dirty (forces full redraw on next refresh)
VOID ATUI_TOUCHSCREEN();

/// Clear all display dirty flags
VOID ATUI_UNTOUCHSCREEN();

/* ================= INSERT / DELETE LINES ================= */

/// Insert/delete lines at cursor: n>0 inserts, n<0 deletes
VOID ATUI_INSDELLN(I32 n);

/* ================= INPUT ================= */

PS2_KB_DATA* ATUI_GETCH();
PS2_KB_DATA* ATUI_GETCH_NB();
VOID ATUI_ECHO(BOOL enable);
VOID ATUI_RAW(BOOL enable);

/// Set GETCH timeout: <0=blocking, 0=non-blocking, >0=wait ms
VOID ATUI_TIMEOUT(I32 ms);

/// Read a line of input into buf (max maxlen-1 chars), returns length
U32 ATUI_GETSTR(CHAR *buf, U32 maxlen);

/// Return a string name for a keycode (e.g. "KEY_UP", "a")
CHAR* ATUI_KEYNAME(U32 keycode);

/* ================= MOUSE ================= */

/// Enable/disable mouse support
VOID ATUI_MOUSE_ENABLE(BOOL enable);

/// Poll mouse state (updates internal ATUI_MOUSE, returns pointer or NULLPTR)
ATUI_MOUSE* ATUI_MOUSE_POLL();

/// Get current mouse state without polling
ATUI_MOUSE* ATUI_MOUSE_GET();

/// Handle mouse input for a window (click on widgets, etc.)
/// Returns the widget that was clicked, or NULLPTR
ATUI_WIDGET* ATUI_WINDOW_HANDLE_MOUSE(ATUI_WINDOW *win, ATUI_MOUSE *ms);

/* ================= CONFIGURATION (ATRC) ================= */

/// Load ATUI configuration from an ATRC .CNF file
/// Returns TRUE on success. Caller may pass NULLPTR for path to use default.
BOOL ATUI_CONF_LOAD(const CHAR *cnf_path);

/// Save current ATUI configuration to an ATRC .CNF file
BOOL ATUI_CONF_SAVE(const CHAR *cnf_path);

/// Get a config value by block and key name (returns string or NULLPTR)
CHAR* ATUI_CONF_GET(const CHAR *block, const CHAR *key);

/// Set a config value by block and key name
BOOL ATUI_CONF_SET(const CHAR *block, const CHAR *key, const CHAR *value);

/// Destroy the config file descriptor and free resources
VOID ATUI_CONF_DESTROY();

/* ================================================= */
/* ================= WINDOW SYSTEM ================== */
/* ================================================= */

/// Create window
ATUI_WINDOW* ATUI_NEWWIN(U32 h, U32 w, U32 y, U32 x);

/// Delete window and free memory
VOID ATUI_DELWIN(ATUI_WINDOW* win);

/// Refresh window to main display
VOID ATUI_WREFRESH(ATUI_WINDOW* win);

/// Clear window contents
VOID ATUI_WCLEAR(ATUI_WINDOW* win);

/// Draw border box (single-line by default)
VOID ATUI_WBOX(ATUI_WINDOW* win);

/// Draw border box with specific style
VOID ATUI_WBOX_STYLED(ATUI_WINDOW* win, ATUI_BORDER_STYLE style);

/// Set window title (drawn centered in top border)
VOID ATUI_WSET_TITLE(ATUI_WINDOW* win, const CHAR *title);

/// Scroll window up by one line
VOID ATUI_WSCROLL(ATUI_WINDOW* win);

/// Move cursor inside window
VOID ATUI_WMOVE(ATUI_WINDOW* win, U32 y, U32 x);

/// Get cursor position in window
VOID ATUI_WGETYX(ATUI_WINDOW* win, U32 *y, U32 *x);

/// Add character to window
VOID ATUI_WADDCH(ATUI_WINDOW* win, CHAR c);

/// Add string to window
VOID ATUI_WADDSTR(ATUI_WINDOW* win, const CHAR *str);

/// Print formatted string into window
VOID ATUI_WPRINTW(ATUI_WINDOW* win, const CHAR *fmt, ...);

/// Move and add string
VOID ATUI_WMVADDSTR(ATUI_WINDOW* win, U32 y, U32 x, const CHAR *str);

/// Move and print formatted
VOID ATUI_WMVPRINTW(ATUI_WINDOW* win, U32 y, U32 x, const CHAR *fmt, ...);

/// Set window foreground/background colors
VOID ATUI_WSET_COLOR(ATUI_WINDOW* win, VBE_COLOUR fg, VBE_COLOUR bg);

/// Draw a horizontal line inside the window
VOID ATUI_WHLINE(ATUI_WINDOW *win, U32 y, U32 x, U32 len, CHAR ch);

/// Draw a vertical line inside the window
VOID ATUI_WVLINE(ATUI_WINDOW *win, U32 y, U32 x, U32 len, CHAR ch);

/// Typewriter roll-in: reveal str character-by-character at interior (y, x),
/// pausing ticks_per_char PIT ticks (~1 ms each) between chars.
/// Pass ticks_per_char=0 to write instantly.
VOID ATUI_WROLLSTR(ATUI_WINDOW *win, U32 y, U32 x, const CHAR *str, U32 ticks_per_char);

/// Move to (y, x) then roll-in str.
VOID ATUI_WMVROLLSTR(ATUI_WINDOW *win, U32 y, U32 x, const CHAR *str, U32 ticks_per_char);

/// Roll-in multiple lines from a string array, one per row, with optional
/// per-line delay.
VOID ATUI_WROLLLINES(ATUI_WINDOW *win, U32 start_y, U32 x,
                     const CHAR **lines, U32 line_count,
                     U32 ticks_per_char, U32 ticks_between_lines);

/* ================= WINDOW ATTRIBUTES ================= */

/// OR attrs into the window's current attribute state
VOID ATUI_WATTRON(ATUI_WINDOW *win, U8 attrs);

/// Clear attrs from the window's attribute state
VOID ATUI_WATTROFF(ATUI_WINDOW *win, U8 attrs);

/// Replace the window's attribute state
VOID ATUI_WATTRSET(ATUI_WINDOW *win, U8 attrs);

/// Get the window's current attribute state
U8 ATUI_WATTRGET(ATUI_WINDOW *win);

/* ================= WINDOW READ-BACK ================= */

/// Return the CHAR_CELL at the window's current cursor position
CHAR_CELL ATUI_WINCH(ATUI_WINDOW *win);

/// Move to (y, x) then return the CHAR_CELL
CHAR_CELL ATUI_MVWINCH(ATUI_WINDOW *win, U32 y, U32 x);

/// Read characters from cursor to end of row into buf
U32 ATUI_WINNSTR(ATUI_WINDOW *win, CHAR *buf, U32 maxlen);

/* ================= WINDOW TOUCH ================= */

/// Mark all window cells dirty
VOID ATUI_TOUCHWIN(ATUI_WINDOW *win);

/// Clear all window dirty flags
VOID ATUI_UNTOUCHWIN(ATUI_WINDOW *win);

/// Returns TRUE if any window cell is dirty
BOOL ATUI_IS_WINTOUCHED(ATUI_WINDOW *win);

/* ================= WINDOW SCROLL CONTROL ================= */

/// Enable/disable scrolling for a window
VOID ATUI_SCROLLOK(ATUI_WINDOW *win, BOOL enable);

/// Set the scrolling region within a window (interior row coords)
VOID ATUI_WSETSCRREG(ATUI_WINDOW *win, U32 top, U32 bot);

/* ================= WINDOW BACKGROUND ================= */

/// Set the window's background cell
VOID ATUI_WBKGD(ATUI_WINDOW *win, CHAR_CELL cell);

/// Get the window's background cell
CHAR_CELL ATUI_WGETBKGD(ATUI_WINDOW *win);

/* ================= WINDOW INSERT/DELETE LINES ================= */

/// Insert/delete lines at cursor within a window
VOID ATUI_WINSDELLN(ATUI_WINDOW *win, I32 n);

/* ================= WINDOW CLEAR TO EOL/BOT ================= */

/// Clear from cursor to end of line within a window
VOID ATUI_WCLRTOEOL(ATUI_WINDOW *win);

/// Clear from cursor to bottom of window
VOID ATUI_WCLRTOBOT(ATUI_WINDOW *win);

/* ================= WINDOW GETSTR ================= */

/// Read a line of input into buf via the window, returns length
U32 ATUI_WGETSTR(ATUI_WINDOW *win, CHAR *buf, U32 maxlen);

/* ================= WINDOW RESIZE ================= */

/// Resize a window (reallocs buffers, copies content that fits)
VOID ATUI_WRESIZE(ATUI_WINDOW *win, U32 new_h, U32 new_w);

/* ================= SUBWINDOWS ================= */

/// Create a subwindow sharing the parent's buffer at screen-absolute (y, x)
ATUI_WINDOW* ATUI_SUBWIN(ATUI_WINDOW *parent, U32 h, U32 w, U32 y, U32 x);

/// Create a derived window (coordinates relative to parent interior)
ATUI_WINDOW* ATUI_DERWIN(ATUI_WINDOW *parent, U32 h, U32 w, U32 y, U32 x);

/// Move a derived window within its parent
VOID ATUI_MVDERWIN(ATUI_WINDOW *win, U32 y, U32 x);

/* ================= COPY OPERATIONS ================= */

/// Copy non-blank characters from src to dst at overlapping positions
VOID ATUI_OVERLAY(ATUI_WINDOW *src, ATUI_WINDOW *dst);

/// Copy ALL cells from src to dst at overlapping positions
VOID ATUI_OVERWRITE(ATUI_WINDOW *src, ATUI_WINDOW *dst);

/// General rectangular copy with source/dest mapping
VOID ATUI_COPYWIN(ATUI_WINDOW *src, ATUI_WINDOW *dst,
                   U32 sminrow, U32 smincol,
                   U32 dminrow, U32 dmincol,
                   U32 dmaxrow, U32 dmaxcol, BOOL overlay);

/* ================= WINDOW INPUT ================= */

/// Handle keyboard input for window (widgets, focus, etc)
VOID ATUI_WINDOW_HANDLE_KEY(ATUI_WINDOW *win, U32 key);

/* ================= WIDGET SYSTEM ================= */

/// Add button widget
ATUI_WIDGET* ATUI_WADD_BUTTON(
    ATUI_WINDOW *win,
    U32 y, U32 x,
    const CHAR *text,
    VOID (*on_press)(ATUI_WIDGET*)
);

/// Add label widget
ATUI_WIDGET* ATUI_WADD_LABEL(
    ATUI_WINDOW *win,
    U32 y, U32 x,
    const CHAR *text
);

/// Add textbox / text-input widget
ATUI_WIDGET* ATUI_WADD_TEXTBOX(
    ATUI_WINDOW *win,
    U32 y, U32 x,
    U32 width,
    const CHAR *placeholder,
    VOID (*on_change)(ATUI_WIDGET*)
);

/// Add password textbox (masks input with '*')
ATUI_WIDGET* ATUI_WADD_PASSWORD(
    ATUI_WINDOW *win,
    U32 y, U32 x,
    U32 width,
    const CHAR *placeholder,
    VOID (*on_change)(ATUI_WIDGET*)
);

/// Add checkbox widget
ATUI_WIDGET* ATUI_WADD_CHECKBOX(
    ATUI_WINDOW *win,
    U32 y, U32 x,
    const CHAR *text,
    BOOL initial,
    VOID (*on_change)(ATUI_WIDGET*)
);

/// Add radio button widget (radio buttons with same group_id are exclusive)
ATUI_WIDGET* ATUI_WADD_RADIO(
    ATUI_WINDOW *win,
    U32 y, U32 x,
    const CHAR *text,
    U32 group_id,
    BOOL selected,
    VOID (*on_change)(ATUI_WIDGET*)
);

/// Add dropdown / combo-box widget
ATUI_WIDGET* ATUI_WADD_DROPDOWN(
    ATUI_WINDOW *win,
    U32 y, U32 x,
    U32 width,
    VOID (*on_change)(ATUI_WIDGET*)
);

/// Add an item to a dropdown widget
VOID ATUI_DROPDOWN_ADD_ITEM(ATUI_WIDGET *dropdown, const CHAR *item);

/// Add progress bar widget
ATUI_WIDGET* ATUI_WADD_PROGRESSBAR(
    ATUI_WINDOW *win,
    U32 y, U32 x,
    U32 width,
    U32 max_val
);

/// Set progress bar value
VOID ATUI_PROGRESSBAR_SET(ATUI_WIDGET *bar, U32 value);

/// Add horizontal separator
ATUI_WIDGET* ATUI_WADD_SEPARATOR(
    ATUI_WINDOW *win,
    U32 y, U32 x,
    U32 width
);

/// Set focused widget
VOID ATUI_WSET_FOCUS(ATUI_WINDOW *win, U32 index);

/// Get focused widget
ATUI_WIDGET* ATUI_WGET_FOCUS(ATUI_WINDOW *win);

/// Advance focus to the next focusable widget
VOID ATUI_WNEXT_FOCUS(ATUI_WINDOW *win);

/// Move focus to the previous focusable widget
VOID ATUI_WPREV_FOCUS(ATUI_WINDOW *win);

/// Get the text content of a textbox widget
CHAR* ATUI_TEXTBOX_GET_TEXT(ATUI_WIDGET *w);

/// Get the checked state of a checkbox widget
BOOL ATUI_CHECKBOX_GET(ATUI_WIDGET *w);

/// Get the selected index of a dropdown widget
U32 ATUI_DROPDOWN_GET_SELECTED(ATUI_WIDGET *w);

/// Get the selected item text of a dropdown widget
CHAR* ATUI_DROPDOWN_GET_TEXT(ATUI_WIDGET *w);

/// Get the selected radio button index within a group
U32 ATUI_RADIO_GET_SELECTED(ATUI_WINDOW *win, U32 group_id);

/// Set widget visibility
VOID ATUI_WIDGET_SET_VISIBLE(ATUI_WIDGET *w, BOOL visible);

/// Set widget enabled state
VOID ATUI_WIDGET_SET_ENABLED(ATUI_WIDGET *w, BOOL enabled);

/// Update widget label/text at runtime
VOID ATUI_WIDGET_SET_TEXT(ATUI_WIDGET *w, const CHAR *text);

/* ================= FORM SYSTEM ================= */

/// Create a form attached to a window
ATUI_FORM* ATUI_FORM_CREATE(ATUI_WINDOW *win, VOID (*on_submit)(ATUI_FORM*));

/// Add an existing widget to the form's tab order
VOID ATUI_FORM_ADD_FIELD(ATUI_FORM *form, ATUI_WIDGET *w);

/// Handle keyboard input for an active form (Tab / Shift-Tab / Enter)
VOID ATUI_FORM_HANDLE_KEY(ATUI_FORM *form, U32 key, BOOL shift);

/// Destroy form (does NOT destroy the window or widgets)
VOID ATUI_FORM_DESTROY(ATUI_FORM *form);

/* ================= INTERNALS ================= */

ATUI_DISPLAY *GET_DISPLAY();
#ifdef ATUI_INTERNALS

VOID ATUI_COPY_TO_TCB_FRAMEBUFFER();

#endif

#endif