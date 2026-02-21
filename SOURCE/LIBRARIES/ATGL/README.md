# ATGL - Graphical Library for atOS

ATGL (atOS Graphical Library) is a lightweight and efficient graphical library designed specifically for the atOS operating system. It provides essential functionalities for creating and managing graphical user interfaces in atOS applications.

## Features

- Basic drawing functions: lines, rectangles, circles, text rendering, image handling and more.
- Fast: Optimized for performance. Draws directly to the task framebuffer.
- Simple API: Easy to use functions for common graphical tasks.

## Usage

Define a RUNTIME_ATGL compilation flag when building your application to include ATGL supportw

```c
#include <LIBRARIES/ATGL/ATGL.h>

U32 ATGL_MAIN(U32 argc, PPU8 argv) {
    // Initialize ATGL Screen

    return 0;
}
```

## In-Depth

 - ATGL_SCREEN
 - This is the main instance of ATGL
    - ATGL_WINDOW
        - This is the window that graphics are drawn onto.
        - ATGL_SCREEN contains one main window.
        - Windows can have child windows which are simply just pop-up windows
    - ATGL_NODES
        - Elements like text, images, buttons, etc.     
    - ATGL_INPUT
        - Input handler
