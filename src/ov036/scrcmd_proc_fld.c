// The script commands that open other screens from the field: the PC Box, phrase inputs, the Hall of Fame after the
// game is cleared, the C-Gear, Geonet, the options, the Xtransceiver, the town map and name inputs. The name is the
// ROM's own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except our descriptive ones of commands 0x231, 0x265 and
// 0x28F and of the ScrcmdProcFld_ helpers.
#include "types.h"
#include "app/box2.h"
#include "app/geonet.h"
#include "app/townmap.h"
#include "app/xtransceiver.h"
#include "field/app_call.h"
#include "field/event_cgear_poweron.h"
#include "field/event_game_clear.h"
#include "field/event_game_manual.h"
#include "field/event_phrase_input.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/pc_sound.h"
#include "field/player_state.h"
#include "field/scrcmd_proc_fld.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "save/box.h"
#include "save/c_gear_savedata.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

// What the PC Box's callback stores its result from
typedef struct {
    GameData *gameData;
    u16 *result;
    Box2Param *param;
} PCCallWork;

// Where the town map's callback stores the place picked
typedef struct {
    u16 *picked;
    u16 *zoneId;
    TownMapParam *param;
} PlaceSelectWork;

static void ScrcmdProcFld_FinishPlaceSelect(void *work);
static void ScrcmdProcFld_FreeXTransceiverParam(void *work);

// Store the Battle Box's team when it was edited, and whether the player closed the PC Box rather than backed out
static void ScrcmdProcFld_FinishOv170Event(void *data) {
    PCCallWork *work = data;
    u16 closed;

    if (work->param->mode == BOX2_MODE_BATTLE_BOX) {
        BattleBoxSave *battleBox = getBattleBox(GameData_GetSaveControl(work->gameData));

        copySelectedPkmToBattleBlk(battleBox, work->param->party);
    }
    if (work->param->mode == BOX2_MODE_BATTLE_BOX) {
        GFL_HeapFree(work->param->party);
    }
    switch (work->param->closed) {
    case FALSE:
    default:
        closed = FALSE;
        break;
    case TRUE:
        closed = TRUE;
        break;
    }
    *work->result = closed;
    GFL_HeapFree(work->param);
}

BOOL Scrcmd_ProcFld_CallPC(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    SaveControl *save = GameData_GetSaveControl(gameData);
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);
    Field *field = GSYS_GetField(gsys);
    u16 *result = ScriptReadVar(vm, env);
    u16 mode = ScriptReadAny(vm, env);
    Box2Param *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(Box2Param), FALSE, "scrcmd_proc_fld.c", 130);
    PCCallWork *callWork;

    param->gameData = gameData;
    param->boxes = GameData_GetBoxSaveAccessor(gameData);
    param->party = GameData_GetParty(gameData);
    param->bag = GameData_GetBag(gameData);
    param->playerInfo = GetGameDataPlayerInfo(gameData);
    param->trainerData = getTrainerDataBlkAddress(save);
    param->isNationalDex = PokeDex_IsNationalObtained(pokedex);
    param->mode = mode;
    param->battleBoxLocked = FALSE;
    if (mode == BOX2_MODE_BATTLE_BOX) {
        BattleBoxSave *battleBox = getBattleBox(save);

        param->party = convertBoxedPokeSetToParty(battleBox, HEAPID_GAMEEVENT);
        param->battleBoxLocked = BattleBoxSave_CheckFlags(battleBox, 3);
    }
    callWork = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PCCallWork), FALSE, "scrcmd_proc_fld.c", 150);
    callWork->gameData = gameData;
    callWork->result = result;
    callWork->param = param;
    ScriptWork_CallEvent(work,
                         EventFieldSubprocessCall_CreateWithCallback(gsys, field, OVERLAY_BOX2, &BOX2_PROC_FUNCTIONS,
                                                                     param, ScrcmdProcFld_FinishOv170Event, callWork));
    return TRUE;
}

BOOL Scrcmd_ProcFld_CallGreetingPhraseInput(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);

    ScriptWork_CallEvent(work, EventPhraseInput_Create(gsys, field, NULL, NAME_ENTRY_GREETING, ScriptReadVar(vm, env)));
    return TRUE;
}

BOOL Scrcmd_ProcFld_CallThanksPhraseInput(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);

    ScriptWork_CallEvent(work, EventPhraseInput_Create(gsys, field, NULL, NAME_ENTRY_THANKS, ScriptReadVar(vm, env)));
    return TRUE;
}

BOOL Scrcmd_ProcFld_CallHappyPhraseInput(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);

    ScriptWork_CallEvent(work,
                         EventPhraseInput_Create(gsys, field, NULL, NAME_ENTRY_HAPPY_MESSAGE, ScriptReadVar(vm, env)));
    return TRUE;
}

// The game clear sequence takes over once the script ends
BOOL Scrcmd_ProcFld_CallGameClear(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GSYS_GetField(gsys);
    ScriptWork_SetPostEvent(work, EventGameClear_Create(gsys, ScriptReadAny(vm, env)));
    VM_Halt(vm);
    return TRUE;
}

BOOL Scrcmd_ProcFld_CGearPowerOn(VM *vm, FieldScriptEnv *env) {
    CGearSettings *cgear = CGearSettings_GetFromSave(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    u16 bootComm = ScriptReadAny(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    CGearSettings_SetOn(cgear, 1);
    ScriptWork_CallEvent(work, EventCGearPowerOn_Create(gsys, bootComm));
    return TRUE;
}

BOOL Scrcmd_ProcFld_CallGeonet(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(gsys);

    ScriptWork_CallEvent(work, EventFieldSubprocessTransition_Create(gsys, field, OVERLAY_GEONET,
                                                                     &GEONET_PROC_FUNCTIONS, GSYS_GetGameData(gsys)));
    return TRUE;
}

BOOL Scrcmd_ProcFld_CallRecordSystem(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 mode;
    u16 *result;
    u16 option;

    GameData_GetSaveControl(GSYS_GetGameData(gsys));
    GSYS_GetField(gsys);
    mode = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    switch (mode) {
    case 0:
    default:
        option = 0;
        break;
    case 1:
        option = 1;
        break;
    }
    ScriptWork_CallEvent(work, Scrcmd_CreateSubProcEvent(gsys, option, result));
    return TRUE;
}

BOOL Scrcmd_ProcFld_CallGameManual(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, EventGameManual_Create(gsys, ScriptReadVar(vm, env)));
    return TRUE;
}

BOOL Scrcmd_ProcFld_CallXTransceiver(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u16 scriptId = ScriptReadAny(vm, env);
    u16 swapPalette = VM_Read16(vm);
    LocalTvtParam *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(LocalTvtParam), TRUE, "scrcmd_proc_fld.c", 401);

    param->mode = LOCAL_TVT_MODE_SCRIPT;
    param->swapPalette = swapPalette;
    param->scriptId = scriptId;
    param->gameData = FieldScriptEnv_GetGameData(env);
    param->gsys = gsys;
    ScriptWork_CallEvent(work, EventFieldSubprocessCall_CreateWithCallback(gsys, field, OVERLAY_XTRANSCEIVER,
                                                                           &XTRANSCEIVER_PROC_FUNCTIONS, param,
                                                                           ScrcmdProcFld_FreeXTransceiverParam, param));
    return TRUE;
}

BOOL Scrcmd_ProcFld_CallXTransceiverSecret(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u16 swapPalette;
    LocalTvtParam *param;

    getHollow_RivalBlk(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    swapPalette = ScriptReadAny(vm, env);
    param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(LocalTvtParam), TRUE, "scrcmd_proc_fld.c", 437);
    param->mode = LOCAL_TVT_MODE_CALL_SECRET;
    param->swapPalette = swapPalette;
    param->gameData = FieldScriptEnv_GetGameData(env);
    param->gsys = gsys;
    ScriptWork_CallEvent(work, EventFieldSubprocessCall_CreateWithCallback(gsys, field, OVERLAY_XTRANSCEIVER,
                                                                           &XTRANSCEIVER_PROC_FUNCTIONS, param,
                                                                           ScrcmdProcFld_FreeXTransceiverParam, param));
    return TRUE;
}

static void ScrcmdProcFld_FinishPlaceSelect(void *data) {
    PlaceSelectWork *work = data;

    if (work->param->result == TOWNMAP_RESULT_FLY) {
        *work->picked = TRUE;
        *work->zoneId = work->param->zoneId;
    } else {
        *work->picked = FALSE;
    }
    GFL_HeapFree(work->param);
}

// Pick a place on the town map
BOOL Scrcmd_ProcFld_CallPlaceSelect(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    PlayerState *playerState = GameData_GetPlayerState(gameData);
    u16 *picked = ScriptReadVar(vm, env);
    u16 *zoneId = ScriptReadVar(vm, env);
    TownMapParam *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(TownMapParam), TRUE, "scrcmd_proc_fld.c", 512);
    PlaceSelectWork *selectWork;

    param->mode = TOWNMAP_MODE_FLY;
    param->gsys = gsys;
    param->zoneId = PlayerState_GetZoneID(playerState);
    param->escapeZoneId = GameData_GetEscapeRopeZone(gameData)->zoneId;
    selectWork = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PlaceSelectWork), TRUE, "scrcmd_proc_fld.c", 519);
    selectWork->picked = picked;
    selectWork->zoneId = zoneId;
    selectWork->param = param;
    ScriptWork_CallEvent(
        work, EventFieldSubprocessCall_CreateWithCallback(gsys, field, OVERLAY_TOWNMAP, &TOWNMAP_PROC_FUNCTIONS, param,
                                                          ScrcmdProcFld_FinishPlaceSelect, selectWork));
    return TRUE;
}

BOOL Scrcmd_ProcFld_CallRivalNameInput(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(work, RivalNameEntry_Create(gsys, ScriptReadVar(vm, env)));
    return TRUE;
}

BOOL Scrcmd_ProcFld_CallWordSetPokeNameInput(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u16 species = ScriptReadAny(vm, env);
    u16 form = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(work, EventPokeNameWordSetInput_Create(gsys, species, form, ScriptReadVar(vm, env), wordSet));
    return TRUE;
}

static void ScrcmdProcFld_FreeXTransceiverParam(void *work) {
    GFL_HeapFree(work);
}
