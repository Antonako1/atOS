/*+++
    SOURCE/STD/GRAPHICS.h - VBE level graphical drawing

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <STD/TYPEDEF.h>
#include <DRIVERS/VIDEO/VBE.h>
void FLUSH_VRAM(VOID);
BOOLEAN DRAW_8x8_CHARACTER(U32 x, U32 y, U8 ch, VBE_COLOUR fg, VBE_COLOUR bg);
BOOLEAN DRAW_8x8_STRING(U32 x, U32 y, U8 *str, VBE_COLOUR fg, VBE_COLOUR bg);
void CLEAR_SCREEN_COLOUR(VBE_COLOUR colour);
BOOLEAN DRAW_PIXEL(VBE_PIXEL_INFO info);
BOOLEAN DRAW_FRAMEBUFFER(U32 pos, VBE_COLOUR colour);
BOOLEAN DRAW_ELLIPSE(U32 x, U32 y, U32 rx, U32 ry, VBE_COLOUR colour);
BOOLEAN DRAW_LINE(U32 x1, U32 y1, U32 x2, U32 y2, VBE_COLOUR colour);
BOOLEAN DRAW_RECTANGLE(U32 x, U32 y, U32 width, U32 height, VBE_COLOUR colour);
BOOLEAN DRAW_FILLED_RECTANGLE(U32 x, U32 y, U32 width, U32 height, VBE_COLOUR colour);
BOOLEAN DRAW_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour);
BOOLEAN DRAW_FILLED_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour);
#endif