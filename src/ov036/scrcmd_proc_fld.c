// The script commands that open other screens from the field: the PC Box, phrase inputs, the Hall of Fame after the
// game is cleared, the C-Gear, Geonet, the options, the Xtransceiver, the town map and name inputs. The name is the
// ROM's own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
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
#include "save/pokedex.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

#define BOX2_MODE_BATTLE_BOX 4

// What the PC Box's callback stores its result from
typedef struct {
    GameData *gameData;
    u16 *result;
    Box2Param *param;
} PCCallWork;

// The Xtransceiver's parameters
typedef struct {
    u32 mode;
    u32 unk04;
    u16 unk08;
    GameData *gameData;
    GameSystem *gsys;
} XTransceiverParam;

// The town map's parameters, and where its callback stores the place picked
typedef struct {
    u32 unk00;
    u32 result;
    u16 zoneId;
    u16 escapeZoneId;
    u32 unk0C;
    u32 unk10;
    GameSystem *gsys;
} TownMapParam;

typedef struct {
    u16 *picked;
    u16 *zoneId;
    TownMapParam *param;
} PlaceSelectWork;

static void func_ov036_021ae21c(void *work);
static void func_ov036_021ae38c(void *work);

// Store the Battle Box's team when it was edited, and whether the player picked a Pokémon
static void func_ov036_021add94(void *data) {
    PCCallWork *work = data;
    u16 picked;

    if (work->param->mode == BOX2_MODE_BATTLE_BOX) {
        BattleBoxSave *battleBox = getBattleBox(GameData_GetSaveControl(work->gameData));

        copySelectedPkmToBattleBlk(battleBox, work->param->party);
    }
    if (work->param->mode == BOX2_MODE_BATTLE_BOX) {
        GFL_HeapFree(work->param->party);
    }
    switch (work->param->unk28) {
    case 0:
    default:
        picked = FALSE;
        break;
    case 1:
        picked = TRUE;
        break;
    }
    *work->result = picked;
    GFL_HeapFree(work->param);
}

BOOL s014F_CallPC(VM *vm, FieldScriptEnv *env) {
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
    param->unk1C = PokeDex_IsNationalObtained(pokedex);
    param->mode = mode;
    param->unk14 = 0;
    if (mode == BOX2_MODE_BATTLE_BOX) {
        BattleBoxSave *battleBox = getBattleBox(save);

        param->party = convertBoxedPokeSetToParty(battleBox, HEAPID_GAMEEVENT);
        param->unk14 = BattleBoxSave_CheckFlags(battleBox, 3);
    }
    callWork = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PCCallWork), FALSE, "scrcmd_proc_fld.c", 150);
    callWork->gameData = gameData;
    callWork->result = result;
    callWork->param = param;
    ScriptWork_CallEvent(work, EventFieldSubprocessCall_CreateWithCallback(gsys, field, OVERLAY_BOX2,
                                                                           &BOX2_PROC_FUNCTIONS, param,
                                                                           func_ov036_021add94, callWork));
    return TRUE;
}

BOOL s0205_CallGreetingPhraseInput(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);

    ScriptWork_CallEvent(work, EventPhraseInput_Create(gsys, field, NULL, 5, ScriptReadVar(vm, env)));
    return TRUE;
}

BOOL s0206_CallThanksPhraseInput(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);

    ScriptWork_CallEvent(work, EventPhraseInput_Create(gsys, field, NULL, 6, ScriptReadVar(vm, env)));
    return TRUE;
}

BOOL s02CA_CallHappyPhraseInput(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);

    ScriptWork_CallEvent(work, EventPhraseInput_Create(gsys, field, NULL, 15, ScriptReadVar(vm, env)));
    return TRUE;
}

// The game clear sequence takes over once the script ends
BOOL s0156_CallGameClear(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GSYS_GetField(gsys);
    ScriptWork_SetPostEvent(work, EventGameClear_Create(gsys, ScriptReadAny(vm, env)));
    VM_Halt(vm);
    return TRUE;
}

BOOL s0197_CGearPowerOn(VM *vm, FieldScriptEnv *env) {
    CGearSettings *cgear = func_02009918(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    u16 bootComm = ScriptReadAny(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    func_020098bc(cgear, 1);
    ScriptWork_CallEvent(work, EventCGearPowerOn_Create(gsys, bootComm));
    return TRUE;
}

BOOL s0152_CallGeonet(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(gsys);

    ScriptWork_CallEvent(work, EventFieldSubprocessTransition_Create(gsys, field, OVERLAY_GEONET,
                                                                     &GEONET_PROC_FUNCTIONS, GSYS_GetGameData(gsys)));
    return TRUE;
}

BOOL s014D_CallRecordSystem(VM *vm, FieldScriptEnv *env) {
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
    ScriptWork_CallEvent(work, func_ov033_02179868(gsys, option, result));
    return TRUE;
}

BOOL func_ov036_021ae0cc(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, func_ov033_02179dd4(gsys, ScriptReadVar(vm, env)));
    return TRUE;
}

BOOL s0155_CallXTransceiver(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u16 a1 = ScriptReadAny(vm, env);
    u16 a2 = VM_Read16(vm);
    XTransceiverParam *param =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(XTransceiverParam), TRUE, "scrcmd_proc_fld.c", 401);

    param->mode = 0;
    param->unk04 = a2;
    param->unk08 = a1;
    param->gameData = FieldScriptEnv_GetGameData(env);
    param->gsys = gsys;
    ScriptWork_CallEvent(work, EventFieldSubprocessCall_CreateWithCallback(gsys, field, OVERLAY_XTRANSCEIVER,
                                                                           &XTRANSCEIVER_PROC_FUNCTIONS, param,
                                                                           func_ov036_021ae38c, param));
    return TRUE;
}

BOOL func_ov036_021ae18c(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u16 a1;
    XTransceiverParam *param;

    getHollow_RivalBlk(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    a1 = ScriptReadAny(vm, env);
    param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(XTransceiverParam), TRUE, "scrcmd_proc_fld.c", 437);
    param->mode = 2;
    param->unk04 = a1;
    param->gameData = FieldScriptEnv_GetGameData(env);
    param->gsys = gsys;
    ScriptWork_CallEvent(work, EventFieldSubprocessCall_CreateWithCallback(gsys, field, OVERLAY_XTRANSCEIVER,
                                                                           &XTRANSCEIVER_PROC_FUNCTIONS, param,
                                                                           func_ov036_021ae38c, param));
    return TRUE;
}

static void func_ov036_021ae21c(void *data) {
    PlaceSelectWork *work = data;

    if (work->param->result == 2) {
        *work->picked = TRUE;
        *work->zoneId = work->param->zoneId;
    } else {
        *work->picked = FALSE;
    }
    GFL_HeapFree(work->param);
}

// Pick a place on the town map
BOOL s0284_CallPlaceSelect(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    PlayerState *playerState = GameData_GetPlayerState(gameData);
    u16 *picked = ScriptReadVar(vm, env);
    u16 *zoneId = ScriptReadVar(vm, env);
    TownMapParam *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(TownMapParam), TRUE, "scrcmd_proc_fld.c", 512);
    PlaceSelectWork *selectWork;

    param->unk00 = 0;
    param->gsys = gsys;
    param->zoneId = PlayerState_GetZoneID(playerState);
    param->escapeZoneId = GameData_GetEscapeRopeZone(gameData)->zoneId;
    selectWork = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PlaceSelectWork), TRUE, "scrcmd_proc_fld.c", 519);
    selectWork->picked = picked;
    selectWork->zoneId = zoneId;
    selectWork->param = param;
    ScriptWork_CallEvent(work, EventFieldSubprocessCall_CreateWithCallback(gsys, field, OVERLAY_TOWNMAP,
                                                                           &TOWNMAP_PROC_FUNCTIONS, param,
                                                                           func_ov036_021ae21c, selectWork));
    return TRUE;
}

BOOL func_ov036_021ae300(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(work, RivalNameEntry_Create(gsys, ScriptReadVar(vm, env)));
    return TRUE;
}

BOOL s0285_CallWordSetPokeNameInput(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u16 a1 = ScriptReadAny(vm, env);
    u16 a2 = ScriptReadAny(vm, env);

    ScriptWork_CallEvent(work, EventPokeNameWordSetInput_Create(gsys, a1, a2, ScriptReadVar(vm, env), wordSet));
    return TRUE;
}

static void func_ov036_021ae38c(void *work) {
    GFL_HeapFree(work);
}
