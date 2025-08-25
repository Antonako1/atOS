/*+++
    Source/KERNEL/32RTOSKRNL/DRIVERS/VIDEO/VBE.h - VESA BIOS Extensions (VBE) Definitions

    Part of atOS-RT

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    32-bit VESA BIOS Extensions (VBE) definitions for atOS-RT.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version. Contains VBE mode structure and constants.
    2025/08/20-25 - Antonako1
        Fixed VBE mode initialization and framebuffer setup.
        Added support for 8x8 character drawing.
        Improved framebuffer performance.

REMARKS
    Dependant on VESA.h for VESA_INFO structure and constants.
    Use this header to include VBE mode definitions in your application.
---*/
#ifndef VBE_H
#define VBE_H

#include "./VESA.h"

/*+++

    VESA Pixel Access Macros
    These macros allow easy access to pixel data in VBE modes.
    Mode: VBE_MODE
    x: X coordinate of the pixel
    y: Y coordinate of the pixel
    Usage:
        VBE_PIXEL_PTR(mode, x, y) - Returns a pointer to the pixel at (x, y) in the specified mode.
    Example:
        U32 *pixel = VBE_PIXEL_PTR(mode, 100, 200);
        *pixel = 0x00FF00; // Set pixel to green

---*/
#define VBE_PIXEL_PTR(mode, x, y) \
    ((U32*)((U8*)(mode)->PhysBasePtr + ((y) * (mode)->XResolution + (x)) * 4))

#define VBE_COLOUR(r, g, b) \
    ((U32)((((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((b) & 0xFF)))

#define VBE_COORDS(x, y) \
    ((U32)((((y) & 0xFFFF) << 16) | ((x) & 0xFFFF)))

#define VBE_COLOUR_COORDS(x, y, r, g, b) \
    ((U32)((((y) & 0xFFFF) << 16) | ((x) & 0xFFFF) | \
    (((r) & 0xFF) << 24) | (((g) & 0xFF) << 16) | ((b) & 0xFF)))
    
#define VBE_SET_PIXEL(mode, x, y, color) \
    *(VBE_PIXEL_PTR(mode, x, y)) = (color)

#define VBE_MODE_OFFSET VESA_LOAD_ADDRESS_PHYS + VESA_CTRL_SIZE
#define VBE_MODE_LOAD_ADDRESS_PHYS VBE_MODE_OFFSET
#define VBE_MODE_SIZE 256
#define VESA_TARGET_MODE 0x117 // 1024x768x32bpp

#define VBE_OFF_X_RESOLUTION       0x12
#define VBE_OFF_Y_RESOLUTION       0x14
#define VBE_OFF_BITS_PER_PIXEL     0x19
#define VBE_OFF_PHYS_BASE_PTR      0x28

// Colours
#define VBE_COLOUR_BLACK   0x000000
#define VBE_COLOUR_WHITE   0xFFFFFF
#define VBE_COLOUR_RED     0xFF0000
#define VBE_COLOUR_GREEN   0x00FF00
#define VBE_COLOUR_BLUE    0x0000FF
#define VBE_COLOUR_YELLOW  0xFFFF00
#define VBE_COLOUR_CYAN    0x00FFFF


#define SCREEN_BPP 32

/*
Framebuffer is located at 0x00F00000u-0x011FFFFFu
It has a size of ~3MB, which is enough for the resolution 1024*768*(32/8)

Video memory data is written into this location, and updated to the vbe framebuffer.
*/
#define FRAMEBUFFER_ADDRESS MEM_FRAMEBUFFER_BASE
#define FRAMEBUFFER_END MEM_FRAMEBUFFER_END
#define FRAMEBUFFER_SIZE (FRAMEBUFFER_END - FRAMEBUFFER_ADDRESS + 1)


// 5:6:5 color format
/*
Usage as follows:
Red min-max: 0-31
Green min-max: 0-63
Blue min-max: 0-31
*/
#define VBE_COLOUR(r, g, b) \
    (U16)((r << 11) | (g << 5) | b)

#define DECONSTRUCT_VBE_COLOUR(colour, r, g, b) \
    do { \
        r = (colour >> 11) & 0x1F; \
        g = (colour >> 5) & 0x3F; \
        b = colour & 0x1F; \
    } while (0)

// This is special colour:
#define VBE_SEE_THROUGH  VBE_COLOUR(30, 62, 30)
// Reds
#define VBE_RED          VBE_COLOUR(31, 0, 0)
#define VBE_DARK_RED     VBE_COLOUR(15, 0, 0)
#define VBE_LIGHT_RED    VBE_COLOUR(31, 16, 16)

// Greens
#define VBE_GREEN        VBE_COLOUR(0, 63, 0)
#define VBE_DARK_GREEN   VBE_COLOUR(0, 31, 0)
#define VBE_LIGHT_GREEN  VBE_COLOUR(16, 63, 16)

// Blues
#define VBE_BLUE         VBE_COLOUR(0, 0, 31)
#define VBE_DARK_BLUE    VBE_COLOUR(0, 0, 15)
#define VBE_LIGHT_BLUE   VBE_COLOUR(16, 16, 31)

// Yellows
#define VBE_YELLOW       VBE_COLOUR(31, 63, 0)
#define VBE_DARK_YELLOW  VBE_COLOUR(15, 31, 0)
#define VBE_LIGHT_YELLOW VBE_COLOUR(31, 63, 16)

// Cyans
#define VBE_CYAN         VBE_COLOUR(0, 63, 31)
#define VBE_DARK_CYAN    VBE_COLOUR(0, 31, 15)
#define VBE_LIGHT_CYAN   VBE_COLOUR(16, 63, 31)

// Magentas
#define VBE_MAGENTA      VBE_COLOUR(31, 0, 31)
#define VBE_DARK_MAGENTA VBE_COLOUR(15, 0, 15)
#define VBE_LIGHT_MAGENTA VBE_COLOUR(31, 16, 31)

// Oranges
#define VBE_ORANGE       VBE_COLOUR(31, 31, 0)
#define VBE_DARK_ORANGE  VBE_COLOUR(15, 15, 0)
#define VBE_LIGHT_ORANGE VBE_COLOUR(31, 47, 16)

// Browns
#define VBE_BROWN        VBE_COLOUR(19, 31, 0)
#define VBE_DARK_BROWN   VBE_COLOUR(9, 15, 0)
#define VBE_LIGHT_BROWN  VBE_COLOUR(23, 47, 8)

// Grays
#define VBE_GRAY         VBE_COLOUR(16, 32, 16)
#define VBE_LIGHT_GRAY   VBE_COLOUR(24, 48, 24)
#define VBE_DARK_GRAY    VBE_COLOUR(8, 16, 8)

// Black & White
#define VBE_BLACK        VBE_COLOUR(0, 0, 0)
#define VBE_WHITE        VBE_COLOUR(31, 63, 31)

// Additional common colors
#define VBE_PINK         VBE_COLOUR(31, 24, 28)
#define VBE_PURPLE       VBE_COLOUR(19, 0, 31)
#define VBE_PURPLE2      VBE_COLOUR(26, 0, 31)
#define VBE_LIME         VBE_COLOUR(16, 63, 16)
#define VBE_TEAL         VBE_COLOUR(0, 31, 31)
#define VBE_NAVY         VBE_COLOUR(0, 0, 19)
#define VBE_OLIVE        VBE_COLOUR(19, 31, 0)
#define VBE_MAROON       VBE_COLOUR(16, 0, 0)
#define VBE_AQUA         VBE_COLOUR(0, 31, 31)
#define VBE_SILVER       VBE_COLOUR(24, 48, 24)
#define VBE_GOLD         VBE_COLOUR(31, 51, 0)
#define VBE_CORAL        VBE_COLOUR(31, 31, 16)
#define VBE_INDIGO       VBE_COLOUR(8, 0, 16)
#define VBE_VIOLET       VBE_COLOUR(23, 0, 31)
#define VBE_BEIGE        VBE_COLOUR(24, 48, 16)
#define VBE_TAN          VBE_COLOUR(21, 40, 8)
#define VBE_KHAKI        VBE_COLOUR(24, 51, 8)
#define VBE_LAVENDER     VBE_COLOUR(28, 24, 28)
#define VBE_SALMON       VBE_COLOUR(31, 24, 16)
#define VBE_CRIMSON      VBE_COLOUR(31, 0, 8)
#define VBE_CYAN2        VBE_COLOUR(0, 63, 28)


// VBE_COLOUR macros
typedef U16 VBE_PIXEL_COLOUR;

typedef struct {
    U32 X;
    U32 Y;
    VBE_PIXEL_COLOUR Colour;
} VBE_PIXEL_INFO;

/// @brief Creates a pixel information structure.
/// Usage: CREATE_VBE_PIXEL_INFO(x, y, colour)
///     CREATE_VBE_PIXEL_INFO(100, 100, VBE_COLOUR_RED)
#define CREATE_VBE_PIXEL_INFO(x, y, colour) \
    ((VBE_PIXEL_INFO){(U32)(x), (U32)(y), (VBE_PIXEL_COLOUR)(colour)})


// Allows for 255 characters. ASCII 0-255
#define VBE_FONT_HEIGHT_8x8 8
#define VBE_FONT_WIDTH_8x8 8
#define VBE_MAX_CHARS 255
// extern U8 VBE_LETTERS_8x8[VBE_MAX_CHARS][VBE_FONT_HEIGHT_8x8];


/// @brief VESA BIOS Extensions (VBE) mode information
typedef struct {
    U16 ModeAttributes;      // Mode attributes (bit flags)
    U8  WinAAttributes;      // Window A attributes
    U8  WinBAttributes;      // Window B attributes
    U16 WinGranularity;      // Window granularity in KB
    U16 WinSize;             // Window size in KB
    U16 WinASegment;         // Segment address of window A
    U16 WinBSegment;         // Segment address of window B
    U32 WinFuncPtr;          // Real mode pointer to window function
    U16 BytesPerScanLine;    // Bytes per scan line

    U16 XResolution;         // Horizontal resolution in pixels
    U16 YResolution;         // Vertical resolution in pixels
    U8  XCharSize;           // Character cell width
    U8  YCharSize;           // Character cell height
    U8  NumberOfPlanes;      // Number of memory planes
    U8  BitsPerPixel;        // Bits per pixel
    U8  NumberOfBanks;       // Number of banks
    U8  MemoryModel;         // Memory model (packed, direct, etc.)
    U8  BankSize;            // Bank size in KB
    U8  NumberOfImagePages;  // Number of images
    U8  Reserved1;           // Reserved

    U8  RedMaskSize;         // # of bits for red
    U8  RedFieldPosition;    // Position of red
    U8  GreenMaskSize;      // # of bits for green
    U8  GreenFieldPosition;  // Position of green
    U8  BlueMaskSize;        // # of bits for blue
    U8  BlueFieldPosition;   // Position of blue
    U8  RsvMaskSize;         // # of bits for reserved
    U8  RsvFieldPosition;    // Position of reserved

    U32 PhysBasePtr;         // Physical address of linear framebuffer
    U32 OffScreenMemOffset;  // Pointer to start of offscreen memory
    U16 OffScreenMemSize;    // Size of offscreen memory in KB
    U8  Reserved2[206];      // Padding / reserved
} __attribute__((packed)) VBE_MODE;

#define GET_VBE_MODE() ((VBE_MODE*)(VBE_MODE_LOAD_ADDRESS_PHYS))

/*+++
STATIC INLINE BOOL vbe_check(U0)

DESCRIPTION
    Checks the validity of the VBE mode.
    This function verifies the VBE mode information structure and ensures
    that the necessary fields are populated correctly.

    This file contains bare minimum VBE support for 32-bit color modes, use 16-bit colours instead.

    Since this file is included by the much smaller KERNEL.c, it is important to ensure that
    only the bare-bones functionality is implemented here.
    The rest should be implemented in VIDEODRIVER.c and VIDEODRIVER.h.

RETURN
    TRUE if the VBE mode is valid, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version.

REMARKS:
    This function is called during the initialization phase to ensure
    that the VBE mode is set up correctly before use.
---*/
BOOL vbe_check(U0);

/*+++
BOOLEAN VBE_DRAW_CHARACTER(U32 x, U32 y, CHAR c, VBE_PIXEL_COLOUR fg, VBE_PIXEL_COLOUR bg);

DESCRIPTION
    Draws a character on the framebuffer.

PARAMETERS
    U32            x
        The x-coordinate to draw the character.
    U32            y
        The y-coordinate to draw the character.
    CHAR           c
        The character to draw.
    VBE_PIXEL_COLOUR fg
        The foreground color.
    VBE_PIXEL_COLOUR bg
        The background color.

RETURN
    TRUE if successful, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version.

REMARKS
    This function is called to draw a character on the screen.
---*/
BOOLEAN VBE_DRAW_CHARACTER_8x8(U32 x, U32 y, CHAR c, VBE_PIXEL_COLOUR fg, VBE_PIXEL_COLOUR bg);

/*+++
U0 UPDATE_VRAM(U0);

DESCRIPTION
    Updates the VRAM by copying the contents of the framebuffer to the physical
    address specified by the VBE mode.

RETURN
    None.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/19 - Antonako1
        Initial version.

REMARKS
    This function is called to refresh the screen contents.
---*/
U0 UPDATE_VRAM(U0);


/*+++
U0 VBE_STOP_DRAWING(U0);

DESCRIPTION
    Calls once drawing is complete to finalize any changes and update the VRAM.

PARAMETERS
    U0

RETURN
    None.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/19 - Antonako1
        Initial version.

REMARKS
    This function is called to refresh the screen contents.
---*/
U0 VBE_STOP_DRAWING(U0);


/*+++
BOOLEAN VBE_DRAW_PIXEL(VBE_PIXEL_INFO pixel_info)

DESCRIPTION
    Draws a pixel on the framebuffer.

PARAMETERS
    VBE_PIXEL_INFO    pixel_info
        Information about the pixel to draw.

RETURN
    True if successful, False otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version.

REMARKS
    This draws a pixel in the frambuffer into the given coordinates inside pixel info
---*/
BOOLEAN VBE_DRAW_PIXEL(VBE_PIXEL_INFO pixel_info);


/*+++
BOOLEAN VBE_DRAW_FRAMEBUFFER(U32 pos, VBE_PIXEL_COLOUR colours)

DESCRIPTION
    Draws a pixel on the framebuffer.

PARAMETERS
    U32    pos
        The position (offset) in the framebuffer.
    VBE_PIXEL_COLOUR colours
        The color information for the pixel.

RETURN
    TRUE if successful, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version.

REMARKS
    This draws a pixel in the framebuffer, not at the current cursor position nor at coordinates.
---*/
BOOLEAN VBE_DRAW_FRAMEBUFFER(U32 pos, VBE_PIXEL_COLOUR colours);


/*+++
BOOLEAN VBE_DRAW_ELLIPSE(U32 x0, U32 y0, U32 a, U32 b, VBE_PIXEL_COLOUR colour)

DESCRIPTION
    Draws an ellipse on the framebuffer.

PARAMETERS
    U32    x0
        The x-coordinate of the center of the ellipse.
    U32    y0
        The y-coordinate of the center of the ellipse.
    U32    a
        Horizontal semi-axis length.
    U32    b
        The vertical semi-axis length.
    VBE_PIXEL_COLOUR colours
        The color information for the ellipse.

        RETURN
    TRUE if successful, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version.

REMARKS
    This draws an ellipse in the framebuffer.
---*/
BOOLEAN VBE_DRAW_ELLIPSE(
    U32 x0, 
    U32 y0, 
    U32 a, 
    U32 b, 
    VBE_PIXEL_COLOUR colour
);

/*+++
BOOLEAN VBE_DRAW_LINE(U32 x1, U32 y1, U32 x2, U32 y2, VBE_PIXEL_COLOUR colours)

DESCRIPTION
    Draws a line on the framebuffer.

PARAMETERS
    U32    x1
        The x-coordinate of the start point.
    U32    y1
        The y-coordinate of the start point.
    U32    x2
        The x-coordinate of the end point.
    U32    y2
        The y-coordinate of the end point.
    VBE_PIXEL_COLOUR colours
        The color information for the line.

RETURN
    TRUE if successful, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version.

REMARKS
    This draws a line in the framebuffer.
---*/
BOOLEAN VBE_DRAW_LINE(U32 x1, U32 y1, U32 x2, U32 y2, VBE_PIXEL_COLOUR colours);


/*+++
BOOLEAN VBE_DRAW_RECTANGLE(U32 x, U32 y, U32 width, U32 height, VBE_PIXEL_COLOUR colours)

DESCRIPTION
    Draws a rectangle on the framebuffer.

PARAMETERS
    U32    x
        The x-coordinate of the top-left corner.
    U32    y
        The y-coordinate of the top-left corner.
    U32    width
        The width of the rectangle.
    U32    height
        The height of the rectangle.
    VBE_PIXEL_COLOUR colours
        The color information for the rectangle.

RETURN
    TRUE if successful, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version.

REMARKS
    This draws a rectangle in the framebuffer.
---*/
BOOLEAN VBE_DRAW_RECTANGLE(U32 x, U32 y, U32 width, U32 height, VBE_PIXEL_COLOUR colours);

/*+++
BOOLEAN VBE_DRAW_RECTANGLE_FILLED(U32 x, U32 y, U32 width, U32 height, VBE_PIXEL_COLOUR colours)

DESCRIPTION
    Draws a filled rectangle on the framebuffer.

PARAMETERS
    U32    x
        The x-coordinate of the top-left corner.
    U32    y
        The y-coordinate of the top-left corner.
    U32    width
        The width of the rectangle.
    U32    height
        The height of the rectangle.
    VBE_PIXEL_COLOUR colours
        The color information for the rectangle.

RETURN
    TRUE if successful, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/25 - Antonako1
        Initial version.

REMARKS
    This draws a filled rectangle in the framebuffer.
---*/
BOOLEAN VBE_DRAW_RECTANGLE_FILLED(U32 x, U32 y, U32 width, U32 height, VBE_PIXEL_COLOUR colours);

/*+++
BOOLEAN VBE_DRAW_RECTANGLE(U32 x, U32 y, U32 width, U32 height, VBE_PIXEL_COLOUR colours)

DESCRIPTION
    Draws a triangle on the framebuffer.

PARAMETERS
    U32    x1
        The x-coordinate of the first vertex.
    U32    y1
        The y-coordinate of the first vertex.
    U32    x2
        The x-coordinate of the second vertex.
    U32    y2
        The y-coordinate of the second vertex.
    U32    x3
        The x-coordinate of the third vertex.
    U32    y3
        The y-coordinate of the third vertex.
    VBE_PIXEL_COLOUR colours
        The color information for the triangle.

RETURN
    TRUE if successful, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version.

REMARKS
    This draws a triangle in the framebuffer.
---*/
BOOLEAN VBE_DRAW_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_PIXEL_COLOUR colours);

/*+++
BOOLEAN VBE_DRAW_TRIANGLE_FILLED(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_PIXEL_COLOUR colours)

DESCRIPTION
    Draws a filled triangle on the framebuffer.

PARAMETERS
    U32    x1
        The x-coordinate of the first vertex.
    U32    y1
        The y-coordinate of the first vertex.
    U32    x2
        The x-coordinate of the second vertex.
    U32    y2
        The y-coordinate of the second vertex.
    U32    x3
        The x-coordinate of the third vertex.
    U32    y3
        The y-coordinate of the third vertex.
    VBE_PIXEL_COLOUR colours
        The color information for the triangle.

RETURN
    TRUE if successful, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/25 - Antonako1
        Initial version.

REMARKS
    This draws a filled triangle in the framebuffer.
---*/
BOOLEAN VBE_DRAW_TRIANGLE_FILLED(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_PIXEL_COLOUR colours);

#endif // VBE_H
