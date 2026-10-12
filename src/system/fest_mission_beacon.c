#include "types.h"
#include "system/fest_mission_beacon.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "field/festival.h"
#include "gfl/std.h"
#include "nitro/mi.h"
#include "pml/item.h"
#include "save/high_link.h"
#include "save/player_info.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The lists the festival keeps of a Funfest mission, most of them filled from the beacons of its players: the log of
// what the players did, the Pass Powers they gave and the missions that other players offer, with the check of a
// beacon's mission. The ROM has no string for the file, and its start is a guess: its tables are sorted apart from
// fest_mission.c's in .rodata, so it is a file of its own that starts between FestMissionConfig_FromBeacon and FestMissionLog_Add.

// What func_020157dc returns for a beacon's mission: an offer's index, or one of these
#define FEST_MISSION_OFFER_NEW FEST_MISSION_OFFER_MAX
#define FEST_MISSION_OFFER_REJECT 4

// The notice that the offers set (GameBeaconSys_SetNotice)
#define GAME_BEACON_NOTICE_MISSION 2

// The kinds of a log entry that the file tells apart
#define LOG_KIND_NONE 0
#define LOG_KIND_START 2
#define LOG_KIND_MEET 3
#define LOG_KIND_TEXT 0x2d
#define LOG_KIND_MAX 0x2e

#define MOVE_COUNT (MOVE_FUSION_BOLT + 1)

typedef BOOL (*FestMissionCheckFunc)(const FestMissionInfo *info);

static int FestMissionLog_Add(FestMissionLog *log, const FestMissionLogEntry *entry);
static void FestMissionOffer_Clear(FestMissionOffer *offer);
static BOOL func_0201560c(const FestMissionInfo *info);
static BOOL func_0201562c(const FestMissionInfo *info);
static BOOL func_02015658(const FestMissionInfo *info);
static BOOL func_02015684(const FestMissionInfo *info);
static BOOL func_0201569c(const FestMissionInfo *info);
static u8 func_020156c8(const GameBeacon *beacon, u32 mode);
static BOOL func_020156f8(LinkFestival *festival, FestMissionLogEntry *entry, const GameBeacon *beacon, u32 score,
                          u32 mode, u32 own);
static int func_020157dc(LinkFestival *festival, const GameBeacon *beacon);

// What unk0_20 has to be below for each target of a mission of kind 5, or 0 when it can be anything
static const u16 FestMission_Kind5Max[10] = { 0, 0, 3, 5, 0, 0, 3, 2, 20, 0 };

// Whether a mission's target is not valid, by the mission's kind
static const FestMissionCheckFunc FestMission_CheckFuncs[6] = {
    NULL,
    func_0201560c,
    func_0201562c,
    func_02015658,
    func_02015684,
    func_0201569c,
};

// What a log entry's value is, by its kind: 1 an item, 3 a move, 5 a Pass Power, 6 a species
static const u8 FestMission_LogCategories[LOG_KIND_MAX] = {
    0, 0, 0, 2, 6, 6, 6, 6, 0, 0, 0, 1, 1, 3, 0, 5, 0, 0, 0, 6, 6, 6, 6,
    1, 0, 1, 1, 6, 6, 6, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 6, 6, 6, 4,
};

void FestMissionLogEntry_Clear(FestMissionLogEntry *entry) {
    u16 terminator;

    sys_memset(entry, 0, sizeof(FestMissionLogEntry));
    terminator = GFL_StrBufGetTerminator();
    sys_memset16(terminator, entry->name, sizeof(entry->name));
    sys_memset16(terminator, entry->text, sizeof(entry->text));
}

void func_02015018(const FestMissionLogEntry *entry, StrBuf *dest) {
    GFL_StrBufLoadFixedString(dest, entry->name, 8);
}

void FestMissionLog_Clear(FestMissionLog *log) {
    int i;

    for (i = 0; i < FEST_MISSION_LOG_MAX; i++) {
        FestMissionLogEntry_Clear(&log->entries[i]);
    }
    log->unk4EC = 0;
    log->count = 0;
    log->unk4EE = 0;
}

// Adds an entry, over the oldest when the log is full, and returns where it went
static int FestMissionLog_Add(FestMissionLog *log, const FestMissionLogEntry *entry) {
    if (log->count == FEST_MISSION_LOG_MAX) {
        sys_memcpy16(&log->entries[1], &log->entries[0], sizeof(FestMissionLogEntry) * (FEST_MISSION_LOG_MAX - 1));
        log->entries[FEST_MISSION_LOG_MAX - 1] = *entry;
    } else {
        log->entries[log->count++] = *entry;
    }
    return log->count - 1;
}

void FestMissionLog_AddBeacon(LinkFestival *festival, const GameBeacon *beacon, u32 score, u32 mode, u32 own) {
    FestMissionLogEntry entry;

    if (func_020156f8(festival, &entry, beacon, score, mode, own)) {
        FestMissionLog_Add(&festival->log, &entry);
    }
}

void FestMissionLog_AddPlayer(FestMissionLog *log, GameData *gameData, u8 joined) {
    PlayerInfo *info = GetGameDataPlayerInfo(gameData);
    FestMissionLogEntry entry;

    FestMissionLogEntry_Clear(&entry);
    entry.kind = joined + 1;
    entry.own = TRUE;
    entry.trainerId = getTrainerID(info);
    entry.gender = getTrainerGender(info);
    entry.look = func_02008bf4(info);
    if (entry.gender == GENDER_FEMALE) {
        entry.look -= 8;
    }
    copyName(info->name, entry.name, 8);
    FestMissionLog_Add(log, &entry);
}

void FestMissionPassPower_Clear(FestMissionPassPower *passPower) {
    sys_memset(passPower, 0, sizeof(FestMissionPassPower));
    passPower->passPower = GAME_BEACON_PASS_POWER_NONE;
    sys_memset16(GFL_StrBufGetTerminator(), passPower->name, sizeof(passPower->name));
}

void func_02015198(FestMissionPassPowers *passPowers) {
    int i;

    FestMissionPassPower_Clear(&passPowers->own);
    for (i = 0; i < FEST_MISSION_PASS_POWER_MAX; i++) {
        FestMissionPassPower_Clear(&passPowers->others[i]);
    }
    passPowers->count = 0;
}

BOOL FestMissionPassPowers_AddBeacon(FestMissionPassPowers *passPowers, const GameBeacon *beacon) {
    FestMissionPassPower passPower;
    u16 id;
    int i;

    if (passPowers->count == FEST_MISSION_PASS_POWER_MAX) {
        return FALSE;
    }
    FestMissionPassPower_Clear(&passPower);
    switch (beacon->type) {
        case 0x32:
            id = beacon->payload.gift.passPower;
            passPower.received = TRUE;
            break;
        default:
            return FALSE;
    }
    for (i = 0; i < passPowers->count; i++) {
        if (id == passPowers->others[i].passPower) {
            return FALSE;
        }
    }
    sys_memcpy16(beacon->name, passPower.name, sizeof(beacon->name));
    passPower.passPower = id;
    passPowers->others[passPowers->count++] = passPower;
    return TRUE;
}

u16 func_02015248(FestMissionPassPower *passPower, const FestMissionLogEntry *entry) {
    FestMissionPassPower_Clear(passPower);
    passPower->passPower = entry->value;
    passPower->received = FALSE;
    sys_memcpy16(entry->name, passPower->name, 14);
    return passPower->passPower;
}

static void FestMissionOffer_Clear(FestMissionOffer *offer) {
    sys_memset(offer, 0, sizeof(FestMissionOffer));
    ClearFesMissionCfg(&offer->config);
    sys_memset16(GFL_StrBufGetTerminator(), offer->name, sizeof(offer->name));
}

void FestMissionOffers_AddBeacon(LinkFestival *festival, const GameBeacon *beacon) {
    FestMissionOffers *offers = &festival->offers;
    FestMissionOffer offer;
    int index;

    if (!func_0202cf98(beacon)) {
        return;
    }
    index = func_020157dc(festival, beacon);
    if (index == FEST_MISSION_OFFER_REJECT) {
        return;
    }
    FestMissionOffer_Clear(&offer);
    FestMissionConfig_FromBeacon(&offer.config, beacon);
    FetchFestMissionCfgGimmickID(festival, ((const FestMissionInfo *)&beacon->payload)->index, &offer.config.header);
    sys_memcpy16(beacon->name, offer.name, sizeof(beacon->name));
    UpdateFesMissionConfigSecondsTime(&offer.config);
    if (index == FEST_MISSION_OFFER_NEW) {
        offers->entries[offers->count++] = offer;
    } else {
        offers->entries[index] = offer;
    }
    GameBeaconSys_SetNotice(GAME_BEACON_NOTICE_MISSION);
}

void FestMissionOffers_DropExpired(LinkFestival *festival) {
    int i;
    FestMissionOffers *offers = &festival->offers;
    int last;

    for (i = 0; i < offers->count; i++) {
        if (!FestMissionConfig_HasTimeLeft(&offers->entries[i].config)) {
            last = offers->count - 1;
            if (i < last) {
                sys_memcpy32(&offers->entries[i + 1], &offers->entries[i], sizeof(FestMissionOffer) * (last - i));
                if (last > 0) {
                    i--;
                }
            }
            FestMissionOffer_Clear(&offers->entries[offers->count - 1]);
            offers->count--;
            if (offers->count == 0) {
                GameBeaconSys_ClearNotice(GAME_BEACON_NOTICE_MISSION);
            }
        }
    }
}

void FestMissionOffers_Remove(LinkFestival *festival, FestMissionConfig *config) {
    int i;
    FestMissionOffers *offers = &festival->offers;
    int last;

    for (i = 0; i < offers->count; i++) {
        if (FestMissionConfig_IsSame(&offers->entries[i].config, config)) {
            last = offers->count - 1;
            if (i < last) {
                sys_memcpy32(&offers->entries[i + 1], &offers->entries[i], sizeof(FestMissionOffer) * (last - i));
                if (last > 0) {
                    i--;
                }
            }
            FestMissionOffer_Clear(&offers->entries[offers->count - 1]);
            offers->count--;
            if (offers->count == 0) {
                GameBeaconSys_ClearNotice(GAME_BEACON_NOTICE_MISSION);
            }
        }
    }
}

u8 func_02015444(u32 kind) {
    if (kind >= LOG_KIND_MAX) {
        return 0;
    }
    return FestMission_LogCategories[kind];
}

BOOL FestMission_IsBeaconInvalid(LinkFestival *festival, const GameBeacon *beacon) {
    FestMissionConfig config;
    FestMissionInfo *info = &config.info;
    FestMissionHeader *header;
    u8 kind;

    FestMissionConfig_FromBeacon(&config, beacon);
    header = &config.header;
    if (info->index >= FEST_MISSION_COUNT || info->level >= 4 || config.missionId > FEST_MISSION_SCORE_MAX) {
        return TRUE;
    }
    FetchFestMissionCfgGimmickID(festival, info->index, header);
    if (header->unk01 == 3 && !info->unk0_30) {
        return TRUE;
    }
    if (FestMission_CheckFuncs[header->kind](info)) {
        return TRUE;
    }
    if (!func_0202cf98(beacon)) {
        return FALSE;
    }
    if (info->seconds > FEST_MISSION_SECONDS_MAX || info->count > FEST_MISSION_SCORE_MAX || info->unk8_30 >= 3 ||
        info->unk10_9 >= 4 || info->unk4_10 > 100 || info->unk10_0 > 50) {
        return TRUE;
    }
    if (PML_ItemIsDummy(info->unk4_0) || PML_ItemIsUnholdable(info->unk4_0) || info->unk4_0 == ITEM_MASTER_BALL) {
        return TRUE;
    }
    if (info->unkC_22 != 0) {
        if (PML_ItemIsDummy(info->unkC_22) || PML_ItemIsUnholdable(info->unkC_22)) {
            return TRUE;
        }
    }
    kind = func_020156c8(beacon, 2);
    if (kind == LOG_KIND_NONE) {
        return FALSE;
    }
    switch (func_02015444(kind)) {
        case 1:
            if (PML_ItemIsDummy(info->unkC_0)) {
                return TRUE;
            }
            break;
        case 3:
            if (info->unkC_0 == MOVE_NONE || info->unkC_0 >= MOVE_COUNT) {
                return TRUE;
            }
            break;
        case 5:
            if (!PassPower_IsIDValid(info->unkC_0)) {
                return TRUE;
            }
            break;
        case 6:
            if (info->unkC_0 == SPECIES_NONE || info->unkC_0 > SPECIES_GENESECT) {
                return TRUE;
            }
            break;
    }
    return FALSE;
}

static BOOL func_0201560c(const FestMissionInfo *info) {
    if (info->target > SPECIES_GENESECT || info->unk0_20 != 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_0201562c(const FestMissionInfo *info) {
    // BUG: the target can't be both, so only a target of 0 is valid
#ifdef BUGFIX
    if (info->target != 0 && info->target != 590 && info->target != 591) {
#else
    if (info->target != 0 && (info->target != 590 || info->target != 591)) {
#endif
        return TRUE;
    }
    if (info->unk0_20 != 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_02015658(const FestMissionInfo *info) {
    if (info->target >= 6) {
        return TRUE;
    }
    if (info->target == 5) {
        if (info->unk0_20 >= 27) {
            return TRUE;
        }
    } else if (info->unk0_20 != 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_02015684(const FestMissionInfo *info) {
    if (info->target >= 17 || info->unk0_20 != 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_0201569c(const FestMissionInfo *info) {
    u16 max;

    if (info->target >= 9) {
        return TRUE;
    }
    max = FestMission_Kind5Max[info->target];
    if (max != 0 && info->unk0_20 >= max) {
        return TRUE;
    }
    return FALSE;
}

// The kind of a log entry: mode 0 is the start of the mission, 1 a player met, and 2 what the beacon's type says
static u8 func_020156c8(const GameBeacon *beacon, u32 mode) {
    switch (mode) {
        case 0:
            return LOG_KIND_START;
        case 1:
            return LOG_KIND_MEET;
        default:
            break;
    }
    if (beacon->type == 0x3c || beacon->type == 0x3d) {
        return LOG_KIND_NONE;
    }
    return beacon->type - 0x3a;
}

// Fills a log entry from a beacon; FALSE when the beacon is not to be logged
static BOOL func_020156f8(LinkFestival *festival, FestMissionLogEntry *entry, const GameBeacon *beacon, u32 score,
                          u32 mode, u32 own) {
    u8 kind = func_020156c8(beacon, mode);

    if (kind == LOG_KIND_NONE) {
        return FALSE;
    }
    FestMissionLogEntry_Clear(entry);
    switch (kind) {
        case LOG_KIND_TEXT:
            sys_memcpy16(beacon->payload.mission.text, entry->text, sizeof(beacon->payload.mission.text));
            entry->value = beacon->arg.mission.unk;
            break;
        case LOG_KIND_MEET:
            entry->value = beacon->parentZoneId;
            break;
        default:
            entry->value = ((const FestMissionInfo *)&beacon->payload)->unkC_0;
            // What counts for the player's own mission is logged when the score is given
            if (!own && mode == 2 && FestMissionConfig_IsTarget(&festival->config, kind, entry->value)) {
                return FALSE;
            }
            break;
    }
    entry->kind = kind;
    entry->own = own;
    entry->trainerId = beacon->trainerId;
    entry->gender = beacon->gender;
    entry->look = beacon->trainerView;
    entry->score = score;
    sys_memcpy16(beacon->name, entry->name, sizeof(beacon->name));
    return TRUE;
}

// Where the mission of a beacon goes in the offers: the index of its offer when the beacon is newer,
// FEST_MISSION_OFFER_NEW, or FEST_MISSION_OFFER_REJECT
static int func_020157dc(LinkFestival *festival, const GameBeacon *beacon) {
    FestMissionOffers *offers = &festival->offers;
    FestMissionConfig config;
    int i;

    FestMissionConfig_FromBeacon(&config, beacon);
    for (i = 0; i < offers->count; i++) {
        if (FestMissionConfig_IsSame(&offers->entries[i].config, &config)) {
            if (offers->entries[i].config.info.seconds >= config.info.seconds) {
                i = FEST_MISSION_OFFER_REJECT;
            }
            return i;
        }
    }
    if (offers->count >= FEST_MISSION_OFFER_MAX) {
        return FEST_MISSION_OFFER_REJECT;
    }
    if (getStatusOfFesMission(festival) != 0 && FestMissionConfig_IsSame(&festival->config, &config)) {
        return FEST_MISSION_OFFER_REJECT;
    }
    if (func_020149b0(festival, &config)) {
        return FEST_MISSION_OFFER_REJECT;
    }
    return FEST_MISSION_OFFER_NEW;
}
