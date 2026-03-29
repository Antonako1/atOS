#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>

CMAIN() {
    if (argc < 2) {
        printf(
            #include <MKDIR.HELP>
        );
        return 1;
    }
    for (U32 i = 1; i < argc; i++) {
        if (STRCMP(argv[i], "-h") == 0) {
            printf(
                #include <MKDIR.HELP>
            );
            return 0;
        }
    }

    PU8 path = argv[1];

    if (DIR_EXISTS(path)) {
        printf("mkdir: '%s': Directory already exists\n", path);
        return 1;
    }

    if (DIR_CREATE(path)) {
        return 0;
    } else {
        printf("mkdir: failed to create '%s'\n", path);
        return 1;
    }
}