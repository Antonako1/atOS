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
    DEBUG_PRINTF("EVENT\n");
    switch (ev->type)
    {
    case ATGL_EV_KEYBOARD:
        if(ev->data.keyboard->cur.pressed && ev->data.keyboard->cur.keycode == KEY_ESC) {
            ATGL_DESTROY_SCREEN();
        }
        break;
    }
}

VOID ATGL_GRAPHICS_LOOP() {

}