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

ATGL_INIT();
PATGL_NODE root = ATGL_CREATE_ROOT();
ATGL_SET_ROOT(root);
```

### 2. Creating Widgets

Widgets are created as nodes and added to the node tree:

```c
PATGL_NODE btn = ATGL_CREATE_BUTTON("Click Me");
ATGL_ADD_CHILD(root, btn);
```

### 3. Event Loop

Integrate ATGL into your main loop:

```c
ATGL_EVENT ev;
while (1) {
    if (ATGL_POLL_EVENTS(&ev)) {
        ATGL_DISPATCH_EVENT(root, &ev);
    }
    ATGL_RENDER_TREE(root);
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

### Types
- `ATGL_RECT` – Rectangle (x, y, w, h)
- `ATGL_EVENT` – Event (type, key/mouse data)
- `ATGL_THEME` – Theme colors and style
- `PATGL_NODE` – Pointer to a node (widget or container)

### Core Functions
- `VOID ATGL_INIT(VOID);` – Initialize ATGL
- `VOID ATGL_QUIT(VOID);` – Cleanup
- `PATGL_NODE ATGL_CREATE_ROOT(VOID);` – Create root node
- `VOID ATGL_SET_ROOT(PATGL_NODE node);` – Set root node
- `VOID ATGL_RENDER_TREE(PATGL_NODE root);` – Render the node tree and mouse cursor
- `BOOL ATGL_POLL_EVENTS(ATGL_EVENT *ev);` – Poll for keyboard/mouse events
- `VOID ATGL_DISPATCH_EVENT(PATGL_NODE root, ATGL_EVENT *ev);` – Dispatch event through tree

### Node/Widget Functions
- `PATGL_NODE ATGL_CREATE_BUTTON(const char *label);`
- `PATGL_NODE ATGL_CREATE_LABEL(const char *text);`
- `PATGL_NODE ATGL_CREATE_TEXTBOX(const char *init);`
- `PATGL_NODE ATGL_CREATE_SLIDER(int min, int max, int value);`
- `PATGL_NODE ATGL_CREATE_CHECKBOX(const char *label, BOOL checked);`
- `PATGL_NODE ATGL_CREATE_CONTAINER(void);`
- `VOID ATGL_ADD_CHILD(PATGL_NODE parent, PATGL_NODE child);`
- `VOID ATGL_REMOVE_CHILD(PATGL_NODE parent, PATGL_NODE child);`
- `VOID ATGL_DESTROY_NODE(PATGL_NODE node);`

### Event/Focus Functions
- `VOID ATGL_SET_FOCUS(PATGL_NODE node);`
- `PATGL_NODE ATGL_GET_FOCUS(VOID);`
- `VOID ATGL_NEXT_FOCUS(VOID);`
- `VOID ATGL_PREV_FOCUS(VOID);`

### Theming
- `atgl.theme` – Global theme object (colors, border, etc.)
- `VOID ATGL_SET_THEME(const ATGL_THEME *theme);`

### Drawing Helpers
- `VOID DRAW_RECT(I32 x, I32 y, I32 w, I32 h, U32 color);`
- `VOID DRAW_TEXT(I32 x, I32 y, const char *text, U32 color);`
- `VOID DRAW_PIXEL(I32 x, I32 y, U32 color);`

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

int main(void) {
    ATGL_INIT();
    PATGL_NODE root = ATGL_CREATE_ROOT();
    PATGL_NODE btn = ATGL_CREATE_BUTTON("Hello!");
    ATGL_ADD_CHILD(root, btn);
    ATGL_SET_ROOT(root);

    ATGL_EVENT ev;
    while (1) {
        if (ATGL_POLL_EVENTS(&ev)) {
            ATGL_DISPATCH_EVENT(root, &ev);
        }
        ATGL_RENDER_TREE(root);
    }
    ATGL_QUIT();
    return 0;
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

- Anton (lead developer)
- atOS contributors

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
