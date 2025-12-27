#include <STD/TYPEDEF.h>
#include <STD/GRAPHICS.h>
#include <STD/IO.h>

U32 main(U32 argc, PPU8 argv) {
    CLEAR_SCREEN_COLOUR(VBE_WHITE);
    BOOL8 running = TRUE;
    while(running) {
        KP_DATA* kp = get_latest_keypress();
        if(valid_kp(kp)) {
            // Handle new keypress
            if(kp->cur.pressed) {
                switch (kp->cur.keycode)
                {
                case KEY_Q:
                    running = FALSE;
                    break;                
                }
            }


            update_kp_seq(kp);
        }
    }
    return 0;
}