#include "types.h"
#include "constants/flags.h"
#include "constants/pokemon.h"
#include "field/field_actor.h"
#include "field/field_script.h"
#include "field/scrcmd_pokemon_center.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/bsubway_save.h"
#include "save/event_work.h"
#include "save/medal_box.h"
#include "save/pokedex.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "save/wifi_list.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

// The script plugin of the Pokémon Centers (plugin 13), commands from 1000: Mr. Medal's Medal Rally, and records of
// link battles

// The medals' definitions, one file of MEDAL_COUNT MedalDef
#define ARCID_MEDAL_DEF 235
// The Pokémon whose types the type medals count, one file of MedalTypeSpecies
#define ARCID_MEDAL_TYPE_SPECIES 239
#define MEDAL_TYPE_SPECIES_COUNT 301

#define TYPE_COUNT 17
#define MEDAL_RANK_COUNT 6
#define MEDAL_CATEGORY_COUNT 5

// A MedalDef's check from this is an index into sMedalChecks, and below it a record that must reach the threshold
#define MEDAL_CHECK_FIRST 10000

// The actions of PokemonCenterCmd_Medal
enum {
    MEDAL_ACTION_CHECK = 1000,
    MEDAL_ACTION_ACKNOWLEDGE,
    MEDAL_ACTION_DISCOVER_INITIAL,
    MEDAL_ACTION_RAISE_RANK,
};

typedef struct {
    u32 threshold;
    int check;
    u8 unk8;
    u8 category;
    // The medals of kinds 1 and 2 count toward their rank, and those of kind 2 are discovered as the ranks fill
    u8 kind;
    u8 rank;
} MedalDef;

typedef struct {
    u16 species;
    u8 type1;
    u8 type2;
    u8 counted;
    u8 unk5[3];
} MedalTypeSpecies;

// What the checks of one Medal Rally check have worked out, each once
typedef struct {
    BOOL typesDone;
    // TRUE for a type while every counted Pokémon of it that has been checked is caught
    BOOL typesCaught[TYPE_COUNT];
    BOOL obtainedCountsDone;
    BOOL partyDone;
    BOOL boxesDone;
    BOOL boxCountDone;
    BOOL dayCareDone;
    // Of the Pokémon checked
    BOOL anyPokerus;
    BOOL anyNPokemon;
    BOOL anyRare;
    // The medals obtained in each category
    u32 obtainedCounts[MEDAL_CATEGORY_COUNT];
    u32 boxCount;
} MedalCheckContext;

typedef BOOL (*MedalCheck)(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId);

u32 func_ov012_021682a0(MMSys *mmSys);

static void PokemonCenter_CheckMedals(GameData *gameData, SaveControl *p_sv, HeapID heapId);
static void PokemonCenter_AcknowledgeMedal(FieldScriptEnv *env);
static void PokemonCenter_DiscoverInitialMedals(FieldScriptEnv *env);
static void PokemonCenter_CheckSpecialMedal(u16 medal, u32 check, u32 threshold, GameData *gameData, MedalBox *box,
                                            MedalCheckContext *ctx, HeapID heapId);
static BOOL func_ov065_021e5c40(BOOL *typesCaught, u8 type1, u8 type2);
static void func_ov065_021e5d1c(BoxPkm *pkm, MedalCheckContext *ctx);
static void func_ov065_021e5e20(MedalCheckContext *ctx, PartyPkm *pkm);
static u32 func_ov065_021e64d8(const u32 *flags, EventWork *eventWork, int count);

BOOL PokemonCenterCmd_Medal(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    SaveControl *save = GameData_GetSaveControl(gameData);
    MedalBox *box = SaveControl_GetMedalBox(save);
    HeapID heapId = FieldScriptEnv_GetHeapID(env);

    switch (VM_Read16(vm)) {
    case MEDAL_ACTION_CHECK:
        PokemonCenter_CheckMedals(gameData, save, heapId);
        break;
    case MEDAL_ACTION_ACKNOWLEDGE:
        PokemonCenter_AcknowledgeMedal(env);
        break;
    case MEDAL_ACTION_DISCOVER_INITIAL:
        PokemonCenter_DiscoverInitialMedals(env);
        break;
    case MEDAL_ACTION_RAISE_RANK:
        MedalBox_IncrementRank(box);
        break;
    }
    return FALSE;
}

static void PokemonCenter_DiscoverInitialMedals(FieldScriptEnv *env) {
    u32 i;
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    RTCDate date;

    RTC_GetCachedDate(&date);
    for (i = 0; i < MEDAL_COUNT; i++) {
        if (MedalBox_GetMedalStatus(box, i) == MEDAL_STATUS_1) {
            MedalBox_DiscoverInitialMedal(box, i, date.year, date.month, date.day);
        }
    }
}

// Returns the first medal earned but not yet handed over, or -1
static s32 PokemonCenter_FindEarnedMedal(FieldScriptEnv *env) {
    u32 i;
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));

    for (i = 0; i < MEDAL_COUNT; i++) {
        if (MedalBox_GetMedalStatus(box, i) == MEDAL_STATUS_EARNED) {
            return i;
        }
    }
    return -1;
}

BOOL PokemonCenterCmd_FindEarnedMedal(VM *vm, FieldScriptEnv *env) {
    u16 *found = ScriptReadVar(vm, env);
    u16 *medal = ScriptReadVar(vm, env);
    s32 index = PokemonCenter_FindEarnedMedal(env);

    if (index < 0) {
        *found = FALSE;
        *medal = 0;
    } else {
        *found = TRUE;
        *medal = index;
    }
    return FALSE;
}

static void PokemonCenter_AcknowledgeMedal(FieldScriptEnv *env) {
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    RTCDate date;

    RTC_GetCachedDate(&date);
    MedalBox_AcknowledgeMedal(box, PokemonCenter_FindEarnedMedal(env), date.year, date.month, date.day);
    GameBeaconSys_SetMedalCount(MedalBox_GetObtainedCount(box, 0));
}

static const struct {
    u16 medal;
    u16 flag;
} sMedalsByFlag[] = {
    { 0x60, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { 0x5b, EVENT_FLAG_ARRIVED_NACRENE_CITY },
    { 0x79, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { 0x7a, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { 0x7b, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { 0x75, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { 0x81, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { 0x84, EVENT_FLAG_ARRIVED_CASTELIA_CITY },
    { 0xa7, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { 0xa8, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { 0xac, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { 0x92, 0x9a1 },
    { 0x94, 0x9a2 },
    { 0x96, 0x9a1 },
    { 0x96, 0x9a2 },
    { 0xad, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
    { 0x83, EVENT_FLAG_ARRIVED_NIMBASA_CITY },
};

// The save keeps its original name, p_sv, which the assertion's string gives
static void PokemonCenter_CheckMedals(GameData *gameData, SaveControl *p_sv, HeapID heapId) {
    MedalDef defs[MEDAL_COUNT];
    MedalCheckContext ctx;
    u8 earned[MEDAL_RANK_COUNT];
    u8 total[MEDAL_RANK_COUNT];
    ArcTool *handle;
    GameRecords *records;
    EventWork *eventWork = GameData_GetEventWork(gameData);
    u16 i;
    MedalBox *box;
    MedalDef *def;
    int j;
    u8 status;
    int maxRank;

    GFL_ASSERT(p_sv);
    sys_memset(&ctx, 0, sizeof(ctx));
    for (j = 0; j < TYPE_COUNT; j++) {
        ctx.typesCaught[j] = TRUE;
    }
    handle = GFL_ArcSysCreateFileHandle(ARCID_MEDAL_DEF, heapId);
    GFL_ArcToolReadRange(handle, 0, 0, sizeof(defs), defs);
    box = SaveControl_GetMedalBox(p_sv);
    records = getTrainerCardInfoBlkAddress(p_sv);
    for (j = 0; j < MEDAL_RANK_COUNT; j++) {
        earned[j] = 0;
        total[j] = 0;
    }
    for (i = 0; i < MEDAL_COUNT; i++) {
        if (MedalBox_GetMedalStatus(box, i) < MEDAL_STATUS_EARNED) {
            def = &defs[i];
            if (def->check < MEDAL_CHECK_FIRST) {
                if (def->threshold <= RecordGet(records, def->check)) {
                    MedalBox_GiveMedal(box, i);
                }
            } else {
                PokemonCenter_CheckSpecialMedal(i, def->check, def->threshold, gameData, box, &ctx, heapId);
            }
        }
        status = MedalBox_GetMedalStatus(box, i);
        if (defs[i].kind == 1 || defs[i].kind == 2) {
            if (status == MEDAL_STATUS_EARNED || status == MEDAL_STATUS_OBTAINED) {
                earned[defs[i].rank]++;
            }
            total[defs[i].rank]++;
        }
    }
    // Earning half of a rank's medals discovers the medals of kind 2 up to the rank after it
    for (j = MEDAL_RANK_COUNT - 1; j >= 0; j--) {
        if (earned[j] >= total[j] / 2 + total[j] % 2) {
            for (i = 0; i < MEDAL_COUNT; i++) {
                status = MedalBox_GetMedalStatus(box, i);
                if (defs[i].kind == 2 && defs[i].rank <= j + 1) {
                    MedalBox_DiscoverMedal(box, i);
                }
            }
            break;
        }
    }
    // And those up to the highest rank of a medal of kind 2 earned
    maxRank = -1;
    for (i = 0; i < MEDAL_COUNT; i++) {
        status = MedalBox_GetMedalStatus(box, i);
        if (defs[i].kind == 2 && (status == MEDAL_STATUS_EARNED || status == MEDAL_STATUS_OBTAINED) &&
            maxRank < defs[i].rank) {
            maxRank = defs[i].rank;
        }
    }
    if (maxRank >= 0) {
        for (i = 0; i < MEDAL_COUNT; i++) {
            status = MedalBox_GetMedalStatus(box, i);
            if (defs[i].kind == 2 && defs[i].rank <= maxRank) {
                MedalBox_DiscoverMedal(box, i);
            }
        }
    }
    for (j = 0; j < NELEMS(sMedalsByFlag); j++) {
        if (EventWork_FlagGet(eventWork, sMedalsByFlag[j].flag) == TRUE) {
            MedalBox_DiscoverMedal(box, sMedalsByFlag[j].medal);
        }
    }
    GFL_ArcToolFree(handle);
}

static MedalCheck sMedalChecks[];

static void PokemonCenter_CheckSpecialMedal(u16 medal, u32 check, u32 threshold, GameData *gameData, MedalBox *box,
                                            MedalCheckContext *ctx, HeapID heapId) {
    BOOL result = FALSE;
    MedalCheck func = sMedalChecks[check - MEDAL_CHECK_FIRST];

    if (func != NULL) {
        result = func(ctx, gameData, threshold, heapId);
    }
    if (result == TRUE) {
        MedalBox_GiveMedal(box, medal);
    }
}

static BOOL PokemonCenter_CheckTypes(MedalCheckContext *ctx, GameData *gameData, HeapID heapId) {
    MedalTypeSpecies species[MEDAL_TYPE_SPECIES_COUNT];
    u16 i;
    PokeDexSave *pokedex;
    MedalTypeSpecies *entry;

    if (ctx->typesDone == FALSE) {
        pokedex = GameData_GetPokedex(gameData);
        GFL_ArcSysRead(species, ARCID_MEDAL_TYPE_SPECIES, 0);
        for (i = 0; i <= MEDAL_TYPE_SPECIES_COUNT - 1; i++) {
            entry = &species[i];
            if (entry->counted == TRUE && PokeDex_IsCaught(pokedex, entry->species) == FALSE &&
                func_ov065_021e5c40(ctx->typesCaught, entry->type1, entry->type2)) {
                break;
            }
        }
        ctx->typesDone = TRUE;
    }
    return TRUE;
}

// Clears the species' types, and returns TRUE if that cleared the last type left
static BOOL func_ov065_021e5c40(BOOL *typesCaught, u8 type1, u8 type2) {
    BOOL cleared = FALSE;
    int i;

    if (type1 == type2) {
        if (typesCaught[type1] == TRUE) {
            typesCaught[type1] = FALSE;
            cleared = TRUE;
        }
    } else {
        if (typesCaught[type1] == TRUE) {
            typesCaught[type1] = FALSE;
            cleared = TRUE;
        }
        if (typesCaught[type2] == TRUE) {
            typesCaught[type2] = FALSE;
            cleared = TRUE;
        }
    }
    if (cleared) {
        for (i = 0; i < TYPE_COUNT; i++) {
            if (typesCaught[i] == TRUE) {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL PokemonCenter_CheckBoxes(MedalCheckContext *ctx, GameData *gameData, HeapID heapId) {
    SaveControl *save;
    BoxSaveAccessor *boxes;
    BattleBoxSave *battleBox;
    u32 box;
    u32 slot;

    if (ctx->boxesDone == FALSE) {
        save = GameData_GetSaveControl(gameData);
        boxes = GameData_GetBoxSaveAccessor(gameData);
        battleBox = getBattleBox(save);
        for (box = 0; box < 24; box++) {
            for (slot = 0; slot < 30; slot++) {
                if (BoxSaveAccessor_GetPkmParam(boxes, box, slot, PKM_PARAM_SPECIES_VALID, NULL)) {
                    func_ov065_021e5d1c(BoxSaveAccessor_GetPkm(boxes, box, slot), ctx);
                }
            }
        }
        if (func_0200c340(battleBox) == TRUE) {
            for (slot = 0; slot < 6; slot++) {
                func_ov065_021e5d1c(getBoxSlotAddress(battleBox, 0, slot), ctx);
            }
        }
        ctx->boxesDone = TRUE;
    }
    return TRUE;
}

static void func_ov065_021e5d1c(BoxPkm *pkm, MedalCheckContext *ctx) {
    BOOL wasEncrypted;

    if (PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
        wasEncrypted = PML_PkmDecrypt(pkm);
        if (PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL) == FALSE) {
            if (ctx->anyPokerus == FALSE && PML_PkmGetParam(pkm, PKM_PARAM_POKERUS, NULL)) {
                ctx->anyPokerus = TRUE;
            }
            if (ctx->anyNPokemon == FALSE) {
                ctx->anyNPokemon = PML_PkmGetParam(pkm, PKM_PARAM_N_POKEMON, NULL);
            }
            if (ctx->anyRare == FALSE) {
                ctx->anyRare = PML_PkmIsRare(pkm);
            }
        }
        PML_PkmReEncrypt(pkm, wasEncrypted);
    }
}

static BOOL PokemonCenter_CheckParty(MedalCheckContext *ctx, GameData *gameData, HeapID heapId) {
    PokeParty *party = GameData_GetParty(gameData);
    int count = PokeParty_GetPkmCount(party);
    PartyPkm *pkm;
    BOOL wasEncrypted;
    int i;

    if (ctx->partyDone == FALSE) {
        for (i = 0; i < count; i++) {
            pkm = PokeParty_GetPkm(party, i);
            wasEncrypted = PokeParty_DecryptPkm(pkm);
            func_ov065_021e5e20(ctx, pkm);
            PokeParty_EncryptPkm(pkm, wasEncrypted);
        }
        ctx->partyDone = TRUE;
    }
    return TRUE;
}

static BOOL PokemonCenter_CheckDayCare(MedalCheckContext *ctx, GameData *gameData, HeapID heapId) {
    int i;
    DayCareSave *dayCare = getDaycareBlockAddress(GameData_GetSaveControl(gameData));

    if (ctx->dayCareDone == FALSE) {
        for (i = 0; i < 2; i++) {
            if (DayCareSave_GetPkmStatus(dayCare, i)) {
                func_ov065_021e5e20(ctx, DayCareSave_GetPkm(dayCare, i));
            }
        }
        ctx->dayCareDone = TRUE;
    }
    return TRUE;
}

static void func_ov065_021e5e20(MedalCheckContext *ctx, PartyPkm *pkm) {
    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) &&
        PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == FALSE) {
        if (ctx->anyPokerus == FALSE && PokeParty_GetParam(pkm, PKM_PARAM_POKERUS, NULL)) {
            ctx->anyPokerus = TRUE;
        }
        if (ctx->anyNPokemon == FALSE) {
            ctx->anyNPokemon = PokeParty_GetParam(pkm, PKM_PARAM_N_POKEMON, NULL);
        }
        if (ctx->anyRare == FALSE) {
            ctx->anyRare = PokeParty_IsRare(pkm);
        }
    }
}

static BOOL PokemonCenter_IsTypeCaught(u32 type, MedalCheckContext *ctx, GameData *gameData, HeapID heapId) {
    if (PokemonCenter_CheckTypes(ctx, gameData, heapId)) {
        return ctx->typesCaught[type];
    }
    return FALSE;
}

static BOOL PokemonCenter_MeetsThreshold(u32 value, u32 threshold) {
    if (value >= threshold) {
        return TRUE;
    }
    return FALSE;
}

static u32 PokemonCenter_GetMedalCategory(u8 category) {
    const u32 categories[] = {
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4,
    };

    return categories[category];
}

static BOOL PokemonCenter_CountObtainedMedals(MedalCheckContext *ctx, GameData *gameData, HeapID heapId) {
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(gameData));
    MedalDef *defs;
    u32 category;
    u32 i;

    if (ctx->obtainedCountsDone == FALSE) {
        defs = GFL_ArcSysReadHeapNewRange(ARCID_MEDAL_DEF, 0, heapId, 0,
                                          GFL_ArcSysGetDataMax(ARCID_MEDAL_DEF) * sizeof(MedalDef));
        sys_memset(ctx->obtainedCounts, 0, sizeof(ctx->obtainedCounts));
        for (i = 0; i < MEDAL_COUNT; i++) {
            category = PokemonCenter_GetMedalCategory(defs[i].category);
            if (MedalBox_GetMedalStatus(box, i) == MEDAL_STATUS_OBTAINED) {
                ctx->obtainedCounts[category]++;
            }
        }
        GFL_HeapFree(defs);
        ctx->obtainedCountsDone = TRUE;
    }
    return TRUE;
}

static BOOL PokemonCenter_IsType0Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(0, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType9Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(9, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType10Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(10, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType12Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(12, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType11Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(11, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType14Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(14, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType1Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(1, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType3Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(3, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType4Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(4, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType2Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(2, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType13Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(13, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType6Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(6, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType5Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(5, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType7Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(7, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType15Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(15, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType16Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(16, ctx, gameData, heapId);
}

static BOOL PokemonCenter_IsType8Caught(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_IsTypeCaught(8, ctx, gameData, heapId);
}

static BOOL func_ov065_021e6064(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return func_0200ae58(getMusicalInfoBlkAddress(gameData));
}

static BOOL PokemonCenter_CheckBoxCount(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    if (ctx->boxCountDone == FALSE) {
        ctx->boxCount = howManyTotalPokesAreInBoxes(GameData_GetBoxSaveAccessor(gameData));
        ctx->boxCountDone = TRUE;
    }
    return PokemonCenter_MeetsThreshold(ctx->boxCount, threshold);
}

static BOOL func_ov065_021e609c(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_MeetsThreshold(func_0200e370(func_0201795c(gameData), 0), threshold);
}

static BOOL func_ov065_021e60b4(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_MeetsThreshold(func_0200e370(func_0201795c(gameData), 1), threshold);
}

static BOOL func_ov065_021e60cc(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    void *block = func_0201795c(gameData);
    u16 a = func_0200e370(block, 2);
    u16 b = func_0200e370(block, 3);

    if (a > b) {
        return PokemonCenter_MeetsThreshold(a, threshold);
    }
    return PokemonCenter_MeetsThreshold(b, threshold);
}

static BOOL func_ov065_021e6100(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    u32 count = 0;
    int i;
    WifiList *wifiList = GameData_GetWifiList(gameData);

    for (i = 0; i < 32; i++) {
        if (func_0200a138(wifiList, i) && func_02009f80(wifiList, i, 9) != 2) {
            count++;
        }
    }
    return PokemonCenter_MeetsThreshold(count, threshold);
}

static BOOL PokemonCenter_CheckCategory0(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    if (PokemonCenter_CountObtainedMedals(ctx, gameData, heapId)) {
        return PokemonCenter_MeetsThreshold(ctx->obtainedCounts[0], threshold);
    }
    return FALSE;
}

static BOOL PokemonCenter_CheckCategory1(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    if (PokemonCenter_CountObtainedMedals(ctx, gameData, heapId)) {
        return PokemonCenter_MeetsThreshold(ctx->obtainedCounts[1], threshold);
    }
    return FALSE;
}

static BOOL PokemonCenter_CheckCategory2(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    if (PokemonCenter_CountObtainedMedals(ctx, gameData, heapId)) {
        return PokemonCenter_MeetsThreshold(ctx->obtainedCounts[2], threshold);
    }
    return FALSE;
}

static BOOL PokemonCenter_CheckCategory3(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    if (PokemonCenter_CountObtainedMedals(ctx, gameData, heapId)) {
        return PokemonCenter_MeetsThreshold(ctx->obtainedCounts[3], threshold);
    }
    return FALSE;
}

static BOOL PokemonCenter_HasNPokemon(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    BOOL result = FALSE;

    if (PokemonCenter_CheckBoxes(ctx, gameData, heapId) && PokemonCenter_CheckParty(ctx, gameData, heapId) &&
        PokemonCenter_CheckDayCare(ctx, gameData, heapId)) {
        result = ctx->anyNPokemon;
    }
    return result;
}

static BOOL PokemonCenter_HasRare(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    BOOL result = FALSE;

    if (PokemonCenter_CheckBoxes(ctx, gameData, heapId) && PokemonCenter_CheckParty(ctx, gameData, heapId) &&
        PokemonCenter_CheckDayCare(ctx, gameData, heapId)) {
        result = ctx->anyRare;
    }
    return result;
}

static BOOL PokemonCenter_HasPokerus(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    BOOL result = FALSE;

    if (PokemonCenter_CheckBoxes(ctx, gameData, heapId) && PokemonCenter_CheckParty(ctx, gameData, heapId) &&
        PokemonCenter_CheckDayCare(ctx, gameData, heapId)) {
        result = ctx->anyPokerus;
    }
    return result;
}

static BOOL func_ov065_021e6268(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_MeetsThreshold(func_ov012_021682a0(GameData_GetMMSys(gameData)), threshold);
}

static BOOL func_ov065_021e6280(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    return PokemonCenter_MeetsThreshold(func_0200c924(getTrainerCardData_wrapper(GameData_GetSaveControl(gameData))),
                                        threshold);
}

static BOOL func_ov065_021e629c(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    u32 sum = 0;
    GameRecords *records = getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData));

    sum += RecordGet(records, 0xd);
    sum += RecordGet(records, 0x11);
    sum += RecordGet(records, 0x25);
    return PokemonCenter_MeetsThreshold(sum, threshold);
}

static BOOL func_ov065_021e62d4(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    u32 sum = 0;
    GameRecords *records = getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData));

    sum += RecordGet(records, 0xc);
    sum += RecordGet(records, 0x10);
    sum += RecordGet(records, 0x24);
    return PokemonCenter_MeetsThreshold(sum, threshold);
}

// These checks count flags, and the ones that ignore the threshold need them all
static BOOL func_ov065_021e630c(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    const u32 flags[] = {
        0xa50, 0xa51, 0xa52, 0xa53, 0xa54, 0xa55, 0xa56, 0xa57, 0xa58, 0xa59,
        0xa5a, 0xa5b, 0xa5c, 0xa5d, 0xa5e, 0xa5f, 0xa60, 0xa61, 0xa62, 0xa63,
    };

    return PokemonCenter_MeetsThreshold(func_ov065_021e64d8(flags, GameData_GetEventWork(gameData), NELEMS(flags)),
                                        NELEMS(flags));
}

static BOOL func_ov065_021e6340(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    const u32 flags[] = {
        0xa64, 0xa65, 0xa66, 0xa67, 0xa68, 0xa69, 0xa6a, 0xa6b, 0xa6c,
        0xa6d, 0xa6e, 0xa6f, 0xa70, 0xa71, 0xa72, 0xa73, 0xa74, 0xa75,
    };

    return PokemonCenter_MeetsThreshold(func_ov065_021e64d8(flags, GameData_GetEventWork(gameData), NELEMS(flags)),
                                        threshold);
}

static BOOL func_ov065_021e6374(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    const u32 flags[] = { 0xa76, 0xa77, 0xa78, 0xa79, 0xa7a, 0xa7b, 0xa7c, 0xa7d, 0xa7e, 0xa7f };

    return PokemonCenter_MeetsThreshold(func_ov065_021e64d8(flags, GameData_GetEventWork(gameData), NELEMS(flags)),
                                        NELEMS(flags));
}

static BOOL func_ov065_021e63a8(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    const u32 flags[] = { 0x9bd, 0x9be, 0x9bf };

    return PokemonCenter_MeetsThreshold(func_ov065_021e64d8(flags, GameData_GetEventWork(gameData), NELEMS(flags)),
                                        NELEMS(flags));
}

static BOOL func_ov065_021e63dc(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    const u32 flags[] = {
        EVENT_FLAG_ARRIVED_ASPERTIA_CITY,   EVENT_FLAG_ARRIVED_FLOCCESY_TOWN,
        EVENT_FLAG_ARRIVED_VIRBANK_CITY,    EVENT_FLAG_ARRIVED_POKESTAR_STUDIOS,
        EVENT_FLAG_ARRIVED_CASTELIA_CITY,   EVENT_FLAG_ARRIVED_UNITY_TOWER,
        EVENT_FLAG_ARRIVED_JOIN_AVENUE,     EVENT_FLAG_ARRIVED_NIMBASA_CITY,
        EVENT_FLAG_ARRIVED_DRIFTVEIL_CITY,  EVENT_FLAG_ARRIVED_PWT,
        EVENT_FLAG_ARRIVED_MISTRALTON_CITY, EVENT_FLAG_ARRIVED_LENTIMAS_TOWN,
        EVENT_FLAG_ARRIVED_UNDELLA_TOWN,    EVENT_FLAG_ARRIVED_LACUNOSA_TOWN,
        EVENT_FLAG_ARRIVED_OPELUCID_CITY,   EVENT_FLAG_ARRIVED_HUMILAU_CITY,
        EVENT_FLAG_ARRIVED_VICTORY_ROAD,    EVENT_FLAG_ARRIVED_POKEMON_LEAGUE,
        EVENT_FLAG_ARRIVED_ICIRRUS_CITY,    EVENT_FLAG_ARRIVED_NACRENE_CITY,
        EVENT_FLAG_ARRIVED_STRIATON_CITY,   EVENT_FLAG_ARRIVED_ACCUMULA_TOWN,
        EVENT_FLAG_ARRIVED_NUVEMA_TOWN,     EVENT_FLAG_ARRIVED_BLACK_CITY_WHITE_FOREST,
    };

    return PokemonCenter_MeetsThreshold(func_ov065_021e64d8(flags, GameData_GetEventWork(gameData), NELEMS(flags)),
                                        NELEMS(flags));
}

static BOOL func_ov065_021e6410(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    const u32 works[] = { 0x4045, 0x4046, 0x4047, 0x4048, 0x4049, 0x404a, 0x404b, 0x404c };
    u32 i;
    u32 count = 0;
    EventWork *eventWork = GameData_GetEventWork(gameData);

    for (i = 0; i < NELEMS(works); i++) {
        if (*EventWork_GetWkPtr(eventWork, works[i]) == 1) {
            count++;
        }
    }
    return PokemonCenter_MeetsThreshold(count, threshold);
}

static BOOL func_ov065_021e6468(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    GameRecords *records = getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData));
    u32 a = RecordGet(records, 0x1a);
    u32 b = RecordGet(records, 0x1b);

    if (a > b) {
        return PokemonCenter_MeetsThreshold(a, threshold);
    }
    return PokemonCenter_MeetsThreshold(b, threshold);
}

static BOOL func_ov065_021e64a0(MedalCheckContext *ctx, GameData *gameData, u32 threshold, HeapID heapId) {
    GameRecords *records = getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData));
    u32 a = RecordGet(records, 0x53);
    u32 b = RecordGet(records, 0x54);

    if (a > b) {
        return PokemonCenter_MeetsThreshold(a, threshold);
    }
    return PokemonCenter_MeetsThreshold(b, threshold);
}

// Returns how many of the flags are set
static u32 func_ov065_021e64d8(const u32 *flags, EventWork *eventWork, int count) {
    int i;
    u32 set = 0;

    for (i = 0; i < count; i++) {
        if (EventWork_FlagGet(eventWork, flags[i]) == TRUE) {
            set++;
        }
    }
    return set;
}

BOOL func_ov065_021e6508(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    RecordSave *record = func_0200f2bc(GameData_GetSaveControl(gameData));
    u16 info = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);

    switch (info) {
    case 0:
        *var = func_0200f300(record);
        break;
    case 1:
        *var = func_0200f308(record);
        break;
    case 2:
        *var = func_0200f334(record);
        break;
    case 3:
        *var = func_02017b8c(gameData);
        func_02017bb4(gameData);
        break;
    case 4:
        *var = func_0200f384(record);
        break;
    }
    return FALSE;
}

BOOL func_ov065_021e658c(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));

    func_0200f2dc(getRecordBlkAddress(GameData_GetSaveControl(gameData)));
    func_02017bb4(gameData);
    return FALSE;
}

BOOL PokemonCenterCmd_ClearMatchInProgress(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    RecordSave_ClearMatchInProgress(
        getRecordBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)))));
    return FALSE;
}

BOOL func_ov065_021e65dc(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    SaveControl *save = GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    RecordSave *record = getRecordBlkAddress(save);
    u16 action = ScriptReadAny(vm, env);
    u16 value = ScriptReadAny(vm, env);

    switch (action) {
    case 4:
        func_0200f37c(record, value);
        break;
    case 5:
        RecordAddOne(getTrainerCardInfoBlkAddress(save), 0x35);
        break;
    }
    return FALSE;
}

BOOL func_ov065_021e6634(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    RecordSave *record = getRecordBlkAddress(GameData_GetSaveControl(gameData));
    u16 value = ScriptReadAny(vm, env);

    func_0200e318(func_0201795c(gameData), value);
    if (value != 0) {
        RecordAdd(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData)), 0x21, value);
    }
    return FALSE;
}

// The checks of the medals from MEDAL_CHECK_FIRST
static MedalCheck sMedalChecks[] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    func_ov065_021e6340,
    NULL,
    PokemonCenter_IsType0Caught,
    PokemonCenter_IsType9Caught,
    PokemonCenter_IsType10Caught,
    PokemonCenter_IsType12Caught,
    PokemonCenter_IsType11Caught,
    PokemonCenter_IsType14Caught,
    PokemonCenter_IsType1Caught,
    PokemonCenter_IsType3Caught,
    PokemonCenter_IsType4Caught,
    PokemonCenter_IsType2Caught,
    PokemonCenter_IsType13Caught,
    PokemonCenter_IsType6Caught,
    PokemonCenter_IsType5Caught,
    PokemonCenter_IsType7Caught,
    PokemonCenter_IsType15Caught,
    PokemonCenter_IsType16Caught,
    PokemonCenter_IsType8Caught,
    NULL,
    NULL,
    PokemonCenter_CheckBoxCount,
    func_ov065_021e64a0,
    NULL,
    func_ov065_021e6268,
    func_ov065_021e6280,
    func_ov065_021e630c,
    PokemonCenter_HasNPokemon,
    PokemonCenter_HasRare,
    PokemonCenter_HasPokerus,
    NULL,
    func_ov065_021e6374,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    func_ov065_021e63a8,
    func_ov065_021e63dc,
    PokemonCenter_CheckCategory0,
    func_ov065_021e629c,
    func_ov065_021e609c,
    func_ov065_021e60b4,
    func_ov065_021e60cc,
    func_ov065_021e6468,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    PokemonCenter_CheckCategory1,
    func_ov065_021e62d4,
    NULL,
    func_ov065_021e6064,
    func_ov065_021e6100,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    func_ov065_021e6410,
    PokemonCenter_CheckCategory2,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    PokemonCenter_CheckCategory3,
};
