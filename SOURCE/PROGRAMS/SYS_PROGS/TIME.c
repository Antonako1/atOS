#include <STD/TYPEDEF.h>
#define ATRC_IMPLEMENTATION
#include <LIBRARIES/ATRC/ATRC.h>
#include <STD/TIME.h>
#include <STD/IO.h>
#include <STD/DEBUG.h>
#include <STD/AUDIO.h>

static PU8 WEEKDAYS[] ATTRIB_DATA = {
    "",
    "SUNDAY",
    "MONDAY",
    "TUESDAY",
    "WEDNESDAY",
    "THURSDAY",
    "FRIDAY",
    "SATURDAY",
};

static PU8 MONTHS[] ATTRIB_DATA = {
    "",
    "JANUARY",
    "FEBRUARY",
    "MARCH",
    "APRIL",
    "MAY",
    "JUNE",
    "JULY",
    "AUGUST",
    "SEPTEMBER",
    "OCTOBER",
    "NOVEMBER",
    "DECEMBER",
};

typedef struct {
    PU8 desc;
    U8 option;
    BOOLEAN IN_USE;
} conf_line;

#define CONFS_LEN 10
static conf_line CONFS[CONFS_LEN] ATTRIB_DATA = {
    {"Seconds", 's', FALSE},
    {"Minutes", 'm', FALSE},
    {"Hours", 'h', FALSE},
    {"Weekday (1=Sunday)", 'w', FALSE},
    {"Day of month", 'd', FALSE},
    {"Month", 'M', FALSE},
    {"Year", 'a', FALSE},
    {"Century", 'c', FALSE},
    {"Weekday name", 'W', FALSE},
    {"Month name", 'O', FALSE},
};

U32 main(U32 argc, PPU8 argv) {
    for(U32 i = 1; i < argc; i++) {
        printf("abc %d!\n", argc);
        printf("[%d]: %s\n", i, argv[i]);
        DEBUG_PRINTF("[%d]: %s\n", i, argv[i]);
        if(
            STRCMP(argv[i], "-h") == 0 ||
            STRCMP(argv[i], "--help") == 0
        ) {
            printf(
                "Usage: %s [FORMAT STRING|ARGS]"
                "\n\tFORMAT STRING (Continuous string of characters):"
                ,argv[0]
            );
            for(U32 j = 0; j < CONFS_LEN; j++) {
                printf(
                    "\n\t\t%c: %s",
                    CONFS[j].option,
                    CONFS[j].desc
                );
            }
            printf(
                "\n\tARGS:"
                "\n\t-h, --help: Display this message"
                "\n\t\tEXAMPLE:"
                "\n\t\tTIME.BIN h:m:s"
            );
            EXIT(0);
        } else {
            PU8 string = argv[i];
            break;
        }
    }
    printf("Exiting...\n");
    // PATRC_FD fd;
    // BOOL use_conf = READ_ATRCFD(&fd, "/ATOS/TIME.CNF", ATRC_READ_WRITE);
    // PU8 format = NULLPTR;
    // if(use_conf) {
    //     format = READ_NKEY(fd, "FORMAT");
    // }
    // if(!format) {
    //     format = "%*8%, %*4%/%*5%/%*6%, %*2%:%*1%.%*0%";
    // }
    return 0;
}
