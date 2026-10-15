#include "types.h"
#include "save/timesig_savedata.h"
#include "gfl/rtc_cache.h"
#include "gfl/std.h"
#include "nitro/rtc.h"
#include "save/save_control.h"

// The save block of the trainer card's signature, when each badge was obtained and saved badge polish. The ROM has
// no string for the file, so the name is a guess (it comes from swan's getTimeSigBlkAddress). Block 0x21.

#define SAVE_BLOCK_TIMESIG 0x21

u32 func_02009184(void) {
    return sizeof(TimeSigSave);
}

void func_0200918c(TimeSigSave *timeSig) {
    sys_memset(timeSig, 0, sizeof(TimeSigSave));
}

void *getTimeSigBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_TIMESIG);
}

void *func_020091a8(void *timeSig) {
    return timeSig;
}

BOOL func_020091ac(TimeSigSave *timeSig) {
    u32 sum = 0;
    int i;

    for (i = 0; i < 0x180; i++) {
        sum += timeSig->signature[i];
    }
    if (sum == 0) {
        return FALSE;
    }
    return TRUE;
}

u8 func_020091d0(TimeSigSave *timeSig) {
    return timeSig->unk602;
}

void func_020091dc(TimeSigSave *timeSig) {
    timeSig->unk602 = 1;
}

u16 func_020091e8(TimeSigSave *timeSig) {
    return timeSig->unk600;
}

void func_020091f0(TimeSigSave *timeSig, u32 value) {
    timeSig->unk600 = value;
}

void func_020091f8(TimeSigSave *timeSig, u8 value) {
    timeSig->unk603 = value;
}

BOOL func_02009204(TimeSigSave *timeSig) {
    return timeSig->unk603;
}

void saveSecondsTime(TimeSigSave *timeSig, u32 low, u32 high) {
    timeSig->secondsLow = low;
    timeSig->secondsHigh = high;
}

s64 loadSecondsTime(TimeSigSave *timeSig) {
    return timeSig->seconds;
}

TimeSigBadgeDate func_02009230(TimeSigSave *timeSig, u32 badgeId) {
    return timeSig->badgeDates[badgeId];
}

void setBadgeGetSecondsTime(TimeSigSave *timeSig, u32 badgeId, u32 year, u32 month, u32 day) {
    RTCDate date;
    RTCTime time;
    s64 seconds;

    RTC_GetCachedDateTime(&date, &time);
    seconds = func_0207d12c(&date, &time);
    timeSig->badgeDates[badgeId].year = year;
    timeSig->badgeDates[badgeId].month = month;
    timeSig->badgeDates[badgeId].day = day;
    timeSig->badgePolish[badgeId] = 240;
    saveSecondsTime(timeSig, seconds, seconds >> 32);
}

u16 func_020092a8(TimeSigSave *timeSig, u32 badgeId) {
    return timeSig->badgePolish[badgeId];
}

void func_020092b8(TimeSigSave *timeSig, u32 badgeId, u32 polish) {
    timeSig->badgePolish[badgeId] = polish;
}
