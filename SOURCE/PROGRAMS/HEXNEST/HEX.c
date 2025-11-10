#include <HEXNEST.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/IO.h>

U32 EDITOR(PU8 file) {
    return 0xFF;
}


#define BYTES_PER_LINE 16
#define LINES_PER_PAGE 16
#define PAGE_SIZE (BYTES_PER_LINE * LINES_PER_PAGE)

U32 HEXDUMP(PU8 path) {
    FILE *file = FOPEN(path, MODE_R | MODE_FAT32);
    if (!file) {
        printf("Failed to open file %s\n", path);
        return 1;
    }

    U8 buffer[PAGE_SIZE];
    U32 offset = 0;
    BOOL looping = TRUE;
    U32 bytes_read = 0;

    while (looping) {
        MEMZERO(buffer, sizeof(buffer));
        bytes_read = FREAD(file, buffer, PAGE_SIZE);
        if (bytes_read == 0) {
            printf("\n<End of file>\n");
            break;
        }

        printf("\nOffset (0x%08X)\n", offset);
        printf("---------------------------------------------\n");

        for (U32 i = 0; i < bytes_read; i += BYTES_PER_LINE) {
            printf("%08X  ", offset + i);

            // print hex bytes
            for (U32 j = 0; j < BYTES_PER_LINE; j++) {
                if (i + j < bytes_read)
                    printf("%02X ", buffer[i + j]);
                else
                    printf("   ");
            }

            printf(" |");

            // print ASCII part
            for (U32 j = 0; j < BYTES_PER_LINE; j++) {
                if (i + j < bytes_read) {
                    U8 c = buffer[i + j];
                    if (c >= 32 && c < 127)
                        putc(c);
                    else
                        putc('.');
                }
            }
            printf("|\n");
        }

        offset += bytes_read;

        // Wait for key input to scroll
        printf("\n[Press any key to continue, ESC to exit]\n");
        while (TRUE) {
            KP_DATA *kp = get_latest_keypress();
            if (kp && kp->cur.pressed) {
                if (kp->cur.keycode == KEY_ESC) {
                    looping = FALSE;
                }
                break; // any key continues
            }
        }
    }

    FCLOSE(file);
    return 0;
}
