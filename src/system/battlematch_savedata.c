#include "types.h"
#include "app/livebattlematch_flow.h"
#include "gfl/net.h"
#include "gfl/rtc_cache.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "save/records.h"
#include "save/save_control.h"

// The save block of the battle competitions: the results of the live competition (livematch_savedata.c) and of the
// random matches and the Wi-Fi competition (rndmatch_savedata.c), and the mark of a match in progress. The ROM has no
// string for the file, so the name is a guess. Block 0x3b.

// A match in progress is over after an hour
#define MATCH_TIMEOUT_SECONDS (60 * 60)

static s64 getSeconds(void);

u32 RecordSave_GetSize(void) {
    return sizeof(RecordSave);
}

void RecordSave_Init(RecordSave *record) {
    LiveMatchData_Init(&record->liveMatch);
    RndMatchData_Init(&record->rndMatch);
    OS_GetMacAddress(record->macAddress);
}

RecordSave *SaveControl_GetRecordSave(SaveControl *save) {
    return getRecordBlkAddress(save);
}

RecordSave *getRecordBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_RECORD);
}

RndMatchData *RecordSave_GetRndMatch(RecordSave *record) {
    return &record->rndMatch;
}

RndMatchData *RecordSave_GetRndMatchData(RecordSave *record) {
    return &record->rndMatch;
}

LiveMatchData *RecordSave_GetLiveMatch(RecordSave *record) {
    return &record->liveMatch;
}

void RecordSave_SetMatchInProgress(RecordSave *record) {
    OS_GetMacAddress(record->macAddress);
    record->matchInProgress = TRUE;
    RecordSave_UpdateMatchSeconds(record);
}

void RecordSave_ClearMatchInProgress(RecordSave *record) {
    record->matchInProgress = FALSE;
}

u8 RecordSave_IsMatchInProgress(RecordSave *record) {
    return record->matchInProgress;
}

u32 RecordSave_IsMacAddressSame(RecordSave *record) {
    u8 macAddress[BATTLEMATCH_MAC_ADDRESS_SIZE];

    OS_GetMacAddress(macAddress);
    if (GFL_STD_MemCmp(record->macAddress, macAddress, BATTLEMATCH_MAC_ADDRESS_SIZE) == 0) {
        return TRUE;
    }
    return FALSE;
}

u32 RecordSave_IsMatchTimedOut(RecordSave *record) {
    if (RecordSave_IsMatchInProgress(record)) {
        if (getSeconds() - record->matchSeconds > MATCH_TIMEOUT_SECONDS) {
            return TRUE;
        }
    }
    return FALSE;
}

void RecordSave_UpdateMatchSeconds(RecordSave *record) {
    record->matchSeconds = getSeconds();
}

void func_0200f37c(RecordSave *record, u32 value) {
    record->unkA3 = value;
}

u8 func_0200f384(RecordSave *record) {
    return record->unkA3;
}

// The seconds since 2000 without the offset of the console's clock, which changing the clock doesn't change
static s64 getSeconds(void) {
    RTCDate date;
    RTCTime time;

    RTC_GetCachedDateTime(&date, &time);
    return RTC_ConvertDateTimeToSecond(&date, &time) - OS_GetOwnerRtcOffset();
}
