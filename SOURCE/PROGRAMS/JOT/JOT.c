// #define ATRC_IMPLEMENTATION
// #include <LIBRARIES/ATRC/ATRC.h>
// #include <LIBRARIES/ATUI/ATUI.h>
#include <STD/TYPEDEF.h>

U0 JOT_EXIT() {

}

VOID PRINT_HELP() {

}

U32 main(U32 argc, PPU8 argv) {
    // ON_EXIT(JOT_EXIT);
    for(U32 i = 0; i < argc; i++) {
        PU8 arg = argv[i];
        printf("%s\n", arg);
    }
    
    return 0;
}