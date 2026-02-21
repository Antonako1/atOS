#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/DEBUG.h>
#include <STD/MEM.h>
#include <LIBRARIES/ARGHAND/ARGHAND.h>
#include <LIBRARIES/ATGL/ATGL.h>
#include <STD/GRAPHICS.h>

U32 ATGL_MAIN(U32 argc, PPU8 argv) {
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
    DEBUG_PRINTF("Starting ATGL Sandbox...\n");



    PATGL_SCREEN screen = ATGL_CREATE_SCREEN(ATGL_SA_NONE);
    if(!screen) {
        printf("Failed to create screen!\n");
        return 1;
    }
    return 0;
}

VOID ATGL_EVENT_LOOP(ATGL_EVENT *ev) {

}

VOID ATGL_GRAPHICS_LOOP() {
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