#include <PROGRAMS/ASTRAC/ASTRAC.h>
#include <STD/PROC_COM.h>
#include <STD/GRAPHICS.h>

static U32 MSG_COUNT ATTRIB_DATA = 0; 

U0 INITIALIZE_ASTRAC();

U0 _start(U32 argc, PPU8 argv) {
    PROC_INIT_CONSOLE();
    while(!IS_PROC_INITIALIZED());

    CLEAR_SCREEN_COLOUR(VBE_BLACK);
    for(U32 i = 0; i < argc; i++) {
        DRAW_8x8_STRING(0, i*16+2, argv[i], VBE_GREEN, VBE_BLACK);
    }
    for(U32 i = 0; i < 0xFFFFFF; i++);

    EXIT(0);
}

U0 INITIALIZE_ASTRAC() {
    DRAW_ELLIPSE(100, 100, 100, 100, 0xFF00FF);
}
