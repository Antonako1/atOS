#include <STD/TYPEDEF.h>
#include <STD/GRAPHICS.h>
#include <STD/IO.h>
#include <STD/DEBUG.h>
#include <STD/STRING.h>
#include <STD/TIME.h>
#include <STD/MATH.h>

/* key geometry */
#define NUM_WHITE 8
#define NUM_BLACK 5

typedef struct {
    U8 keycode;
    U32 x,y,w,h;
    U32 default_col;
    U32 active_col;
    BOOL8 pressed;
} KEY_DEF;

/* drawing helpers (top-level, not nested) */
static void draw_white_key(KEY_DEF *k){
    DRAW_FILLED_RECTANGLE(k->x, k->y, k->w, k->h, (VBE_COLOUR)k->default_col);
    DRAW_RECTANGLE(k->x, k->y, k->w, k->h, (VBE_COLOUR)0x000000);
}

static void draw_white_key_active(KEY_DEF *k){
    DRAW_FILLED_RECTANGLE(k->x, k->y, k->w, k->h, (VBE_COLOUR)k->active_col);
    DRAW_RECTANGLE(k->x, k->y, k->w, k->h, (VBE_COLOUR)0x000000);
}

static void draw_black_key(KEY_DEF *k){
    DRAW_FILLED_RECTANGLE(k->x, k->y, k->w, k->h, (VBE_COLOUR)k->default_col);
    DRAW_RECTANGLE(k->x, k->y, k->w, k->h, (VBE_COLOUR)0x444444);
}

static void draw_black_key_active(KEY_DEF *k){
    DRAW_FILLED_RECTANGLE(k->x, k->y, k->w, k->h, (VBE_COLOUR)k->active_col);
    DRAW_RECTANGLE(k->x, k->y, k->w, k->h, (VBE_COLOUR)0x444444);
}

/* redraw all keys in correct order (whites then blacks) so blacks stay on top */
static void redraw_keyboard(KEY_DEF *white, U32 num_white, KEY_DEF *black, U32 num_black){
    for(U32 i=0;i<num_white;i++){
        if(white[i].pressed) draw_white_key_active(&white[i]);
        else draw_white_key(&white[i]);
    }
    for(U32 i=0;i<num_black;i++){
        if(black[i].pressed) draw_black_key_active(&black[i]);
        else draw_black_key(&black[i]);
    }
    FLUSH_VRAM();
}

U32 main(U32 argc, PPU8 argv) {
    /* Simple piano layout drawing and interaction
       - draw white keys then black keys
       - color keys while pressed
    */
    BOOL8 running = TRUE;
    U8 note_pressed = 0; // currently held keycode

    KEY_DEF white[NUM_WHITE];
    KEY_DEF black[NUM_BLACK];

    /* screen positions (fixed simple layout) */
    U32 start_x = 40;
    U32 key_w = 48;
    U32 key_h = 160;
    U32 key_y = 60;

    /* white keys: A S D F G H J K */
    U8 white_codes[NUM_WHITE] = { KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K };
    for(U32 i=0;i<NUM_WHITE;i++){
        white[i].keycode = white_codes[i];
        white[i].x = start_x + i * key_w;
        white[i].y = key_y;
        white[i].w = key_w - 2;
        white[i].h = key_h;
        white[i].default_col = 0xFFFFFF; /* white */
        white[i].active_col = 0xA0D0FF;  /* light blue when active */
        white[i].pressed = FALSE;
    }

    /* black keys: W E   T Y U  placed between whites (no key between E and F) */
    U8 black_codes[NUM_BLACK] = { KEY_W, KEY_E, KEY_T, KEY_Y, KEY_U };
    /* positions: between white[0]&[1], [1]&[2], [3]&[4], [4]&[5], [5]&[6] */
    U32 black_pos_idx[NUM_BLACK] = {0,1,3,4,5};
    for(U32 i=0;i<NUM_BLACK;i++){
        U32 wi = black_pos_idx[i];
        black[i].keycode = black_codes[i];
        black[i].w = key_w/2;
        black[i].h = key_h * 2 / 3;
        black[i].x = white[wi].x + white[wi].w - (black[i].w/2);
        black[i].y = key_y;
        black[i].default_col = 0x000000; /* black */
        black[i].active_col = 0xFFFF00;   /* bright yellow when active */
        black[i].pressed = FALSE;
    }

    /* initial draw */
    CLEAR_SCREEN_COLOUR((VBE_COLOUR)0x202020);
    for(U32 i=0;i<NUM_WHITE;i++) draw_white_key(&white[i]);
    for(U32 i=0;i<NUM_BLACK;i++) draw_black_key(&black[i]);
    FLUSH_VRAM();

    while(running){
        PS2_KB_DATA *kp = kb_poll();
        if(kp) {
            U32 freq = 0;
            if(kp->cur.pressed) {
                switch(kp->cur.keycode) {
                    case KEY_A: freq = 261; break;   // C4
                    case KEY_W: freq = 277; break;   // C#4
                    case KEY_S: freq = 293; break;   // D4
                    case KEY_E: freq = 311; break;   // D#4
                    case KEY_D: freq = 329; break;   // E4
                    case KEY_F: freq = 349; break;   // F4
                    case KEY_T: freq = 370; break;   // F#4
                    case KEY_G: freq = 392; break;   // G4
                    case KEY_Y: freq = 415; break;   // G#4
                    case KEY_H: freq = 440; break;   // A4
                    case KEY_U: freq = 466; break;   // A#4
                    case KEY_J: freq = 493; break;   // B4
                    case KEY_K: freq = 523; break;   // C5

                    default: freq = 0; break;
                }

                if(freq && note_pressed != kp->cur.keycode) {
                    AUDIO_TONE(freq, 0, 8000, 44100); // play indefinitely until released
                    /* draw key active: inline search */
                    KEY_DEF *k = NULL;
                    for(U32 ii=0; ii<NUM_WHITE; ++ii) if(white[ii].keycode == kp->cur.keycode) { k = &white[ii]; break; }
                    if(!k) for(U32 ii=0; ii<NUM_BLACK; ++ii) if(black[ii].keycode == kp->cur.keycode) { k = &black[ii]; break; }
                    if(k){
                        k->pressed = TRUE;
                        redraw_keyboard(white, NUM_WHITE, black, NUM_BLACK);
                    }
                    note_pressed = kp->cur.keycode;
                }
            } else {
                /* key release event */
                if(note_pressed && kp->cur.keycode == note_pressed) {
                    AUDIO_STOP();
                    /* redraw key default: inline search */
                    KEY_DEF *k = NULL;
                    for(U32 ii=0; ii<NUM_WHITE; ++ii) if(white[ii].keycode == kp->cur.keycode) { k = &white[ii]; break; }
                    if(!k) for(U32 ii=0; ii<NUM_BLACK; ++ii) if(black[ii].keycode == kp->cur.keycode) { k = &black[ii]; break; }
                    if(k){
                        k->pressed = FALSE;
                        redraw_keyboard(white, NUM_WHITE, black, NUM_BLACK);
                    }
                    note_pressed = 0;
                }
            }
        }
    }

    return 1;
}