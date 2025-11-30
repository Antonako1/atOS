#include <STD/TIME.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <STD/MEM.h>


RTC_DATE_TIME GET_DATE_TIME(U0) {
    RTC_DATE_TIME retval = { 0 };
    RTC_DATE_TIME *res = SYSCALL0(SYSCALL_GET_TIME);
    if(!res) return retval;
    MEMCPY(&retval, res, sizeof(RTC_DATE_TIME));
    MFree(res);
    return retval;
}

U32 GET_SECONDS(RTC_DATE_TIME *dt) {
    if(!dt) return -1;
    return (U32)dt->seconds;
}
U32 GET_MINUTES(RTC_DATE_TIME *dt) {
    if(!dt) return -1;
    return (U32)dt->minutes;
}
U32 GET_HOURS(RTC_DATE_TIME *dt) {
    if(!dt) return -1;
    return (U32)dt->hours;
}
U32 GET_WEEKDAY(RTC_DATE_TIME *dt) {
    if(!dt) return -1;
    return (U32)dt->weekday;
}
U32 GET_DAY_OF_MONTH(RTC_DATE_TIME *dt) {
    if(!dt) return -1;
    return (U32)dt->day_of_month;
}
U32 GET_MONTH(RTC_DATE_TIME *dt) {
    if(!dt) return -1;
    return (U32)dt->month;
}
U32 GET_YEAR(RTC_DATE_TIME *dt) {
    if(!dt) return -1;
    return (U32)dt->year;
}
U32 GET_CENTURY(RTC_DATE_TIME *dt) {
    if(!dt) return -1;
    return (U32)dt->century;
}