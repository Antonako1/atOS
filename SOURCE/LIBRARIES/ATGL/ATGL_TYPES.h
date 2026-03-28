/*+++
    LIBRARIES/ATGL/ATGL_TYPES.h - atOS Graphics Library type definitions

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    All type definitions for ATGL: rectangles, nodes, events, themes,
    widget data structures, and the polymorphic node tree.
---*/
#ifndef ATGL_TYPES_H
#define ATGL_TYPES_H

#include <STD/TYPEDEF.h>
#include <DRIVERS/VESA/VBE.h>
#include <DRIVERS/PS2/KEYBOARD_MOUSE.h>

/* ================================================================ */
/*                           RECTANGLE                              */
/* ================================================================ */

typedef struct {
    I32 x;
    I32 y;
    I32 w;
    I32 h;
} ATGL_RECT;

static inline BOOL ATGL_RECT_CONTAINS(ATGL_RECT *r, I32 px, I32 py)
{
    return (px >= r->x && px < r->x + r->w &&
            py >= r->y && py < r->y + r->h);
}

/* ================================================================ */
/*                       SCREEN ATTRIBUTES                          */
/* ================================================================ */

typedef enum {
    ATGL_SA_NONE     = 0,
    ATGL_SA_NO_CLEAR = (1 << 0), // Don't clear the screen on initialization (useful for hot-reloading)
} ATGL_SCREEN_ATTRIBS;

/* ================================================================ */
/*                          NODE TYPES                              */
/* ================================================================ */

typedef enum {
    ATGL_NODE_ROOT = 0,
    ATGL_NODE_PANEL,
    ATGL_NODE_LABEL,
    ATGL_NODE_BUTTON,
    ATGL_NODE_CHECKBOX,
    ATGL_NODE_RADIO,
    ATGL_NODE_TEXTINPUT,
    ATGL_NODE_SLIDER,
    ATGL_NODE_PROGRESSBAR,
    ATGL_NODE_LISTBOX,
    ATGL_NODE_SEPARATOR,
    ATGL_NODE_IMAGE,
} ATGL_NODE_TYPE;

/* ================================================================ */
/*                       LAYOUT DIRECTION                           */
/* ================================================================ */

typedef enum {
    ATGL_LAYOUT_NONE = 0,
    ATGL_LAYOUT_VERTICAL,
    ATGL_LAYOUT_HORIZONTAL,
} ATGL_LAYOUT_DIR;

/* ================================================================ */
/*                         EVENT TYPES                              */
/* ================================================================ */

typedef enum {
    ATGL_EVT_NONE = 0,
    ATGL_EVT_KEY_DOWN,
    ATGL_EVT_KEY_UP,
    ATGL_EVT_MOUSE_MOVE,
    ATGL_EVT_MOUSE_DOWN,
    ATGL_EVT_MOUSE_UP,
    ATGL_EVT_MOUSE_CLICK,
    ATGL_EVT_FOCUS_IN,
    ATGL_EVT_FOCUS_OUT,
    ATGL_EVT_TIMER,
} ATGL_EVENT_TYPE;

/* ================================================================ */
/*                            EVENT                                 */
/* ================================================================ */

typedef struct {
    ATGL_EVENT_TYPE type;
    union {
        struct {
            U32  keycode;
            U8   ch;
            BOOL shift;
            BOOL ctrl;
            BOOL alt;
        } key;
        struct {
            I32  x;
            I32  y;
            BOOL btn_left;
            BOOL btn_right;
            BOOL btn_middle;
        } mouse;
        struct {
            U32 ticks;
        } timer;
    };
    BOOL handled;
} ATGL_EVENT;

/* ================================================================ */
/*                            THEME                                 */
/* ================================================================ */

typedef struct {
    /* Background / foreground */
    VBE_COLOUR bg;
    VBE_COLOUR fg;

    /* Widget defaults */
    VBE_COLOUR widget_bg;
    VBE_COLOUR widget_fg;
    VBE_COLOUR widget_border;

    /* 3D borders (Win32yr-style) */
    VBE_COLOUR highlight;       /* Light edge (top-left)    */
    VBE_COLOUR shadow;          /* Dark edge (bottom-right) */
    VBE_COLOUR dark_shadow;     /* Darkest edge             */

    /* Button */
    VBE_COLOUR btn_bg;
    VBE_COLOUR btn_fg;
    VBE_COLOUR btn_hover;
    VBE_COLOUR btn_pressed;
    VBE_COLOUR btn_disabled_bg;
    VBE_COLOUR btn_disabled_fg;

    /* Text input */
    VBE_COLOUR input_bg;
    VBE_COLOUR input_fg;
    VBE_COLOUR input_border;
    VBE_COLOUR input_focus;
    VBE_COLOUR input_placeholder;

    /* Checkbox / Radio */
    VBE_COLOUR check_bg;
    VBE_COLOUR check_fg;
    VBE_COLOUR check_mark;

    /* Slider */
    VBE_COLOUR slider_track;
    VBE_COLOUR slider_fill;
    VBE_COLOUR slider_thumb;

    /* Progress bar */
    VBE_COLOUR progress_bg;
    VBE_COLOUR progress_fill;
    VBE_COLOUR progress_text;

    /* Listbox */
    VBE_COLOUR list_bg;
    VBE_COLOUR list_fg;
    VBE_COLOUR list_sel_bg;
    VBE_COLOUR list_sel_fg;

    /* Focus */
    VBE_COLOUR focus_ring;

    /* Metrics */
    U32 border_width;
    U32 padding;
    U32 char_w;
    U32 char_h;
} ATGL_THEME;

/* ================================================================ */
/*                    WIDGET-SPECIFIC DATA                          */
/* ================================================================ */

typedef struct {
    BOOL pressed;
    BOOL hovered;
} ATGL_BUTTON_DATA;

typedef struct {
    BOOL checked;
} ATGL_CHECKBOX_DATA;

typedef struct {
    U32  group_id;
    BOOL selected;
} ATGL_RADIO_DATA;

#define ATGL_TEXTINPUT_MAX    256
#define ATGL_PLACEHOLDER_MAX  64

typedef struct {
    CHAR buffer[ATGL_TEXTINPUT_MAX];
    U32  len;
    U32  cursor;
    U32  scroll_offset;
    U32  max_len;
    BOOL password;
    CHAR placeholder[ATGL_PLACEHOLDER_MAX];
    U32  selection_start;
    U32  selection_end;
    BOOL is_selecting;
} ATGL_TEXTINPUT_DATA;

typedef struct {
    I32  value;
    I32  min;
    I32  max;
    I32  step;
    BOOL dragging;
} ATGL_SLIDER_DATA;

typedef struct {
    U32  value;
    U32  max;
    BOOL show_text;
} ATGL_PROGRESS_DATA;

#define ATGL_LISTBOX_MAX_ITEMS  64
#define ATGL_LISTBOX_ITEM_LEN   64

typedef struct {
    CHAR items[ATGL_LISTBOX_MAX_ITEMS][ATGL_LISTBOX_ITEM_LEN];
    U32  item_count;
    U32  selected;
    U32  scroll_offset;
    U32  visible_rows;
} ATGL_LISTBOX_DATA;

typedef struct {
    ATGL_LAYOUT_DIR layout;
    U32 padding;
    U32 spacing;
} ATGL_PANEL_DATA;

typedef struct {
    PU8 pixels;     /* VBE_COLOUR array */
    U32 img_w;
    U32 img_h;
} ATGL_IMAGE_DATA;

/* ================================================================ */
/*                          NODE TREE                               */
/* ================================================================ */

#define ATGL_MAX_CHILDREN    32
#define ATGL_NODE_MAX_TEXT  128

typedef struct _ATGL_NODE ATGL_NODE, *PATGL_NODE;

/* Polymorphic function-pointer signatures */
typedef VOID (*ATGL_RENDER_FN) (PATGL_NODE node);
typedef VOID (*ATGL_EVENT_FN)  (PATGL_NODE node, ATGL_EVENT *ev);
typedef VOID (*ATGL_DESTROY_FN)(PATGL_NODE node);

struct _ATGL_NODE {
    ATGL_NODE_TYPE type;
    U32            id;

    ATGL_RECT rect;         /* Position/size relative to parent */
    ATGL_RECT abs_rect;     /* Computed absolute position       */

    BOOL visible;
    BOOL enabled;
    BOOL focused;
    BOOL dirty;
    BOOL focusable;         /* Can receive keyboard focus        */

    VBE_COLOUR fg;
    VBE_COLOUR bg;

    /* Tree relationships */
    PATGL_NODE parent;
    PATGL_NODE children[ATGL_MAX_CHILDREN];
    U32        child_count;

    /* Polymorphic dispatch */
    ATGL_RENDER_FN  fn_render;
    ATGL_EVENT_FN   fn_event;
    ATGL_DESTROY_FN fn_destroy;

    /* User callbacks */
    VOID (*on_click) (PATGL_NODE node);
    VOID (*on_change)(PATGL_NODE node);

    /* Text content */
    CHAR text[ATGL_NODE_MAX_TEXT];

    /* Widget-specific data (discriminated by type) */
    union {
        ATGL_BUTTON_DATA    button;
        ATGL_CHECKBOX_DATA  checkbox;
        ATGL_RADIO_DATA     radio;
        ATGL_TEXTINPUT_DATA textinput;
        ATGL_SLIDER_DATA    slider;
        ATGL_PROGRESS_DATA  progress;
        ATGL_LISTBOX_DATA   listbox;
        ATGL_PANEL_DATA     panel;
        ATGL_IMAGE_DATA     image;
    } data;
};

#endif /* ATGL_TYPES_H */
