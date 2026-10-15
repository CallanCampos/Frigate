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
    func_02008c40(time);
    return time;
}

void func_02008c40(PlayTime *time) {
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

void func_02008cdc(PlayTime *dest, const PlayTime *src) {
    sys_memcpy(src, dest, sizeof(PlayTime));
}

u16 func_02008cec(PlayTime *time) {
    return time->hours;
}

u8 func_02008cf0(PlayTime *time) {
    return time->minutes;
}

u8 func_02008cf4(PlayTime *time) {
    return time->seconds;
}

void func_02008cf8(PlayTime *time) {
    RTCDate date;
    RTCTime now;

    func_0207cc10(&date);
    func_0207cc80(&now);
    time->year = date.year;
    time->month = date.month;
    time->day = date.day;
    time->hour = now.hour;
    time->minute = now.minute;
}

u32 func_02008d68(PlayTime *time) {
    return time->year;
}

u32 func_02008d70(PlayTime *time) {
    return time->month;
}

u32 func_02008d78(PlayTime *time) {
    return time->day;
}

u32 func_02008d80(PlayTime *time) {
    return time->hour;
}

u32 func_02008d88(PlayTime *time) {
    return time->minute;
}

void func_02008d90(PlayTime *time, u32 *copy) {
    *copy = time->lastSaved;
}

void func_02008d98(PlayTime *time, u32 *copy) {
    time->lastSaved = *copy;
}
