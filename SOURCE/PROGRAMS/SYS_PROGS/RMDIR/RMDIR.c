#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/DEBUG.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
CMAIN() {
    if(argc < 2){
        printf(
            #include <RMDIR.HELP>
        );
        return 1;
    }

    for(U32 i = 0; i < argc; i++) {
        if(STRCMP(argv[i], "-h") == 0) {
            printf(
                #include <RMDIR.HELP>
            );
            return 0; 
        }
    }
    BOOL force = FALSE;
    PU8 path = argv[1];
    if(argc >= 3) {
        force = TRUE;
        path = argv[2];
    }
    if(DIR_DELETE(path, force)) {
        printf("Deleted directory '%s' succesfully.\n",path);
        return 0;
    } else {
        printf("Failed to delete directory '%s'.\n",path);
        return 1;
    }
}