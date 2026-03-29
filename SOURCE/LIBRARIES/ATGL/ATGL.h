/*+++
    LIBRARIES/ATGL/ATGL.h - atOS Graphics Library

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    GUI library for atOS, similar to Win32 API.
    Provides a node-tree based widget system with theming, layout,
    and event dispatch.

USAGE
    #include <LIBRARIES/ATGL/ATGL.h>

    Programs using RUNTIME_ATGL must define:
        U32  ATGL_MAIN(U32 argc, PPU8 argv);
        VOID ATGL_EVENT_LOOP(ATGL_EVENT *ev);
        VOID ATGL_GRAPHICS_LOOP(U32 ticks);
---*/
#ifndef ATGL_H
#define ATGL_H

#include <LIBRARIES/ATGL/ATGL_TYPES.h>

/* ================================================================ */
/*                      SCREEN MANAGEMENT                           */
/* ================================================================ */

VOID       ATGL_CREATE_SCREEN(ATGL_SCREEN_ATTRIBS attrs);
VOID       ATGL_DESTROY_SCREEN(VOID);
VOID       ATGL_INIT(VOID);
PATGL_NODE ATGL_GET_SCREEN_ROOT_NODE(VOID);
U32        ATGL_GET_SCREEN_WIDTH(VOID);
U32        ATGL_GET_SCREEN_HEIGHT(VOID);

/* ================================================================ */
/*                           QUIT                                   */
/* ================================================================ */

VOID ATGL_QUIT(VOID);
BOOL ATGL_SHOULD_QUIT(VOID);

/* ================================================================ */
/*                        NODE TREE                                 */
/* ================================================================ */

PATGL_NODE ATGL_NODE_CREATE(ATGL_NODE_TYPE type, PATGL_NODE parent,
                            ATGL_RECT rect);
BOOL       ATGL_NODE_ADD_CHILD(PATGL_NODE parent, PATGL_NODE child);
BOOL       ATGL_NODE_REMOVE_CHILD(PATGL_NODE parent, PATGL_NODE child);
VOID       ATGL_NODE_DESTROY(PATGL_NODE node);

VOID       ATGL_NODE_SET_TEXT(PATGL_NODE node, PU8 text);
VOID       ATGL_NODE_SET_VISIBLE(PATGL_NODE node, BOOL visible);
VOID       ATGL_NODE_SET_ENABLED(PATGL_NODE node, BOOL enabled);
VOID       ATGL_NODE_SET_RECT(PATGL_NODE node, ATGL_RECT rect);
VOID       ATGL_NODE_SET_COLORS(PATGL_NODE node, VBE_COLOUR fg,
                                VBE_COLOUR bg);
VOID       ATGL_NODE_INVALIDATE(PATGL_NODE node);
PATGL_NODE ATGL_NODE_FIND_BY_ID(PATGL_NODE root, U32 id);

/* ================================================================ */
/*                      WIDGET CREATION                             */
/* ================================================================ */

PATGL_NODE ATGL_CREATE_LABEL(PATGL_NODE parent, ATGL_RECT rect,
                             PU8 text, VBE_COLOUR fg, VBE_COLOUR bg);

PATGL_NODE ATGL_CREATE_BUTTON(PATGL_NODE parent, ATGL_RECT rect,
                              PU8 text,
                              VOID (*on_click)(PATGL_NODE));

PATGL_NODE ATGL_CREATE_CHECKBOX(PATGL_NODE parent, ATGL_RECT rect,
                                PU8 text, BOOL initial,
                                VOID (*on_change)(PATGL_NODE));

PATGL_NODE ATGL_CREATE_RADIO(PATGL_NODE parent, ATGL_RECT rect,
                             PU8 text, U32 group_id, BOOL selected,
                             VOID (*on_change)(PATGL_NODE));

PATGL_NODE ATGL_CREATE_TEXTINPUT(PATGL_NODE parent, ATGL_RECT rect,
                                 PU8 placeholder, U32 max_len);

PATGL_NODE ATGL_CREATE_SLIDER(PATGL_NODE parent, ATGL_RECT rect,
                              I32 min, I32 max, I32 value, I32 step);

PATGL_NODE ATGL_CREATE_PROGRESSBAR(PATGL_NODE parent, ATGL_RECT rect,
                                   U32 max);

PATGL_NODE ATGL_CREATE_PANEL(PATGL_NODE parent, ATGL_RECT rect,
                             ATGL_LAYOUT_DIR layout,
                             U32 padding, U32 spacing);

PATGL_NODE ATGL_CREATE_LISTBOX(PATGL_NODE parent, ATGL_RECT rect,
                               U32 visible_rows);

PATGL_NODE ATGL_CREATE_SEPARATOR(PATGL_NODE parent, ATGL_RECT rect);

PATGL_NODE ATGL_CREATE_IMAGE(PATGL_NODE parent, ATGL_RECT rect,
                             PU8 pixels, U32 img_w, U32 img_h);

PATGL_NODE ATGL_CREATE_IMAGE_FROM_FILE(PATGL_NODE parent, ATGL_RECT rect,
                             const CHAR *filename);

/// Create an image widget with a freshly allocated blank canvas.
/// The pixel buffer is owned by ATGL and freed on destroy.
PATGL_NODE ATGL_CREATE_BLANK_IMAGE(PATGL_NODE parent, ATGL_RECT rect,
                                   U32 img_w, U32 img_h,
                                   VBE_COLOUR fill);

/* ================================================================ */
/*                   WIDGET GETTERS / SETTERS                       */
/* ================================================================ */

BOOL ATGL_CHECKBOX_GET(PATGL_NODE node);
VOID ATGL_CHECKBOX_SET(PATGL_NODE node, BOOL checked);

U32  ATGL_RADIO_GET_SELECTED(PATGL_NODE parent, U32 group_id);

PU8  ATGL_TEXTINPUT_GET_TEXT(PATGL_NODE node);
VOID ATGL_TEXTINPUT_SET_TEXT(PATGL_NODE node, PU8 text);

I32  ATGL_SLIDER_GET_VALUE(PATGL_NODE node);
VOID ATGL_SLIDER_SET_VALUE(PATGL_NODE node, I32 value);

VOID ATGL_PROGRESSBAR_SET(PATGL_NODE node, U32 value);
U32  ATGL_PROGRESSBAR_GET(PATGL_NODE node);

VOID ATGL_LISTBOX_ADD_ITEM(PATGL_NODE node, PU8 text);
U32  ATGL_LISTBOX_GET_SELECTED(PATGL_NODE node);
PU8  ATGL_LISTBOX_GET_TEXT(PATGL_NODE node);
VOID ATGL_LISTBOX_CLEAR(PATGL_NODE node);

/* ================================================================ */
/*                   IMAGE MANIPULATION                             */
/* ================================================================ */

/// Set a single pixel in the image buffer (image coordinates).
VOID ATGL_IMAGE_SET_PIXEL(PATGL_NODE node, U32 x, U32 y,
                          VBE_COLOUR colour);

/// Get a single pixel from the image buffer (image coordinates).
VBE_COLOUR ATGL_IMAGE_GET_PIXEL(PATGL_NODE node, U32 x, U32 y);

/// Fill the entire image with a solid colour.
VOID ATGL_IMAGE_CLEAR(PATGL_NODE node, VBE_COLOUR colour);

/// Fill a rectangular region inside the image.
VOID ATGL_IMAGE_FILL_RECT(PATGL_NODE node, U32 x, U32 y,
                          U32 w, U32 h, VBE_COLOUR colour);

/// Draw a 1px outline rectangle inside the image.
VOID ATGL_IMAGE_DRAW_RECT(PATGL_NODE node, U32 x, U32 y,
                          U32 w, U32 h, VBE_COLOUR colour);

/// Draw a line inside the image (Bresenham).
VOID ATGL_IMAGE_DRAW_LINE(PATGL_NODE node, I32 x0, I32 y0,
                          I32 x1, I32 y1, VBE_COLOUR colour);

/// Draw a filled circle inside the image.
VOID ATGL_IMAGE_FILL_CIRCLE(PATGL_NODE node, I32 cx, I32 cy,
                            U32 radius, VBE_COLOUR colour);

/// Get the raw pixel buffer.
PU8  ATGL_IMAGE_GET_PIXELS(PATGL_NODE node);

/// Get image dimensions.
VOID ATGL_IMAGE_GET_SIZE(PATGL_NODE node, U32 *w, U32 *h);

/// Replace the pixel buffer (frees old if owned).
VOID ATGL_IMAGE_SET_PIXELS(PATGL_NODE node, PU8 pixels,
                           U32 img_w, U32 img_h);

/* ---- Zoom ---- */

/// Set zoom level (percent, 100 = 1:1). Clamped to [25 .. 3200].
VOID ATGL_IMAGE_SET_ZOOM(PATGL_NODE node, U32 zoom);

/// Get current zoom level.
U32  ATGL_IMAGE_GET_ZOOM(PATGL_NODE node);

/// Zoom in by one step (×2).
VOID ATGL_IMAGE_ZOOM_IN(PATGL_NODE node);

/// Zoom out by one step (÷2).
VOID ATGL_IMAGE_ZOOM_OUT(PATGL_NODE node);

/* ---- Pan / Scroll ---- */

/// Set the pan offset (which image pixel is at top-left of viewport).
VOID ATGL_IMAGE_SET_OFFSET(PATGL_NODE node, I32 ox, I32 oy);

/// Get the current pan offset.
VOID ATGL_IMAGE_GET_OFFSET(PATGL_NODE node, I32 *ox, I32 *oy);

/// Nudge the pan offset by delta pixels.
VOID ATGL_IMAGE_PAN(PATGL_NODE node, I32 dx, I32 dy);

/* ---- Coordinate conversion ---- */

/// Convert a screen coordinate to an image pixel coordinate,
/// accounting for zoom and pan. Returns FALSE if outside image.
BOOL ATGL_IMAGE_SCREEN_TO_IMG(PATGL_NODE node, I32 sx, I32 sy,
                              U32 *ix, U32 *iy);

/* ---- Grid ---- */

/// Enable/disable the pixel grid (visible when zoom >= 400%).
VOID ATGL_IMAGE_SHOW_GRID(PATGL_NODE node, BOOL show);

/// Set the grid line colour.
VOID ATGL_IMAGE_SET_GRID_COLOUR(PATGL_NODE node, VBE_COLOUR colour);

/* ---- Saving ---- */

/// Save the image to a file.
BOOL ATGL_IMAGE_SAVE(PATGL_NODE node, const CHAR *filename);

/* ================================================================ */
/*                       EVENT SYSTEM                               */
/* ================================================================ */

BOOL ATGL_POLL_EVENTS(ATGL_EVENT *ev);
VOID ATGL_DISPATCH_EVENT(PATGL_NODE root, ATGL_EVENT *ev);
VOID ATGL_RENDER_TREE(PATGL_NODE root);

/* ================================================================ */
/*                          FOCUS                                   */
/* ================================================================ */

VOID       ATGL_SET_FOCUS(PATGL_NODE node);
PATGL_NODE ATGL_GET_FOCUS(VOID);
VOID       ATGL_NEXT_FOCUS(VOID);
VOID       ATGL_PREV_FOCUS(VOID);

/* ================================================================ */
/*                          THEME                                   */
/* ================================================================ */

VOID        ATGL_SET_THEME(ATGL_THEME *theme);
ATGL_THEME *ATGL_GET_THEME(VOID);
ATGL_THEME  ATGL_DEFAULT_THEME(VOID);

/* ================================================================ */
/*                          LAYOUT                                  */
/* ================================================================ */

VOID ATGL_LAYOUT_APPLY(PATGL_NODE panel);
VOID ATGL_CENTER_IN_PARENT(PATGL_NODE node);
VOID ATGL_CENTER_IN_SCREEN(PATGL_NODE node);

/* ================================================================ */
/*                     DRAWING HELPERS                              */
/* ================================================================ */

VOID ATGL_DRAW_RAISED(I32 x, I32 y, I32 w, I32 h);
VOID ATGL_DRAW_SUNKEN(I32 x, I32 y, I32 w, I32 h);
VOID ATGL_DRAW_TEXT(I32 x, I32 y, PU8 text, VBE_COLOUR fg,
                    VBE_COLOUR bg);
VOID ATGL_DRAW_TEXT_CLIPPED(I32 x, I32 y, I32 max_w, PU8 text,
                            VBE_COLOUR fg, VBE_COLOUR bg);

/* ================================================================ */
/*                  SHAPES AND PIXELS                               */
/* ================================================================ */
VOID ATGL_DRAW_PIXEL(U32 x, U32 y, VBE_COLOUR colour);
VOID ATGL_DRAW_RECTANGLE(U32 x, U32 y, U32 w, U32 h, VBE_COLOUR colour);
VOID ATGL_DRAW_FILLED_RECTANGLE(U32 x, U32 y, U32 w, U32 h, VBE_COLOUR colour);
VOID ATGL_DRAW_ELLIPSE(U32 x, U32 y, U32 rx, U32 ry, VBE_COLOUR colour);
VOID ATGL_DRAW_FILLED_ELLIPSE(U32 x, U32 y, U32 rx, U32 ry, VBE_COLOUR colour);
VOID ATGL_DRAW_LINE(U32 x1, U32 y1, U32 x2, U32 y2, VBE_COLOUR colour);
VOID ATGL_DRAW_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour);
VOID ATGL_DRAW_FILLED_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour);

/* ================================================================ */
/*              USER-DEFINED CALLBACKS (RUNTIME_ATGL)               */
/* ================================================================ */

U32  ATGL_MAIN(U32 argc, PPU8 argv);
VOID ATGL_EVENT_LOOP(ATGL_EVENT *ev);
VOID ATGL_GRAPHICS_LOOP(U32 ticks);

#endif /* ATGL_H */
