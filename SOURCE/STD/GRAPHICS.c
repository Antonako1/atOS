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
    U32 len = STRLEN(str);
    if (len == 0) return;

    /* Fast path: use a stack buffer to avoid heap alloc+free per call.
       Covers virtually all UI strings (ATGL_NODE_MAX_TEXT = 128).      */
    CHAR stack_buf[256];
    PU8 m_str;
    BOOL on_heap = FALSE;

    if (len < sizeof(stack_buf)) {
        MEMCPY_OPT(stack_buf, str, len + 1);
        m_str = (PU8)stack_buf;
    } else {
        m_str = MAlloc(len + 1);
        MEMCPY_OPT(m_str, str, len + 1);
        on_heap = TRUE;
    }

    SYSCALL(SYSCALL_VBE_DRAW_STRING, (U32)x, (U32)y, (U32)m_str, (U32)fg, (U32)bg);

    if (on_heap) MFree(m_str);
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
    SYSCALL(SYSCALL_VBE_DRAW_FILLED_RECTANGLE, (U32)x, (U32)y, (U32)width, (U32)height, (U32)colour);
}
BOOLEAN DRAW_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour) {
    SYSCALL(SYSCALL_VBE_DRAW_LINE, (U32)x1, (U32)y1, (U32)x2, (U32)y2, (U32)colour);
    SYSCALL(SYSCALL_VBE_DRAW_LINE, (U32)x2, (U32)y2, (U32)x3, (U32)y3, (U32)colour);
    SYSCALL(SYSCALL_VBE_DRAW_LINE, (U32)x3, (U32)y3, (U32)x1, (U32)y1, (U32)colour);
}
BOOLEAN DRAW_FILLED_TRIANGLE(U32 x1, U32 y1, U32 x2, U32 y2, U32 x3, U32 y3, VBE_COLOUR colour) {
    /* Scanline rasterisation: O(h) DRAW_LINE syscalls instead of
       O(w×h) per-pixel DRAW_PIXEL syscalls. */

    /* Sort vertices by Y (v0.y <= v1.y <= v2.y) using I32 for safety */
    I32 vx0 = (I32)x1, vy0 = (I32)y1;
    I32 vx1 = (I32)x2, vy1 = (I32)y2;
    I32 vx2 = (I32)x3, vy2 = (I32)y3;

    /* Bubble sort the three vertices by y */
    if (vy0 > vy1) { I32 t; t = vx0; vx0 = vx1; vx1 = t; t = vy0; vy0 = vy1; vy1 = t; }
    if (vy1 > vy2) { I32 t; t = vx1; vx1 = vx2; vx2 = t; t = vy1; vy1 = vy2; vy2 = t; }
    if (vy0 > vy1) { I32 t; t = vx0; vx0 = vx1; vx1 = t; t = vy0; vy0 = vy1; vy1 = t; }

    if (vy0 == vy2) {
        /* Degenerate: all on one scanline */
        I32 lo = vx0, hi = vx0;
        if (vx1 < lo) lo = vx1; if (vx1 > hi) hi = vx1;
        if (vx2 < lo) lo = vx2; if (vx2 > hi) hi = vx2;
        if (lo < 0) lo = 0;
        DRAW_LINE((U32)lo, (U32)vy0, (U32)hi, (U32)vy0, colour);
        return TRUE;
    }

    BOOLEAN any = FALSE;

    /* Upper half: vy0 → vy1 */
    I32 dy_long = vy2 - vy0;
    I32 dy_short = vy1 - vy0;

    for (I32 y = vy0; y <= vy1; y++) {
        if (y < 0) continue;
        if ((U32)y >= SCREEN_HEIGHT) break;

        /* Interpolate x along long edge (v0→v2) and short edge (v0→v1) */
        I32 xa = (dy_long > 0) ? vx0 + (vx2 - vx0) * (y - vy0) / dy_long : vx0;
        I32 xb = (dy_short > 0) ? vx0 + (vx1 - vx0) * (y - vy0) / dy_short : vx1;

        if (xa > xb) { I32 t = xa; xa = xb; xb = t; }

        /* Clip to screen */
        if (xa < 0) xa = 0;
        if ((U32)xb >= SCREEN_WIDTH) xb = (I32)SCREEN_WIDTH - 1;
        if (xa <= xb) {
            DRAW_LINE((U32)xa, (U32)y, (U32)xb, (U32)y, colour);
            any = TRUE;
        }
    }

    /* Lower half: vy1 → vy2 */
    I32 dy_short2 = vy2 - vy1;

    for (I32 y = vy1 + 1; y <= vy2; y++) {
        if (y < 0) continue;
        if ((U32)y >= SCREEN_HEIGHT) break;

        I32 xa = (dy_long > 0) ? vx0 + (vx2 - vx0) * (y - vy0) / dy_long : vx2;
        I32 xb = (dy_short2 > 0) ? vx1 + (vx2 - vx1) * (y - vy1) / dy_short2 : vx2;

        if (xa > xb) { I32 t = xa; xa = xb; xb = t; }

        if (xa < 0) xa = 0;
        if ((U32)xb >= SCREEN_WIDTH) xb = (I32)SCREEN_WIDTH - 1;
        if (xa <= xb) {
            DRAW_LINE((U32)xa, (U32)y, (U32)xb, (U32)y, colour);
            any = TRUE;
        }
    }

    return any;
}

BOOLEAN DRAW_FILLED_ELLIPSE(U32 x, U32 y, U32 rx, U32 ry, VBE_COLOUR colour) {
    SYSCALL(SYSCALL_VBE_DRAW_FILLED_ELLIPSE, (U32)x, (U32)y, (U32)rx, (U32)ry, (U32)colour);
}
