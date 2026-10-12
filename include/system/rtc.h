#ifndef POKEBW2_SYSTEM_RTC_H
#define POKEBW2_SYSTEM_RTC_H

#include "types.h"
#include "nitro/rtc.h"

// The clock as GFL caches it is in gfl/rtc_cache.h
void func_0207cc10(RTCDate *date);
// NitroSDK's RTC_GetTime
void func_0207cc80(RTCTime *time);

// The periods of a day
enum {
    DAY_PERIOD_MORNING,
    DAY_PERIOD_DAY,
    DAY_PERIOD_EVENING,
    DAY_PERIOD_NIGHT,
    DAY_PERIOD_MIDNIGHT,
    DAY_PERIOD_COUNT
};

// rtc.c (the name is a guess)
// The period of the day at the clock's hour, in the season
u16 GetRealTimeDayPeriod(int season);
// The period of the day at the hour, in the season
u16 GetDayPeriod(int season, u32 hour);
// The hour a season's day period starts at
u8 GetLightChangeHoursForSeasons(u32 season, u32 period);
// Whether it is night or midnight in the season. Not from swan
BOOL IsDayPeriodNight(int season);

#endif // POKEBW2_SYSTEM_RTC_H
