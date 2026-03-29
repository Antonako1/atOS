/*
 * CP.c — Copy a file from source to destination (FAT32)
 */
#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>
#include <STD/MEM.h>

CMAIN() {
    if (argc < 3) {
        printf(
            #include <CP.HELP>
        );
        return 1;
    }
    for (U32 i = 1; i < argc; i++) {
        if (STRCMP(argv[i], "-h") == 0) {
            printf(
                #include <CP.HELP>
            );
            return 0;
        }
    }

    PU8 src  = argv[1];
    PU8 dest = argv[2];

    /* Open source file for reading */
    FILE *src_file = FOPEN(src, MODE_FR);
    if (!src_file) {
        printf("cp: cannot open '%s': No such file\n", src);
        return 1;
    }

    U32 sz = FSIZE(src_file);

    /* If destination already exists, delete it first so we get a clean copy */
    if (FILE_EXISTS(dest)) {
        FILE_DELETE(dest);
    }

    /* Create the destination file */
    if (!FILE_CREATE(dest)) {
        printf("cp: cannot create '%s'\n", dest);
        FCLOSE(src_file);
        return 1;
    }

    /* Write source contents to destination */
    if (sz > 0) {
        FILE *dst_file = FOPEN(dest, MODR_FRW);
        if (!dst_file) {
            printf("cp: cannot open destination '%s' for writing\n", dest);
            FCLOSE(src_file);
            return 1;
        }

        U32 written = FWRITE(dst_file, src_file->data, sz);
        if (written != sz) {
            printf("cp: write error (wrote %d of %d bytes)\n", written, sz);
            FCLOSE(dst_file);
            FCLOSE(src_file);
            return 1;
        }

        FCLOSE(dst_file);
    }

    FCLOSE(src_file);
    return 0;
}