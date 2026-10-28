// The slash that Cut leaves, and the script commands of the League's lift, the beacons and the Pass Powers that share
// its range. The ROM names it fldeff_iaigiri.c, by GFL_HeapAllocate's file argument; the commands' names are from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except s01DD_BroadcastGameBeacon and s01DF_CallPassPowerFlash,
// which are ours
#include "types.h"
#include "field/event_league_lift.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_move_tcb.h"
#include "field/field_pass_power.h"
#include "field/field_script.h"
#include "field/game_beacon_set.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "save/high_link.h"
#include "system/game_system.h"

#define CUT_ANM_COUNT 3

typedef struct {
    FieldEffects *effects;
    G3DResource *mdlRes;
    G3DResource *anmRes[CUT_ANM_COUNT];
    u32 unk14;
    u32 unk18;
} FieldEffectCut;

typedef struct {
    FieldEffectCut *effect;
    VecFx32 pos;
} CutTaskParams;

typedef struct {
    FieldEffectCut *effect;
    G3DActor *actor;
    G3DAnimation *anms[CUT_ANM_COUNT];
    G3DModel *mdl;
} CutTask;

static void FldeffCut_LoadResources(FieldEffectCut *effect);
static void FldeffCut_FreeResources(FieldEffectCut *effect);
static void FldeffCut_Init(FieldEffectTask *task, void *data);
static void FldeffCut_Exit(FieldEffectTask *task, void *data);
static void FldeffCut_Update(FieldEffectTask *task, void *data);
static void FldeffCut_Draw(FieldEffectTask *task, void *data);

static const FieldEffectTaskVTable CUT_TASK_VTABLE;
static u32 CUT_ANM_FILE_IDS[CUT_ANM_COUNT];

void *FldeffCut_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectCut *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectCut), TRUE, "fldeff_iaigiri.c", 102);

    effect->effects = effects;
    FldeffCut_LoadResources(effect);
    return effect;
}

void FldeffCut_Delete(FieldEffects *effects, void *effect) {
    FldeffCut_FreeResources(effect);
    GFL_HeapFree(effect);
}

static void FldeffCut_LoadResources(FieldEffectCut *effect) {
    int i;
    ArcTool *arc = FieldEffects_GetArc(effect->effects);

    effect->mdlRes = GFL_G3DSysReadArcToolResource(arc, 0x52);
    GFL_G3DResUploadTexData(effect->mdlRes);
    for (i = 0; i < CUT_ANM_COUNT; i++) {
        effect->anmRes[i] = GFL_G3DSysReadArcToolResource(arc, CUT_ANM_FILE_IDS[i]);
    }
}

static void FldeffCut_FreeResources(FieldEffectCut *effect) {
    int i;

    for (i = 0; i < CUT_ANM_COUNT; i++) {
        GFL_G3DResFree(effect->anmRes[i]);
    }
    GFL_G3DResFreeTexData(effect->mdlRes);
    GFL_G3DResFree(effect->mdlRes);
}

void FldeffCut_Start(FieldActor *actor, FieldEffects *effects) {
    VecFx32 pos;
    CutTaskParams params;

    CopyActorWPos(actor, &pos);
    switch (GetActorFaceDir(actor)) {
    case DIR_UP:
        pos.z -= FX32_CONST(16);
        break;
    case DIR_DOWN:
        pos.z += FX32_CONST(16);
        break;
    case DIR_LEFT:
        pos.x -= FX32_CONST(16);
        break;
    case DIR_RIGHT:
        pos.x += FX32_CONST(16);
        break;
    }
    params.effect = FieldEffects_GetHandleData(effects, 11);
    params.pos = pos;
    FieldEffects_TCBCreate(effects, &CUT_TASK_VTABLE, NULL, 0, &params, 0);
}

static void FldeffCut_Init(FieldEffectTask *task, void *data) {
    CutTask *work = data;
    int i;
    CutTaskParams *params = FieldEffects_GetObjectParam(task);

    work->effect = params->effect;
    FieldEffects_SetObjectPos(task, &params->pos);
    work->mdl = GFL_G3DMdlCreate(work->effect->mdlRes, 0, work->effect->mdlRes);
    for (i = 0; i < CUT_ANM_COUNT; i++) {
        work->anms[i] = GFL_G3DAnmCreate(work->mdl, work->effect->anmRes[i], 0);
    }
    work->actor = GFL_G3DActorCreate(work->mdl, work->anms, CUT_ANM_COUNT);
    for (i = 0; i < CUT_ANM_COUNT; i++) {
        GFL_G3DActorBindAnm(work->actor, i);
    }
}

static void FldeffCut_Exit(FieldEffectTask *task, void *data) {
    CutTask *work = data;
    int i;

    for (i = 0; i < CUT_ANM_COUNT; i++) {
        GFL_G3DAnmFree(work->anms[i]);
    }
    GFL_G3DActorFree(work->actor);
    GFL_G3DMdlFree(work->mdl);
}

// Steps the animations, and ends the task once every one has reached its end
static void FldeffCut_Update(FieldEffectTask *task, void *data) {
    CutTask *work = data;
    int i;
    BOOL done = TRUE;

    for (i = 0; i < CUT_ANM_COUNT; i++) {
        if (GFL_G3DActorStepAnmFrame(work->actor, i, FX32_ONE) == TRUE) {
            done = FALSE;
        }
    }
    if (done) {
        FieldEffects_DeleteObject(task);
    }
}

static void FldeffCut_Draw(FieldEffectTask *task, void *data) {
    CutTask *work = data;
    SRTMatrix mtx = {
        { 0, 0, 0 },
        { FX32_ONE, FX32_ONE, FX32_ONE },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    };

    MTX_Identity33_(&mtx.rotation);
    FieldEffects_GetObjectPos(task, &mtx.translation);
    GFL_G3DSysDrawObjBBoxCull(work->actor, &mtx);
}

BOOL s01B1_CallLeagueLiftWarp(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(work, EventLeagueLiftWarp_Create(gsys, GSYS_GetField(gsys)));
    return TRUE;
}

BOOL s01DD_BroadcastGameBeacon(VM *vm, FieldScriptEnv *env) {
    u16 command = ScriptReadAny(vm, env);
    u16 value = ScriptReadAny(vm, env);
    u16 extra = ScriptReadAny(vm, env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 zoneId;

    GSYS_GetGameData(gsys);
    GSYS_GetField(gsys);
    FieldScriptEnv_GetScriptWork(env);
    zoneId = GetScriptEnvZoneID(env);

    switch (command) {
    case 0:
        GameBeacon_BroadcastFoundItem(value);
        break;
    case 1:
        GameBeacon_BroadcastFieldMove(value);
        break;
    case 2:
        GameBeacon_BroadcastUnionRoomReception();
        break;
    case 3:
        GameBeacon_BroadcastBattleInstituteChallenge();
        break;
    case 4:
        GameBeacon_BroadcastBattleInstituteRank(value);
        break;
    case 5:
        GameBeacon_BroadcastPokeTransfer();
        break;
    case 6:
        GameBeacon_BroadcastBattleSubwayChallenge();
        break;
    case 7:
        GameBeacon_BroadcastBattleSubwayWinStreak(value);
        break;
    case 8:
        GameBeacon_BroadcastBattleSubwayTrophy();
        break;
    case 9:
        GameBeacon_BroadcastTournamentChallenge(value);
        break;
    case 10:
        GameBeacon_BroadcastTournamentWin(value, extra);
        break;
    case 11:
        GameBeacon_BroadcastTournamentLoss(value, extra);
        break;
    case 12:
        GameBeacon_BroadcastAreaChallenge(value, zoneId);
        break;
    case 13:
        GameBeacon_BroadcastAreaCleared(value, zoneId);
        break;
    case 14:
        GameBeacon_BroadcastPokestarShooting(value);
        break;
    }
    return FALSE;
}

BOOL s01DE_PassPowerPopDepletedID(VM *vm, FieldScriptEnv *env) {
    u16 *depletedId = ScriptReadVar(vm, env);
    u16 *remains = ScriptReadVar(vm, env);

    *depletedId = PassPower_GetDepletedID();
    if (*depletedId != PASS_POWER_NONE) {
        PassPowerData *data = PassPowerData_Create(HEAPID_TAIL(HEAPID_FIELDMAP));

        PassPower_Deactivate(*depletedId, data);
        PassPowerData_Free(data);
        *remains = PassPower_GetDepletedID() != PASS_POWER_NONE;
    } else {
        *remains = FALSE;
    }
    return FALSE;
}

BOOL s01DF_CallPassPowerFlash(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, EventPassPowerFlash_Create(FieldScriptEnv_GetGameSystem(env)));
    return TRUE;
}

static const FieldEffectTaskVTable CUT_TASK_VTABLE = {
    sizeof(CutTask), FldeffCut_Init, FldeffCut_Exit, FldeffCut_Update, FldeffCut_Draw,
};

static u32 CUT_ANM_FILE_IDS[CUT_ANM_COUNT] = { 0x76, 0x89, 0x9c };
