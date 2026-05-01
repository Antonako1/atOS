#include "IRONCLAD.h"
#include <STD/IO.h>
#include <RTOSKRNL/PROC/PROC.h>
U32 IRONCLAD_LOOP() {

}

VOID HELP() {
    printf("IRONCLAD - atOS's Dynamic Library Loader and Host Process\n");
    printf("Usage: ironclad [options] <binary> [args...]\n");
    printf("Options:\n");
    printf("  -h, --help       Show this help message and exit\n");
    printf("  -l, --load       Load the specified binary into memory and execute it\n");
    printf("\nExample:\n");
    printf("  ironclad -l /path/to/binary arg1 arg2\n");
}

U32 main(U32 argc, PPU8 argv) {
    if (argc < 2) {
        HELP();
        return 0;
    }
    PU8 lib_path = NULLPTR;
    PU8 lib_args[32] = {0}; // at most 32 args
    for(U32 i = 0; i < argc; i++) {
        if (STRCMP((PU8)argv[i], "-h") == 0 || STRCMP((PU8)argv[i], "--help") == 0) {
            HELP();
            return 0;
        } else if (STRCMP((PU8)argv[i], "-l") == 0 || STRCMP((PU8)argv[i], "--load") == 0) {
            if (i + 1 < argc) {
                lib_path = argv[i + 1];
                break;
            } else {
                printf("Error: Missing binary path after %s\n", (PU8)argv[i]);
                return 1;
            }
        } else {
            // Positional arguments after options are treated as binary path and args
            if (!lib_path) {
                lib_path = argv[i];
            } else {
                // Collect additional args for the binary
                for (U32 j = 0; j < 32 && i + j < argc; j++) {
                    lib_args[j] = argv[i + j];
                }
                break;
            }
        }
    }
    if (!lib_path) {
        printf("Error: No binary specified. Use -l or --load to specify a binary.\n");
        return 1;
    }

    return IRONCLAD_LOOP();
}