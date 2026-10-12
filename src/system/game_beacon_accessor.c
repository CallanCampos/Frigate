#include "types.h"
#include "system/game_beacon.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/version.h"
#include "constants/zones.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/mi.h"
#include "save/high_link.h"
#include "save/survey_savedata.h"
#include "system/pms_data.h"
#include "system/wordset.h"

// The tables of received beacons and the readers of a beacon's fields, which give a default for a value out of range.
// The file's name is from the ROM's string.

// What a beacon's message shows and its payload holds, GameBeaconTypeFormat's format
enum {
    BEACON_FORMAT_NONE,
    BEACON_FORMAT_TRAINER,
    BEACON_FORMAT_SPECIES,
    BEACON_FORMAT_NICKNAME,
    BEACON_FORMAT_SPECIES_NICKNAME,
    BEACON_FORMAT_ITEM,
    BEACON_FORMAT_NUMBER3,
    BEACON_FORMAT_NUMBER3_LONG,
    BEACON_FORMAT_GIFT_SPECIES,
    BEACON_FORMAT_GIFT_ITEM,
    BEACON_FORMAT_MOVE,
    BEACON_FORMAT_NUMBER1,
    BEACON_FORMAT_BATTLE_INSTITUTE,
    BEACON_FORMAT_PASS_POWER,
    BEACON_FORMAT_TEXT,
    BEACON_FORMAT_GIFT_PASS_POWER,
    BEACON_FORMAT_POKEWOOD,
    BEACON_FORMAT_TOURNAMENT,
    BEACON_FORMAT_ZONE,
    BEACON_FORMAT_COUNT
};

#define BEACON_FORMAT_TYPE_MAX 0x3c
// The kinds of a recent event, with GAME_BEACON_RECENT_NONE
#define BEACON_RECENT_KIND_COUNT 6
#define BEACON_PLAY_HOURS_MAX 999
#define BEACON_PLAY_MINUTES_MAX 59
#define BEACON_ZONE_COUNT (ZONE_PLEDGE_GROVE + 1)
#define BEACON_TRAINER_COUNT (TRAINER_COLRESS_2 + 1)
#define BEACON_MOVE_COUNT (MOVE_FUSION_BOLT + 1)

static int GameBeaconInfoTable_Add(GameBeaconInfoTable *table, const GameBeacon *beacon, const GameBeaconTime *time,
                         BOOL overwrite);
static u16 func_02013ebc(const GameBeacon *beacon);
static u16 func_02013f00(const GameBeacon *beacon);
static u16 func_02013f14(const GameBeacon *beacon);
static u16 func_02013f24(const GameBeacon *beacon);
static u16 func_02013f34(const GameBeacon *beacon);
static void func_02013f54(const GameBeacon *beacon, StrBuf *dest);
static u16 func_02013fb8(const GameBeacon *beacon);
static u32 func_02013fec(const GameBeacon *beacon);
static u16 func_02013ffc(const GameBeacon *beacon);
static u32 func_0201400c(const GameBeacon *beacon);
static u8 func_0201401c(const GameBeacon *beacon);
static u16 func_0201402c(const GameBeacon *beacon);
static u8 func_0201403c(const GameBeacon *beacon);
static void func_02014050(const GameBeacon *beacon, u8 *tournament, u8 *round);
static u8 func_02014098(const GameBeacon *beacon, u16 *zoneId);
static u8 GameBeacon_GetTypeFormat(int type);
static BOOL func_020142e0(const GameBeacon *beacon);
static BOOL func_020142e4(const GameBeacon *beacon);
static BOOL func_0201430c(const GameBeacon *beacon);
static BOOL func_02014324(const GameBeacon *beacon);
static BOOL func_02014328(const GameBeacon *beacon);
static BOOL func_02014340(const GameBeacon *beacon);
static BOOL func_02014358(const GameBeacon *beacon);
static BOOL func_02014378(const GameBeacon *beacon);
static BOOL func_0201438c(const GameBeacon *beacon);
static BOOL func_020143a4(const GameBeacon *beacon);
static BOOL func_020143bc(const GameBeacon *beacon);
static BOOL func_020143d4(const GameBeacon *beacon);
static BOOL func_020143e8(const GameBeacon *beacon);
static BOOL func_020143f8(const GameBeacon *beacon);
static BOOL func_0201440c(const GameBeacon *beacon);
static BOOL func_02014420(const GameBeacon *beacon);
static BOOL func_02014430(const GameBeacon *beacon);
static BOOL func_02014470(const GameBeacon *beacon);

// Whether the payload of a beacon is invalid, by format
static BOOL (*const sFormatChecks[BEACON_FORMAT_COUNT])(const GameBeacon *beacon) = {
    func_020142e0, func_020142e4, func_0201430c, func_02014324, func_02014328, func_02014340, func_02014358,
    func_02014378, func_0201438c, func_020143a4, func_020143bc, func_020143d4, func_020143e8, func_020143f8,
    func_020142e0, func_0201440c, func_02014420, func_02014430, func_02014470,
};

// By beacon type; the types from BEACON_FORMAT_TYPE_MAX on have no format
static const GameBeaconTypeFormat sTypeFormats[BEACON_FORMAT_TYPE_MAX] = {
    { 0, 0 }, { 0, 0 }, { 2, 1 }, { 2, 2 },
    { 2, 1 }, { 2, 2 }, { 1, 1 }, { 1, 2 },
    { 1, 1 }, { 1, 2 }, { 1, 1 }, { 1, 2 },
    { 1, 1 }, { 1, 2 }, { 2, 3 }, { 2, 3 },
    { 3, 4 }, { 4, 5 }, { 0, 6 }, { 5, 7 },
    { 6, 8 }, { 0, 8 }, { 7, 8 }, { 0, 9 },
    { 0, 11 }, { 8, 8 }, { 9, 8 }, { 0, 8 },
    { 3, 1 }, { 3, 1 }, { 0, 1 }, { 3, 2 },
    { 3, 2 }, { 3, 2 }, { 3, 2 }, { 5, 7 },
    { 10, 10 }, { 0, 10 }, { 2, 10 }, { 0, 10 },
    { 0, 1 }, { 11, 2 }, { 0, 2 }, { 0, 1 },
    { 12, 2 }, { 0, 10 }, { 0, 10 }, { 3, 10 },
    { 13, 6 }, { 14, 11 }, { 15, 6 }, { 16, 10 },
    { 16, 10 }, { 17, 10 }, { 17, 10 }, { 17, 10 },
    { 17, 10 }, { 0, 10 }, { 18, 10 }, { 18, 10 },
};

int GameBeaconInfoTable_TakeNew(GameBeaconInfoTable *table) {
    int count = 0;
    int index = 0;
    GameBeaconTime time;
    int found;
    GameBeacon *beacon;

    for (;;) {
        found = GameBeaconSys_GetNextNew(&index);
        if (found == GAME_BEACON_LOG_MAX) {
            break;
        }
        beacon = GameBeaconSys_GetLog(found, &time);
        if (GameBeaconInfoTable_Add(table, beacon, &time, FALSE) == GAME_BEACON_LOG_MAX) {
            break;
        }
        GameBeaconSys_ClearNew(found);
        count++;
    }
    return count;
}

BOOL GameBeaconInfoTable_PopFront(GameBeaconInfoTable *table, GameBeacon *dest, u16 *time) {
    int i;

    if (table->beacons[0].targetVersions == 0) {
        return FALSE;
    }
    *dest = table->beacons[0];
    *time = table->times[0];
    for (i = 1; i < GAME_BEACON_LOG_MAX; i++) {
        table->beacons[i - 1] = table->beacons[i];
        table->times[i - 1] = table->times[i];
    }
    sys_memset(&table->beacons[GAME_BEACON_LOG_MAX - 1], 0, sizeof(GameBeacon));
    return TRUE;
}

void GameBeaconInfoTable_MarkAllNew(GameBeaconInfoTable *table) {
    int i;

    for (i = 0; i < GAME_BEACON_LOG_MAX; i++) {
        if (table->beacons[i].targetVersions == 0) {
            break;
        }
        GameBeaconSys_MarkNew(&table->beacons[i]);
    }
}

// Stores a beacon in the list, over the one of the same trainer or in the first free entry, and returns the entry.
// When the list is full, it drops the first beacon if overwrite is set, and else returns GAME_BEACON_LOG_MAX
static int GameBeaconInfoTable_Add(GameBeaconInfoTable *table, const GameBeacon *beacon, const GameBeaconTime *time,
                         BOOL overwrite) {
    int i;

    for (i = 0; i < GAME_BEACON_LOG_MAX; i++) {
        if (table->beacons[i].targetVersions == 0 || beacon->trainerId == table->beacons[i].trainerId) {
            table->beacons[i] = *beacon;
            table->times[i] = (time->hour << 8) | time->minute;
            return i;
        }
    }
    if (overwrite == TRUE) {
        sys_memcpy32(&table->beacons[1], &table->beacons[0], sizeof(GameBeacon) * (GAME_BEACON_LOG_MAX - 1));
        sys_memcpy16(&table->times[1], &table->times[0], sizeof(u16) * (GAME_BEACON_LOG_MAX - 1));
        table->beacons[GAME_BEACON_LOG_MAX - 1] = *beacon;
        table->times[GAME_BEACON_LOG_MAX - 1] = (time->hour << 8) | time->minute;
        return GAME_BEACON_LOG_MAX - 1;
    }
    return GAME_BEACON_LOG_MAX;
}

int GameBeaconInfoTable_RingIndex(GameBeaconInfoTable *table, int offset) {
    int index = table->head;

    if (offset < 0) {
        index = index + offset + GAME_BEACON_LOG_MAX;
    } else {
        index += offset;
    }
    return index % GAME_BEACON_LOG_MAX;
}

int GameBeaconInfoTable_RingAdd(GameBeaconInfoTable *table, const GameBeacon *beacon, u16 time, BOOL *isNew) {
    int i;
    int index;

    if (isNew != NULL) {
        *isNew = FALSE;
    }
    for (i = 0; i < GAME_BEACON_LOG_MAX; i++) {
        index = GameBeaconInfoTable_RingIndex(table, i);
        if (table->beacons[index].targetVersions == 0) {
            break;
        }
        if (beacon->trainerId == table->beacons[index].trainerId) {
            table->beacons[index] = *beacon;
            table->times[index] = time;
            return i;
        }
    }
    if (isNew != NULL) {
        *isNew = TRUE;
    }
    index = GameBeaconInfoTable_RingIndex(table, -1);
    table->beacons[index] = *beacon;
    table->times[index] = time;
    table->head = index;
    if (table->count < GAME_BEACON_LOG_MAX) {
        table->count++;
    }
    return 0;
}

BOOL GameBeaconInfoTable_RingGet(GameBeaconInfoTable *table, GameBeacon *dest, u16 *time, int offset) {
    int index = GameBeaconInfoTable_RingIndex(table, offset);

    if (table->beacons[index].targetVersions == 0) {
        return FALSE;
    }
    *dest = table->beacons[index];
    *time = table->times[index];
    return TRUE;
}

u8 GameBeaconInfoTable_GetCount(GameBeaconInfoTable *table) {
    return table->count;
}

GameBeacon *GameBeacon_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(GameBeacon), TRUE, "game_beacon_accessor.c", 461);
}

GameBeaconInfoTable *GameBeaconInfoTable_Alloc(HeapID heapId) {
    GameBeaconInfoTable *table =
        GFL_HeapAllocate(heapId, sizeof(GameBeaconInfoTable), TRUE, "game_beacon_accessor.c", 475);

    table->head = 0;
    table->count = 0;
    return table;
}

BOOL GameBeacon_IsInvalid(const GameBeacon *beacon) {
    if (beacon->type == 0 || beacon->type >= GAME_BEACON_TYPE_MAX) {
        return TRUE;
    }
    if (beacon->recentKind >= BEACON_RECENT_KIND_COUNT) {
        return TRUE;
    }
    if (beacon->surveyRank >= 6) {
        return TRUE;
    }
    if (beacon->playHours > BEACON_PLAY_HOURS_MAX || beacon->playMinutes > BEACON_PLAY_MINUTES_MAX) {
        return TRUE;
    }
    if (PassPower_IsIDInvalid(beacon->passPower)) {
        return TRUE;
    }
    if (beacon->zoneId >= BEACON_ZONE_COUNT || beacon->parentZoneId >= BEACON_ZONE_COUNT) {
        return TRUE;
    }
    if (beacon->unk04 > 99999 || beacon->unk08 > 99999) {
        return TRUE;
    }
    if (sFormatChecks[GameBeacon_GetTypeFormat(beacon->type)](beacon)) {
        return TRUE;
    }
    return FALSE;
}

BOOL GameBeacon_IsGiftType(const GameBeacon *beacon) {
    switch (beacon->type) {
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x32:
        return TRUE;
    }
    return FALSE;
}

void GameBeacon_GetName(const GameBeacon *beacon, StrBuf *dest) {
    GFL_StrBufLoadFixedString(dest, beacon->name, 8);
}

u32 GameBeacon_GetVersion(const GameBeacon *beacon) {
    return beacon->version;
}

u32 GameBeacon_GetLanguage(const GameBeacon *beacon) {
    return beacon->language;
}

u8 GameBeacon_GetCountry(const GameBeacon *beacon) {
    return beacon->country;
}

u8 GameBeacon_GetRegion(const GameBeacon *beacon) {
    return beacon->region;
}

u8 func_02013cd8(GameBeacon *beacon) {
    u32 answer = func_0200ec3c(beacon->surveyAnswers, 0x1a);

    if (answer == 0) {
        return 0xff;
    }
    return answer;
}

u8 func_02013cf0(GameBeacon *beacon) {
    u32 answer = func_0200ec3c(beacon->surveyAnswers, 0x19);

    if (answer == 0) {
        return 0xff;
    }
    return answer;
}

void GameBeacon_GetCGearRecord(const GameBeacon *beacon, PMSData *dest) {
    dest->type = beacon->cgear0;
    dest->id = beacon->cgear1;
    dest->words[0] = beacon->cgear2;
    dest->words[1] = beacon->cgear3;
}

u8 GameBeacon_GetSurveyRank(const GameBeacon *beacon) {
    return beacon->surveyRank;
}

void func_02013d38(u16 *dest, const GameBeacon *beacon) {
    u32 color = beacon->favoriteColor;
    const u16 *colors = func_0207c3b4();

    *dest = colors[color & 0xf];
}

u8 GameBeacon_GetFavoriteColor(const GameBeacon *beacon) {
    return beacon->favoriteColor;
}

u16 GameBeacon_GetTrainerId(const GameBeacon *beacon) {
    return beacon->trainerId;
}

u8 GameBeacon_GetTrainerViewWithGender(const GameBeacon *beacon) {
    if (beacon->gender == 0) {
        return beacon->trainerView;
    }
    return beacon->trainerView + 8;
}

u32 GameBeacon_GetGender(const GameBeacon *beacon) {
    return beacon->gender;
}

void GameBeacon_GetPlayTime(const GameBeacon *beacon, u16 *hours, u16 *minutes) {
    *hours = beacon->playHours;
    *minutes = beacon->playMinutes;
}

u32 GameBeacon_GetPassPower(const GameBeacon *beacon) {
    return beacon->passPower;
}

u16 GameBeacon_GetZone(const GameBeacon *beacon) {
    return beacon->zoneId;
}

u16 GameBeacon_GetParentZone(const GameBeacon *beacon) {
    return beacon->parentZoneId;
}

u32 func_02013db8(const GameBeacon *beacon) {
    return beacon->unk08;
}

u32 func_02013dc0(const GameBeacon *beacon) {
    return beacon->unk04;
}

u8 func_02013dc8(const GameBeacon *beacon) {
    return beacon->unk5B;
}

u8 GameBeacon_GetMedalCount(const GameBeacon *beacon) {
    return beacon->medalCount;
}

void GameBeacon_GetGreeting(const GameBeacon *beacon, StrBuf *dest) {
    GFL_StrBufClear(dest);
    GFL_StrBufLoadFixedString(dest, beacon->greeting, 9);
}

void func_02013df4(const GameBeacon *beacon, StrBuf *dest) {
    if (beacon->type != 0x18) {
        GFL_StrBufClear(dest);
        return;
    }
    GFL_StrBufLoadString(dest, beacon->payload.message.text);
}

void func_02013e14(const GameBeacon *beacon, StrBuf *dest) {
    if (beacon->type != 0x31) {
        GFL_StrBufClear(dest);
        return;
    }
    GFL_StrBufLoadString(dest, beacon->payload.text);
}

u32 GameBeacon_GetRecentKind(const GameBeacon *beacon) {
    u32 kind = beacon->recentKind;

    if (kind >= BEACON_RECENT_KIND_COUNT) {
        kind = GAME_BEACON_RECENT_NONE;
    }
    return kind;
}

u16 GameBeacon_GetRecentTrainer(const GameBeacon *beacon) {
    u32 trainerId = beacon->recentValue;

    switch (beacon->recentKind) {
    case 2:
    case 3:
    case 4:
        if (trainerId == 0 || trainerId >= BEACON_TRAINER_COUNT) {
            trainerId = 1;
        }
        return trainerId;
    }
    return 1;
}

u16 GameBeacon_GetRecentSpecies(const GameBeacon *beacon) {
    u32 species = beacon->recentValue;

    switch (beacon->recentKind) {
    case 0:
    case 1:
        if (species == SPECIES_NONE || species > SPECIES_GENESECT) {
            species = SPECIES_PATRAT;
        }
        return species;
    }
    return SPECIES_PATRAT;
}

u16 GameBeacon_GetType(const GameBeacon *beacon) {
    u16 type = beacon->type;

    if (type == 0 || type >= GAME_BEACON_TYPE_MAX) {
        type = 1;
    }
    return type;
}

// The readers of a type's value: it, or a default for another type

static u16 func_02013ebc(const GameBeacon *beacon) {
    switch (beacon->type) {
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0xe:
    case 0xf:
    case 0x11:
    case 0x26:
        return beacon->arg.value;
    }
    return SPECIES_PATRAT;
}

static u16 func_02013f00(const GameBeacon *beacon) {
    switch (beacon->type) {
    case 0x13:
    case 0x23:
        return beacon->payload.value;
    }
    return ITEM_POTION;
}

static u16 func_02013f14(const GameBeacon *beacon) {
    if (beacon->type == 0x24) {
        return beacon->payload.value;
    }
    return MOVE_POUND;
}

static u16 func_02013f24(const GameBeacon *beacon) {
    if (beacon->type == 0x19) {
        return beacon->payload.gift.species;
    }
    return SPECIES_PATRAT;
}

static u16 func_02013f34(const GameBeacon *beacon) {
    if (beacon->type == 0x1a) {
        return beacon->payload.gift.item;
    }
    return ITEM_POTION;
}

u16 GameBeacon_GetGiftPassPower(const GameBeacon *beacon) {
    if (beacon->type == 0x32) {
        return beacon->payload.gift.passPower;
    }
    return 0;
}

// Loads the nickname of the types that send one. The switch does nothing; its cases are the types whose format has
// a nickname, and 0x17 is a guess, since the code only shows that the cases from 0x10 to 0x26 are dense
static void func_02013f54(const GameBeacon *beacon, StrBuf *dest) {
    GFL_StrBufClear(dest);
    GFL_StrBufLoadFixedString(dest, beacon->payload.text, GAME_BEACON_TEXT_LEN + 1);
    switch (beacon->type) {
    case 0x10:
    case 0x11:
    case 0x17:
    case 0x1c:
    case 0x1d:
    case 0x1f:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x26:
    case 0x2f:
        return;
    }
}

static u16 func_02013fb8(const GameBeacon *beacon) {
    switch (beacon->type) {
    case 0x6:
    case 0x7:
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
        return beacon->payload.value;
    }
    return 1;
}

static u32 func_02013fec(const GameBeacon *beacon) {
    if (beacon->type == 0x16) {
        return beacon->payload.value32;
    }
    return 1;
}

static u16 func_02013ffc(const GameBeacon *beacon) {
    if (beacon->type == 0x14) {
        return beacon->payload.value;
    }
    return 1;
}

static u32 func_0201400c(const GameBeacon *beacon) {
    if (beacon->type == 0x29) {
        return beacon->payload.value32;
    }
    return 1;
}

static u8 func_0201401c(const GameBeacon *beacon) {
    if (beacon->type == 0x2c) {
        return beacon->payload.value8;
    }
    return 0;
}

static u16 func_0201402c(const GameBeacon *beacon) {
    if (beacon->type == 0x30) {
        return beacon->payload.value;
    }
    return 0;
}

static u8 func_0201403c(const GameBeacon *beacon) {
    switch (beacon->type) {
    case 0x33:
    case 0x34:
        return beacon->payload.value8;
    }
    return 0;
}

static void func_02014050(const GameBeacon *beacon, u8 *tournament, u8 *round) {
    switch (beacon->type) {
    case 0x35:
    case 0x36:
    case 0x38:
        *tournament = beacon->payload.pair.a;
        *round = beacon->payload.pair.b;
        break;
    case 0x37:
        *tournament = beacon->payload.pair.a;
        *round = beacon->payload.pair.b + 1;
        break;
    default:
        *tournament = 0;
        *round = 0;
        break;
    }
}

static u8 func_02014098(const GameBeacon *beacon, u16 *zoneId) {
    switch (beacon->type) {
    case 0x3a:
    case 0x3b:
        *zoneId = beacon->payload.withExtra.value;
        return beacon->payload.withExtra.extra + 1;
    }
    *zoneId = 0;
    return 0;
}

static u8 GameBeacon_GetTypeFormat(int type) {
    if (type >= BEACON_FORMAT_TYPE_MAX) {
        return BEACON_FORMAT_NONE;
    }
    return sTypeFormats[type].format;
}

u8 func_020140cc(int type) {
    if (type >= BEACON_FORMAT_TYPE_MAX) {
        return 0;
    }
    return sTypeFormats[type].unk1;
}

void GameBeacon_SetMessageWords(const GameBeacon *beacon, WordSet *wordSet, HeapID heapId) {
    StrBuf *str;
    u32 format;
    u8 tournament;
    u8 round;
    u16 zoneId;

    GFL_WordSetClearAll(wordSet);
    str = GFL_StrBufCreate(0x21, heapId);
    if (str == NULL) {
        return;
    }
    GameBeacon_GetName(beacon, str);
    func_0202437c(wordSet, 0, str, 0, 1, 2);
    format = GameBeacon_GetTypeFormat(GameBeacon_GetType(beacon));
    switch (format) {
    case BEACON_FORMAT_TRAINER: {
        u16 trainerId = func_02013fb8(beacon);

        loadTrainerTypeToStrbuf(wordSet, 1, trainerId);
        loadTrainerNamesToStrbuf(wordSet, 2, trainerId);
        break;
    }
    case BEACON_FORMAT_SPECIES:
    case BEACON_FORMAT_SPECIES_NICKNAME:
        WordSet_LoadSpeciesName(wordSet, 1, func_02013ebc(beacon));
        if (format == BEACON_FORMAT_SPECIES) {
            break;
        }
        // fallthrough
    case BEACON_FORMAT_NICKNAME:
        func_02013f54(beacon, str);
        func_0202437c(wordSet, 2, str, 0, 1, 2);
        break;
    case BEACON_FORMAT_ITEM:
        loadItemTextNameToStrbuf(wordSet, 1, func_02013f00(beacon));
        break;
    case BEACON_FORMAT_NUMBER3:
        WordSetNumber(wordSet, 1, func_02013ffc(beacon), 3, 0, TRUE);
        break;
    case BEACON_FORMAT_NUMBER3_LONG:
        WordSetNumber(wordSet, 1, func_02013fec(beacon), 3, 0, TRUE);
        break;
    case BEACON_FORMAT_GIFT_SPECIES:
        WordSet_LoadSpeciesName(wordSet, 1, func_02013f24(beacon));
        break;
    case BEACON_FORMAT_GIFT_ITEM:
        loadItemNameToStrbuf(wordSet, 1, func_02013f34(beacon));
        break;
    case BEACON_FORMAT_MOVE:
        loadMoveNameToStrbuf(wordSet, 1, func_02013f14(beacon));
        break;
    case BEACON_FORMAT_NUMBER1:
        WordSetNumber(wordSet, 1, func_0201400c(beacon), 1, 0, TRUE);
        break;
    case BEACON_FORMAT_BATTLE_INSTITUTE:
        loadBattleInstituteMsgForDisplay(wordSet, 1, func_0201401c(beacon));
        break;
    case BEACON_FORMAT_PASS_POWER:
        loadPassPowerToStrbuf(wordSet, 1, func_0201402c(beacon));
        break;
    case BEACON_FORMAT_TEXT:
        func_02013e14(beacon, str);
        func_0202437c(wordSet, 1, str, GameBeacon_GetGender(beacon), 1, 2);
        break;
    case BEACON_FORMAT_POKEWOOD:
        loadPokewoodLineToStrbuf(wordSet, 1, func_0201403c(beacon));
        break;
    case BEACON_FORMAT_TOURNAMENT:
        func_02014050(beacon, &tournament, &round);
        func_0202483c(wordSet, 1, tournament);
        WordSetNumber(wordSet, 2, round, 1, 0, TRUE);
        break;
    case BEACON_FORMAT_ZONE: {
        u8 number = func_02014098(beacon, &zoneId);

        loadLocationNameToStrbuf(wordSet, 1, ZoneData_GetPlaceNameID(zoneId));
        WordSetNumber(wordSet, 2, number, 2, 0, TRUE);
        break;
    }
    }
    GFL_StrBufFree(str);
}

// The checks of a beacon's payload, by format: whether it is invalid

static BOOL func_020142e0(const GameBeacon *beacon) {
    return FALSE;
}

// Black and White have other trainers
static BOOL func_020142e4(const GameBeacon *beacon) {
    if (beacon->version == VERSION_WHITE || beacon->version == VERSION_BLACK) {
        return TRUE;
    }
    if (beacon->payload.value == 0 || beacon->payload.value >= BEACON_TRAINER_COUNT) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_0201430c(const GameBeacon *beacon) {
    if (beacon->arg.value == SPECIES_NONE || beacon->arg.value > SPECIES_GENESECT) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_02014324(const GameBeacon *beacon) {
    return FALSE;
}

static BOOL func_02014328(const GameBeacon *beacon) {
    if (beacon->arg.value == SPECIES_NONE || beacon->arg.value > SPECIES_GENESECT) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_02014340(const GameBeacon *beacon) {
    if (beacon->payload.value == ITEM_NONE || beacon->payload.value > ITEM_LAST) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_02014358(const GameBeacon *beacon) {
    if (beacon->playHours > BEACON_PLAY_HOURS_MAX || beacon->playMinutes > BEACON_PLAY_MINUTES_MAX) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_02014378(const GameBeacon *beacon) {
    if (beacon->payload.value32 > 99999) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_0201438c(const GameBeacon *beacon) {
    if (beacon->payload.gift.species == SPECIES_NONE || beacon->payload.gift.species > SPECIES_GENESECT) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_020143a4(const GameBeacon *beacon) {
    if (beacon->payload.gift.item == ITEM_NONE || beacon->payload.gift.item > ITEM_LAST) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_020143bc(const GameBeacon *beacon) {
    if (beacon->payload.value == MOVE_NONE || beacon->payload.value >= BEACON_MOVE_COUNT) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_020143d4(const GameBeacon *beacon) {
    if (beacon->payload.value32 == 0 || beacon->payload.value32 > 7) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_020143e8(const GameBeacon *beacon) {
    if (beacon->payload.value8 > 6) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_020143f8(const GameBeacon *beacon) {
    if (!PassPower_IsIDValid(beacon->payload.value)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_0201440c(const GameBeacon *beacon) {
    if (!PassPower_IsIDValid(beacon->payload.gift.passPower)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_02014420(const GameBeacon *beacon) {
    if (beacon->payload.value8 >= 0x29) {
        return TRUE;
    }
    return FALSE;
}

// The tournament, and the round, whose range depends on the type
static BOOL func_02014430(const GameBeacon *beacon) {
    u8 min;
    u8 max;

    if (beacon->payload.pair.a == 0 || beacon->payload.pair.a >= 0x10) {
        return TRUE;
    }
    if (beacon->type == 0x35) {
        max = 0;
        min = 0;
    } else if (beacon->type == 0x37) {
        min = 0;
        max = 2;
    } else {
        min = 1;
        max = 3;
    }
    if (beacon->payload.pair.b < min || beacon->payload.pair.b > max) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_02014470(const GameBeacon *beacon) {
    if (beacon->payload.withExtra.value != ZONE_WHITE_TREEHOLLOW
        && beacon->payload.withExtra.value != ZONE_BLACK_TOWER) {
        return TRUE;
    }
    if (beacon->payload.withExtra.extra >= 10) {
        return TRUE;
    }
    return FALSE;
}
