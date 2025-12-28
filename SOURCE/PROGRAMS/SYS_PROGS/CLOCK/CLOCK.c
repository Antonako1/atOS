#include <STD/TYPEDEF.h>
#include <STD/GRAPHICS.h>
#include <STD/IO.h>
#include <STD/DEBUG.h>
#include <STD/STRING.h>
#include <STD/TIME.h>
#include <STD/MATH.h>
#include <LIBRARIES/ARGHAND/ARGHAND.h>

/* Helper to convert polar coordinates to screen lines */
/* Angle is in Radians. 0 is 3 o'clock. */
static VOID draw_hand(I32 cx, I32 cy, F32 angle, I32 length, U32 color, U32 thickness) {
    I32 x_end = cx + (I32)(cosf(angle) * (F32)length);
    I32 y_end = cy + (I32)(sinf(angle) * (F32)length);
    DRAW_LINE(cx, cy, x_end, y_end, color);
}

static U32 prev_sec ATTRIB_DATA = 99; // Force initial draw

VOID DRAW_CLOCK(RTC_DATE_TIME *ts) {
    CLEAR_SCREEN_COLOUR(VBE_NOTEPAD_PAPER2);

    I32 cx = SCREEN_CENTER_X;
    I32 cy = SCREEN_CENTER_Y;
    I32 radius = 350;

    // 1. Draw Clock Face
    // Outer rim (Black)
    DRAW_ELLIPSE(cx, cy, radius + 5, radius + 5, VBE_BLACK);    
    // Inner face (White/Paper)
    DRAW_ELLIPSE(cx, cy, radius, radius, VBE_WHITE);

    /* ============================
       Hour + Second tick markers
       ============================ */
    
    // We start at -PI/2 (12 o'clock)
    for (U32 i = 0; i < 60; i++) {
        // Calculate angle: i / 60 * 2PI
        F32 fraction = (F32)i / 60.0f;
        F32 theta = (fraction * TAU) - HALF_PI;

        I32 r_outer = radius;
        I32 r_inner;
        U32 thickness; // wip until atgl

        // Hour markers (0, 5, 10...) are longer and thicker
        if (i % 5 == 0) {
            r_inner = radius - 30;
            thickness = 3;
        } else {
            r_inner = radius - 10;
            thickness = 1;
        }

        I32 x1 = cx + (I32)(cosf(theta) * (F32)r_inner);
        I32 y1 = cy + (I32)(sinf(theta) * (F32)r_inner);
        I32 x2 = cx + (I32)(cosf(theta) * (F32)r_outer);
        I32 y2 = cy + (I32)(sinf(theta) * (F32)r_outer);

        DRAW_LINE(x1, y1, x2, y2, VBE_BLACK);
    }

    /* ============================
       Draw Hands
       ============================ */

    // Calculate fractions for time
    F32 sec_f = (F32)ts->seconds;
    F32 min_f = (F32)ts->minutes + (sec_f / 60.0f);
    F32 hr_f  = (F32)(ts->hours % 12) + (min_f / 60.0f);

    // Convert to angles (Angle = Fraction * 2PI - PI/2)
    // -HALF_PI rotates 0 from 3 o'clock to 12 o'clock
    F32 ang_sec = (sec_f / 60.0f) * TAU - HALF_PI;
    F32 ang_min = (min_f / 60.0f) * TAU - HALF_PI;
    F32 ang_hr  = (hr_f  / 12.0f) * TAU - HALF_PI;

    // Draw Hour Hand (Short, Thick)
    draw_hand(cx, cy, ang_hr, (I32)(radius * 0.5f), VBE_BLACK, 6);

    // Draw Minute Hand (Long, Medium)
    draw_hand(cx, cy, ang_min, (I32)(radius * 0.8f), VBE_BLACK, 4);

    // Draw Second Hand (Long, Thin, Red usually, but using Blue per your style)
    draw_hand(cx, cy, ang_sec, (I32)(radius * 0.9f), VBE_BLUE, 2);

    // Center Cap
    DRAW_ELLIPSE(cx, cy, 8, 8, VBE_BLUE);

    U32 row = 10;
    DRAW_8x8_STRING(10,row,"Press ESC to exit.", VBE_BLACK, VBE_SEE_THROUGH);
    row += 8+2;
    DRAW_8x8_STRING(10,row,"Time shown as GMT+0.", VBE_BLACK, VBE_SEE_THROUGH);
    row += 8+2;
    U8 buf[21];
    FORMATTED_DATE_TIME_STRING(buf, ts);
    DRAW_8x8_STRING(10, row, buf, VBE_BLACK, VBE_SEE_THROUGH);
}

VOID UPDATE_CLOCK() {
    RTC_DATE_TIME ts = GET_DATE_TIME();
    if (ts.seconds != prev_sec) {
        prev_sec = ts.seconds;
        DRAW_CLOCK(&ts);
    }
}

U32 main(U32 argc, PPU8 argv) {
    PU8 help[] = ARGHAND_ARG("-h", "--help");
    PPU8 allAliases[] = ARGHAND_ALL_ALIASES(help);
    U32 aliasCounts[] = { ARGHAND_ALIAS_COUNT(help) };
    
    ARGHANDLER ah;
    ARGHAND_INIT(&ah, argc, argv, allAliases, aliasCounts, 1);

    if (ARGHAND_IS_PRESENT(&ah, "-h")) {
        ARGHAND_FREE(&ah);
        printf("Usage: Press ESC to exit\n");
        return 0;
    }
    ARGHAND_FREE(&ah);

    // Initialize previous second to something impossible so it draws immediately
    prev_sec = 99; 
    
    BOOL8 running = TRUE;
    while (running) {
        PS2_KB_MOUSE_DATA* kp = get_latest_keypress();
        if (valid_kp(kp)) {
            if (kp->kb.cur.pressed) {
                if (kp->kb.cur.keycode == KEY_ESC) {
                    running = FALSE;
                }
            }
            update_kp_seq(kp);
        }
        UPDATE_CLOCK();
        cpu_relax();
        // Optional: Yield CPU slightly to prevent 100% usage while waiting for second to tick
        // sys_yield(); 
    }

    return 0;
}