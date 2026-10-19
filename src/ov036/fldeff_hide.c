// The objects an actor hides in or behind. The name is the ROM's own, from GFL_HeapAllocate's file argument. Function
// names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
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

static void func_ov036_021c6720(FieldEffectHide *effect);
static void func_ov036_021c679c(FieldEffectHide *effect);
static void func_ov036_021c6834(FieldEffectTask *task, void *work);
static void func_ov036_021c6860(FieldEffectTask *task, void *work);
static void func_ov036_021c687c(FieldEffectTask *task, void *work);
static void func_ov036_021c68c4(FieldEffectTask *task, void *work);

static const u32 HIDE_SE_IDS[HIDE_KIND_COUNT] = { 0x570, 0x7a5, 0x87e };

static const HideResIds HIDE_RES_IDS[HIDE_KIND_COUNT] = { { 0x5e, 0x6d }, { 0x5d, 0x6c }, { 0x62, 0x86 } };

static const FieldEffectTaskVTable HIDE_TASK_VTABLE = {
    sizeof(HideTask), func_ov036_021c6834, func_ov036_021c6860, func_ov036_021c687c, func_ov036_021c68c4,
};

void *func_ov036_021c66e8(FieldEffects *effects, HeapID heapId) {
    FieldEffectHide *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectHide), TRUE, "fldeff_hide.c", 107);

    effect->effects = effects;
    func_ov036_021c6720(effect);
    return effect;
}

void func_ov036_021c670c(FieldEffects *effects, void *data) {
    func_ov036_021c679c(data);
    GFL_HeapFree(data);
}

static void func_ov036_021c6720(FieldEffectHide *effect) {
    FieldG3DObjResRequest request;
    int i = 0;
    const HideResIds *ids = HIDE_RES_IDS;
    ArcTool *arc = FieldEffects_GetArc(effect->effects);
    FieldG3DObjSystem *objSys = func_ov036_021a3724(effect->effects);

    for (; i < HIDE_KIND_COUNT; i++, ids++) {
        FieldG3DObjResRequest_Clear(&request);
        FieldG3DObjResRequest_SetModel(&request, arc, ids->modelId);
        FieldG3DObjResRequest_SetAnmArc(&request, arc);
        FieldG3DObjResRequest_AddAnm(&request, ids->anmId);
        effect->resGroup[i] = FieldG3DObjSystem_AddResGroup(objSys, &request, FALSE);
        effect->objIdx[i] = FieldG3DObjSystem_AddObj(objSys, effect->resGroup[i], 0, NULL);
        func_ov036_021c02b4(objSys, effect->objIdx[i], TRUE);
    }
}

static void func_ov036_021c679c(FieldEffectHide *effect) {
    int i = 0;
    FieldG3DObjSystem *objSys = func_ov036_021a3724(effect->effects);

    for (; i < HIDE_KIND_COUNT; i++) {
        FieldG3DObjSystem_FreeObj(objSys, effect->objIdx[i]);
        FieldG3DObjSystem_FreeResGroup(objSys, effect->resGroup[i]);
    }
}

// A hiding object of the given kind for the actor
FieldEffectTask *func_ov036_021c67c8(FieldEffects *effects, FieldActor *actor, u32 kind) {
    HideTaskParams params;

    params.effect = FieldEffects_GetHandleData(effects, 14);
    params.objSys = func_ov036_021a3724(effects);
    params.actor = actor;
    params.kind = kind;
    return FieldEffects_TCBCreate(effects, &HIDE_TASK_VTABLE, NULL, 0, &params, 0);
}

// Starts the object's animation and its sound
void func_ov036_021c6800(FieldEffectTask *task) {
    if (task != NULL) {
        HideTask *hideTask = func_ov036_021a3afc(task);

        hideTask->soundPlayed = TRUE;
        GFL_SndSEPlay(HIDE_SE_IDS[hideTask->params.kind]);
    }
}

// Whether the object's animation has ended
u16 func_ov036_021c6820(FieldEffectTask *task) {
    if (task != NULL) {
        HideTask *hideTask = func_ov036_021a3afc(task);

        return hideTask->animDone;
    }
    return TRUE;
}

static void func_ov036_021c6834(FieldEffectTask *task, void *work) {
    HideTask *hideTask = work;
    HideTaskParams *params = func_ov036_021a3ac8(task);

    hideTask->params = *params;
    func_ov012_02167d88(hideTask->params.actor, &hideTask->identity);
    hideTask->objIdx = hideTask->params.effect->objIdx[hideTask->params.kind];
}

static void func_ov036_021c6860(FieldEffectTask *task, void *work) {
    HideTask *hideTask = work;

    if (func_ov012_02167da8(hideTask->params.actor, &hideTask->identity) == TRUE) {
        func_ov036_02193110(hideTask->params.actor, NULL);
    }
}

static void func_ov036_021c687c(FieldEffectTask *task, void *work) {
    HideTask *hideTask = work;
    VecFx32 pos;
    fx32 height;

    if (func_ov012_02167da8(hideTask->params.actor, &hideTask->identity) == FALSE) {
        func_ov036_021a3a70(task);
        return;
    }
    CopyActorWPos(hideTask->params.actor, &pos);
    if (GetHeightFromMap(hideTask->params.actor, &pos, &height) == TRUE) {
        pos.y = height;
    }
    func_ov036_021a3ae8(task, &pos);
}

static void func_ov036_021c68c4(FieldEffectTask *task, void *work) {
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
    func_ov036_021a3ad4(task, &pos);
    FieldG3DObjSystem_SetObjPosition(hideTask->params.objSys, hideTask->objIdx, &pos);
    FieldG3DObjSystem_DrawObj(hideTask->params.objSys, hideTask->objIdx);
}
