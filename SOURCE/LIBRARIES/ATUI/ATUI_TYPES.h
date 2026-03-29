#ifndef ATUI_TYPES_H
#define ATUI_TYPES_H

#include <STD/TYPEDEF.h>
#include <DRIVERS/VESA/VBE.h>
#include <PROC/PROC.h>
#include <STD/FS_DISK.h>

/* ================= CELL ATTRIBUTES ================= */

#define ATUI_A_NORMAL    0x00
#define ATUI_A_BOLD      0x01
#define ATUI_A_UNDERLINE 0x02
#define ATUI_A_REVERSE   0x04
#define ATUI_A_DIM       0x08
#define ATUI_A_BLINK     0x10
#define ATUI_A_STANDOUT  (ATUI_A_REVERSE | ATUI_A_BOLD)

/// @brief A character cell
typedef struct _CHAR_CELL {
    VBE_COLOUR fg; 
    VBE_COLOUR bg;
    CHAR c;
    U8 attrs;
} ATTRIB_PACKED CHAR_CELL, *PCHAR_CELL;

/// @brief Configurations for ATUI
typedef enum _ATUI_CONF {
    ATUIC_NONE              = 0x00000000,
    ATUIC_ECHO              = 0x00000001, 
    ATUIC_RAW               = 0x00000002, 
    ATUIC_CURSOR_VISIBLE    = 0x00000004, 
    ATUIC_INSERT_MODE       = 0x00000008, 
    ATUIC_MOUSE_ENABLED     = 0x00000010, 
    ATUIC_DEFAULT = ATUIC_CURSOR_VISIBLE | ATUIC_ECHO | ATUIC_INSERT_MODE,
    ATUIC_DEFAULT_MOUSE = ATUIC_DEFAULT | ATUIC_MOUSE_ENABLED,
} ATUI_CONF;

#define ATUI_MAX_WINDOWS  32
#define ATUI_MAX_WIDGETS  32
#define ATUI_MAX_PANELS   32
#define ATUI_MAX_MENU_ITEMS 64

/* ================= BOX-DRAWING CHARACTERS ================= */
/* Codepage 437 / ASCII 255 box-drawing glyphs               */

#define ATUI_BOX_H          196  /* horizontal line     ─  */
#define ATUI_BOX_V          179  /* vertical   line     │  */
#define ATUI_BOX_TL         218  /* top-left  corner    ┌  */
#define ATUI_BOX_TR         191  /* top-right corner    ┐  */
#define ATUI_BOX_BL         192  /* bottom-left         └  */
#define ATUI_BOX_BR         217  /* bottom-right        ┘  */

#define ATUI_BOX_DH         205  /* double horizontal   ═  */
#define ATUI_BOX_DV         186  /* double vertical     ║  */
#define ATUI_BOX_DTL        201  /* double top-left     ╔  */
#define ATUI_BOX_DTR        187  /* double top-right    ╗  */
#define ATUI_BOX_DBL        200  /* double bottom-left  ╚  */
#define ATUI_BOX_DBR        188  /* double bottom-right ╝  */

#define ATUI_BOX_TEE_L      195  /* T left      ├  */
#define ATUI_BOX_TEE_R      180  /* T right     ┤  */
#define ATUI_BOX_TEE_T      194  /* T top       ┬  */
#define ATUI_BOX_TEE_B      193  /* T bottom    ┴  */
#define ATUI_BOX_CROSS      197  /* cross       ┼  */

#define ATUI_SHADE_LIGHT    176  /* light shade  ░  */
#define ATUI_SHADE_MED      177  /* medium shade ▒  */
#define ATUI_SHADE_DARK     178  /* dark shade   ▓  */
#define ATUI_BLOCK_FULL     219  /* full block   █  */
#define ATUI_BLOCK_HALF_B   220  /* half block   ▄  */
#define ATUI_BLOCK_HALF_T   223  /* upper half   ▀  */

#define ATUI_ARROW_RIGHT    16   /* right arrow  ►  */
#define ATUI_ARROW_LEFT     17   /* left arrow   ◄  */
#define ATUI_ARROW_UP       30   /* up arrow     ▲  */
#define ATUI_ARROW_DOWN     31   /* down arrow   ▼  */

#define ATUI_CHECK_MARK     251  /* checkmark    √  */
#define ATUI_BULLET         7    /* bullet       •  */
#define ATUI_DIAMOND        4    /* diamond      ♦  */

/* ================= ACS (Alternate Char Set) MACROS ================= */
/* ncurses-compatible names mapped to CP437 box-drawing codepoints     */

#define ATUI_ACS_ULCORNER   ATUI_BOX_TL     /* ┌  */
#define ATUI_ACS_URCORNER   ATUI_BOX_TR     /* ┐  */
#define ATUI_ACS_LLCORNER   ATUI_BOX_BL     /* └  */
#define ATUI_ACS_LRCORNER   ATUI_BOX_BR     /* ┘  */
#define ATUI_ACS_HLINE      ATUI_BOX_H      /* ─  */
#define ATUI_ACS_VLINE      ATUI_BOX_V      /* │  */
#define ATUI_ACS_PLUS       ATUI_BOX_CROSS  /* ┼  */
#define ATUI_ACS_LTEE       ATUI_BOX_TEE_L  /* ├  */
#define ATUI_ACS_RTEE       ATUI_BOX_TEE_R  /* ┤  */
#define ATUI_ACS_TTEE       ATUI_BOX_TEE_T  /* ┬  */
#define ATUI_ACS_BTEE       ATUI_BOX_TEE_B  /* ┴  */
#define ATUI_ACS_DIAMOND    ATUI_DIAMOND    /* ♦  */
#define ATUI_ACS_CKBOARD    ATUI_SHADE_MED  /* ▒  */
#define ATUI_ACS_BLOCK      ATUI_BLOCK_FULL /* █  */
#define ATUI_ACS_BULLET     ATUI_BULLET     /* •  */

/* ================= WIDGET TYPES ================= */

typedef enum {
    ATUI_WIDGET_NONE = 0,
    ATUI_WIDGET_BUTTON,
    ATUI_WIDGET_LABEL,
    ATUI_WIDGET_TEXTBOX,
    ATUI_WIDGET_CHECKBOX,
    ATUI_WIDGET_RADIO,
    ATUI_WIDGET_DROPDOWN,
    ATUI_WIDGET_PROGRESSBAR,
    ATUI_WIDGET_SEPARATOR,
} ATUI_WIDGET_TYPE;

/* ================= WIDGET DATA STRUCTURES ================= */

/// @brief Label auto-scroll (marquee) state
typedef struct {
    U32 scroll_offset;     // current marquee position
    U32 last_tick;         // PIT tick of last scroll step
    U32 ticks_per_step;    // PIT ticks between steps (~1 ms each, default 50)
} ATUI_LABEL;

/// @brief Textbox / text-input field state
typedef struct {
    CHAR buffer[128];
    U32 len;
    U32 cursor;
    U32 scroll_offset;     // horizontal scroll for long text
    U32 visible_width;     // visible chars (set to widget w)
    BOOL password_mode;    // mask characters with '*'
    CHAR placeholder[64];  // greyed-out hint text
} ATUI_TEXTBOX;

/// @brief Checkbox state
typedef struct {
    BOOL checked;
} ATUI_CHECKBOX;

/// @brief Radio-button state
typedef struct {
    U32 group_id;          // widgets sharing the same group_id are exclusive
    BOOL selected;
} ATUI_RADIO;

/// @brief Dropdown / combo-box state
typedef struct {
    CHAR items[16][32];    // up to 16 items × 31-char labels
    U32 item_count;
    U32 selected;          // currently selected index
    BOOL open;             // is the list visible?
    U32 scroll_offset;     // top visible item when open
    U32 max_visible;       // max rows to show at once
} ATUI_DROPDOWN;

/// @brief Progress bar state
typedef struct {
    U32 value;             // 0–100
    U32 max;               // maximum value (default 100)
    BOOL show_percent;     // display "xx%" text
} ATUI_PROGRESSBAR;

/* ================= WIDGET ================= */
#define ATUI_WIDGET_MAX_TEXT 128
typedef struct _ATUI_WIDGET {
    ATUI_WIDGET_TYPE type;

    U32 x, y;
    U32 w, h;

    CHAR text[ATUI_WIDGET_MAX_TEXT];

    BOOL focused;
    BOOL visible;
    BOOL enabled;

    VBE_COLOUR fg;
    VBE_COLOUR bg;
    VBE_COLOUR focus_fg;
    VBE_COLOUR focus_bg;

    VOID (*on_press)(struct _ATUI_WIDGET *w);
    VOID (*on_change)(struct _ATUI_WIDGET *w);

    union {
        ATUI_LABEL       label;
        ATUI_TEXTBOX     textbox;
        ATUI_CHECKBOX    checkbox;
        ATUI_RADIO       radio;
        ATUI_DROPDOWN    dropdown;
        ATUI_PROGRESSBAR progress;
    } data;

} ATUI_WIDGET;

/* ================= BORDER STYLE ================= */

typedef enum {
    ATUI_BORDER_NONE   = 0,
    ATUI_BORDER_SINGLE = 1,  // single-line box drawing
    ATUI_BORDER_DOUBLE = 2,  // double-line box drawing
} ATUI_BORDER_STYLE;

/* ================= WINDOW ================= */

typedef struct _ATUI_WINDOW {
    U32 x, y;
    U32 w, h;

    U32 cursor_x;
    U32 cursor_y;

    VBE_COLOUR fg;
    VBE_COLOUR bg;
    U8 current_attrs;       // current attribute state for output

    BOOL boxed;
    BOOL scroll;
    ATUI_BORDER_STYLE border_style;

    CHAR title[64];

    CHAR_CELL *buffer;       // back buffer
    CHAR_CELL *front_buffer; // composited buffer
    BOOL8 *dirty;

    ATUI_WIDGET widgets[ATUI_MAX_WIDGETS];
    U32 widget_count;
    U32 focused_widget;

    BOOL needs_full_redraw;

    // --- subwindow support ---
    struct _ATUI_WINDOW *parent;  // NULL for top-level windows
    BOOL is_subwin;               // TRUE = buffer points into parent's buffer
    BOOL is_derwin;               // TRUE = coords relative to parent interior
    U32 par_y, par_x;             // offset within parent

    // --- scroll region ---
    U32 scroll_top;               // top of scrolling region (0-based interior row)
    U32 scroll_bot;               // bottom of scrolling region (0-based interior row)

    // --- background character ---
    CHAR_CELL bkgd;               // background cell used for clearing

} ATUI_WINDOW;

/* ================= FORM ================= */

#define ATUI_MAX_FORM_FIELDS 16

typedef struct _ATUI_FORM {
    struct _ATUI_WINDOW *win;          // parent window
    U32 field_indices[ATUI_MAX_FORM_FIELDS]; // widget indices in the window
    U32 field_count;
    U32 current_field;                 // currently active field
    BOOL submitted;
    VOID (*on_submit)(struct _ATUI_FORM *form);
} ATUI_FORM;

/* ================= MOUSE STATE ================= */

typedef struct _ATUI_MOUSE {
    U32 cell_col;       // mouse column in character cells
    U32 cell_row;       // mouse row in character cells
    U32 prev_col;
    U32 prev_row;
    U32 px_x;           // raw pixel x
    U32 px_y;           // raw pixel y
    BOOL btn1;          // left button
    BOOL btn2;          // right button
    BOOL btn3;          // middle button
    BOOL clicked;       // left-click event (edge-triggered)
    BOOL rclicked;      // right-click event
    BOOL dragging;      // left held while moving
} ATUI_MOUSE;

/// @brief Main display structure
typedef struct _ATUI_DISPLAY {
    U32 cols;
    U32 rows;
    
    VBE_COLOUR fg; 
    VBE_COLOUR bg;
    U8 current_attrs;        // current attribute state for output
    
    struct {
        U32 col;
        U32 row;
        U32 prev_col;
        U32 prev_row;
    } cursor;
    ATUI_CONF cnf;
    struct {
        FILE *file;
        U32 char_height;
        U32 char_width;
        U32 glyph_cnt;
    } font;
    CHAR_CELL *back_buffer;
    BOOL8 *dirty;
    U32 buffer_sz;
    U32 dirty_sz;

    CHAR_CELL bkgd;          // background cell used for clearing

    I32 input_timeout;       // GETCH timeout: <0=blocking, 0=non-blocking, >0=ms
    
    ATUI_MOUSE mouse;
    
    TCB *t;
} ATUI_DISPLAY, *PATUI_DISPLAY;

#endif // ATUI_TYPES_H
