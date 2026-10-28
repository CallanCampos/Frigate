// The objects an actor hides in or behind. The name is the ROM's own, from GFL_HeapAllocate's file argument. Function
// names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_g3dobj.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"

#define HIDE_KIND_COUNT 3

typedef struct {
    u16 resGroup[HIDE_KIND_COUNT];
    u16 pad;
    FieldEffects *effects;
    u16 objIdx[HIDE_KIND_COUNT];
} FieldEffectHide;

// What a hiding object starts from
typedef struct {
    FieldEffectHide *effect;
    FieldG3DObjSystem *objSys;
    FieldActor *actor;
    u32 kind;
} HideTaskParams;

typedef struct {
    HideTaskParams params;
    FieldActorIdentity identity;
    u16 pad;
    u16 objIdx;
    u16 soundPlayed;
    u16 animDone;
    fx32 anmFrame;
} HideTask;

typedef struct {
    u16 modelId;
    u16 anmId;
} HideResIds;

static void Hide_LoadObjects(FieldEffectHide *effect);
static void Hide_FreeObjects(FieldEffectHide *effect);
static void HideTask_Init(FieldEffectTask *task, void *work);
static void HideTask_Exit(FieldEffectTask *task, void *work);
static void HideTask_UpdatePosition(FieldEffectTask *task, void *work);
static void HideTask_Draw(FieldEffectTask *task, void *work);

static const u32 HIDE_SE_IDS[HIDE_KIND_COUNT] = { SEQ_SE_TB_START, SEQ_SE_FLD_149, SEQ_SE_OCHIBA_01 };

static const HideResIds HIDE_RES_IDS[HIDE_KIND_COUNT] = { { 0x5e, 0x6d }, { 0x5d, 0x6c }, { 0x62, 0x86 } };

static const FieldEffectTaskVTable HIDE_TASK_VTABLE = {
    sizeof(HideTask), HideTask_Init, HideTask_Exit, HideTask_UpdatePosition, HideTask_Draw,
};

void *Hide_CreateResources(FieldEffects *effects, HeapID heapId) {
    FieldEffectHide *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectHide), TRUE, "fldeff_hide.c", 107);

    effect->effects = effects;
    Hide_LoadObjects(effect);
    return effect;
}

void Hide_DeleteResources(FieldEffects *effects, void *data) {
    Hide_FreeObjects(data);
    GFL_HeapFree(data);
}

static void Hide_LoadObjects(FieldEffectHide *effect) {
    FieldG3DObjResRequest request;
    int i = 0;
    const HideResIds *ids = HIDE_RES_IDS;
    ArcTool *arc = FieldEffects_GetArc(effect->effects);
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(effect->effects);

    for (; i < HIDE_KIND_COUNT; i++, ids++) {
        FieldG3DObjResRequest_Clear(&request);
        FieldG3DObjResRequest_SetModel(&request, arc, ids->modelId);
        FieldG3DObjResRequest_SetAnmArc(&request, arc);
        FieldG3DObjResRequest_AddAnm(&request, ids->anmId);
        effect->resGroup[i] = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);
        effect->objIdx[i] = FieldG3DObjSystem_AddObj(objSys, effect->resGroup[i], 0, NULL);
        FieldG3DObjSystem_SetObjectManualDraw(objSys, effect->objIdx[i], TRUE);
    }
}

static void Hide_FreeObjects(FieldEffectHide *effect) {
    int i = 0;
    FieldG3DObjSystem *objSys = FieldEffects_GetFieldMapData(effect->effects);

    for (; i < HIDE_KIND_COUNT; i++) {
        FieldG3DObjSystem_FreeObj(objSys, effect->objIdx[i]);
        FieldG3DObjSystem_FreeResGroup(objSys, effect->resGroup[i]);
    }
}

// A hiding object of the given kind for the actor
FieldEffectTask *HideTask_Start(FieldEffects *effects, FieldActor *actor, u32 kind) {
    HideTaskParams params;

    params.effect = FieldEffects_GetHandleData(effects, FLDEFF_HIDE);
    params.objSys = FieldEffects_GetFieldMapData(effects);
    params.actor = actor;
    params.kind = kind;
    return FieldEffects_TCBCreate(effects, &HIDE_TASK_VTABLE, NULL, 0, &params, 0);
}

// Starts the object's animation and its sound
void HideTask_RequestEnd(FieldEffectTask *task) {
    if (task != NULL) {
        HideTask *hideTask = FieldEffects_GetObjectWork(task);

        hideTask->soundPlayed = TRUE;
        GFL_SndSEPlay(HIDE_SE_IDS[hideTask->params.kind]);
    }
}

// Whether the object's animation has ended
u16 HideTask_IsFinished(FieldEffectTask *task) {
    if (task != NULL) {
        HideTask *hideTask = FieldEffects_GetObjectWork(task);

        return hideTask->animDone;
    }
    return TRUE;
}

static void HideTask_Init(FieldEffectTask *task, void *work) {
    HideTask *hideTask = work;
    HideTaskParams *params = FieldEffects_GetObjectParam(task);

    hideTask->params = *params;
    FldAct_GetIdentity(hideTask->params.actor, &hideTask->identity);
    hideTask->objIdx = hideTask->params.effect->objIdx[hideTask->params.kind];
}

static void HideTask_Exit(FieldEffectTask *task, void *work) {
    HideTask *hideTask = work;

    if (FldAct_CheckIdentity(hideTask->params.actor, &hideTask->identity) == TRUE) {
        FieldActorMove_SetHideEffect(hideTask->params.actor, NULL);
    }
}

static void HideTask_UpdatePosition(FieldEffectTask *task, void *work) {
    HideTask *hideTask = work;
    VecFx32 pos;
    fx32 height;

    if (FldAct_CheckIdentity(hideTask->params.actor, &hideTask->identity) == FALSE) {
        FieldEffects_DeleteObject(task);
        return;
    }
    CopyActorWPos(hideTask->params.actor, &pos);
    if (GetHeightFromMap(hideTask->params.actor, &pos, &height) == TRUE) {
        pos.y = height;
    }
    FieldEffects_SetObjectPos(task, &pos);
}

static void HideTask_Draw(FieldEffectTask *task, void *work) {
    HideTask *hideTask = work;
    VecFx32 pos;

    if (hideTask->soundPlayed == TRUE && hideTask->animDone == FALSE) {
        FieldG3DObjSystem_SetAnmFrame(hideTask->params.objSys, hideTask->objIdx, 0, hideTask->anmFrame);
        if (!FieldG3DObjSystem_StepObjAnm(hideTask->params.objSys, hideTask->objIdx, FX32_ONE)) {
            hideTask->animDone = TRUE;
        }
        hideTask->anmFrame = FieldG3DObjSystem_GetAnmFrame(hideTask->params.objSys, hideTask->objIdx, 0);
    } else {
        FieldG3DObjSystem_SetAnmFrame(hideTask->params.objSys, hideTask->objIdx, 0, hideTask->anmFrame);
    }
    FieldEffects_GetObjectPos(task, &pos);
    FieldG3DObjSystem_SetObjPosition(hideTask->params.objSys, hideTask->objIdx, &pos);
    FieldG3DObjSystem_DrawObj(hideTask->params.objSys, hideTask->objIdx);
}
