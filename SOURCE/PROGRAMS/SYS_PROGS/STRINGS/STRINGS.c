/*
 * STRINGS.c — Extract printable ASCII strings from a file
 *
 * Scans binary (or text) file data and prints runs of >= 4 consecutive
 * printable characters, each on its own line — like Unix 'strings'.
 */
#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/STRING.h>
#include <STD/FS_DISK.h>

#define MIN_STRING_LEN 4

static U0 HELP_MESSAGE(VOID) {
    printf(
        #include <STRINGS.HELP>
    );
}

CMAIN() {
    if (argc < 2) {
        HELP_MESSAGE();
        return 1;
    }

    for (U32 i = 1; i < argc; i++) {
        if (STRCMP(argv[i], "-h") == 0 || STRCMP(argv[i], "--help") == 0) {
            HELP_MESSAGE();
            return 0;
        }
    }

    PU8 file_path = argv[1];
    FILE *file = FOPEN(file_path, MODE_FR);
    if (!file) {
        printf("strings: cannot open '%s'\n", file_path);
        return 1;
    }

    PU8 data = (PU8)file->data;
    U32 sz   = file->sz;

    U32 run_start = 0;
    BOOL in_run   = FALSE;

    for (U32 i = 0; i < sz; i++) {
        U8 c = data[i];
        BOOL printable = (c >= 0x20 && c <= 0x7E) || c == '\t';

        if (printable) {
            if (!in_run) {
                run_start = i;
                in_run = TRUE;
            }
        } else {
            if (in_run) {
                U32 run_len = i - run_start;
                if (run_len >= MIN_STRING_LEN) {
                    /* Print the run */
                    for (U32 j = run_start; j < i; j++) {
                        printf("%c", data[j]);
                    }
                    printf("\n");
                }
                in_run = FALSE;
            }
        }
    }

    /* Flush any trailing run */
    if (in_run) {
        U32 run_len = sz - run_start;
        if (run_len >= MIN_STRING_LEN) {
            for (U32 j = run_start; j < sz; j++) {
                printf("%c", data[j]);
            }
            printf("\n");
        }
    }

    FCLOSE(file);
    return 0;
}
