#include <PROGRAMS/ASTRAC/ASTRAC.h>
#include <STD/PROC_COM.h>
#include <STD/GRAPHICS.h>
#include <STD/IO.h>

static U32 MSG_COUNT ATTRIB_DATA = 0; 

U0 INITIALIZE_ASTRAC();

U0 _start(U32 argc, PPU8 argv) {
    PROC_INIT_CONSOLE();
    while(!IS_PROC_INITIALIZED());
    puts("AstraC C compiler and Assembler!\n");
    for(U32 i = 0; i < argc;i++) {
        puts(argv[i]);
        putc('\n');
    }
    puts(line_end);
    EXIT(0);
}

U0 INITIALIZE_ASTRAC() {
    DRAW_ELLIPSE(100, 100, 100, 100, 0xFF00FF);
}
