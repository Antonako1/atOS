/*+++
    LIBRARIES/ATGL/ATGL_INTERNAL.h - Internal shared state for ATGL

    Part of atOS — NOT for public use.
---*/
#ifndef ATGL_INTERNAL_H
#define ATGL_INTERNAL_H

#include <LIBRARIES/ATGL/ATGL.h>

#ifndef ATGL_SCREEN_WIDTH
#define ATGL_SCREEN_WIDTH  1024
#endif

#ifndef ATGL_SCREEN_HEIGHT
#define ATGL_SCREEN_HEIGHT 768
#endif

/* Mouse cursor dimensions */
#define ATGL_CURSOR_WIDTH  8
#define ATGL_CURSOR_HEIGHT 12

/* Mouse cursor save/restore state.
   Stores the bounding rect, a buffer of the framebuffer pixels
   that lived under the cursor before it was drawn, and a cached
   pointer to the process framebuffer for direct read/write. */
typedef struct {
    U32  x, y;                         /* Position of last drawn cursor   */
    U32  prev_x, prev_y;               /* Previous position for hide      */
    U32  width, height;                /* Cursor bounding-rect size       */
    VOID *previous_buffer;             /* Saved pixels under cursor       */
    VBE_PIXEL_COLOUR *framebuffer;     /* Process framebuffer (from TCB)  */
    U32  stride;                       /* Pixels per scanline (= width)   */
    BOOL has_saved;                    /* TRUE if previous_buffer valid   */
} ATGL_CURSOR;

/* Global library state, defined in ATGL.c */
typedef struct {
    PATGL_NODE root;
    U32 width;
    U32 height;
    ATGL_THEME theme;
    PATGL_NODE focus;
    BOOL initialized;
    BOOL quit;
    U32  next_id;
    ATGL_SCREEN_ATTRIBS attrs;
    U32 bpp; // bytes per pixel

    /* Event polling state */
    U32  last_kb_seq;
    U32  last_ms_seq;
    I32  last_mouse_x;
    I32  last_mouse_y;
    BOOL last_btn_left;
    BOOL last_btn_right;
    BOOL last_btn_middle;

    /* Clipboard */
    CHAR clipboard[ATGL_TEXTINPUT_MAX];

    /* Mouse cursor */
    ATGL_CURSOR cursor;
} ATGL_STATE;

typedef struct _ATGL_IMAGE_HEADER {
    U8 signature[4]; // "ATGI"
    U32 version;    // 1
    U32 flags;      // reserved for future use
    U32 data_offset; // offset to pixel data from start of file
    U32 data_size;   // size of pixel data in bytes
    U32 width;
    U32 height;
    U32 bpp; // bits per pixel (e.g. 24 for RGB, 32 for RGBA)
} ATGL_IMAGE_HEADER;

extern ATGL_STATE atgl;

/* Internal helpers (implemented in ATGL_NODE.c) */
VOID       atgl_compute_abs_rect(PATGL_NODE node, I32 parent_x, I32 parent_y);
PATGL_NODE atgl_hit_test_recursive(PATGL_NODE node, I32 x, I32 y);
VOID       atgl_collect_focusable(PATGL_NODE node,
                                  PATGL_NODE *out, U32 *count, U32 max);

#endif /* ATGL_INTERNAL_H */
