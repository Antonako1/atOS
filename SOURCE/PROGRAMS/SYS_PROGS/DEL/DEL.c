#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>

CMAIN() {
    if(argc < 2) {
        printf(
            #include <DEL.HELP>
        );
        return 1;
    }
    for(U32 i = 0; i < argc; i++) {
        if(STRCMP(argv[i], "-h") == 0) {
            printf(
                #include <DEL.HELP>
            );
            return 0; 
        }
    }
    PU8 file = argv[1];
    return FILE_DELETE(file);
}