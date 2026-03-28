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

    /* Event polling state */
    U32  last_kb_seq;
    U32  last_ms_seq;
    I32  last_mouse_x;
    I32  last_mouse_y;
    BOOL last_btn_left;
    BOOL last_btn_right;
    BOOL last_btn_middle;
} ATGL_STATE;

extern ATGL_STATE atgl;

/* Internal helpers (implemented in ATGL_NODE.c) */
VOID       atgl_compute_abs_rect(PATGL_NODE node, I32 parent_x, I32 parent_y);
PATGL_NODE atgl_hit_test_recursive(PATGL_NODE node, I32 x, I32 y);
VOID       atgl_collect_focusable(PATGL_NODE node,
                                  PATGL_NODE *out, U32 *count, U32 max);

#endif /* ATGL_INTERNAL_H */
