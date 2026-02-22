#include <STD/IO.h>
#include <STD/STRING.h>
#include <STD/PROC_COM.h>

U32 main(U32 argc, PPU8 argv) {
    if(argc < 2 || STRCMP(argv[1], "-h") == 0 || STRCMP(argv[1], "--help") == 0) {
        printf("Usage: kill <pid>\n");
        printf("Sends a kill signal to the process with the specified PID.\n");
        printf("Examples:\n\tkill 1234\n\tkill 0x20\n\tkill 0b00000010\n");
        return 0;
    }
    U32 pid = 0;
    if(argv[1][1] == 'x' || argv[1][1] == 'X') {
        pid = (U32)ATOI_HEX(argv[1] + 2);
    }
    else if(argv[1][1] == 'b' || argv[1][1] == 'B') {
        pid = (U32)ATOI_BIN(argv[1] + 2);
    }
    else {
        pid = (U32)ATOI(argv[1]);
    }
    KILL_PROCESS_INSTANCE(pid);
    printf("Sent kill signal to PID number %02x\n", pid);
    return 0;
}