// The ripple that spreads where an actor steps in shallow water. The name is the ROM's own, from GFL_HeapAllocate's
// file argument.
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "gfl/blact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

typedef struct {
    FieldEffects *effects;
    BlActSys *blact;
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

static void func_ov036_021be7d8(FieldEffectRipple *ripple);
static void func_ov036_021be818(FieldEffectRipple *ripple);
static void func_ov036_021be8b0(FieldEffectTask *task, void *work);
static void func_ov036_021be948(FieldEffectTask *task, void *work);
static void func_ov036_021be964(FieldEffectTask *task, void *work);
static void func_ov036_021be984(FieldEffectTask *task, void *work);

static const FieldEffectTaskVTable RIPPLE_TASK_VTABLE = {
    sizeof(RippleTask), func_ov036_021be8b0, func_ov036_021be948, func_ov036_021be964, func_ov036_021be984,
};

static const BlActAnimStep RIPPLE_ANIM_SPREAD[] = {
    { 0, 0, 0, 3 }, { 1, 0, 0, 3 }, { 2, 0, 0, 3 }, { 3, 0, 0, 3 }, { BLACT_ANIM_CMD_END, 0, 0, 0 },
};

static const BlActAnimStep *RIPPLE_ANIMS[] = { RIPPLE_ANIM_SPREAD };

void *func_ov036_021be790(FieldEffects *effects, HeapID heapId) {
    FieldEffectRipple *ripple = GFL_HeapAllocate(heapId, sizeof(FieldEffectRipple), TRUE, "fldeff_ripple.c", 88);

    ripple->effects = effects;
    ripple->blact = Field_GetWildEffectBlAct(FieldEffects_GetField(effects));
    func_ov036_021be7d8(ripple);
    return ripple;
}

void func_ov036_021be7c4(FieldEffects *effects, void *data) {
    func_ov036_021be818(data);
    GFL_HeapFree(data);
}

static void func_ov036_021be7d8(FieldEffectRipple *ripple) {
    BlActMatRequest request;
    ArcTool *arc = FieldEffects_GetArc(ripple->effects);

    request.format = 2;
    request.size = 19;
    request.faceWidth = 16;
    request.faceHeight = 16;
    request.mode = BLACT_LOAD_TRIM;
    request.texResource = GFL_G3DSysReadArcToolResource(arc, 46);
    FieldEffects_ApplyLuminanceTable(ripple->effects, request.texResource);
    ripple->matIndex = BlActSys_ExecMatLoadRequests(ripple->blact, &request, 1);
}

static void func_ov036_021be818(FieldEffectRipple *ripple) {
    BlActSys_FreeMaterials(ripple->blact, ripple->matIndex, 1);
}

// Start a ripple at the actor's feet, further back if big, and at its first tile if atInit
void func_ov036_021be828(void *effects, FieldActor *actor, u32 big, u32 atInit) {
    VecFx32 pos;
    RippleTaskParams params;
    GridPos gpos;

    if (atInit) {
        GetActorInitGPos(actor, &gpos);
        ConvGXZToVector(gpos.x, gpos.z, &pos);
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
    params.ripple = FieldEffects_GetHandleData(effects, 7);
    params.pos = pos;
    FieldEffects_TCBCreate(effects, &RIPPLE_TASK_VTABLE, NULL, 0, &params, 0);
}

static void func_ov036_021be8b0(FieldEffectTask *task, void *work) {
    RippleTask *rippleTask = work;
    RippleTaskParams *params = func_ov036_021a3ac8(task);
    BlActActorRequest request;
    BlActSys *blact;
    u16 actorIndex;

    rippleTask->ripple = params->ripple;
    func_ov036_021a3ae8(task, &params->pos);
    blact = rippleTask->ripple->blact;
    request.texMat = rippleTask->ripple->matIndex;
    request.scaleX = FX32_CONST(1);
    request.scaleY = FX32_CONST(1);
    request.alpha = 31;
    request.visible = TRUE;
    request.lights = 1;
    request.pos = params->pos;
    request.callback = NULL;
    request.callbackData = rippleTask;
    rippleTask->actorIndex = BlActSys_ExecActorRequests(blact, 0, &request, 1, 3);
    if (rippleTask->actorIndex == 0xffff) {
        func_ov036_021a3a70(task);
        return;
    }
    BlActSys_SetActorAnims(blact, rippleTask->actorIndex, RIPPLE_ANIMS, 1);
    actorIndex = rippleTask->actorIndex;
    BlActSys_SetActorAnim(blact, actorIndex, 0);
    BlActSys_SetActorAnimating(blact, actorIndex, TRUE);
}

static void func_ov036_021be948(FieldEffectTask *task, void *work) {
    RippleTask *rippleTask = work;
    BlActSys *blact = rippleTask->ripple->blact;
    u16 actorIndex = rippleTask->actorIndex;

    if (actorIndex != 0xffff) {
        BlActSys_DeleteActors(blact, actorIndex, 1);
    }
}

static void func_ov036_021be964(FieldEffectTask *task, void *work) {
    RippleTask *rippleTask = work;
    u16 cmd;

    if (BlActSys_GetActorLastCmd(rippleTask->ripple->blact, rippleTask->actorIndex, &cmd)) {
        func_ov036_021a3a70(task);
    }
}

static void func_ov036_021be984(FieldEffectTask *task, void *work) {
}
