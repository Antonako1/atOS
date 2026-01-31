#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/DEBUG.h>
#include <STD/MEM.h>
#include <LIBRARIES/ARGHAND/ARGHAND.h>
#include <LIBRARIES/ATGL/ATGL.h>
#include <STD/GRAPHICS.h>

U32 main(U32 argc, PPU8 argv) {
    #define args_set 1
    PU8 help[] = ARGHAND_ARG("-h", "--help");

    PU8 all[] = ARGHAND_ALL_ALIASES(help);
    U32 counts[] = { ARGHAND_ALIAS_COUNT(help) };
    
    ARGHANDLER ah;
    ARGHAND_INIT(&ah, argc, argv, all, counts, args_set);
    
    if(ARGHAND_IS_PRESENT(&ah, "-h")) {
        ARGHAND_FREE(&ah);
        printf("SANDBOX.BIN - a ATGL test binary\n");
        return 0;
    }
    ARGHAND_FREE(&ah);




    PATGL_SCREEN screen = ATGL_CREATE_SCREEN(ATGL_SA_NONE);
    if(!screen) {
        printf("Failed to create screen!\n");
        return 1;
    }
    while(ATGL_IS_SCREEN_RUNNING()) {
        PS2_KB_DATA *kp = kb_poll();
        if(kp && kp->cur.pressed) {
            switch (kp->cur.keycode)
            {
            case KEY_ESC:
                ATGL_DESTROY_SCREEN();
                break;
            }
        }
    }



    
    #ifdef test
    BOOL8 running = TRUE;
    CLEAR_SCREEN_COLOUR(VBE_WHITE);
    DRAW_8x8_STRING(10,10,"ESC to exit.", VBE_BLACK, VBE_SEE_THROUGH);
    while(running){
        PS2_KB_DATA *kp = kb_poll();
        if(kp && kp->cur.pressed) {
            switch (kp->cur.keycode)
            {
            case KEY_ESC:
                running = FALSE;
                break;
            }
        }
        PS2_MOUSE_DATA *ms = mouse_poll();
         if(ms) {
            U16 c = VBE_RED;
            if(ms->cur.mouse1) c = VBE_YELLOW;
            if(ms->cur.mouse2) VBE_GREEN;
            if(ms->cur.mouse3) VBE_BLUE;

            if(ms->cur.scroll_wheel != ms->prev.scroll_wheel)
                DRAW_8x8_CHARACTER(ms->cur.x, ms->cur.y, 'S', VBE_BROWN, VBE_SEE_THROUGH);
            else
                DRAW_FILLED_RECTANGLE(ms->cur.x, ms->cur.y, 3, 3, c);
        }
    }
    #endif // test
    return 0;
}