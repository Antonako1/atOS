#include <STD/TYPEDEF.h>
#define ATRC_IMPLEMENTATION
// #define ATRC_DEBUG
#include <LIBRARIES/ATRC/ATRC.h>
#include <STD/TIME.h>
#include <STD/IO.h>
#include <STD/DEBUG.h>
#include <STD/AUDIO.h>

static PU8 WEEKDAYS[] ATTRIB_DATA = {
    "",
    "SUNDAY","MONDAY","TUESDAY","WEDNESDAY","THURSDAY","FRIDAY","SATURDAY",
};

static PU8 MONTHS[] ATTRIB_DATA = {
    "",
    "JANUARY","FEBRUARY","MARCH","APRIL","MAY","JUNE",
    "JULY","AUGUST","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER",
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

#define FORMAT_DEF "%*8%, %*4%/%*5%/%*6%, %*2%:%*1%.%*0%"
static U8 format[255] ATTRIB_DATA = {0};
static PU8 args[CONFS_LEN] ATTRIB_DATA = {0};
static I32 TIMEZONE = 0;
static RTC_DATE_TIME time;

BOOL CREATE_ARGS() {
    U8 buf[100];

    ITOA(time.seconds, buf, 10); args[0] = STRDUP(buf);
    ITOA(time.minutes, buf, 10); args[1] = STRDUP(buf);
    ITOA(time.hours, buf, 10); args[2] = STRDUP(buf);
    ITOA(time.weekday, buf, 10); args[3] = STRDUP(buf);
    ITOA(time.day_of_month, buf, 10); args[4] = STRDUP(buf);
    ITOA(time.month, buf, 10); args[5] = STRDUP(buf);
    ITOA(time.year, buf, 10); args[6] = STRDUP(buf);
    ITOA(time.century, buf, 10); args[7] = STRDUP(buf);

    args[8] = STRDUP(WEEKDAYS[time.weekday]);
    args[9] = STRDUP(MONTHS[time.month]);

    return TRUE;
}

U0 RELEASE_ARGS() {
    for(U32 i = 0; i < CONFS_LEN; i++) {
        if(args[i]) { MFree(args[i]); args[i] = NULL; }
    }
}

U0 PARSE_ARG(PU8 arg) {
    if(!arg) return;
    U32 len = STRLEN(arg);

    for(U32 i = 0; i < len; i++) {
        CHAR c = arg[i];
        BOOL matched = FALSE;

        for(U32 j = 0; j < CONFS_LEN; j++) {
            if(c == CONFS[j].option) {
                CONFS[j].IN_USE = TRUE;
                matched = TRUE;

                STRNCAT(format, "%*", sizeof(format));
                U8 buf[4]; ITOA(j, buf, 10);
                STRNCAT(format, buf, sizeof(format));
                STRNCAT(format, "%", sizeof(format));
                break;
            }
        }

        if(!matched) {
            U8 tmp[2] = {c, '\0'};
            STRNCAT(format, tmp, sizeof(format));
        }
    }

    STRNCAT(format, " ", sizeof(format));
}


U32 main(U32 argc, PPU8 argv) {
    // Parse args
    for(U32 i = 1; i < argc; i++) {
        if(STRCMP(argv[i], "-h") == 0 || STRCMP(argv[i], "--help") == 0) {
            printf("Usage: %s [FORMAT STRING|ARGS] [-g N | --gmt N]\n", argv[0]);
            for(U32 j = 0; j < CONFS_LEN; j++)
            printf("\t%c: %s\n", CONFS[j].option, CONFS[j].desc);
            printf("\t-h, --help: Show this help\n");
            printf("\t-g, --gmt N: Set timezone offset in hours\n");
            EXIT(0);
        } else if(STRCMP(argv[i], "-g") == 0 || STRCMP(argv[i], "--gmt") == 0) {
            if(i + 1 < argc) {
                TIMEZONE = ATOI_I32(argv[i + 1]);
                i++;
            }
        } else {
            PARSE_ARG(argv[i]);
        }
    }

    BOOL format_arg_specified = TRUE;
    if(format[0] == '\0') {
        format_arg_specified = FALSE;
        STRNCPY(format, FORMAT_DEF, sizeof(format));
    }
    DEBUG_PRINTF("Here I am! Rock you like a hurricane!\n");
    time = GET_DATE_TIME();
    if(!format_arg_specified) {
        PATRC_FD fd = CREATE_ATRCFD("/ATOS/CONFS/TIME.CNF", ATRC_READ_WRITE);
        if(fd) {
            PU8 atrc_format = READ_NKEY(fd, "FORMAT");
            if(atrc_format && atrc_format[0] != '\0') STRNCPY(format, atrc_format, sizeof(format));
            DEBUG_PRINTF("BLOCKS: %d\n", fd->blocks.len);
            PU8 tz = READ_NKEY(fd, "TIMEZONE");
            if(tz && tz[0] != '\0') TIMEZONE = ATOI_I32(tz);
            DESTROY_ATRCFD(fd);
        }
    }

    // Apply timezone
    if(TIMEZONE != 0) ADD_HOURS(&time, TIMEZONE);

    if(!CREATE_ARGS()) return 1;
    DEBUG_PRINTF("Here I am! Rock you like a hurricane!\n");
    PU8 res = INSERT_VAR(format, args, CONFS_LEN);
    printf("%s\n", res);
    MFree(res);
    RELEASE_ARGS();
    return 0;
}
