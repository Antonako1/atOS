/*
 * IMG.c — Image Viewer for atOS
 *
 * Toolbar (Paint-style):
 *   [IMG Viewer]  [Open]  [−] [100%] [+]  [Fit]  [1:1]  [Grid]  [Exit]
 *
 * Mouse controls:
 *   Left-drag    — pan
 *   Right-drag   — pan
 *
 * Keyboard:
 *   +/=          — zoom in
 *   −            — zoom out
 *   Arrow keys   — pan 16 px
 *   F            — fit to window
 *   1            — 100 %
 *   G            — toggle grid
 *   ESC          — exit
 */

#include <LIBRARIES/ATGL/ATGL.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/PROC_COM.h>

/* ================================================================ */
/*                        UI NODES                                  */
/* ================================================================ */

static PATGL_NODE toolbar;
static PATGL_NODE title_label;
static PATGL_NODE open_btn;
static PATGL_NODE zoom_out_btn;
static PATGL_NODE zoom_label;
static PATGL_NODE zoom_in_btn;
static PATGL_NODE fit_btn;
static PATGL_NODE actual_btn;
static PATGL_NODE grid_btn;
static PATGL_NODE exit_btn;
static PATGL_NODE status_label;   /* bottom status bar */
static PATGL_NODE canvas;         /* ATGL_NODE_IMAGE — the loaded image */

/* ── Open-file popup ── */
static PATGL_NODE popup_overlay;
static PATGL_NODE popup_panel;
static PATGL_NODE popup_input;
static PATGL_NODE popup_ok_btn;
static PATGL_NODE popup_cancel_btn;

/* ================================================================ */
/*                       APP STATE                                  */
/* ================================================================ */

#define TOOLBAR_H 36
#define STATUS_H  18
#define ZOOM_STEP_IN  200   /* multiply by 2 per step   */
#define ZOOM_STEP_OUT 50    /* divide by 2 per step     */
#define PAN_SPEED     16

typedef struct {
    BOOL  popup_open;
    BOOL  panning;
    I32   pan_last_x;
    I32   pan_last_y;
    CHAR  current_path[256];
} IMG_STATE;

static IMG_STATE g_state ATTRIB_DATA;

/* ================================================================ */
/*                         HELPERS                                  */
/* ================================================================ */

static VOID update_zoom_label(VOID) {
    if (!canvas) return;
    U8 buf[16];
    SPRINTF(buf, "%u%%", ATGL_IMAGE_GET_ZOOM(canvas));
    ATGL_NODE_SET_TEXT(zoom_label, buf);
}

static VOID update_status(VOID) {
    if (!canvas) return;
    U32 w = 0, h = 0;
    ATGL_IMAGE_GET_SIZE(canvas, &w, &h);
    I32 ox = 0, oy = 0;
    ATGL_IMAGE_GET_OFFSET(canvas, &ox, &oy);

    U8 buf[128];
    SPRINTF(buf, "%s  |  %u x %u px  |  offset %d,%d",
            g_state.current_path[0] ? g_state.current_path : "No image",
            w, h, ox, oy);
    ATGL_NODE_SET_TEXT(status_label, (PU8)buf);
}

/* Replace the canvas node with a freshly loaded image. */
static VOID load_image(CONST CHAR *path) {
    U32 sw = ATGL_GET_SCREEN_WIDTH();
    U32 sh = ATGL_GET_SCREEN_HEIGHT();
    ATGL_RECT img_rect = { 0, TOOLBAR_H, (I32)sw, (I32)(sh - TOOLBAR_H - STATUS_H) };

    PATGL_NODE loaded = ATGL_CREATE_IMAGE_FROM_FILE(ATGL_GET_SCREEN_ROOT_NODE(),
                                                     img_rect, path);
    if (!loaded) {
        ATGL_NODE_SET_TEXT(status_label, (PU8)"Error: could not open file.");
        return;
    }

    /* Remove old canvas if present */
    if (canvas)
        ATGL_NODE_DESTROY(canvas);
    canvas = loaded;

    STRNCPY((PU8)g_state.current_path, (CONST PU8)path, 255);
    g_state.current_path[255] = '\0';

    ATGL_IMAGE_SET_ZOOM(canvas, 100);
    ATGL_IMAGE_SET_OFFSET(canvas, 0, 0);
    update_zoom_label();
    update_status();
}

/* Fit image to viewport (keep aspect ratio, zoom in power-of-2 steps). */
static VOID do_fit(VOID) {
    if (!canvas) return;
    U32 iw = 0, ih = 0;
    ATGL_IMAGE_GET_SIZE(canvas, &iw, &ih);
    if (!iw || !ih) return;

    U32 vw = (U32)canvas->rect.w;
    U32 vh = (U32)canvas->rect.h;

    /* Scale so the image fits: zoom = min(vw/iw, vh/ih) * 100 */
    U32 zoom_x = (vw * 100) / iw;
    U32 zoom_y = (vh * 100) / ih;
    U32 zoom   = (zoom_x < zoom_y) ? zoom_x : zoom_y;
    if (zoom < 25) zoom = 25;

    ATGL_IMAGE_SET_ZOOM(canvas, zoom);
    ATGL_IMAGE_SET_OFFSET(canvas, 0, 0);
    update_zoom_label();
    update_status();
}

/* ================================================================ */
/*                      BUTTON CALLBACKS                            */
/* ================================================================ */

static VOID on_zoom_in(PATGL_NODE node)  { (void)node; if (canvas) { ATGL_IMAGE_ZOOM_IN(canvas); update_zoom_label(); update_status(); } }
static VOID on_zoom_out(PATGL_NODE node) { (void)node; if (canvas) { ATGL_IMAGE_ZOOM_OUT(canvas); update_zoom_label(); update_status(); } }
static VOID on_fit(PATGL_NODE node)      { (void)node; do_fit(); }
static VOID on_actual(PATGL_NODE node)   { (void)node; if (canvas) { ATGL_IMAGE_SET_ZOOM(canvas, 100); ATGL_IMAGE_SET_OFFSET(canvas, 0, 0); update_zoom_label(); update_status(); } }
static VOID on_grid(PATGL_NODE node)     { (void)node; if (canvas) ATGL_IMAGE_SHOW_GRID(canvas, !(canvas->data.image.show_grid)); }
static VOID on_exit(PATGL_NODE node)     { (void)node; ATGL_QUIT(); }

/* ── Popup ── */
static VOID hide_popup(VOID) {
    g_state.popup_open = FALSE;
    ATGL_NODE_SET_VISIBLE(popup_overlay, FALSE);
    ATGL_SET_FOCUS(NULLPTR);
    ATGL_NODE_INVALIDATE(popup_overlay);
}

static VOID show_popup(VOID) {
    g_state.popup_open = TRUE;
    ATGL_NODE_SET_VISIBLE(popup_overlay, TRUE);
    ATGL_TEXTINPUT_SET_TEXT(popup_input, (PU8)g_state.current_path);
    ATGL_SET_FOCUS(popup_input);
    ATGL_NODE_INVALIDATE(popup_overlay);
}

static VOID on_open(PATGL_NODE node)  { (void)node; show_popup(); }

static VOID on_popup_cancel(PATGL_NODE node) { (void)node; hide_popup(); }
static VOID on_popup_ok(PATGL_NODE node) {
    (void)node;
    PU8 txt = ATGL_TEXTINPUT_GET_TEXT(popup_input);
    if (txt && txt[0]) load_image((CONST CHAR *)txt);
    hide_popup();
}

/* ================================================================ */
/*                     ATGL ENTRY POINT                             */
/* ================================================================ */

CMAIN() {
    DISABLE_SHELL_KEYBOARD();
    ON_EXIT(ENABLE_SHELL_KEYBOARD);

    ATGL_CREATE_SCREEN(ATGL_SA_NONE);
    ATGL_INIT();
    PU8 def = argv ? argv[1] : "/HOME/TEST.TGI";

    g_state.popup_open  = FALSE;
    g_state.panning     = FALSE;
    g_state.current_path[0] = '\0';
    canvas = NULLPTR;

    U32 sw = ATGL_GET_SCREEN_WIDTH();
    U32 sh = ATGL_GET_SCREEN_HEIGHT();
    PATGL_NODE root = ATGL_GET_SCREEN_ROOT_NODE();

    /* ── Toolbar ─────────────────────────────────────────────── */
    toolbar = ATGL_CREATE_PANEL(root,
                                (ATGL_RECT){0, 0, (I32)sw, TOOLBAR_H},
                                ATGL_LAYOUT_HORIZONTAL, 4, 3);
    ATGL_NODE_SET_COLORS(toolbar, RGB(255,255,255), RGB(0,0,128));

    title_label  = ATGL_CREATE_LABEL(toolbar, (ATGL_RECT){0,0,0,0},
                                     (PU8)"IMG Viewer",
                                     RGB(255,255,0), RGB(0,0,128));

    open_btn     = ATGL_CREATE_BUTTON(toolbar, (ATGL_RECT){0,0,52,24}, (PU8)"Open",  on_open);
    ATGL_CREATE_SEPARATOR(toolbar, (ATGL_RECT){0,0,2,TOOLBAR_H});
    zoom_out_btn = ATGL_CREATE_BUTTON(toolbar, (ATGL_RECT){0,0,22,24}, (PU8)"-",     on_zoom_out);
    zoom_label   = ATGL_CREATE_LABEL (toolbar, (ATGL_RECT){0,0,44,20}, (PU8)"100%",
                                     RGB(255,255,255), RGB(0,0,128));
    zoom_in_btn  = ATGL_CREATE_BUTTON(toolbar, (ATGL_RECT){0,0,22,24}, (PU8)"+",     on_zoom_in);
    ATGL_CREATE_SEPARATOR(toolbar, (ATGL_RECT){0,0,2,TOOLBAR_H});
    fit_btn      = ATGL_CREATE_BUTTON(toolbar, (ATGL_RECT){0,0,36,24}, (PU8)"Fit",   on_fit);
    actual_btn   = ATGL_CREATE_BUTTON(toolbar, (ATGL_RECT){0,0,36,24}, (PU8)"1:1",   on_actual);
    ATGL_CREATE_SEPARATOR(toolbar, (ATGL_RECT){0,0,2,TOOLBAR_H});
    grid_btn     = ATGL_CREATE_BUTTON(toolbar, (ATGL_RECT){0,0,40,24}, (PU8)"Grid",  on_grid);
    ATGL_CREATE_SEPARATOR(toolbar, (ATGL_RECT){0,0,2,TOOLBAR_H});
    exit_btn     = ATGL_CREATE_BUTTON(toolbar, (ATGL_RECT){0,0,44,24}, (PU8)"Exit",  on_exit);

    /* ── Status bar ──────────────────────────────────────────── */
    status_label = ATGL_CREATE_LABEL(root,
                                     (ATGL_RECT){0, (I32)(sh - STATUS_H), (I32)sw, STATUS_H},
                                     (PU8)"No image loaded.",
                                     RGB(200,200,200), RGB(16,16,48));

    /* ── Placeholder canvas (blank grey) ────────────────────── */
    canvas = ATGL_CREATE_IMAGE_FROM_FILE(root,
                                     (ATGL_RECT){0, TOOLBAR_H, (I32)sw, (I32)(sh - TOOLBAR_H - STATUS_H)},
                                     def);

    /* ── Open-file popup ─────────────────────────────────────── */
    popup_overlay = ATGL_CREATE_PANEL(root,
                                      (ATGL_RECT){0, 0, (I32)sw, (I32)sh},
                                      ATGL_LAYOUT_NONE, 0, 0);
    ATGL_NODE_SET_COLORS(popup_overlay, RGB(255,255,255), VBE_SEE_THROUGH);

    popup_panel = ATGL_CREATE_PANEL(popup_overlay,
                                    (ATGL_RECT){0, 0, 320, 90},
                                    ATGL_LAYOUT_NONE, 0, 0);
    ATGL_NODE_SET_COLORS(popup_panel, RGB(255,255,255), RGB(32,32,64));
    ATGL_NODE_SET_TEXT(popup_panel, (PU8)"Open Image");
    ATGL_CENTER_IN_SCREEN(popup_panel);

    ATGL_CREATE_LABEL(popup_panel, (ATGL_RECT){10, 8,  60, 16},
                      (PU8)"Path:", RGB(255,255,255), RGB(32,32,64));
    popup_input = ATGL_CREATE_TEXTINPUT(popup_panel,
                                        (ATGL_RECT){72, 6, 238, 20},
                                        (PU8)"/HOME/image.TGI", 240);
    popup_ok_btn     = ATGL_CREATE_BUTTON(popup_panel, (ATGL_RECT){72,  46, 60, 24}, (PU8)"Open",   on_popup_ok);
    popup_cancel_btn = ATGL_CREATE_BUTTON(popup_panel, (ATGL_RECT){142, 46, 72, 24}, (PU8)"Cancel", on_popup_cancel);

    ATGL_NODE_SET_VISIBLE(popup_overlay, FALSE);

    return 0;
}

/* ================================================================ */
/*                       EVENT LOOP                                 */
/* ================================================================ */
#define ret ATGL_DISPATCH_EVENT(ATGL_GET_SCREEN_ROOT_NODE(), ev); return;
VOID ATGL_EVENT_LOOP(ATGL_EVENT *ev) {
    /* ── Popup captures all input ── */
    if (g_state.popup_open) {
        if (ev->type == ATGL_EVT_KEY_DOWN) {
            if (ev->key.keycode == KEY_ESC)   { hide_popup(); ret; }
            if (ev->key.keycode == KEY_ENTER)  { on_popup_ok(NULLPTR); ret; }
        }
        ret;
    }


    /* ── Keyboard shortcuts ── */
    if (ev->type == ATGL_EVT_KEY_DOWN) {
        if (ev->key.keycode == KEY_ESC) { ATGL_QUIT(); ret; }

        /* Zoom */
        if (ev->key.ch == '+' || ev->key.ch == '=') { on_zoom_in(NULLPTR);  ret; }
        if (ev->key.ch == '-')                       { on_zoom_out(NULLPTR); ret; }

        /* Fit / actual */
        if (ev->key.ch == 'f' || ev->key.ch == 'F') { do_fit(); ret; }
        if (ev->key.ch == '1')                       { on_actual(NULLPTR); ret; }

        /* Grid */
        if (ev->key.ch == 'g' || ev->key.ch == 'G') { on_grid(NULLPTR); ret; }

        /* Open */
        if (ev->key.ch == 'o' || ev->key.ch == 'O') { show_popup(); ret; }

        /* Arrow pan */
        if (canvas) {
            if (ev->key.keycode == KEY_ARROW_UP)    { ATGL_IMAGE_PAN(canvas,  0, -PAN_SPEED); update_status(); ret; }
            if (ev->key.keycode == KEY_ARROW_DOWN)  { ATGL_IMAGE_PAN(canvas,  0,  PAN_SPEED); update_status(); ret; }
            if (ev->key.keycode == KEY_ARROW_LEFT)  { ATGL_IMAGE_PAN(canvas, -PAN_SPEED, 0); update_status(); ret; }
            if (ev->key.keycode == KEY_ARROW_RIGHT) { ATGL_IMAGE_PAN(canvas,  PAN_SPEED, 0); update_status(); ret; }
        }
        ret;
    }

    if (ev->handled || !canvas) ret;

    /* ── Mouse pan (left or right button drag) ── */
    if (ev->type == ATGL_EVT_MOUSE_DOWN &&
        (ev->mouse.btn_left || ev->mouse.btn_right) &&
        ev->mouse.y >= TOOLBAR_H)
    {
        g_state.panning    = TRUE;
        g_state.pan_last_x = ev->mouse.x;
        g_state.pan_last_y = ev->mouse.y;
        ret;
    }

    if (ev->type == ATGL_EVT_MOUSE_UP) {
        g_state.panning = FALSE;
        ret;
    }

    if (ev->type == ATGL_EVT_MOUSE_MOVE && g_state.panning) {
        I32 dx = g_state.pan_last_x - ev->mouse.x;
        I32 dy = g_state.pan_last_y - ev->mouse.y;

        /* Scale delta by inverse zoom so pan speed feels constant */
        U32 zoom = ATGL_IMAGE_GET_ZOOM(canvas);
        if (zoom > 0) {
            dx = (dx * 100) / (I32)zoom;
            dy = (dy * 100) / (I32)zoom;
        }

        ATGL_IMAGE_PAN(canvas, dx, dy);
        g_state.pan_last_x = ev->mouse.x;
        g_state.pan_last_y = ev->mouse.y;
        update_status();
    }
    ATGL_DISPATCH_EVENT(ATGL_GET_SCREEN_ROOT_NODE(), ev);
}

/* ================================================================ */
/*                     GRAPHICS LOOP                                */
/* ================================================================ */

VOID ATGL_GRAPHICS_LOOP(U32 ticks) {
    (void)ticks;
    ATGL_RENDER_TREE(ATGL_GET_SCREEN_ROOT_NODE());
}
