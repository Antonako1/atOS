# ATGL: atOS Graphics Library

## Overview

ATGL (atOS Graphics Library) is a modern, extensible, and modular graphics and UI toolkit for the atOS operating system. It provides a node-tree-based widget system, event handling, theming, and rendering primitives, enabling the creation of rich graphical user interfaces and applications on atOS.

ATGL is designed to be:
- **Modular**: All UI elements are nodes in a tree, supporting composition and extensibility.
- **Themed**: Supports customizable look-and-feel via theme objects.
- **Event-driven**: Handles keyboard, mouse, and focus events with a flexible dispatch system.
- **Efficient**: Optimized for real-time rendering and input on atOS.

---

## Features

- **Node Tree Architecture**: All UI elements are nodes, supporting parent/child relationships, layout, and rendering.
- **Widget System**: Includes buttons, labels, textboxes, sliders, checkboxes, containers, and more.
- **Event Handling**: Keyboard, mouse, and focus events are dispatched through the node tree.
- **Theming**: Fully customizable colors, borders, and styles via theme objects.
- **Focus Management**: Tab/Shift-Tab navigation, programmatic focus control.
- **Layout Helpers**: Automatic layout for containers and widgets.
- **Drawing Primitives**: Rectangle, text, and pixel drawing helpers.
- **Mouse Cursor Rendering**: Custom-drawn mouse cursor always rendered on top.
- **Integration with atOS Runtime**: Seamless integration with the atOS event/render loop.

---

## Directory Structure

- `ATGL.h`           – Public API header
- `ATGL_TYPES.h`     – Type definitions (nodes, events, theme, etc.)
- `ATGL_INTERNAL.h`  – Internal shared state and helpers
- `ATGL.c`           – Core implementation (init, quit, layout, helpers)
- `ATGL_NODE.c`      – Node tree management (create, destroy, add/remove child, hit-test)
- `ATGL_WIDGETS.c`   – Widget implementations (render, event, constructors)
- `ATGL_EVENT.c`     – Event polling, dispatch, focus, rendering, mouse cursor
- `ATGL_THEME.c`     – Theme management and default theme
- `README.md`        – This documentation
- `SOURCES`          – List of source files for build system

---

## Getting Started

### 1. Initialization

Before using ATGL, initialize the library and set up the root node:

```c
#include <LIBRARIES/ATGL/ATGL.h>

U32 ATGL_MAIN(U32 argc, PPU8 argv) {
    ATGL_CREATE_SCREEN(ATGL_SA_NONE);
    ATGL_INIT();
    ...

PATGL_NODE root = ATGL_GET_SCREEN_ROOT_NODE();
```

### 2. Creating Widgets

Widgets are created as nodes and added to the node tree:

```c
    ATGL_CREATE_LABEL(root, (ATGL_RECT){10, 10, 300, 12},
        "ATGL Demo", VBE_BLACK, VBE_SEE_THROUGH);

    // If non-zero is returned, the program will exit immediately with that code.
    return 0;
}
```

### 3. Event Loop

Integrate ATGL into your main loop:

```c
VOID ATGL_EVENT_LOOP(ATGL_EVENT *ev)
{
    /* ESC to quit */
    if (ev->type == ATGL_EVT_KEY_DOWN && ev->key.keycode == KEY_ESC) {
        ATGL_QUIT();
        return;
    }

    ATGL_DISPATCH_EVENT(ATGL_GET_SCREEN_ROOT_NODE(), ev);
}

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

```

### 4. Theming

Customize the look-and-feel by modifying the theme:

```c
atgl.theme.bg = RGB(30, 30, 30); // Set background color
atgl.theme.fg = RGB(220, 220, 220); // Set foreground color
```

---

## API Reference

### 1. Screen Management
These functions handle the initialization and management of the underlying screen buffer and ATGL environment.

- `VOID ATGL_CREATE_SCREEN(ATGL_SCREEN_ATTRIBS attrs);`
  Initializes the graphics screen with the specified attributes (e.g., standard, double-buffered, etc.).
- `VOID ATGL_DESTROY_SCREEN(VOID);`
  Destroys the active ATGL screen and frees graphics resources.
- `VOID ATGL_INIT(VOID);`
  Initializes the ATGL system, context, global state, and default theme.
- `PATGL_NODE ATGL_GET_SCREEN_ROOT_NODE(VOID);`
  Returns the root node of the current screen context. All widgets should be descendants of this node.
- `U32 ATGL_GET_SCREEN_WIDTH(VOID);`
  Returns the width of the active screen in pixels.
- `U32 ATGL_GET_SCREEN_HEIGHT(VOID);`
  Returns the height of the active screen in pixels.

### 2. Quit & Lifecycle
- `VOID ATGL_QUIT(VOID);`
  Signals ATGL to begin its shutdown process and break out of the runtime loop.
- `BOOL ATGL_SHOULD_QUIT(VOID);`
  Returns whether `ATGL_QUIT()` has been called, indicating the application is terminating.

### 3. Node Tree Operations
Every widget or container in ATGL is a "node".

- `PATGL_NODE ATGL_NODE_CREATE(ATGL_NODE_TYPE type, PATGL_NODE parent, ATGL_RECT rect);`
  Creates a baseline node of the given type, attaches it to the parent, and positions it natively.
- `BOOL ATGL_NODE_ADD_CHILD(PATGL_NODE parent, PATGL_NODE child);`
  Appends a child node to an existing parent node's list of children.
- `BOOL ATGL_NODE_REMOVE_CHILD(PATGL_NODE parent, PATGL_NODE child);`
  Unlinks a child node from its parent.
- `VOID ATGL_NODE_DESTROY(PATGL_NODE node);`
  Recursively destroys the specified node and all of its descendants, freeing associated memory.
- `VOID ATGL_NODE_SET_TEXT(PATGL_NODE node, PU8 text);`
  Updates the node's internal text buffer (used by labels, buttons, inputs).
- `VOID ATGL_NODE_SET_VISIBLE(PATGL_NODE node, BOOL visible);`
  Shows or hides a node (and its children).
- `VOID ATGL_NODE_SET_ENABLED(PATGL_NODE node, BOOL enabled);`
  Enables or disables interactivity for the node.
- `VOID ATGL_NODE_SET_RECT(PATGL_NODE node, ATGL_RECT rect);`
  Changes the rectangle bounding box of the node.
- `VOID ATGL_NODE_SET_COLORS(PATGL_NODE node, VBE_COLOUR fg, VBE_COLOUR bg);`
  Overrides the generic foreground and background colors for a specific node.
- `VOID ATGL_NODE_INVALIDATE(PATGL_NODE node);`
  Marks the node as "dirty", forcing it to be redrawn during the next frame.
- `PATGL_NODE ATGL_NODE_FIND_BY_ID(PATGL_NODE root, U32 id);`
  Performs a recursive search starting from `root` to find a node by its numerical `id`.

### 4. Widget Creation
High-level factory functions for built-in UI components.

- `PATGL_NODE ATGL_CREATE_LABEL(PATGL_NODE parent, ATGL_RECT rect, PU8 text, VBE_COLOUR fg, VBE_COLOUR bg);`
  Creates a static text label.
- `PATGL_NODE ATGL_CREATE_BUTTON(PATGL_NODE parent, ATGL_RECT rect, PU8 text, VOID (*on_click)(PATGL_NODE));`
  Creates a clickable button and registers the provided callback.
- `PATGL_NODE ATGL_CREATE_CHECKBOX(PATGL_NODE parent, ATGL_RECT rect, PU8 text, BOOL initial, VOID (*on_change)(PATGL_NODE));`
  Creates a checkbox toggle.
- `PATGL_NODE ATGL_CREATE_RADIO(PATGL_NODE parent, ATGL_RECT rect, PU8 text, U32 group_id, BOOL selected, VOID (*on_change)(PATGL_NODE));`
  Creates a radio button belonging to `group_id`. Only one radio in a group can be selected among siblings.
- `PATGL_NODE ATGL_CREATE_TEXTINPUT(PATGL_NODE parent, ATGL_RECT rect, PU8 placeholder, U32 max_len);`
  Creates an editable single-line text input field.
- `PATGL_NODE ATGL_CREATE_SLIDER(PATGL_NODE parent, ATGL_RECT rect, I32 min, I32 max, I32 value, I32 step);`
  Creates a draggable slider.
- `PATGL_NODE ATGL_CREATE_PROGRESSBAR(PATGL_NODE parent, ATGL_RECT rect, U32 max);`
  Creates a progress bar widget.
- `PATGL_NODE ATGL_CREATE_PANEL(PATGL_NODE parent, ATGL_RECT rect, ATGL_LAYOUT_DIR layout, U32 padding, U32 spacing);`
  Creates a generic container panel that can apply automatic spacing/padding constraints to its children.
- `PATGL_NODE ATGL_CREATE_LISTBOX(PATGL_NODE parent, ATGL_RECT rect, U32 visible_rows);`
  Creates a vertical scrollable listbox.
- `PATGL_NODE ATGL_CREATE_SEPARATOR(PATGL_NODE parent, ATGL_RECT rect);`
  Creates a 3D-styled horizontal or vertical separator line.
- `PATGL_NODE ATGL_CREATE_IMAGE(PATGL_NODE parent, ATGL_RECT rect, PU8 pixels, U32 img_w, U32 img_h);`
  Creates a widget that renders a flat buffer of image pixels.
- `PATGL_NODE ATGL_CREATE_BLANK_IMAGE(PATGL_NODE parent, ATGL_RECT rect, U32 img_w, U32 img_h, VBE_COLOUR fill);`
  Creates an image widget with a freshly allocated blank canvas.

### 5. Widget Getters / Setters
- `BOOL ATGL_CHECKBOX_GET(PATGL_NODE node);` / `VOID ATGL_CHECKBOX_SET(PATGL_NODE node, BOOL checked);`
  Accesses or updates the checked state of a Checkbox widget.
- `U32 ATGL_RADIO_GET_SELECTED(PATGL_NODE parent, U32 group_id);`
  Finds the node ID of the currently active Radio button within the `parent` container for the specified `group_id`.
- `PU8 ATGL_TEXTINPUT_GET_TEXT(PATGL_NODE node);` / `VOID ATGL_TEXTINPUT_SET_TEXT(PATGL_NODE node, PU8 text);`
  Retrieves or populates the content of a Text Input widget.
- `I32 ATGL_SLIDER_GET_VALUE(PATGL_NODE node);` / `VOID ATGL_SLIDER_SET_VALUE(PATGL_NODE node, I32 value);`
  Accesses or modifies the numeric thumb value of a Slider widget.
- `VOID ATGL_PROGRESSBAR_SET(PATGL_NODE node, U32 value);` / `U32 ATGL_PROGRESSBAR_GET(PATGL_NODE node);`
  Gets or sets the filled completion value of a Progress Bar widget.
- `VOID ATGL_LISTBOX_ADD_ITEM(PATGL_NODE node, PU8 text);`
  Appends a new selectable item to the end of a List Box.
- `U32 ATGL_LISTBOX_GET_SELECTED(PATGL_NODE node);`
  Returns the index of the actively highlighted row in a List Box.
- `PU8 ATGL_LISTBOX_GET_TEXT(PATGL_NODE node);`
  Returns the string text corresponding to the currently selected List Box item.
- `VOID ATGL_LISTBOX_CLEAR(PATGL_NODE node);`
  Empties the List Box of all internal items.

### 6. Image Manipulation
Advanced functions for drawing and manipulating pixel buffers within an Image widget.

- `VOID ATGL_IMAGE_SET_PIXEL(PATGL_NODE node, U32 x, U32 y, VBE_COLOUR colour);`
  Sets a single pixel in the image buffer using image-relative coordinates.
- `VBE_COLOUR ATGL_IMAGE_GET_PIXEL(PATGL_NODE node, U32 x, U32 y);`
  Retrieves a pixel color from the image buffer.
- `VOID ATGL_IMAGE_CLEAR(PATGL_NODE node, VBE_COLOUR colour);`
  Fills the entire image buffer with a solid color.
- `VOID ATGL_IMAGE_FILL_RECT(PATGL_NODE node, U32 x, U32 y, U32 w, U32 h, VBE_COLOUR colour);`
  Fills a rectangular region inside the image.
- `VOID ATGL_IMAGE_DRAW_RECT(PATGL_NODE node, U32 x, U32 y, U32 w, U32 h, VBE_COLOUR colour);`
  Draws a 1px outline rectangle inside the image.
- `VOID ATGL_IMAGE_DRAW_LINE(PATGL_NODE node, I32 x0, I32 y0, I32 x1, I32 y1, VBE_COLOUR colour);`
  Draws a line between two points inside the image using Bresenham's algorithm.
- `VOID ATGL_IMAGE_FILL_CIRCLE(PATGL_NODE node, I32 cx, I32 cy, U32 radius, VBE_COLOUR colour);`
  Draws a filled circle inside the image.
- `PU8 ATGL_IMAGE_GET_PIXELS(PATGL_NODE node);`
  Returns a pointer to the raw pixel buffer of the image.
- `VOID ATGL_IMAGE_GET_SIZE(PATGL_NODE node, U32 *w, U32 *h);`
  Retrieves the original dimensions (width and height) of the image.
- `VOID ATGL_IMAGE_SET_PIXELS(PATGL_NODE node, PU8 pixels, U32 img_w, U32 img_h);`
  Replaces the pixel buffer and updates the image dimensions.
- `VOID ATGL_IMAGE_SET_ZOOM(PATGL_NODE node, U32 zoom);` / `U32 ATGL_IMAGE_GET_ZOOM(PATGL_NODE node);`
  Sets or gets the zoom level (e.g., 100 for 1:1, 200 for 2x).
- `VOID ATGL_IMAGE_ZOOM_IN(PATGL_NODE node);` / `VOID ATGL_IMAGE_ZOOM_OUT(PATGL_NODE node);`
  Convenience functions to double or halve the current zoom level.
- `VOID ATGL_IMAGE_SET_OFFSET(PATGL_NODE node, I32 ox, I32 oy);` / `VOID ATGL_IMAGE_GET_OFFSET(PATGL_NODE node, I32 *ox, I32 *oy);`
  Sets or gets the panning offset of the image within its widget viewport.
- `VOID ATGL_IMAGE_PAN(PATGL_NODE node, I32 dx, I32 dy);`
  Adjusts the current pan offset by a relative delta.
- `BOOL ATGL_IMAGE_SCREEN_TO_IMG(PATGL_NODE node, I32 sx, I32 sy, U32 *ix, U32 *iy);`
  Converts screen-space coordinates to image-relative pixel coordinates, accounting for zoom and pan.
- `VOID ATGL_IMAGE_SHOW_GRID(PATGL_NODE node, BOOL show);`
  Toggles the visibility of a pixel grid (typically visible at high zoom levels).
- `VOID ATGL_IMAGE_SET_GRID_COLOUR(PATGL_NODE node, VBE_COLOUR colour);`
  Sets the color used for the pixel grid.

### 7. Event System
- `BOOL ATGL_POLL_EVENTS(ATGL_EVENT *ev);`
  Pops the next pending event (mouse/keyboard) from the OS message queue. Returns FALSE if the queue is empty.
- `VOID ATGL_DISPATCH_EVENT(PATGL_NODE root, ATGL_EVENT *ev);`
  Sends the queried event recursively down the node tree, updating bounds, hit-tests, and invoking node-specific event callbacks.
- `VOID ATGL_RENDER_TREE(PATGL_NODE root);`
  Handles redrawing the whole UI scene starting from the `root` node. Usually called once per frame.

### 8. Focus Management
- `VOID ATGL_SET_FOCUS(PATGL_NODE node);`
  Forces input focus onto the specific node (e.g. for typing into a text field).
- `PATGL_NODE ATGL_GET_FOCUS(VOID);`
  Returns the node that currently holds UI focus, if any.
- `VOID ATGL_NEXT_FOCUS(VOID);`
  Shifts the focus sequentially to the next eligible interactive widget (Tab-style mapping).
- `VOID ATGL_PREV_FOCUS(VOID);`
  Shifts the focus back to the preceding eligible widget (Shift+Tab-style mapping).

### 9. Theme System
Theme manipulation to change base style.
- `VOID ATGL_SET_THEME(ATGL_THEME *theme);`
  Applies a new visual configuration (colors, font, etc.) globally to ATGL instances.
- `ATGL_THEME *ATGL_GET_THEME(VOID);`
  Returns a pointer to the current actively used global Theme structure.
- `ATGL_THEME ATGL_DEFAULT_THEME(VOID);`
  Returns the default built-in graphical theme to easily revert style changes.

### 10. Layout
- `VOID ATGL_LAYOUT_APPLY(PATGL_NODE panel);`
  Triggers a manual re-calculation of dynamic layout bounding boxes for children of a panel.
- `VOID ATGL_CENTER_IN_PARENT(PATGL_NODE node);`
  Reposition the node straight in the exact center of its parent's bounding box.
- `VOID ATGL_CENTER_IN_SCREEN(PATGL_NODE node);`
  Reposition the node in the exact center of the screen viewport.

### 11. Drawing Helpers
Raw 2D rendering procedures exposed for custom widget creation.
- `VOID ATGL_DRAW_RAISED(I32 x, I32 y, I32 w, I32 h);`
  Draws a classic 3D "button out" bevel.
- `VOID ATGL_DRAW_SUNKEN(I32 x, I32 y, I32 w, I32 h);`
  Draws a recessed 3D "input box" bevel.
- `VOID ATGL_DRAW_TEXT(I32 x, I32 y, PU8 text, VBE_COLOUR fg, VBE_COLOUR bg);`
  Draws standard raw text at the given raster coordinates.
- `VOID ATGL_DRAW_TEXT_CLIPPED(I32 x, I32 y, I32 max_w, PU8 text, VBE_COLOUR fg, VBE_COLOUR bg);`
  Draws raw text but strictly truncates it (visually via bounds) if it exceeds `max_w` width.

### 12. Shapes and Pixels
Additional raw drawing primitives.

- `VOID ATGL_DRAW_PIXEL(U32 x, U32 y, VBE_COLOUR colour);`
  Draws a single pixel on the screen.
- `VOID ATGL_DRAW_RECTANGLE(U32 x, U32 y, U32 w, U32 h, VBE_COLOUR colour);`
  Draws a 1px outline rectangle.
- `VOID ATGL_DRAW_FILLED_RECTANGLE(U32 x, U32 y, U32 w, U32 h, VBE_COLOUR colour);`
  Draws a solid filled rectangle.
- `VOID ATGL_DRAW_ELLIPSE(U32 x, U32 y, U32 rx, U32 ry, VBE_COLOUR colour);`
  Draws an outline ellipse.
- `VOID ATGL_DRAW_FILLED_ELLIPSE(U32 x, U32 y, U32 rx, U32 ry, VBE_COLOUR colour);`
  Draws a solid filled ellipse.
- `VOID ATGL_DRAW_LINE(U32 x1, U32 y1, U32 x2, U32 y2, VBE_COLOUR colour);`
  Draws a 2D line.
- `VOID ATGL_DRAW_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour);`
  Draws an outline triangle.
- `VOID ATGL_DRAW_FILLED_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour);`
  Draws a solid filled triangle.

### 13. User-Defined Callbacks (Runtime_ATGL)
These routines must be mapped by the user-defined program when using ATGL's managed runtime setup.
- `U32 ATGL_MAIN(U32 argc, PPU8 argv);`
  The main entry point executed upon process startup to construct the user UI. 
- `VOID ATGL_EVENT_LOOP(ATGL_EVENT *ev);`
  Callback invoked for each individual event polled in the system tick.
- `VOID ATGL_GRAPHICS_LOOP(U32 ticks);`
  A periodically invoked callback allowing dynamic re-rendering, continuous animations, or programmatic widget updates.

---

## Widget System

ATGL provides a set of built-in widgets, each with rendering and event handling:

- **Button**: Clickable, focusable, supports keyboard and mouse events.
- **Label**: Static text display.
- **Textbox**: Editable text field, supports keyboard input and cursor.
- **Slider**: Adjustable value, supports mouse drag and keyboard.
- **Checkbox**: Toggleable boolean, supports mouse and keyboard.
- **Container**: Holds child nodes, supports layout.

All widgets can be extended or customized by providing custom render/event functions.

---

## Event Handling

ATGL supports keyboard and mouse events:
- **Keyboard**: Key down/up, ASCII mapping, modifiers (shift, ctrl, alt)
- **Mouse**: Move, button down/up, click, position, button state
- **Focus**: Tab/Shift-Tab to cycle, programmatic focus

Events are dispatched depth-first through the node tree. Focused node receives key events first. Mouse events perform hit-testing to determine the target node.

---

## Theming

Themes control the appearance of all widgets. The default theme mimics classic Win32, but all colors and styles can be customized:

```c
atgl.theme.bg = RGB(40, 40, 40);
atgl.theme.fg = RGB(220, 220, 220);
atgl.theme.button = RGB(60, 60, 120);
atgl.theme.button_hover = RGB(80, 80, 160);
atgl.theme.button_active = RGB(100, 100, 200);
// ...and more
```

You can set a new theme at runtime using `ATGL_SET_THEME()`.

---

## Mouse Cursor

ATGL draws a custom mouse cursor (arrow) at the current mouse position, always rendered on top of all UI elements. The cursor color is taken from the theme foreground color.

---

## Example: Minimal Program

```c
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

```

---

## Advanced Topics

### Custom Widgets
You can create your own widgets by subclassing nodes and providing custom render/event functions.

### Layout
Containers can automatically arrange children. You can implement custom layout logic for advanced UIs.

### Performance
ATGL is optimized for real-time rendering. Only dirty nodes are re-rendered. The mouse cursor is drawn last for maximum responsiveness.

### Integration
ATGL is designed to work seamlessly with the atOS runtime and other libraries. See the demo program in `SANDBOX.c` for a comprehensive example.

---

## Contributing

Contributions are welcome! Please see the atOS repository for guidelines. All code is MIT licensed.

---

## License

ATGL is part of atOS and is licensed under the MIT License. See the LICENSE file for details.

---

## Authors

- Antonako1 (lead developer)

---

## See Also

- [atOS Kernel](../../KERNEL/README.md)
- [ATUI Library](../ATUI/README.md)
- [atOS Runtime](../../RUNTIME/README.md)
- [Demo Program](../../PROGRAMS/SYS_PROGS/SANDBOX/SANDBOX.c)

---

## Changelog

- **v1.0**: Initial release with node tree, widgets, theming, event system, and demo.

---

## Contact

For questions, bug reports, or contributions, please open an issue on the atOS GitHub repository.
