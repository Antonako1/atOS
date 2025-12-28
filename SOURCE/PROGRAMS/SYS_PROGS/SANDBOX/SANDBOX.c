#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/DEBUG.h>
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
    
    BOOL8 running = TRUE;
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
            DRAW_PIXEL(CREATE_VBE_PIXEL_INFO(ms->cur.x, ms->cur.y, VBE_RED));
        }
    }
    
    return 0;
}