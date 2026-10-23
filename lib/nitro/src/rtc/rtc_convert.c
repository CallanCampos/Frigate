// NitroSDK's RTC (rtc_convert.c): conversions between the clock's date and time and the days and seconds since
// 2000-01-01, and the day of the week of a date.
#include "nitro/rtc.h"

#define RTC_DATE_YEAR_MAX 100
#define RTC_DAYS_PER_YEAR 365
#define RTC_SECONDS_PER_MINUTE 60
#define RTC_SECONDS_PER_DAY 86400
// The last day and the last second the clock counts: the end of 2099
#define RTC_DAY_MAX 36524
#define RTC_SECOND_OF_DAY_MAX 86399
#define RTC_SECOND_MAX 3155759999LL

// The days of the year before each month, in a year that is not a leap year
static s32 sDayOfYear[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

// NitroSDK's RTC_ConvertDateToDay. The month is checked twice, once with the rest of the date and once before it
// indexes the table
s32 RTC_ConvertDateToDay(const RTCDate *date) {
    s32 days;

    if (date->year >= RTC_DATE_YEAR_MAX || date->month < 1 || date->month > 12 || date->day < 1 || date->day > 31 ||
        date->week >= 7 || date->month < 1 || date->month > 12) {
        return -1;
    }
    days = date->day - 1 + sDayOfYear[date->month - 1];
    if (date->month >= 3 && (date->year & 3) == 0) {
        days++;
    }
    days += date->year * RTC_DAYS_PER_YEAR;
    days += (date->year + 3) / 4;
    return days;
}

s32 RTCi_ConvertTimeToSecond(const RTCTime *time) {
    return (time->hour * RTC_SECONDS_PER_MINUTE + time->minute) * RTC_SECONDS_PER_MINUTE + time->second;
}

s64 RTC_ConvertDateTimeToSecond(const RTCDate *date, const RTCTime *time) {
    s32 days = RTC_ConvertDateToDay(date);
    s32 seconds;

    if (days == -1) {
        return -1;
    }
    seconds = RTCi_ConvertTimeToSecond(time);
    if (seconds == -1) {
        return -1;
    }
    return (s64)days * RTC_SECONDS_PER_DAY + seconds;
}

// NitroSDK's RTC_ConvertDayToDate: the year, then the month and the day of the month in it. The days are clamped to
// the end of 2099
void RTC_ConvertDayToDate(RTCDate *date, s32 day) {
    u32 year;
    int prev;
    int month;

    if (day < 0) {
        day = 0;
    }
    if (day > RTC_DAY_MAX) {
        day = RTC_DAY_MAX;
    }
    date->week = (day + 6) % 7;

    for (year = 0; year < RTC_DATE_YEAR_MAX - 1; year++) {
        prev = day;
        day -= (year & 3) == 0 ? RTC_DAYS_PER_YEAR + 1 : RTC_DAYS_PER_YEAR;
        if (day < 0) {
            day = prev;
            break;
        }
    }
    if (day > RTC_DAYS_PER_YEAR) {
        day = RTC_DAYS_PER_YEAR;
    }
    date->year = year;

    if ((year & 3) == 0) {
        if (day < 60) {
            if (day < 31) {
                month = 1;
            } else {
                month = 2;
                day -= 31;
            }
            date->month = month;
            date->day = day + 1;
            return;
        }
        day--;
    }
    for (month = 11; month >= 0; month--) {
        if (day >= sDayOfYear[month]) {
            date->month = month + 1;
            date->day = day - sDayOfYear[month] + 1;
            return;
        }
    }
}

void RTCi_ConvertSecondToTime(RTCTime *time, s32 second) {
    if (second < 0) {
        second = 0;
    }
    if (second > RTC_SECOND_OF_DAY_MAX) {
        second = RTC_SECOND_OF_DAY_MAX;
    }
    time->second = second % RTC_SECONDS_PER_MINUTE;
    second /= RTC_SECONDS_PER_MINUTE;
    time->minute = second % RTC_SECONDS_PER_MINUTE;
    time->hour = second / RTC_SECONDS_PER_MINUTE;
}

void RTC_ConvertSecondToDateTime(RTCDate *date, RTCTime *time, s64 seconds) {
    if (seconds < 0) {
        seconds = 0;
    } else if (seconds > RTC_SECOND_MAX) {
        seconds = RTC_SECOND_MAX;
    }
    RTCi_ConvertSecondToTime(time, seconds % RTC_SECONDS_PER_DAY);
    RTC_ConvertDayToDate(date, seconds / RTC_SECONDS_PER_DAY);
}

// NitroSDK's RTC_GetDayOfWeek, by Zeller's congruence with January and February as the months 11 and 12 of the year
// before
RTCWeek RTC_GetDayOfWeek(RTCDate *date) {
    int month;
    int year;
    int day;
    int century;
    int yearOfCentury;

    year = date->year + 2000;
    month = date->month - 2;
    day = date->day;
    if (month < 1) {
        month += 12;
        year--;
    }
    century = year / 100;
    yearOfCentury = year % 100;
    return (day + (26 * month - 2) / 10 + yearOfCentury + yearOfCentury / 4 + century / 4 + century * 5) % 7;
}
