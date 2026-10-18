// The fallen leaves an actor kicks up, and the player sinking into a pile of leaves. The name is the ROM's own, from
// GFL_HeapAllocate's file argument ("ochiba" is fallen leaves). Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_g3dobj.h"
#include "field/field_player.h"
#include "field/field_script.h"
#include "field/scrcmd_ochiba.h"
#include "gfl/calctool.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "nitro/fx.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

typedef struct {
    FieldEffects *effects;
    u16 resGroup;
} FieldEffectOchiba;

// What a leaf effect starts from
typedef struct {
    FieldEffectOchiba *effect;
    FieldActor *actor;
    u16 dir;
    VecFx32 pos;
} OchibaTaskParams;

typedef struct {
    FieldEffectOchiba *effect;
    FieldG3DObjSystem *objSys;
    u16 objIdx;
    u16 dir;
} OchibaTask;

typedef struct {
    OchibaEffectArgs args;
    u32 unk8;
    u32 unkC;
    u16 frames;
    fx32 y;
    fx32 targetY;
    fx32 speed;
} LeafPileStuckWork;

static void func_ov036_021c9484(FieldEffectOchiba *effect);
static void func_ov036_021c94cc(FieldEffectOchiba *effect);
static void func_ov036_021c955c(FieldEffectTask *task, void *work);
static void func_ov036_021c95a4(FieldEffectTask *task, void *work);
static void func_ov036_021c95b0(FieldEffectTask *task, void *work);
static void func_ov036_021c95cc(FieldEffectTask *task, void *work);
static GameEvent *EventLeafPileStuck_Create(GameEvent *parent, GameSystem *gsys);
static GameEventReturnCode EventLeafPileStuck_Callback(GameEvent *event, u32 *state, void *data);

static const FieldEffectTaskVTable OCHIBA_TASK_VTABLE = {
    sizeof(OchibaTask), func_ov036_021c955c, func_ov036_021c95a4, func_ov036_021c95b0, func_ov036_021c95cc,
};

void *func_ov036_021c944c(FieldEffects *effects, HeapID heapId) {
    FieldEffectOchiba *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectOchiba), TRUE, "fldeff_ochiba.c", 109);

    effect->effects = effects;
    func_ov036_021c9484(effect);
    return effect;
}

void func_ov036_021c9470(FieldEffects *effects, void *data) {
    func_ov036_021c94cc(data);
    GFL_HeapFree(data);
}

static void func_ov036_021c9484(FieldEffectOchiba *effect) {
    FieldG3DObjResRequest request;
    FieldG3DObjSystem *objSys = func_ov036_021a3724(effect->effects);
    ArcTool *arc = FieldEffects_GetArc(effect->effects);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 0x60);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 0x84);
    effect->resGroup = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);
}

static void func_ov036_021c94cc(FieldEffectOchiba *effect) {
    FieldG3DObjSystem_FreeResGroup(func_ov036_021a3724(effect->effects), effect->resGroup);
}

// Leaves under the actor, which face the way it does
void func_ov036_021c94e0(void *effects, FieldActor *actor) {
    VecFx32 pos;
    OchibaTaskParams params;
    fx32 height;
    s16 gx = GetGPosX(actor);
    s16 gz = GetGPosZ(actor);
    u16 dir;

    CopyActorWPos(actor, &pos);
    ConvGXZToVector(gx, gz, &pos);
    height = 0;
    GetHeightFromMap(actor, &pos, &height);
    pos.y = height;
    dir = GetActorFaceDir(actor);
    params.effect = FieldEffects_GetHandleData(effects, 17);
    params.actor = actor;
    params.pos = pos;
    params.dir = dir;
    FieldEffects_TCBCreate(effects, &OCHIBA_TASK_VTABLE, &params.pos, 0, &params, 0);
}

static void func_ov036_021c955c(FieldEffectTask *task, void *work) {
    OchibaTask *ochibaTask = work;
    OchibaTaskParams *params = func_ov036_021a3ac8(task);
    FieldEffectOchiba *effect = params->effect;
    FieldG3DObjSystem *objSys;

    ochibaTask->effect = effect;
    objSys = func_ov036_021a3724(effect->effects);
    ochibaTask->dir = params->dir;
    ochibaTask->objSys = objSys;
    func_ov036_021a3ae8(task, &params->pos);
    ochibaTask->objIdx = FieldG3DObjSystem_AddObj(objSys, ochibaTask->effect->resGroup, 0, &params->pos);
    func_ov036_021c02b4(objSys, ochibaTask->objIdx, TRUE);
}

static void func_ov036_021c95a4(FieldEffectTask *task, void *work) {
    OchibaTask *ochibaTask = work;

    FieldG3DObjSystem_FreeObj(ochibaTask->objSys, ochibaTask->objIdx);
}

// End when the animation does
static void func_ov036_021c95b0(FieldEffectTask *task, void *work) {
    OchibaTask *ochibaTask = work;

    if (!FieldG3DObjSystem_StepObjAnm(ochibaTask->objSys, ochibaTask->objIdx, FX32_ONE)) {
        func_ov036_021a3a70(task);
    }
}

static void func_ov036_021c95cc(FieldEffectTask *task, void *work) {
    OchibaTask *ochibaTask = work;
    u16 rotations[4][3] = { { 0, 0xc000, 0 }, { 0, 0x4000, 0 }, { 0, 0, 0 }, { 0, 0x8000, 0 } };
    u16 *rot = rotations[ochibaTask->dir];
    SRTMatrix *srt = FieldG3DObjSystem_GetObjTransform(ochibaTask->objSys, ochibaTask->objIdx);

    MAT3_RotationEulerZYX(rot[0], rot[1], rot[2], &srt->rotation);
    func_ov036_021a3ad4(task, &srt->translation);
    FieldG3DObjSystem_DrawObj(ochibaTask->objSys, ochibaTask->objIdx);
}

BOOL s02AC_CallLeafPileStuck(VM *vm, FieldScriptEnv *env) {
    ScriptWork *scriptWork = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(scriptWork, EventLeafPileStuck_Create(NULL, FieldScriptEnv_GetGameSystem(env)));
    return TRUE;
}

static GameEvent *EventLeafPileStuck_Create(GameEvent *parent, GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    FieldPlayer *player = Field_GetPlayer(field);
    VecFx32 pos;
    GameEvent *event;
    LeafPileStuckWork *work;

    FieldPlayer_GetWPos(player, &pos);
    event = GameEvent_Create(gsys, parent, EventLeafPileStuck_Callback, sizeof(LeafPileStuckWork));
    work = GameEvent_GetData(event);
    work->args.field = field;
    work->args.player = player;
    work->frames = 0;
    work->y = pos.y;
    work->targetY = pos.y - FX32_CONST(10);
    work->speed = FX32_CONST(10) / 2;
    return event;
}

static GameEventReturnCode EventLeafPileStuck_Callback(GameEvent *event, u32 *state, void *data) {
    LeafPileStuckWork *work = data;
    VecFx32 pos;
    FieldActor *actor;
    u32 keys;

    switch (*state) {
    case 0:
        if (FieldPlayer_DeriveExState(work->args.player) == 0) {
            *state = 2;
        } else {
            *state = 1;
        }
        break;
    case 1:
        FieldPlayer_SetSpecialSeq(work->args.player, 1);
        func_ov036_0219a58c(work->args.player);
        *state = 2;
        break;
    case 2:
        func_ov036_021c97b8(&work->args);
        *state = 3;
    case 3:
        work->y -= work->speed;
        if (work->y <= work->targetY) {
            work->y = work->targetY;
            *state = 4;
        }
        FieldPlayer_GetWPos(work->args.player, &pos);
        pos.y = work->y;
        FieldPlayer_SetWPos(work->args.player, &pos);
        break;
    case 4:
        actor = FieldPlayer_GetActor(work->args.player);
        keys = GCTX_HIDGetHeldKeys();
        if (keys & PAD_KEY_UP) {
            FldAct_SetAcmd(actor, 0x24);
        }
        if (keys & PAD_KEY_DOWN) {
            FldAct_SetAcmd(actor, 0x25);
        }
        if (keys & PAD_KEY_LEFT) {
            FldAct_SetAcmd(actor, 0x26);
        }
        if (keys & PAD_KEY_RIGHT) {
            FldAct_SetAcmd(actor, 0x27);
        }
        if (work->frames++ > 40) {
            *state = 5;
        }
        break;
    case 5: {
        u32 dir = FieldPlayer_GetFaceDir(work->args.player);

        actor = FieldPlayer_GetActor(work->args.player);
        FldAct_SetAcmd(actor, dir + 0x30);
        CheckRecalcActorY(actor);
        *state = 6;
        break;
    }
    case 6:
        if (func_ov036_0219a870(work->args.player) == TRUE) {
            *state = 7;
        }
        break;
    case 7:
        return TRUE;
    }
    return FALSE;
}
