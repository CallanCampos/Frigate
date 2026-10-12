#include "types.h"
#include "save/records.h"
#include "app/livebattlematch_flow.h"
#include "gfl/net.h"
#include "gfl/rtc_cache.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "save/save_control.h"

// The save block of the battle competitions: the results of the live competition (livematch_savedata.c) and of the
// random matches and the Wi-Fi competition (rndmatch_savedata.c), and the mark of a match in progress. The ROM has no
// string for the file, so the name is a guess. Block 0x3b.

#define SAVE_BLOCK_RECORD 0x3b
// A match in progress is over after an hour
#define MATCH_TIMEOUT_SECONDS (60 * 60)

static s64 getSeconds(void);

u32 func_0200f29c(void) {
    return sizeof(RecordSave);
}

void func_0200f2a0(RecordSave *record) {
    func_0200f1c4(&record->liveMatch);
    RndMatchData_Init(record->rndMatch);
    func_0207c33c(record->macAddress);
}

RecordSave *func_0200f2bc(SaveControl *save) {
    return getRecordBlkAddress(save);
}

RecordSave *getRecordBlkAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_RECORD);
}

void *func_0200f2d0(RecordSave *record) {
    return record->rndMatch;
}

void *func_0200f2d4(RecordSave *record) {
    return record->rndMatch;
}

void *func_0200f2d8(RecordSave *record) {
    return &record->liveMatch;
}

void func_0200f2dc(RecordSave *record) {
    func_0207c33c(record->macAddress);
    record->matchInProgress = TRUE;
    func_0200f368(record);
}

void RecordSave_ClearMatchInProgress(RecordSave *record) {
    record->matchInProgress = FALSE;
}

u8 func_0200f300(RecordSave *record) {
    return record->matchInProgress;
}

u32 func_0200f308(RecordSave *record) {
    u8 macAddress[BATTLEMATCH_MAC_ADDRESS_SIZE];

    func_0207c33c(macAddress);
    if (GFL_STD_MemCmp(record->macAddress, macAddress, BATTLEMATCH_MAC_ADDRESS_SIZE) == 0) {
        return TRUE;
    }
    return FALSE;
}

u32 func_0200f334(RecordSave *record) {
    if (func_0200f300(record)) {
        if (getSeconds() - record->matchSeconds > MATCH_TIMEOUT_SECONDS) {
            return TRUE;
        }
    }
    return FALSE;
}

void func_0200f368(RecordSave *record) {
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
    return func_0207d12c(&date, &time) - OS_GetOwnerRtcOffset();
}
