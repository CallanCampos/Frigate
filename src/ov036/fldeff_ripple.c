// The ripple that spreads where an actor steps in shallow water. The name is the ROM's own, from GFL_HeapAllocate's
// file argument.
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "gfl/billboard_actor.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

typedef struct {
    FieldEffects *effects;
    BillboardActorSys *billboardActor;
    u16 matIndex;
} FieldEffectRipple;

// What a ripple starts from
typedef struct {
    FieldEffectRipple *ripple;
    VecFx32 pos;
} RippleTaskParams;

typedef struct {
    FieldEffectRipple *ripple;
    u16 actorIndex;
} RippleTask;

static void FieldEffectRipple_LoadMaterial(FieldEffectRipple *ripple);
static void FieldEffectRipple_FreeMaterial(FieldEffectRipple *ripple);
static void RippleTask_Init(FieldEffectTask *task, void *work);
static void RippleTask_Free(FieldEffectTask *task, void *work);
static void RippleTask_Update(FieldEffectTask *task, void *work);
static void RippleTask_Draw(FieldEffectTask *task, void *work);

static const FieldEffectTaskVTable RIPPLE_TASK_VTABLE = {
    sizeof(RippleTask), RippleTask_Init, RippleTask_Free, RippleTask_Update, RippleTask_Draw,
};

static const BillboardActorAnimStep RIPPLE_ANIM_SPREAD[] = {
    { 0, 0, 0, 3 }, { 1, 0, 0, 3 }, { 2, 0, 0, 3 }, { 3, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *RIPPLE_ANIMS[] = { RIPPLE_ANIM_SPREAD };

void *FldeffRipple_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectRipple *ripple = GFL_HeapAllocate(heapId, sizeof(FieldEffectRipple), TRUE, "fldeff_ripple.c", 88);

    ripple->effects = effects;
    ripple->billboardActor = Field_GetWildEffectBillboardActor(FieldEffects_GetField(effects));
    FieldEffectRipple_LoadMaterial(ripple);
    return ripple;
}

void FldeffRipple_Delete(FieldEffects *effects, void *data) {
    FieldEffectRipple_FreeMaterial(data);
    GFL_HeapFree(data);
}

static void FieldEffectRipple_LoadMaterial(FieldEffectRipple *ripple) {
    BillboardActorMatRequest request;
    ArcTool *arc = FieldEffects_GetArc(ripple->effects);

    request.format = 2;
    request.size = 19;
    request.faceWidth = 16;
    request.faceHeight = 16;
    request.mode = BILLBOARD_ACTOR_LOAD_TRIM;
    request.texResource = GFL_G3DSysReadArcToolResource(arc, 46);
    FieldEffects_ApplyLuminanceTable(ripple->effects, request.texResource);
    ripple->matIndex = BillboardActorSys_ExecMatLoadRequests(ripple->billboardActor, &request, 1);
}

static void FieldEffectRipple_FreeMaterial(FieldEffectRipple *ripple) {
    BillboardActorSys_FreeMaterials(ripple->billboardActor, ripple->matIndex, 1);
}

// Start a ripple at the actor's feet, further back if big, and at its first tile if atInit
void FldeffRipple_Start(FieldEffects *effects, FieldActor *actor, u32 big, u32 atInit) {
    VecFx32 pos;
    RippleTaskParams params;
    GridPos gpos;

    if (atInit) {
        GetActorInitGPos(actor, &gpos);
        ConvGXZToVector(gpos.x, gpos.z, &pos);
        // BUG: The initial-position path leaves the ripple height uninitialized.
#ifdef BUGFIX
        pos.y = GetActorPosY(actor);
#endif
    } else {
        CopyActorWPos(actor, &pos);
    }
    if (!big) {
        pos.y -= FX32_CONST(8);
        pos.z -= FX32_CONST(5);
    } else {
        pos.y -= FX32_CONST(14);
        pos.z -= FX32_CONST(10);
    }
    params.ripple = FieldEffects_GetHandleData(effects, FLDEFF_RIPPLE);
    params.pos = pos;
    FieldEffects_TCBCreate(effects, &RIPPLE_TASK_VTABLE, NULL, 0, &params, 0);
}

static void RippleTask_Init(FieldEffectTask *task, void *work) {
    RippleTask *rippleTask = work;
    RippleTaskParams *params = FieldEffects_GetObjectParam(task);
    BillboardActorRequest request;
    BillboardActorSys *billboardActor;
    u16 actorIndex;

    rippleTask->ripple = params->ripple;
    FieldEffects_SetObjectPos(task, &params->pos);
    billboardActor = rippleTask->ripple->billboardActor;
    request.texMat = rippleTask->ripple->matIndex;
    request.scaleX = FX32_CONST(1);
    request.scaleY = FX32_CONST(1);
    request.alpha = 31;
    request.visible = TRUE;
    request.lights = 1;
    request.pos = params->pos;
    request.callback = NULL;
    request.callbackData = rippleTask;
    rippleTask->actorIndex = BillboardActorSys_ExecActorRequests(billboardActor, 0, &request, 1, 3);
    if (rippleTask->actorIndex == BILLBOARD_ACTOR_NONE) {
        FieldEffects_DeleteObject(task);
        return;
    }
    BillboardActorSys_SetActorAnims(billboardActor, rippleTask->actorIndex, RIPPLE_ANIMS, 1);
    actorIndex = rippleTask->actorIndex;
    BillboardActorSys_SetActorAnim(billboardActor, actorIndex, 0);
    BillboardActorSys_SetActorAnimating(billboardActor, actorIndex, TRUE);
}

static void RippleTask_Free(FieldEffectTask *task, void *work) {
    RippleTask *rippleTask = work;
    BillboardActorSys *billboardActor = rippleTask->ripple->billboardActor;
    u16 actorIndex = rippleTask->actorIndex;

    if (actorIndex != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys_DeleteActors(billboardActor, actorIndex, 1);
    }
}

static void RippleTask_Update(FieldEffectTask *task, void *work) {
    RippleTask *rippleTask = work;
    u16 cmd;

    if (BillboardActorSys_GetActorLastCmd(rippleTask->ripple->billboardActor, rippleTask->actorIndex, &cmd)) {
        FieldEffects_DeleteObject(task);
    }
}

static void RippleTask_Draw(FieldEffectTask *task, void *work) {
}
