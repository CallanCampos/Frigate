// The shadow under each field actor: a billboard that follows the actor and hides while it can't have one. The name is
// the ROM's own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/zone.h"
#include "gfl/blact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

typedef struct {
    FieldEffects *effects;
    VecFx32 scale;
    BlActSys *blact;
    BlActScene *scene;
    u32 matIndex;
    void *postFx;
} FieldEffectShadow;

// What the task of an actor's shadow starts from
typedef struct {
    FieldEffectShadow *shadow;
    FieldActor *actor;
} ShadowTaskParams;

typedef struct {
    BOOL hidden;
    FieldEffectShadow *shadow;
    FieldActor *actor;
    FieldActorIdentity identity;
    u32 actorIndex;
} ShadowTask;

static void func_ov036_021a3b9c(FieldEffectShadow *shadow);
static void func_ov036_021a3bdc(FieldEffectShadow *shadow);
static void func_ov036_021a3c20(FieldEffectTask *task, void *work);
static void func_ov036_021a3cb4(FieldEffectTask *task, void *work);
static void func_ov036_021a3cd4(FieldEffectTask *task, void *work);
static void func_ov036_021a3d28(FieldEffectTask *task, void *work);

static const VecFx32 SHADOW_SCALE = { FX32_ONE, FX32_ONE, FX32_CONST(0.75) };

// clang-format off
static const FieldEffectTaskVTable SHADOW_TASK_VTABLE = {
    sizeof(ShadowTask),
    func_ov036_021a3c20,
    func_ov036_021a3cb4,
    func_ov036_021a3cd4,
    func_ov036_021a3d28,
};

// clang-format on

void *FieldEffect_Shadow_Create(FieldEffects *effects, HeapID heapId) {
    Field *field;
    FieldEffectShadow *shadow = GFL_HeapAllocate(heapId, sizeof(FieldEffectShadow), TRUE, "fldeff_shadow.c", 95);

    shadow->effects = effects;
    shadow->scale = SHADOW_SCALE;
    field = FieldEffects_GetField(effects);
    shadow->blact = Field_GetEffectBlAct(field);
    shadow->scene = BlActSys_GetScene(shadow->blact);
    shadow->postFx = Field_GetColorPostFX(field);
    func_ov036_021a3b9c(shadow);
    return shadow;
}

void FieldEffect_Shadow_Free(FieldEffects *effects, void *data) {
    func_ov036_021a3bdc(data);
    GFL_HeapFree(data);
}

static void func_ov036_021a3b9c(FieldEffectShadow *shadow) {
    BlActMatRequest request;
    void *texture = GFL_G3DSysReadArcToolResource(FieldEffects_GetArc(shadow->effects), 47);

    FieldColorPostFX_Apply(shadow->postFx, texture);
    request.texResource = texture;
    request.format = 2;
    request.size = 17;
    request.faceWidth = 16;
    request.faceHeight = 16;
    request.mode = BLACT_LOAD_TRIM;
    shadow->matIndex = BlActSys_ExecMatLoadRequests(shadow->blact, &request, 1);
}

static void func_ov036_021a3bdc(FieldEffectShadow *shadow) {
    BlActSys_FreeMaterials(shadow->blact, shadow->matIndex, 1);
}

// Give the actor a shadow
void func_ov036_021a3bf0(FieldActor *actor, FieldEffects *effects) {
    ShadowTaskParams params;

    params.shadow = FieldEffects_GetHandleData(effects, 0);
    params.actor = actor;
    FieldEffects_TCBCreate(effects, &SHADOW_TASK_VTABLE, NULL, 0, &params, 0);
}

static void func_ov036_021a3c20(FieldEffectTask *task, void *work) {
    ShadowTask *shadowTask = work;
    const ShadowTaskParams *params = func_ov036_021a3ac8(task);
    BlActActorRequest request;
    u8 polygonId;

    shadowTask->shadow = params->shadow;
    shadowTask->actor = params->actor;
    func_ov012_02167d88(params->actor, &shadowTask->identity);
    request.texMat = 0;
    request.scaleX = FX32_ONE;
    request.scaleY = FX32_ONE;
    request.alpha = 19;
    request.visible = TRUE;
    request.lights = 1;
    request.pos = (VecFx32){ 0, 0, 0 };
    request.callback = NULL;
    request.callbackData = shadowTask;
    shadowTask->actorIndex =
        BlActSys_ExecActorRequests(shadowTask->shadow->blact, shadowTask->shadow->matIndex, &request, 1, 3);
    if (shadowTask->actorIndex == 0xffff) {
        func_ov036_021a3a70(task);
        return;
    }
    polygonId = 2;
    BlActScene_SetActorPolygonId(shadowTask->shadow->scene, shadowTask->actorIndex, &polygonId);
    func_ov036_021a3a94(task);
}

static void func_ov036_021a3cb4(FieldEffectTask *task, void *work) {
    ShadowTask *shadowTask = work;

    if (shadowTask->actorIndex != 0xffff) {
        BlActSys_DeleteActors(shadowTask->shadow->blact, (u16)shadowTask->actorIndex, 1);
    }
}

// End with the actor; hide while the actor is hidden or flagged so
static void func_ov036_021a3cd4(FieldEffectTask *task, void *work) {
    ShadowTask *shadowTask = work;

    if (!func_ov012_02167da8(shadowTask->actor, &shadowTask->identity)) {
        func_ov036_021a3a70(task);
        return;
    }
    if (!func_ov012_021673f8(GetActorMModelSystem(shadowTask->actor))) {
        func_ov036_021a3a70(task);
        return;
    }
    if (func_ov012_02167520(shadowTask->actor) == TRUE || CheckActorMovementFlag(shadowTask->actor, 0x8000)) {
        shadowTask->hidden = TRUE;
    } else {
        shadowTask->hidden = FALSE;
    }
}

// Follow the actor, a little above its feet
static void func_ov036_021a3d28(FieldEffectTask *task, void *work) {
    VecFx32 pos;
    VecFx32 offset;
    BOOL visible;
    ShadowTask *shadowTask = work;
    u32 actorIndex = shadowTask->actorIndex;
    const FieldActorConfig *config = GetActorMdlInfo(shadowTask->actor);

    if (!shadowTask->hidden) {
        visible = TRUE;
        CopyActorWPos(shadowTask->actor, &pos);
        CopyActorPosOffset(shadowTask->actor, &offset);
        VEC_Add(&pos, &offset, &pos);
        if (config->sceneNodeType == 10 || config->sceneNodeType == 11) {
            if (GetZoneIsEntreeForest(GetActorZoneID(shadowTask->actor))) {
                func_ov012_0216731c(shadowTask->actor, &offset);
                offset.y = 0;
            } else {
                offset.x = 0;
                offset.y = 0;
                offset.z = 0;
            }
            if (config->billboardSize == 0) {
                offset.y = FX32_CONST(-0.5);
            }
            VEC_Add(&pos, &offset, &pos);
        }
        BlActScene_SetActorHidden(shadowTask->shadow->scene, actorIndex, &visible);
        pos.y += FX32_ONE;
        BlActScene_SetActorPos(shadowTask->shadow->scene, actorIndex, &pos);
    } else {
        BOOL hidden = FALSE;

        BlActScene_SetActorHidden(shadowTask->shadow->scene, actorIndex, &hidden);
    }
}
