#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>

CMAIN() {
    if (argc < 2) {
        printf(
            #include <TOUCH.HELP>
        );
        return 1;
    }
    for (U32 i = 1; i < argc; i++) {
        if (STRCMP(argv[i], "-h") == 0) {
            printf(
                #include <TOUCH.HELP>
            );
            return 0;
        }
    }

    PU8 file = argv[1];

    /* If the file already exists, touch is a no-op (success) */
    if (FILE_EXISTS(file)) {
        return 0;
    }

    if (FILE_CREATE(file)) {
        return 0;
    } else {
        printf("touch: cannot create '%s'\n", file);
        return 1;
    }
}