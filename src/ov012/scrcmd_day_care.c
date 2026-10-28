// The ROM names this file scrcmd_sodateya.c.
#include "types.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "field/field_daycare.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "gfl/heap.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/pokedex_savedata.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL Scrcmd_DayCare_DayCareCheckSpawnFlag(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    *result = DayCare_CheckSpawnFlag(dayCare);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareBreed(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    PokeParty *party = GameData_GetParty(FieldScriptEnv_GetGameData(env));
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    DayCare_Breed(dayCare, party);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareResetSeed(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    DayCare_CheckResetSeed(dayCare);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareCallPokeSelect(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    GameData_GetParty(gameData);
    getDaycareBlockAddress(GameData_GetSaveControl(gameData));
    ScriptWork_CallEvent(work, EventDayCarePokeSelect_Create(gsys, field, result));
    return TRUE;
}

BOOL Scrcmd_DayCare_DayCareDeposit(VM *vm, FieldScriptEnv *env) {
    u16 slot = ScriptReadAny(vm, env);
    PokeParty *party = GameData_GetParty(FieldScriptEnv_GetGameData(env));
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    DayCare_AddPkm(dayCare, party, slot);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareWithdraw(VM *vm, FieldScriptEnv *env) {
    u16 slot = ScriptReadAny(vm, env);
    PokeParty *party = GameData_GetParty(FieldScriptEnv_GetGameData(env));
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    DayCare_RemovePkm(dayCare, slot, party);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareGetPkmCount(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    *result = DayCare_GetPkmCount(dayCare);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareCalcEggSpawnChance(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    *result = DayCare_CalcEggSpawnChance(dayCare);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareGetSpecies(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 slot = ScriptReadAny(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    PartyPkm *pkm = DayCare_GetPkm(dayCare, slot);
    *result = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareGetForme(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 slot = ScriptReadAny(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    PartyPkm *pkm = DayCare_GetPkm(dayCare, slot);
    *result = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareGetSex(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 slot = ScriptReadAny(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    PartyPkm *pkm = DayCare_GetPkm(dayCare, slot);
    *result = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareCalcNewLevel(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 slot = ScriptReadAny(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    *result = DayCare_CalcNewLevel(dayCare, slot);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareCalcLevelGain(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 slot = ScriptReadAny(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    DayCare_GetPkm(dayCare, slot);
    *result = DayCare_CalcLevelGain(dayCare, slot);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareCalcWithdrawCost(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    u16 slot = ScriptReadAny(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    DayCare_GetPkm(dayCare, slot);
    *result = DayCare_CalcWithdrawCost(dayCare, slot);
    return FALSE;
}

BOOL Scrcmd_DayCare_DayCareGetSexForNamePrint(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    DayCare *dayCare = Field_GetDayCare(GSYS_GetField(gsys));
    u16 *result = ScriptReadVar(vm, env);
    u16 showSex = ScriptReadAny(vm, env);
    u16 slot = ScriptReadAny(vm, env);
    PartyPkm *pkm = DayCare_GetPkm(dayCare, slot);
    if (showSex == 0) {
        *result = getNameGenderStatus(pkm);
    } else {
        *result = 0;
    }
    return FALSE;
}

u32 getNameGenderStatus(PartyPkm *pkm) {
    u32 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
    if (species == SPECIES_NIDORAN_M || species == SPECIES_NIDORAN_F) {
        if (PokeParty_GetParam(pkm, 0x75, NULL) == 0) {
            return 0;
        }
    }
    switch (sex) {
    case 2:
    default:
        return 0;
    case 0:
        return 1;
    case 1:
        return 2;
    }
}

GameEventReturnCode EventDayCarePokeSelect_Callback(GameEvent *event, u32 *state, void *data) {
    DayCarePokeSelectWork *work = data;
    GameSystem *gsys = work->gsys;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceOutTransitionDefault(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0));
        *state = 1;
        break;
    case 1:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, work->field));
        work->partyParam->index = work->summaryParam->partyIndex;
        *state = 2;
        break;
    case 2:
        GameEvent_ChainNext(event, EventPokeList_Create(gsys, work->field, work->partyParam, work->summaryParam));
        *state = 3;
        break;
    case 3:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        *state = 4;
        break;
    case 4:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceInTransition(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        *state = 5;
        break;
    case 5:
        *work->result = work->partyParam->index;
        GFL_HeapFree(work->partyParam);
        GFL_HeapFree(work->summaryParam);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventDayCarePokeSelect_Create(GameSystem *gsys, Field *field, u16 *result) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PokeParty *party = GameData_GetParty(gameData);
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);
    PokeListParam *partyParam = PokeListParam_Create(gameData, 0x12, party, HEAPID_GAMEEVENT);
    PStatusParam *summaryParam =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PStatusParam), FALSE, "scrcmd_sodateya.c", 0x22e);
    GameEvent *event;
    DayCarePokeSelectWork *work;

    summaryParam->party = partyParam->party;
    summaryParam->trainerData = partyParam->trainerData;
    summaryParam->gameData = gameData;
    summaryParam->dataType = PSTATUS_DATA_PARTY;
    summaryParam->partyCount = PokeParty_GetPkmCount(party);
    summaryParam->mode = PSTATUS_MODE_NORMAL;
    summaryParam->page = PSTATUS_PAGE_INFO;
    summaryParam->partyIndex = 0;
    summaryParam->isNationalDex = PokeDex_IsNationalObtained(pokedex);
    event = GameEvent_Create(gsys, NULL, EventDayCarePokeSelect_Callback, sizeof(DayCarePokeSelectWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = field;
    work->partyParam = partyParam;
    work->summaryParam = summaryParam;
    work->result = result;
    return event;
}
