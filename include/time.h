#ifndef POKEBW2_TIME_H
#define POKEBW2_TIME_H

// The C library's time.h, as CodeWarrior's MSL has it for ARM. The game links none of its functions; GameSpy's code
// uses its types, with its own gmtime and mktime (gsiSecondsToDate and gsiDateToSeconds)

// Seconds from 1970
typedef long time_t;

// A date: the year from 1900, the month from 0 and the day of the year from 0
struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

#endif // POKEBW2_TIME_H
