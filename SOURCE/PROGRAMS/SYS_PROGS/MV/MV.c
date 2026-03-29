/*
 * MV.c — Move (rename) a file or directory from src to dest (FAT32)
 *
 * Implemented as copy + delete since there is no rename syscall.
 * Directory moves are not supported (only files).
 */
#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>
#include <STD/MEM.h>

CMAIN() {
    if (argc < 3) {
        printf(
            #include <MV.HELP>
        );
        return 1;
    }
    for (U32 i = 1; i < argc; i++) {
        if (STRCMP(argv[i], "-h") == 0) {
            printf(
                #include <MV.HELP>
            );
            return 0;
        }
    }

    PU8 src  = argv[1];
    PU8 dest = argv[2];

    /* Check source exists */
    if (!FILE_EXISTS(src)) {
        printf("mv: cannot stat '%s': No such file\n", src);
        return 1;
    }

    /* Open source for reading */
    FILE *src_file = FOPEN(src, MODE_FR);
    if (!src_file) {
        printf("mv: cannot open '%s'\n", src);
        return 1;
    }

    U32 sz = FSIZE(src_file);

    /* Remove destination if it exists */
    if (FILE_EXISTS(dest)) {
        FILE_DELETE(dest);
    }

    /* Create destination */
    if (!FILE_CREATE(dest)) {
        printf("mv: cannot create '%s'\n", dest);
        FCLOSE(src_file);
        return 1;
    }

    /* Copy data if non-empty */
    if (sz > 0) {
        FILE *dst_file = FOPEN(dest, MODR_FRW);
        if (!dst_file) {
            printf("mv: cannot open '%s' for writing\n", dest);
            FCLOSE(src_file);
            return 1;
        }

        U32 written = FWRITE(dst_file, src_file->data, sz);
        FCLOSE(dst_file);

        if (written != sz) {
            printf("mv: write error (wrote %d of %d bytes)\n", written, sz);
            FCLOSE(src_file);
            return 1;
        }
    }

    FCLOSE(src_file);

    /* Delete original */
    if (!FILE_DELETE(src)) {
        printf("mv: warning: copied ok but failed to remove '%s'\n", src);
        return 1;
    }

    return 0;
}