#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/DEBUG.h>
#include <STD/FS_DISK.h>
CMAIN() {
    if(argc < 2){
        printf(
            #include <MKDIR.HELP>
        );
        return 1;
    }
    for(U32 i = 0; i < argc; i++) {
        if(STRCMP(argv[i], "-h") == 0) {
            printf(
                #include <MKDIR.HELP>
            );
            return 0; 
        }
    }
    PU8 path = argv[1];
    return DIR_CREATE(path);
}