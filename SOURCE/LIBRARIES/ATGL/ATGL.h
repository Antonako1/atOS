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
/*              USER-DEFINED CALLBACKS (RUNTIME_ATGL)               */
/* ================================================================ */

U32  ATGL_MAIN(U32 argc, PPU8 argv);
VOID ATGL_EVENT_LOOP(ATGL_EVENT *ev);
VOID ATGL_GRAPHICS_LOOP(U32 ticks);

#endif /* ATGL_H */
