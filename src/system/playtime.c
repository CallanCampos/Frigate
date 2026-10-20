#include "types.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/rtc.h"
#include "save/playtime.h"
#include "system/rtc.h"

// The time played, and the date and time of the last save (the ROM's string for the file, "playtime.c"). The names of
// the fields are guessed.

PlayTime *setupPlaytimeCounters(HeapID heapId) {
    PlayTime *time = GFL_HeapAllocate(heapId, sizeof(PlayTime), TRUE, "playtime.c", 32);
    PlayTime_Init(time);
    return time;
}

void PlayTime_Init(PlayTime *time) {
    time->hours = 0;
    time->minutes = 0;
    time->seconds = 0;
    time->year = 0;
    time->month = 0;
    time->day = 0;
    time->hour = 0;
    time->minute = 0;
}

void updateGameTime(PlayTime *time, u32 addSeconds) {
    u32 hours = time->hours;
    u32 minutes;
    u32 seconds;

    if (hours == PLAYTIME_MAX_HOURS && time->minutes == 59 && time->seconds == 59) {
        return;
    }

    minutes = time->minutes;
    seconds = time->seconds + addSeconds;
    if (seconds > 59) {
        minutes += seconds / 60;
        seconds %= 60;
        if (minutes > 59) {
            hours += minutes / 60;
            minutes %= 60;
            if (hours > PLAYTIME_MAX_HOURS) {
                hours = PLAYTIME_MAX_HOURS;
                minutes = 59;
                seconds = 59;
            }
        }
    }
    time->hours = hours;
    time->minutes = minutes;
    time->seconds = seconds;
}

void PlayTime_Copy(PlayTime *dest, const PlayTime *src) {
    MI_CpuCopy8(src, dest, sizeof(PlayTime));
}

u16 PlayTime_GetHours(PlayTime *time) {
    return time->hours;
}

u8 PlayTime_GetMinutes(PlayTime *time) {
    return time->minutes;
}

u8 PlayTime_GetSeconds(PlayTime *time) {
    return time->seconds;
}

void PlayTime_SetLastSavedToNow(PlayTime *time) {
    RTCDate date;
    RTCTime now;

    RTC_GetDate(&date);
    RTC_GetTime(&now);
    time->year = date.year;
    time->month = date.month;
    time->day = date.day;
    time->hour = now.hour;
    time->minute = now.minute;
}

u32 PlayTime_GetLastSavedYear(PlayTime *time) {
    return time->year;
}

u32 PlayTime_GetLastSavedMonth(PlayTime *time) {
    return time->month;
}

u32 PlayTime_GetLastSavedDay(PlayTime *time) {
    return time->day;
}

u32 PlayTime_GetLastSavedHour(PlayTime *time) {
    return time->hour;
}

u32 PlayTime_GetLastSavedMinute(PlayTime *time) {
    return time->minute;
}

void PlayTime_GetLastSaved(PlayTime *time, u32 *copy) {
    *copy = time->lastSaved;
}

void PlayTime_SetLastSaved(PlayTime *time, u32 *copy) {
    time->lastSaved = *copy;
}
