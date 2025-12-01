#ifndef CMOS_DRIVER_H
#define CMOS_DRIVER_H
#include <STD/TYPEDEF.h>

// 24-hour format used
typedef struct {
    U16 seconds; // +0x0, 0-59
    U16 minutes; // +0x2, 0-59
    U16 hours;   // +0x4, 0-24 or 0-12
    U8 weekday;  // +0x6, 1-7. 1=sunday
    U8 day_of_month; // +0x7, 1-31
    U8 month;   // +0x8, 1-12
    U16 year;    // +0x9, 0-99
    U8 century; // +0x32, 19-20?
} RTC_DATE_TIME;

#ifndef CMOS_ONLY_DEFINES
typedef struct {
    RTC_DATE_TIME t; // 0x0-0x9
    U8 reg_a;
    U8 reg_b;
} RTC_REG;

#define CMOS_ADDR_REG   0x70
#define CMOS_DATA_REG   0x71

#define RTC_TIME_REG_S  0x0
#define RTC_TIME_REG_E  0x9
#define RTC_REG_A       0x0A
#define RTC_REG_B       0x0B
#define RTC_REG_C       0x0C
#define RTC_REG_CENTURY 0x32

BOOLEAN INIT_RTC(VOID);

RTC_DATE_TIME GET_SYS_TIME();


#endif // CMOS_ONLY_DEFINES
#endif // CMOS_DRIVER_H