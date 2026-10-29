#include "types.h"
#include "field/entree_scripts.h"
#include "field/event_funfest_mission.h"
#include "field/event_mapchange.h"
#include "field/event_sound.h"
#include "field/fest_mission_gimmick.h"
#include "field/festival.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/funfest_scripts.h"
#include "field/game_beacon_set.h"
#include "save/high_link.h"
#include "save/save_control.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL Scrcmd_Funfest_FunfestBGMReturn(VM *vm, FieldScriptEnv *env) {
    ScriptWork *scriptWork = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    FesGimmick *gimmick = Field_GetFesGimmick(GSYS_GetField(gsys));
    LinkFestival *festival = GSYS_GetLinkFestival(gsys);
    u32 bgm;

    if (getStatusOfFesMission(festival) == 0) {
        return TRUE;
    }
    FesGimmick_EndMission(gimmick);
    bgm = LinkFestival_GetNormalChangeBGMID(festival);
    if (bgm != 0xffffffff) {
        ScriptWork_CallEvent(scriptWork, EventBGMPlay_Create(gsys, bgm));
    }
    return TRUE;
}

BOOL Scrcmd_Funfest_FunfestMissionBroadcast(VM *vm, FieldScriptEnv *env) {
    u8 type;
    u16 value;

    FieldScriptEnv_GetScriptWork(env);
    FieldScriptEnv_GetGameSystem(env);
    type = ScriptReadAny(vm, env);
    value = ScriptReadAny(vm, env);
    GameBeacon_BroadcastMission(type, value);
    return TRUE;
}

BOOL Scrcmd_Funfest_FunfestActorDelete(VM *vm, FieldScriptEnv *env) {
    ScriptWork *scriptWork = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u16 zoneId = GetScriptEnvZoneID(env);
    u8 actorIndex = GetActorUID(ScriptWork_GetParentActor(scriptWork)) - 0xec;
    FesGimmick *gimmick = Field_GetFesGimmick(field);

    if (!FesGimmick_IsCurrent(gimmick, 5)) {
        return TRUE;
    }
    DeleteFunfestActor(Field_GetFesGimmick(field), zoneId, actorIndex);
    return TRUE;
}

BOOL Scrcmd_Funfest_FunfestGetGenericInfo(VM *vm, FieldScriptEnv *env) {
    ScriptWork *scriptWork = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    FesGimmick *gimmick = Field_GetFesGimmick(GSYS_GetField(gsys));
    u16 param = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    if (!FesGimmick_IsCurrent(gimmick, 5)) {
        *result = 0;
        return TRUE;
    }
    *result = GetActorUserParam(ScriptWork_GetParentActor(scriptWork), param);
    return TRUE;
}

BOOL Scrcmd_Funfest_FunfestGetItemExchangeInfo(VM *vm, FieldScriptEnv *env) {
    ScriptWork *scriptWork = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    FesGimmick *gimmick = Field_GetFesGimmick(GSYS_GetField(gsys));
    u16 arg0 = ScriptReadAny(vm, env);
    u16 arg1 = ScriptReadAny(vm, env);
    u16 *out0 = ScriptReadVar(vm, env);
    u16 *out1 = ScriptReadVar(vm, env);

    if (!FesGimmick_IsCurrent(gimmick, 5)) {
        *out0 = *out1 = 0x11;
        return TRUE;
    }
    FesMissionGimmick_GetItemExchangeInfo(gimmick, ScriptWork_GetParentActor(scriptWork), arg0, arg1, out0, out1);
    return TRUE;
}

BOOL Scrcmd_Funfest_FunfestGetItemSaleInfo(VM *vm, FieldScriptEnv *env) {
    ScriptWork *scriptWork = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    FesGimmick *gimmick = Field_GetFesGimmick(GSYS_GetField(gsys));
    u16 *out0 = ScriptReadVar(vm, env);
    u16 *out1 = ScriptReadVar(vm, env);

    if (!FesGimmick_IsCurrent(gimmick, 5)) {
        *out0 = 0x11;
        *out1 = 0x270f;
        return TRUE;
    }
    FesMissionGimmick_GetItemSaleInfo(gimmick, ScriptWork_GetParentActor(scriptWork), out0, out1);
    return TRUE;
}

BOOL Scrcmd_Funfest_FunfestGetPokemonQuizInfo(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    FesGimmick *gimmick;
    u16 *out0;
    u16 *out1;
    u16 *out2;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gimmick = Field_GetFesGimmick(GSYS_GetField(gsys));
    out0 = ScriptReadVar(vm, env);
    out1 = ScriptReadVar(vm, env);
    out2 = ScriptReadVar(vm, env);

    if (!FesGimmick_IsCurrent(gimmick, 5)) {
        *out0 = 1;
        *out1 = *out2 = 0;
        return TRUE;
    }
    FesMissionGimmick_MakePokemonQuiz(gimmick, out0, out1, out2);
    return TRUE;
}

BOOL Scrcmd_Funfest_FunfestGetPokemonQuizSpecies(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    FesGimmick *gimmick;
    u16 index;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gimmick = Field_GetFesGimmick(GSYS_GetField(gsys));
    index = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);

    if (!FesGimmick_IsCurrent(gimmick, 5)) {
        *result = 0x1f8 + index;
        return TRUE;
    }
    *result = FesMissionGimmick_GetQuizSpecies(gimmick, (u8)index);
    return TRUE;
}

BOOL Scrcmd_Funfest_FunfestGetPokemonQuizBogusSpecies(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    FesGimmick *gimmick;
    u16 index;
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gimmick = Field_GetFesGimmick(GSYS_GetField(gsys));
    index = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);

    if (!FesGimmick_IsCurrent(gimmick, 5)) {
        *result = 0x1f8 + index;
        return TRUE;
    }
    *result = FesMissionGimmick_GetQuizChoice(gimmick, (u8)index);
    return TRUE;
}

BOOL func_ov033_021772c8(VM *vm, FieldScriptEnv *env) {
    u16 *result;
    FestMissionConfig *missionCfg;

    result = ScriptReadVar(vm, env);
    missionCfg = GetFestMissionCfg(GSYS_GetLinkFestival(FieldScriptEnv_GetGameSystem(env)));
    if (isFesMissionAvailable(missionCfg)) {
        *result = 2;
    } else {
        *result = 0;
    }
    return FALSE;
}

BOOL func_ov033_021772f4(VM *vm, FieldScriptEnv *env) {
    u16 *result;
    HighLinkSave *save;

    result = ScriptReadVar(vm, env);
    save = getHighLinkBlockAddress(GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))));
    if (HighLinkSave_GetPassPower(save, 0) != 0x30) {
        *result = 1;
    } else {
        *result = 0;
    }
    return FALSE;
}

BOOL Scrcmd_Funfest_FunfestMissionStart(VM *vm, FieldScriptEnv *env) {
    ScriptWork *scriptWork = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(scriptWork, func_ov033_02176d88(gsys));
    return TRUE;
}

BOOL Scrcmd_Funfest_CallEntralinkWarpOut(VM *vm, FieldScriptEnv *env) {
    ScriptWork *scriptWork = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(scriptWork, EventEntralinkWarp_CreateOut(gsys));
    return TRUE;
}
