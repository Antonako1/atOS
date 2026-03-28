#include <STD/TYPEDEF.h>

CMAIN() {
    if(argc < 3) {
        printf(
            #include <CP.HELP>
        );
        return 1;
    }
    for(U32 i = 0; i < argc; i++) {
        if(STRCMP(argv[i], "-h") == 0) {
            printf(
                #include <CP.HELP>
            );
            return 0; 
        }
    }
}