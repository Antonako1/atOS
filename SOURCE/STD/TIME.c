#include <STD/TIME.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <STD/MEM.h>

#define SECONDS_PER_MIN 60
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_DAY 86400

// Days in each month
static const U8 DAYS_IN_MONTH[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// Check leap year
static BOOL IS_LEAP_YEAR(U16 year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

// Get current date/time
RTC_DATE_TIME GET_DATE_TIME(void) {
    RTC_DATE_TIME retval = { 0 };
    RTC_DATE_TIME *res = SYSCALL0(SYSCALL_GET_TIME);
    if (!res) return retval;
    MEMCPY(&retval, res, sizeof(RTC_DATE_TIME));
    MFree(res);
    return retval;
}

// Simple getters
U32 GET_SECONDS(RTC_DATE_TIME *dt)       { return dt ? dt->seconds : 0xFFFFFFFF; }
U32 GET_MINUTES(RTC_DATE_TIME *dt)       { return dt ? dt->minutes : 0xFFFFFFFF; }
U32 GET_HOURS(RTC_DATE_TIME *dt)         { return dt ? dt->hours : 0xFFFFFFFF; }
U32 GET_WEEKDAY(RTC_DATE_TIME *dt)       { return dt ? dt->weekday : 0xFFFFFFFF; }
U32 GET_DAY_OF_MONTH(RTC_DATE_TIME *dt)  { return dt ? dt->day_of_month : 0xFFFFFFFF; }
U32 GET_MONTH(RTC_DATE_TIME *dt)         { return dt ? dt->month : 0xFFFFFFFF; }
U32 GET_YEAR(RTC_DATE_TIME *dt)          { return dt ? dt->year : 0xFFFFFFFF; }
U32 GET_CENTURY(RTC_DATE_TIME *dt)       { return dt ? dt->century : 0xFFFFFFFF; }

// Calculate seconds difference (32-bit safe, dt2 >= dt1)
U32 SECONDS_PASSED(RTC_DATE_TIME *dt1, RTC_DATE_TIME *dt2) {
    if (!dt1 || !dt2) return 0;

    U32 seconds = 0;

    // Years
    for (U16 y = dt1->year; y < dt2->year; y++) {
        seconds += (IS_LEAP_YEAR(y) ? 366 : 365) * SECONDS_PER_DAY;
        if (seconds > 0xFFFFFFFF) return 0xFFFFFFFF;
    }

    // Months
    for (U8 m = 0; m < dt2->month; m++) {
        U8 days = DAYS_IN_MONTH[m];
        if (m == 1 && IS_LEAP_YEAR(dt2->year)) days++;
        seconds += days * SECONDS_PER_DAY;
        if (seconds > 0xFFFFFFFF) return 0xFFFFFFFF;
    }

    // Days
    if (dt2->day_of_month > dt1->day_of_month)
        seconds += (dt2->day_of_month - dt1->day_of_month) * SECONDS_PER_DAY;

    // Hours, minutes, seconds
    seconds += (dt2->hours - dt1->hours) * SECONDS_PER_HOUR;
    seconds += (dt2->minutes - dt1->minutes) * SECONDS_PER_MIN;
    seconds += (dt2->seconds - dt1->seconds);

    return seconds;
}

// Derived utilities
U32 MINUTES_PASSED(RTC_DATE_TIME *dt1, RTC_DATE_TIME *dt2) { return SECONDS_PASSED(dt1, dt2) / 60; }
U32 HOURS_PASSED(RTC_DATE_TIME *dt1, RTC_DATE_TIME *dt2)   { return SECONDS_PASSED(dt1, dt2) / 3600; }
U32 DAYS_PASSED(RTC_DATE_TIME *dt1, RTC_DATE_TIME *dt2)    { return SECONDS_PASSED(dt1, dt2) / 86400; }

// Same calendar day
BOOL SAME_DAY(RTC_DATE_TIME *dt1, RTC_DATE_TIME *dt2) {
    return dt1 && dt2 &&
           dt1->year == dt2->year &&
           dt1->month == dt2->month &&
           dt1->day_of_month == dt2->day_of_month;
}

// Add seconds (32-bit safe, may wrap around years far in future)
void ADD_SECONDS(RTC_DATE_TIME *dt, U32 seconds) {
    if (!dt) return;

    // Add seconds to current time
    dt->seconds += seconds;

    // Normalize seconds -> minutes
    while (dt->seconds >= 60) {
        dt->seconds -= 60;
        dt->minutes++;
    }

    // Normalize minutes -> hours
    while (dt->minutes >= 60) {
        dt->minutes -= 60;
        dt->hours++;
    }

    // Normalize hours -> days
    while (dt->hours >= 24) {
        dt->hours -= 24;
        dt->day_of_month++;
    }

    // Normalize days -> months
    while (1) {
        U8 dim = DAYS_IN_MONTH[dt->month];
        if (dt->month == 1 && IS_LEAP_YEAR(dt->year)) dim++;
        if (dt->day_of_month <= dim) break;
        dt->day_of_month -= dim;
        dt->month++;
        if (dt->month >= 12) {
            dt->month = 0;
            dt->year++;
        }
    }
}
