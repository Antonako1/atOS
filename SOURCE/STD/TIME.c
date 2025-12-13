#include <STD/TIME.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <STD/MEM.h>

#define SECONDS_PER_MIN 60
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_DAY 86400

static const U8 DAYS_IN_MONTH[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

static BOOL IS_LEAP_YEAR(U16 year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

// Normalize date/time components
static void NORMALIZE_DATETIME(RTC_DATE_TIME *dt) {
    if (!dt) return;

    // Seconds -> minutes
    while (dt->seconds >= 60) {
        dt->seconds -= 60;
        dt->minutes++;
    }
    while ((S32)dt->seconds < 0) {
        dt->seconds += 60;
        dt->minutes--;
    }

    // Minutes -> hours
    while (dt->minutes >= 60) {
        dt->minutes -= 60;
        dt->hours++;
    }
    while ((S32)dt->minutes < 0) {
        dt->minutes += 60;
        dt->hours--;
    }

    // Hours -> days
    while (dt->hours >= 24) {
        dt->hours -= 24;
        dt->day_of_month++;
    }
    while ((S32)dt->hours < 0) {
        dt->hours += 24;
        dt->day_of_month--;
    }

    // Days -> months/years
    while (1) {
        U8 dim = DAYS_IN_MONTH[dt->month - 1];
        if (dt->month == 2 && IS_LEAP_YEAR(dt->year)) dim++;
        if (dt->day_of_month > dim) {
            dt->day_of_month -= dim;
            dt->month++;
            if (dt->month > 12) {
                dt->month = 1;
                dt->year++;
            }
        } else if (dt->day_of_month < 1) {
            dt->month--;
            if (dt->month < 1) {
                dt->month = 12;
                dt->year--;
            }
            dim = DAYS_IN_MONTH[dt->month - 1];
            if (dt->month == 2 && IS_LEAP_YEAR(dt->year)) dim++;
            dt->day_of_month += dim;
        } else break;
    }
}

// Basic getters
RTC_DATE_TIME GET_DATE_TIME(void) {
    RTC_DATE_TIME retval = {0};
    RTC_DATE_TIME *res = SYSCALL0(SYSCALL_GET_TIME);
    if (!res) return retval;
    MEMCPY(&retval, res, sizeof(RTC_DATE_TIME));
    MFree(res);
    return retval;
}

U32 GET_SECONDS(RTC_DATE_TIME *dt)      { return dt ? dt->seconds : 0xFFFFFFFF; }
U32 GET_MINUTES(RTC_DATE_TIME *dt)      { return dt ? dt->minutes : 0xFFFFFFFF; }
U32 GET_HOURS(RTC_DATE_TIME *dt)        { return dt ? dt->hours : 0xFFFFFFFF; }
U32 GET_WEEKDAY(RTC_DATE_TIME *dt)      { return dt ? dt->weekday : 0xFFFFFFFF; }
U32 GET_DAY_OF_MONTH(RTC_DATE_TIME *dt) { return dt ? dt->day_of_month : 0xFFFFFFFF; }
U32 GET_MONTH(RTC_DATE_TIME *dt)        { return dt ? dt->month : 0xFFFFFFFF; }
U32 GET_YEAR(RTC_DATE_TIME *dt)         { return dt ? dt->year : 0xFFFFFFFF; }
U32 GET_CENTURY(RTC_DATE_TIME *dt)      { return dt ? dt->century : 0xFFFFFFFF; }

// Add/subtract seconds
void ADD_SECONDS(RTC_DATE_TIME *dt, U32 seconds) {
    if (!dt) return;
    dt->seconds += seconds;
    NORMALIZE_DATETIME(dt);
}
void SUBTRACT_SECONDS(RTC_DATE_TIME *dt, U32 seconds) {
    if (!dt) return;
    dt->seconds -= seconds;
    NORMALIZE_DATETIME(dt);
}

// Minutes
void ADD_MINUTES(RTC_DATE_TIME *dt, U32 minutes) {
    if (!dt) return;
    dt->minutes += minutes;
    NORMALIZE_DATETIME(dt);
}
void SUBTRACT_MINUTES(RTC_DATE_TIME *dt, U32 minutes) {
    if (!dt) return;
    dt->minutes -= minutes;
    NORMALIZE_DATETIME(dt);
}

// Hours
void ADD_HOURS(RTC_DATE_TIME *dt, U32 hours) {
    if (!dt) return;
    dt->hours += hours;
    NORMALIZE_DATETIME(dt);
}
void SUBTRACT_HOURS(RTC_DATE_TIME *dt, U32 hours) {
    if (!dt) return;
    dt->hours -= hours;
    NORMALIZE_DATETIME(dt);
}

// Days
void ADD_DAYS(RTC_DATE_TIME *dt, U32 days) {
    if (!dt) return;
    dt->day_of_month += days;
    NORMALIZE_DATETIME(dt);
}
void SUBTRACT_DAYS(RTC_DATE_TIME *dt, U32 days) {
    if (!dt) return;
    dt->day_of_month -= days;
    NORMALIZE_DATETIME(dt);
}

// Weeks
void ADD_WEEKS(RTC_DATE_TIME *dt, U32 weeks)    { ADD_DAYS(dt, weeks * 7); }
void SUBTRACT_WEEKS(RTC_DATE_TIME *dt, U32 weeks) { SUBTRACT_DAYS(dt, weeks * 7); }

// Months
void ADD_MONTHS(RTC_DATE_TIME *dt, U32 months) {
    if (!dt) return;
    dt->month += months;
    NORMALIZE_DATETIME(dt);
}
void SUBTRACT_MONTHS(RTC_DATE_TIME *dt, U32 months) {
    if (!dt) return;
    dt->month -= months;
    NORMALIZE_DATETIME(dt);
}

// Years
void ADD_YEARS(RTC_DATE_TIME *dt, U32 years) {
    if (!dt) return;
    dt->year += years;
    NORMALIZE_DATETIME(dt);
}
void SUBTRACT_YEARS(RTC_DATE_TIME *dt, U32 years) {
    if (!dt) return;
    dt->year -= years;
    NORMALIZE_DATETIME(dt);
}

// Time differences
U32 SECONDS_PASSED(RTC_DATE_TIME *dt1, RTC_DATE_TIME *dt2) {
    if (!dt1 || !dt2) return 0;

    U32 seconds = 0;

    // Years
    for (U16 y = dt1->year; y < dt2->year; y++)
        seconds += (IS_LEAP_YEAR(y) ? 366 : 365) * SECONDS_PER_DAY;

    // Months
    for (U8 m = 0; m < dt2->month - 1; m++) {
        U32 dim = DAYS_IN_MONTH[m];
        if (m == 1 && IS_LEAP_YEAR(dt2->year)) dim++;
        seconds += dim * SECONDS_PER_DAY;
    }

    // Days, hours, minutes, seconds
    seconds += (dt2->day_of_month - dt1->day_of_month) * SECONDS_PER_DAY;
    seconds += (dt2->hours - dt1->hours) * SECONDS_PER_HOUR;
    seconds += (dt2->minutes - dt1->minutes) * SECONDS_PER_MIN;
    seconds += (dt2->seconds - dt1->seconds);

    return seconds;
}

U32 MINUTES_PASSED(RTC_DATE_TIME *dt1, RTC_DATE_TIME *dt2) { return SECONDS_PASSED(dt1, dt2) / 60; }
U32 HOURS_PASSED(RTC_DATE_TIME *dt1, RTC_DATE_TIME *dt2)   { return SECONDS_PASSED(dt1, dt2) / 3600; }
U32 DAYS_PASSED(RTC_DATE_TIME *dt1, RTC_DATE_TIME *dt2)    { return SECONDS_PASSED(dt1, dt2) / 86400; }
BOOL SAME_DAY(RTC_DATE_TIME *dt1, RTC_DATE_TIME *dt2) {
    return dt1 && dt2 &&
           dt1->year == dt2->year &&
           dt1->month == dt2->month &&
           dt1->day_of_month == dt2->day_of_month;
}
