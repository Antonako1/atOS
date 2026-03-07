#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/STRING.h>
#include <STD/FS_DISK.h>


U0 HELP_MESSAGE() {
    printf(
        #include <STRINGS.HELP>
    );
}   

U32 main(U32 argc, U8 **argv) {
    if(argc < 2) {
        HELP_MESSAGE();
        return 1;
    }

    for(U32 i = 1; i < argc; i++) {
        if(STRCMP(argv[i], "-h") == 0 || STRCMP(argv[i], "--help") == 0) {
            HELP_MESSAGE();
            return 0;
        }
    }

    PU8 file_path = argv[1];
    FILE* file = FOPEN(file_path, MODE_FR);

    if(!file) {
        printf("Failed to open file %s\n", file_path);
        return 1;
    }

    PU8 start = file->data;
    PU8 end = file->data + file->sz;

    BOOL8 running = TRUE;

    while(running) {
        PS2_KB_DATA *kp = kb_poll();

        if(kp && kp->cur.pressed) {

            if(kp->cur.keycode == KEY_ESC) {
                running = FALSE;
                break;
            }

            PU8 p = start;

            while(p < end && *p != '\n') {
                p++;
            }

            U32 len = p - start;

            printf("%.*s\n", len, start);

            if(p < end)
                start = p + 1;
            else
                running = FALSE;
        }
    }

    FCLOSE(file);
    return 0;
}
