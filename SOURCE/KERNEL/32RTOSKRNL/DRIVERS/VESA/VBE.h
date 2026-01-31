/*+++
    Source/KERNEL/32RTOSKRNL/DRIVERS/VESA/VBE.h - VESA BIOS Extensions (VBE) Definitions

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    32-bit VESA BIOS Extensions (VBE) definitions for atOS.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version. Contains VBE mode structure and constants.

REMARKS
    Dependant on VESA.h for VESA_INFO structure and constants.
    Use this header to include VBE mode definitions in your application.

    Access VBE struct at memory address VBE_MODE_LOAD_ADDRESS_PHYS,
        or use macro GET_VBE_MODE()

    OS runs in VBE 116h, 1024x768x32bpp.
     Although allowing 32-bit real colours,
     use the 5:6:5 colour format and its macros.

    When compiling include VBE.c and VESA.c

FUNCTIONS
    NOTE: Always update video ram after done drawing if not told otherwise

    BOOL vbe_check(...);                    // Checks VBE compatibility
    BOOLEAN VBE_DRAW_CHARACTER(...);        // Draws a character on the screen
    BOOLEAN VBE_DRAW_STRING(...);           // Draws a string on the screen
    U0 VBE_UPDATE_VRAM(...);                    // Updates the video RAM
    U0 VBE_STOP_DRAWING(...);               // Updates the video RAM
    BOOLEAN VBE_CLEAR_SCREEN(...);          // Clears the screen with given colour
    BOOLEAN VBE_FLUSH_SCREEN(...);          // Clears screen with black
    BOOLEAN VBE_DRAW_PIXEL(...);            // Draws a pixel on the screen
    BOOLEAN VBE_DRAW_FRAMEBUFFER(...);      // Draws a pixel in the framebuffer
    BOOLEAN VBE_DRAW_ELLIPSE(...);          // Draws an ellipse on the screen
    BOOLEAN VBE_DRAW_LINE(...);             // Draws a line on the screen
    BOOLEAN VBE_DRAW_RECTANGLE(...);        // Draws a rectangle on the screen
    BOOLEAN VBE_DRAW_TRIANGLE(...);         // Draws a triangle on the screen
    BOOLEAN VBE_DRAW_TRIANGLE_FILLED(...);  // Draws a filled triangle on the screen
---*/
#ifndef VBE_H
#define VBE_H

#include "./VESA.h"

#if defined(__RTOS__) || defined(KERNEL_ENTRY)
#define VBE_MODE_OFFSET (VESA_LOAD_ADDRESS_PHYS + VESA_CTRL_SIZE)
#define VBE_MODE_LOAD_ADDRESS_PHYS (VBE_MODE_OFFSET)
#define VBE_MODE_SIZE 256

#define SCREEN_BPP 32

/*
Framebuffer is located at 0x08562000-0x0B000000
It has a size of ~3MB, which is enough for the resolution 1024*768*(32/8)

Video memory data is written into this location, and updated to the vbe framebuffer.
*/
#define FRAMEBUFFER_ADDRESS MEM_FRAMEBUFFER_BASE
#define FRAMEBUFFER_END MEM_FRAMEBUFFER_END
#define FRAMEBUFFER_SIZE 3145728

void flush_focused_framebuffer();
void update_current_framebuffer();

void debug_vram_start();
void debug_vram_dump();
#endif
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

#define VBE_SEE_THROUGH VBE_COLOUR(0, 1, 2)

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

#define VBE_NOTEPAD_PAPER1 VBE_COLOUR(31, 31, 24) // slightly brighter
#define VBE_NOTEPAD_PAPER2 VBE_COLOUR(28, 28, 16) // more muted, aged look
#define VBE_NOTEPAD_PAPER3 VBE_COLOUR(31, 28, 16) // slightly orange tint

// VBE_COLOUR macros
typedef U16 VBE_PIXEL_COLOUR;
typedef U16 VBE_COLOUR; // For compatibility with older code

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

#if defined(__RTOS__) || defined(KERNEL_ENTRY)
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

    /* remainder is optional for VESA modes in v1.0/1.1, needed for OEM modes */
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

    /* VBE 1.2+ */
    U8  RedMaskSize;         // # of bits for red
    U8  RedFieldPosition;    // Position of red
    U8  GreenMaskSize;      // # of bits for green
    U8  GreenFieldPosition;  // Position of green
    U8  BlueMaskSize;        // # of bits for blue
    U8  BlueFieldPosition;   // Position of blue
    U8  RsvMaskSize;         // # of bits for reserved
    U8  RsvFieldPosition;    // Position of reserved
    U8  DirectColorModeInfo; // Direct color mode info
    /* Bit 0: 0 = Direct Color, 1 = Pseudo Color */

    /* VBE 2.0+ */
    U32 PhysBasePtr;         // Physical address of linear framebuffer
    U32 OffScreenMemOffset;  // Pointer to start of offscreen memory
    U16 OffScreenMemSize;    // Size of offscreen memory in KB

    /* VBE 3.0+ */
    U16 BytesPerScanLineLinear; // Bytes per scan line in linear mode
    U8 NumOfImagesBanked;          // Number of images in banked mode
    U8 NumOfImagesLinear;          // Number of images in linear mode
    U8 LinearModesRedMaskSz;     // Red mask size in linear mode
    U8 LinearModesRedLsb; // Red LSB position in linear mode
    U8 LinearModesGreenMaskSz;   // Green mask size in linear mode
    U8 LinearModesGreenLsb;       // Green LSB position in linear mode
    U8 LinearModesBlueMaskSz;     // Blue mask size in linear mode
    U8 LinearModesBlueLsb;        // Blue LSB position in linear mode
    U8 LinearModesRsvMaskSz;      // Reserved mask size in linear mode
    U8 LinearModesRsvLsb;         // Reserved LSB position in linear mode
    U32 PhysBasePtr2;             // Second physical base pointer (if applicable)
} __attribute__((packed)) VBE_MODEINFO;

#define GET_VBE_MODE() ((VBE_MODEINFO*)(VBE_MODE_LOAD_ADDRESS_PHYS))

/*+++
BOOL vbe_check(U0)

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


// Allows for 127 characters; Extended ASCII is implemented as 16x16 in the VIDEODRIVER
#define UNUSABLE_CHARS 33 // Used for saving space for KRNL.
#define VBE_MAX_CHARS 127 - UNUSABLE_CHARS
#define VBE_CHAR_HEIGHT 8
#define VBE_CHAR_WIDTH 8
typedef U8 VBE_LETTERS_TYPE;

/*+++
BOOLEAN VBE_DRAW_CHARACTER(U32 x, U32 y, U8 c, VBE_PIXEL_COLOUR fg, VBE_PIXEL_COLOUR bg)

DESCRIPTION
    Draws a character on the framebuffer.

PARAMETERS
    U32    x
        The x-coordinate to draw the character.
    U32    y
        The y-coordinate to draw the character.
    U8     c
        The ASCII code of the character to draw.
    VBE_PIXEL_COLOUR fg
        The foreground color.
    VBE_PIXEL_COLOUR bg
        The background color.

RETURN
    TRUE if successful, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/27 - Antonako1
        Initial version.

REMARKS
    This draws a character in the framebuffer.
---*/
BOOLEAN VBE_DRAW_CHARACTER(U32 x, U32 y, U8 c, VBE_PIXEL_COLOUR fg, VBE_PIXEL_COLOUR bg);

/*+++
BOOLEAN VBE_DRAW_STRING(U32 x, U32 y, const U8* str, VBE_PIXEL_COLOUR fg, VBE_PIXEL_COLOUR bg)

DESCRIPTION
    Draws a string on the framebuffer.

PARAMETERS
    U32    x
        The x-coordinate to draw the string.
    U32    y
        The y-coordinate to draw the string.
    const char* str
        The string to draw.
    VBE_PIXEL_COLOUR fg
        The foreground color.
    VBE_PIXEL_COLOUR bg
        The background color.

RETURN
    TRUE if successful, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/27 - Antonako1
        Initial version.

REMARKS
    This draws a string in the framebuffer.
---*/
BOOLEAN VBE_DRAW_STRING(U32 x, U32 y, const char* str, VBE_PIXEL_COLOUR fg, VBE_PIXEL_COLOUR bg);

/*+++
U0 VBE_UPDATE_VRAM(U0);

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
U0 VBE_UPDATE_VRAM(U0);


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
BOOLEAN VBE_CLEAR_SCREEN(VBE_PIXEL_COLOUR colour)

DESCRIPTION
    Clears the screen and fills it with the specified color.

PARAMETERS
    VBE_PIXEL_COLOUR colour
        The color to fill the screen with.

RETURN
    TRUE if successful, FALSE otherwise.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/19 - Antonako1
        Initial version.

REMARKS
    This function is called to refresh the screen contents.
    Requires framebuffer refresh
---*/
BOOLEAN VBE_CLEAR_SCREEN(VBE_PIXEL_COLOUR colour);

/*+++
BOOLEAN VBE_FLUSH_SCREEN(U0);

DESCRIPTION
    Flushes the framebuffer to the screen.

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
    This function is called to clear screen with black
    Refreshes the framebuffer itself.
---*/
BOOLEAN VBE_FLUSH_SCREEN(U0);

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
    2025/08/27 - Antonako1
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
    2025/08/27 - Antonako1
        Initial version.

REMARKS
    This draws a filled triangle in the framebuffer.
---*/
BOOLEAN VBE_DRAW_TRIANGLE_FILLED(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_PIXEL_COLOUR colours);
#endif // __RTOS__ || KERNEL_ENTRY
#endif