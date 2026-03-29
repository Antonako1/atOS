#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>

CMAIN() {
    if (argc < 2) {
        printf(
            #include <DEL.HELP>
        );
        return 1;
    }
    for (U32 i = 1; i < argc; i++) {
        if (STRCMP(argv[i], "-h") == 0) {
            printf(
                #include <DEL.HELP>
            );
            return 0;
        }
    }

    PU8 file = argv[1];

    if (!FILE_EXISTS(file)) {
        printf("del: '%s': No such file\n", file);
        return 1;
    }

    if (FILE_DELETE(file)) {
        return 0;
    } else {
        printf("del: failed to delete '%s'\n", file);
        return 1;
    }
}