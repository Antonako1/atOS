/*+++
    PROGRAMS/SYS_PROGS/SANDBOX/SANDBOX.c - ATGL demo program

    Part of atOS

    Demonstrates every ATGL widget: label, button, checkbox, radio,
    text input, slider, progress bar, listbox, separator, and panel layout.
---*/
#include <LIBRARIES/ATGL/ATGL.h>
#include <STD/TYPEDEF.h>
#include <STD/PROC_COM.h>

static PATGL_NODE info_label;
static PATGL_NODE progress;
static U32 tick_count;

/* ================================================================ */
/*                        CALLBACKS                                 */
/* ================================================================ */

static VOID on_hello_click(PATGL_NODE node)
{
    ATGL_NODE_SET_TEXT(info_label, "Button was clicked!");
}

static VOID on_quit_click(PATGL_NODE node)
{
    ATGL_QUIT();
}

static VOID on_checkbox_change(PATGL_NODE node)
{
    if (ATGL_CHECKBOX_GET(node))
        ATGL_NODE_SET_TEXT(info_label, "Checkbox: ON");
    else
        ATGL_NODE_SET_TEXT(info_label, "Checkbox: OFF");
}

static VOID on_radio_change(PATGL_NODE node)
{
    ATGL_NODE_SET_TEXT(info_label, node->text);
}

/* ================================================================ */
/*                     ATGL ENTRY POINT                             */
/* ================================================================ */

VOID exit1(VOID) {
    ENABLE_SHELL_KEYBOARD();
}

U32 ATGL_MAIN(U32 argc, PPU8 argv)
{
    DISABLE_SHELL_KEYBOARD();
    ON_EXIT(exit1);

    ATGL_CREATE_SCREEN(ATGL_SA_NONE);
    ATGL_INIT();

    PATGL_NODE root = ATGL_GET_SCREEN_ROOT_NODE();
    tick_count = 0;

    /* ---- Title ---- */
    ATGL_CREATE_LABEL(root, (ATGL_RECT){10, 10, 300, 12},
                      "ATGL Sandbox Demo", VBE_BLACK, VBE_SEE_THROUGH);

    /* ---- Status label (updated by callbacks) ---- */
    info_label = ATGL_CREATE_LABEL(root, (ATGL_RECT){10, 28, 400, 12},
                                   "Click a button below.",
                                   VBE_BLACK, VBE_SEE_THROUGH);

    /* ---- Buttons ---- */
    ATGL_CREATE_BUTTON(root, (ATGL_RECT){10, 48, 100, 24},
                       "Hello!", on_hello_click);
    ATGL_CREATE_BUTTON(root, (ATGL_RECT){120, 48, 100, 24},
                       "Quit", on_quit_click);

    /* ---- Separator ---- */
    ATGL_CREATE_SEPARATOR(root, (ATGL_RECT){10, 82, 300, 4});

    /* ---- Checkbox ---- */
    ATGL_CREATE_CHECKBOX(root, (ATGL_RECT){10, 94, 200, 16},
                         "Enable feature", FALSE, on_checkbox_change);

    /* ---- Radio group ---- */
    ATGL_CREATE_RADIO(root, (ATGL_RECT){10, 118, 200, 16},
                      "Option A", 1, TRUE,  on_radio_change);
    ATGL_CREATE_RADIO(root, (ATGL_RECT){10, 138, 200, 16},
                      "Option B", 1, FALSE, on_radio_change);
    ATGL_CREATE_RADIO(root, (ATGL_RECT){10, 158, 200, 16},
                      "Option C", 1, FALSE, on_radio_change);

    /* ---- Text input ---- */
    ATGL_CREATE_TEXTINPUT(root, (ATGL_RECT){10, 182, 200, 20},
                          "Type here...", 64);

    /* ---- Slider ---- */
    ATGL_CREATE_SLIDER(root, (ATGL_RECT){10, 210, 200, 20},
                       0, 100, 50, 5);

    /* ---- Progress bar ---- */
    progress = ATGL_CREATE_PROGRESSBAR(root,
                       (ATGL_RECT){10, 240, 200, 20}, 100);

    /* ---- Listbox ---- */
    PATGL_NODE list = ATGL_CREATE_LISTBOX(root,
                       (ATGL_RECT){10, 270, 200, 60}, 5);
    ATGL_LISTBOX_ADD_ITEM(list, "Item 1");
    ATGL_LISTBOX_ADD_ITEM(list, "Item 2");
    ATGL_LISTBOX_ADD_ITEM(list, "Item 3");
    ATGL_LISTBOX_ADD_ITEM(list, "Item 4");
    ATGL_LISTBOX_ADD_ITEM(list, "Item 5");

    return 0;
}

/* ================================================================ */
/*                       EVENT LOOP                                 */
/* ================================================================ */

VOID ATGL_EVENT_LOOP(ATGL_EVENT *ev)
{
    /* ESC to quit */
    if (ev->type == ATGL_EVT_KEY_DOWN && ev->key.keycode == KEY_ESC) {
        ATGL_QUIT();
        return;
    }

    ATGL_DISPATCH_EVENT(ATGL_GET_SCREEN_ROOT_NODE(), ev);
}

/* ================================================================ */
/*                     GRAPHICS LOOP                                */
/* ================================================================ */

VOID ATGL_GRAPHICS_LOOP(U32 ticks)
{
    /* Animate the progress bar */
    tick_count++;
    if (tick_count % 10 == 0) {
        U32 val = ATGL_PROGRESSBAR_GET(progress);
        val = (val + 1) % 101;
        ATGL_PROGRESSBAR_SET(progress, val);
    }

    ATGL_RENDER_TREE(ATGL_GET_SCREEN_ROOT_NODE());
}
