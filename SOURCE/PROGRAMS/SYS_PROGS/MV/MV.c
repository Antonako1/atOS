#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/FS_DISK.h>

CMAIN() {
    if(argc > 3) {
        printf(
            #include <MV.HELP>
        );
        return 1;
    }
    for(U32 i = 0; i < argc; i++) {
        if(STRCMP(argv[i], "-h") == 0) {
            printf(
                #include <MV.HELP>
            );
            return 0; 
        }
    }
    PU8 src = argv[1];
    PU8 dest = argv[2];
    
}