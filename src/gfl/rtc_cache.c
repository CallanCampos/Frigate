#include "types.h"
#include "gfl/rtc_cache.h"
#include "gfl/std.h"
#include "nitro/rtc.h"

// The date and time, read from the clock every few frames so the game need not wait for it. The file's name is a guess

typedef struct {
    BOOL valid;
    // Whether a read is in progress
    BOOL busy;
    int frames;
    int result;
    RTCDate date;
    RTCTime time;
    // Where the read in progress goes
    RTCDate readDate;
    RTCTime readTime;
} RTCCache;

static const u16 sDaysBeforeMonth[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

static RTCCache sRTCCache;

static void RTCCache_ReadDone(int result, void *arg);
static void RTCCache_Read(RTCCache *cache);

void RTCCache_Init(void) {
    RTC_Init();
    MI_CpuFill8(&sRTCCache, 0, sizeof(RTCCache));
    sRTCCache.valid = FALSE;
    sRTCCache.busy = FALSE;
    sRTCCache.frames = 0;
    RTCCache_Read(&sRTCCache);
}

void RTCCache_Update(void) {
    if (sRTCCache.busy == FALSE) {
        sRTCCache.frames++;
        if (sRTCCache.frames > 10) {
            sRTCCache.frames = 0;
            RTCCache_Read(&sRTCCache);
        }
    }
}

static void RTCCache_ReadDone(int result, void *arg) {
    RTCCache *cache = arg;

    cache->result = result;
    cache->valid = TRUE;
    cache->date = cache->readDate;
    cache->time = cache->readTime;
    cache->busy = FALSE;
}

static void RTCCache_Read(RTCCache *cache) {
    cache->busy = TRUE;
    cache->result = RTC_GetDateTimeAsync(&cache->readDate, &cache->readTime, RTCCache_ReadDone, cache);
}

void RTC_GetCachedDateTime(RTCDate *date, RTCTime *time) {
    *date = sRTCCache.date;
    *time = sRTCCache.time;
}

void RTC_GetCachedTime(RTCTime *time) {
    *time = sRTCCache.time;
}

void RTC_GetCachedDate(RTCDate *date) {
    *date = sRTCCache.date;
}

int RTC_ConvertDaySecondsCached(void) {
    const RTCTime *time = &sRTCCache.time;

    return time->second + (time->minute * 60 + time->hour * 3600);
}

s64 RTC_ConvertSecondsCached(void) {
    return RTC_ConvertDateTimeToSecond(&sRTCCache.date, &sRTCCache.time);
}

int RTC_GetDayOfYear(const RTCDate *date) {
    int days;
    RTCDate firstDay;

    days = date->day;
    // Months count from 1
    days += *(sDaysBeforeMonth + date->month - 1);
    if (date->month >= 3) {
        int year = date->year;

        if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
            days++;
        }
    }
    firstDay = *date;
    firstDay.month = 1;
    firstDay.day = 1;
    RTC_ConvertDateToDay(&firstDay);
    RTC_ConvertDateToDay(date);
    return days;
}
