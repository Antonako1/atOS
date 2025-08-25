#include "./VBE.h"
#include "../../../../STD/MATH.h"
#define EMPTY_CHAR \
    (U8)0b11111000, \
    (U8)0b11111111, \
    (U8)0b00000111, \
    (U8)0b00001100, \
    (U8)0b00011000, \
    (U8)0b00111111, \
    (U8)0b11111111, \
    (U8)0b00000000
#define CHARACTERIZE(x) (U8)x

U8 VBE_LETTERS_8x8[VBE_MAX_CHARS][VBE_FONT_HEIGHT_8x8] = {
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 0-3
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 4-7
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 8-11
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 12-15
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 16-19
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 20-23
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 24-27
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 28-31
    {EMPTY_CHAR}, // Spacebar               (   ), 32
    {EMPTY_CHAR}, // Exclamation mark       ( ! ), 33
    {EMPTY_CHAR}, // Quotation mark         ( " ), 34
    {EMPTY_CHAR}, // Number sign            ( # ), 35
    {EMPTY_CHAR}, // Dollar sign            ( $ ), 36
    {EMPTY_CHAR}, // Percent sign           ( % ), 37
    {EMPTY_CHAR}, // Ampersand              ( & ), 38
    {EMPTY_CHAR}, // Apostrophe             ( ' ), 39
    {EMPTY_CHAR}, // Left parenthesis       ( ( ), 40
    {EMPTY_CHAR}, // Right parenthesis      ( ) ), 41
    {EMPTY_CHAR}, // Asterisk               ( * ), 42
    {EMPTY_CHAR}, // Plus                   ( + ), 43
    {EMPTY_CHAR}, // Comma                  ( , ), 44
    {EMPTY_CHAR}, // Hyphen                 ( - ), 45
    {EMPTY_CHAR}, // Period                 ( . ), 46
    {EMPTY_CHAR}, // Slash                  ( / ), 47
    {EMPTY_CHAR}, // Zero                   ( 0 ), 48
    {EMPTY_CHAR}, // One                    ( 1 ), 49
    {EMPTY_CHAR}, // Two                    ( 2 ), 50
    {EMPTY_CHAR}, // Three                  ( 3 ), 51
    {EMPTY_CHAR}, // Four                   ( 4 ), 52
    {EMPTY_CHAR}, // Five                   ( 5 ), 53
    {EMPTY_CHAR}, // Six                    ( 6 ), 54
    {EMPTY_CHAR}, // Seven                  ( 7 ), 55
    {EMPTY_CHAR}, // Eight                  ( 8 ), 56
    {EMPTY_CHAR}, // Nine                   ( 9 ), 57
    {EMPTY_CHAR}, // Colon                  ( : ), 58
    {EMPTY_CHAR}, // Semicolon              ( ; ), 59
    {EMPTY_CHAR}, // Less than              ( < ), 60
    {EMPTY_CHAR}, // Equal sign             ( = ), 61
    {EMPTY_CHAR}, // Greater than           ( > ), 62
    {EMPTY_CHAR}, // Question mark          ( ? ), 63
    {EMPTY_CHAR}, // At sign                ( @ ), 64
    {
    (U8)0b00111100,
    (U8)0b01100110,
    (U8)0b11000011,
    (U8)0b11000011,
    (U8)0b11111111,
    (U8)0b11000011,
    (U8)0b11000011,
    (U8)0b00000000,
    }, // A                       ( A ), 65
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
};

BOOLEAN VBE_DRAW_CHARACTER_8x8(U32 x, U32 y, CHAR c, VBE_PIXEL_COLOUR fg, VBE_PIXEL_COLOUR bg) {
    // VBE_DRAW_ELLIPSE(x + 4, y + 4, 10, 10, fg);
    // VBE_DRAW_ELLIPSE(x + 4, y + 4, 4, 4, bg);
    if (x + VBE_FONT_WIDTH_8x8 > SCREEN_WIDTH || y + VBE_FONT_HEIGHT_8x8 > SCREEN_HEIGHT){
        return FALSE;
    }
    if (c >= VBE_MAX_CHARS){
        return FALSE;
    }
    for (U32 row = 0; row < VBE_FONT_HEIGHT_8x8; row++) {
        U8 row_data = VBE_LETTERS_8x8[c][row];
        for (U32 col = 0; col < VBE_FONT_WIDTH_8x8; col++) {
            VBE_PIXEL_COLOUR colour = bg;
            if ((row_data & (1 << (7 - col))) != 0) colour = fg;
            VBE_DRAW_PIXEL(CREATE_VBE_PIXEL_INFO(x + col, y + row, colour));
        }
    }
    return TRUE;
}


BOOL vbe_check(U0) {
    VBE_MODE* mode = (VBE_MODE*)(VBE_MODE_LOAD_ADDRESS_PHYS);
    // Check if the mode is valid
    if (mode->ModeAttributes == 0) {
        return FALSE;
    }
    // Check if the physical base pointer is valid
    if (mode->PhysBasePtr == 0) {
        return FALSE;
    }
    // Check if the mode is compatible with the screen size
    if (mode->XResolution < SCREEN_WIDTH || mode->YResolution < SCREEN_HEIGHT) {
        return FALSE;
    }
    return TRUE;
}


U0 ___memcpy(void* dest, const void* src, U32 n) {
    U8* d = (U8*)dest;
    const U8* s = (const U8*)src;
    for (U32 i = 0; i < n; i++) {
        d[i] = s[i];
    }
}


U0 UPDATE_VRAM(U0) {
    VBE_MODE* mode = GET_VBE_MODE();
    if (!mode) return;

    // Copy only the required framebuffer bytes
    U32 copy_size = mode->BytesPerScanLineLinear * mode->YResolution;
    if (copy_size > FRAMEBUFFER_SIZE) copy_size = FRAMEBUFFER_SIZE;
    ___memcpy((void*)mode->PhysBasePtr, (void*)FRAMEBUFFER_ADDRESS, copy_size);
}

U0 VBE_STOP_DRAWING(U0) {
    UPDATE_VRAM();
}

BOOLEAN VBE_DRAW_FRAMEBUFFER(U32 pos, VBE_PIXEL_COLOUR colour) {
    VBE_MODE* mode = (VBE_MODE*)(VBE_MODE_LOAD_ADDRESS_PHYS);
    U8* framebuffer = (U8*)(FRAMEBUFFER_ADDRESS);

    if (!framebuffer || !mode) return FALSE;

    U32 bytes_per_pixel = (mode->BitsPerPixel + 7) / 8;
    if (pos + bytes_per_pixel > mode->BytesPerScanLineLinear * mode->YResolution) return FALSE;
    if (pos + bytes_per_pixel > FRAMEBUFFER_SIZE) return FALSE; // RAM framebuffer bounds
    switch (bytes_per_pixel) {
        case 1:
            // Contrast blue hues
            framebuffer[pos] = (U8)(colour  & 0xFF);
            break;

        case 2: {
            U16 val16 = (U16)colour;
            framebuffer[pos + 0] = (U8)(val16 & 0xFF);
            framebuffer[pos + 1] = (U8)((val16 >> 8) & 0xFF);
            break;
        }

        case 3:
            framebuffer[pos + 0] = (U8)(colour & 0xFF);
            framebuffer[pos + 1] = (U8)((colour >> 8) & 0xFF);
            framebuffer[pos + 2] = (U8)((colour >> 16) & 0xFF);
            break;

        case 4:
            framebuffer[pos + 0] = (U8)(colour & 0xFF);
            framebuffer[pos + 1] = (U8)((colour >> 8) & 0xFF);
            framebuffer[pos + 2] = (U8)((colour >> 16) & 0xFF);
            framebuffer[pos + 3] = (U8)((colour >> 24) & 0xFF);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

BOOLEAN VBE_DRAW_PIXEL(VBE_PIXEL_INFO pixel_info) {
    VBE_MODE* mode = (VBE_MODE*)(VBE_MODE_LOAD_ADDRESS_PHYS);
    if (!mode) return FALSE;
    if(pixel_info.Colour == VBE_SEE_THROUGH) return TRUE;

    if ((I32)pixel_info.X < 0 || (I32)pixel_info.Y < 0) return FALSE;
    if ((U32)pixel_info.X >= SCREEN_WIDTH || (U32)pixel_info.Y >= SCREEN_HEIGHT) return FALSE;

    U32 bytes_per_pixel = (mode->BitsPerPixel + 7) / 8;
    U32 pos = (pixel_info.Y * mode->BytesPerScanLineLinear) + (pixel_info.X * bytes_per_pixel);

    return VBE_DRAW_FRAMEBUFFER(pos, pixel_info.Colour);
}



BOOLEAN VBE_DRAW_ELLIPSE(U32 x0, U32 y0, U32 a, U32 b, VBE_PIXEL_COLOUR fill_colours) {
    I32 x = 0;
    I32 y = b;
    U32 a2 = a*a;
    U32 b2 = b*b;
    I32 dx = 0;
    I32 dy = 2*a2*y;
    I32 d1 = b2 - a2*b + a2/4;

    while(dx < dy) {
        for(I32 px = -x; px <= x; px++) {
            VBE_DRAW_PIXEL(CREATE_VBE_PIXEL_INFO(x0 + px, y0 + y, fill_colours));
            VBE_DRAW_PIXEL(CREATE_VBE_PIXEL_INFO(x0 + px, y0 - y, fill_colours));
        }
        x++;
        dx += 2*b2;
        if(d1 < 0) {
            d1 += b2 + dx;
        } else {
            y--;
            dy -= 2*a2;
            d1 += b2 + dx - dy;
        }
    }

    I32 d2 = b2*(x*x + x) + a2*(y-1)*(y-1) - a2*b2;
    while(y >= 0) {
        for(I32 px = -x; px <= x; px++) {
            VBE_DRAW_PIXEL(CREATE_VBE_PIXEL_INFO(x0 + px, y0 + y, fill_colours));
            VBE_DRAW_PIXEL(CREATE_VBE_PIXEL_INFO(x0 + px, y0 - y, fill_colours));
        }
        y--;
        dy -= 2*a2;
        if(d2 > 0) {
            d2 += a2 - dy;
        } else {
            x++;
            dx += 2*b2;
            d2 += a2 - dy + dx;
        }
    }

    return TRUE;
}

BOOLEAN VBE_DRAW_LINE(U32 x0_in, U32 y0_in, U32 x1_in, U32 y1_in, VBE_PIXEL_COLOUR colour) {
    I32 x0 = (I32)x0_in;
    I32 y0 = (I32)y0_in;
    I32 x1 = (I32)x1_in;
    I32 y1 = (I32)y1_in;

    I32 dx = abs(x1 - x0);
    I32 sx = (x0 < x1) ? 1 : -1;
    I32 dy = -abs(y1 - y0);
    I32 sy = (y0 < y1) ? 1 : -1;
    I32 err = dx + dy;  // error term

    // safety: upper-bound on iterations to avoid infinite loops while debugging
    I32 max_iters = dx + abs(dy) + 4;
    if (max_iters < 0) max_iters = FRAMEBUFFER_SIZE; // paranoid fallback. Loops should not exceed framebuffer size
    I32 iters = 0;

    while (1) {
        // draw only when inside the screen bounds (VBE_DRAW_PIXEL already checks, but
        // avoiding the call can be a tiny speedup)
        if ((U32)x0 < SCREEN_WIDTH && (U32)y0 < SCREEN_HEIGHT) {
            VBE_DRAW_PIXEL(CREATE_VBE_PIXEL_INFO((U32)x0, (U32)y0, colour));
        }

        if (x0 == x1 && y0 == y1) break;

        if (++iters > max_iters) {
            // something went wrong — bail out to avoid hang
            return FALSE;
        }

        I32 e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }

    return TRUE;
}






BOOLEAN VBE_DRAW_RECTANGLE(U32 x, U32 y, U32 width, U32 height, VBE_PIXEL_COLOUR colours) {
    if (width == 0 || height == 0) {
        return TRUE;
    }

    U32 errcnt = 0;

    if(!VBE_DRAW_LINE(x, y, width, y, colours)) errcnt++;
    if(!VBE_DRAW_LINE(width, y, width, height, colours)) errcnt++;
    if(!VBE_DRAW_LINE(width, height, x, height, colours)) errcnt++;
    if(!VBE_DRAW_LINE(x, height, x, y, colours)) errcnt++;

    return errcnt == 0;
}
BOOLEAN VBE_DRAW_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_PIXEL_COLOUR colours) {
    VBE_DRAW_LINE(x1, y1, x2, y2, colours);
    VBE_DRAW_LINE(x2, y2, x3, y3, colours);
    VBE_DRAW_LINE(x3, y3, x1, y1, colours);
    return TRUE;
}


BOOLEAN VBE_DRAW_TRIANGLE_FILLED(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_PIXEL_COLOUR colours) {
    (void)x1; (void)y1; (void)x2; (void)y2; (void)x3; (void)y3; (void)colours;
    return TRUE;
}

BOOLEAN VBE_DRAW_RECTANGLE_FILLED(U32 x, U32 y, U32 width, U32 height, VBE_PIXEL_COLOUR colours) {
    if (width == 0 || height == 0) {
        return TRUE;
    }

    for (U32 row = y; row < y + height; row++) {
        for (U32 col = x; col < x + width; col++) {
            VBE_DRAW_PIXEL(CREATE_VBE_PIXEL_INFO(col, row, colours));
        }
    }

    return TRUE;
}


/* Saved for later:
Bugs and draws out line patterns
BOOLEAN VBE_DRAW_LINE(U32 x1, U32 y1, U32 x2, U32 y2, VBE_PIXEL_COLOUR colours, U32 thickness) {
    I32 t_2 = (I32)thickness / 2;
    I32 A = (I32)y2 - (I32)y1;
    I32 B = (I32)x1 - (I32)x2;
    I32 C = (I32)x2 * (I32)y1 - (I32)x1 * (I32)y2;
    I32 A2 = A * A;
    I32 B2 = B * B;

    I32 xmin = (I32)x1 < (I32)x2 ? (I32)x1 : (I32)x2;
    I32 xmax = (I32)x1 > (I32)x2 ? (I32)x1 : (I32)x2;
    I32 ymin = (I32)y1 < (I32)y2 ? (I32)y1 : (I32)y2;
    I32 ymax = (I32)y1 > (I32)y2 ? (I32)y1 : (I32)y2;

    xmin -= t_2; xmax += t_2;
    ymin -= t_2; ymax += t_2;

    I32 threshold2 = pow2(t_2) * (A2 + B2);
    VBE_MODE* mode = GET_VBE_MODE();
    for(I32 x = xmin; x <= xmax; x++) {
        for(I32 y = ymin; y <= ymax; y++) {
            if(x < 0 || y < 0 || x >= mode->XResolution || y >= mode->YResolution) continue;
            I32 d = A * x + B * y + C;
            if (d*d <= threshold2) {
                VBE_DRAW_PIXEL(CREATE_VBE_PIXEL_INFO(x, y, colours));
            }
        }
    }
    return TRUE;
}
*/
