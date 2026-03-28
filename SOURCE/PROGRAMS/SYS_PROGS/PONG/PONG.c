#include <STD/TYPEDEF.h>
#include <LIBRARIES/ATUI/ATUI.h>
#include <LIBRARIES/ATUI/ATUI_WIDGETS.h>
#include <DRIVERS/PS2/KEYBOARD_MOUSE.h>
#include <STD/IO.h>
#include <STD/PROC_COM.h>


CMAIN() {
    ATUI_INITIALIZE(NULLPTR, ATUIC_RAW); 
    ATUI_CURSOR_SET(FALSE);
    ATUI_SET_COLOR(VBE_LIME, VBE_BLACK);
    DISABLE_SHELL_KEYBOARD();
    BOOL8 singleplayer = ATUI_MSGBOX_YESNO("Single Player?", "Do you want to play against the AI? (Yes for singleplayer, No for 2-player)");
    ATUI_DISPLAY *d = GET_DISPLAY();
    U32 screen_w = d->cols;
    U32 screen_h = d->rows;

    // Sidebar config
    U32 sidebar_w = 22;
    U32 play_w    = screen_w - sidebar_w; // The actual game boundary

    // Game Variables
    I32 ball_x = play_w / 2, ball_y = screen_h / 2;
    I32 ball_dx = 3, ball_dy = 2;
    I32 paddle_l = screen_h / 2 - 2, paddle_r = screen_h / 2 - 2;
    I32 paddle_h = 4;
    
    U32 score_l = 0, score_r = 0;

    while(1) {
        // --- 1. Input Handling ---
        PS2_KB_DATA *kp = ATUI_GETCH_NB();
        if (kp) {
            U32 key = kp->cur.keycode;
            
            if (key == KEY_ESC ) {
                ATUI_DESTROY();
                ENABLE_SHELL_KEYBOARD();
                EXIT(0);
                break; 
            }

            // Paddle Controls
            if (key == KEY_W && paddle_l > 0) paddle_l--;
            if (key == KEY_S && paddle_l < (I32)screen_h - paddle_h) paddle_l++;
            if (!singleplayer) {
                if (key == KEY_ARROW_UP && paddle_r > 0) paddle_r--;
                if (key == KEY_ARROW_DOWN && paddle_r < (I32)screen_h - paddle_h) paddle_r++;
            }
        }

        // --- AI for Singleplayer ---
        if (singleplayer) {
            // Predictive AI: simulate ball trajectory to AI paddle x
            if (ball_dx > 0) {
                I32 sim_x = ball_x;
                I32 sim_y = ball_y;
                I32 sim_dx = ball_dx;
                I32 sim_dy = ball_dy;
                I32 target_y = sim_y;

                while (sim_x < (I32)play_w - 2 && sim_x >= 0) {
                    sim_x += sim_dx;
                    sim_y += sim_dy;

                    if (sim_y <= 0) {
                        sim_y = -sim_y;
                        sim_dy = -sim_dy;
                    } else if (sim_y >= (I32)screen_h - 1) {
                        sim_y = (I32)screen_h - 1 - (sim_y - ((I32)screen_h - 1));
                        sim_dy = -sim_dy;
                    }
                }

                target_y = sim_y;

                // Move paddle toward predicted intercept with limited AI speed
                I32 ai_speed = 2; // max paddle moves per frame for AI
                I32 paddle_center = paddle_r + paddle_h / 2;
                I32 move = target_y - paddle_center;

                if (move > 0) {
                    if (move > ai_speed) move = ai_speed;
                    if (paddle_r + move > (I32)screen_h - paddle_h) paddle_r = (I32)screen_h - paddle_h;
                    else paddle_r += move;
                } else if (move < 0) {
                    if (-move > ai_speed) move = -ai_speed;
                    if (paddle_r + move < 0) paddle_r = 0;
                    else paddle_r += move;
                }
            } else {
                // Ball moving away — return paddle toward center
                I32 center = (I32)screen_h / 2 - paddle_h / 2;
                if (paddle_r < center) {
                    paddle_r += 1;
                    if (paddle_r > center) paddle_r = center;
                } else if (paddle_r > center) {
                    paddle_r -= 1;
                    if (paddle_r < center) paddle_r = center;
                }
            }
        }

        // --- 2. Physics ---
        ball_x += ball_dx;
        ball_y += ball_dy;

        // Ceiling/Floor bounce
        if (ball_y <= 0 || ball_y >= (I32)screen_h - 1) ball_dy *= -1;

        // Paddle Collisions (Boundary is now play_w)
        if (ball_x == 1 && (ball_y >= paddle_l && ball_y < paddle_l + paddle_h)) {
            ball_dx *= -1;
            if (ball_dx > 0) ball_dx++; else ball_dx--;
            if (ball_dx > 5) ball_dx = 5; if (ball_dx < -5) ball_dx = -5;
        } 
        else if (ball_x == (I32)play_w - 2 && (ball_y >= paddle_r && ball_y < paddle_r + paddle_h)) {
            ball_dx *= -1;
            if (ball_dx > 0) ball_dx++; else ball_dx--;
            if (ball_dx > 5) ball_dx = 5; if (ball_dx < -5) ball_dx = -5;
        }
        // Scoring
        else if (ball_x <= 0) {
            score_r++;
            ball_x = play_w / 2; ball_dx = 3; // reset to faster speed
        }
        else if (ball_x >= (I32)play_w - 1) {
            score_l++;
            ball_x = play_w / 2; ball_dx = -3; // reset to faster speed
        }

        // --- 3. Rendering ---
        ATUI_CLEAR();
        
        // Draw Sidebar Divider
        for(U32 i = 0; i < screen_h; i++) {
            ATUI_MVADDSTR(i, play_w, "|");
        }

        // Draw Info Frame Text
        ATUI_SET_COLOR(VBE_WHITE, VBE_BLACK);
        ATUI_MVPRINTW(2,  play_w + 2, "ATUI PONG");
        ATUI_MVPRINTW(4,  play_w + 2, "P1: %d", score_l);
        ATUI_MVPRINTW(5,  play_w + 2, "P2: %d", score_r);
        
        ATUI_SET_COLOR(VBE_YELLOW, VBE_BLACK);
        ATUI_MVPRINTW(8,  play_w + 2, "CONTROLS:");
        ATUI_MVPRINTW(10, play_w + 2, "P1: W/S");
        if (singleplayer)
            ATUI_MVPRINTW(11, play_w + 2, "P2: AI");
        else
            ATUI_MVPRINTW(11, play_w + 2, "P2: UP/DN");
        
        ATUI_SET_COLOR(VBE_RED, VBE_BLACK);
        ATUI_MVPRINTW(screen_h - 2, play_w + 2, "ESC EXIT");

        // Draw Game Elements
        ATUI_SET_COLOR(VBE_LIME, VBE_BLACK);
        for(I32 i = 0; i < paddle_h; i++) {
            ATUI_MVADDSTR(paddle_l + i, 0, "|");
            ATUI_MVADDSTR(paddle_r + i, play_w - 1, ":");
        }
        ATUI_MVADDSTR(ball_y, ball_x, "o");

        ATUI_REFRESH();
        
        // Game Speed
        for(volatile int delay = 0; delay < 300000; delay++); 
    }
    
    ENABLE_SHELL_KEYBOARD();
    ATUI_DESTROY();
}