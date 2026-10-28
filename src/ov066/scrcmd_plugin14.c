#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_script.h"
#include "field/field_task.h"
#include "field/gimmick_aspertia_city.h"
#include "field/gimmick_dragonspiral_tower_9.h"
#include "field/gimmick_giant_chasm_6.h"
#include "field/gimmick_league_center.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/sound.h"
#include "nitro/hw.h"
#include "system/game_system.h"
#include "system/vm.h"

// Script plugin 14 (overlay 66), commands from 1000, of zones with story events: the release of the Dark Stone's power
// at the top of Dragonspiral Tower (zone 213, whose gimmick is overlay 132), the Giant Chasm's innermost cave (zone
// 604, overlay 116), and zones 139 and 427, whose gimmicks are overlays 115 and 112

typedef struct {
    u32 seq;
    s32 volume;
} Plugin14SeqFadeOut;

static BOOL Scrcmd_Plugin14_ShowGC6Model(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickGC6_ShowModel01(gsys, VM_Read16(vm));
    return FALSE;
}

static BOOL Scrcmd_Plugin14_PlayGC6Model(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickGC6_PlayModel01(gsys, VM_Read16(vm));
    return FALSE;
}

static BOOL Scrcmd_Plugin14_StartActorHover(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickGC6_CallStartHover(gsys, VM_Read16(vm));
    return FALSE;
}

static BOOL Scrcmd_Plugin14_PlayGC6Model23(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GimmickGC6_PlayModel23(gsys, ScriptReadAny(vm, env) == TRUE);
    return FALSE;
}

static BOOL Scrcmd_Plugin14_HideGC6Model3(VM *vm, FieldScriptEnv *env) {
    GimmickGC6_HideModel3(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

// Moves an actor by whole units along x and z
static BOOL Scrcmd_Plugin14_ShiftActor(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u32 actorId = ScriptReadAny(vm, env);
    s16 dx = ScriptReadAny(vm, env);
    s16 dz = ScriptReadAny(vm, env);
    FieldActor *actor = FindFieldActor(GetScrEnvActorSys(env), actorId);

    if (actor == NULL) {
        return FALSE;
    } else {
        VecFx32 pos = { 0, 0, 0 };

        CopyActorWPos(actor, &pos);
        pos.x += dx * FX32_ONE;
        pos.z += dz * FX32_ONE;
        SetActorWPosValue(actor, &pos);
    }
    return FALSE;
}

static BOOL Scrcmd_Plugin14_ResetLCDToBlack(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u32 enabledA = GFL_BGSysGetEnabledBGsA();
    u32 enabledB = GFL_BGSysGetEnabledBGsB();

    FieldG2D_SetLCDConfig();
    GFL_BGSysSetEnabledBGsA(enabledA);
    GFL_BGSysSetEnabledBGsB(enabledB);
    FieldG2D_Prepare3DSurface(field);
    GX_SetMasterBrightness(0);
    G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, 0x3d, -16);
    GXS_SetMasterBrightness(0);
    G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, 0x3f, -16);
    return FALSE;
}

static BOOL Scrcmd_Plugin14_StartAspertiaSeasonAnimations(VM *vm, FieldScriptEnv *env) {
    AspertiaCity_PlaySeasonModel(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

static BOOL Scrcmd_Plugin14_StartLeagueCenterAnimations(VM *vm, FieldScriptEnv *env) {
    LeagueCenter_PlayModel(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

static BOOL Plugin14_SeqFadeOutTask(void *data);

// Fades a sound sequence out and stops it
static BOOL Scrcmd_Plugin14_FadeOutSeq(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    HeapID heapId = Field_GetHeapID(field);
    u16 seq = ScriptReadAny(vm, env);
    FieldTaskManager *mgr = Field_GetTaskManager(field);
    FieldTask *task = FieldTask_Create(heapId, sizeof(Plugin14SeqFadeOut), Plugin14_SeqFadeOutTask);
    Plugin14SeqFadeOut *fade = FieldTask_GetData(task);

    fade->seq = seq;
    fade->volume = 127;
    FieldTaskManager_AddTask(mgr, task, 0);
    return FALSE;
}

static BOOL Plugin14_SeqFadeOutTask(void *data) {
    Plugin14SeqFadeOut *fade = data;
    s32 volume = fade->volume - 2;

    fade->volume = volume;
    if (volume < 0) {
        GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(fade->seq));
        return TRUE;
    }
    GFL_SndPlayerSetVolume(GFL_SndSeqGetPlayerIndex(fade->seq), volume);
    return FALSE;
}

static BOOL Scrcmd_Plugin14_LoadAspertiaBG3Picture(VM *vm, FieldScriptEnv *env) {
    AspertiaCity_RequestBG(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

static BOOL Scrcmd_Plugin14_CallDT9ReleaseEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = GimmickDT9_CreateReleaseEvent(FieldScriptEnv_GetGameSystem(env));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static BOOL Scrcmd_Plugin14_CallDT9ThrowEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u16 actorId = ScriptReadAny(vm, env);
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    u16 a4 = ScriptReadAny(vm, env);
    u16 a5 = ScriptReadAny(vm, env);
    FieldActor *actor = FindFieldActor(Field_GetActorSystem(field), actorId);
    VecFx32 from;
    VecFx32 to;
    GameEvent *event;

    if (actor != NULL) {
        CopyActorWPos(actor, &from);
    }
    to.x = x * (16 * FX32_ONE) + 8 * FX32_ONE;
    to.y = y * (FX32_ONE / 2) * 16;
    to.z = z * (16 * FX32_ONE) + 8 * FX32_ONE;
    event = GimmickDT9_CreateThrowEvent(gsys, &from, &to, a4 * (FX32_ONE / 2) * 16, a5);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static BOOL Scrcmd_Plugin14_PlayDT9Thrown(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 x = ScriptReadAny(vm, env);
    u16 y = ScriptReadAny(vm, env);
    u16 z = ScriptReadAny(vm, env);
    VecFx32 pos;

    pos.x = x * (16 * FX32_ONE) + 8 * FX32_ONE;
    pos.y = y * (FX32_ONE / 2) * 16;
    pos.z = z * (16 * FX32_ONE) + 8 * FX32_ONE;
    GimmickDT9_PlayThrown(gsys, &pos);
    return FALSE;
}

static BOOL Scrcmd_Plugin14_CallDT9WaitThrownDoneEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = GimmickDT9_CreateWaitThrownDoneEvent(FieldScriptEnv_GetGameSystem(env));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static BOOL Scrcmd_Plugin14_CallDT9WaitFifthFrameEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = GimmickDT9_CreateWaitFifthFrameEvent(FieldScriptEnv_GetGameSystem(env));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

const FieldScriptCommand PLUGIN_14_SCRIPT_COMMANDS[] = {
    Scrcmd_Plugin14_StartAspertiaSeasonAnimations,
    Scrcmd_Plugin14_ShowGC6Model,
    Scrcmd_Plugin14_PlayGC6Model,
    Scrcmd_Plugin14_StartActorHover,
    Scrcmd_Plugin14_PlayGC6Model23,
    Scrcmd_Plugin14_HideGC6Model3,
    Scrcmd_Plugin14_ShiftActor,
    Scrcmd_Plugin14_ResetLCDToBlack,
    NULL,
    Scrcmd_Plugin14_StartLeagueCenterAnimations,
    Scrcmd_Plugin14_FadeOutSeq,
    Scrcmd_Plugin14_LoadAspertiaBG3Picture,
    Scrcmd_Plugin14_CallDT9ReleaseEvent,
    Scrcmd_Plugin14_CallDT9ThrowEvent,
    Scrcmd_Plugin14_PlayDT9Thrown,
    Scrcmd_Plugin14_CallDT9WaitThrownDoneEvent,
    Scrcmd_Plugin14_CallDT9WaitFifthFrameEvent,
    (FieldScriptCommand)0xFFFFFFFF,
};
