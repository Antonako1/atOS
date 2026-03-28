/**
 * atOS paint program - simple paint application demonstrating ATGL
 * Part of atOS
 * Licensed under the MIT License. See LICENSE file in the project root for full license information.
 * This program demonstrates the use of ATGL for drawing operations by implementing a simple paint application.
 * It allows the user to draw on the screen using the mouse, with a basic color palette and brush size options.
 * The program also demonstrates handling mouse input and rendering graphics using ATGL.
 * Authors: Antonako1
 * Revision History:
 * 2025/03/27 - Antonako1
 *     Initial version.
 * Remarks:
 *     This program is intended as a demonstration of ATGL's capabilities for drawing and handling input.
 *     It is not a full-featured paint application, but serves as a starting point for developers looking to create graphical applications using ATGL.
 */

#include <LIBRARIES/ATGL/ATGL.h>
#include <LIBRARIES/ATRC/ATRC.h>
#include <STD/PROC_COM.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/DEBUG.h>

PATGL_NODE header_label; // Displays the title and status messages
PATGL_NODE canvas; // The drawing area where the user can paint

PATGL_NODE open_btn; // opens file dialog to load an image into the canvas
PATGL_NODE save_btn; // opens file dialog to load an image into the canvas
PATGL_NODE clear_btn; // clears the canvas when clicked
PATGL_NODE pencil_btn; // enable pencil mode when selected, disable eraser mode
PATGL_NODE eraser_btn; // enable eraser mode when selected, disable pencil mode

// RGB sliders for color selection
PATGL_NODE red_slider;
PATGL_NODE green_slider;
PATGL_NODE blue_slider; 

PATGL_NODE color_preview; // Shows the currently selected color

PATGL_NODE primary_color_btn; // ms1
PATGL_NODE secondary_color_btn; // ms2

PATGL_NODE brush_size_slider; // 1-50

PATGL_NODE zoom_in_btn;
PATGL_NODE zoom_out_btn;
PATGL_NODE zoom_label;
PATGL_NODE grid_btn;

typedef struct _PAINT_APP_STATE {
    VBE_COLOUR primary_color;
    VBE_COLOUR secondary_color;
    U32 brush_size;
    BOOL pencil_mode;
    BOOL eraser_mode;

    U32 header_height;

    BOOLEAN primary_color_selected; // TRUE if primary color is selected, FALSE if secondary color is selected

    ATGL_RECT canvas_rect;

    /* Pan state */
    BOOL panning;
    I32  pan_last_x;
    I32  pan_last_y;
    
} PAINT_APP_STATE;

static PAINT_APP_STATE app_state ATTRIB_DATA;

/* ================================================================ */
/*                        HELPERS                                   */
/* ================================================================ */

static VBE_COLOUR get_active_color(VOID)
{
    return app_state.primary_color_selected
           ? app_state.primary_color
           : app_state.secondary_color;
}

static VOID update_color_preview(VOID)
{
    VBE_COLOUR c = get_active_color();
    ATGL_NODE_SET_COLORS(color_preview, c, c);
    ATGL_NODE_INVALIDATE(color_preview);
}

static VOID update_zoom_label(VOID)
{
    static CHAR buf[16];
    U32 z = ATGL_IMAGE_GET_ZOOM(canvas);
    /* simple itoa into buf: "100%" */
    U32 i = 0;
    U32 tmp = z;
    CHAR digits[8];
    U32 nd = 0;
    if (tmp == 0) digits[nd++] = '0';
    while (tmp) { digits[nd++] = '0' + (tmp % 10); tmp /= 10; }
    while (nd--) buf[i++] = digits[nd];
    buf[i++] = '%';
    buf[i]   = '\0';
    ATGL_NODE_SET_TEXT(zoom_label, (PU8)buf);
}

/* ================================================================ */
/*                       CALLBACKS                                  */
/* ================================================================ */

static VOID on_clear(PATGL_NODE node)
{
    (void)node;
    ATGL_IMAGE_CLEAR(canvas, RGB(255, 255, 255));
}

static VOID on_pencil(PATGL_NODE node)
{
    (void)node;
    app_state.pencil_mode = TRUE;
    app_state.eraser_mode = FALSE;
}

static VOID on_eraser(PATGL_NODE node)
{
    (void)node;
    app_state.pencil_mode = FALSE;
    app_state.eraser_mode = TRUE;
}

static VOID on_primary(PATGL_NODE node)
{
    (void)node;
    app_state.primary_color_selected = TRUE;
    ATGL_SLIDER_SET_VALUE(red_slider,   (app_state.primary_color >> 16) & 0xFF);
    ATGL_SLIDER_SET_VALUE(green_slider, (app_state.primary_color >> 8)  & 0xFF);
    ATGL_SLIDER_SET_VALUE(blue_slider,   app_state.primary_color        & 0xFF);
    update_color_preview();
}

static VOID on_secondary(PATGL_NODE node)
{
    (void)node;
    app_state.primary_color_selected = FALSE;
    ATGL_SLIDER_SET_VALUE(red_slider,   (app_state.secondary_color >> 16) & 0xFF);
    ATGL_SLIDER_SET_VALUE(green_slider, (app_state.secondary_color >> 8)  & 0xFF);
    ATGL_SLIDER_SET_VALUE(blue_slider,   app_state.secondary_color        & 0xFF);
    update_color_preview();
}

static VOID on_zoom_in(PATGL_NODE node)
{
    (void)node;
    ATGL_IMAGE_ZOOM_IN(canvas);
    update_zoom_label();
}

static VOID on_zoom_out(PATGL_NODE node)
{
    (void)node;
    ATGL_IMAGE_ZOOM_OUT(canvas);
    update_zoom_label();
}

static VOID on_grid_toggle(PATGL_NODE node)
{
    (void)node;
    ATGL_IMAGE_DATA *id = &canvas->data.image;
    ATGL_IMAGE_SHOW_GRID(canvas, !id->show_grid);
}

/* ================================================================ */
/*                     ATGL ENTRY POINT                             */
/* ================================================================ */

CMAIN() {
    DISABLE_SHELL_KEYBOARD();
    ON_EXIT(ENABLE_SHELL_KEYBOARD);
    ON_EXIT(ATGL_QUIT);
    ATGL_INIT();
    ATGL_CREATE_SCREEN(ATGL_SA_NONE);

    app_state.primary_color = RGB(0, 0, 0);
    app_state.secondary_color = RGB(255, 255, 255);
    app_state.brush_size = 5;
    app_state.pencil_mode = TRUE;
    app_state.eraser_mode = FALSE;
    app_state.header_height = 40;
    app_state.primary_color_selected = TRUE;
    app_state.panning = FALSE;

    U32 width = ATGL_GET_SCREEN_WIDTH();
    U32 height = ATGL_GET_SCREEN_HEIGHT();

    PATGL_NODE root = ATGL_GET_SCREEN_ROOT_NODE();

    /* ---- Header toolbar ---- */
    header_label = ATGL_CREATE_PANEL(root, (ATGL_RECT){0, 0, width, app_state.header_height}, ATGL_LAYOUT_HORIZONTAL, 4, 4);
    ATGL_NODE_SET_COLORS(header_label, RGB(255, 255, 255), RGB(0, 0, 128));
    ATGL_CREATE_LABEL(header_label, (ATGL_RECT){0, 0, 0, 0}, (PU8)"PAINT", RGB(255, 255, 0), RGB(0, 0, 128));

    open_btn    = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 52, 24}, (PU8)"Open",    NULL);
    save_btn    = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 52, 24}, (PU8)"Save",    NULL);
    clear_btn   = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 52, 24}, (PU8)"Clear",   on_clear);
    pencil_btn  = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 60, 24}, (PU8)"Pencil",  on_pencil);
    eraser_btn  = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 60, 24}, (PU8)"Eraser",  on_eraser);

    red_slider   = ATGL_CREATE_SLIDER(header_label, (ATGL_RECT){0, 0, 80, 20}, 0, 255, 0, 1);
    green_slider = ATGL_CREATE_SLIDER(header_label, (ATGL_RECT){0, 0, 80, 20}, 0, 255, 0, 1);
    blue_slider  = ATGL_CREATE_SLIDER(header_label, (ATGL_RECT){0, 0, 80, 20}, 0, 255, 0, 1);

    color_preview       = ATGL_CREATE_LABEL(header_label, (ATGL_RECT){0, 0, 24, 20}, NULL, app_state.primary_color, app_state.primary_color);
    primary_color_btn   = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 28, 24}, (PU8)"P1", on_primary);
    secondary_color_btn = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 28, 24}, (PU8)"P2", on_secondary);

    brush_size_slider = ATGL_CREATE_SLIDER(header_label, (ATGL_RECT){0, 0, 60, 20}, 1, 50, 5, 1);

    zoom_out_btn = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 24, 24}, (PU8)"-",  on_zoom_out);
    zoom_label   = ATGL_CREATE_LABEL(header_label,  (ATGL_RECT){0, 0, 40, 20}, (PU8)"100%", RGB(255,255,255), RGB(0,0,128));
    zoom_in_btn  = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 24, 24}, (PU8)"+",  on_zoom_in);
    grid_btn     = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 40, 24}, (PU8)"Grid", on_grid_toggle);

    /* ---- Canvas (blank white image) ---- */
    app_state.canvas_rect = (ATGL_RECT){0, app_state.header_height, width, height - app_state.header_height};
    canvas = ATGL_CREATE_BLANK_IMAGE(root, app_state.canvas_rect, width, height - app_state.header_height, RGB(255, 255, 255));
    
    return 0;
}

/* ================================================================ */
/*                         DRAWING                                  */
/* ================================================================ */

static VOID paint_at_screen(I32 sx, I32 sy, VBE_COLOUR colour)
{
    U32 brush = app_state.brush_size;
    if (brush <= 1) {
        U32 ix, iy;
        if (ATGL_IMAGE_SCREEN_TO_IMG(canvas, sx, sy, &ix, &iy))
            ATGL_IMAGE_SET_PIXEL(canvas, ix, iy, colour);
    } else {
        /* Paint a circle of radius brush/2, converting each point */
        I32 r = (I32)brush / 2;
        for (I32 dy = -r; dy <= r; dy++) {
            for (I32 dx = -r; dx <= r; dx++) {
                if (dx * dx + dy * dy > r * r) continue;
                U32 ix, iy;
                if (ATGL_IMAGE_SCREEN_TO_IMG(canvas, sx + dx, sy + dy, &ix, &iy))
                    ATGL_IMAGE_SET_PIXEL(canvas, ix, iy, colour);
            }
        }
    }
}

/* ================================================================ */
/*                       EVENT LOOP                                 */
/* ================================================================ */

void ATGL_EVENT_LOOP(ATGL_EVENT *ev) {
    if (ev->type == ATGL_EVT_KEY_DOWN) {
        if (ev->key.keycode == KEY_ESC) {
            ATGL_QUIT();
            return;
        }
        /* Keyboard zoom: +/- keys */
        if (ev->key.ch == '+' || ev->key.ch == '=') { on_zoom_in(NULLPTR);  return; }
        if (ev->key.ch == '-')                       { on_zoom_out(NULLPTR); return; }

        /* Arrow-key pan */
        if (ev->key.keycode == KEY_ARROW_UP)    { ATGL_IMAGE_PAN(canvas, 0, -8); return; }
        if (ev->key.keycode == KEY_ARROW_DOWN)  { ATGL_IMAGE_PAN(canvas, 0,  8); return; }
        if (ev->key.keycode == KEY_ARROW_LEFT)  { ATGL_IMAGE_PAN(canvas, -8, 0); return; }
        if (ev->key.keycode == KEY_ARROW_RIGHT) { ATGL_IMAGE_PAN(canvas,  8, 0); return; }
    }

    /* Right-click drag to pan */
    if (ev->type == ATGL_EVT_MOUSE_DOWN && ev->mouse.btn_right) {
        app_state.panning    = TRUE;
        app_state.pan_last_x = ev->mouse.x;
        app_state.pan_last_y = ev->mouse.y;
    }
    if (ev->type == ATGL_EVT_MOUSE_UP && !ev->mouse.btn_right) {
        app_state.panning = FALSE;
    }
    if (ev->type == ATGL_EVT_MOUSE_MOVE && app_state.panning) {
        I32 dx = app_state.pan_last_x - ev->mouse.x;
        I32 dy = app_state.pan_last_y - ev->mouse.y;
        U32 zoom = ATGL_IMAGE_GET_ZOOM(canvas);
        U32 scale = zoom / 100;
        if (scale < 1) scale = 1;
        ATGL_IMAGE_PAN(canvas, dx / (I32)scale, dy / (I32)scale);
        app_state.pan_last_x = ev->mouse.x;
        app_state.pan_last_y = ev->mouse.y;
        return;
    }

    /* Left-click / drag to paint */
    if ((ev->type == ATGL_EVT_MOUSE_DOWN || ev->type == ATGL_EVT_MOUSE_MOVE) && ev->mouse.btn_left) {
        if (ev->mouse.y >= (I32)app_state.header_height) {
            VBE_COLOUR colour;
            if (app_state.eraser_mode)
                colour = RGB(255, 255, 255);
            else
                colour = get_active_color();

            paint_at_screen(ev->mouse.x, ev->mouse.y, colour);
        }
    }

    /* Update colour from sliders every event */
    {
        U32 r = (U32)ATGL_SLIDER_GET_VALUE(red_slider);
        U32 g = (U32)ATGL_SLIDER_GET_VALUE(green_slider);
        U32 b = (U32)ATGL_SLIDER_GET_VALUE(blue_slider);
        VBE_COLOUR c = RGB(r, g, b);

        if (app_state.primary_color_selected)
            app_state.primary_color = c;
        else
            app_state.secondary_color = c;

        update_color_preview();
    }

    app_state.brush_size = (U32)ATGL_SLIDER_GET_VALUE(brush_size_slider);

    ATGL_DISPATCH_EVENT(ATGL_GET_SCREEN_ROOT_NODE(), ev);
}

/* ================================================================ */
/*                     GRAPHICS LOOP                                */
/* ================================================================ */

void ATGL_GRAPHICS_LOOP(U32 ticks) {
    (void)ticks;
    ATGL_RENDER_TREE(ATGL_GET_SCREEN_ROOT_NODE());
}