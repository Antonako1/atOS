# ATUI — Advanced Text User Interface Library

ATUI is a static, C-based console UI library for atOS, providing windows, widgets, forms, and input handling for text-mode and graphical console applications. It supports extended ASCII (box-drawing, shading, etc) and is designed for both system and user programs.

Modelled after **ncurses**, ATUI provides cell attributes, panels with Z-ordering, a full menu system, subwindows, and copy operations — all with direct RGB color (no color-pair tables).

---

## Features

- **Window system**: Movable, boxed windows with titles, color, and compositing
- **Cell attributes**: Bold, underline, reverse, dim, blink, standout per-character
- **Panels**: Z-ordered window deck with show/hide, raise/lower, and compositing
- **Menus**: Multi-column scrolling menus with mark chars, multi-select, and drivers
- **Subwindows**: Derived windows (relative coords) and subwindows (absolute coords)
- **Copy operations**: overlay, overwrite, and rectangular copywin
- **Widgets**: Buttons, labels, textboxes, checkboxes, radio buttons, dropdowns, progress bars, separators
- **Forms**: Tab-order, field navigation, and submit callbacks
- **Input**: Keyboard navigation, configurable timeout, raw/echo, getstr
- **ACS characters**: Portable line-drawing macros mapped to CP437 codepoints
- **Custom colors**: Per-widget and per-window RGB foreground/background
- **Easy integration**: Single include, static linking, no dynamic allocation required by user

---

## Quick Start

### 1. Initialization

```c
ATUI_INITIALIZE(NULLPTR, ATUIC_DEFAULT); // Use default font and config
ATUI_SET_COLOR(VBE_WHITE, VBE_BLACK);    // Set default colors
ATUI_CURSOR_SET(FALSE);                  // Hide cursor if desired
```

### 2. Creating a Window

```c
ATUI_WINDOW *win = ATUI_NEWWIN(10, 40, 5, 10); // h, w, y, x
win->boxed = TRUE;
ATUI_WBOX(win); // Draw border
ATUI_WSET_TITLE(win, "Demo Window");
```

### 3. Adding Widgets

```c
ATUI_WADD_LABEL(win, 1, 2, "Welcome to ATUI!");
ATUI_WIDGET *btn = ATUI_WADD_BUTTON(win, 3, 2, "OK", on_ok_press);
ATUI_WIDGET *cb  = ATUI_WADD_CHECKBOX(win, 5, 2, "Enable feature", FALSE, on_cb_change);
ATUI_WIDGET *tb  = ATUI_WADD_TEXTBOX(win, 7, 2, 20, "Type here...", on_tb_change);
```

### 4. Handling Input

```c
while (1) {
    PS2_KB_DATA *kp = ATUI_GETCH();
    if (!kp) continue;
    U32 code = kp->cur.keycode;
    ATUI_WINDOW_HANDLE_KEY(win, code); // Handles Tab, Enter, arrows, etc
    ATUI_WREFRESH(win);                // Redraw window and widgets
}
```

### 5. Forms (Tab Order & Submit)

```c
ATUI_FORM *form = ATUI_FORM_CREATE(win, on_form_submit);
ATUI_FORM_ADD_FIELD(form, tb);
ATUI_FORM_ADD_FIELD(form, btn);
// ...
while (!form->submitted) {
    PS2_KB_DATA *kp = ATUI_GETCH();
    if (!kp) continue;
    ATUI_FORM_HANDLE_KEY(form, kp->cur.keycode, kp->mods.shift);
    ATUI_WREFRESH(win);
}
ATUI_FORM_DESTROY(form);
```

### 6. Destroying

```c
ATUI_DELWIN(win);
ATUI_DESTROY();
```

---

## API Overview

### Initialization & Shutdown
- `BOOL ATUI_INITIALIZE(PU8 font, ATUI_CONF conf);`
- `VOID ATUI_DESTROY();`
- `VOID ATUI_REFRESH();`

### Screen Control
- `VOID ATUI_CLEAR();`, `VOID ATUI_CLRTOEOL();`, `VOID ATUI_CLRTOBOT();`

### Cursor
- `VOID ATUI_MOVE(U32 y, U32 x);`, `VOID ATUI_GETYX(U32 *y, U32 *x);`, `VOID ATUI_CURSOR_SET(BOOL visible);`

### Output
- `VOID ATUI_ADDCH(CHAR c);`, `VOID ATUI_ADDSTR(CHAR *str);`, `VOID ATUI_PRINTW(CHAR *fmt, ...);`
- `VOID ATUI_MVPRINTW(U32 y, U32 x, CHAR *fmt, ...);`, `VOID ATUI_MVADDSTR(U32 y, U32 x, CHAR *str);`

### Colors
- `VOID ATUI_SET_FG(VBE_COLOUR color);`, `VOID ATUI_SET_BG(VBE_COLOUR color);`, `VOID ATUI_SET_COLOR(VBE_COLOUR fg, VBE_COLOUR bg);`

### Input
- `PS2_KB_DATA* ATUI_GETCH();` (blocking)
- `PS2_KB_DATA* ATUI_GETCH_NB();` (non-blocking)
- `VOID ATUI_ECHO(BOOL enable);`, `VOID ATUI_RAW(BOOL enable);`

### Window System
- `ATUI_WINDOW* ATUI_NEWWIN(U32 h, U32 w, U32 y, U32 x);`
- `VOID ATUI_DELWIN(ATUI_WINDOW* win);`
- `VOID ATUI_WREFRESH(ATUI_WINDOW* win);`
- `VOID ATUI_WCLEAR(ATUI_WINDOW* win);`
- `VOID ATUI_WBOX(ATUI_WINDOW* win);`
- `VOID ATUI_WSET_TITLE(ATUI_WINDOW* win, const CHAR *title);`
- `VOID ATUI_WMOVE(ATUI_WINDOW* win, U32 y, U32 x);`
- `VOID ATUI_WADDCH(ATUI_WINDOW* win, CHAR c);`
- `VOID ATUI_WADDSTR(ATUI_WINDOW* win, const CHAR *str);`
- `VOID ATUI_WPRINTW(ATUI_WINDOW* win, const CHAR *fmt, ...);`

### Widgets
- `ATUI_WIDGET* ATUI_WADD_BUTTON(...)`
- `ATUI_WIDGET* ATUI_WADD_LABEL(...)`
- `ATUI_WIDGET* ATUI_WADD_TEXTBOX(...)`
- `ATUI_WIDGET* ATUI_WADD_PASSWORD(...)`
- `ATUI_WIDGET* ATUI_WADD_CHECKBOX(...)`
- `ATUI_WIDGET* ATUI_WADD_RADIO(...)`
- `ATUI_WIDGET* ATUI_WADD_DROPDOWN(...)`
- `VOID ATUI_DROPDOWN_ADD_ITEM(...)`
- `ATUI_WIDGET* ATUI_WADD_PROGRESSBAR(...)`
- `VOID ATUI_PROGRESSBAR_SET(...)`
- `ATUI_WIDGET* ATUI_WADD_SEPARATOR(...)`

### Widget Focus & State
- `VOID ATUI_WSET_FOCUS(win, idx);`, `ATUI_WIDGET* ATUI_WGET_FOCUS(win);`
- `VOID ATUI_WNEXT_FOCUS(win);`, `VOID ATUI_WPREV_FOCUS(win);`
- `CHAR* ATUI_TEXTBOX_GET_TEXT(w);`, `BOOL ATUI_CHECKBOX_GET(w);`
- `U32 ATUI_DROPDOWN_GET_SELECTED(w);`, `CHAR* ATUI_DROPDOWN_GET_TEXT(w);`
- `U32 ATUI_RADIO_GET_SELECTED(win, group_id);`
- `VOID ATUI_WIDGET_SET_VISIBLE(w, BOOL);`, `VOID ATUI_WIDGET_SET_ENABLED(w, BOOL);`
- `VOID ATUI_WIDGET_SET_TEXT(w, const CHAR *);`

### Forms
- `ATUI_FORM* ATUI_FORM_CREATE(win, on_submit);`
- `VOID ATUI_FORM_ADD_FIELD(form, w);`
- `VOID ATUI_FORM_HANDLE_KEY(form, key, shift);`
- `VOID ATUI_FORM_DESTROY(form);`

### Attributes (ncurses-style)
- `VOID ATUI_ATTRON(U8 attrs);` — OR attrs into display state
- `VOID ATUI_ATTROFF(U8 attrs);` — clear attrs from display state
- `VOID ATUI_ATTRSET(U8 attrs);` — replace display attribute state
- `U8 ATUI_ATTRGET();` — query current attrs
- `VOID ATUI_WATTRON(win, U8 attrs);` — per-window attron
- `VOID ATUI_WATTROFF(win, U8 attrs);`
- `VOID ATUI_WATTRSET(win, U8 attrs);`
- `U8 ATUI_WATTRGET(win);`

Available attributes: `ATUI_A_BOLD`, `ATUI_A_UNDERLINE`, `ATUI_A_REVERSE`, `ATUI_A_DIM`, `ATUI_A_BLINK`, `ATUI_A_STANDOUT`

### Background Character
- `VOID ATUI_BKGD(CHAR_CELL cell);` — set display background cell
- `CHAR_CELL ATUI_GETBKGD();`
- `VOID ATUI_WBKGD(win, CHAR_CELL cell);` — set window background cell
- `CHAR_CELL ATUI_WGETBKGD(win);`

### Read-back (inch/instr)
- `CHAR_CELL ATUI_INCH();` — get cell at cursor
- `CHAR_CELL ATUI_MVINCH(y, x);`
- `CHAR_CELL ATUI_WINCH(win);` — get cell at window cursor
- `CHAR_CELL ATUI_MVWINCH(win, y, x);`
- `U32 ATUI_INSTR(buf, maxlen);` — read row into buffer
- `U32 ATUI_MVINSTR(y, x, buf, maxlen);`
- `U32 ATUI_WINNSTR(win, buf, maxlen);`

### Touch / Dirty Tracking
- `VOID ATUI_TOUCHSCREEN();` — force full redraw
- `VOID ATUI_UNTOUCHSCREEN();`
- `VOID ATUI_TOUCHWIN(win);`
- `VOID ATUI_UNTOUCHWIN(win);`
- `BOOL ATUI_IS_WINTOUCHED(win);`

### Cursor Control
- `VOID ATUI_CURS_SET(U32 visibility);` — 0=invisible, 1=underline, 2=block

### Input Extensions
- `VOID ATUI_TIMEOUT(I32 ms);` — set getch timeout (<0=block, 0=nb, >0=wait ms)
- `U32 ATUI_GETSTR(buf, maxlen);` — line input at cursor
- `U32 ATUI_WGETSTR(win, buf, maxlen);` — line input in window
- `CHAR* ATUI_KEYNAME(U32 keycode);` — human-readable key name

### Insert/Delete Lines
- `VOID ATUI_INSDELLN(I32 n);` — insert (n>0) or delete (n<0) lines at cursor
- `VOID ATUI_WINSDELLN(win, I32 n);`

### Window Clear Extensions
- `VOID ATUI_WCLRTOEOL(win);`
- `VOID ATUI_WCLRTOBOT(win);`

### Scroll Control
- `VOID ATUI_SCROLLOK(win, BOOL enable);` — enable/disable auto-scroll
- `VOID ATUI_WSETSCRREG(win, top, bot);` — set scrolling region

### Window Resize
- `VOID ATUI_WRESIZE(win, new_h, new_w);`

### Subwindows
- `ATUI_WINDOW* ATUI_SUBWIN(parent, h, w, y, x);` — absolute coordinates
- `ATUI_WINDOW* ATUI_DERWIN(parent, h, w, y, x);` — relative to parent interior
- `VOID ATUI_MVDERWIN(win, y, x);` — reposition derived window

### Copy Operations
- `VOID ATUI_OVERLAY(src, dst);` — copy non-blank cells
- `VOID ATUI_OVERWRITE(src, dst);` — copy all cells
- `VOID ATUI_COPYWIN(src, dst, sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol, overlay);`

### Panel System (`#include <LIBRARIES/ATUI/ATUI_PANEL.h>`)
- `ATUI_PANEL* ATUI_NEW_PANEL(ATUI_WINDOW *win);` — create panel from window
- `VOID ATUI_DEL_PANEL(ATUI_PANEL *pan);`
- `VOID ATUI_SHOW_PANEL(ATUI_PANEL *pan);`
- `VOID ATUI_HIDE_PANEL(ATUI_PANEL *pan);`
- `VOID ATUI_TOP_PANEL(ATUI_PANEL *pan);` — raise to top
- `VOID ATUI_BOTTOM_PANEL(ATUI_PANEL *pan);` — lower to bottom
- `ATUI_WINDOW* ATUI_PANEL_WINDOW(ATUI_PANEL *pan);`
- `ATUI_PANEL* ATUI_PANEL_ABOVE(ATUI_PANEL *pan);`
- `ATUI_PANEL* ATUI_PANEL_BELOW(ATUI_PANEL *pan);`
- `VOID ATUI_REPLACE_PANEL(ATUI_PANEL *pan, ATUI_WINDOW *win);`
- `VOID ATUI_MOVE_PANEL(ATUI_PANEL *pan, U32 y, U32 x);`
- `VOID ATUI_UPDATE_PANELS();` — composite all panels to display

### Menu System (`#include <LIBRARIES/ATUI/ATUI_MENU.h>`)
- `ATUI_MENU* ATUI_NEW_MENU(ATUI_MENU_ITEM *items, U32 count);`
- `VOID ATUI_FREE_MENU(ATUI_MENU *menu);`
- `VOID ATUI_SET_MENU_WIN(menu, win);`
- `VOID ATUI_SET_MENU_SUB(menu, sub);`
- `VOID ATUI_SET_MENU_FORMAT(menu, rows, cols);`
- `VOID ATUI_SET_MENU_MARK(menu, mark);` / `ATUI_SET_MENU_UNMARK(menu, ch);`
- `VOID ATUI_SET_MENU_MULTI(menu, BOOL);`
- `VOID ATUI_SET_MENU_COLORS(menu, fg, bg, sel_fg, sel_bg);`
- `VOID ATUI_POST_MENU(menu);` / `VOID ATUI_UNPOST_MENU(menu);`
- `I32 ATUI_MENU_DRIVER(menu, request);` — handles UP/DOWN/LEFT/RIGHT/FIRST/LAST/SCROLL/TOGGLE/CLEAR/SELECT
- `ATUI_MENU_ITEM* ATUI_CURRENT_ITEM(menu);`
- `U32 ATUI_MENU_CURRENT_INDEX(menu);`
- `ATUI_MENU_ITEM* ATUI_MENU_ITEM_AT(menu, index);`

### ACS Line-Drawing Characters
Use `ATUI_ACS_*` macros instead of raw codepoints for portability:
`ATUI_ACS_HLINE`, `ATUI_ACS_VLINE`, `ATUI_ACS_ULCORNER`, `ATUI_ACS_URCORNER`, `ATUI_ACS_LLCORNER`, `ATUI_ACS_LRCORNER`, `ATUI_ACS_TTEE`, `ATUI_ACS_BTEE`, `ATUI_ACS_LTEE`, `ATUI_ACS_RTEE`, `ATUI_ACS_PLUS`, `ATUI_ACS_DIAMOND`, `ATUI_ACS_CKBOARD`, `ATUI_ACS_BULLET`, `ATUI_ACS_DARROW`, `ATUI_ACS_UARROW`

---

## Extended ASCII/Box Drawing

ATUI supports all CP437/extended ASCII box-drawing and shading characters. Use these for borders, lines, and UI effects:

- Single: `─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼`
- Double: `═ ║ ╔ ╗ ╚ ╝ ╠ ╣ ╦ ╩ ╬`
- Shade: `░ ▒ ▓ █ ▄ ▀`
- Arrows: `► ◄ ▲ ▼`
- Checkmark: `√`
- Bullet: `•`
- Diamond: `♦`

Use the provided defines (see `ATUI_TYPES.h`) for portable code:

```c
ATUI_WADDCH(win, ATUI_BOX_H);
ATUI_WADDCH(win, ATUI_BOX_V);
ATUI_WADDCH(win, ATUI_BOX_TL);
// ...
```

---

## Example: Yes/No Message Box

```c
BOOL atui_yesno_box(const CHAR *question) {
    U32 win_w = 32, win_h = 7;
    U32 win_x = (GET_DISPLAY()->cols - win_w) / 2;
    U32 win_y = (GET_DISPLAY()->rows - win_h) / 2;
    ATUI_WINDOW *win = ATUI_NEWWIN(win_h, win_w, win_y, win_x);
    ATUI_WBOX(win);
    ATUI_WSET_TITLE(win, "Confirm");
    ATUI_WADD_LABEL(win, 2, 3, question);
    ATUI_WADD_LABEL(win, 4, 3, "[Enter] Yes   [Backspace] No");
    ATUI_WREFRESH(win);
    while (1) {
        PS2_KB_DATA *kp = ATUI_GETCH();
        if (!kp) continue;
        U32 code = kp->cur.keycode;
        if (code == KEY_ENTER) { ATUI_DELWIN(win); return TRUE; }
        if (code == KEY_BACKSPACE || code == 8) { ATUI_DELWIN(win); return FALSE; }
        ATUI_WREFRESH(win);
    }
}
```

---

## Example: Attributes

```c
ATUI_ATTRON(ATUI_A_BOLD);
ATUI_ADDSTR("Bold text ");
ATUI_ATTROFF(ATUI_A_BOLD);

ATUI_ATTRON(ATUI_A_REVERSE);
ATUI_ADDSTR(" Highlighted ");
ATUI_ATTROFF(ATUI_A_REVERSE);

ATUI_ATTRON(ATUI_A_UNDERLINE | ATUI_A_DIM);
ATUI_ADDSTR("Dim underlined");
ATUI_ATTRSET(ATUI_A_NORMAL);
ATUI_REFRESH();
```

---

## Example: Panels

```c
#include <LIBRARIES/ATUI/ATUI_PANEL.h>

ATUI_WINDOW *w1 = ATUI_NEWWIN(10, 30, 2, 2);
ATUI_WINDOW *w2 = ATUI_NEWWIN(10, 30, 5, 10);
ATUI_WBOX(w1); ATUI_WBOX(w2);
ATUI_WSET_TITLE(w1, "Panel 1");
ATUI_WSET_TITLE(w2, "Panel 2");
ATUI_WADDSTR(w1, "Background panel");
ATUI_WADDSTR(w2, "Foreground panel");

ATUI_PANEL *p1 = ATUI_NEW_PANEL(w1);
ATUI_PANEL *p2 = ATUI_NEW_PANEL(w2);
ATUI_UPDATE_PANELS(); // composites both, p2 on top

// Raise p1 to top:
ATUI_TOP_PANEL(p1);
ATUI_UPDATE_PANELS();

// Cleanup
ATUI_DEL_PANEL(p2);
ATUI_DEL_PANEL(p1);
ATUI_DELWIN(w2);
ATUI_DELWIN(w1);
```

---

## Example: Menu

```c
#include <LIBRARIES/ATUI/ATUI_MENU.h>

ATUI_MENU_ITEM items[3];
ATUI_MENU_ITEM_INIT(items[0], "Open", "Open a file");
ATUI_MENU_ITEM_INIT(items[1], "Save", "Save current file");
ATUI_MENU_ITEM_INIT(items[2], "Quit", "Exit the program");

ATUI_MENU *menu = ATUI_NEW_MENU(items, 3);
ATUI_WINDOW *menu_win = ATUI_NEWWIN(8, 20, 3, 5);
ATUI_WBOX(menu_win);
ATUI_WINDOW *menu_sub = ATUI_DERWIN(menu_win, 6, 18, 1, 1);
ATUI_SET_MENU_WIN(menu, menu_win);
ATUI_SET_MENU_SUB(menu, menu_sub);
ATUI_POST_MENU(menu);
ATUI_WREFRESH(menu_win);

while (1) {
    PS2_KB_DATA *kp = ATUI_GETCH();
    if (!kp) continue;
    U32 code = kp->cur.keycode;
    if (code == KEY_DOWN)  ATUI_MENU_DRIVER(menu, ATUI_REQ_DOWN);
    if (code == KEY_UP)    ATUI_MENU_DRIVER(menu, ATUI_REQ_UP);
    if (code == KEY_ENTER) {
        ATUI_MENU_DRIVER(menu, ATUI_REQ_SELECT);
        break;
    }
    ATUI_WREFRESH(menu_win);
}

ATUI_MENU_ITEM *sel = ATUI_CURRENT_ITEM(menu);
// sel->text contains the chosen item
ATUI_UNPOST_MENU(menu);
ATUI_FREE_MENU(menu);
ATUI_DELWIN(menu_sub);
ATUI_DELWIN(menu_win);
```

---

## Example: Subwindows

```c
ATUI_WINDOW *parent = ATUI_NEWWIN(20, 60, 2, 5);
ATUI_WBOX(parent);

// Derived window: coords relative to parent interior
ATUI_WINDOW *sub = ATUI_DERWIN(parent, 5, 20, 2, 3);
ATUI_WATTRON(sub, ATUI_A_BOLD);
ATUI_WADDSTR(sub, "I'm inside the parent!");
ATUI_WATTROFF(sub, ATUI_A_BOLD);

ATUI_WREFRESH(sub);
ATUI_WREFRESH(parent);

ATUI_DELWIN(sub);    // syncs content back to parent
ATUI_DELWIN(parent);
```

---

## Tips

- Always call `ATUI_WREFRESH(win)` after input to redraw widgets.
- Use `ATUI_SET_COLOR` and `ATUI_WSET_COLOR` for custom color schemes.
- Use `ATUI_ATTRON`/`ATUI_ATTROFF` for bold, underline, reverse, dim effects.
- Use panels for overlapping windows — call `ATUI_UPDATE_PANELS()` to composite.
- Use forms for multi-field dialogs and tab-order.
- Use `ATUI_ACS_*` macros for portable box-drawing characters.
- Use `ATUI_TIMEOUT()` to control blocking behavior of `ATUI_GETCH()`.
- Use extended ASCII for beautiful retro UIs!

---

## See Also
- `ATUI.h` and `ATUI_TYPES.h` for full API
- Example: `PONG.c` in SYS_PROGS
- atOS documentation

---

## Mouse Support

ATUI supports PS/2 mouse input for clickable widgets and pointer-based navigation. Mouse events are mapped to widget actions (button press, checkbox toggle, dropdown open/select, textbox cursor placement, etc).

### Enabling Mouse

- Enable or disable mouse support at runtime:

```c
ATUI_MOUSE_ENABLE(TRUE); // Enable mouse
ATUI_MOUSE_ENABLE(FALSE); // Disable mouse
```

- Poll mouse state (updates internal state, returns pointer or NULLPTR):

```c
ATUI_MOUSE *ms = ATUI_MOUSE_POLL();
if (ms && ms->clicked) {
    // Handle click event
}
```

- Get current mouse state without polling:

```c
ATUI_MOUSE *ms = ATUI_MOUSE_GET();
```

- Handle mouse input for a window (hit-tests widgets, triggers actions):

```c
ATUI_WIDGET *w = ATUI_WINDOW_HANDLE_MOUSE(win, ms);
if (w) {
    // Widget was clicked or activated
}
```

#### ATUI_MOUSE struct fields
- `cell_col`, `cell_row`: Mouse position in character cells
- `px_x`, `px_y`: Mouse position in pixels
- `btn1`, `btn2`, `btn3`: Button states (left, right, middle)
- `clicked`, `rclicked`: Edge-triggered click events
- `dragging`: True if left button is held and mouse is moving

---

## Configuration File (ATRC)

ATUI supports persistent configuration via ATRC-formatted `.CNF` files. These control colors, cursor, echo, insert mode, mouse, and border style.

### Example ATUI.CNF

```
#!ATRC
# This is a default fallback ATUI.CNF file

[ATUI]
FONT=/HOME/FONTS/SYS_FONT.FNT
FG=WHITE
BG=BLACK
CURSOR=ON
ECHO=ON
INSERT=ON
MOUSE=OFF
BORDER=SINGLE
```

### Supported Keys
- `FONT`: Path to font file
- `FG`: Foreground color (name or hex)
- `BG`: Background color (name or hex)
- `CURSOR`: ON/OFF
- `ECHO`: ON/OFF
- `INSERT`: ON/OFF
- `MOUSE`: ON/OFF
- `BORDER`: SINGLE/DOUBLE/NONE

### Loading and Saving Config

```c
ATUI_CONF_LOAD("/HOME/ATUI.CNF"); // Load config
ATUI_CONF_SAVE("/HOME/ATUI.CNF"); // Save current state
```

- Get/set config values by key:

```c
CHAR *fg = ATUI_CONF_GET("ATUI", "FG");
ATUI_CONF_SET("ATUI", "FG", "CYAN");
```