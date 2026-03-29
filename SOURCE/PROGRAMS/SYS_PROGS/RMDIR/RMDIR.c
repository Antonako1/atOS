#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>

CMAIN() {
    if (argc < 2) {
        printf(
            #include <RMDIR.HELP>
        );
        return 1;
    }

    for (U32 i = 1; i < argc; i++) {
        if (STRCMP(argv[i], "-h") == 0 || STRCMP(argv[i], "--help") == 0) {
            printf(
                #include <RMDIR.HELP>
            );
            return 0;
        }
    }

    BOOL force = FALSE;
    PU8 path = NULLPTR;

    for (U32 i = 1; i < argc; i++) {
        if (STRCMP(argv[i], "-f") == 0 || STRCMP(argv[i], "--force") == 0) {
            force = TRUE;
        } else {
            path = argv[i];
        }
    }

    if (!path) {
        printf("rmdir: missing directory path\n");
        return 1;
    }

    if (!DIR_EXISTS(path)) {
        printf("rmdir: '%s': No such directory\n", path);
        return 1;
    }

    if (DIR_DELETE(path, force)) {
        printf("Deleted directory '%s' successfully.\n", path);
        return 0;
    } else {
        printf("Failed to delete directory '%s'.\n", path);
        return 1;
    }
}