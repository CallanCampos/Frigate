#include "types.h"
#include "gfl/rtc_cache.h"
#include "gfl/std.h"
#include "nitro/rtc.h"
#include "save/save_control.h"
#include "save/timesig_savedata.h"

// The save block of the trainer card's signature, when each badge was obtained and saved badge polish. The ROM has
// no string for the file, so the name is a guess (it comes from swan's getTimeSigBlkAddress). Block 0x21.

u32 TimeSigSave_GetSize(void) {
    return sizeof(TimeSigSave);
}

void TimeSigSave_Init(TimeSigSave *timeSig) {
    MI_CpuFill8(timeSig, 0, sizeof(TimeSigSave));
}

TimeSigSave *getTimeSigBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_TIMESIG);
}

u32 *TimeSigSave_GetSignature(TimeSigSave *timeSig) {
    return timeSig->signature;
}

BOOL TimeSigSave_HasSignature(TimeSigSave *timeSig) {
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

u8 TimeSigSave_IsUnionViewChanged(TimeSigSave *timeSig) {
    return timeSig->unionViewChanged;
}

void TimeSigSave_SetUnionViewChanged(TimeSigSave *timeSig) {
    timeSig->unionViewChanged = 1;
}

u16 TimeSigSave_GetPersonality(TimeSigSave *timeSig) {
    return timeSig->personality;
}

void TimeSigSave_SetPersonality(TimeSigSave *timeSig, u32 value) {
    timeSig->personality = value;
}

void TimeSigSave_SetSignBlinks(TimeSigSave *timeSig, u8 value) {
    timeSig->signBlinks = value;
}

BOOL TimeSigSave_GetSignBlinks(TimeSigSave *timeSig) {
    return timeSig->signBlinks;
}

void saveSecondsTime(TimeSigSave *timeSig, u32 low, u32 high) {
    timeSig->secondsLow = low;
    timeSig->secondsHigh = high;
}

s64 loadSecondsTime(TimeSigSave *timeSig) {
    return timeSig->seconds;
}

TimeSigBadgeDate TimeSigSave_GetBadgeDate(TimeSigSave *timeSig, u32 badgeId) {
    return timeSig->badgeDates[badgeId];
}

void setBadgeGetSecondsTime(TimeSigSave *timeSig, u32 badgeId, u32 year, u32 month, u32 day) {
    RTCDate date;
    RTCTime time;
    s64 seconds;

    RTC_GetCachedDateTime(&date, &time);
    seconds = RTC_ConvertDateTimeToSecond(&date, &time);
    timeSig->badgeDates[badgeId].year = year;
    timeSig->badgeDates[badgeId].month = month;
    timeSig->badgeDates[badgeId].day = day;
    timeSig->badgePolish[badgeId] = 240;
    saveSecondsTime(timeSig, seconds, seconds >> 32);
}

u16 TimeSigSave_GetBadgePolish(TimeSigSave *timeSig, u32 badgeId) {
    return timeSig->badgePolish[badgeId];
}

void TimeSigSave_SetBadgePolish(TimeSigSave *timeSig, u32 badgeId, u32 polish) {
    timeSig->badgePolish[badgeId] = polish;
}
