#ifndef ATGL_ELEMENTS_H
#define ATGL_ELEMENTS_H

#include <STD/TYPEDEF.h>
#include <LIBRARIES/ATGL/ATGL_DEFS.h>
#include <DRIVERS/VESA/VBE.h>

#define ATGL_MAX_CHILDREN 16

// --- Types ---

typedef enum {
    ATGL_NT_GENERIC = 0,
    ATGL_NT_WINDOW,
    ATGL_NT_BUTTON,
    ATGL_NT_TEXT,
    ATGL_NT_AMOUNT,
} ATGL_NODE_TYPE;
typedef enum {
    ATGL_WF_NONE       = 0x00000000,
    ATGL_WF_NO_BORDER  = 0x00000001,
    
    ATGL_WF_NO_RESIZE  = 0x00000002,

    ATGL_WF_NO_CLOSE   = 0x00000004,
    ATGL_WF_NO_MINIMIZE= 0x00000008,
    ATGL_WF_NO_TITLE   = 0x00000010,
    ATGL_WF_NO_MAXIMIZE= 0x00000020,
    ATGL_NO_TOOLBAR    = ATGL_WF_NO_CLOSE | ATGL_WF_NO_MINIMIZE | ATGL_WF_NO_MAXIMIZE | ATGL_WF_NO_TITLE,


} ATGL_WIN_FLAGS;

typedef enum {
    ATGL_TRF_NONE      = 0x00000000,
    ATGL_TRF_CENTER_X  = SCREEN_WIDTH * 10,     // If ATGL_RECT.x is set to this, it centers horizontally
    ATGL_TRF_CENTER_Y  = SCREEN_HEIGHT * 10,    // Same as above but vertically
    
    ATGL_TRF_FIT_WIDTH = SCREEN_WIDTH * 11,     // If ATGL_RECT.width is set to this, it resizes to fit width
    ATGL_TRF_FIT_HEIGHT= SCREEN_HEIGHT * 11,    // Same as above but height

    ATGL_TRF_FIT_HEIGHT_TO_FONT = SCREEN_HEIGHT * 12, // resize height to fit font height
} ATGL_TEXT_RECT_FLAGS;

typedef enum {
    ATGL_EV_MOUSE_MOVE,
    ATGL_EV_MOUSE_DOWN,
    ATGL_EV_KEY_DOWN,
} ATGL_EV_TYPE;

typedef struct _ATGL_EVENT {
    ATGL_EV_TYPE type;
    union {
        struct { I32 x, y; U8 button; } mouse;
        struct { U32 scancode; U16 unicode; } key;
    } data;
} ATGL_EVENT;

typedef struct _ATGL_NODE {
    ATGL_RECT area;
    BOOL is_visible;
    BOOL is_dirty;
    
    struct _ATGL_NODE* parent;
    struct _ATGL_NODE* children[ATGL_MAX_CHILDREN];
    U16 child_count;

    // Polymorphism
    VOID (*on_render)(struct _ATGL_NODE* self, ATGL_RECT clip);
    VOID (*on_event)(struct _ATGL_NODE* self, ATGL_EVENT* event);
    VOID (*on_destroy)(struct _ATGL_NODE* self);

    // Flattened Data - No more nested structs
    VOID* priv_data;
    ATGL_NODE_TYPE type; 
} ATGL_NODE, *PATGL_NODE;


// --- Specific Data Structs ---

typedef struct {
    PU8 title;
    U32 border_color;
    WINHNDL hndl;
    // Add flags here if needed
} ATGL_WIN_DATA;

typedef struct {
    PU8 label;
    U32 bg_color;
    BOOL is_pressed;
} ATGL_BTN_DATA;

typedef struct {
    PU8 text;
    VBE_COLOUR fg_colour;
    VBE_COLOUR bg_colour;
} ATGL_TEXT_DATA;

// --- Constructors (The "Simpler Way") ---

// 1. Generic Node (Base)
PATGL_NODE ATGL_CREATE_NODE(PATGL_NODE parent, ATGL_RECT area);

// 2. Specific Widget Constructors (Handles data & renderers for you)
PATGL_NODE ATGL_CREATE_WINDOW(PATGL_NODE parent, ATGL_RECT area, PU8 title, ATGL_WIN_FLAGS flags);
PATGL_NODE ATGL_CREATE_BUTTON(PATGL_NODE parent, ATGL_RECT area, PU8 label);
PATGL_NODE ATGL_CREATE_TEXT(PATGL_NODE parent, ATGL_RECT area, PU8 text, VBE_COLOUR fg, VBE_COLOUR bg);

// --- Core Utils ---
VOID ATGL_DESTROY_NODE(PATGL_NODE node);
VOID ATGL_MARK_DIRTY(PATGL_NODE node);

#endif // ATGL_ELEMENTS_H