# ATUI — Advanced Text User Interface Library

ATUI is a static, C-based console UI library for atOS, providing windows, widgets, forms, and input handling for text-mode and graphical console applications. It supports extended ASCII (box-drawing, shading, etc) and is designed for both system and user programs.

---

## Features

- **Window system**: Movable, boxed windows with titles, color, and compositing
- **Widgets**: Buttons, labels, textboxes, checkboxes, radio buttons, dropdowns, progress bars, separators
- **Forms**: Tab-order, field navigation, and submit callbacks
- **Input**: Keyboard navigation, focus, raw/echo modes, extended key support
- **ASCII/CP437**: Full support for box-drawing, arrows, checkmarks, and shading
- **Custom colors**: Per-widget and per-window foreground/background
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

## Tips

- Always call `ATUI_WREFRESH(win)` after input to redraw widgets.
- Use `ATUI_SET_COLOR` and `ATUI_WSET_COLOR` for custom color schemes.
- Use forms for multi-field dialogs and tab-order.
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