// The slash that Cut leaves, and the script commands of the League's lift, the beacons and the Pass Powers that share
// its range. The name is the ROM's own, from GFL_HeapAllocate's file argument; the commands' names are from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
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

#define IAIGIRI_ANM_COUNT 3

typedef struct {
    FieldEffects *effects;
    void *mdlRes;
    void *anmRes[IAIGIRI_ANM_COUNT];
    u32 unk14;
    u32 unk18;
} FieldEffectIaigiri;

typedef struct {
    FieldEffectIaigiri *effect;
    VecFx32 pos;
} IaigiriTaskParams;

typedef struct {
    FieldEffectIaigiri *effect;
    G3DActor *actor;
    void *anms[IAIGIRI_ANM_COUNT];
    G3DModel *mdl;
} IaigiriTask;

static void func_ov036_021c2e14(FieldEffectIaigiri *effect);
static void func_ov036_021c2e4c(FieldEffectIaigiri *effect);
static void func_ov036_021c2ef8(FieldEffectTask *task, void *data);
static void func_ov036_021c2f5c(FieldEffectTask *task, void *data);
static void func_ov036_021c2f80(FieldEffectTask *task, void *data);
static void func_ov036_021c2fb0(FieldEffectTask *task, void *data);

static const FieldEffectTaskVTable IAIGIRI_TASK_VTABLE;
static u32 IAIGIRI_ANM_FILE_IDS[IAIGIRI_ANM_COUNT];

void *func_ov036_021c2ddc(FieldEffects *effects, HeapID heapId) {
    FieldEffectIaigiri *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectIaigiri), TRUE, "fldeff_iaigiri.c", 102);

    effect->effects = effects;
    func_ov036_021c2e14(effect);
    return effect;
}

void func_ov036_021c2e00(FieldEffects *effects, void *effect) {
    func_ov036_021c2e4c(effect);
    GFL_HeapFree(effect);
}

static void func_ov036_021c2e14(FieldEffectIaigiri *effect) {
    int i;
    ArcTool *arc = FieldEffects_GetArc(effect->effects);

    effect->mdlRes = GFL_G3DSysReadArcToolResource(arc, 0x52);
    GFL_G3DResUploadTexData(effect->mdlRes);
    for (i = 0; i < IAIGIRI_ANM_COUNT; i++) {
        effect->anmRes[i] = GFL_G3DSysReadArcToolResource(arc, IAIGIRI_ANM_FILE_IDS[i]);
    }
}

static void func_ov036_021c2e4c(FieldEffectIaigiri *effect) {
    int i;

    for (i = 0; i < IAIGIRI_ANM_COUNT; i++) {
        GFL_G3DResFree(effect->anmRes[i]);
    }
    GFL_G3DResFreeTexData(effect->mdlRes);
    GFL_G3DResFree(effect->mdlRes);
}

void func_ov036_021c2e70(FieldActor *actor, FieldEffects *effects) {
    VecFx32 pos;
    IaigiriTaskParams params;

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
    FieldEffects_TCBCreate(effects, &IAIGIRI_TASK_VTABLE, NULL, 0, &params, 0);
}

static void func_ov036_021c2ef8(FieldEffectTask *task, void *data) {
    IaigiriTask *work = data;
    int i;
    IaigiriTaskParams *params = func_ov036_021a3ac8(task);

    work->effect = params->effect;
    func_ov036_021a3ae8(task, &params->pos);
    work->mdl = GFL_G3DMdlCreate(work->effect->mdlRes, 0, work->effect->mdlRes);
    for (i = 0; i < IAIGIRI_ANM_COUNT; i++) {
        work->anms[i] = GFL_G3DAnmCreate(work->mdl, work->effect->anmRes[i], 0);
    }
    work->actor = GFL_G3DActorCreate(work->mdl, work->anms, IAIGIRI_ANM_COUNT);
    for (i = 0; i < IAIGIRI_ANM_COUNT; i++) {
        GFL_G3DActorBindAnm(work->actor, i);
    }
}

static void func_ov036_021c2f5c(FieldEffectTask *task, void *data) {
    IaigiriTask *work = data;
    int i;

    for (i = 0; i < IAIGIRI_ANM_COUNT; i++) {
        GFL_G3DAnmFree(work->anms[i]);
    }
    GFL_G3DActorFree(work->actor);
    GFL_G3DMdlFree(work->mdl);
}

// Steps the animations, and ends the task once every one has reached its end
static void func_ov036_021c2f80(FieldEffectTask *task, void *data) {
    IaigiriTask *work = data;
    int i;
    BOOL done = TRUE;

    for (i = 0; i < IAIGIRI_ANM_COUNT; i++) {
        if (GFL_G3DActorStepAnmFrame(work->actor, i, FX32_ONE) == TRUE) {
            done = FALSE;
        }
    }
    if (done) {
        func_ov036_021a3a70(task);
    }
}

static void func_ov036_021c2fb0(FieldEffectTask *task, void *data) {
    IaigiriTask *work = data;
    SRTMatrix mtx = {
        { 0, 0, 0 },
        { FX32_ONE, FX32_ONE, FX32_ONE },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    };

    MTX_Identity33_(&mtx.rotation);
    func_ov036_021a3ad4(task, &mtx.translation);
    GFL_G3DSysDrawObjBBoxCull(work->actor, &mtx);
}

BOOL s01B1_CallLeagueLiftWarp(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(work, EventLeagueLiftWarp_Create(gsys, GSYS_GetField(gsys)));
    return TRUE;
}

BOOL func_ov036_021c3018(VM *vm, FieldScriptEnv *env) {
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
        void *data = PassPowerData_Create(HEAPID_TAIL(HEAPID_FIELDMAP));

        PassPower_Deactivate(*depletedId, data);
        PassPowerData_Free(data);
        *remains = PassPower_GetDepletedID() != PASS_POWER_NONE;
    } else {
        *remains = FALSE;
    }
    return FALSE;
}

BOOL func_ov036_021c315c(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, EventPassPowerFlash_Create(FieldScriptEnv_GetGameSystem(env)));
    return TRUE;
}

static const FieldEffectTaskVTable IAIGIRI_TASK_VTABLE = {
    sizeof(IaigiriTask), func_ov036_021c2ef8, func_ov036_021c2f5c, func_ov036_021c2f80, func_ov036_021c2fb0,
};

static u32 IAIGIRI_ANM_FILE_IDS[IAIGIRI_ANM_COUNT] = { 0x76, 0x89, 0x9c };
