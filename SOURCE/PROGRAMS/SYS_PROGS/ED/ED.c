#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>

#define ARG_COMP(case1, case2) (STRCMP(argv[i], (PU8)(case1)) == 0 || STRCMP(argv[i], (PU8)(case2)) == 0)

U32 main(U32 argc, PPU8 argv) {
    for(U32 i = 1; i < argc; i++) {
        if(ARG_COMP("-h", "--help")) {
            printf("Usage: ED [OPTIONS] [FILE]\n");
            return 0;
        }
    }
    return 0;
}