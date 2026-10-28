// The fallen leaves an actor kicks up, and the player sinking into a pile of leaves. The ROM names it fldeff_ochiba.c,
// by GFL_HeapAllocate's file argument ("ochiba" is fallen leaves). Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except the FldeffOchiba_ functions, which are ours
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_g3dobj.h"
#include "field/field_player.h"
#include "field/field_script.h"
#include "field/scrcmd_leaves.h"
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
} FieldEffectLeaves;

// What a leaf effect starts from
typedef struct {
    FieldEffectLeaves *effect;
    FieldActor *actor;
    u16 dir;
    VecFx32 pos;
} LeavesTaskParams;

typedef struct {
    FieldEffectLeaves *effect;
    FieldG3DObjSystem *objSys;
    u16 objIdx;
    u16 dir;
} LeavesTask;

typedef struct {
    LeavesEffectArgs args;
    u32 unk8;
    u32 unkC;
    u16 frames;
    fx32 y;
    fx32 targetY;
    fx32 speed;
} LeafPileStuckWork;

static void FldeffLeaves_LoadResource(FieldEffectLeaves *effect);
static void FldeffLeaves_FreeResource(FieldEffectLeaves *effect);
static void FldeffLeaves_Init(FieldEffectTask *task, void *work);
static void FldeffLeaves_Exit(FieldEffectTask *task, void *work);
static void FldeffLeaves_Update(FieldEffectTask *task, void *work);
static void FldeffLeaves_Draw(FieldEffectTask *task, void *work);
static GameEvent *EventLeafPileStuck_Create(GameEvent *parent, GameSystem *gsys);
static GameEventReturnCode EventLeafPileStuck_Callback(GameEvent *event, u32 *state, void *data);

static const FieldEffectTaskVTable LEAVES_TASK_VTABLE = {
    sizeof(LeavesTask), FldeffLeaves_Init, FldeffLeaves_Exit, FldeffLeaves_Update, FldeffLeaves_Draw,
};

void *FldeffLeaves_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectLeaves *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectLeaves), TRUE, "fldeff_ochiba.c", 109);

    effect->effects = effects;
    FldeffLeaves_LoadResource(effect);
    return effect;
}

void FldeffLeaves_Delete(FieldEffects *effects, void *data) {
    FldeffLeaves_FreeResource(data);
    GFL_HeapFree(data);
}

static void FldeffLeaves_LoadResource(FieldEffectLeaves *effect) {
    FieldG3DObjResRequest request;
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(effect->effects);
    ArcTool *arc = FieldEffects_GetArc(effect->effects);

    FieldG3DObjResRequest_Clear(&request);
    FieldG3DObjResRequest_SetModel(&request, arc, 0x60);
    FieldG3DObjResRequest_SetAnmArc(&request, arc);
    FieldG3DObjResRequest_AddAnm(&request, 0x84);
    effect->resGroup = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);
}

static void FldeffLeaves_FreeResource(FieldEffectLeaves *effect) {
    FieldG3DObjSystem_FreeResGroup(FieldEffects_GetFieldMapData(effect->effects), effect->resGroup);
}

// Leaves under the actor, which face the way it does
void FldeffLeaves_Start(FieldEffects *effects, FieldActor *actor) {
    VecFx32 pos;
    LeavesTaskParams params;
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
    FieldEffects_TCBCreate(effects, &LEAVES_TASK_VTABLE, &params.pos, 0, &params, 0);
}

static void FldeffLeaves_Init(FieldEffectTask *task, void *work) {
    LeavesTask *leavesTask = work;
    LeavesTaskParams *params = FieldEffects_GetObjectParam(task);
    FieldEffectLeaves *effect = params->effect;
    FieldG3DObjSystem *objSys;

    leavesTask->effect = effect;
    objSys = FieldEffects_GetFieldMapData(effect->effects);
    leavesTask->dir = params->dir;
    leavesTask->objSys = objSys;
    FieldEffects_SetObjectPos(task, &params->pos);
    leavesTask->objIdx = FieldG3DObjSystem_AddObj(objSys, leavesTask->effect->resGroup, 0, &params->pos);
    FieldG3DObjSystem_SetObjectManualDraw(objSys, leavesTask->objIdx, TRUE);
}

static void FldeffLeaves_Exit(FieldEffectTask *task, void *work) {
    LeavesTask *leavesTask = work;

    FieldG3DObjSystem_FreeObj(leavesTask->objSys, leavesTask->objIdx);
}

// End when the animation does
static void FldeffLeaves_Update(FieldEffectTask *task, void *work) {
    LeavesTask *leavesTask = work;

    if (!FieldG3DObjSystem_StepObjAnm(leavesTask->objSys, leavesTask->objIdx, FX32_ONE)) {
        FieldEffects_DeleteObject(task);
    }
}

static void FldeffLeaves_Draw(FieldEffectTask *task, void *work) {
    LeavesTask *leavesTask = work;
    u16 rotations[4][3] = { { 0, 0xc000, 0 }, { 0, 0x4000, 0 }, { 0, 0, 0 }, { 0, 0x8000, 0 } };
    u16 *rot = rotations[leavesTask->dir];
    SRTMatrix *srt = FieldG3DObjSystem_GetObjTransform(leavesTask->objSys, leavesTask->objIdx);

    MAT3_RotationEulerZYX(rot[0], rot[1], rot[2], &srt->rotation);
    FieldEffects_GetObjectPos(task, &srt->translation);
    FieldG3DObjSystem_DrawObj(leavesTask->objSys, leavesTask->objIdx);
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
        FieldPlayer_FinishFormChange(work->args.player);
        *state = 2;
        break;
    case 2:
        LeavesEffect_Start(&work->args);
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
        if (FieldPlayer_CheckAnimePaused(work->args.player) == TRUE) {
            *state = 7;
        }
        break;
    case 7:
        return TRUE;
    }
    return FALSE;
}
