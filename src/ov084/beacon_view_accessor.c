#include "types.h"
#include "field/beacon_view.h"
#include "field/festival.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/high_link.h"
#include "system/beacon_status.h"
#include "system/fest_mission.h"
#include "system/fest_mission_beacon.h"
#include "system/game_beacon.h"

// What the beacon view reads and writes in the festival's work and the game's beacon status. The ROM has no name for
// this file; "beacon_view_accessor.c" is a guess from overlay 85's "fmission_log_accessor_fv.c", which is at the same
// place in the Funfest mission's view

BOOL BeaconView_GetMissionOffer(LinkFestival *festival, FestMissionConfig *dest, StrBuf *name) {
    int i;
    FestMissionOffers *offers = &festival->offers;

    if (offers->count == 0) {
        return FALSE;
    }
    for (i = 0; i < offers->count; i++) {
        if (offers->entries[i].config.info.unk0_30 && FestMissionConfig_HasTimeLeft(&offers->entries[i].config)) {
            GFL_StrBufLoadFixedString(name, offers->entries[i].name, 8);
            *dest = offers->entries[i].config;
            return TRUE;
        }
    }
    for (i = 0; i < offers->count; i++) {
        if (FestMissionConfig_HasTimeLeft(&offers->entries[i].config)) {
            GFL_StrBufLoadFixedString(name, offers->entries[i].name, 8);
            *dest = offers->entries[i].config;
            return TRUE;
        }
    }
    GFL_StrBufClear(name);
    return FALSE;
}

void BeaconStatus_AddRecord(BeaconStatus *status, u8 record) {
    int i;

    for (i = 0; i < status->recordCount; i++) {
        if (status->records[i] == record) {
            return;
        }
    }
    status->records[status->recordCount++] = record;
}

u8 BeaconStatus_GetRecord(BeaconStatus *status) {
    return status->records[0];
}

void BeaconStatus_PopRecord(BeaconStatus *status) {
    if (status->recordCount != 0) {
        status->recordCount--;
        MI_CpuCopy8(&status->records[1], &status->records[0], status->recordCount);
        status->records[status->recordCount] = 0;
    }
}

void BeaconStatus_SetGivenPassPower(BeaconStatus *status, u32 passPower, const StrBuf *name) {
    status->givenPassPower = passPower;
    GFL_StrBufCopy(status->givenName, name);
}

u32 BeaconStatus_GetGivenPassPower(BeaconStatus *status, StrBuf *name) {
    if (status->givenPassPower != PASS_POWER_NONE) {
        GFL_StrBufCopy(name, status->givenName);
        return status->givenPassPower;
    }
    GFL_StrBufClear(name);
    return PASS_POWER_NONE;
}

void BeaconStatus_ClearGivenPassPower(BeaconStatus *status) {
    status->givenPassPower = PASS_POWER_NONE;
    GFL_StrBufClear(status->givenName);
}
