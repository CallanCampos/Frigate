// resonance_resort_data.c: the Join Avenue's saved data. The people on the avenue, the lists of people met through
// beacons, the entries and records that stand in for them, and the avenue's own info: how each is filled, checked,
// merged and read or written field by field. Named by the file name string at 0x0209a820.
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0): getYearMonthDay, getSpecialJoinAveTextToBuf,
// JoinAvenuePerson_IsEmpty, joinAveTextHandler, JoinAvenuePerson_SetParam, JoinAvenuePersonList_Create,
// JoinAvenuePersonList_Free, JoinAvenuePersonList_GetCount, JoinAvenuePersonList_Get, JoinAvenue_GetParam,
// setImportantJABlockAddresses and gfRand. The rest are ours
#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/script_text_banks.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "field/survey.h"
#include "field/unity_tower.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/rtc_cache.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/adventure.h"
#include "save/event_work.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "system/country_region.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/resort_binary.h"
#include "system/resort_work.h"
#include "system/str_tool.h"
#include "system/union_view.h"

// The script message file of the default texts, by trainer ID
#define MSG_JOIN_AVENUE_DEFAULTS SCRIPT_TEXT_GLOBAL_10685
#define MSG_JOIN_AVENUE_DEFAULT_GREETING 0x776
#define MSG_JOIN_AVENUE_DEFAULT_MESSAGE1 0x6ea
#define MSG_JOIN_AVENUE_DEFAULT_MESSAGE2 0x730
// The system message file of the avenue's default name, and the script message of its default second name
#define MSG_JOIN_AVENUE_NAMES TEXT_BANK_PLACE_NAMES
#define MSG_JOIN_AVENUE_DEFAULT_NAME 0x79
#define MSG_JOIN_AVENUE_DEFAULT_NAME2 0xac

// Set once the player has entered the Hall of Fame
#define EVENT_FLAG_HALL_OF_FAME 0x960
// What a person's adventure gives as species for an egg
#define JOIN_AVE_SPECIES_EGG 650

// JOIN_AVE_PARAM_STATUS: how a person last came, replacing one whose stay was over, into an empty place, or as a merge
// that changed the trainer type or not
#define JOIN_AVE_STATUS_NEW 1
#define JOIN_AVE_STATUS_ADDED 2
#define JOIN_AVE_STATUS_CHANGED 3
#define JOIN_AVE_STATUS_UPDATED 4

// How long a person with JoinAvenueShopChoice.unk0 set stays, in seconds (sStayDurations has the others)
#define JOIN_AVE_STAY_SHORT (3 * 60 * 60)

typedef void (*JoinAvenueDataFillFunc)(JoinAvenueData *data, GameData *gameData);
typedef BOOL (*JoinAvenueDataCheckFunc)(JoinAvenueData *data);

typedef BOOL (*JoinAvenuePersonMatchFunc)(JoinAvenuePerson *person, const void *source);
typedef u32 (*JoinAvenuePersonMergeFunc)(JoinAvenuePerson *person, const void *source);
typedef BOOL (*JoinAvenueRecordMatchFunc)(JoinAvenueRecord *record, const void *source);
typedef u32 (*JoinAvenueRecordMergeFunc)(JoinAvenueRecord *record, const void *source);

// What a person matches and merges by kind of source: a visitor, a person or an entry
typedef struct {
    JoinAvenuePersonMatchFunc personMatches;
    JoinAvenuePersonMergeFunc personMerge;
    JoinAvenueRecordMatchFunc recordMatches;
    JoinAvenueRecordMergeFunc recordMerge;
} JoinAvenueMergeFuncs;

typedef struct {
    JoinAvenueInfo *info;
    JoinAvenueOccupants *occupants;
} JoinAvenueBlocks;

static u16 getYearMonthDay(const RTCDate *date);
static void JoinAvenue_UnpackDate(u16 packed, RTCDate *date);
static void JoinAvenueShop_Init(JoinAvenueShop *shop);
static void JoinAvenueProfile_SetFromPlayerInfo(JoinAvenueProfile *profile, PlayerInfo *playerInfo);
static void JoinAvenueProfile_SetFromGameData(JoinAvenueProfile *profile, GameData *gameData);
static void JoinAvenueProfile_SetFromBeacon(JoinAvenueProfile *profile, const GameBeacon *beacon);
static BOOL JoinAvenueProfile_IsValid(const JoinAvenueProfile *profile);
static BOOL JoinAvenueProfile_IsEmpty(const JoinAvenueProfile *profile);
static void JoinAvenueProfile_GetName(const JoinAvenueProfile *profile, u16 *dest);
static BOOL JoinAvenueProfile_IsSame(const JoinAvenueProfile *profile, const JoinAvenueProfile *other);
static void JoinAvenueProfile_GetGreeting(const JoinAvenueProfile *profile, u16 *dest);
static void JoinAvenueShopChoice_SetFromGameData(JoinAvenueShopChoice *choice, GameData *gameData);
static BOOL JoinAvenueShopChoice_IsValid(const JoinAvenueShopChoice *choice);
static void JoinAvenueData_FillAdventure(JoinAvenueData *data, GameData *gameData);
static BOOL JoinAvenueData_CheckAdventure(JoinAvenueData *data);
static void JoinAvenueData_FillRecords1(JoinAvenueData *data, GameData *gameData);
static BOOL JoinAvenueData_CheckRecords1(JoinAvenueData *data);
static void JoinAvenueData_FillRecords2(JoinAvenueData *data, GameData *gameData);
static BOOL JoinAvenueData_CheckRecords2(JoinAvenueData *data);
static void JoinAvenueData_FillValues(JoinAvenueData *data, GameData *gameData);
static BOOL JoinAvenueData_CheckValues(JoinAvenueData *data);
static void JoinAvenueData_FillRecent(JoinAvenueData *data, GameData *gameData);
static BOOL JoinAvenueData_CheckRecent(JoinAvenueData *data);
static void JoinAvenueData_FillMessage1(JoinAvenueData *data, GameData *gameData);
static BOOL JoinAvenueData_CheckMessage1(JoinAvenueData *data);
static void getSpecialJoinAveTextToBuf(const JoinAvenueMessage *message, u16 *dest, int trainerId);
static void JoinAvenueData_FillMessage2(JoinAvenueData *data, GameData *gameData);
static BOOL JoinAvenueData_CheckMessage2(JoinAvenueData *data);
static void JoinAvenue_GetMessage2(const JoinAvenueMessage *message, u16 *dest, int trainerId);
static BOOL JoinAvenuePerson_MatchesVisitor(JoinAvenuePerson *person, const JoinAvenueVisitor *visitor);
static BOOL JoinAvenuePerson_MatchesPerson(JoinAvenuePerson *person, const JoinAvenuePerson *other);
static BOOL JoinAvenuePerson_MatchesEntry(JoinAvenuePerson *person, const JoinAvenueEntry *entry);
static void JoinAvenuePerson_Stamp(JoinAvenuePerson *person);
static BOOL JoinAvenuePerson_SetFromVisitor(JoinAvenuePerson *person, const JoinAvenueVisitor *visitor);
static BOOL JoinAvenuePerson_Copy(JoinAvenuePerson *person, const JoinAvenuePerson *src);
static u32 JoinAvenuePerson_MergeVisitor(JoinAvenuePerson *person, const JoinAvenueVisitor *visitor);
static u32 JoinAvenuePerson_MergePerson(JoinAvenuePerson *person, const JoinAvenuePerson *src);
static u32 JoinAvenuePerson_MergeEntry(JoinAvenuePerson *person, const JoinAvenueEntry *entry);
static BOOL JoinAvenuePerson_IsExpired(const JoinAvenuePerson *person, s64 now);
static u16 JoinAvenueVisitor_GetTrainerId(const JoinAvenueVisitor *visitor);
static BOOL JoinAvenueEntry_Merge(JoinAvenueEntry *entry, const JoinAvenueEntry *src);
static BOOL JoinAvenueEntry_Copy(JoinAvenueEntry *entry, const JoinAvenueEntry *src);
static JoinAvenueEntry *JoinAvenueEntryList_FindEmpty(JoinAvenueEntryList *list, BOOL skipFirst);
static JoinAvenueEntry *JoinAvenueEntryList_Find(JoinAvenueEntryList *list, const JoinAvenueProfile *profile);
static JoinAvenuePerson *JoinAvenuePersonList_FindEmpty(JoinAvenuePersonList *list);
static JoinAvenuePerson *JoinAvenuePersonList_Find(JoinAvenuePersonList *list, const JoinAvenueProfile *profile);
static JoinAvenuePerson *JoinAvenuePersonList_FindExpired(JoinAvenuePersonList *list);
static void JoinAvenuePerson_SetupShop(JoinAvenuePerson *person, ResortShopData *shops);
static void JoinAvenuePerson_AddVisitorData(JoinAvenuePerson *person, const JoinAvenueVisitor *visitor);
static void JoinAvenuePerson_SetupShopKind(JoinAvenuePerson *person, ResortShopData *shops);
static BOOL JoinAvenueRecord_MatchesVisitor(JoinAvenueRecord *record, const JoinAvenueVisitor *visitor);
static BOOL JoinAvenueRecord_MatchesPerson(JoinAvenueRecord *record, const JoinAvenuePerson *person);
static BOOL JoinAvenueRecord_MatchesEntry(JoinAvenueRecord *record, const JoinAvenueEntry *entry);
static u32 JoinAvenueRecord_MergeVisitor(JoinAvenueRecord *record, const JoinAvenueVisitor *visitor);
static u32 JoinAvenueRecord_MergePerson(JoinAvenueRecord *record, const JoinAvenuePerson *person);
static u32 JoinAvenueRecord_MergeEntry(JoinAvenueRecord *record, const JoinAvenueEntry *entry);
static u32 JoinAvenue_GetDefaultShop(u16 trainerId, u16 version);
static void JoinAvenueInfo_AddVisitorId(JoinAvenueInfo *info, u32 id);
static BOOL JoinAvenueInfo_HasVisitorId(const JoinAvenueInfo *info, u32 id);
static u32 gfRand(u32 seed);

// gfRand's generator: a global constant that the code folds, so nothing refers to it
#define GF_RAND_MULTIPLIER 0x5d583d6d6c078979ULL
const u64 GF_RAND_INCREMENT = 0x26a693;

// The most each value of a person's extra data 3 can be
static const u8 sValueMax[16] = { 4, 2, 5, 2, 4, 8, 2, 2, 3, 2, 9, 5, 2, 2, 2, 2 };

static const JoinAvenueDataCheckFunc sDataCheckFuncs[JOIN_AVE_DATA_COUNT] = {
    JoinAvenueData_CheckAdventure, JoinAvenueData_CheckRecords1, JoinAvenueData_CheckRecords2,
    JoinAvenueData_CheckValues,    JoinAvenueData_CheckRecent,   JoinAvenueData_CheckMessage1,
    JoinAvenueData_CheckMessage2,
};

static const JoinAvenueDataFillFunc sDataFillFuncs[JOIN_AVE_DATA_COUNT] = {
    JoinAvenueData_FillAdventure, JoinAvenueData_FillRecords1, JoinAvenueData_FillRecords2, JoinAvenueData_FillValues,
    JoinAvenueData_FillRecent,    JoinAvenueData_FillMessage1, JoinAvenueData_FillMessage2,
};

// How long a person stays, by the number of extra data the person brought
static const u32 sStayDurations[JOIN_AVE_DATA_COUNT + 1] = {
    10 * 60, 20 * 60, 30 * 60, 40 * 60, 50 * 60, 60 * 60, 70 * 60, 24 * 60 * 60,
};

static const JoinAvenueMergeFuncs sMergeFuncs[] = {
    {
        (JoinAvenuePersonMatchFunc)JoinAvenuePerson_MatchesVisitor,
        (JoinAvenuePersonMergeFunc)JoinAvenuePerson_MergeVisitor,
        (JoinAvenueRecordMatchFunc)JoinAvenueRecord_MatchesVisitor,
        (JoinAvenueRecordMergeFunc)JoinAvenueRecord_MergeVisitor,
    },
    {
        (JoinAvenuePersonMatchFunc)JoinAvenuePerson_MatchesPerson,
        (JoinAvenuePersonMergeFunc)JoinAvenuePerson_MergePerson,
        (JoinAvenueRecordMatchFunc)JoinAvenueRecord_MatchesPerson,
        (JoinAvenueRecordMergeFunc)JoinAvenueRecord_MergePerson,
    },
    {
        (JoinAvenuePersonMatchFunc)JoinAvenuePerson_MatchesEntry,
        (JoinAvenuePersonMergeFunc)JoinAvenuePerson_MergeEntry,
        (JoinAvenueRecordMatchFunc)JoinAvenueRecord_MatchesEntry,
        (JoinAvenueRecordMergeFunc)JoinAvenueRecord_MergeEntry,
    },
};

// The kind of the shop that a trainer ID gives by default, by the ID's remainder of 7
static const u8 sDefaultShopKinds[7][7] = {
    { 0, 2, 7, 4, 6, 1, 3 }, { 1, 3, 0, 2, 7, 4, 6 }, { 4, 6, 1, 3, 0, 2, 7 }, { 2, 7, 4, 6, 1, 3, 0 },
    { 3, 0, 2, 7, 4, 6, 1 }, { 6, 1, 3, 0, 2, 7, 4 }, { 7, 4, 6, 1, 3, 0, 2 },
};

// Nothing refers to it
u32 data_02141810;
static JoinAvenueBlocks sJoinAvenue;

// The visitor's data in a beacon, which the beacon may be const for
static inline JoinAvenueBeaconPayload *GameBeacon_GetAvenuePayload(const GameBeacon *beacon) {
    return (JoinAvenueBeaconPayload *)&beacon->payload.avenue;
}

static u16 getYearMonthDay(const RTCDate *date) {
    return ((date->year & 0x7f) << 9) | ((date->month & 0xf) << 5) | (date->day & 0x1f);
}

static void JoinAvenue_UnpackDate(u16 packed, RTCDate *date) {
    sys_memset(date, 0, sizeof(RTCDate));
    date->year = (packed >> 9) & 0x7f;
    date->month = (packed >> 5) & 0xf;
    date->day = packed & 0x1f;
}

static void JoinAvenueShop_Init(JoinAvenueShop *shop) {
    sys_memset(shop, 0, sizeof(JoinAvenueShop));
    shop->id = 0xffff;
}

u32 func_020363e0(JoinAvenueShop *shop, u32 which) {
    switch (which) {
    case 0:
        return shop->id;
    case 1:
        return shop->level;
    case 2:
        return shop->unk2;
    case 3:
        return shop->flags;
    }
    return 0;
}

void func_0203640c(JoinAvenueShop *shop, u32 which, u32 value) {
    switch (which) {
    case 0:
        shop->id = value;
        break;
    case 1:
        shop->level = value;
        break;
    case 2:
        shop->unk2 = value;
        break;
    case 3:
        shop->flags = value;
        break;
    }
}

BOOL func_02036434(JoinAvenueShop *shop, u32 bit) {
    if (shop->flags & (1 << bit)) {
        return TRUE;
    }
    return FALSE;
}

void func_02036448(JoinAvenueShop *shop, u32 bit, BOOL set) {
    if (set) {
        shop->flags |= 1 << bit;
    } else {
        shop->flags &= ~(1 << bit);
    }
}

static void JoinAvenueProfile_SetFromPlayerInfo(JoinAvenueProfile *profile, PlayerInfo *playerInfo) {
    sys_memcpy(GetPlayerName(playerInfo), profile->name, sizeof(profile->name));
    profile->trainerId = getTrainerID(playerInfo);
    profile->country = UnityTowerVisitor_GetCountry(playerInfo);
    profile->region = UnityTowerVisitor_GetProvince(playerInfo);
    profile->gender = getTrainerGender(playerInfo);
    profile->version = func_02008bfc(playerInfo);
    profile->language = TrainerInfo_GetRegion(playerInfo);
    profile->trainerType = UnionView_GetTrainerType(func_02008bf4(playerInfo));
}

static void JoinAvenueProfile_SetFromGameData(JoinAvenueProfile *profile, GameData *gameData) {
    void *answers;
    PlayTime *playTime;

    sys_memset(profile, 0, sizeof(JoinAvenueProfile));
    JoinAvenueProfile_SetFromPlayerInfo(profile, GetGameDataPlayerInfo(gameData));
    sys_memcpy(func_0200c93c(getTrainerGameInfoAddress(GameData_GetSaveControl(gameData))), profile->greeting,
               sizeof(profile->greeting));
    answers = func_0200ec38(func_0200ec2c(GameData_GetSaveControl(gameData)));
    profile->job = func_0200ec3c(answers, 26);
    profile->hobby = func_0200ec3c(answers, 25);
    playTime = func_02017a40(gameData);
    profile->playHours = func_02008cec(playTime);
    profile->playMinutes = func_02008cf0(playTime);
}

static void JoinAvenueProfile_SetFromBeacon(JoinAvenueProfile *profile, const GameBeacon *beacon) {
    void *answers;

    sys_memset(profile, 0, sizeof(JoinAvenueProfile));
    sys_memcpy(beacon->name, profile->name, sizeof(profile->name));
    sys_memcpy(beacon->greeting, profile->greeting, sizeof(profile->greeting));
    profile->trainerId = beacon->trainerId;
    profile->gender = beacon->gender;
    profile->version = beacon->version;
    profile->language = beacon->language;
    profile->country = beacon->country;
    profile->region = beacon->region;
    if (profile->gender == GENDER_MALE) {
        profile->trainerType = UnionView_GetTrainerType(beacon->trainerView);
    } else {
        profile->trainerType = UnionView_GetTrainerType(beacon->trainerView + 8);
    }
    answers = (void *)beacon->surveyAnswers;
    profile->job = func_0200ec3c(answers, 26);
    profile->hobby = func_0200ec3c(answers, 25);
    profile->playHours = beacon->playHours;
    profile->playMinutes = beacon->playMinutes;
}

static BOOL JoinAvenueProfile_IsValid(const JoinAvenueProfile *profile) {
    BOOL found;
    int i;

    if (profile->gender != GENDER_FEMALE && profile->gender != GENDER_MALE) {
        return FALSE;
    }
    found = FALSE;
    for (i = 0; i <= 15; i++) {
        if (profile->trainerType == UnionView_GetTrainerType(i)) {
            found = TRUE;
            break;
        }
    }
    if (!found) {
        return FALSE;
    }
    if (Country_IsValidPlace(profile->country, profile->region, profile->language)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL JoinAvenueProfile_IsEmpty(const JoinAvenueProfile *profile) {
    if (profile->version == 0 || profile->name[0] == 0) {
        return TRUE;
    }
    return FALSE;
}

static void JoinAvenueProfile_GetName(const JoinAvenueProfile *profile, u16 *dest) {
    sys_memcpy(profile->name, dest, sizeof(profile->name));
    dest[NELEMS(profile->name)] = GFL_StrBufGetTerminator();
}

static BOOL JoinAvenueProfile_IsSame(const JoinAvenueProfile *profile, const JoinAvenueProfile *other) {
    u16 name[8];
    u16 otherName[8];
    BOOL sameName = FALSE;
    BOOL sameGender = FALSE;
    BOOL sameId = profile->trainerId == other->trainerId;

    if (sameId) {
        JoinAvenueProfile_GetName(profile, name);
        JoinAvenueProfile_GetName(other, otherName);
        sameName = wcharscmp(name, otherName);
        sameGender = profile->gender == other->gender;
    }
    if (sameId && sameName && sameGender) {
        return TRUE;
    }
    return FALSE;
}

static void JoinAvenueProfile_GetGreeting(const JoinAvenueProfile *profile, u16 *dest) {
    MsgData *msgData;
    StrBuf *str;

    if (profile->greeting[0] != GFL_StrBufGetTerminator()) {
        sys_memcpy(profile->greeting, dest, sizeof(profile->greeting));
        dest[NELEMS(profile->greeting)] = GFL_StrBufGetTerminator();
    } else {
        msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, MSG_JOIN_AVENUE_DEFAULTS, HEAPID_TAIL(HEAPID_GAMEEVENT));
        str = GFL_MsgDataLoadStrbufNew(msgData, profile->trainerId % 64 + MSG_JOIN_AVENUE_DEFAULT_GREETING);
        GFL_StrBufStoreString(str, dest, 9);
        GFL_StrBufFree(str);
        GFL_MsgDataFree(msgData);
    }
}

static void JoinAvenueShopChoice_SetFromGameData(JoinAvenueShopChoice *choice, GameData *gameData) {
    JoinAvenueSave *joinAvenue;
    JoinAvenueInfo *info;
    JoinAvenuePerson *player;
    PlayerInfo *playerInfo;

    sys_memset(choice, 0, sizeof(JoinAvenueShopChoice));
    joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
    info = JoinAvenue_GetInfo(joinAvenue);
    player = func_02038a18(getAddressOfBeginningOfOccupants(joinAvenue));
    playerInfo = GetGameDataPlayerInfo(gameData);
    choice->rank = JoinAvenue_GetParam(info, 2, NULL);
    choice->shopId = func_02038a20(player, playerInfo);
    choice->unk0 = FALSE;
}

static BOOL JoinAvenueShopChoice_IsValid(const JoinAvenueShopChoice *choice) {
    if (choice->shopId >= 0x141) {
        return FALSE;
    }
    if (choice->rank > 100) {
        return FALSE;
    }
    if (choice->unk0) {
        return FALSE;
    }
    return TRUE;
}

static void JoinAvenueData_FillAdventure(JoinAvenueData *data, GameData *gameData) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    JoinAvenueAdventure *adventure = &data->adventure;
    AdventureTime *time;
    RTCDate date;
    RTCTime rtcTime;
    MedalBox *medalBox;
    u8 year;
    u8 month;
    u8 day;
    JoinAvenueOccupants *occupants;
    JoinAvenuePerson *person;
    int i;
    PokeDexSave *pokedex;
    PokeParty *party;
    PartyPkm *pkm;

    sys_memset(adventure, 0, sizeof(JoinAvenueAdventure));
    time = getSaveAdventureTimeBlock(save);
    func_0207d244(&date, &rtcTime, time->startSeconds);
    adventure->startDate = getYearMonthDay(&date);
    if (EventWork_FlagGet(GameData_GetEventWork(gameData), EVENT_FLAG_HALL_OF_FAME)) {
        func_0207d244(&date, &rtcTime, time->seconds);
        adventure->hallOfFameDate = getYearMonthDay(&date);
    }

    medalBox = SaveControl_GetMedalBox(save);
    adventure->medalCount = MedalBox_GetObtainedCount(medalBox, 0);
    adventure->lastMedal = func_0200fa44(medalBox);
    adventure->medalRank = MedalBox_GetRank(medalBox);
    if (adventure->lastMedal < 0xff) {
        MedalBox_GetMedalDate(medalBox, adventure->lastMedal, &year, &month, &day);
        adventure->medalDate = ((year & 0x7f) << 9) | ((month & 0xf) << 5) | (day & 0x1f);
    }

    occupants = getAddressOfBeginningOfOccupants(SaveControl_GetJoinAvenue(save));
    for (i = 0; i < JOIN_AVE_OCCUPANT_COUNT; i++) {
        person = func_02038860(occupants, i);
        if (!JoinAvenuePerson_IsEmpty(person)) {
            switch (joinAveTextHandler(person, JOIN_AVE_PARAM_SHOP_KIND, NULL)) {
            case 5:
                adventure->shopCount5++;
                break;
            case 4:
                adventure->shopCount4++;
                break;
            case 0:
                adventure->shopCount0++;
                break;
            case 6:
                adventure->shopCount6++;
                break;
            case 1:
                adventure->shopCount1++;
                break;
            case 2:
                adventure->shopCount2++;
                break;
            case 7:
                adventure->shopCount7++;
                break;
            case 3:
                adventure->shopCount3++;
                break;
            }
        }
    }

    pokedex = GameData_GetPokedex(gameData);
    party = GameData_GetParty(gameData);
    adventure->dexCaught = PokeDex_GetCaughtNoNational(pokedex);
    if (PokeParty_GetPkmCount(party) >= 1) {
        pkm = PokeParty_GetPkm(party, 0);
        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
            adventure->species = JOIN_AVE_SPECIES_EGG;
        } else {
            adventure->species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        }
    }
}

static BOOL JoinAvenueData_CheckAdventure(JoinAvenueData *data) {
    JoinAvenueAdventure *adventure = &data->adventure;

    if (adventure->lastMedal < 0xff) {
        u8 month = (adventure->medalDate >> 5) & 0xf;
        u8 day = adventure->medalDate & 0x1f;
        u8 year = (adventure->medalDate >> 9) & 0x7f;

        if (year > 99) {
            return FALSE;
        }
        if (month < 1 || month > 12) {
            return FALSE;
        }
        if (day < 1 || day > 31) {
            return FALSE;
        }
    }
    return TRUE;
}

static void JoinAvenueData_FillRecords1(JoinAvenueData *data, GameData *gameData) {
    ResortWork *work = func_02017b84(gameData);

    sys_memset(data, 0, sizeof(JoinAvenueData));
    data->records[0] = ResortWork_Get(work, 0);
    data->records[1] = ResortWork_Get(work, 1);
    data->records[2] = ResortWork_Get(work, 2);
    data->records[3] = ResortWork_Get(work, 3);
}

static BOOL JoinAvenueData_CheckRecords1(JoinAvenueData *data) {
    int i;

    for (i = 0; i < 4; i++) {
        if (data->records[i] > RESORT_WORK_RECORD_MAX) {
            return FALSE;
        }
    }
    return TRUE;
}

static void JoinAvenueData_FillRecords2(JoinAvenueData *data, GameData *gameData) {
    ResortWork *work = func_02017b84(gameData);

    sys_memset(data, 0, sizeof(JoinAvenueData));
    data->records[0] = ResortWork_Get(work, 4);
    data->records[1] = ResortWork_Get(work, 5);
    data->records[2] = ResortWork_Get(work, 6);
    data->records[3] = ResortWork_Get(work, 7);
}

static BOOL JoinAvenueData_CheckRecords2(JoinAvenueData *data) {
    int i;

    for (i = 0; i < 4; i++) {
        if (data->records[i] > RESORT_WORK_RECORD_MAX) {
            return FALSE;
        }
    }
    return TRUE;
}

static void JoinAvenueData_FillValues(JoinAvenueData *data, GameData *gameData) {
    JoinAvenuePerson *player =
        func_02038a18(getAddressOfBeginningOfOccupants(SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData))));
    int i;

    sys_memset(data, 0, sizeof(JoinAvenueData));
    for (i = 0; i < 16; i++) {
        data->values[i] = joinAveTextHandler(player, JOIN_AVE_PARAM_VALUE + i, NULL);
    }
}

static BOOL JoinAvenueData_CheckValues(JoinAvenueData *data) {
    int i;

    for (i = 0; i < 16; i++) {
        if (data->values[i] > sValueMax[i]) {
            data->values[i] = 0;
        }
    }
    return TRUE;
}

static void JoinAvenueData_FillRecent(JoinAvenueData *data, GameData *gameData) {
    JoinAvenuePerson *player =
        func_02038a18(getAddressOfBeginningOfOccupants(SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData))));
    int i;

    sys_memset(data, 0, sizeof(JoinAvenueData));
    for (i = 0; i < JOIN_AVE_RECENT_COUNT; i++) {
        data->recent.ids[i] = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_ID + i, NULL);
        data->recent.dates[i] = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_DATE + i, NULL);
    }
}

static BOOL JoinAvenueData_CheckRecent(JoinAvenueData *data) {
    int i;

    for (i = 0; i < JOIN_AVE_RECENT_COUNT; i++) {
        if (data->recent.ids[i] >= 33) {
            data->recent.ids[0] = 0;
            data->recent.ids[1] = 0;
            data->recent.ids[2] = 0;
            data->recent.ids[3] = 0;
            break;
        }
    }
    return TRUE;
}

static void JoinAvenueData_FillMessage1(JoinAvenueData *data, GameData *gameData) {
    sys_memset(data, 0, sizeof(JoinAvenueData));
    sys_memcpy((void *)joinAveTextHandler(func_02038a18(getAddressOfBeginningOfOccupants(
                                              SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData)))),
                                          JOIN_AVE_PARAM_MESSAGE1_RAW, NULL),
               data, sizeof(JoinAvenueMessage));
}

static BOOL JoinAvenueData_CheckMessage1(JoinAvenueData *data) {
    return TRUE;
}

static void getSpecialJoinAveTextToBuf(const JoinAvenueMessage *message, u16 *dest, int trainerId) {
    MsgData *msgData;
    StrBuf *str;

    if (message->str[0] != GFL_StrBufGetTerminator()) {
        sys_memcpy(message, dest, sizeof(JoinAvenueMessage));
        dest[NELEMS(message->str)] = GFL_StrBufGetTerminator();
    } else {
        msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, MSG_JOIN_AVENUE_DEFAULTS, HEAPID_TAIL(HEAPID_GAMEEVENT));
        str = GFL_MsgDataLoadStrbufNew(msgData, trainerId % 64 + MSG_JOIN_AVENUE_DEFAULT_MESSAGE1);
        GFL_StrBufStoreString(str, dest, 9);
        GFL_StrBufFree(str);
        GFL_MsgDataFree(msgData);
    }
}

static void JoinAvenueData_FillMessage2(JoinAvenueData *data, GameData *gameData) {
    sys_memset(data, 0, sizeof(JoinAvenueData));
    sys_memcpy((void *)joinAveTextHandler(func_02038a18(getAddressOfBeginningOfOccupants(
                                              SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData)))),
                                          JOIN_AVE_PARAM_MESSAGE2_RAW, NULL),
               data, sizeof(JoinAvenueMessage));
}

static BOOL JoinAvenueData_CheckMessage2(JoinAvenueData *data) {
    return TRUE;
}

static void JoinAvenue_GetMessage2(const JoinAvenueMessage *message, u16 *dest, int trainerId) {
    MsgData *msgData;
    StrBuf *str;

    if (message->str[0] != GFL_StrBufGetTerminator()) {
        sys_memcpy(message, dest, sizeof(JoinAvenueMessage));
        dest[NELEMS(message->str)] = GFL_StrBufGetTerminator();
    } else {
        msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, MSG_JOIN_AVENUE_DEFAULTS, HEAPID_TAIL(HEAPID_GAMEEVENT));
        str = GFL_MsgDataLoadStrbufNew(msgData, trainerId % 64 + MSG_JOIN_AVENUE_DEFAULT_MESSAGE2);
        GFL_StrBufStoreString(str, dest, 9);
        GFL_StrBufFree(str);
        GFL_MsgDataFree(msgData);
    }
}

JoinAvenuePerson *func_02036d94(HeapID heapId) {
    JoinAvenuePerson *person =
        GFL_HeapAllocate(heapId, sizeof(JoinAvenuePerson), TRUE, "resonance_resort_data.c", 1360);

    func_02036e14(person);
    return person;
}

void func_02036db8(JoinAvenuePerson *person) {
    GFL_HeapFree(person);
}

void func_02036dc0(JoinAvenuePerson *person, GameData *gameData) {
    int i;
    JoinAvenueData *data;

    func_02036e14(person);
    JoinAvenueProfile_SetFromGameData(&person->profile, gameData);
    JoinAvenueShopChoice_SetFromGameData(&person->choice, gameData);
    for (data = person->data, i = 0; i < JOIN_AVE_DATA_COUNT; i++, data++) {
        sDataFillFuncs[i](data, gameData);
    }
    person->dataMask = (1 << JOIN_AVE_DATA_COUNT) - 1;
}

void func_02036e14(JoinAvenuePerson *person) {
    sys_memset(person, 0, sizeof(JoinAvenuePerson));
    person->data[5].message.str[0] = GFL_StrBufGetTerminator();
    person->data[6].message.str[0] = GFL_StrBufGetTerminator();
    person->profile.greeting[0] = GFL_StrBufGetTerminator();
    JoinAvenueShop_Init(&person->shop);
}

BOOL JoinAvenuePerson_IsEmpty(JoinAvenuePerson *person) {
    return JoinAvenueProfile_IsEmpty(&person->profile);
}

BOOL func_02036e4c(JoinAvenuePerson *person, u32 index) {
    if (person->dataMask & (1 << index)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL JoinAvenuePerson_MatchesVisitor(JoinAvenuePerson *person, const JoinAvenueVisitor *visitor) {
    return JoinAvenueProfile_IsSame(&person->profile, &visitor->profile);
}

static BOOL JoinAvenuePerson_MatchesPerson(JoinAvenuePerson *person, const JoinAvenuePerson *other) {
    return JoinAvenueProfile_IsSame(&person->profile, &other->profile);
}

static BOOL JoinAvenuePerson_MatchesEntry(JoinAvenuePerson *person, const JoinAvenueEntry *entry) {
    return JoinAvenueProfile_IsSame(&person->profile, &entry->profile);
}

static void JoinAvenuePerson_Stamp(JoinAvenuePerson *person) {
    RTCDate date;
    RTCTime time;

    RTC_GetCachedDateTime(&date, &time);
    JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_YEAR, date.year);
    JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_MONTH, date.month);
    JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_DAY, date.day);
    JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_HOUR, time.hour);
    JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_MINUTE, time.minute);
    JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_SEED, GFL_RandomLC(0));
}

static BOOL JoinAvenuePerson_SetFromVisitor(JoinAvenuePerson *person, const JoinAvenueVisitor *visitor) {
    func_02036e14(person);
    JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_UNK_26, visitor->unk44);
    person->profile = visitor->profile;
    if (visitor->dataIndex < JOIN_AVE_DATA_COUNT) {
        person->choice = visitor->choice;
        JoinAvenuePerson_AddVisitorData(person, visitor);
    }
    JoinAvenuePerson_Stamp(person);
    return TRUE;
}

static BOOL JoinAvenuePerson_Copy(JoinAvenuePerson *person, const JoinAvenuePerson *src) {
    *person = *src;
    JoinAvenuePerson_Stamp(person);
    return TRUE;
}

static u32 JoinAvenuePerson_MergeVisitor(JoinAvenuePerson *person, const JoinAvenueVisitor *visitor) {
    BOOL changed = FALSE;

    if (joinAveTextHandler(person, JOIN_AVE_PARAM_UNK_27, NULL)) {
        return 2;
    }
    if (person->profile.trainerType != visitor->profile.trainerType) {
        changed = TRUE;
    }
    person->profile = visitor->profile;
    if (visitor->dataIndex < JOIN_AVE_DATA_COUNT) {
        person->choice = visitor->choice;
        JoinAvenuePerson_AddVisitorData(person, visitor);
    }
    if (changed) {
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_CHANGED);
    } else {
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_UPDATED);
    }
    return 1;
}

static u32 JoinAvenuePerson_MergePerson(JoinAvenuePerson *person, const JoinAvenuePerson *src) {
    int i;
    const JoinAvenueData *srcData;
    JoinAvenueData *data;
    BOOL changed;

    if (joinAveTextHandler(person, JOIN_AVE_PARAM_UNK_27, NULL)) {
        return 2;
    }
    changed = FALSE;
    if (person->profile.trainerType != src->profile.trainerType) {
        changed = TRUE;
    }
    person->profile = src->profile;
    person->choice = src->choice;
    for (srcData = src->data, data = person->data, i = 0; i < JOIN_AVE_DATA_COUNT; i++, srcData++, data++) {
        if (src->dataMask & (1 << i)) {
            sys_memcpy(srcData, data, sizeof(JoinAvenueData));
            person->dataMask |= 1 << i;
        }
    }
    if (changed) {
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_CHANGED);
    } else {
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_UPDATED);
    }
    return 1;
}

static u32 JoinAvenuePerson_MergeEntry(JoinAvenuePerson *person, const JoinAvenueEntry *entry) {
    BOOL changed = FALSE;

    if (joinAveTextHandler(person, JOIN_AVE_PARAM_UNK_27, NULL)) {
        return 2;
    }
    if (person->profile.trainerType != entry->profile.trainerType) {
        changed = TRUE;
    }
    person->profile.trainerType = entry->profile.trainerType;
    if (changed) {
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_CHANGED);
    } else {
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_UPDATED);
    }
    return 1;
}

BOOL func_020370a4(JoinAvenuePerson *person) {
    JoinAvenueData *data;
    int i;

    if (!JoinAvenueProfile_IsValid(&person->profile)) {
        return FALSE;
    }
    if (!JoinAvenueShopChoice_IsValid(&person->choice)) {
        return FALSE;
    }
    for (data = person->data, i = 0; i < JOIN_AVE_DATA_COUNT; i++, data++) {
        if (!sDataCheckFuncs[i](data)) {
            return FALSE;
        }
    }
    return TRUE;
}

u32 joinAveTextHandler(const JoinAvenuePerson *person, JoinAvenuePersonParam param, void *buffer) {
    switch (param) {
    case JOIN_AVE_PARAM_SLOT:
        return person->slot;
    case JOIN_AVE_PARAM_ROW:
        return person->row;
    case JOIN_AVE_PARAM_GENDER:
        return person->profile.gender;
    case JOIN_AVE_PARAM_TRAINER_TYPE:
        return person->profile.trainerType;
    case JOIN_AVE_PARAM_NAME:
        JoinAvenueProfile_GetName(&person->profile, buffer);
        return 0;
    case JOIN_AVE_PARAM_GREETING:
        JoinAvenueProfile_GetGreeting(&person->profile, buffer);
        return 0;
    case JOIN_AVE_PARAM_UNK_32:
        return 0;
    case JOIN_AVE_PARAM_UNK_6:
        return person->unkAB;
    case JOIN_AVE_PARAM_COUNTRY:
        return person->profile.country;
    case JOIN_AVE_PARAM_AREA:
        return person->profile.region;
    case JOIN_AVE_PARAM_PLAY_HOURS:
        return person->profile.playHours;
    case JOIN_AVE_PARAM_PLAY_MINUTES:
        return person->profile.playMinutes;
    case JOIN_AVE_PARAM_JOB:
        return person->profile.job;
    case JOIN_AVE_PARAM_HOBBY:
        return person->profile.hobby;
    case JOIN_AVE_PARAM_TRAINER_ID:
        return person->profile.trainerId;
    case JOIN_AVE_PARAM_VERSION:
        return person->profile.version;
    case JOIN_AVE_PARAM_LANGUAGE:
        return person->profile.language;
    case JOIN_AVE_PARAM_UNK_18:
        return person->choice.unk0;
    case JOIN_AVE_PARAM_RANK:
        return person->choice.rank;
    case JOIN_AVE_PARAM_SHOP_ID:
        return person->choice.shopId;
    case JOIN_AVE_PARAM_UNK_20:
        return person->unkA0;
    case JOIN_AVE_PARAM_YEAR:
        return person->year;
    case JOIN_AVE_PARAM_MONTH:
        return person->month;
    case JOIN_AVE_PARAM_DAY:
        return person->day;
    case JOIN_AVE_PARAM_HOUR:
        return person->hour;
    case JOIN_AVE_PARAM_MINUTE:
        return person->minute;
    case JOIN_AVE_PARAM_UNK_26:
        return person->unkA8;
    case JOIN_AVE_PARAM_UNK_27:
        return person->unkBC_9;
    case JOIN_AVE_PARAM_STATUS:
        return person->status;
    case JOIN_AVE_PARAM_UNK_29:
        return person->unkAA_0;
    case JOIN_AVE_PARAM_UNK_30:
        return person->unkA9_7;
    case JOIN_AVE_PARAM_UNK_37:
        return person->unkA9_0;
    case JOIN_AVE_PARAM_UNK_38:
        return person->unkA9_1;
    case JOIN_AVE_PARAM_SHOP_KIND:
        return person->shopKind;
    case JOIN_AVE_PARAM_DATA_MASK:
        return person->dataMask;
    case JOIN_AVE_PARAM_DATA_COUNT: {
        u32 count = 0;
        int i;

        for (i = 0; i < JOIN_AVE_DATA_COUNT; i++) {
            if ((1 << i) & person->dataMask) {
                count++;
            }
        }
        return count;
    }
    case JOIN_AVE_PARAM_UNK_39:
        return person->unkA9_2;
    case JOIN_AVE_PARAM_UNK_36:
        return person->unkB8;
    case JOIN_AVE_PARAM_SEED:
        return person->seed;
    case JOIN_AVE_PARAM_LAST_MEDAL:
        return person->data[0].adventure.lastMedal;
    case JOIN_AVE_PARAM_MEDAL_DATE:
        return person->data[0].adventure.medalDate;
    case JOIN_AVE_PARAM_MEDAL_COUNT:
        return person->data[0].adventure.medalCount;
    case JOIN_AVE_PARAM_MEDAL_RANK:
        return person->data[0].adventure.medalRank;
    case JOIN_AVE_PARAM_START_DATE:
        return person->data[0].adventure.startDate;
    case JOIN_AVE_PARAM_HALL_OF_FAME_DATE:
        return person->data[0].adventure.hallOfFameDate;
    case JOIN_AVE_PARAM_DEX_CAUGHT:
        return person->data[0].adventure.dexCaught;
    case JOIN_AVE_PARAM_SPECIES:
        return person->data[0].adventure.species;
    case JOIN_AVE_PARAM_SHOP_COUNT + 0:
        return person->data[0].adventure.shopCount0;
    case JOIN_AVE_PARAM_SHOP_COUNT + 1:
        return person->data[0].adventure.shopCount1;
    case JOIN_AVE_PARAM_SHOP_COUNT + 2:
        return person->data[0].adventure.shopCount2;
    case JOIN_AVE_PARAM_SHOP_COUNT + 3:
        return person->data[0].adventure.shopCount3;
    case JOIN_AVE_PARAM_SHOP_COUNT + 4:
        return person->data[0].adventure.shopCount4;
    case JOIN_AVE_PARAM_SHOP_COUNT + 5:
        return person->data[0].adventure.shopCount5;
    case JOIN_AVE_PARAM_SHOP_COUNT + 6:
        return person->data[0].adventure.shopCount6;
    case JOIN_AVE_PARAM_SHOP_COUNT + 7:
        return person->data[0].adventure.shopCount7;
    case JOIN_AVE_PARAM_RECORD1 + 0:
    case JOIN_AVE_PARAM_RECORD1 + 1:
    case JOIN_AVE_PARAM_RECORD1 + 2:
    case JOIN_AVE_PARAM_RECORD1 + 3:
        return person->data[1].records[param - JOIN_AVE_PARAM_RECORD1];
    case JOIN_AVE_PARAM_RECORD2 + 0:
    case JOIN_AVE_PARAM_RECORD2 + 1:
    case JOIN_AVE_PARAM_RECORD2 + 2:
    case JOIN_AVE_PARAM_RECORD2 + 3:
        return person->data[2].records[param - JOIN_AVE_PARAM_RECORD2];
    case JOIN_AVE_PARAM_VALUE + 0:
    case JOIN_AVE_PARAM_VALUE + 1:
    case JOIN_AVE_PARAM_VALUE + 2:
    case JOIN_AVE_PARAM_VALUE + 3:
    case JOIN_AVE_PARAM_VALUE + 4:
    case JOIN_AVE_PARAM_VALUE + 5:
    case JOIN_AVE_PARAM_VALUE + 6:
    case JOIN_AVE_PARAM_VALUE + 7:
    case JOIN_AVE_PARAM_VALUE + 8:
    case JOIN_AVE_PARAM_VALUE + 9:
    case JOIN_AVE_PARAM_VALUE + 10:
    case JOIN_AVE_PARAM_VALUE + 11:
    case JOIN_AVE_PARAM_VALUE + 12:
    case JOIN_AVE_PARAM_VALUE + 13:
    case JOIN_AVE_PARAM_VALUE + 14:
    case JOIN_AVE_PARAM_VALUE + 15:
        return person->data[3].values[param - JOIN_AVE_PARAM_VALUE];
    case JOIN_AVE_PARAM_RECENT_ID + 0:
    case JOIN_AVE_PARAM_RECENT_ID + 1:
    case JOIN_AVE_PARAM_RECENT_ID + 2:
    case JOIN_AVE_PARAM_RECENT_ID + 3:
        return person->data[4].recent.ids[param - JOIN_AVE_PARAM_RECENT_ID];
    case JOIN_AVE_PARAM_RECENT_DATE + 0:
    case JOIN_AVE_PARAM_RECENT_DATE + 1:
    case JOIN_AVE_PARAM_RECENT_DATE + 2:
    case JOIN_AVE_PARAM_RECENT_DATE + 3:
        return person->data[4].recent.dates[param - JOIN_AVE_PARAM_RECENT_DATE];
    case JOIN_AVE_PARAM_MESSAGE1:
        getSpecialJoinAveTextToBuf(&person->data[5].message, buffer, person->profile.trainerId);
        return 0;
    case JOIN_AVE_PARAM_MESSAGE2:
        JoinAvenue_GetMessage2(&person->data[6].message, buffer, person->profile.trainerId);
        return 0;
    case JOIN_AVE_PARAM_MESSAGE1_RAW:
        if (buffer != NULL) {
            sys_memcpy(&person->data[5].message, buffer, sizeof(JoinAvenueMessage));
            ((u16 *)buffer)[8] = GFL_StrBufGetTerminator();
        }
        return (u32)&person->data[5].message;
    case JOIN_AVE_PARAM_MESSAGE2_RAW:
        if (buffer != NULL) {
            sys_memcpy(&person->data[6].message, buffer, sizeof(JoinAvenueMessage));
            ((u16 *)buffer)[8] = GFL_StrBufGetTerminator();
        }
        return (u32)&person->data[6].message;
    }
    return 0;
}

void JoinAvenuePerson_SetParam(JoinAvenuePerson *person, JoinAvenuePersonParam param, u32 value) {
    switch (param) {
    case JOIN_AVE_PARAM_SLOT:
        person->slot = value;
        break;
    case JOIN_AVE_PARAM_ROW:
        person->row = value;
        break;
    case JOIN_AVE_PARAM_UNK_6:
        person->unkAB = value;
        break;
    case JOIN_AVE_PARAM_COUNTRY:
        person->profile.country = value;
        break;
    case JOIN_AVE_PARAM_AREA:
        person->profile.region = value;
        break;
    case JOIN_AVE_PARAM_PLAY_HOURS:
        person->profile.playHours = value;
        break;
    case JOIN_AVE_PARAM_PLAY_MINUTES:
        person->profile.playMinutes = value;
        break;
    case JOIN_AVE_PARAM_JOB:
        person->profile.job = value;
        break;
    case JOIN_AVE_PARAM_HOBBY:
        person->profile.hobby = value;
        break;
    case JOIN_AVE_PARAM_TRAINER_ID:
        person->profile.trainerId = value;
        break;
    case JOIN_AVE_PARAM_VERSION:
        person->profile.version = value;
        break;
    case JOIN_AVE_PARAM_LANGUAGE:
        person->profile.language = value;
        break;
    case JOIN_AVE_PARAM_NAME:
        sys_memcpy((void *)value, person->profile.name, sizeof(person->profile.name));
        break;
    case JOIN_AVE_PARAM_GREETING:
        sys_memcpy((void *)value, person->profile.greeting, sizeof(person->profile.greeting));
        break;
    case JOIN_AVE_PARAM_SHOP_ID:
        person->choice.shopId = value;
        break;
    case JOIN_AVE_PARAM_TRAINER_TYPE:
        person->profile.trainerType = value;
        break;
    case JOIN_AVE_PARAM_GENDER:
        person->profile.gender = value;
        break;
    case JOIN_AVE_PARAM_UNK_18:
        person->choice.unk0 = value;
        break;
    case JOIN_AVE_PARAM_RANK:
        person->choice.rank = value;
        break;
    case JOIN_AVE_PARAM_UNK_20:
        person->unkA0 = value;
        break;
    case JOIN_AVE_PARAM_YEAR:
        person->year = value;
        break;
    case JOIN_AVE_PARAM_MONTH:
        person->month = value;
        break;
    case JOIN_AVE_PARAM_DAY:
        person->day = value;
        break;
    case JOIN_AVE_PARAM_HOUR:
        person->hour = value;
        break;
    case JOIN_AVE_PARAM_MINUTE:
        person->minute = value;
        break;
    case JOIN_AVE_PARAM_UNK_26:
        person->unkA8 = value;
        break;
    case JOIN_AVE_PARAM_UNK_27:
        person->unkBC_9 = value;
        break;
    case JOIN_AVE_PARAM_STATUS:
        // Keeps the lowest status set since it was cleared
        if (person->status == 0) {
            person->status = value;
        } else if (person->status > value) {
            person->status = value;
        }
        break;
    case JOIN_AVE_PARAM_UNK_29:
        person->unkAA_0 = value;
        break;
    case JOIN_AVE_PARAM_UNK_30:
        person->unkA9_7 = value;
        break;
    case JOIN_AVE_PARAM_UNK_37:
        person->unkA9_0 = value;
        break;
    case JOIN_AVE_PARAM_UNK_38:
        person->unkA9_1 = value;
        break;
    case JOIN_AVE_PARAM_SHOP_KIND:
        person->shopKind = value;
        break;
    case JOIN_AVE_PARAM_DATA_MASK:
        person->dataMask = value;
        break;
    case JOIN_AVE_PARAM_DATA_RECEIVED:
        person->dataMask |= 1 << value;
        break;
    case JOIN_AVE_PARAM_UNK_39:
        person->unkA9_2 = value;
        break;
    case JOIN_AVE_PARAM_UNK_36:
        person->unkB8 = value;
        break;
    case JOIN_AVE_PARAM_SEED:
        person->seed = value;
        break;
    case JOIN_AVE_PARAM_LAST_MEDAL:
        person->data[0].adventure.lastMedal = value;
        break;
    case JOIN_AVE_PARAM_MEDAL_DATE:
        person->data[0].adventure.medalDate = value;
        break;
    case JOIN_AVE_PARAM_MEDAL_COUNT:
        person->data[0].adventure.medalCount = value;
        break;
    case JOIN_AVE_PARAM_MEDAL_RANK:
        person->data[0].adventure.medalRank = value;
        break;
    case JOIN_AVE_PARAM_START_DATE:
        person->data[0].adventure.startDate = value;
        break;
    case JOIN_AVE_PARAM_HALL_OF_FAME_DATE:
        person->data[0].adventure.hallOfFameDate = value;
        break;
    case JOIN_AVE_PARAM_DEX_CAUGHT:
        person->data[0].adventure.dexCaught = value;
        break;
    case JOIN_AVE_PARAM_SPECIES:
        person->data[0].adventure.species = value;
        break;
    case JOIN_AVE_PARAM_SHOP_COUNT + 0:
        person->data[0].adventure.shopCount0 = value;
        break;
    case JOIN_AVE_PARAM_SHOP_COUNT + 1:
        person->data[0].adventure.shopCount1 = value;
        break;
    case JOIN_AVE_PARAM_SHOP_COUNT + 2:
        person->data[0].adventure.shopCount2 = value;
        break;
    case JOIN_AVE_PARAM_SHOP_COUNT + 3:
        person->data[0].adventure.shopCount3 = value;
        break;
    case JOIN_AVE_PARAM_SHOP_COUNT + 4:
        person->data[0].adventure.shopCount4 = value;
        break;
    case JOIN_AVE_PARAM_SHOP_COUNT + 5:
        person->data[0].adventure.shopCount5 = value;
        break;
    case JOIN_AVE_PARAM_SHOP_COUNT + 6:
        person->data[0].adventure.shopCount6 = value;
        break;
    case JOIN_AVE_PARAM_SHOP_COUNT + 7:
        person->data[0].adventure.shopCount7 = value;
        break;
    case JOIN_AVE_PARAM_RECORD1 + 0:
    case JOIN_AVE_PARAM_RECORD1 + 1:
    case JOIN_AVE_PARAM_RECORD1 + 2:
    case JOIN_AVE_PARAM_RECORD1 + 3:
        person->data[1].records[param - JOIN_AVE_PARAM_RECORD1] = value;
        break;
    case JOIN_AVE_PARAM_RECORD2 + 0:
    case JOIN_AVE_PARAM_RECORD2 + 1:
    case JOIN_AVE_PARAM_RECORD2 + 2:
    case JOIN_AVE_PARAM_RECORD2 + 3:
        person->data[2].records[param - JOIN_AVE_PARAM_RECORD2] = value;
        break;
    case JOIN_AVE_PARAM_VALUE + 0:
    case JOIN_AVE_PARAM_VALUE + 1:
    case JOIN_AVE_PARAM_VALUE + 2:
    case JOIN_AVE_PARAM_VALUE + 3:
    case JOIN_AVE_PARAM_VALUE + 4:
    case JOIN_AVE_PARAM_VALUE + 5:
    case JOIN_AVE_PARAM_VALUE + 6:
    case JOIN_AVE_PARAM_VALUE + 7:
    case JOIN_AVE_PARAM_VALUE + 8:
    case JOIN_AVE_PARAM_VALUE + 9:
    case JOIN_AVE_PARAM_VALUE + 10:
    case JOIN_AVE_PARAM_VALUE + 11:
    case JOIN_AVE_PARAM_VALUE + 12:
    case JOIN_AVE_PARAM_VALUE + 13:
    case JOIN_AVE_PARAM_VALUE + 14:
    case JOIN_AVE_PARAM_VALUE + 15:
        person->data[3].values[param - JOIN_AVE_PARAM_VALUE] = value;
        break;
    case JOIN_AVE_PARAM_RECENT_ID + 0:
    case JOIN_AVE_PARAM_RECENT_ID + 1:
    case JOIN_AVE_PARAM_RECENT_ID + 2:
    case JOIN_AVE_PARAM_RECENT_ID + 3:
        person->data[4].recent.ids[param - JOIN_AVE_PARAM_RECENT_ID] = value;
        break;
    case JOIN_AVE_PARAM_RECENT_DATE + 0:
    case JOIN_AVE_PARAM_RECENT_DATE + 1:
    case JOIN_AVE_PARAM_RECENT_DATE + 2:
    case JOIN_AVE_PARAM_RECENT_DATE + 3:
        person->data[4].recent.dates[param - JOIN_AVE_PARAM_RECENT_DATE] = value;
        break;
    case JOIN_AVE_PARAM_MESSAGE1:
        sys_memcpy((void *)value, &person->data[5].message, sizeof(JoinAvenueMessage));
        break;
    case JOIN_AVE_PARAM_MESSAGE2:
        sys_memcpy((void *)value, &person->data[6].message, sizeof(JoinAvenueMessage));
        break;
    }
}

static BOOL JoinAvenuePerson_IsExpired(const JoinAvenuePerson *person, s64 now) {
    RTCDate date;
    RTCTime time;
    s64 arrived;
    u32 count;
    BOOL expired;

    if (person->unkA9_1) {
        return FALSE;
    }
    expired = FALSE;
    date.year = person->year;
    date.week = 0;
    time.second = 0;
    date.month = person->month;
    date.day = person->day;
    time.hour = person->hour;
    time.minute = person->minute;
    arrived = func_0207d12c(&date, &time);
    count = joinAveTextHandler(person, JOIN_AVE_PARAM_DATA_COUNT, NULL);
    if (person->choice.unk0) {
        if (now - arrived > JOIN_AVE_STAY_SHORT) {
            expired = TRUE;
        }
    } else if (now - arrived > sStayDurations[count]) {
        expired = TRUE;
    }
    return expired;
}

u32 func_020378f8(JoinAvenuePerson *person, int count, u32 max) {
    return func_0203941c(joinAveTextHandler(person, JOIN_AVE_PARAM_SEED, NULL), count, max);
}

JoinAvenueVisitor *func_02037910(HeapID heapId) {
    JoinAvenueVisitor *visitor =
        GFL_HeapAllocate(heapId, sizeof(JoinAvenueVisitor), TRUE, "resonance_resort_data.c", 2367);

    visitor->dataIndex = JOIN_AVE_DATA_COUNT;
    return visitor;
}

void func_02037930(JoinAvenueVisitor *visitor) {
    GFL_HeapFree(visitor);
}

void func_02037938(JoinAvenueVisitor *visitor, int index, GameData *gameData) {
    JoinAvenueProfile_SetFromGameData(&visitor->profile, gameData);
    JoinAvenueShopChoice_SetFromGameData(&visitor->choice, gameData);
    visitor->unk44 = 8;
    if (index >= JOIN_AVE_DATA_COUNT) {
        index = 0;
    }
    sDataFillFuncs[index](&visitor->data, gameData);
    visitor->dataIndex = index;
}

void func_02037970(JoinAvenueVisitor *visitor, GameBeacon *beacon) {
    sys_memcpy(&visitor->choice, &GameBeacon_GetAvenuePayload(beacon)->choice, sizeof(JoinAvenueShopChoice));
    sys_memcpy(&visitor->data, &GameBeacon_GetAvenuePayload(beacon)->data, sizeof(JoinAvenueData));
    beacon->arg.value = visitor->dataIndex;
}

void func_02037998(JoinAvenueVisitor *visitor, const GameBeacon *beacon, u32 a2) {
    sys_memset(visitor, 0, sizeof(JoinAvenueVisitor));
    visitor->unk44 = a2;
    JoinAvenueProfile_SetFromBeacon(&visitor->profile, beacon);
    if (beacon->version == VERSION_WHITE2 || beacon->version == VERSION_BLACK2) {
        if (a2 == 8) {
            sys_memcpy(&GameBeacon_GetAvenuePayload(beacon)->choice, &visitor->choice, sizeof(JoinAvenueShopChoice));
            sys_memcpy(&GameBeacon_GetAvenuePayload(beacon)->data, &visitor->data, sizeof(JoinAvenueData));
            visitor->dataIndex = beacon->arg.value;
        } else {
            visitor->dataIndex = JOIN_AVE_DATA_COUNT;
        }
    } else {
        visitor->dataIndex = JOIN_AVE_DATA_COUNT;
    }
}

static u16 JoinAvenueVisitor_GetTrainerId(const JoinAvenueVisitor *visitor) {
    return visitor->profile.trainerId;
}

BOOL func_020379f8(JoinAvenueVisitor *visitor) {
    if (!JoinAvenueProfile_IsValid(&visitor->profile)) {
        return FALSE;
    }
    if (!JoinAvenueShopChoice_IsValid(&visitor->choice)) {
        return FALSE;
    }
    if (visitor->dataIndex > JOIN_AVE_DATA_COUNT) {
        return FALSE;
    }
    if (visitor->dataIndex < JOIN_AVE_DATA_COUNT && !sDataCheckFuncs[visitor->dataIndex](&visitor->data)) {
        return FALSE;
    }
    return TRUE;
}

JoinAvenueEntry *func_02037a40(HeapID heapId) {
    JoinAvenueEntry *entry = GFL_HeapAllocate(heapId, sizeof(JoinAvenueEntry), TRUE, "resonance_resort_data.c", 2566);

    func_02037a70(entry);
    return entry;
}

void func_02037a68(JoinAvenueEntry *entry) {
    GFL_HeapFree(entry);
}

void func_02037a70(JoinAvenueEntry *entry) {
    sys_memset(entry, 0, sizeof(JoinAvenueEntry));
    entry->message1.str[0] = GFL_StrBufGetTerminator();
    entry->message2.str[0] = GFL_StrBufGetTerminator();
    entry->profile.greeting[0] = GFL_StrBufGetTerminator();
}

BOOL func_02037a90(JoinAvenueEntry *entry) {
    return JoinAvenueProfile_IsEmpty(&entry->profile);
}

BOOL func_02037a98(JoinAvenueEntry *entry) {
    if (entry->unk4E <= 1 && !JoinAvenueProfile_IsValid(&entry->profile)) {
        return FALSE;
    }
    return TRUE;
}

void func_02037ab4(JoinAvenueEntry *entry, PlayerInfo *playerInfo, u16 species, u32 type) {
    RTCDate date;
    u16 trainerId;
    u32 version;

    func_02037a70(entry);
    switch (type) {
    default:
        type = 6;
    case 6:
        entry->unk4E = 0;
        break;
    case 7:
        entry->unk4E = 1;
        break;
    }
    entry->unk56 = type;
    trainerId = getTrainerID(playerInfo);
    version = func_02008bfc(playerInfo);
    entry->shopId = JoinAvenue_GetDefaultShop(trainerId, version);
    JoinAvenueProfile_SetFromPlayerInfo(&entry->profile, playerInfo);
    entry->species = species;
    RTC_GetCachedDate(&date);
    entry->year = date.year;
    entry->month = date.month;
    entry->day = date.day;
    entry->seed = GFL_RandomLC(0);
}

u32 func_02037b38(JoinAvenueEntry *entry, JoinAvenuePersonParam param, void *buffer) {
    switch (param) {
    case JOIN_AVE_PARAM_SLOT:
        return entry->slot;
    case JOIN_AVE_PARAM_ROW:
        return entry->row;
    case JOIN_AVE_PARAM_GENDER:
        return entry->profile.gender;
    case JOIN_AVE_PARAM_TRAINER_TYPE:
        return entry->profile.trainerType;
    case JOIN_AVE_PARAM_NAME:
        sys_memcpy(entry->profile.name, buffer, sizeof(entry->profile.name));
        ((u16 *)buffer)[7] = GFL_StrBufGetTerminator();
        return 0;
    case JOIN_AVE_PARAM_UNK_20:
        return entry->unk4F;
    case JOIN_AVE_PARAM_SEED:
        return entry->seed;
    case JOIN_AVE_PARAM_SHOP_KIND:
        return entry->unk4E;
    case JOIN_AVE_PARAM_UNK_26:
        return entry->unk56;
    case JOIN_AVE_PARAM_SHOP_ID:
        return entry->shopId;
    case JOIN_AVE_PARAM_COUNTRY:
        return entry->profile.country;
    case JOIN_AVE_PARAM_AREA:
        return entry->profile.region;
    case JOIN_AVE_PARAM_PLAY_HOURS:
        return 0;
    case JOIN_AVE_PARAM_PLAY_MINUTES:
        return 0;
    case JOIN_AVE_PARAM_TRAINER_ID:
        return entry->profile.trainerId;
    case JOIN_AVE_PARAM_VERSION:
        return entry->profile.version;
    case JOIN_AVE_PARAM_LANGUAGE:
        return entry->profile.language;
    case JOIN_AVE_PARAM_MESSAGE1:
        getSpecialJoinAveTextToBuf(&entry->message1, buffer, entry->profile.trainerId);
        return 0;
    case JOIN_AVE_PARAM_MESSAGE2:
        JoinAvenue_GetMessage2(&entry->message2, buffer, entry->profile.trainerId);
        return 0;
    case JOIN_AVE_PARAM_GREETING:
        JoinAvenueProfile_GetGreeting(&entry->profile, buffer);
        return 0;
    case JOIN_AVE_PARAM_DATA_COUNT:
        return 0;
    case JOIN_AVE_PARAM_UNK_6:
        return entry->unk52;
    case JOIN_AVE_PARAM_YEAR:
        return entry->year;
    case JOIN_AVE_PARAM_MONTH:
        return entry->month;
    case JOIN_AVE_PARAM_DAY:
        return entry->day;
    case JOIN_AVE_PARAM_UNK_18:
        return 0;
    case JOIN_AVE_PARAM_ENTRY_SPECIES:
        return entry->species;
    case JOIN_AVE_PARAM_JOB:
        return 0;
    case JOIN_AVE_PARAM_HOBBY:
        return 0;
    case JOIN_AVE_PARAM_RANK:
        return 0;
    }
    return 0;
}

void func_02037c70(JoinAvenueEntry *entry, JoinAvenuePersonParam param, u32 value) {
    switch (param) {
    case JOIN_AVE_PARAM_SLOT:
        entry->slot = value;
        break;
    case JOIN_AVE_PARAM_ROW:
        entry->row = value;
        break;
    case JOIN_AVE_PARAM_NAME:
        sys_memcpy((void *)value, entry->profile.name, sizeof(entry->profile.name));
        break;
    case JOIN_AVE_PARAM_MESSAGE1:
        sys_memcpy((void *)value, &entry->message1, sizeof(JoinAvenueMessage));
        break;
    case JOIN_AVE_PARAM_MESSAGE2:
        sys_memcpy((void *)value, &entry->message2, sizeof(JoinAvenueMessage));
        break;
    case JOIN_AVE_PARAM_GREETING:
        sys_memcpy((void *)value, entry->profile.greeting, sizeof(entry->profile.greeting));
        break;
    case JOIN_AVE_PARAM_GENDER:
        entry->profile.gender = value;
        break;
    case JOIN_AVE_PARAM_VERSION:
        entry->profile.version = value;
        break;
    case JOIN_AVE_PARAM_LANGUAGE:
        entry->profile.language = value;
        break;
    case JOIN_AVE_PARAM_SEED:
        entry->seed = value;
        break;
    case JOIN_AVE_PARAM_TRAINER_TYPE:
        entry->profile.trainerType = value;
        break;
    case JOIN_AVE_PARAM_TRAINER_ID:
        entry->profile.trainerId = value;
        break;
    case JOIN_AVE_PARAM_SHOP_KIND:
        entry->unk4E = value;
        break;
    case JOIN_AVE_PARAM_UNK_6:
        entry->unk52 = value;
        break;
    case JOIN_AVE_PARAM_SHOP_ID:
        entry->shopId = value;
        break;
    case JOIN_AVE_PARAM_YEAR:
        entry->year = value;
        break;
    case JOIN_AVE_PARAM_MONTH:
        entry->month = value;
        break;
    case JOIN_AVE_PARAM_DAY:
        entry->day = value;
        break;
    case JOIN_AVE_PARAM_UNK_20:
        entry->unk4F = value;
        break;
    case JOIN_AVE_PARAM_UNK_38:
        break;
    case JOIN_AVE_PARAM_ENTRY_SPECIES:
        break;
    }
}

static BOOL JoinAvenueEntry_Merge(JoinAvenueEntry *entry, const JoinAvenueEntry *src) {
    if (entry->unk4E == 4) {
        u16 greeting[8];

        sys_memcpy(entry->profile.greeting, greeting, sizeof(greeting));
        entry->profile = src->profile;
        sys_memcpy(greeting, entry->profile.greeting, sizeof(greeting));
    } else {
        entry->profile = src->profile;
        entry->message1 = src->message1;
        entry->message2 = src->message2;
        if (entry->unk56 == src->unk56) {
            entry->species = src->species;
        }
    }
    return TRUE;
}

static BOOL JoinAvenueEntry_Copy(JoinAvenueEntry *entry, const JoinAvenueEntry *src) {
    *entry = *src;
    return TRUE;
}

u32 func_02037e34(JoinAvenueEntry *entry, int count, u32 max) {
    return func_0203941c(func_02037b38(entry, JOIN_AVE_PARAM_SEED, NULL), count, max);
}

void func_02037e4c(JoinAvenueEntryList *list, u32 count) {
    u32 i = 0;

    sys_memset(list, 0, sizeof(JoinAvenueEntryList));
    list->count = count;
    for (; i < list->count; i++) {
        func_02037a70(&list->entries[i]);
    }
}

u32 func_02037e7c(JoinAvenueEntryList *list, JoinAvenueEntry *entry) {
    BOOL skipFirst;
    u32 kind;
    JoinAvenueEntry *slot;

    if (!func_02037a98(entry)) {
        return 2;
    }
    skipFirst = FALSE;
    if (JoinAvenue_GetParam(sJoinAvenue.info, 7, NULL)) {
        kind = func_02037b38(entry, JOIN_AVE_PARAM_SHOP_KIND, NULL);
        if (kind != 2 && kind != 3) {
            skipFirst = TRUE;
        }
    }
    slot = JoinAvenueEntryList_FindEmpty(list, skipFirst);
    if (slot != NULL) {
        JoinAvenueEntry_Copy(slot, entry);
        return 1;
    }
    return 0;
}

u32 func_02037ed4(JoinAvenueEntryList *list) {
    return list->count;
}

u32 func_02037ed8(JoinAvenueEntryList *list) {
    u32 i;
    u32 count = 0;

    for (i = 0; i < list->count; i++) {
        if (!func_02037a90(&list->entries[i])) {
            count++;
        }
    }
    return count;
}

JoinAvenueEntry *func_02037f04(JoinAvenueEntryList *list, u32 index) {
    return &list->entries[index];
}

BOOL func_02037f10(JoinAvenueEntryList *list, const JoinAvenueVisitor *visitor) {
    if (JoinAvenueEntryList_Find(list, &visitor->profile) != NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_02037f24(JoinAvenueEntryList *list, const JoinAvenuePerson *person) {
    if (JoinAvenueEntryList_Find(list, &person->profile) != NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_02037f38(JoinAvenueEntryList *list, const JoinAvenueEntry *entry) {
    JoinAvenueEntry *found = JoinAvenueEntryList_Find(list, &entry->profile);

    if (found != NULL) {
        JoinAvenueEntry_Merge(found, entry);
        return TRUE;
    }
    return FALSE;
}

static JoinAvenueEntry *JoinAvenueEntryList_FindEmpty(JoinAvenueEntryList *list, BOOL skipFirst) {
    u32 i;

    for (i = 0; i < list->count; i++) {
        JoinAvenueEntry *entry = &list->entries[i];

        if ((!skipFirst || i != 0) && func_02037a90(entry)) {
            return entry;
        }
    }
    return NULL;
}

static JoinAvenueEntry *JoinAvenueEntryList_Find(JoinAvenueEntryList *list, const JoinAvenueProfile *profile) {
    u32 i;

    for (i = 0; i < list->count; i++) {
        if (JoinAvenueProfile_IsSame(&list->entries[i].profile, profile)) {
            return &list->entries[i];
        }
    }
    return NULL;
}

JoinAvenuePersonList *JoinAvenuePersonList_Create(HeapID heapId, u32 count) {
    JoinAvenuePersonList *list = GFL_HeapAllocate(
        heapId, sizeof(JoinAvenuePersonList) + count * sizeof(JoinAvenuePerson), TRUE, "resonance_resort_data.c", 3248);

    if (list == NULL) {
        return NULL;
    }
    func_02037ffc(list, count);
    return list;
}

void JoinAvenuePersonList_Free(JoinAvenuePersonList *list) {
    GFL_HeapFree(list);
}

void func_02037ffc(JoinAvenuePersonList *list, u32 count) {
    u32 i = 0;

    sys_memset(list, 0, sizeof(JoinAvenuePersonList));
    list->count = count;
    for (; i < list->count; i++) {
        func_02036e14(&list->people[i]);
    }
}

u32 JoinAvenuePersonList_GetCount(JoinAvenuePersonList *list) {
    return list->count;
}

u32 func_02038030(JoinAvenuePersonList *list) {
    u32 i;
    u32 count = 0;

    for (i = 0; i < list->count; i++) {
        if (!JoinAvenuePerson_IsEmpty(&list->people[i])) {
            count++;
        }
    }
    return count;
}

JoinAvenuePerson *JoinAvenuePersonList_Get(JoinAvenuePersonList *list, u32 index) {
    return &list->people[index];
}

u32 func_0203806c(JoinAvenuePersonList *list, const JoinAvenueVisitor *visitor) {
    JoinAvenuePerson *person = JoinAvenuePersonList_Find(list, &visitor->profile);

    if (person != NULL) {
        if (list->locked) {
            return 2;
        }
        return JoinAvenuePerson_MergeVisitor(person, visitor);
    }
    return 0;
}

u32 func_02038090(JoinAvenuePersonList *list, const JoinAvenuePerson *src) {
    JoinAvenuePerson *person = JoinAvenuePersonList_Find(list, &src->profile);

    if (person != NULL) {
        if (list->locked) {
            return 2;
        }
        return JoinAvenuePerson_MergePerson(person, src);
    }
    return 0;
}

u32 func_020380b4(JoinAvenuePersonList *list, const JoinAvenueEntry *entry) {
    JoinAvenuePerson *person = JoinAvenuePersonList_Find(list, &entry->profile);

    if (person != NULL) {
        if (list->locked) {
            return 2;
        }
        return JoinAvenuePerson_MergeEntry(person, entry);
    }
    return 0;
}

u32 func_020380d8(JoinAvenuePersonList *list, const JoinAvenueVisitor *visitor) {
    u16 trainerId;
    JoinAvenuePerson *person;

    if (list->locked) {
        return 2;
    }
    trainerId = JoinAvenueVisitor_GetTrainerId(visitor);
    if (!JoinAvenueInfo_HasVisitorId(sJoinAvenue.info, trainerId)) {
        person = JoinAvenuePersonList_FindEmpty(list);
        if (person != NULL) {
            JoinAvenuePerson_SetFromVisitor(person, visitor);
            JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_ADDED);
            JoinAvenueInfo_AddVisitorId(sJoinAvenue.info, trainerId);
            return 1;
        }
        person = JoinAvenuePersonList_FindExpired(list);
        if (person != NULL) {
            func_02036e14(person);
            JoinAvenuePerson_SetFromVisitor(person, visitor);
            JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_NEW);
            JoinAvenueInfo_AddVisitorId(sJoinAvenue.info, trainerId);
            return 1;
        }
    }
    return 0;
}

u32 func_0203815c(JoinAvenuePersonList *list, const JoinAvenuePerson *src) {
    JoinAvenuePerson *person;

    if (list->locked) {
        return 2;
    }
    person = JoinAvenuePersonList_FindEmpty(list);
    if (person != NULL) {
        JoinAvenuePerson_Copy(person, src);
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_ADDED);
        return 1;
    }
    person = JoinAvenuePersonList_FindExpired(list);
    if (person != NULL) {
        func_02036e14(person);
        JoinAvenuePerson_Copy(person, src);
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_STATUS, JOIN_AVE_STATUS_NEW);
        return 1;
    }
    return 0;
}

static JoinAvenuePerson *JoinAvenuePersonList_FindEmpty(JoinAvenuePersonList *list) {
    u32 i;

    for (i = 0; i < list->count; i++) {
        if (JoinAvenuePerson_IsEmpty(&list->people[i])) {
            return &list->people[i];
        }
    }
    return NULL;
}

static JoinAvenuePerson *JoinAvenuePersonList_Find(JoinAvenuePersonList *list, const JoinAvenueProfile *profile) {
    u32 i;

    for (i = 0; i < list->count; i++) {
        if (JoinAvenueProfile_IsSame(&list->people[i].profile, profile)) {
            return &list->people[i];
        }
    }
    return NULL;
}

static JoinAvenuePerson *JoinAvenuePersonList_FindExpired(JoinAvenuePersonList *list) {
    RTCDate date;
    RTCTime time;
    s64 now;
    u32 i;

    RTC_GetCachedDateTime(&date, &time);
    now = func_0207d12c(&date, &time);
    for (i = 0; i < list->count; i++) {
        if (!JoinAvenuePerson_IsEmpty(&list->people[i]) && JoinAvenuePerson_IsExpired(&list->people[i], now)) {
            return &list->people[i];
        }
    }
    return NULL;
}

void func_02038274(JoinAvenuePerson *person, const JoinAvenuePerson *src, ResortShopData *shops, u32 slot, u16 row) {
    if (person != src) {
        *person = *src;
    }
    person->slot = slot;
    person->row = row;
    JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_UNK_27, 0);
    JoinAvenuePerson_SetupShopKind(person, shops);
}

void func_020382d8(JoinAvenuePerson *person, const JoinAvenueEntry *entry, ResortShopData *shops, u32 slot, u16 row) {
    func_02036e14(person);
    person->profile = entry->profile;
    person->data[5].message = entry->message1;
    person->data[6].message = entry->message2;
    person->year = entry->year;
    person->month = entry->month;
    person->day = entry->day;
    person->seed = entry->seed;
    person->choice.shopId = entry->shopId;
    person->slot = slot;
    person->row = row;
    JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_UNK_27, 0);
    JoinAvenuePerson_SetupShopKind(person, shops);
}

static void JoinAvenuePerson_SetupShop(JoinAvenuePerson *person, ResortShopData *shops) {
    u32 shopId = joinAveTextHandler(person, JOIN_AVE_PARAM_SHOP_ID, NULL);
    u32 trainerId = joinAveTextHandler(person, JOIN_AVE_PARAM_TRAINER_ID, NULL);
    u32 version = joinAveTextHandler(person, JOIN_AVE_PARAM_VERSION, NULL);
    u32 unk18 = joinAveTextHandler(person, JOIN_AVE_PARAM_UNK_18, NULL);
    u16 id = func_020394b0(shopId, trainerId, version, unk18, sJoinAvenue.info, shops);
    u16 level = ResortShopData_GetShopParam(ResortShopData_GetShop(shops, id), 2);
    JoinAvenueShop *shop = func_02038470(person);

    JoinAvenueShop_Init(shop);
    func_0203640c(shop, 0, id);
    func_0203640c(shop, 1, level);
}

static void JoinAvenuePerson_AddVisitorData(JoinAvenuePerson *person, const JoinAvenueVisitor *visitor) {
    if (visitor->dataIndex < JOIN_AVE_DATA_COUNT) {
        sys_memcpy(&visitor->data, &person->data[visitor->dataIndex], sizeof(JoinAvenueData));
        person->dataMask |= 1 << visitor->dataIndex;
    }
}

JoinAvenueShop *func_02038470(JoinAvenuePerson *person) {
    return &person->shop;
}

static void JoinAvenuePerson_SetupShopKind(JoinAvenuePerson *person, ResortShopData *shops) {
    JoinAvenuePerson_SetupShop(person, shops);
    person->shopKind = ResortShopData_GetShopParam(ResortShopData_GetPersonShop(shops, person), 0);
}

JoinAvenueRecord *func_020384a4(HeapID heapId) {
    JoinAvenueRecord *record =
        GFL_HeapAllocate(heapId, sizeof(JoinAvenueRecord), TRUE, "resonance_resort_data.c", 3859);

    func_020384d4(record);
    return record;
}

void func_020384cc(JoinAvenueRecord *record) {
    GFL_HeapFree(record);
}

void func_020384d4(JoinAvenueRecord *record) {
    sys_memset(record, 0, sizeof(JoinAvenueRecord));
}

BOOL func_020384e0(JoinAvenueRecord *record) {
    return JoinAvenueProfile_IsEmpty(&record->profile);
}

static BOOL JoinAvenueRecord_MatchesVisitor(JoinAvenueRecord *record, const JoinAvenueVisitor *visitor) {
    return JoinAvenueProfile_IsSame(&record->profile, &visitor->profile);
}

static BOOL JoinAvenueRecord_MatchesPerson(JoinAvenueRecord *record, const JoinAvenuePerson *person) {
    return JoinAvenueProfile_IsSame(&record->profile, &person->profile);
}

static BOOL JoinAvenueRecord_MatchesEntry(JoinAvenueRecord *record, const JoinAvenueEntry *entry) {
    return JoinAvenueProfile_IsSame(&record->profile, &entry->profile);
}

static u32 JoinAvenueRecord_MergeVisitor(JoinAvenueRecord *record, const JoinAvenueVisitor *visitor) {
    record->profile = visitor->profile;
    if (visitor->dataIndex == 5) {
        sys_memcpy(&visitor->data, &record->message1, sizeof(JoinAvenueMessage));
    } else if (visitor->dataIndex == 6) {
        sys_memcpy(&visitor->data, &record->message2, sizeof(JoinAvenueMessage));
    }
    return 1;
}

static u32 JoinAvenueRecord_MergePerson(JoinAvenueRecord *record, const JoinAvenuePerson *person) {
    record->profile = person->profile;
    if (person->dataMask & (1 << 5)) {
        record->message1 = person->data[5].message;
    }
    if (person->dataMask & (1 << 6)) {
        record->message2 = person->data[6].message;
    }
    return 1;
}

static u32 JoinAvenueRecord_MergeEntry(JoinAvenueRecord *record, const JoinAvenueEntry *entry) {
    record->profile.trainerType = entry->profile.trainerType;
    return 1;
}

u32 func_020385a8(JoinAvenueRecord *record, JoinAvenuePersonParam param, void *buffer) {
    switch (param) {
    case JOIN_AVE_PARAM_SLOT:
        return record->slot;
    case JOIN_AVE_PARAM_ROW:
        return record->row;
    case JOIN_AVE_PARAM_NAME:
        JoinAvenueProfile_GetName(&record->profile, buffer);
        return 0;
    case JOIN_AVE_PARAM_GREETING:
        JoinAvenueProfile_GetGreeting(&record->profile, buffer);
        return 0;
    case JOIN_AVE_PARAM_GENDER:
        return record->profile.gender;
    case JOIN_AVE_PARAM_TRAINER_TYPE:
        return record->profile.trainerType;
    case JOIN_AVE_PARAM_UNK_20:
        return record->unk33;
    case JOIN_AVE_PARAM_SEED:
        return record->seed;
    case JOIN_AVE_PARAM_YEAR:
        return record->year;
    case JOIN_AVE_PARAM_MONTH:
        return record->month;
    case JOIN_AVE_PARAM_DAY:
        return record->day;
    case JOIN_AVE_PARAM_SHOP_KIND:
        return record->shopKind;
    case JOIN_AVE_PARAM_MESSAGE1:
        getSpecialJoinAveTextToBuf(&record->message1, buffer, record->profile.trainerId);
        return 0;
    case JOIN_AVE_PARAM_MESSAGE2:
        JoinAvenue_GetMessage2(&record->message2, buffer, record->profile.trainerId);
        return 0;
    case JOIN_AVE_PARAM_TRAINER_ID:
        return record->profile.trainerId;
    }
    return 0;
}

void func_02038680(JoinAvenueRecord *record, JoinAvenuePersonParam param, u32 value) {
    switch (param) {
    case JOIN_AVE_PARAM_SLOT:
        record->slot = value;
        break;
    case JOIN_AVE_PARAM_ROW:
        record->row = value;
        break;
    case JOIN_AVE_PARAM_YEAR:
        record->year = value;
        break;
    case JOIN_AVE_PARAM_MONTH:
        record->month = value;
        break;
    case JOIN_AVE_PARAM_DAY:
        record->day = value;
        break;
    case JOIN_AVE_PARAM_UNK_20:
        record->unk33 = value;
        break;
    case JOIN_AVE_PARAM_SEED:
        record->seed = value;
        break;
    case JOIN_AVE_PARAM_SHOP_KIND:
        record->shopKind = value;
        break;
    }
}

void func_020386f4(JoinAvenueRecord *record, const JoinAvenuePerson *person, u32 slot, u32 row, u16 shopKind) {
    func_020384d4(record);
    record->profile = person->profile;
    record->year = person->year;
    record->month = person->month;
    record->day = person->day;
    record->message1 = person->data[5].message;
    record->message2 = person->data[6].message;
    record->shopKind = shopKind;
    record->slot = slot;
    record->row = row;
}

void func_02038778(JoinAvenueRecord *record, const JoinAvenueEntry *entry, u32 slot, u32 row, u16 shopKind) {
    record->profile = entry->profile;
    record->year = entry->year;
    record->month = entry->month;
    record->day = entry->day;
    record->message1 = entry->message1;
    record->message2 = entry->message2;
    record->shopKind = shopKind;
    record->slot = slot;
    record->row = row;
}

u32 func_020387f4(JoinAvenueRecord *record, int count, u32 max) {
    return func_0203941c(func_020385a8(record, JOIN_AVE_PARAM_SEED, NULL), count, max);
}

void func_0203880c(JoinAvenueOccupants *occupants) {
    int i;

    sys_memset(occupants, 0, sizeof(JoinAvenueOccupants));
    for (i = 0; i < JOIN_AVE_OCCUPANT_COUNT; i++) {
        func_02036e14(&occupants->people[i]);
    }
    for (i = 0; i < JOIN_AVE_RECORD_COUNT; i++) {
        func_020384d4(&occupants->records[i]);
    }
    func_02036e14(&occupants->player);
    JoinAvenuePerson_SetParam(&occupants->player, JOIN_AVE_PARAM_SHOP_ID, 0);
}

JoinAvenuePerson *func_02038860(JoinAvenueOccupants *occupants, u32 index) {
    return &occupants->people[index];
}

u32 func_02038868(JoinAvenueOccupants *occupants) {
    int i;
    u32 count = 0;

    for (i = 0; i < JOIN_AVE_OCCUPANT_COUNT; i++) {
        if (!JoinAvenuePerson_IsEmpty(func_02038860(occupants, i))) {
            count++;
        }
    }
    return count;
}

JoinAvenueRecord *func_0203888c(JoinAvenueOccupants *occupants, u32 index) {
    return &occupants->records[index];
}

u32 func_0203889c(JoinAvenueOccupants *occupants) {
    int i;
    u32 count = 0;

    for (i = 0; i < JOIN_AVE_RECORD_COUNT; i++) {
        if (!func_020384e0(func_0203888c(occupants, i))) {
            count++;
        }
    }
    return count;
}

JoinAvenueRecord *func_020388c0(JoinAvenueOccupants *occupants) {
    int i;

    for (i = 0; i < JOIN_AVE_RECORD_COUNT; i++) {
        JoinAvenueRecord *record = func_0203888c(occupants, i);

        if (func_020384e0(record)) {
            return record;
        }
    }
    return NULL;
}

u32 func_020388e8(JoinAvenueOccupants *occupants, const void *source, u32 kind) {
    int i;
    u32 result;

    for (i = 0; i < JOIN_AVE_OCCUPANT_COUNT; i++) {
        JoinAvenuePerson *person = func_02038860(occupants, i);

        if (!JoinAvenuePerson_IsEmpty(person) && sMergeFuncs[kind].personMatches(person, source)) {
            if (occupants->locked) {
                return 2;
            }
            result = sMergeFuncs[kind].personMerge(person, source);
            if (result == 1 || result == 2) {
                return result;
            }
        }
    }
    for (i = 0; i < JOIN_AVE_RECORD_COUNT; i++) {
        JoinAvenueRecord *record = func_0203888c(occupants, i);

        if (!func_020384e0(record) && sMergeFuncs[kind].recordMatches(record, source)) {
            if (occupants->locked) {
                return 2;
            }
            result = sMergeFuncs[kind].recordMerge(record, source);
            if (result == 1 || result == 2) {
                return result;
            }
        }
    }
    return 0;
}

void func_020389a0(JoinAvenueOccupants *occupants, BOOL fullDay) {
    int i;

    for (i = 0; i < JOIN_AVE_OCCUPANT_COUNT; i++) {
        JoinAvenuePerson *person = func_02038860(occupants, i);

        if (!JoinAvenuePerson_IsEmpty(person) && !fullDay) {
            JoinAvenueShop *shop = func_02038470(person);

            JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_UNK_30, 0);
            JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_UNK_37, 0);
            JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_SEED,
                                      gfRand(joinAveTextHandler(person, JOIN_AVE_PARAM_SEED, NULL)));
            func_0203640c(shop, 3, 0);
        }
    }
}

void func_02038a0c(JoinAvenueOccupants *occupants, u32 locked) {
    occupants->locked = locked;
}

JoinAvenuePerson *func_02038a18(JoinAvenueOccupants *occupants) {
    return &occupants->player;
}

u32 func_02038a20(JoinAvenuePerson *person, PlayerInfo *playerInfo) {
    u16 shopId = joinAveTextHandler(person, JOIN_AVE_PARAM_SHOP_ID, NULL);
    u8 version = func_02008bfc(playerInfo);
    u16 trainerId = getIDAsUInt(playerInfo);

    if (shopId == 0) {
        return JoinAvenue_GetDefaultShop(trainerId, version);
    }
    return shopId;
}

static u32 JoinAvenue_GetDefaultShop(u16 trainerId, u16 version) {
    switch (sDefaultShopKinds[trainerId % 7][0]) {
    default:
    case 0:
    case 5:
        switch (version) {
        case VERSION_BLACK:
            return 1;
        case VERSION_WHITE:
            return 81;
        case VERSION_BLACK2:
            return 161;
        case VERSION_WHITE2:
        default:
            return 241;
        }
    case 4:
        switch (version) {
        case VERSION_BLACK:
            return 41;
        case VERSION_WHITE:
            return 121;
        case VERSION_BLACK2:
            return 201;
        case VERSION_WHITE2:
        default:
            return 281;
        }
    case 2:
        switch (version) {
        case VERSION_BLACK:
            return 21;
        case VERSION_WHITE:
            return 101;
        case VERSION_BLACK2:
            return 181;
        case VERSION_WHITE2:
        default:
            return 261;
        }
    case 3:
        switch (version) {
        case VERSION_BLACK:
            return 31;
        case VERSION_WHITE:
            return 111;
        case VERSION_BLACK2:
            return 191;
        case VERSION_WHITE2:
        default:
            return 271;
        }
    case 6:
        switch (version) {
        case VERSION_BLACK:
            return 61;
        case VERSION_WHITE:
            return 141;
        case VERSION_BLACK2:
            return 221;
        case VERSION_WHITE2:
        default:
            return 301;
        }
    case 1:
        switch (version) {
        case VERSION_BLACK:
            return 11;
        case VERSION_WHITE:
            return 91;
        case VERSION_BLACK2:
            return 171;
        case VERSION_WHITE2:
        default:
            return 251;
        }
    case 7:
        switch (version) {
        case VERSION_BLACK:
            return 71;
        case VERSION_WHITE:
            return 151;
        case VERSION_BLACK2:
            return 231;
        case VERSION_WHITE2:
        default:
            return 311;
        }
    }
}

void func_02038bc8(u32 id) {
    JoinAvenuePerson *player = func_02038a18(sJoinAvenue.occupants);
    RTCDate today;
    RTCDate date;
    RTCDate dateA;
    RTCDate dateB;
    u16 todayPacked;
    BOOL found = FALSE;
    s32 todayDays;
    s32 days;
    int i;
    int j;

    RTC_GetCachedDate(&today);
    todayPacked = getYearMonthDay(&today);
    todayDays = func_0207d0b4(&today);

    // Forget the activities dated after today
    for (i = 0; i < JOIN_AVE_RECENT_COUNT; i++) {
        u32 recentId = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_ID + i, NULL);

        JoinAvenue_UnpackDate(joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_DATE + i, NULL), &date);
        days = func_0207d0b4(&date);
        if (recentId == 0) {
            days = 0;
        }
        if (days > todayDays) {
            JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_ID + i, 0);
            JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_DATE + i, 0);
        }
    }

    if (joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_ID, NULL) == 0) {
        // The first activity
        JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_ID, id);
    } else {
        for (i = 0; i < JOIN_AVE_RECENT_COUNT; i++) {
            if (id == joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_ID + i, 0)) {
                JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_ID + i, 0);
                JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_DATE + i, 0);
                found = TRUE;
                break;
            }
        }
        if (found) {
            // Close the gap the activity leaves
            for (i = JOIN_AVE_RECENT_COUNT - 1; i >= 1; i--) {
                u32 recentId = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_ID + i, NULL);
                u16 recentDate = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_DATE + i, NULL);
                u32 prev = i - 1;

                if (recentId == 0) {
                    u32 prevId = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_ID + prev, NULL);
                    u16 prevDate = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_DATE + prev, NULL);

                    JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_ID + i, prevId);
                    JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_DATE + i, prevDate);
                    JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_ID + prev, recentId);
                    JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_DATE + prev, recentDate);
                }
            }
        } else {
            // Make room at the front, forgetting the oldest
            for (i = JOIN_AVE_RECENT_COUNT - 1; i >= 0; i--) {
                u32 next = i + 1;

                if (next < JOIN_AVE_RECENT_COUNT) {
                    u32 recentId = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_ID + i, NULL);
                    u16 recentDate = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_DATE + i, NULL);

                    JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_ID + next, recentId);
                    JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_DATE + next, recentDate);
                }
            }
        }
        JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_ID, id);
    }
    JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_DATE, todayPacked);

    // Sort them, the latest first
    for (i = 0; i < JOIN_AVE_RECENT_COUNT - 1; i++) {
        for (j = JOIN_AVE_RECENT_COUNT - 1; j > i; j--) {
            s16 a = j - 1;
            s16 b = j;
            u32 idA = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_ID + a, NULL);
            u16 packedA = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_DATE + a, NULL);
            u32 idB = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_ID + b, NULL);
            u16 packedB = joinAveTextHandler(player, JOIN_AVE_PARAM_RECENT_DATE + b, NULL);
            s32 daysB;

            JoinAvenue_UnpackDate(packedA, &dateA);
            JoinAvenue_UnpackDate(packedB, &dateB);
            days = func_0207d0b4(&dateA);
            daysB = func_0207d0b4(&dateB);
            if (idA == 0) {
                days = 0;
            }
            if (idB == 0) {
                daysB = 0;
            }
            if (days < daysB) {
                JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_ID + a, idB);
                JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_DATE + a, packedB);
                JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_ID + b, idA);
                JoinAvenuePerson_SetParam(player, JOIN_AVE_PARAM_RECENT_DATE + b, packedA);
            }
        }
    }
}

void func_02038e54(JoinAvenueInfo *info) {
    MsgData *msgData;
    StrBuf *str;

    sys_memset(info, 0, sizeof(JoinAvenueInfo));
    info->name[0] = GFL_StrBufGetTerminator();
    info->name2[0] = GFL_StrBufGetTerminator();
    info->seed = GFL_RandomLC(0);
    info->rank = 1;
    info->unkDC_1 = TRUE;
    sys_memset32(0xffffffff, info->visitorIds, sizeof(info->visitorIds));

    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, MSG_JOIN_AVENUE_NAMES, HEAPID_TAIL(HEAPID_SAVEDATA));
    str = GFL_MsgDataLoadStrbufNew(msgData, MSG_JOIN_AVENUE_DEFAULT_NAME);
    GFL_StrBufStoreString(str, info->name, NELEMS(info->name));
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);

    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, MSG_JOIN_AVENUE_DEFAULTS, HEAPID_TAIL(HEAPID_SAVEDATA));
    str = GFL_MsgDataLoadStrbufNew(msgData, MSG_JOIN_AVENUE_DEFAULT_NAME2);
    GFL_StrBufStoreString(str, info->name2, NELEMS(info->name2));
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
}

u32 JoinAvenue_GetParam(JoinAvenueInfo *info, u32 param, void *buffer) {
    switch (param) {
    case 0:
        if (buffer != NULL) {
            wcharsncpy(info->name, buffer, NELEMS(info->name));
        }
        return (u32)info->name;
    case 1:
        if (buffer != NULL) {
            wcharsncpy(info->name2, buffer, 11);
        }
        return (u32)info->name2;
    case 2:
        return info->rank;
    case 3:
        return info->unkD4;
    case 4:
        return info->seed;
    case 5:
        return info->unkDA;
    case 6:
        return info->unkDC_0;
    case 7:
        return info->unkDC_1;
    case 8:
        return info->unkDC_2;
    case 9:
        return info->unkDC_3;
    case 10:
        return info->unkDC_6;
    case 11:
        return info->unkDC_4;
    case 12:
        return info->unkDC_5;
    case 13:
        return info->unkDC_14;
    case 14:
        return info->unkDC_16;
    case 15:
        return info->unkDC_18;
    case 16:
        return info->unkDC_7;
    case 17:
        return info->unkDC_8;
    case 18:
        return info->unkDC_11;
    case 19:
        return info->unkDC_9;
    case 20:
        return info->unkDC_10;
    case 21:
        return info->unkDC_15;
    case 22:
        return info->unkDC_17;
    case 23:
        return info->unkDC_19;
    case 24:
        return info->unkDC_12;
    case 25:
        return info->unkDC_13;
    case 26:
        return info->unkE8;
    case 27:
        return info->unkEA;
    }
    return 0;
}

void func_02039064(JoinAvenueInfo *info, u32 param, u32 value) {
    switch (param) {
    case 0:
        wcharsncpy((const u16 *)value, info->name, NELEMS(info->name));
        break;
    case 1:
        wcharsncpy((const u16 *)value, info->name2, 11);
        break;
    case 2:
        info->rank = value;
        break;
    case 3:
        info->unkD4 = value;
        break;
    case 4:
        break;
    case 5:
        info->unkDA = value;
        break;
    case 6:
        info->unkDC_0 = value;
        break;
    case 7:
        info->unkDC_1 = value;
        break;
    case 8:
        info->unkDC_2 = value;
        break;
    case 9:
        info->unkDC_3 = value;
        break;
    case 10:
        info->unkDC_6 = value;
        break;
    case 11:
        info->unkDC_4 = value;
        break;
    case 12:
        info->unkDC_5 = value;
        break;
    case 13:
        info->unkDC_14 = value;
        break;
    case 14:
        info->unkDC_16 = value;
        break;
    case 15:
        info->unkDC_18 = value;
        break;
    case 16:
        info->unkDC_7 = value;
        break;
    case 17:
        info->unkDC_8 = value;
        break;
    case 18:
        info->unkDC_11 = value;
        break;
    case 19:
        info->unkDC_9 = value;
        break;
    case 20:
        info->unkDC_10 = value;
        break;
    case 21:
        info->unkDC_15 = value;
        break;
    case 22:
        info->unkDC_17 = value;
        break;
    case 23:
        info->unkDC_19 = value;
        break;
    case 24:
        info->unkDC_12 = value;
        break;
    case 25:
        info->unkDC_13 = value;
        break;
    case 26:
        info->unkE8 = value;
        break;
    case 27:
        info->unkEA = value;
        break;
    }
}

void func_020392d4(JoinAvenueInfo *info, BOOL fullDay) {
    if (!fullDay) {
        sys_memset32(0xffffffff, info->visitorIds, sizeof(info->visitorIds));
        info->visitorIdIndex = 0;
        info->visitorIdCount = 0;
        info->seed = gfRand(info->seed);
        info->unkDC_0 = FALSE;
        info->unkDC_12 = FALSE;
    }
    if (info->unkEA != 0) {
        info->unkE8++;
        if (info->unkE8 >= 7) {
            info->unkEA = 0;
            info->unkE8 = 0;
        }
    }
}

static void JoinAvenueInfo_AddVisitorId(JoinAvenueInfo *info, u32 id) {
    int count;

    info->visitorIds[info->visitorIdIndex] = id;
    info->visitorIdIndex++;
    info->visitorIdIndex %= JOIN_AVE_VISITOR_ID_COUNT;
    info->visitorIdCount++;
    count = info->visitorIdCount;
    info->visitorIdCount = count > JOIN_AVE_VISITOR_ID_COUNT ? JOIN_AVE_VISITOR_ID_COUNT : count;
}

static BOOL JoinAvenueInfo_HasVisitorId(const JoinAvenueInfo *info, u32 id) {
    int i;

    for (i = 0; i < info->visitorIdCount; i++) {
        if (info->visitorIds[i] != 0xffffffff && id == info->visitorIds[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

u32 func_020393e4(JoinAvenueInfo *info, int count, u32 max) {
    return func_0203941c(JoinAvenue_GetParam(info, 4, NULL), count, max);
}

void setImportantJABlockAddresses(SaveControl *save) {
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(save);

    sJoinAvenue.info = JoinAvenue_GetInfo(joinAvenue);
    sJoinAvenue.occupants = getAddressOfBeginningOfOccupants(joinAvenue);
}

u32 func_0203941c(u32 seed, int count, u32 max) {
    u64 state = seed;
    u32 result = 0;
    int i;

    for (i = 0; i <= count; i++) {
        state = state * 0x5d588b656c078965ULL + 0x269ec3;
        if (max == 0) {
            result = state >> 32;
        } else {
            result = ((state >> 32) * max) >> 32;
        }
    }
    return result;
}

static u32 gfRand(u32 seed) {
    if (seed == 0) {
        return GFL_RandomLC(0);
    }
    return ((u64)seed * GF_RAND_MULTIPLIER + GF_RAND_INCREMENT) >> 32;
}

u16 func_020394b0(u16 shopId, u16 trainerId, u16 version, BOOL unk18, JoinAvenueInfo *info, ResortShopData *shops) {
    u32 unk13 = JoinAvenue_GetParam(info, 13, NULL);
    const u16 *shop = ResortShopData_GetShop(shops, shopId);
    u16 level = ResortShopData_GetShopParam(shop, 2);
    u16 kind = ResortShopData_GetShopParam(shop, 0);

    if (shopId == 0 || (unk13 == 0 && kind == 5) || (level != 0 && !unk18)) {
        return JoinAvenue_GetDefaultShop(trainerId, version);
    }
    return shopId;
}
