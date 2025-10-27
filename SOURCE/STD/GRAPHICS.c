#include <STD/GRAPHICS.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <STD/MEM.h>

void FLUSH_VRAM(VOID) {
    SYSCALL(SYSCALL_VBE_UPDATE_VRAM, 0, 0, 0, 0, 0);
}

BOOLEAN DRAW_8x8_CHARACTER(U32 x, U32 y, U8 ch, VBE_COLOUR fg, VBE_COLOUR bg) {
    SYSCALL(SYSCALL_VBE_DRAW_CHARACTER, (U32)x, (U32)y, (U32)ch, (U32)fg, (U32)bg);
}


BOOLEAN DRAW_8x8_STRING(U32 x, U32 y, U8 *str, VBE_COLOUR fg, VBE_COLOUR bg) {
    if (!str) return;
    PU8 m_str = MAlloc(STRLEN(str) + 1);
    MEMCPY(m_str, str, STRLEN(str) + 1);
    SYSCALL(SYSCALL_VBE_DRAW_STRING, (U32)x, (U32)y, (U32)m_str, (U32)fg, (U32)bg);
    Free(m_str);
}
void CLEAR_SCREEN_COLOUR(VBE_COLOUR colour) {
    SYSCALL(SYSCALL_VBE_CLEAR_SCREEN, (U32)colour, 0, 0, 0, 0);
}
BOOLEAN DRAW_PIXEL(VBE_PIXEL_INFO info) {
    SYSCALL(SYSCALL_VBE_DRAW_PIXEL, (U32)info.X, (U32)info.Y, (U32)info.Colour, 0, 0);
}
BOOLEAN DRAW_FRAMEBUFFER(U32 pos, VBE_COLOUR colour) {
    SYSCALL(SYSCALL_VBE_DRAW_FRAMEBUFFER, (U32)pos, (U32)colour, 0, 0, 0);
}
BOOLEAN DRAW_ELLIPSE(U32 x, U32 y, U32 rx, U32 ry, VBE_COLOUR colour) {
    SYSCALL(SYSCALL_VBE_DRAW_ELLIPSE, (U32)x, (U32)y, (U32)rx, (U32)ry, (U32)colour);
}
BOOLEAN DRAW_LINE(U32 x1, U32 y1, U32 x2, U32 y2, VBE_COLOUR colour) {
    SYSCALL(SYSCALL_VBE_DRAW_LINE, (U32)x1, (U32)y1, (U32)x2, (U32)y2, (U32)colour);
}
BOOLEAN DRAW_RECTANGLE(U32 x, U32 y, U32 width, U32 height, VBE_COLOUR colour) {
    SYSCALL(SYSCALL_VBE_DRAW_RECTANGLE, (U32)x, (U32)y, (U32)width, (U32)height, (U32)colour);
}
BOOLEAN DRAW_FILLED_RECTANGLE(U32 x, U32 y, U32 width, U32 height, VBE_COLOUR colour) {
    for (U32 i = 0; i < height; i++) {
        DRAW_LINE(x, y + i, x + width - 1, y + i, colour);
    }
}
BOOLEAN DRAW_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour) {
    SYSCALL(SYSCALL_VBE_DRAW_LINE, (U32)x1, (U32)y1, (U32)x2, (U32)y2, (U32)colour);
    SYSCALL(SYSCALL_VBE_DRAW_LINE, (U32)x2, (U32)y2, (U32)x3, (U32)y3, (U32)colour);
    SYSCALL(SYSCALL_VBE_DRAW_LINE, (U32)x3, (U32)y3, (U32)x1, (U32)y1, (U32)colour);
}
BOOLEAN DRAW_FILLED_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour) {
    U32 minx = x1;
    if (x2 < minx) minx = x2;
    if (x3 < minx) minx = x3;

    U32 maxx = x1;
    if (x2 > maxx) maxx = x2;
    if (x3 > maxx) maxx = x3;

    U32 miny = y1;
    if (y2 < miny) miny = y2;
    if (y3 < miny) miny = y3;

    U32 maxy = y1;
    if (y2 > maxy) maxy = y2;
    if (y3 > maxy) maxy = y3;

    /* Quick reject if bbox completely off-screen */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    if (maxx < 0 || maxy < 0) return FALSE; /* defensive, though U32 can't be <0 */
#pragma GCC diagnostic pop
    if (minx >= SCREEN_WIDTH || miny >= SCREEN_HEIGHT) return FALSE;
    /* Clip bounding box to screen */
    if (minx >= SCREEN_WIDTH) return FALSE;
    if (miny >= SCREEN_HEIGHT) return FALSE;

    U32 bx0 = minx;
    U32 by0 = miny;
    U32 bx1 = maxx;
    U32 by1 = maxy;

    if (bx0 >= SCREEN_WIDTH) bx0 = SCREEN_WIDTH - 1;
    if (by0 >= SCREEN_HEIGHT) by0 = SCREEN_HEIGHT - 1;
    if (bx1 >= SCREEN_WIDTH) bx1 = SCREEN_WIDTH - 1;
    if (by1 >= SCREEN_HEIGHT) by1 = SCREEN_HEIGHT - 1;

    /* Use integer edge functions (64-bit to avoid overflow) */
    const I32 ax = (I32)x1;
    const I32 ay = (I32)y1;
    const I32 bx = (I32)x2;
    const I32 by = (I32)y2;
    const I32 cx = (I32)x3;
    const I32 cy = (I32)y3;

    VBE_PIXEL_INFO p;
    p.Colour = colour;
    BOOLEAN any = FALSE;

    for (U32 yy = bx0 /* dummy init */; yy <= bx1; ++yy) { /* we will overwrite loops below */
        break;
    }
    /* iterate y then x within clipped bbox */
    for (U32 yy = by0; yy <= by1; ++yy) {
        for (U32 xx = bx0; xx <= bx1; ++xx) {
            /* compute edge functions relative to point (xx, yy) */
            I32 px = (I32)xx;
            I32 py = (I32)yy;

            I32 e0 = (bx - ax) * (py - ay) - (by - ay) * (px - ax); /* edge AB */
            I32 e1 = (cx - bx) * (py - by) - (cy - by) * (px - bx); /* edge BC */
            I32 e2 = (ax - cx) * (py - cy) - (ay - cy) * (px - cx); /* edge CA */

            /* point is inside if all edge functions have same sign (or zero) */
            if ((e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0)) {
                p.X = xx;
                p.Y = yy;
                if (DRAW_PIXEL(p)) any = TRUE;
            }
        }
    }

    return any;
}