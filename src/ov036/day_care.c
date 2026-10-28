// The day care: leaving and taking back Pokémon, their growth, and the egg that appears and what hatches from it.
// The ROM names it sodateya.c (the day care). Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/vars.h"
#include "field/field.h"
#include "field/field_daycare.h"
#include "field/game_beacon_set.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/item.h"
#include "pml/met_data.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save/bag.h"
#include "save/box.h"
#include "save/chatter.h"
#include "save/event_work.h"
#include "save/player_info.h"
#include "save/pokedex_savedata.h"
#include "system/game_data.h"
#include "system/game_system.h"

// Default parameter tables copied into the local work arrays used by move and IV inheritance.
typedef struct {
    u32 values[4];
} DayCareMoveParamTable;
typedef struct {
    u32 values[6];
} DayCareStatParamTable;
typedef struct {
    u32 values[12][3];
} DayCareIncenseTable;

static const DayCareMoveParamTable sOpenSlotMoveParams;
static const DayCareMoveParamTable sExistingMoveParams;
static const DayCareIncenseTable sIncenses;
static const DayCareStatParamTable sPowerItems;
static const DayCareMoveParamTable sLevelMoveParams;
static const DayCareMoveParamTable sTmMoveParams;
static const DayCareStatParamTable sInheritedIVParams;
static const DayCareStatParamTable sEggIVParams;
static const DayCareMoveParamTable sEggMoveParams;

// The day care's work area
struct DayCare {
    HeapID heapId;
    Field *field;
    DayCareSave *save;
    u16 steps;
};

// The two parents, and the egg's data as it is worked out
typedef struct {
    PartyPkm *parents[2];
} EggParents;

typedef struct {
    u32 species;
    u32 form;
    u32 level;
    u32 nature;
    u32 happiness;
    u32 ivs[6];
    u32 trainerId;
    u32 pid;
    u32 moveCount;
    u32 moves[4];
    u32 abilitySlot;
    u32 natureFromParent1;
    u32 natureFromParent2;
    u32 ivFromParent1[6];
    u32 ivFromParent2[6];
    u32 inheritedIvCount;
} EggData;

typedef struct {
    u64 state;
    u64 multiplier;
    u64 increment;
} EggRandom;

static inline u32 EggRandom_Next(EggRandom *rnd, u32 max) {
    rnd->state = rnd->multiplier * rnd->state + rnd->increment;
    if (max == 0) {
        return rnd->state >> 32;
    }
    return ((rnd->state >> 32) * max) >> 32;
}

static void DayCare_RecoverPkm(PartyPkm *pkm);
static void shayminTransform(PartyPkm *pkm);
static void DayCare_SyncPokedex(DayCare *dayCare, PartyPkm *pkm);
static void setPartyHasSpaceToConst(Field *field);
static void setPartyFullToConst(Field *field);
static u32 DayCare_CalcNewExp(u32 exp, u32 steps);
static void DayCare_CommitPkmGrowth(PartyPkm *pkm, u32 steps);
static void writeLvUpMoveOpenSlot(PartyPkm *pkm, u32 move);
static BOOL DayCare_MonKnowsMove(PartyPkm *pkm, int move);
static void DayCareSave_ShiftPkms(DayCareSave *save);
static u32 DayCare_CalcBaseEggSpawnChance(PartyPkm *pkm1, PartyPkm *pkm2);
static u32 DayCare_CalcBaseEggSpawnChanceCore(PartyPkm *pkm1, PartyPkm *pkm2);
static BOOL DayCare_CheckEggSpawn(DayCare *dayCare);
static void DayCare_CreateChildPkm(DayCare *dayCare, PartyPkm *egg);
static void updateChatotPartyStatus(Field *field, PokeParty *party);
static void breeding(GameSystem *gsys, HeapID heapId, PlayerInfo *info, u16 place, PartyPkm *pkm1, PartyPkm *pkm2,
                     PartyPkm *out, EggRandom *rnd);
static void genderSort(EggParents *parents);
static void setGenderlessFirst(EggParents *parents);
static void swapDaycarePKMPointers(EggParents *parents);
static void clearEggDataBlk(EggData *egg);
static void getChild(EggParents *parents, EggData *egg);
static void nidoranCompute(EggParents *parents, EggData *egg, EggRandom *rnd);
static void volbeatCompute(EggParents *parents, EggData *egg, EggRandom *rnd);
static void manaphyHandler(EggParents *parents, EggData *egg);
static void incenseHandler(EggParents *parents, EggData *egg);
static PartyPkm *checkSecondPokemonDitto(EggParents *parents);
static void zeroFormData(EggParents *parents, EggData *egg);
static void shellosHandler(EggParents *parents, EggData *egg);
static void burmyHandler(EggParents *parents, EggData *egg);
static void basculinHandler(EggParents *parents, EggData *egg);
static PartyPkm *checkSecondPokemonDitto2(EggParents *parents);
static void getNature(EggParents *parents, EggData *egg, EggRandom *rnd);
static void everstoneHandler(EggParents *parents, EggData *egg, EggRandom *rnd);
static void abilDecision(EggParents *parents, EggData *egg, HeapID heapId, EggRandom *rnd);
static void dittoAbilFix(EggParents *parents, EggData *egg, EggRandom *rnd);
static void FetchEggBaseHappiness(EggData *egg, HeapID heapId);
static void SetPowerItemInheritedIVs(EggParents *parents, EggData *egg, EggRandom *rnd);
static void SetRandomInheritedIVs(EggParents *parents, EggData *egg, EggRandom *rnd);
static void makeIVSet(EggParents *parents, EggData *egg, EggRandom *rnd);
static void getEggIDSet(PlayerInfo *info, EggData *egg);
static void rerollEggPID(EggParents *parents, EggData *egg);
static void shinyBreedHandler(EggParents *parents, EggData *egg, BOOL hasShinyCharm);
static void levelMoves(EggData *egg);
static void inheritEggMoves(EggParents *parents, EggData *egg, HeapID heapId);
static void inheritLvlUpMoves(EggParents *parents, EggData *egg);
static void inheritTmMoves(EggParents *parents, EggData *egg, HeapID heapId);
static void pichuLightBallHandler(EggParents *parents, EggData *egg);
static void PushEggPkmMove(EggData *egg, int move);
static void createEggPkm(PartyPkm *pkm, const EggData *egg, PlayerInfo *info, u32 place, HeapID heapId);
static BOOL canPkmLearnTMItemNumber(HeapID heapId, u16 species, u16 form, u16 item);
static u32 validateSpeciesAndGetOffspring(u32 species);

DayCare *DayCare_Create(HeapID heapId, Field *field, DayCareSave *save) {
    DayCare *dayCare = GFL_HeapAllocate(heapId, sizeof(DayCare), FALSE, "sodateya.c", 101);
    dayCare->heapId = heapId;
    dayCare->field = field;
    dayCare->save = save;
    dayCare->steps = 0;
    return dayCare;
}

void DayCare_Free(DayCare *dayCare) {
    GFL_HeapFree(dayCare);
}

void DayCare_AddPkm(DayCare *dayCare, PokeParty *party, u32 slot) {
    int count = DayCareSave_GetPkmCount(dayCare->save);
    PartyPkm *pkm = PokeParty_GetPkm(party, slot);

    if (count < 2) {
        DayCare_RecoverPkm(pkm);
        shayminTransform(pkm);
        DayCareSave_AddPkm(dayCare->save, count, pkm);
        PokeParty_RemovePkm(party, slot);
        updateChatotPartyStatus(dayCare->field, party);
    }
}

static void DayCare_RecoverPkm(PartyPkm *pkm) {
    PokeParty_Recover(pkm);
}

static void shayminTransform(PartyPkm *pkm) {
    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == SPECIES_SHAYMIN) {
        PokeParty_ChangeForme(pkm, 0);
    }
}

void DayCare_RemovePkm(DayCare *dayCare, u32 slot, PokeParty *party) {
    if (DayCareSave_GetPkmStatus(dayCare->save, slot) == TRUE) {
        PartyPkm *pkm = DayCareSave_GetPkm(dayCare->save, slot);
        u32 steps = DayCareSave_GetPkmStepCounter(dayCare->save, slot);

        DayCare_CommitPkmGrowth(pkm, steps);
        DayCare_SyncPokedex(dayCare, pkm);
        PokeParty_AddPkm(party, pkm);
        DayCareSave_DisablePkmStatus(dayCare->save, slot);
        DayCareSave_ShiftPkms(dayCare->save);
    }
}

static void DayCare_SyncPokedex(DayCare *dayCare, PartyPkm *pkm) {
    PokeDexSave *pokedex = GameData_GetPokedex(GSYS_GetGameData(Field_GetGameSystem(dayCare->field)));

    if (!PokeDex_IsCaught(pokedex, PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL))) {
        addPkmToDex(pokedex, pkm);
    }
}

void DayCare_Breed(DayCare *dayCare, PokeParty *party) {
    PartyPkm *egg = PokeParty_NewTempPkm(1, 1, 1, dayCare->heapId);

    if (DayCareSave_CheckSpawnFlag(dayCare->save) == TRUE) {
        GameData *gameData;

        GameBeacon_BroadcastEggReceived();
        setPartyFullToConst(dayCare->field);
        DayCare_CreateChildPkm(dayCare, egg);
        PokeParty_AddPkm(party, egg);
        DayCareSave_ResetSeed(dayCare->save);
        GFL_HeapFree(egg);
        gameData = GSYS_GetGameData(Field_GetGameSystem(dayCare->field));
        TransformVsPokePartyBySeason(gameData, party, GameData_GetSeason(gameData));
    }
}

void DayCare_CheckResetSeed(DayCare *dayCare) {
    if (DayCareSave_CheckSpawnFlag(dayCare->save) == TRUE) {
        DayCareSave_ResetSeed(dayCare->save);
        setPartyFullToConst(dayCare->field);
    }
}

BOOL DayCare_Update(DayCare *dayCare) {
    DayCareSave_UpdateStepCount(dayCare->save, 1);
    if (DayCare_CheckEggSpawn(dayCare) == TRUE) {
        DayCareSave_SetSpawnFlag(dayCare->save);
        setPartyHasSpaceToConst(dayCare->field);
        return TRUE;
    }
    return FALSE;
}

static void setPartyHasSpaceToConst(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    EventWork *eventWork = GameData_GetEventWork(gameData);
    PokeParty *party = GameData_GetParty(gameData);

    if (PokeParty_GetPkmCount(party) < PokeParty_GetCapacity(party)) {
        *EventWork_GetWkPtr(eventWork, EVENT_WORK_0x417f) = 1;
    }
}

static void setPartyFullToConst(Field *field) {
    EventWork *eventWork = GameData_GetEventWork(GSYS_GetGameData(Field_GetGameSystem(field)));
    *EventWork_GetWkPtr(eventWork, EVENT_WORK_0x417f) = 0;
}

BOOL DayCare_CheckSpawnFlag(DayCare *dayCare) {
    return DayCareSave_CheckSpawnFlag(dayCare->save);
}

u8 DayCare_GetPkmCount(DayCare *dayCare) {
    return DayCareSave_GetPkmCount(dayCare->save);
}

u32 DayCare_CalcNewLevel(DayCare *dayCare, u32 slot) {
    PartyPkm *pkm;
    u32 species;
    u32 form;
    u32 exp;

    if (DayCareSave_GetPkmStatus(dayCare->save, slot) != TRUE) {
        return 1;
    }
    pkm = DayCareSave_GetPkm(dayCare->save, slot);
    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    exp = PokeParty_GetParam(pkm, PKM_PARAM_EXP, NULL);
    PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
    return CalcLevelByExp(species, form, DayCare_CalcNewExp(exp, DayCareSave_GetPkmStepCounter(dayCare->save, slot)));
}

u32 DayCare_CalcLevelGain(DayCare *dayCare, u32 slot) {
    u32 level = PokeParty_GetParam(DayCareSave_GetPkm(dayCare->save, slot), PKM_PARAM_LEVEL, NULL);
    return DayCare_CalcNewLevel(dayCare, slot) - level;
}

u32 DayCare_CalcWithdrawCost(DayCare *dayCare, u32 slot) {
    return (DayCare_CalcLevelGain(dayCare, slot) + 1) * 100;
}

u32 DayCare_CalcEggSpawnChance(DayCare *dayCare) {
    u8 chance;
    PartyPkm *pkm1;
    PartyPkm *pkm2;
    BagSave *bag;

    if ((int)DayCareSave_GetPkmCount(dayCare->save) < 2) {
        return 0;
    }
    pkm1 = DayCareSave_GetPkm(dayCare->save, 0);
    pkm2 = DayCareSave_GetPkm(dayCare->save, 1);
    chance = DayCare_CalcBaseEggSpawnChance(pkm1, pkm2);
    bag = GameData_GetBag(GSYS_GetGameData(Field_GetGameSystem(dayCare->field)));
    if (BagSave_CheckAmount(bag, ITEM_OVAL_CHARM, 1, dayCare->heapId)) {
        switch (chance) {
        case 70:
            chance = 88;
            break;
        case 50:
            chance = 80;
            break;
        case 20:
            chance = 40;
            break;
        case 0:
            break;
        }
    }
    return chance;
}

PartyPkm *DayCare_GetPkm(DayCare *dayCare, u32 slot) {
    DayCareSave_GetPkmStatus(dayCare->save, slot);
    return DayCareSave_GetPkm(dayCare->save, slot);
}

static u32 DayCare_CalcNewExp(u32 exp, u32 steps) {
    u64 sum = (u64)exp + steps;

    if (sum > 0xffffffff) {
        return 0xffffffff;
    }
    return sum;
}

static void DayCare_CommitPkmGrowth(PartyPkm *pkm, u32 steps) {
    LearnsetEntry learnset[LEARNSET_MAX];
    u32 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    u32 maxExp = PML_UtilGetPkmLvExp(species, form, 100);
    int oldLevel = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
    u32 exp = PokeParty_GetParam(pkm, PKM_PARAM_EXP, NULL);
    u32 newExp = DayCare_CalcNewExp(exp, steps);
    int newLevel;
    int i;

    if (maxExp < newExp) {
        newExp = maxExp;
    }
    PokeParty_SetParam(pkm, PKM_PARAM_EXP, newExp);
    PokeParty_RecalcStats(pkm);
    i = 0;
    newLevel = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
    PML_LearnsetLvUpLoad(species, form, learnset);
    while (!LearnsetEntry_IsEnd(learnset[i])) {
        int level = LearnsetEntry_GetLevel(learnset[i]);
        u32 move = LearnsetEntry_GetMove(learnset[i]);

        if (newLevel < level) {
            return;
        }
        if (oldLevel < level) {
            writeLvUpMoveOpenSlot(pkm, move);
        }
        i++;
    }
}

static void writeLvUpMoveOpenSlot(PartyPkm *pkm, u32 move) {
    DayCareMoveParamTable moveParams = sOpenSlotMoveParams;
    int i;

    if (move != 0 && !DayCare_MonKnowsMove(pkm, move)) {
        for (i = 0; i < 4; i++) {
            if (PokeParty_GetParam(pkm, moveParams.values[i], NULL) == 0) {
                PokeParty_SetMove(pkm, move, i);
                return;
            }
        }
        PokeParty_SetLastMove(pkm, move);
    }
}

static BOOL DayCare_MonKnowsMove(PartyPkm *pkm, int move) {
    DayCareMoveParamTable moveParams = sExistingMoveParams;
    int i;

    for (i = 0; i < 4; i++) {
        if (move == PokeParty_GetParam(pkm, moveParams.values[i], NULL)) {
            return TRUE;
        }
    }
    return FALSE;
}

static void DayCareSave_ShiftPkms(DayCareSave *save) {
    int i;
    int j;

    for (i = 0; i < 2; i++) {
        if (DayCareSave_GetPkmStatus(save, i) != TRUE) {
            for (j = i + 1; j < 2; j++) {
                if (DayCareSave_GetPkmStatus(save, j) == TRUE) {
                    DayCareSave_SwapSlots(save, j, j - 1);
                }
            }
        }
    }
}

static u32 DayCare_CalcBaseEggSpawnChance(PartyPkm *pkm1, PartyPkm *pkm2) {
    u32 species[2];
    u32 form[2];
    u32 sex[2];
    u32 eggGroup1[2];
    u32 eggGroup2[2];
    PartyPkm *pkms[2];
    int i;

    pkms[0] = pkm1;
    pkms[1] = pkm2;
    for (i = 0; i < 2; i++) {
        species[i] = PokeParty_GetParam(pkms[i], PKM_PARAM_SPECIES, NULL);
        PokeParty_GetParam(pkms[i], PKM_PARAM_ID, NULL);
        form[i] = PokeParty_GetParam(pkms[i], PKM_PARAM_FORM, NULL);
        sex[i] = PokeParty_GetParam(pkms[i], PKM_PARAM_SEX, NULL);
        eggGroup1[i] = PML_PersonalGetParamSingle(species[i], form[i], PERSONAL_EGG_GROUP_1);
        eggGroup2[i] = PML_PersonalGetParamSingle(species[i], form[i], PERSONAL_EGG_GROUP_2);
    }
    if (eggGroup1[0] == EGG_GROUP_UNDISCOVERED || eggGroup1[1] == EGG_GROUP_UNDISCOVERED ||
        eggGroup2[0] == EGG_GROUP_UNDISCOVERED || eggGroup2[1] == EGG_GROUP_UNDISCOVERED) {
        return 0;
    }
    if (species[0] == SPECIES_DITTO && species[1] == SPECIES_DITTO) {
        return 0;
    }
    if (species[0] == SPECIES_DITTO || species[1] == SPECIES_DITTO) {
        return DayCare_CalcBaseEggSpawnChanceCore(pkm1, pkm2);
    }
    if (sex[0] == sex[1]) {
        return 0;
    }
    if (sex[0] == GENDER_UNKNOWN || sex[1] == GENDER_UNKNOWN) {
        return 0;
    }
    if (eggGroup1[0] != eggGroup1[1] && eggGroup1[0] != eggGroup2[1] && eggGroup2[0] != eggGroup1[1] &&
        eggGroup2[0] != eggGroup2[1]) {
        return 0;
    }
    return DayCare_CalcBaseEggSpawnChanceCore(pkm1, pkm2);
}

static u32 DayCare_CalcBaseEggSpawnChanceCore(PartyPkm *pkm1, PartyPkm *pkm2) {
    u32 species[2];
    u32 ids[2];
    PartyPkm *pkms[2];
    int i;

    pkms[0] = pkm1;
    pkms[1] = pkm2;
    for (i = 0; i < 2; i++) {
        species[i] = PokeParty_GetParam(pkms[i], PKM_PARAM_SPECIES, NULL);
        ids[i] = PokeParty_GetParam(pkms[i], PKM_PARAM_ID, NULL);
    }
    if (species[0] != species[1]) {
        if (ids[0] == ids[1]) {
            return 20;
        }
        return 50;
    }
    if (ids[0] == ids[1]) {
        return 50;
    }
    return 70;
}

static BOOL DayCare_CheckEggSpawn(DayCare *dayCare) {
    u16 steps = dayCare->steps;

    dayCare->steps += 16;
    if (steps > 0xff) {
        dayCare->steps = 0;
        if ((int)DayCareSave_GetPkmCount(dayCare->save) >= 2) {
            u32 chance = DayCare_CalcEggSpawnChance(dayCare);
            if (GFL_RandomLCAlt(1600) < chance) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void DayCare_CreateChildPkm(DayCare *dayCare, PartyPkm *egg) {
    GameSystem *gsys = Field_GetGameSystem(dayCare->field);
    PlayerInfo *info = GetGameDataPlayerInfo(GSYS_GetGameData(gsys));
    PartyPkm *pkm1 = DayCareSave_GetPkm(dayCare->save, 0);
    PartyPkm *pkm2 = DayCareSave_GetPkm(dayCare->save, 1);
    EggRandom rnd;

    rnd.state = DayCareSave_GetSeed(dayCare->save);
    rnd.multiplier = 0x5d588b656c078965ULL;
    rnd.increment = 0x269ec3;
    breeding(gsys, dayCare->heapId, info, 0xea62, pkm1, pkm2, egg, &rnd);
}

static void updateChatotPartyStatus(Field *field, PokeParty *party) {
    checkChatotInParty(getChatterDataAddress(GSYS_GetGameData(Field_GetGameSystem(field))), party);
}

static void breeding(GameSystem *gsys, HeapID heapId, PlayerInfo *info, u16 place, PartyPkm *pkm1, PartyPkm *pkm2,
                     PartyPkm *out, EggRandom *rnd) {
    EggParents parents;
    EggData egg;
    BOOL hasShinyCharm = BagSave_CheckAmount(GameData_GetBag(GSYS_GetGameData(gsys)), ITEM_SHINY_CHARM, 1, heapId);

    parents.parents[0] = pkm1;
    parents.parents[1] = pkm2;
    genderSort(&parents);
    setGenderlessFirst(&parents);
    clearEggDataBlk(&egg);
    getChild(&parents, &egg);
    nidoranCompute(&parents, &egg, rnd);
    volbeatCompute(&parents, &egg, rnd);
    manaphyHandler(&parents, &egg);
    incenseHandler(&parents, &egg);
    zeroFormData(&parents, &egg);
    shellosHandler(&parents, &egg);
    burmyHandler(&parents, &egg);
    basculinHandler(&parents, &egg);
    getNature(&parents, &egg, rnd);
    everstoneHandler(&parents, &egg, rnd);
    abilDecision(&parents, &egg, heapId, rnd);
    dittoAbilFix(&parents, &egg, rnd);
    FetchEggBaseHappiness(&egg, heapId);
    SetPowerItemInheritedIVs(&parents, &egg, rnd);
    SetRandomInheritedIVs(&parents, &egg, rnd);
    makeIVSet(&parents, &egg, rnd);
    getEggIDSet(info, &egg);
    rerollEggPID(&parents, &egg);
    shinyBreedHandler(&parents, &egg, hasShinyCharm);
    levelMoves(&egg);
    inheritEggMoves(&parents, &egg, heapId);
    inheritLvlUpMoves(&parents, &egg);
    inheritTmMoves(&parents, &egg, heapId);
    pichuLightBallHandler(&parents, &egg);
    createEggPkm(out, &egg, info, place, heapId);
}

static void genderSort(EggParents *parents) {
    int sex1 = PokeParty_GetParam(parents->parents[0], PKM_PARAM_SEX, NULL);
    int sex2 = PokeParty_GetParam(parents->parents[1], PKM_PARAM_SEX, NULL);

    if (sex1 == GENDER_FEMALE) {
        swapDaycarePKMPointers(parents);
    }
    sex1 = PokeParty_GetParam(parents->parents[0], PKM_PARAM_SEX, NULL);
    sex2 = PokeParty_GetParam(parents->parents[1], PKM_PARAM_SEX, NULL);
    if (sex2 == GENDER_MALE) {
        swapDaycarePKMPointers(parents);
    }
}

static void setGenderlessFirst(EggParents *parents) {
    int species = PokeParty_GetParam(parents->parents[0], PKM_PARAM_SPECIES, NULL);
    int sex = PokeParty_GetParam(parents->parents[1], PKM_PARAM_SEX, NULL);

    if (species == SPECIES_DITTO && sex == GENDER_UNKNOWN) {
        swapDaycarePKMPointers(parents);
    }
}

static void swapDaycarePKMPointers(EggParents *parents) {
    PartyPkm *tmp = parents->parents[0];
    parents->parents[0] = parents->parents[1];
    parents->parents[1] = tmp;
}

static void clearEggDataBlk(EggData *egg) {
    MI_CpuFill8(egg, 0, sizeof(EggData));
    egg->level = 1;
}

static void getChild(EggParents *parents, EggData *egg) {
    u16 species = PokeParty_GetParam(checkSecondPokemonDitto(parents), PKM_PARAM_SPECIES, NULL);
    egg->species = validateSpeciesAndGetOffspring(species);
}

static void nidoranCompute(EggParents *parents, EggData *egg, EggRandom *rnd) {
    u32 species = PokeParty_GetParam(checkSecondPokemonDitto(parents), PKM_PARAM_SPECIES, NULL);

    if (species == SPECIES_NIDORAN_F || species - SPECIES_NIDORAN_M <= 2) {
        if (EggRandom_Next(rnd, 2) == 0) {
            egg->species = SPECIES_NIDORAN_F;
        } else {
            egg->species = SPECIES_NIDORAN_M;
        }
    }
}

static void volbeatCompute(EggParents *parents, EggData *egg, EggRandom *rnd) {
    u32 species = PokeParty_GetParam(checkSecondPokemonDitto(parents), PKM_PARAM_SPECIES, NULL);

    if (species - SPECIES_VOLBEAT <= 1) {
        if (EggRandom_Next(rnd, 2) == 0) {
            egg->species = SPECIES_VOLBEAT;
        } else {
            egg->species = SPECIES_ILLUMISE;
        }
    }
}

static void manaphyHandler(EggParents *parents, EggData *egg) {
    if (PokeParty_GetParam(checkSecondPokemonDitto(parents), PKM_PARAM_SPECIES, NULL) == SPECIES_MANAPHY) {
        egg->species = SPECIES_PHIONE;
    }
}

static void incenseHandler(EggParents *parents, EggData *egg) {
    // Each baby species, the incense one of the parents has to hold for it, and the species the egg is without it
    DayCareIncenseTable incenses = sIncenses;
    PartyPkm *pkm = checkSecondPokemonDitto(parents);
    int item1;
    u32 item2;
    int i;

    PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
    item1 = PokeParty_GetParam(parents->parents[0], PKM_PARAM_ITEM, NULL);
    item2 = PokeParty_GetParam(parents->parents[1], PKM_PARAM_ITEM, NULL);
    for (i = 0; i < 12; i++) {
        if (egg->species == incenses.values[i][0] && item1 != incenses.values[i][1] && item2 != incenses.values[i][1]) {
            egg->species = incenses.values[i][2];
            return;
        }
    }
}

static PartyPkm *checkSecondPokemonDitto(EggParents *parents) {
    if (PokeParty_GetParam(parents->parents[1], PKM_PARAM_SPECIES, NULL) == SPECIES_DITTO) {
        return parents->parents[0];
    }
    return parents->parents[1];
}

static void zeroFormData(EggParents *parents, EggData *egg) {
    egg->form = 0;
}

static void shellosHandler(EggParents *parents, EggData *egg) {
    PartyPkm *pkm = checkSecondPokemonDitto2(parents);

    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) - SPECIES_SHELLOS <= 1) {
        egg->form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    }
}

static void burmyHandler(EggParents *parents, EggData *egg) {
    PartyPkm *pkm = checkSecondPokemonDitto2(parents);

    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) - SPECIES_BURMY <= 1) {
        egg->form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    }
}

static void basculinHandler(EggParents *parents, EggData *egg) {
    PartyPkm *pkm = checkSecondPokemonDitto2(parents);

    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == SPECIES_BASCULIN) {
        egg->form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    }
}

static PartyPkm *checkSecondPokemonDitto2(EggParents *parents) {
    if (PokeParty_GetParam(parents->parents[1], PKM_PARAM_SPECIES, NULL) == SPECIES_DITTO) {
        return parents->parents[0];
    }
    return parents->parents[1];
}

static void getNature(EggParents *parents, EggData *egg, EggRandom *rnd) {
    egg->nature = EggRandom_Next(rnd, 25);
}

static void everstoneHandler(EggParents *parents, EggData *egg, EggRandom *rnd) {
    u32 nature1;
    u32 item1;
    u32 nature2;
    BOOL everstone1;
    BOOL everstone2;

    item1 = PokeParty_GetParam(parents->parents[0], PKM_PARAM_ITEM, NULL);
    everstone1 = FALSE;
    if (item1 == ITEM_EVERSTONE) {
        everstone1 = TRUE;
    }
    everstone2 = FALSE;
    if (PokeParty_GetParam(parents->parents[1], PKM_PARAM_ITEM, NULL) == ITEM_EVERSTONE) {
        everstone2 = TRUE;
    }
    nature1 = PokeParty_GetParam(parents->parents[0], PKM_PARAM_NATURE, NULL);
    nature2 = PokeParty_GetParam(parents->parents[1], PKM_PARAM_NATURE, NULL);
    if (everstone1 && everstone2) {
        if (EggRandom_Next(rnd, 2) == 0) {
            egg->nature = nature1;
            egg->natureFromParent1 = 1;
        } else {
            egg->nature = nature2;
            egg->natureFromParent2 = 1;
        }
    } else if (everstone1) {
        egg->nature = nature1;
        egg->natureFromParent1 = 1;
    } else if (everstone2) {
        egg->nature = nature2;
        egg->natureFromParent2 = 1;
    }
}

static void abilDecision(EggParents *parents, EggData *egg, HeapID heapId, EggRandom *rnd) {
    int slot = getParentAbilSlotNumber(parents->parents[1]);
    u32 roll = EggRandom_Next(rnd, 100);

    switch (slot) {
    case ABILITY_SLOT_1:
    default:
        if (roll < 80) {
            egg->abilitySlot = ABILITY_SLOT_1;
        } else {
            egg->abilitySlot = ABILITY_SLOT_2;
        }
        break;
    case ABILITY_SLOT_2:
        if (roll < 20) {
            egg->abilitySlot = ABILITY_SLOT_1;
        } else {
            egg->abilitySlot = ABILITY_SLOT_2;
        }
        break;
    case ABILITY_SLOT_HIDDEN:
        if (roll < 20) {
            egg->abilitySlot = ABILITY_SLOT_1;
        } else if (roll < 40) {
            egg->abilitySlot = ABILITY_SLOT_2;
        } else {
            egg->abilitySlot = ABILITY_SLOT_HIDDEN;
        }
        break;
    }
}

static void dittoAbilFix(EggParents *parents, EggData *egg, EggRandom *rnd) {
    int species1 = PokeParty_GetParam(parents->parents[0], PKM_PARAM_SPECIES, NULL);
    int species2 = PokeParty_GetParam(parents->parents[1], PKM_PARAM_SPECIES, NULL);

    if (species1 == SPECIES_DITTO || species2 == SPECIES_DITTO) {
        if (EggRandom_Next(rnd, 2) == 0) {
            egg->abilitySlot = ABILITY_SLOT_1;
        } else {
            egg->abilitySlot = ABILITY_SLOT_2;
        }
    }
}

static void FetchEggBaseHappiness(EggData *egg, HeapID heapId) {
    PersonalData *personal = PML_PersonalLoad(egg->species, egg->form, heapId);
    u32 happiness = PML_PersonalGetParam(personal, PERSONAL_HATCH_CYCLES);

    PML_PersonalFree(personal);
    egg->happiness = happiness;
}

static void SetPowerItemInheritedIVs(EggParents *parents, EggData *egg, EggRandom *rnd) {
    BOOL power1 = FALSE;
    BOOL power2 = FALSE;
    int item1 = PokeParty_GetParam(parents->parents[0], PKM_PARAM_ITEM, NULL);
    int item2 = PokeParty_GetParam(parents->parents[1], PKM_PARAM_ITEM, NULL);
    DayCareStatParamTable powerItems = sPowerItems;
    int stat1;
    int stat2;
    int i;

    for (i = 0; i < 6; i++) {
        if (item1 == powerItems.values[i]) {
            power1 = TRUE;
            stat1 = i;
            break;
        }
    }
    for (i = 0; i < 6; i++) {
        if (item2 == powerItems.values[i]) {
            power2 = TRUE;
            stat2 = i;
            break;
        }
    }
    if (power1 && power2) {
        if (EggRandom_Next(rnd, 2) == 0) {
            egg->ivFromParent1[stat1] = 1;
            egg->inheritedIvCount++;
        } else {
            egg->ivFromParent2[stat2] = 1;
            egg->inheritedIvCount++;
        }
    } else if (power1) {
        egg->ivFromParent1[stat1] = 1;
        egg->inheritedIvCount++;
    } else if (power2) {
        egg->ivFromParent2[stat2] = 1;
        egg->inheritedIvCount++;
    }
}

static void SetRandomInheritedIVs(EggParents *parents, EggData *egg, EggRandom *rnd) {
    while (egg->inheritedIvCount < 3) {
        u32 stat = EggRandom_Next(rnd, 6);

        if (EggRandom_Next(rnd, 2) == 0) {
            if (egg->ivFromParent1[stat] == 0 && egg->ivFromParent2[stat] == 0) {
                egg->ivFromParent1[stat] = 1;
                egg->inheritedIvCount++;
            }
        } else {
            if (egg->ivFromParent1[stat] == 0 && egg->ivFromParent2[stat] == 0) {
                egg->ivFromParent2[stat] = 1;
                egg->inheritedIvCount++;
            }
        }
    }
}

static void makeIVSet(EggParents *parents, EggData *egg, EggRandom *rnd) {
    DayCareStatParamTable ivParams = sInheritedIVParams;
    int i;

    for (i = 0; i < 6; i++) {
        BOOL set = FALSE;

        if (egg->ivFromParent1[i] != 0) {
            egg->ivs[i] = PokeParty_GetParam(parents->parents[0], ivParams.values[i], NULL);
            set = TRUE;
        }
        if (egg->ivFromParent2[i] != 0) {
            egg->ivs[i] = PokeParty_GetParam(parents->parents[1], ivParams.values[i], NULL);
            set = TRUE;
        }
        if (!set) {
            egg->ivs[i] = EggRandom_Next(rnd, 32);
        }
    }
}

static void getEggIDSet(PlayerInfo *info, EggData *egg) {
    egg->trainerId = getIDAsUInt(info);
}

static void rerollEggPID(EggParents *parents, EggData *egg) {
    u32 ability;

    if (egg->abilitySlot == ABILITY_SLOT_1 || egg->abilitySlot != ABILITY_SLOT_2) {
        ability = 0;
    } else {
        ability = 1;
    }
    egg->pid = PML_GenPID(egg->trainerId, egg->species, egg->form, GENDER_UNKNOWN, ability, PID_RARE_RANDOM);
}

static void shinyBreedHandler(EggParents *parents, EggData *egg, BOOL hasShinyCharm) {
    int i = 0;
    int region1 = PokeParty_GetParam(parents->parents[0], PKM_PARAM_REGION, NULL);
    int region2 = PokeParty_GetParam(parents->parents[1], PKM_PARAM_REGION, NULL);

    if (!PML_UtilPIDIsRare(egg->trainerId, egg->pid)) {
        if (hasShinyCharm == TRUE) {
            do {
                rerollEggPID(parents, egg);
                if (PML_UtilPIDIsRare(egg->trainerId, egg->pid)) {
                    return;
                }
                i++;
            } while (i < 2);
        }
        if (region1 != region2) {
            int j = 0;
            do {
                rerollEggPID(parents, egg);
                if (PML_UtilPIDIsRare(egg->trainerId, egg->pid)) {
                    return;
                }
                j++;
            } while (j < 5);
        }
    }
}

static void levelMoves(EggData *egg) {
    LearnsetEntry learnset[LEARNSET_MAX];
    int i;
    u32 param;

    PML_LearnsetLvUpLoad(egg->species, egg->form, learnset);
    i = 0;
    while (!LearnsetEntry_IsEnd(learnset[i])) {
        u32 level = LearnsetEntry_GetLevel(learnset[i]);
        u32 move = LearnsetEntry_GetMove(learnset[i]);

        if (egg->level < level) {
            return;
        }
        PushEggPkmMove(egg, move);
        i++;
    }
}

static void inheritEggMoves(EggParents *parents, EggData *egg, HeapID heapId) {
    DayCareMoveParamTable moveParams = sEggMoveParams;
    u16 *eggMoves = GFL_ArcSysReadHeapNew(124, egg->species, heapId);
    u16 count = eggMoves[0];
    int i;

    for (i = 0; i < 4; i++) {
        u32 move = PokeParty_GetParam(parents->parents[0], moveParams.values[i], NULL);
        int j;

        for (j = 1; j <= count; j++) {
            if (move == eggMoves[j]) {
                PushEggPkmMove(egg, move);
                break;
            }
        }
    }
    GFL_HeapFree(eggMoves);
}

static void inheritLvlUpMoves(EggParents *parents, EggData *egg) {
    DayCareMoveParamTable moveParams = sLevelMoveParams;
    LearnsetEntry learnset[LEARNSET_MAX];
    int i;
    u32 param;

    PML_LearnsetLvUpLoad(egg->species, egg->form, learnset);
    i = 0;
    while (!LearnsetEntry_IsEnd(learnset[i])) {
        int j;
        u32 move = LearnsetEntry_GetMove(learnset[i]);
        BOOL knownBy1 = FALSE;
        BOOL knownBy2 = FALSE;

        for (j = 0; j < 4; j++) {
            param = PokeParty_GetParam(parents->parents[0], moveParams.values[j], NULL);
            if (param == move) {
                knownBy1 = TRUE;
                break;
            }
        }
        for (j = 0; j < 4; j++) {
            param = PokeParty_GetParam(parents->parents[1], moveParams.values[j], NULL);
            if (param == move) {
                knownBy2 = TRUE;
                break;
            }
        }
        if (knownBy1 && knownBy2) {
            PushEggPkmMove(egg, move);
        }
        i++;
    }
}

static void inheritTmMoves(EggParents *parents, EggData *egg, HeapID heapId) {
    DayCareMoveParamTable moveParams = sTmMoveParams;
    int i = 0;

    do {
        int move = PokeParty_GetParam(parents->parents[0], moveParams.values[i], NULL);
        int item;

        for (item = ITEM_TM01; item <= ITEM_LAST; item++) {
            if (move == PML_ItemGetTMMoveID(item) && canPkmLearnTMItemNumber(heapId, egg->species, egg->form, item)) {
                PushEggPkmMove(egg, move);
                break;
            }
        }
        i++;
    } while (i < 4);
}

static void pichuLightBallHandler(EggParents *parents, EggData *egg) {
    if (egg->species == SPECIES_PICHU &&
        (PokeParty_GetParam(parents->parents[0], PKM_PARAM_ITEM, NULL) == ITEM_LIGHT_BALL ||
         PokeParty_GetParam(parents->parents[1], PKM_PARAM_ITEM, NULL) == ITEM_LIGHT_BALL)) {
        PushEggPkmMove(egg, MOVE_VOLT_TACKLE);
    }
}

static void PushEggPkmMove(EggData *egg, int move) {
    u32 i;
    u32 count;
    int j;

    if (move != 0) {
        count = egg->moveCount;
        for (i = 0; i < count; i++) {
            if (move == egg->moves[i]) {
                return;
            }
        }
        if (count < 4) {
            egg->moves[count] = move;
            egg->moveCount++;
            return;
        }
        for (j = 0; j < 3; j++) {
            egg->moves[j] = egg->moves[j + 1];
        }
        egg->moves[j] = move;
    }
}

static void createEggPkm(PartyPkm *pkm, const EggData *egg, PlayerInfo *info, u32 place, HeapID heapId) {
    DayCareStatParamTable ivParams = sEggIVParams;
    StrBuf *name;
    int i;

    PokeParty_CreatePkm(pkm, egg->species, egg->level, egg->trainerId, -1, egg->pid);
    PokeParty_SetParam(pkm, PKM_PARAM_FORM, egg->form);
    PokeParty_SetParam(pkm, PKM_PARAM_NATURE, egg->nature);
    PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, egg->happiness);
    if (egg->abilitySlot == ABILITY_SLOT_HIDDEN) {
        PokeParty_SetHiddenAbil(pkm, egg->species, egg->form);
    }
    for (i = 0; i < 6; i++) {
        PokeParty_SetParam(pkm, ivParams.values[i], egg->ivs[i]);
    }
    PokeParty_SetParam(pkm, PKM_PARAM_MOVE1, egg->moves[0]);
    PokeParty_SetParam(pkm, PKM_PARAM_MOVE2, egg->moves[1]);
    PokeParty_SetParam(pkm, PKM_PARAM_MOVE3, egg->moves[2]);
    PokeParty_SetParam(pkm, PKM_PARAM_MOVE4, egg->moves[3]);
    name = copyTrainerNameToNewStrbuf(info->name, heapId);
    PokeParty_SetParam(pkm, PKM_PARAM_OT_NAME, (u32)name);
    GFL_StrBufFree(name);
    name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, SPECIES_EGG);
    PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME, (u32)name);
    GFL_StrBufFree(name);
    PokeParty_SetupMetData(pkm, 5, info, place, heapId);
    PokeParty_SetParam(pkm, PKM_PARAM_IS_EGG, 1);
    PokeParty_PPRecover(pkm);
    PokeParty_RecalcStats(pkm);
    PokeParty_Recover(pkm);
}

static BOOL canPkmLearnTMItemNumber(HeapID heapId, u16 species, u16 form, u16 item) {
    u32 flags;
    u32 bit;
    PersonalData *personal;

    if (species >= SPECIES_GENESECT) {
        return FALSE;
    }
    if (!PML_ItemIsTMHM(item)) {
        return FALSE;
    }
    personal = PML_PersonalLoad(species, form, heapId);
    bit = PML_ItemGetTMBitMask(item);
    if (bit < 32) {
        flags = PML_PersonalGetParam(personal, PERSONAL_MACHINES_1);
    } else if (bit < 64) {
        flags = PML_PersonalGetParam(personal, PERSONAL_MACHINES_2);
        bit -= 32;
    } else if (bit < 96) {
        flags = PML_PersonalGetParam(personal, PERSONAL_MACHINES_3);
        bit -= 64;
    } else {
        flags = PML_PersonalGetParam(personal, PERSONAL_MACHINES_4);
        bit -= 96;
    }
    PML_PersonalFree(personal);
    return (1 << bit) & flags;
}

static u32 validateSpeciesAndGetOffspring(u32 species) {
    u16 baby;

    if (species < SPECIES_GENESECT) {
        GFL_ArcSysReadRange(&baby, ARCID_BABYMONS, species, 0, 2);
        return baby;
    }
    return species;
}

static const DayCareMoveParamTable sOpenSlotMoveParams = { { PKM_PARAM_MOVE1, PKM_PARAM_MOVE2, PKM_PARAM_MOVE3,
                                                             PKM_PARAM_MOVE4 } };

static const DayCareMoveParamTable sExistingMoveParams = { { PKM_PARAM_MOVE1, PKM_PARAM_MOVE2, PKM_PARAM_MOVE3,
                                                             PKM_PARAM_MOVE4 } };

static const DayCareIncenseTable sIncenses = { {
    { SPECIES_AZURILL, ITEM_SEA_INCENSE, SPECIES_MARILL },
    { SPECIES_WYNAUT, ITEM_LAX_INCENSE, SPECIES_WOBBUFFET },
    { SPECIES_MIME_JR, ITEM_ODD_INCENSE, SPECIES_MR_MIME },
    { SPECIES_BONSLY, ITEM_ROCK_INCENSE, SPECIES_SUDOWOODO },
    { SPECIES_MUNCHLAX, ITEM_FULL_INCENSE, SPECIES_SNORLAX },
    { SPECIES_MANTYKE, ITEM_WAVE_INCENSE, SPECIES_MANTINE },
    { SPECIES_BUDEW, ITEM_ROSE_INCENSE, SPECIES_ROSELIA },
    { SPECIES_HAPPINY, ITEM_LUCK_INCENSE, SPECIES_CHANSEY },
    { SPECIES_CHINGLING, ITEM_PURE_INCENSE, SPECIES_CHIMECHO },
} };

static const DayCareStatParamTable sPowerItems = { { ITEM_POWER_WEIGHT, ITEM_POWER_BRACER, ITEM_POWER_BELT,
                                                     ITEM_POWER_LENS, ITEM_POWER_BAND, ITEM_POWER_ANKLET } };

static const DayCareMoveParamTable sLevelMoveParams = { { PKM_PARAM_MOVE1, PKM_PARAM_MOVE2, PKM_PARAM_MOVE3,
                                                          PKM_PARAM_MOVE4 } };

static const DayCareMoveParamTable sTmMoveParams = { { PKM_PARAM_MOVE1, PKM_PARAM_MOVE2, PKM_PARAM_MOVE3,
                                                       PKM_PARAM_MOVE4 } };

static const DayCareStatParamTable sInheritedIVParams = { { PKM_PARAM_IV_HP, PKM_PARAM_IV_ATTACK, PKM_PARAM_IV_DEFENSE,
                                                            PKM_PARAM_IV_SP_ATTACK, PKM_PARAM_IV_SP_DEFENSE,
                                                            PKM_PARAM_IV_SPEED } };

static const DayCareStatParamTable sEggIVParams = { { PKM_PARAM_IV_HP, PKM_PARAM_IV_ATTACK, PKM_PARAM_IV_DEFENSE,
                                                      PKM_PARAM_IV_SP_ATTACK, PKM_PARAM_IV_SP_DEFENSE,
                                                      PKM_PARAM_IV_SPEED } };

static const DayCareMoveParamTable sEggMoveParams = { { PKM_PARAM_MOVE1, PKM_PARAM_MOVE2, PKM_PARAM_MOVE3,
                                                        PKM_PARAM_MOVE4 } };
