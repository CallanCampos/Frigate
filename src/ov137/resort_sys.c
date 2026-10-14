#include "field/resort.h"
#include "types.h"
#include "constants/arc.h"
#include "constants/flags.h"
#include "constants/script_text_banks.h"
#include "constants/species.h"
#include "constants/version.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_script.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "nitro/rtc.h"
#include "save/event_work.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/resort_binary.h"
#include "system/resort_layout.h"
#include "system/resort_work.h"
#include "system/union_view.h"
#include "system/wordset.h"

// The Join Avenue's system (resort_sys.c, from its assert string): the avenue's people and their data, its tables and
// the questions the scripts ask about them. The scripts get a message of the avenue's texts, SCRIPT_TEXT_GLOBAL_10685,
// by the number of the question, a man's or a woman's version by the person's gender

// The kinds of shop that the avenue's ranks unlock, and the people of the avenue
#define RESORT_SHOP_KIND_COUNT 8
#define RESORT_PERSON_COUNT 8

// The fields of the info that JoinAvenue_GetParam reads and func_02039064 sets: the avenue's rank, and the popularity
// points toward the next rank
#define RESORT_INFO_RANK 2
#define RESORT_INFO_POPULARITY 3

// The values of the ResortWork that the visits use: the group of the day's visits and their count, then a flag for
// each of the eight people who has had a visit
#define RESORT_WORK_GROUP 9
#define RESORT_WORK_VISITS 10
#define RESORT_WORK_MET_FIRST 11
#define RESORT_WORK_MET_LAST 18

// The base of the messages of the page of a ranking in SCRIPT_TEXT_GLOBAL_10685
#define RESORT_MSG_RANK_PAGE 2173

struct ResortSys {
    ResortSysSetup setup;
    ResortShopData *shopData;
    // The person types: their kinds of shop, what they wear and say
    ResortBinary *types;
    // The avenue's ranks: the popularity they need and the shops they bring
    ResortBinary *ranks;
    ResortBinary *unk20;
    ResortBinary *unk24;
    // The texts of a trainer type, by column
    ResortBinary *texts;
    // The shops that open at a popularity
    ResortBinary *unk2c;
    ResortPersonData **datas;
    ResortSlots *slots;
    ResortWork *work;
};

// A value of a table, by the key before it
typedef struct {
    u16 key;
    u16 value;
} ResortKeyValue;

// What the avenue's ranks unlock, in order
typedef struct {
    u32 infoParamA;
    u32 infoParamB;
    u32 id;
    BOOL (*check)(ResortSys *sys, GameData *gameData);
    BOOL flag;
} ResortUnlock;

// The people next to a person at the avenue, then the others, each ended by -1
typedef struct {
    s8 near[8];
    s8 far[8];
} ResortPeopleRow;

// A person of a ranking, and where the ranking puts them
typedef struct {
    u32 index;
    u32 value;
} ResortRankEntry;

// A person of the ranking of ResortSys_GetRecordRank, who may be the one asked about
typedef struct {
    u32 index;
    u32 value;
    BOOL isSelf;
} ResortRankSelf;

static ResortBinary *ResortSys_GetPersonTypes(ResortSys *sys);
static u32 ResortSys_PickByGender(u32 gender, u32 male, u32 female);
static BOOL ResortSys_MatchCondition(u32 type, JoinAvenuePerson *person, ResortSys *sys, u32 *row);
static u32 ResortSys_RollTier(ResortPersonData *data, ResortSys *sys);
static BOOL ResortSys_IsTypeAvailable(ResortSys *sys, ResortBinary *table, u16 row);
static u32 ResortSys_PickTypeOfTier(ResortPersonData *data, ResortSys *sys, u32 tier);
static u32 ResortSys_ChoosePersonType(ResortPersonData *data, ResortSys *sys, GameData *gameData);
static u32 ResortSys_FindUnlockRow(ResortBinary *table, u32 id);
static u32 ResortSys_GetRecordRank(JoinAvenueOccupants *occupants, JoinAvenuePerson *person, u32 page);
static BOOL ResortActor_IsBehindCamera(FieldActor *actor, FieldCamera *camera);
static BOOL ResortUnlock_Always(ResortSys *sys, GameData *gameData);
static BOOL ResortUnlock_FlagSet(ResortSys *sys, GameData *gameData);
static BOOL ResortUnlock_AllPeople(ResortSys *sys, GameData *gameData);
static BOOL ResortSys_GetEmptyData(ResortSys *sys, u32 kind, u32 row, u16 *slot, ResortPersonData **data);
static void ResortEntry_Fill(JoinAvenueEntry *entry, u16 shopKind, ResortBinary *table, u16 row, HeapID heapId);
static void ResortEntry_Setup(ResortSys *sys, GameData *gameData, JoinAvenueEntry *entry, u32 shopKind, u16 row,
                              HeapID heapId);
static void ResortRecord_Setup(ResortSys *sys, JoinAvenueRecord *record, u16 shopKind, u32 type, u16 slot,
                               HeapID heapId);
static void ResortEntry_SetupFromType(ResortSys *sys, JoinAvenueEntry *entry, u16 shopKind, u32 type, HeapID heapId);
static void ResortSys_ResetWork(ResortSys *sys, u32 value);

const u16 RESORT_SYS_UNUSED[2] = {0, 8};
// The kinds of data of the occupants
static const u32 sOccupantKinds[1] = {0};
// The shop kinds that need a row of the texts, and the type that row has
static const ResortKeyValue sShopKindTexts[2] = {{4, 2}, {5, 3}};
// The chance in 100 of a new shop of the kind in a tier
static const s32 sNewShopChances[3] = {10, 15, 20};
static const ResortKeyValue sEntryTypes[3] = {{0, 2}, {1, 3}, {3, 5}};
static const u8 sUnk021f56bc[13] = {0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6};
// How much a person's shop kind counts for, in 12-bit fixed point
static const s32 sShopKindMultipliers[3] = {0x800, 0x1000, 0x2000};
static const ResortKeyValue sRecordTypes[4] = {{4, 0}, {5, 1}, {6, 2}, {7, 3}};
// The kinds of people that hide, by the kind of the person that is hidden: the types before a 4
static const u32 sHiddenTypes[2][4] = {
    {2, 1, 4, 0},
    {0, 2, 1, 4},
};
static const u8 sUnk021f56fc[7][7] = {
    {0, 2, 7, 4, 6, 1, 3}, {1, 3, 0, 2, 7, 4, 6}, {4, 6, 1, 3, 0, 2, 7}, {2, 7, 4, 6, 1, 3, 0},
    {3, 0, 2, 7, 4, 6, 1}, {6, 1, 3, 0, 2, 7, 4}, {7, 4, 6, 1, 3, 0, 2},
};
// The data that is made by the kind of the data it is made from and the kind it is made as
static const u32 sMakeKinds[4][4] = {
    {0, 2, 1, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 4, 3, 0},
};
static const ResortPeopleRow sPeopleRows[RESORT_PERSON_COUNT] = {
    {{1, 2, -1}, {3, 4, 5, 6, 7, -1}}, {{0, 2, 3, -1}, {4, 5, 6, 7, -1}}, {{0, 1, 3, 4, -1}, {5, 6, 7, -1}},
    {{1, 2, 4, 5, -1}, {0, 6, 7, -1}}, {{2, 3, 5, 6, -1}, {0, 1, 7, -1}}, {{3, 4, 6, 7, -1}, {0, 1, 2, -1}},
    {{4, 5, 7, -1}, {0, 1, 2, 3, -1}}, {{5, 6, -1}, {0, 1, 2, 3, 4, -1}},
};
static const ResortUnlock sUnlocks[RESORT_SHOP_KIND_COUNT] = {
    {16, 8, 1, ResortUnlock_Always, FALSE},     {17, 9, 2, ResortUnlock_Always, FALSE},
    {19, 11, 4, ResortUnlock_Always, FALSE},    {20, 12, 5, ResortUnlock_Always, FALSE},
    {18, 10, 3, ResortUnlock_Always, FALSE},    {21, 13, 6, ResortUnlock_FlagSet, FALSE},
    {22, 14, 7, ResortUnlock_AllPeople, FALSE}, {23, 14, 8, ResortUnlock_Always, TRUE},
};

ResortSys *ResortSys_Create(const ResortSysSetup *setup, ResortWork *resortWork, HeapID heapId) {
    ResortSys *sys = GFL_HeapAllocate(heapId, sizeof(ResortSys), TRUE, "resort_sys.c", 75);

    sys->setup = *setup;
    sys->work = resortWork;
    sys->shopData = ResortShopData_Load(heapId);
    sys->types = ResortBinary_Load(4, 42, heapId);
    sys->ranks = ResortBinary_Load(7, 13, heapId);
    sys->unk20 = ResortBinary_Load(3, 4, heapId);
    sys->unk24 = ResortBinary_Load(8, 22, heapId);
    sys->texts = ResortBinary_Load(9, 23, heapId);
    sys->unk2c = ResortBinary_Load(5, 11, heapId);
    sys->datas = func_ov137_021f19c4(setup, heapId);
    sys->slots = func_ov137_021f1710(sys->datas, heapId);
    return sys;
}

void ResortSys_Free(ResortSys *sys) {
    func_ov137_021f1740(sys->slots);
    func_ov137_021f1ac0(sys->datas);
    ResortBinary_Free(sys->unk2c);
    ResortBinary_Free(sys->texts);
    ResortBinary_Free(sys->unk24);
    ResortBinary_Free(sys->unk20);
    ResortBinary_Free(sys->ranks);
    ResortBinary_Free(sys->types);
    ResortShopData_Free(sys->shopData);
    GFL_HeapFree(sys);
}

ResortShopData *ResortSys_GetShopData(ResortSys *sys) {
    return sys->shopData;
}

static ResortBinary *ResortSys_GetPersonTypes(ResortSys *sys) {
    return sys->types;
}

ResortBinary *ResortSys_GetRanks(ResortSys *sys) {
    return sys->ranks;
}

ResortBinary *ResortSys_GetUnk20Table(ResortSys *sys) {
    return sys->unk20;
}

ResortBinary *ResortSys_GetUnk24Table(ResortSys *sys) {
    return sys->unk24;
}

ResortBinary *ResortSys_GetTexts(ResortSys *sys) {
    return sys->texts;
}

ResortBinary *ResortSys_GetShopOpenings(ResortSys *sys) {
    return sys->unk2c;
}

ResortPersonData **ResortSys_GetDatas(ResortSys *sys) {
    return sys->datas;
}

ResortSlots *ResortSys_GetSlots(ResortSys *sys) {
    return sys->slots;
}

JoinAvenueOccupants *ResortSys_GetOccupants(ResortSys *sys) {
    return sys->setup.occupants;
}

JoinAvenuePersonList *ResortSys_GetList(ResortSys *sys) {
    return sys->setup.list;
}

JoinAvenuePersonList *ResortSys_GetList2(ResortSys *sys) {
    return sys->setup.list2;
}

void *ResortSys_GetEntries(ResortSys *sys) {
    return sys->setup.entries;
}

JoinAvenueInfo *ResortSys_GetInfo(ResortSys *sys) {
    return sys->setup.info;
}

ResortWork *ResortSys_GetWork(ResortSys *sys) {
    return sys->work;
}

// The version of a message for the person's gender
static u32 ResortSys_PickByGender(u32 gender, u32 male, u32 female) {
    if (gender != 0) {
        male = female;
    }
    return male;
}

// The answer to a question of the scripts: a message of the avenue's texts for the person asked about, or a number
u16 ResortSys_GetScriptValue(u32 query, ResortSys *sys, ResortPersonData *data, GameData *gameData, WordSet *wordSet,
                        HeapID heapId) {
    ResortPersonData **datas = ResortSys_GetDatas(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    ResortBinary *types = ResortSys_GetPersonTypes(sys);
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    ResortBinary *ranks = ResortSys_GetRanks(sys);
    ResortBinary *texts = ResortSys_GetTexts(sys);
    u32 kind;
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    u32 shopKind;
    u32 gender;

    kind = 0;
    shopKind = 0;
    gender = 0;
    if (data != NULL) {
        gender = func_ov137_021f1968(data, JOIN_AVE_PARAM_GENDER, NULL);
        kind = (u16)func_ov137_021f1984(data);
        shopKind = func_ov137_021f1968(data, JOIN_AVE_PARAM_SHOP_KIND, NULL);
    }

    switch (query) {
    case 0:
    case 1:
    case 2:
    case 3: {
        u16 result = 0;
        u32 activity = func_ov137_021f1968(data, JOIN_AVE_PARAM_RECENT_ID + query, NULL);
        u16 date = func_ov137_021f1968(data, JOIN_AVE_PARAM_RECENT_DATE + query, NULL);

        WordSetNumber(wordSet, 0, (date >> 9) & 0x7f, 2, NUM_PAD_ZERO, TRUE);
        WordSetNumber(wordSet, 2, (date >> 5) & 0xf, 2, NUM_PAD_NONE, TRUE);
        WordSetNumber(wordSet, 3, date & 0x1f, 2, NUM_PAD_NONE, TRUE);
        if (activity != 0) {
            result = activity + ResortSys_PickByGender(gender, 1048, 1081);
        }
        return result;
    }
    case 4: {
        u32 personType;

        if (kind == 1 && shopKind == 5) {
            return ResortSys_GetTrainerTypeText(texts, data, 12);
        }
        personType = func_ov137_021f1968(data, JOIN_AVE_PARAM_UNK_6, NULL);
        if (personType == 0) {
            personType = ResortSys_ChoosePersonType(data, sys, gameData);
            func_ov137_021f1974(data, JOIN_AVE_PARAM_UNK_6, personType);
        }
        if (gender == 0) {
            return ResortBinary_Get(types, personType, 0);
        }
        return ResortBinary_Get(types, personType, 1);
    }
    case 5: {
        u16 shopId = func_ov137_021f1968(data, JOIN_AVE_PARAM_SHOP_ID, NULL);
        u16 trainerId = func_ov137_021f1968(data, JOIN_AVE_PARAM_TRAINER_ID, NULL);
        u16 version = func_ov137_021f1968(data, JOIN_AVE_PARAM_VERSION, NULL);
        u16 unk18 = func_ov137_021f1968(data, JOIN_AVE_PARAM_UNK_18, NULL);
        u32 shop = func_020394b0(shopId, trainerId, version, unk18, info, shopData);

        return ResortShopData_GetShopParam(ResortShopData_GetShop(shopData, shop), 0) +
               ResortSys_PickByGender(gender, 1120, 1128);
    }
    case 6: {
        u16 shopId = func_020363e0(func_02038470(func_ov137_021f1980(data)), 0);

        return ResortShopData_GetShopParam(ResortShopData_GetShop(shopData, shopId), 0) +
               ResortSys_PickByGender(gender, 328, 336);
    }
    case 8:
        return ResortSys_PickByGender(gender, 1116, 1117);
    case 9:
        return ResortSys_PickByGender(gender, 1118, 1119);
    case 10:
        return ResortSys_PickByGender(gender, 1114, 1115);
    case 11:
        return ResortSys_PickByGender(gender, 1048, 1081);
    case 50: {
        u32 message;
        u32 unk26 = func_ov137_021f1968(data, JOIN_AVE_PARAM_UNK_26, NULL);
        u32 dataCount = func_ov137_021f1968(data, JOIN_AVE_PARAM_DATA_COUNT, NULL);
        u32 version = func_ov137_021f1968(data, JOIN_AVE_PARAM_VERSION, NULL);

        if (version == VERSION_WHITE2 || version == VERSION_BLACK2) {
            if (unk26 == 8) {
                message = ResortSys_PickByGender(gender, 8, 27);

                if (dataCount == 0) {
                    return ResortSys_PickByGender(gender, 18, 37);
                }
                return message + dataCount - 1;
            } else if (unk26 == 9) {
                return ResortSys_PickByGender(gender, 15, 34);
            } else if (unk26 == 7) {
                u16 species = func_ov137_021f1968(data, JOIN_AVE_PARAM_ENTRY_SPECIES, NULL);

                if (species == 0) {
                    species = SPECIES_BULBASAUR;
                }
                WordSet_LoadSpeciesName(wordSet, 1, species);
                return ResortSys_PickByGender(gender, 7, 26);
            } else if (unk26 == 6) {
                u16 species = func_ov137_021f1968(data, JOIN_AVE_PARAM_ENTRY_SPECIES, NULL);

                if (species == 0) {
                    species = SPECIES_BULBASAUR;
                }
                WordSet_LoadSpeciesName(wordSet, 1, species);
                return ResortSys_PickByGender(gender, 6, 25);
            } else if (unk26 == 0) {
                break;
            }
            message = ResortSys_PickByGender(gender, 1, 20);
            return message + unk26 - 1;
        } else if (unk26 == 6) {
            u16 species = func_ov137_021f1968(data, JOIN_AVE_PARAM_ENTRY_SPECIES, NULL);

            if (species == 0) {
                species = SPECIES_BULBASAUR;
            }
            WordSet_LoadSpeciesName(wordSet, 1, species);
            return ResortSys_PickByGender(gender, 6, 25);
        } else if (unk26 == 7) {
            u16 species = func_ov137_021f1968(data, JOIN_AVE_PARAM_ENTRY_SPECIES, NULL);

            if (species == 0) {
                species = SPECIES_BULBASAUR;
            }
            WordSet_LoadSpeciesName(wordSet, 1, species);
            return ResortSys_PickByGender(gender, 7, 26);
        }
        return ResortSys_PickByGender(gender, 19, 38);
    }
    case 51: {
        u32 country = func_ov137_021f1968(data, JOIN_AVE_PARAM_COUNTRY, NULL);
        u32 area = func_ov137_021f1968(data, JOIN_AVE_PARAM_AREA, NULL);

        if (country != 0 && area != 0) {
            loadCountryToStrbuf(wordSet, 1, country);
            loadCountryAreaToStrbuf(wordSet, 2, country, area);
            return ResortSys_PickByGender(gender, 40, 40);
        }
        if (country != 0) {
            loadCountryToStrbuf(wordSet, 1, country);
            return ResortSys_PickByGender(gender, 41, 44);
        }
        return ResortSys_PickByGender(gender, 39, 42);
    }
    case 52:
        return ResortSys_PickByGender(gender, 51, 55);
    case 53:
        return ResortSys_PickByGender(gender, 52, 56);
    case 54:
        return ResortSys_PickByGender(gender, 53, 57);
    case 55:
        return ResortSys_PickByGender(gender, 54, 58);
    case 56: {
        if (kind == 1 && shopKind == 5) {
            return ResortSys_GetTrainerTypeText(texts, data, 13);
        }
        return ResortSys_PickByGender(gender, 75, 76);
    }
    case 57:
        return ResortSys_PickByGender(gender, 49, 50);
    case 58: {
        if (kind == 1 && shopKind == 5) {
            return ResortSys_GetTrainerTypeText(texts, data, 22);
        }
        return ResortSys_PickByGender(gender, 47, 48);
    }
    case 59:
        return ResortSys_PickByGender(gender, 45, 46);
    case 60: {
        if (kind == 1 && shopKind == 5) {
            return ResortSys_GetTrainerTypeText(texts, data, 14);
        }
        return 268;
    }
    case 62:
        return ResortSys_PickByGender(gender, 1484, 1485);
    case 80:
        return ResortSys_PickByGender(gender, 63, 64);
    case 81:
        return ResortSys_PickByGender(gender, 65, 66);
    case 82:
        return ResortSys_PickByGender(gender, 67, 68);
    case 83:
        return ResortSys_PickByGender(gender, 69, 70);
    case 84:
        return ResortSys_PickByGender(gender, 71, 72);
    case 85:
        return ResortSys_PickByGender(gender, 73, 74);
    case 100: {
        switch (shopKind) {
        case 0:
        default:
            return ResortSys_PickByGender(gender, 122, 126);
        case 1:
            return ResortSys_PickByGender(gender, 123, 127);
        case 3:
            return ResortSys_PickByGender(gender, 124, 128);
        case 2:
            return ResortSys_PickByGender(gender, 125, 129);
        }
    }
    case 101: {
        switch (shopKind) {
        case 0:
        default:
            return ResortSys_PickByGender(gender, 130, 134);
        case 1:
            return ResortSys_PickByGender(gender, 131, 135);
        case 3:
            return ResortSys_PickByGender(gender, 132, 136);
        case 2:
            return ResortSys_PickByGender(gender, 133, 137);
        }
    }
    case 102: {
        u16 rank = JoinAvenue_GetParam(info, RESORT_INFO_RANK, NULL);

        if (rank >= ResortBinary_GetLastRangeEnd(ranks)) {
            return ResortSys_PickByGender(gender, 148, 165);
        }
        return ResortSys_PickByGender(gender, 138, 155);
    }
    case 103:
        return ResortSys_PickByGender(gender, 142, 159);
    case 104:
        return ResortSys_PickByGender(gender, 143, 160);
    case 105:
        return ResortSys_PickByGender(gender, 145, 162);
    case 106:
        return ResortSys_PickByGender(gender, 144, 161);
    case 107:
        return ResortSys_PickByGender(gender, 146, 163);
    case 108:
        return ResortSys_PickByGender(gender, 147, 164);
    case 112:
        return ResortSys_PickByGender(gender, 141, 158);
    case 113:
        return ResortSys_PickByGender(gender, 139, 156);
    case 114:
        return ResortSys_PickByGender(gender, 140, 157);
    case 115:
        return ResortSys_PickByGender(gender, 153, 170);
    case 116:
        return ResortSys_PickByGender(gender, 154, 171);
    case 86: {
        u16 level = func_020363e0(func_02038470(func_ov137_021f1980(data)), 1);

        if (level >= 9) {
            return ResortSys_PickByGender(gender, 80, 85);
        }
        return ResortSys_PickByGender(gender, 79, 84);
    }
    case 87:
        return ResortSys_PickByGender(gender, 77, 78);
    case 125:
        return 61;
    case 130:
        return 62;
    case 120:
        return ResortSys_PickByGender(gender, 59, 60);
    case 140:
        return ResortSys_PickByGender(gender, 361, 367);
    case 141:
        return ResortSys_PickByGender(gender, 360, 366);
    case 142:
        return ResortSys_PickByGender(gender, 362, 368);
    case 143:
        return ResortSys_PickByGender(gender, 363, 369);
    case 144:
        return ResortSys_PickByGender(gender, 364, 370);
    case 145:
        return ResortSys_PickByGender(gender, 365, 371);
    case 163:
        return ResortSys_PickByGender(gender, 344, 345);
    case 164:
        return ResortSys_PickByGender(gender, 346, 347);
    case 165:
        return ResortSys_PickByGender(gender, 348, 349);
    case 166:
        return ResortSys_PickByGender(gender, 350, 351);
    case 167:
        return ResortSys_PickByGender(gender, 352, 353);
    case 168:
        return ResortSys_PickByGender(gender, 354, 355);
    case 169:
        return ResortSys_PickByGender(gender, 356, 357);
    case 176:
        return ResortSys_PickByGender(gender, 358, 359);
    case 150:
        return ResortSys_PickByGender(gender, 373, 375);
    case 151:
        return ResortSys_PickByGender(gender, 372, 374);
    case 152:
        return ResortSys_PickByGender(gender, 377, 379);
    case 153:
        return ResortSys_PickByGender(gender, 376, 378);
    case 154:
        return ResortSys_PickByGender(gender, 381, 383);
    case 155:
        return ResortSys_PickByGender(gender, 380, 382);
    case 156:
        return ResortSys_PickByGender(gender, 385, 392);
    case 157:
        return ResortSys_PickByGender(gender, 384, 391);
    case 158:
        return ResortSys_PickByGender(gender, 386, 393);
    case 159:
        return ResortSys_PickByGender(gender, 387, 394);
    case 160:
        return ResortSys_PickByGender(gender, 388, 395);
    case 161:
        return ResortSys_PickByGender(gender, 389, 396);
    case 162:
        return ResortSys_PickByGender(gender, 390, 397);
    case 170:
        return ResortSys_GetTrainerTypeText(texts, data, 11);
    case 171:
        return ResortSys_GetTrainerTypeText(texts, data, 17);
    case 172:
        return ResortSys_GetTrainerTypeText(texts, data, 18);
    case 173:
        return ResortSys_GetTrainerTypeText(texts, data, 19);
    case 174:
        return ResortSys_GetTrainerTypeText(texts, data, 20);
    case 175:
        return ResortSys_GetTrainerTypeText(texts, data, 21);
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19: {
        ResortPersonData *occupant;
        StrBuf *name;
        u32 shopName;
        u16 shopId;

        query -= 12;
        occupant = func_ov137_021f1b94(datas, 0, query);
        shopName =
            ResortShopData_GetShopParam(ResortShopData_GetPersonShop(shopData, func_ov137_021f1980(occupant)), 0);
        shopId = func_020363e0(func_02038470(func_ov137_021f1980(occupant)), 0);
        name = ResortSys_FormatShopText(shopData, occupant, shopId, heapId);
        func_0202437c(wordSet, 0, name, func_ov137_021f1968(occupant, JOIN_AVE_PARAM_GENDER, NULL), 1, 2);
        GFL_StrBufFree(name);
        return shopName + 2165;
    }
    case 88: {
        u32 message;
        u32 candidates[9] = {0};
        s32 count = 0;
        JoinAvenuePerson *person = func_ov137_021f1980(data);
        BOOL hasRecord = FALSE;
        s32 i;
        u32 pick;

        for (i = 0; i < JOIN_AVE_RECORD_COUNT; i++) {
            JoinAvenueRecord *record = func_0203888c(occupants, i);

            if (!func_020384e0(record) && func_020385a8(record, JOIN_AVE_PARAM_SHOP_KIND, NULL) == 2) {
                hasRecord = TRUE;
                break;
            }
        }
        if (!hasRecord) {
            return 0;
        }
        for (i = 0; i <= 8; i++) {
            if (ResortSys_GetRecordRank(occupants, person, i) == 0) {
                candidates[count] = i;
                count++;
            }
        }
        if (count == 0) {
            return 0;
        }
        pick = GFL_RandomLC(count);
        message = ResortSys_PickByGender(gender, 103, 112);
        return message + candidates[pick];
    }
    case 89:
        return ResortSys_PickByGender(gender, 89, 96);
    case 90:
        return ResortSys_PickByGender(gender, 90, 97);
    case 91:
        return ResortSys_PickByGender(gender, 91, 98);
    case 92:
        return ResortSys_PickByGender(gender, 92, 99);
    case 95:
        return ResortSys_PickByGender(gender, 93, 100);
    case 94:
        return ResortSys_PickByGender(gender, 94, 101);
    case 93:
        return ResortSys_PickByGender(gender, 95, 102);
    case 96: {
        u32 level = ResortSys_GetVisitPoints(sys, data);

        if (!ResortSys_IsFull(sys)) {
            return 0;
        }
        if (level >= 20) {
            return ResortSys_PickByGender(gender, 83, 88);
        }
        if (level >= 10) {
            return ResortSys_PickByGender(gender, 82, 87);
        }
        if (level >= 5) {
            return ResortSys_PickByGender(gender, 81, 86);
        }
        return 0;
    }
    }
    return 0;
}

// Whether a person matches one of the conditions of a type of person, and the row of the condition that is chosen: the
// one of priority 2, else the highest
static BOOL ResortSys_MatchCondition(u32 type, JoinAvenuePerson *person, ResortSys *sys, u32 *row) {
    s32 i;
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    ResortBinary *types = ResortSys_GetPersonTypes(sys);
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    JoinAvenueShop *shop = func_02038470(person);
    const u16 *shopRow = ResortShopData_GetPersonShop(shopData, person);
    u32 shopKind;
    BOOL found;
    s32 best;
    u32 index;

    func_020363e0(shop, 1);
    ResortShopData_GetShopParam(shopRow, 0);
    shopKind = ResortShopData_GetShopParam(shopRow, 0);
    found = FALSE;
    best = found - 1;
    index = 0;

    for (i = 0; i < 6; i++) {
        u16 condition = ResortBinary_Get(types, type, i * 6 + 6);
        u16 value = ResortBinary_Get(types, type, i * 6 + 7);
        u16 priority = ResortBinary_Get(types, type, i * 6 + 8);
        BOOL matches = FALSE;

        switch (condition) {
        case 0:
            if (shopKind == value) {
                matches = TRUE;
            }
            break;
        case 1:
            if (value == joinAveTextHandler(person, JOIN_AVE_PARAM_GENDER, NULL)) {
                matches = TRUE;
            }
            break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            if (value + 1 == joinAveTextHandler(person, JOIN_AVE_PARAM_VALUE + (u16)(condition - 2), NULL)) {
                matches = TRUE;
            }
            break;
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27: {
            u16 record = condition - 19;

            if (value == 0) {
                if (ResortSys_GetRecordRank(occupants, person, record) == 0) {
                    matches = TRUE;
                }
            } else if (record == 8) {
                if (joinAveTextHandler(person, JOIN_AVE_PARAM_MEDAL_COUNT, NULL) >= value) {
                    matches = TRUE;
                }
            } else if (record < 8) {
                if (joinAveTextHandler(person, JOIN_AVE_PARAM_RECORD1 + record, NULL) >= value) {
                    matches = TRUE;
                }
            }
            break;
        }
        default:
            matches = FALSE;
            break;
        }

        if (matches) {
            found = TRUE;
            if (priority == 2) {
                index = i;
                break;
            }
            if (best < priority) {
                best = priority;
                index = i;
            }
        }
    }

    if (row != NULL) {
        *row = index;
    }
    return found;
}

BOOL ResortSys_GetPersonVisit(ResortPersonData *data, JoinAvenuePerson *person, ResortSys *sys, u16 *message, u16 *a4,
                         u16 *a5) {
    ResortBinary *types = ResortSys_GetPersonTypes(sys);
    ResortBinary *texts = ResortSys_GetTexts(sys);
    u16 personType;
    u16 gender;
    BOOL isShop5;
    u32 multiplier;
    BOOL found;
    u16 kind;
    u16 shopKind;
    u32 index;

    personType = func_ov137_021f1968(data, JOIN_AVE_PARAM_UNK_6, NULL);
    isShop5 = FALSE;
    gender = func_ov137_021f1968(data, JOIN_AVE_PARAM_GENDER, NULL);
    multiplier = 0;
    kind = func_ov137_021f1984(data);
    shopKind = func_ov137_021f1968(data, JOIN_AVE_PARAM_SHOP_KIND, NULL);
    if (kind == 1 && shopKind == 5) {
        isShop5 = TRUE;
    }

    found = ResortSys_MatchCondition(personType, person, sys, &index);
    if (joinAveTextHandler(person, JOIN_AVE_PARAM_GENDER, NULL) == 0) {
        *a4 = ResortBinary_Get(types, personType, index * 6 + 9);
    } else {
        *a4 = ResortBinary_Get(types, personType, index * 6 + 10);
    }

    if (found) {
        *a5 = ResortBinary_Get(types, personType, 4);
        multiplier = ResortBinary_Get(types, personType, index * 6 + 8);
        *a5 = (*a5 * sShopKindMultipliers[multiplier]) >> 12;
        if (func_ov137_021f1984(data) == 1) {
            switch (func_ov137_021f1968(data, JOIN_AVE_PARAM_SHOP_KIND, NULL)) {
            case 5:
                *a5 = (*a5 * 0x2000) >> 12;
                break;
            case 4:
                *a5 = (*a5 * 0xc00) >> 12;
                break;
            }
        }
    }

    if (found) {
        if (isShop5) {
            *message = ResortSys_GetTrainerTypeText(texts, data, 15);
        } else {
            multiplier += 251;
            *message = multiplier;
        }
    } else if (isShop5) {
        *message = ResortSys_GetTrainerTypeText(texts, data, 16);
    } else if (gender == 0) {
        *message = 256;
    } else {
        *message = 257;
    }
    return found;
}

// Adds points to the avenue's popularity, and returns what that brought: 0 nothing, 1 a new rank, 2 no more ranks, 3
// a new shop, 4 a new rank with new texts, 5 both
u32 ResortSys_AddPopularity(ResortSys *sys, GameData *gameData, u32 points, u16 *rank) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    ResortBinary *ranks = ResortSys_GetRanks(sys);
    BOOL newTexts = FALSE;
    u32 popularity = JoinAvenue_GetParam(info, RESORT_INFO_POPULARITY, NULL);
    u32 required;
    u32 range;
    u32 nextRange;
    u32 oldMessage;
    u32 shop;
    BOOL newShop;

    *rank = JoinAvenue_GetParam(info, RESORT_INFO_RANK, NULL);
    if (*rank >= ResortBinary_GetLastRangeEnd(ranks)) {
        return 2;
    }

    range = ResortBinary_FindRange(ranks, *rank);
    required = ResortBinary_Get(ranks, range, 5);
    if (popularity + points < required) {
        func_02039064(info, RESORT_INFO_POPULARITY, popularity + points);
        return 0;
    }

    (*rank)++;
    func_02039064(info, RESORT_INFO_RANK, *rank);
    func_02039064(info, RESORT_INFO_POPULARITY, 0);
    nextRange = ResortBinary_FindRange(ranks, *rank);
    oldMessage = ResortBinary_Get(ranks, range, 6);
    shop = ResortSys_GetUnlockForRank(sys, gameData, *rank);
    if (oldMessage != ResortBinary_Get(ranks, nextRange, 6)) {
        newTexts = TRUE;
    }

    if (shop != 0) {
        if (shop == 8) {
            func_02039064(info, 0x17, 1);
        }
        func_02039064(info, shop + 15, 1);
        newShop = TRUE;
    } else {
        newShop = FALSE;
    }

    if (newShop && newTexts) {
        return 5;
    }
    if (newShop) {
        return 3;
    }
    return newTexts ? 4 : 1;
}

// Adds experience to a person's shop, and returns what that brought: 0 nothing, 1 a new level, 2 a new level of a
// different kind of shop, 3 the highest level already
u32 ResortSys_AddShopExperience(JoinAvenuePerson *person, ResortSys *sys, u32 points, u16 *level) {
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    JoinAvenueShop *shop = func_02038470(person);
    u32 experience = func_020363e0(shop, 2);
    const u16 *shopRow = ResortShopData_GetPersonShop(shopData, person);
    u32 needed = ResortShopData_GetShopParam(shopRow, 3);
    u32 kind = ResortShopData_GetShopParam(shopRow, 0);
    u32 column1 = ResortShopData_GetShopParam(shopRow, 1);
    const u16 *newRow;

    *level = func_020363e0(shop, 1);
    if (*level >= 9) {
        return 3;
    }
    if (experience + points < needed) {
        func_0203640c(shop, 2, experience + points);
        return 0;
    }

    (*level)++;
    func_0203640c(shop, 1, *level);
    func_0203640c(shop, 2, 0);
    func_0203640c(shop, 3, 0);
    func_0203640c(shop, 0, ResortShopData_FindShop(shopData, kind, *level, column1));
    newRow = ResortShopData_GetPersonShop(shopData, person);
    if (ResortSys_AreAllShopsMaxLevel(sys)) {
        func_02039064(info, 0x1b, 1);
        func_02039064(info, 0x1a, 0);
    }
    if (ResortShopData_GetShopParam(shopRow, 6) != ResortShopData_GetShopParam(newRow, 6)) {
        return 2;
    }
    return 1;
}

u32 ResortSys_GetInfoFlag1b(ResortSys *sys) {
    return JoinAvenue_GetParam(ResortSys_GetInfo(sys), 0x1b, NULL);
}

// Whether all of the occupants' shops are at the highest level
BOOL ResortSys_AreAllShopsMaxLevel(ResortSys *sys) {
    ResortPersonData **datas = ResortSys_GetDatas(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    ResortDataIter iter = func_ov137_021f1ae0(datas, sOccupantKinds, 1);
    ResortPersonData *data;

    for (;;) {
        data = func_ov137_021f1b0c(datas, &iter, sOccupantKinds, 1);
        if (data == NULL) {
            break;
        }
        if (func_020363e0(func_02038470(func_ov137_021f1980(data)), 1) != 9) {
            return FALSE;
        }
    }
    return TRUE;
}

// Puts a text in the word set: the one of the kind of the first parameter, about the person that the second one
// chooses: the script's own person (0xfe), none (0xff), or the one of a kind of data of the avenue by the third
void ResortSys_SetScriptWord(u32 type, u32 who, u16 index, u32 wordIndex, ResortSys *sys, ResortPerson *person,
                         WordSet *wordSet, GameData *gameData, HeapID heapId) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    ResortBinary *ranks = ResortSys_GetRanks(sys);
    JoinAvenuePerson *player = func_02038a18(ResortSys_GetOccupants(sys));
    ResortPersonData *data;
    JoinAvenuePerson *joinPerson;

    switch (who) {
    case 0xff:
        data = NULL;
        break;
    case 0xfe:
        data = func_ov137_021f1110(person);
        joinPerson = func_ov137_021f1980(data);
        break;
    default:
        data = func_ov137_021f1ba8(ResortSys_GetDatas(sys), who, index);
        joinPerson = func_ov137_021f1980(data);
        break;
    }

    switch (type) {
    case 0: {
        StrBuf *buf = GFL_StrBufCreate(8, HEAPID_TAIL(heapId));
        u32 gender = func_ov137_021f1968(data, JOIN_AVE_PARAM_GENDER, NULL);
        u16 name[8];

        func_ov137_021f1968(data, JOIN_AVE_PARAM_NAME, name);
        GFL_StrBufLoadString(buf, name);
        func_0202437c(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case 1: {
        u16 shopId = func_020363e0(func_02038470(joinPerson), 0);
        StrBuf *buf = ResortSys_FormatShopText(shopData, data, shopId, heapId);

        func_0202437c(wordSet, wordIndex, buf, func_ov137_021f1968(data, JOIN_AVE_PARAM_GENDER, NULL), 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case 14: {
        u16 shopId = func_ov137_021f1968(data, JOIN_AVE_PARAM_SHOP_ID, NULL);
        u16 trainerId = func_ov137_021f1968(data, JOIN_AVE_PARAM_TRAINER_ID, NULL);
        u16 version = func_ov137_021f1968(data, JOIN_AVE_PARAM_VERSION, NULL);
        u16 unk18 = func_ov137_021f1968(data, JOIN_AVE_PARAM_UNK_18, NULL);
        StrBuf *buf = ResortSys_FormatShopText(shopData, data,
                                          func_020394b0(shopId, trainerId, version, unk18, info, shopData), heapId);

        func_0202437c(wordSet, wordIndex, buf, func_ov137_021f1968(data, JOIN_AVE_PARAM_GENDER, NULL), 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case 3: {
        StrBuf *buf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
        u32 gender = func_ov137_021f1968(data, JOIN_AVE_PARAM_GENDER, NULL);
        u16 greeting[9];

        func_ov137_021f1968(data, JOIN_AVE_PARAM_GREETING, greeting);
        GFL_StrBufLoadString(buf, greeting);
        func_0202437c(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case 4: {
        StrBuf *buf = GFL_StrBufCreate(11, HEAPID_TAIL(heapId));
        u32 gender = getTrainerGender(GetGameDataPlayerInfo(gameData));
        u16 name[11];

        JoinAvenue_GetParam(info, 1, name);
        GFL_StrBufLoadString(buf, name);
        func_0202437c(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case 6: {
        StrBuf *buf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
        u32 gender = func_ov137_021f1968(data, JOIN_AVE_PARAM_GENDER, NULL);
        u16 message[9];

        func_ov137_021f1968(data, JOIN_AVE_PARAM_MESSAGE1, message);
        GFL_StrBufLoadString(buf, message);
        func_0202437c(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case 18: {
        StrBuf *buf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
        u32 gender = func_ov137_021f1968(data, JOIN_AVE_PARAM_GENDER, NULL);
        u16 message[9];

        func_ov137_021f1968(data, JOIN_AVE_PARAM_MESSAGE2, message);
        GFL_StrBufLoadString(buf, message);
        func_0202437c(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case 19: {
        PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
        StrBuf *buf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
        u32 gender = getTrainerGender(playerInfo);
        u16 message[9];

        joinAveTextHandler(player, JOIN_AVE_PARAM_MESSAGE1, message);
        GFL_StrBufLoadString(buf, message);
        func_0202437c(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case 20: {
        PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
        StrBuf *buf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
        u32 gender = getTrainerGender(playerInfo);
        u16 message[9];

        joinAveTextHandler(player, JOIN_AVE_PARAM_MESSAGE2, message);
        GFL_StrBufLoadString(buf, message);
        func_0202437c(wordSet, wordIndex, buf, gender, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case 21:
        WordSetNumber(wordSet, wordIndex, func_ov137_021f1968(data, JOIN_AVE_PARAM_YEAR, NULL), 2, NUM_PAD_ZERO, TRUE);
        break;
    case 22:
        WordSetNumber(wordSet, wordIndex, func_ov137_021f1968(data, JOIN_AVE_PARAM_MONTH, NULL), 2, NUM_PAD_NONE, TRUE);
        break;
    case 23:
        WordSetNumber(wordSet, wordIndex, func_ov137_021f1968(data, JOIN_AVE_PARAM_DAY, NULL), 2, NUM_PAD_NONE, TRUE);
        break;
    case 7: {
        u16 shopId = func_020363e0(func_02038470(joinPerson), 0);
        u32 shopName = ResortShopData_GetShopParam(ResortShopData_GetShop(shopData, shopId), 0);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, shopName + 280);

        func_0202437c(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    case 15: {
        u16 shopId = func_ov137_021f1968(data, JOIN_AVE_PARAM_SHOP_ID, NULL);
        u16 trainerId = func_ov137_021f1968(data, JOIN_AVE_PARAM_TRAINER_ID, NULL);
        u16 version = func_ov137_021f1968(data, JOIN_AVE_PARAM_VERSION, NULL);
        u16 unk18 = func_ov137_021f1968(data, JOIN_AVE_PARAM_UNK_18, NULL);
        u32 shopName = ResortShopData_GetShopParam(
            ResortShopData_GetShop(shopData, func_020394b0(shopId, trainerId, version, unk18, info, shopData)), 0);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, shopName + 280);

        func_0202437c(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    case 16: {
        JoinAvenuePerson *occupant = func_02038a18(ResortSys_GetOccupants(sys));
        u32 shopName = ResortShopData_GetShopParam(
            ResortShopData_GetShop(shopData, (u16)func_02038a20(occupant, GetGameDataPlayerInfo(gameData))), 0);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, shopName + 280);

        func_0202437c(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    case 8: {
        StrBuf *buf = GFL_StrBufCreate(21, HEAPID_TAIL(heapId));
        u16 name[21];

        JoinAvenue_GetParam(info, 0, name);
        GFL_StrBufLoadString(buf, name);
        func_0202437c(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        break;
    }
    case 17:
        WordSetNumber(wordSet, wordIndex, 7 - JoinAvenue_GetParam(info, 0x1a, NULL), 1, NUM_PAD_NONE, TRUE);
        break;
    case 9: {
        const u16 *shopRow = ResortShopData_GetPersonShop(shopData, joinPerson);
        JoinAvenueShop *shop = func_02038470(joinPerson);
        s32 remaining = ResortShopData_GetShopParam(shopRow, 3) - (u16)func_020363e0(shop, 2);

        if (remaining < 0) {
            remaining = 0;
        }
        WordSetNumber(wordSet, wordIndex, remaining, 5, NUM_PAD_NONE, TRUE);
        break;
    }
    case 10: {
        u16 level;

        ResortShopData_GetPersonShop(shopData, joinPerson);
        level = func_020363e0(func_02038470(joinPerson), 1);
        WordSetNumber(wordSet, wordIndex, level + 1, 4, NUM_PAD_NONE, TRUE);
        break;
    }
    case 11:
        WordSetNumber(wordSet, wordIndex, JoinAvenue_GetParam(info, RESORT_INFO_RANK, NULL), 4, NUM_PAD_NONE, TRUE);
        break;
    case 12: {
        u32 message = ResortBinary_Get(
            ranks, ResortBinary_FindRange(ranks, JoinAvenue_GetParam(info, RESORT_INFO_RANK, NULL)), 6);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, message);

        func_0202437c(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    case 13: {
        u32 rank = JoinAvenue_GetParam(info, RESORT_INFO_RANK, NULL);
        u32 popularity = JoinAvenue_GetParam(info, RESORT_INFO_POPULARITY, NULL);
        s32 remaining = ResortBinary_Get(ranks, ResortBinary_FindRange(ranks, rank), 5) - popularity;

        if (remaining < 0) {
            remaining = 0;
        }
        WordSetNumber(wordSet, wordIndex, remaining, 5, NUM_PAD_NONE, TRUE);
        break;
    }
    case 2: {
        u32 shopName = ResortShopData_GetShopParam(ResortShopData_GetPersonShop(shopData, joinPerson), 0);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, shopName + 320);

        func_0202437c(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    case 5: {
        u32 unk26 = func_ov137_021f1968(data, JOIN_AVE_PARAM_UNK_26, NULL);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        StrBuf *buf = GFL_MsgDataLoadStrbufNew(msgData, unk26 + 1488);

        func_0202437c(wordSet, wordIndex, buf, 2, 1, 2);
        GFL_StrBufFree(buf);
        GFL_MsgDataFree(msgData);
        break;
    }
    }
}

// The tier of a new person, rolled with the weights of the avenue's rank
static u32 ResortSys_RollTier(ResortPersonData *data, ResortSys *sys) {
    ResortBinary *ranks = ResortSys_GetRanks(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    u32 total = 0;
    u32 range = ResortBinary_FindRange(ranks, JoinAvenue_GetParam(info, RESORT_INFO_RANK, NULL));
    u32 roll = func_ov137_021f1990(data, 2, 100);
    s32 tier;

    for (tier = 0; tier < 4; tier++) {
        total += ResortBinary_Get(ranks, range, tier + 2);
        if (total > roll) {
            return tier;
        }
    }
    return 0;
}

// Whether a type of person is available at the avenue's popularity
static BOOL ResortSys_IsTypeAvailable(ResortSys *sys, ResortBinary *table, u16 row) {
    u32 unlock = ResortBinary_Get(table, row, 2);
    u16 rank;
    BOOL available;
    ResortBinary *shops;
    JoinAvenueInfo *info;

    switch (unlock) {
    default:
        return FALSE;
    case 0:
        return TRUE;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        info = ResortSys_GetInfo(sys);
        shops = ResortSys_GetShopOpenings(sys);
        available = FALSE;
        rank = JoinAvenue_GetParam(info, RESORT_INFO_RANK, NULL);
        if (rank >= ResortBinary_Get(shops, unlock - 1, 4)) {
            available = TRUE;
        }
        return available;
    }
}

// A random type of person of the tier, among those available
static u32 ResortSys_PickTypeOfTier(ResortPersonData *data, ResortSys *sys, u32 tier) {
    ResortBinary *types = ResortSys_GetPersonTypes(sys);
    u32 count = 0;
    u32 row;
    u32 pick;

    for (row = 1; row < ResortBinary_GetRowCount(types); row++) {
        u32 rowTier = ResortBinary_Get(types, row, 3);
        BOOL available = ResortSys_IsTypeAvailable(sys, types, row);

        if (tier == rowTier && available) {
            count++;
        }
    }

    pick = func_ov137_021f1990(data, 3, count);
    count = 0;
    for (row = 1; row < ResortBinary_GetRowCount(types); row++) {
        u32 rowTier = ResortBinary_Get(types, row, 3);
        BOOL available = ResortSys_IsTypeAvailable(sys, types, row);

        if (tier == rowTier && available) {
            if (count == pick) {
                return row;
            }
            count++;
        }
    }
    return 1;
}

// The type of person that a new person is made, by the tier it rolls
static u32 ResortSys_ChoosePersonType(ResortPersonData *data, ResortSys *sys, GameData *gameData) {
    u32 result;
    u32 tier = ResortSys_RollTier(data, sys);
    BOOL hasPlayerShop = FALSE;
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    JoinAvenuePerson *player = func_02038a18(ResortSys_GetOccupants(sys));
    u32 playerShopKind = ResortShopData_GetShopParam(
        ResortShopData_GetShop(shopData, (u16)func_02038a20(player, GetGameDataPlayerInfo(gameData))), 0);
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    s32 i;

    for (i = 0; i < RESORT_PERSON_COUNT; i++) {
        JoinAvenuePerson *person = func_02038860(occupants, i);

        if (!JoinAvenuePerson_IsEmpty(person)) {
            u32 shopKind;

            func_02038470(person);
            shopKind = ResortShopData_GetShopParam(ResortShopData_GetPersonShop(ResortSys_GetShopData(sys), person), 0);
            if (shopKind == playerShopKind) {
                hasPlayerShop = TRUE;
                break;
            }
        }
    }

    if (hasPlayerShop) {
        s32 roll = func_ov137_021f1990(data, 4, 100);

        if (roll < sNewShopChances[tier]) {
            return playerShopKind + 1;
        }
    }

    result = ResortSys_PickTypeOfTier(data, sys, tier);
    if (tier != 0) {
        JoinAvenueOccupants *occupants2;
        BOOL unused;
        u16 type = result;
        u32 row;

        ResortSys_GetPersonTypes(sys);
        occupants2 = ResortSys_GetOccupants(sys);
        unused = TRUE;
        for (i = 0; i < RESORT_PERSON_COUNT; i++) {
            JoinAvenuePerson *person = func_02038860(occupants2, i);

            if (!JoinAvenuePerson_IsEmpty(person) && ResortSys_MatchCondition(type, person, sys, &row)) {
                unused = FALSE;
                break;
            }
        }
        if (unused) {
            result = ResortSys_PickTypeOfTier(data, sys, 0);
        }
    }
    return result;
}

void *ResortRecords_CreateWindow(void *msgBGSys, HeapID heapId) {
    return FieldMsgBG_CreateMoneyWin(msgBGSys, NULL, 1, 1, 30, 22);
}

// Draws a page of the avenue's records: the people with the most of one thing, with their rank and number
void ResortRecords_PrintPage(void *window, JoinAvenueOccupants *occupants, void *msgBGSys, WordSet *wordSet,
                         GameData *gameData, u32 page, HeapID heapId) {
    ResortRankEntry entries[RESORT_PERSON_COUNT + 1];
    s32 total;
    s32 count = 0;
    u32 rank = 0;
    u32 position = 0;
    ResortWork *work = func_02017b84(gameData);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    StrBuf *nameBuf;
    MsgData *msgData;
    Font *font;
    StrBuf *templateBuf;
    StrBuf *lineBuf;
    s32 i;
    s32 j;

    for (i = 0; i < RESORT_PERSON_COUNT + 1; i++) {
        entries[i].index = -1;
        entries[i].value = -1;
    }

    for (j = RESORT_PERSON_COUNT - 1; j >= 0; j--) {
        JoinAvenuePerson *person = func_02038860(occupants, j);

        if (!JoinAvenuePerson_IsEmpty(person)) {
            if ((func_02036e4c(person, 1) && page < 4) || (func_02036e4c(person, 2) && page >= 4) ||
                (func_02036e4c(person, 0) && page == 8)) {
                ResortRankEntry *entry = &entries[count];

                entry->index = j;
                if (page < 8) {
                    entry->value = joinAveTextHandler(person, JOIN_AVE_PARAM_RECORD1 + page, NULL);
                } else {
                    entry->value = joinAveTextHandler(person, JOIN_AVE_PARAM_MEDAL_COUNT, NULL);
                }
                if (entry->value != 0) {
                    count++;
                }
            }
        }
    }

    entries[count].index = RESORT_PERSON_COUNT;
    if (page < 8) {
        entries[count].value = ResortWork_Get(work, page);
    } else {
        entries[count].value = MedalBox_GetObtainedCount(SaveControl_GetMedalBox(GameData_GetSaveControl(gameData)), 0);
    }

    for (i = 0; i < count; i++) {
        for (j = count; j > i; j--) {
            ResortRankEntry *upper = &entries[j - 1];
            u32 value = upper->value;
            ResortRankEntry *lower = &entries[j];

            if (value < lower->value) {
                u32 index = upper->index;

                *upper = *lower;
                lower->value = value;
                lower->index = index;
            }
        }
    }

    nameBuf = GFL_StrBufCreate(8, HEAPID_TAIL(heapId));
    templateBuf = GFL_StrBufCreate(128, HEAPID_TAIL(heapId));
    lineBuf = GFL_StrBufCreate(128, HEAPID_TAIL(heapId));
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
    font = func_ov036_0218799c(msgBGSys);
    func_ov036_02187c7c(window);
    GFL_MsgDataLoadStrbuf(msgData, RESORT_MSG_RANK_PAGE + page, lineBuf);
    func_ov036_02187c4c(window, 120 - GFL_FontGetBlockWidth(lineBuf, font, 0) / 2, 0, lineBuf);

    total = count + 1;
    for (i = 0; i < total; i++) {
        ResortRankEntry *entry = &entries[i];
        u32 index = entry->index;
        JoinAvenuePerson *person;
        u32 width;
        u32 y;
        u32 value;
        BOOL shared;

        if (index == 8) {
            person = func_02038a18(occupants);
        } else {
            person = func_02038860(occupants, index);
        }

        GFL_MsgDataLoadStrbuf(msgData, RESORT_MSG_RANK_PAGE + 9, templateBuf);
        WordSetNumber(wordSet, 0, rank + 1, 1, 1, 1);
        GFL_WordSetFormatStrbuf(wordSet, lineBuf, templateBuf);
        width = GFL_FontGetBlockWidth(lineBuf, font, 0);
        y = (i * 2 + 3) * 8;
        func_ov036_02187c4c(window, 40 - width, y, lineBuf);

        GFL_MsgDataLoadStrbuf(msgData, RESORT_MSG_RANK_PAGE + 10, templateBuf);
        if (index == 8) {
            copyVarForText(wordSet, 0, playerInfo);
        } else {
            u32 gender = joinAveTextHandler(person, JOIN_AVE_PARAM_GENDER, NULL);
            u16 name[8];

            joinAveTextHandler(person, JOIN_AVE_PARAM_NAME, name);
            GFL_StrBufLoadString(nameBuf, name);
            func_0202437c(wordSet, 0, nameBuf, gender, 1, 2);
        }
        GFL_WordSetFormatStrbuf(wordSet, lineBuf, templateBuf);
        GFL_FontGetBlockWidth(lineBuf, font, 0);
        func_ov036_02187c4c(window, 56, y, lineBuf);

        GFL_MsgDataLoadStrbuf(msgData, RESORT_MSG_RANK_PAGE + RESORT_WORK_MET_FIRST, lineBuf);
        func_ov036_02187c4c(window, 128, y, lineBuf);

        GFL_MsgDataLoadStrbuf(msgData, RESORT_MSG_RANK_PAGE + 12, templateBuf);
        value = entry->value;
        WordSetNumber(wordSet, 0, value, 9, 1, 1);
        GFL_WordSetFormatStrbuf(wordSet, lineBuf, templateBuf);
        func_ov036_02187c4c(window, 216 - GFL_FontGetBlockWidth(lineBuf, font, 0), y, lineBuf);

        shared = TRUE;
        if (i + 1 < total && value == entries[i + 1].value) {
            shared = FALSE;
        }
        position++;
        if (shared) {
            rank = position;
        }
    }

    GFL_MsgDataFree(msgData);
    GFL_StrBufFree(lineBuf);
    GFL_StrBufFree(templateBuf);
    GFL_StrBufFree(nameBuf);
}

// The rank of the avenue's popularity that a tier gets
u32 ResortSys_GetUnlock(ResortSys *sys, GameData *gameData) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    u32 i;

    for (i = 0; i < RESORT_SHOP_KIND_COUNT; i++) {
        const ResortUnlock *unlock = &sUnlocks[i];

        if (unlock->check(sys, gameData) && JoinAvenue_GetParam(info, unlock->infoParamA, NULL) != 0) {
            return sUnlocks[i].id;
        }
    }
    return 0;
}

// The row of the table whose column 9 is an unlock
static u32 ResortSys_FindUnlockRow(ResortBinary *table, u32 id) {
    u32 row;

    for (row = 0; row < ResortBinary_GetRowCount(table); row++) {
        if (id == ResortBinary_Get(table, row, 9)) {
            return row;
        }
    }
    return 0;
}

u32 ResortSys_GetUnlockForRank(ResortSys *sys, GameData *gameData, u32 rank) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    ResortBinary *ranks = ResortSys_GetRanks(sys);
    BOOL rankReached;
    BOOL blocked;
    BOOL available;
    const ResortUnlock *unlock;
    u32 i;

    for (i = 0; i < RESORT_SHOP_KIND_COUNT; i++) {
        rankReached = FALSE;
        unlock = &sUnlocks[i];
        blocked = FALSE;
        available = FALSE;
        if (rank >= ResortBinary_Get(ranks, ResortSys_FindUnlockRow(ranks, unlock->id), 0)) {
            rankReached = TRUE;
        }
        if (JoinAvenue_GetParam(info, unlock->infoParamB, NULL) != 0) {
            blocked = FALSE;
            if (unlock->flag == 0) {
                blocked = TRUE;
            }
        }
        if (JoinAvenue_GetParam(info, unlock->infoParamA, NULL) != 0) {
            blocked = TRUE;
        }
        if (unlock->check(sys, gameData)) {
            available = TRUE;
        }
        if (rankReached && available && !blocked) {
            return sUnlocks[i].id;
        }
    }
    return 0;
}

// The rank of a person among the occupants for a record, or 8 if the person has none
static u32 ResortSys_GetRecordRank(JoinAvenueOccupants *occupants, JoinAvenuePerson *person, u32 page) {
    ResortRankSelf entries[RESORT_PERSON_COUNT];
    s32 count;
    u32 position;
    u32 result;
    s32 i;
    s32 j;

    count = 0;
    result = 0;
    position = 0;
    for (i = 0; i < RESORT_PERSON_COUNT; i++) {
        entries[i].index = -1;
        entries[i].value = -1;
        entries[i].isSelf = FALSE;
    }

    for (i = 0; i < RESORT_PERSON_COUNT; i++) {
        JoinAvenuePerson *other = func_02038860(occupants, i);

        if (!JoinAvenuePerson_IsEmpty(other)) {
            if ((func_02036e4c(other, 1) && page < 4) || (func_02036e4c(other, 2) && page >= 4 && page < 8) ||
                (func_02036e4c(other, 0) && page == 8)) {
                ResortRankSelf *entry = &entries[count];

                entry->index = i;
                if (page == 8) {
                    entry->value = joinAveTextHandler(other, JOIN_AVE_PARAM_MEDAL_COUNT, NULL);
                } else {
                    entry->value = joinAveTextHandler(other, JOIN_AVE_PARAM_RECORD1 + page, NULL);
                }
                if (person == other) {
                    entry->isSelf = TRUE;
                }
                count++;
            }
        }
    }

    for (i = 0; i < count - 1; i++) {
        for (j = count - 1; j > i; j--) {
            s32 prev = j - 1;

            if (entries[prev].value < entries[j].value) {
                u32 value = entries[prev].value;
                u32 index = entries[prev].index;
#ifdef BUGFIX
                BOOL isSelf = entries[prev].isSelf;

                entries[prev] = entries[j];
                entries[j].value = value;
                entries[j].index = index;
                entries[j].isSelf = isSelf;
#else
                // BUG: only the value and the index are put back, so the person asked about loses isSelf when
                // moved down and the function returns 8. Its callers only test for 0, which it isn't then anyway
                entries[prev] = entries[j];
                entries[j].value = value;
                entries[j].index = index;
#endif
            }
        }
    }

    for (i = 0; i < count; i++) {
        BOOL shared;

        if (entries[i].isSelf) {
            if (entries[i].value == 0) {
                result = 8;
                return result;
            }
            return result;
        }

        shared = TRUE;
        if (i + 1 < count && entries[i].value == entries[i + 1].value) {
            shared = FALSE;
        }
        position++;
        if (shared) {
            result = position;
        }
    }
    return 8;
}

// The column of the texts for a person's trainer type
u32 ResortSys_GetTrainerTypeText(ResortBinary *texts, ResortPersonData *data, u32 column) {
    u32 trainerType = func_ov137_021f1968(data, JOIN_AVE_PARAM_TRAINER_TYPE, NULL);
    u32 row;

    for (row = 0; row < ResortBinary_GetRowCount(texts); row++) {
        if (trainerType == ResortBinary_Get(texts, row, 2)) {
            return ResortBinary_Get(texts, row, column);
        }
    }
    return 0;
}

// Hides the people and NPCs who would share the screen with the player's; mode 2 only hides those that are behind
// the camera
void ResortSys_HideOverlappingActors(ResortPeople *people, ResortNPC *npc, Field *field, ResortPerson *person, u32 mode) {
    FieldPlayer *player = Field_GetPlayer(field);
    FieldCamera *camera = Field_GetCameraSystem(field);
    BOOL cameraOnly = FALSE;
    u32 i;

    if (mode == 2) {
        cameraOnly = TRUE;
        mode = 1;
    }

    for (i = 0; i < func_ov137_021f134c(people); i++) {
        ResortPerson *other = func_ov137_021f1634(people, i);

        if (other != NULL) {
            u32 type = func_ov137_021f10f4(other, 0);
            const u32 *hidden = sHiddenTypes[mode];
            s32 j;

            for (j = 0; j < 4 && hidden[j] != 4; j++) {
                if (other != person && type == hidden[j]) {
                    BOOL hide = TRUE;

                    if (cameraOnly) {
                        if (ResortActor_IsBehindCamera(func_ov137_021f110c(other), camera)) {
                            hide = FALSE;
                        }
                    }
                    if (hide) {
                        func_ov137_021f1098(other, 1);
                    }
                    break;
                }
            }
        }
    }

    if (cameraOnly) {
        FieldActor *actor = FieldPlayer_GetActor(player);

        if (!ResortActor_IsBehindCamera(actor, camera)) {
            SetActorHidden(actor, TRUE);
        }
    }

    if (cameraOnly) {
        for (i = 0; i < func_ov137_021f1d00(npc); i++) {
            FieldActor *actor = func_ov137_021f1cc8(npc, i);

            if (actor != NULL && !ResortActor_IsBehindCamera(actor, camera)) {
                SetActorHidden(actor, TRUE);
            }
        }
    } else {
        func_ov137_021f1d04(npc, FALSE);
    }
}

// Whether an actor is behind the camera's eye
static BOOL ResortActor_IsBehindCamera(FieldActor *actor, FieldCamera *camera) {
    VecFx32 pos;
    VecFx32 eye;

    FieldCamera_CoordsGetEye(camera, &eye);
    CopyActorWPos(actor, &pos);
    if (pos.z < eye.z - 128 * FX32_ONE) {
        return TRUE;
    }
    return FALSE;
}

static BOOL ResortUnlock_Always(ResortSys *sys, GameData *gameData) {
    return TRUE;
}

static BOOL ResortUnlock_FlagSet(ResortSys *sys, GameData *gameData) {
    return EventWork_FlagGet(GameData_GetEventWork(gameData), EVENT_FLAG_0x0960);
}

static BOOL ResortUnlock_AllPeople(ResortSys *sys, GameData *gameData) {
    return func_02038868(ResortSys_GetOccupants(sys)) == 8;
}

ResortSys *ResortSys_Get(Field *field) {
    if (Field_CheckGimmickWorkPassword(field, 1)) {
        return func_ov137_021eeec8(field);
    }
    return func_ov137_021f0e80(field);
}

ResortPerson *ResortSys_GetScriptPerson(FieldScriptEnv *env, ResortPeople *people) {
    return func_ov137_021f15cc(people, ScriptWork_GetParentActor(FieldScriptEnv_GetScriptWork(env)));
}

// Makes the person of a kind of data of the avenue from another, or gives the data a new shop of its kind. Returns the
// index of the data made, or 0
u32 ResortSys_MakeData(ResortSys *sys, ResortPersonData *data, u32 kind) {
    ResortSlots *slots = ResortSys_GetSlots(sys);
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    ResortPersonData *made = NULL;
    u32 row = func_020395a0((u16)kind);
    u32 dataKind = func_ov137_021f1984(data);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    u16 slot;

    if (dataKind == kind) {
        JoinAvenuePerson *person = func_ov137_021f1980(data);
        u16 oldShopId = func_020363e0(func_02038470(person), 0);
        const u16 *shop = ResortShopData_GetShop(shopData, oldShopId);
        u16 column2 = ResortShopData_GetShopParam(shop, 2);
        u16 column1 = ResortShopData_GetShopParam(shop, 1);
        u16 shopId = joinAveTextHandler(person, JOIN_AVE_PARAM_SHOP_ID, NULL);
        u16 trainerId = joinAveTextHandler(person, JOIN_AVE_PARAM_TRAINER_ID, NULL);
        u16 version = joinAveTextHandler(person, JOIN_AVE_PARAM_VERSION, NULL);
        u16 unk18 = joinAveTextHandler(person, JOIN_AVE_PARAM_UNK_18, NULL);
        u16 newShop;
        u32 status;

        newShop = func_020394b0(shopId, trainerId, version, unk18, info, shopData);
        newShop = ResortShopData_FindShop(
            shopData, ResortShopData_GetShopParam(ResortShopData_GetShop(shopData, newShop), 0), column2, column1);
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_SHOP_ID, newShop);
        slot = joinAveTextHandler(person, JOIN_AVE_PARAM_SLOT, NULL);
        status = joinAveTextHandler(person, JOIN_AVE_PARAM_UNK_18, NULL);
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_UNK_18, 1);
        func_02038274(person, person, shopData, slot, row);
        JoinAvenuePerson_SetParam(person, JOIN_AVE_PARAM_UNK_18, status);
        return func_ov137_021f198c(data);
    }

    if (ResortSys_GetEmptyData(sys, func_ov137_021f17fc(kind), row, &slot, &made)) {
        switch (sMakeKinds[kind][dataKind]) {
        case 1:
            func_02038274(func_ov137_021f1980(made), func_ov137_021f1980(data), shopData, slot, row);
            break;
        case 2:
            func_020382d8(func_ov137_021f1980(made), func_ov137_021f1980(data), shopData, slot, row);
            break;
        case 3: {
            JoinAvenueRecord *record = func_ov137_021f1980(made);
            u16 shopKind = func_ov137_021f198c(made);

            func_020386f4(record, func_ov137_021f1980(data), slot, (u16)row, shopKind);
            break;
        }
        case 4: {
            JoinAvenueRecord *record = func_ov137_021f1980(made);
            u16 shopKind = func_ov137_021f198c(made);

            func_02038778(record, func_ov137_021f1980(data), slot, (u16)row, shopKind);
            break;
        }
        default:
            return 0;
        }

        func_ov137_021f1748(slots);
        return func_ov137_021f198c(made);
    }
    return 0;
}

// The first empty data of the kind, and the slot it would be placed in; both are returned when given
static BOOL ResortSys_GetEmptyData(ResortSys *sys, u32 kind, u32 row, u16 *slot, ResortPersonData **data) {
    ResortSlots *slots = ResortSys_GetSlots(sys);
    ResortPersonData **datas = ResortSys_GetDatas(sys);
    u16 emptySlot = func_ov137_021f184c(slots, row);
    ResortPersonData *empty = func_ov137_021f1bd0(datas, kind);

    if (emptySlot == 0xffff) {
        return FALSE;
    }
    if (empty == NULL) {
        return FALSE;
    }
    if (slot != NULL) {
        *slot = emptySlot;
    }
    if (data != NULL) {
        *data = empty;
    }
    return TRUE;
}

// The text of a shop for a person, with the person's name in it
StrBuf *ResortSys_FormatShopText(ResortShopData *shops, ResortPersonData *data, u16 shopId, HeapID heapId) {
    const u16 *shop = ResortShopData_GetShop(shops, shopId);
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(HEAPID_TAIL(heapId));
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
    StrBuf *src = GFL_MsgDataLoadStrbufNew(msgData, ResortShopData_GetShopParam(shop, 4));
    StrBuf *nameBuf = GFL_StrBufCreate(8, HEAPID_TAIL(heapId));
    StrBuf *dest = GFL_StrBufCreate(64, heapId);
    u32 gender = func_ov137_021f1968(data, JOIN_AVE_PARAM_GENDER, NULL);
    u16 name[8];

    func_ov137_021f1968(data, JOIN_AVE_PARAM_NAME, name);
    GFL_StrBufLoadString(nameBuf, name);
    func_0202437c(wordSet, 0, nameBuf, gender, 1, 2);
    GFL_WordSetFormatStrbuf(wordSet, dest, src);
    GFL_StrBufFree(nameBuf);
    GFL_StrBufFree(src);
    GFL_MsgDataFree(msgData);
    GFL_WordSetSystemFree(wordSet);
    return dest;
}

// Fills an entry from a row of the texts
static void ResortEntry_Fill(JoinAvenueEntry *entry, u16 shopKind, ResortBinary *table, u16 row, HeapID heapId) {
    u32 nameMessage;
    u32 greetingMessage;
    u32 message1;
    u32 message2;
    u32 gender;
    u32 trainerType;
    u32 trainerId;
    u32 shopId;
    u32 unk6;
    MsgData *msgData;
    StrBuf *buf;
    RTCDate date;

    func_02037a70(entry);
    nameMessage = ResortBinary_Get(table, row, 1);
    greetingMessage = ResortBinary_Get(table, row, 7);
    message1 = ResortBinary_Get(table, row, 8);
    message2 = ResortBinary_Get(table, row, 9);
    gender = ResortBinary_Get(table, row, 3);
    trainerType = ResortBinary_Get(table, row, 2);
    trainerId = ResortBinary_Get(table, row, 4);
    shopId = ResortBinary_Get(table, row, 5);
    unk6 = ResortBinary_Get(table, row, 6);

    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
    buf = GFL_MsgDataLoadStrbufNew(msgData, nameMessage);
    func_02037c70(entry, JOIN_AVE_PARAM_NAME, (u32)GFL_StrBufGetStringPtr(buf));
    GFL_StrBufFree(buf);
    buf = GFL_MsgDataLoadStrbufNew(msgData, greetingMessage);
    func_02037c70(entry, JOIN_AVE_PARAM_GREETING, (u32)GFL_StrBufGetStringPtr(buf));
    GFL_StrBufFree(buf);
    buf = GFL_MsgDataLoadStrbufNew(msgData, message1);
    func_02037c70(entry, JOIN_AVE_PARAM_MESSAGE1, (u32)GFL_StrBufGetStringPtr(buf));
    GFL_StrBufFree(buf);
    buf = GFL_MsgDataLoadStrbufNew(msgData, message2);
    func_02037c70(entry, JOIN_AVE_PARAM_MESSAGE2, (u32)GFL_StrBufGetStringPtr(buf));
    GFL_StrBufFree(buf);
    func_02037c70(entry, JOIN_AVE_PARAM_GENDER, gender);
    func_02037c70(entry, JOIN_AVE_PARAM_TRAINER_TYPE, trainerType);
    func_02037c70(entry, JOIN_AVE_PARAM_TRAINER_ID, trainerId);
    func_02037c70(entry, JOIN_AVE_PARAM_SHOP_ID, shopId);
    func_02037c70(entry, JOIN_AVE_PARAM_UNK_6, unk6);
    func_02037c70(entry, JOIN_AVE_PARAM_SHOP_KIND, shopKind);
    GFL_MsgDataFree(msgData);

    RTC_GetCachedDate(&date);
    func_02037c70(entry, JOIN_AVE_PARAM_YEAR, date.year);
    func_02037c70(entry, JOIN_AVE_PARAM_MONTH, date.month);
    func_02037c70(entry, JOIN_AVE_PARAM_DAY, date.day);
    func_02037c70(entry, JOIN_AVE_PARAM_SEED, GFL_RandomLC(0));
    func_02037c70(entry, JOIN_AVE_PARAM_VERSION, GAME_VERSION);
    func_02037c70(entry, JOIN_AVE_PARAM_LANGUAGE, 2);
}

// Makes an entry of a kind of shop, for the kind's row of the texts that the avenue's rank picks
void ResortSys_MakeShopEntry(ResortSys *sys, GameData *gameData, void *entry, u32 shopKind, u32 salt, HeapID heapId) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    ResortBinary *texts = ResortSys_GetTexts(sys);
    u32 pick;
    u32 selected;
    u32 type;
    u32 count;
    u32 i;

    func_02038a18(occupants);
    selected = 0xff;
    type = 0xff;
    for (i = 0; i < 2; i++) {
        if (shopKind == sShopKindTexts[i].key) {
            type = sShopKindTexts[i].value;
            break;
        }
    }

    count = 0;
    for (i = 0; i < ResortBinary_GetRowCount(texts); i++) {
        if (type == ResortBinary_Get(texts, i, 0)) {
            count++;
        }
    }

    pick = func_020393e4(info, salt + 22, count);
    count = 0;
    for (i = 0; i < ResortBinary_GetRowCount(texts); i++) {
        if (type == ResortBinary_Get(texts, i, 0)) {
            if (count == pick) {
                selected = i;
                break;
            }
            count++;
        }
    }

    ResortEntry_Setup(sys, gameData, entry, shopKind, selected, heapId);
}

static void ResortEntry_Setup(ResortSys *sys, GameData *gameData, JoinAvenueEntry *entry, u32 shopKind, u16 row,
                              HeapID heapId) {
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    ResortBinary *texts = ResortSys_GetTexts(sys);
    JoinAvenuePerson *player = func_02038a18(occupants);
    u16 message[64];

    ResortEntry_Fill(entry, shopKind, texts, row, heapId);
    if (shopKind == 4) {
        func_02037c70(entry, JOIN_AVE_PARAM_GREETING,
                      (u32)TrainerGameInfo_GetGreeting(getTrainerGameInfoAddress(GameData_GetSaveControl(gameData))));
        joinAveTextHandler(player, JOIN_AVE_PARAM_MESSAGE1, message);
        func_02037c70(entry, JOIN_AVE_PARAM_MESSAGE1, (u32)message);
        joinAveTextHandler(player, JOIN_AVE_PARAM_MESSAGE2, message);
        func_02037c70(entry, JOIN_AVE_PARAM_MESSAGE2, (u32)message);
    }
}

static void ResortRecord_Setup(ResortSys *sys, JoinAvenueRecord *record, u16 shopKind, u32 type, u16 slot,
                               HeapID heapId) {
    ResortBinary *texts = ResortSys_GetTexts(sys);
    u32 row;
    JoinAvenueEntry *entry;

    for (row = 0; row < ResortBinary_GetRowCount(texts); row++) {
        if (type == ResortBinary_Get(texts, row, 0)) {
            break;
        }
    }

    entry = func_02037a40(HEAPID_TAIL(heapId));
    ResortEntry_Fill(entry, 0, texts, row, heapId);
    func_02038778(record, entry, slot, 2, shopKind);
    func_02037a68(entry);
}

static void ResortEntry_SetupFromType(ResortSys *sys, JoinAvenueEntry *entry, u16 shopKind, u32 type, HeapID heapId) {
    ResortBinary *texts = ResortSys_GetTexts(sys);
    u32 row;

    for (row = 0; row < ResortBinary_GetRowCount(texts); row++) {
        if (type == ResortBinary_Get(texts, row, 0)) {
            break;
        }
    }

    ResortEntry_Fill(entry, shopKind, texts, row, heapId);
}

// Adds an entry (a2 1) or a record (a2 3) of a kind of shop to the avenue
void ResortSys_AddEntryOrRecord(ResortSys *sys, PlayerInfo *playerInfo, u32 mode, u32 shopKind, HeapID heapId) {
    ResortSlots *slots = ResortSys_GetSlots(sys);
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    JoinAvenueEntryList *entries = ResortSys_GetEntries(sys);

    ResortSys_GetTexts(sys);
    switch (mode) {
    case 1: {
        u16 type = 0xff;
        JoinAvenueEntry *entry;
        u32 i;

        for (i = 0; i < 3; i++) {
            if (shopKind == sEntryTypes[i].key) {
                type = sEntryTypes[i].value;
            }
        }
        entry = func_02037a40(HEAPID_TAIL(heapId));
        ResortEntry_SetupFromType(sys, entry, type, shopKind, heapId);
        if (shopKind == 0) {
            func_02037c70(entry, JOIN_AVE_PARAM_SHOP_ID, func_02038a20(func_02038a18(occupants), playerInfo));
        }
        func_02037e7c(entries, entry);
        func_02037a68(entry);
        break;
    }
    case 3: {
        u16 type = 0xff;
        JoinAvenueRecord *record;
        u32 i;

        for (i = 0; i < 4; i++) {
            if (shopKind == sRecordTypes[i].key) {
                type = sRecordTypes[i].value;
            }
        }
        record = func_020384a4(HEAPID_TAIL(heapId));
        ResortRecord_Setup(sys, record, type, shopKind, func_ov137_021f184c(slots, 2), heapId);
        sys_memcpy(record, func_020388c0(occupants), sizeof(JoinAvenueRecord));
        func_ov137_021f1748(slots);
        func_020384cc(record);
        break;
    }
    }
}

// Picks a person from the table, to let in
void ResortSys_ChooseVisitGroup(ResortSys *sys, ResortPersonData *a1, ResortPersonData *data) {
    ResortWork *work;
    u32 selected;
    u32 roll;
    u32 total;
    u32 i;
    ResortBinary *table;
    u16 index;

    ResortSys_GetInfo(sys);
    work = ResortSys_GetWork(sys);
    selected = 0xffff;
    roll = func_ov137_021f199c(a1, data, 25, 100);
    total = 0;
    table = ResortBinary_Load(12, 8, HEAPID_GAMEEVENT);
    for (i = 0; i < ResortBinary_GetRowCount(table); i++) {
        total += ResortBinary_Get(table, i, 0);
        if (roll < total) {
            selected = i;
            break;
        }
    }
    if (selected > ResortBinary_GetRowCount(table)) {
        selected = 0;
    }
    ResortBinary_Free(table);
    ResortSys_ResetWork(sys, selected);
    index = func_ov137_021f198c(a1);
    ResortWork_Set(work, index + RESORT_WORK_MET_FIRST, 1);
}

static void ResortSys_ResetWork(ResortSys *sys, u32 value) {
    ResortWork *work;
    s32 i;

    ResortSys_GetInfo(sys);
    work = ResortSys_GetWork(sys);
    ResortWork_Set(work, RESORT_WORK_GROUP, value);
    ResortWork_Set(work, RESORT_WORK_VISITS, 0);
    for (i = RESORT_WORK_MET_FIRST; i <= RESORT_WORK_MET_LAST; i++) {
        ResortWork_Set(work, i, 0);
    }
}

// Picks which of the people not yet met has something to say, among the people near the one asked about and then the
// others
BOOL ResortSys_PickPersonToMeet(ResortSys *sys, ResortPersonData *a1, ResortPersonData *data, u32 *out) {
    ResortWork *work = ResortSys_GetWork(sys);
    u16 index;
    s32 count;
    s32 i;
    u32 pick;

    ResortSys_GetInfo(sys);
    index = func_ov137_021f198c(a1);

    count = 0;
    for (i = 0; sPeopleRows[index].near[i] != -1; i++) {
        u8 id = sPeopleRows[index].near[i];

        if (ResortWork_Get(work, id + RESORT_WORK_MET_FIRST) == 0) {
            count++;
        }
    }
    if (count > 0) {
        u32 near;

        pick = func_ov137_021f199c(a1, data, 23, count);
        near = 0;

        for (i = 0; sPeopleRows[index].near[i] != -1; i++) {
            u8 id = sPeopleRows[index].near[i];

            if (ResortWork_Get(work, id + RESORT_WORK_MET_FIRST) == 0) {
                if (near == pick) {
                    *out = id;
                    return TRUE;
                }
                near++;
            }
        }
    }

    count = 0;
    for (i = 0; sPeopleRows[index].far[i] != -1; i++) {
        u8 id = sPeopleRows[index].far[i];

        if (ResortWork_Get(work, id + RESORT_WORK_MET_FIRST) == 0) {
            count++;
        }
    }
    if (count > 0) {
        u32 near;

        pick = func_ov137_021f199c(a1, data, 24, count);
        near = 0;

        for (i = 0; sPeopleRows[index].far[i] != -1; i++) {
            u8 id = sPeopleRows[index].far[i];

            if (ResortWork_Get(work, id + RESORT_WORK_MET_FIRST) == 0) {
                if (near == pick) {
                    *out = id;
                    return TRUE;
                }
                near++;
            }
        }
    }
    return FALSE;
}

// The points that a visit of a person brings: those of their trainer type and shop kind for the day of the week
u32 ResortSys_GetVisitPoints(ResortSys *sys, ResortPersonData *data) {
    ResortShopData *shopData = ResortSys_GetShopData(sys);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    u32 base;
    RTCDate date;
    u16 shopId;
    u32 shopKind;
    u8 trainerType;
    ResortBinary *table;
    u32 typePoints;
    u32 shopPoints;

    ResortSys_GetWork(sys);
    base = JoinAvenue_GetParam(info, 5, NULL);
    RTC_GetCachedDate(&date);
    shopId = func_020363e0(func_02038470(func_ov137_021f1980(data)), 0);
    shopKind = ResortShopData_GetShopParam(ResortShopData_GetShop(shopData, shopId), 0);
    trainerType = UnionView_FindTrainerType((u8)func_ov137_021f1968(data, JOIN_AVE_PARAM_TRAINER_TYPE, NULL));
    if (trainerType >= 8) {
        trainerType -= 8;
    }

    table = ResortBinary_Load(10, 7, HEAPID_GAMEEVENT);
    typePoints = ResortBinary_Get(table, trainerType, date.week);
    ResortBinary_Free(table);
    table = ResortBinary_Load(11, 7, HEAPID_GAMEEVENT);
    shopPoints = ResortBinary_Get(table, base + shopKind * 4, date.week);
    ResortBinary_Free(table);
    return typePoints + shopPoints;
}

// The points of a visit, plus those of the avenue's popularity, from 0 to 100
u32 ResortSys_GetVisitScore(ResortSys *sys, ResortPersonData *data) {
    ResortWork *work = ResortSys_GetWork(sys);
    s16 visit = ResortSys_GetVisitPoints(sys, data);
    u32 day = ResortWork_Get(work, RESORT_WORK_VISITS);
    u32 group = ResortWork_Get(work, RESORT_WORK_GROUP);
    ResortBinary *table = ResortBinary_Load(12, 8, HEAPID_GAMEEVENT);
    s16 bonus = ResortBinary_Get(table, group, day + 1);
    s32 points;

    ResortBinary_Free(table);
    points = visit + bonus;
    if (points > 100) {
        return 100;
    }
    if (points < 0) {
        return 0;
    }
    return points;
}

void ResortSys_MarkMet(ResortSys *sys, ResortPersonData *a1, u32 a2) {
    ResortWork *work = ResortSys_GetWork(sys);

    ResortWork_Set(work, a2 + RESORT_WORK_MET_FIRST, 1);
    ResortWork_Set(work, RESORT_WORK_VISITS, ResortWork_Get(work, RESORT_WORK_VISITS) + 1);
}

BOOL ResortSys_HasShopKind(ResortSys *sys, GameData *gameData, u32 a2) {
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    s32 trainerIdMod;
    u32 rank;
    u32 i;

    if (a2 == 5) {
        return JoinAvenue_GetParam(info, 13, NULL);
    }

    trainerIdMod = getTrainerID(GetGameDataPlayerInfo(gameData)) % 7;
    rank = JoinAvenue_GetParam(info, RESORT_INFO_RANK, NULL);
    if (rank >= 13) {
        return 1;
    }
    for (i = 0; i <= rank; i++) {
        if (a2 == sUnk021f56fc[trainerIdMod][sUnk021f56bc[i]]) {
            return 1;
        }
    }
    return 0;
}

// Whether all eight of the avenue's people are there
BOOL ResortSys_IsFull(ResortSys *sys) {
    return func_02038868(ResortSys_GetOccupants(sys)) == 8;
}
