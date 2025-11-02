#include <HEXNEST.h>
#include <STD/RUNTIME.h>
#include <STD/IO.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
static HEXNEST_ARGS hexn ATTRIB_DATA = { 0 };

U32 main(U32 argc, PPU8 argv) {
    if(argc < 2) {
        printf("Usage: %s {FILE}. See -h for help\n", argv[0]);
        return 1;
    }
    MEMZERO(&hexn, sizeof(HEXNEST_ARGS));

    for(U32 i = 1; i < argc; i++) {
        PU8 arg = argv[i];
        printf("[%d]: %s\n", i, arg);
        if(STRCMP(arg, "-e") == 0 || STRCMP(arg, "--edit") == 0) {
            hexn.display_editor = TRUE;        
        } else {
            hexn.file = arg;
        }
    }

    if(hexn.display_editor) {
        return EDITOR(hexn.file);
    } else {
        return HEXDUMP(hexn.file);
    }
}