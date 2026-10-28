// The shadow under each field actor: a billboard that follows the actor and hides while it can't have one. The name is
// the ROM's own, from GFL_HeapAllocate's file argument. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/zone.h"
#include "gfl/billboard_actor.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

typedef struct {
    FieldEffects *effects;
    VecFx32 scale;
    BillboardActorSys *billboardActor;
    BillboardActorScene *scene;
    u32 matIndex;
    FieldColorPostFX *postFx;
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

static void FldeffShadow_CreateSlot(FieldEffectShadow *shadow);
static void FldeffShadow_DeleteSlot(FieldEffectShadow *shadow);
static void FldeffShadow_Init(FieldEffectTask *task, void *work);
static void FldeffShadow_Exit(FieldEffectTask *task, void *work);
static void FldeffShadow_Update(FieldEffectTask *task, void *work);
static void FldeffShadow_Draw(FieldEffectTask *task, void *work);

static const VecFx32 SHADOW_SCALE = { FX32_ONE, FX32_ONE, FX32_CONST(0.75) };

// clang-format off
static const FieldEffectTaskVTable SHADOW_TASK_VTABLE = {
    sizeof(ShadowTask),
    FldeffShadow_Init,
    FldeffShadow_Exit,
    FldeffShadow_Update,
    FldeffShadow_Draw,
};

// clang-format on

void *FieldEffect_Shadow_Create(FieldEffects *effects, HeapID heapId) {
    Field *field;
    FieldEffectShadow *shadow = GFL_HeapAllocate(heapId, sizeof(FieldEffectShadow), TRUE, "fldeff_shadow.c", 95);

    shadow->effects = effects;
    shadow->scale = SHADOW_SCALE;
    field = FieldEffects_GetField(effects);
    shadow->billboardActor = Field_GetEffectBillboardActor(field);
    shadow->scene = BillboardActorSys_GetScene(shadow->billboardActor);
    shadow->postFx = Field_GetColorPostFX(field);
    FldeffShadow_CreateSlot(shadow);
    return shadow;
}

void FieldEffect_Shadow_Free(FieldEffects *effects, void *data) {
    FldeffShadow_DeleteSlot(data);
    GFL_HeapFree(data);
}

static void FldeffShadow_CreateSlot(FieldEffectShadow *shadow) {
    BillboardActorMatRequest request;
    G3DResource *texture = GFL_G3DSysReadArcToolResource(FieldEffects_GetArc(shadow->effects), 47);

    FieldColorPostFX_Apply(shadow->postFx, texture);
    request.texResource = texture;
    request.format = 2;
    request.size = 17;
    request.faceWidth = 16;
    request.faceHeight = 16;
    request.mode = BILLBOARD_ACTOR_LOAD_TRIM;
    shadow->matIndex = BillboardActorSys_ExecMatLoadRequests(shadow->billboardActor, &request, 1);
}

static void FldeffShadow_DeleteSlot(FieldEffectShadow *shadow) {
    BillboardActorSys_FreeMaterials(shadow->billboardActor, shadow->matIndex, 1);
}

// Give the actor a shadow
void FldeffShadow_Start(FieldActor *actor, FieldEffects *effects) {
    ShadowTaskParams params;

    params.shadow = FieldEffects_GetHandleData(effects, FLDEFF_SHADOW);
    params.actor = actor;
    FieldEffects_TCBCreate(effects, &SHADOW_TASK_VTABLE, NULL, 0, &params, 0);
}

static void FldeffShadow_Init(FieldEffectTask *task, void *work) {
    ShadowTask *shadowTask = work;
    const ShadowTaskParams *params = FieldEffects_GetObjectParam(task);
    BillboardActorRequest request;
    u8 polygonId;

    shadowTask->shadow = params->shadow;
    shadowTask->actor = params->actor;
    FldAct_GetIdentity(params->actor, &shadowTask->identity);
    request.texMat = 0;
    request.scaleX = FX32_ONE;
    request.scaleY = FX32_ONE;
    request.alpha = 19;
    request.visible = TRUE;
    request.lights = 1;
    request.pos = (VecFx32){ 0, 0, 0 };
    request.callback = NULL;
    request.callbackData = shadowTask;
    shadowTask->actorIndex = BillboardActorSys_ExecActorRequests(shadowTask->shadow->billboardActor,
                                                                 shadowTask->shadow->matIndex, &request, 1, 3);
    if (shadowTask->actorIndex == BILLBOARD_ACTOR_NONE) {
        FieldEffects_DeleteObject(task);
        return;
    }
    polygonId = 2;
    BillboardActorScene_SetActorPolygonId(shadowTask->shadow->scene, shadowTask->actorIndex, &polygonId);
    FieldEffects_UpdateObject(task);
}

static void FldeffShadow_Exit(FieldEffectTask *task, void *work) {
    ShadowTask *shadowTask = work;

    if (shadowTask->actorIndex != BILLBOARD_ACTOR_NONE) {
        BillboardActorSys_DeleteActors(shadowTask->shadow->billboardActor, (u16)shadowTask->actorIndex, 1);
    }
}

// End with the actor; hide while the actor is hidden or its shadow is
static void FldeffShadow_Update(FieldEffectTask *task, void *work) {
    ShadowTask *shadowTask = work;

    if (!FldAct_CheckIdentity(shadowTask->actor, &shadowTask->identity)) {
        FieldEffects_DeleteObject(task);
        return;
    }
    if (!FldActSys_AreShadowsEnabled(GetActorMModelSystem(shadowTask->actor))) {
        FieldEffects_DeleteObject(task);
        return;
    }
    if (IsActorHidden(shadowTask->actor) == TRUE ||
        CheckActorMovementFlag(shadowTask->actor, ACTOR_MOVEMENT_FLAG_HIDE_SHADOW)) {
        shadowTask->hidden = TRUE;
    } else {
        shadowTask->hidden = FALSE;
    }
}

// Follow the actor, a little above its feet
static void FldeffShadow_Draw(FieldEffectTask *task, void *work) {
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
                CopyActorWPosOffset(shadowTask->actor, &offset);
                offset.y = 0;
            } else {
                offset.x = 0;
                offset.y = 0;
                offset.z = 0;
            }
            if (config->billboardSize == ACTOR_BILLBOARD_SIZE_32) {
                offset.y = FX32_CONST(-0.5);
            }
            VEC_Add(&pos, &offset, &pos);
        }
        BillboardActorScene_SetActorHidden(shadowTask->shadow->scene, actorIndex, &visible);
        pos.y += FX32_ONE;
        BillboardActorScene_SetActorPos(shadowTask->shadow->scene, actorIndex, &pos);
    } else {
        BOOL hidden = FALSE;

        BillboardActorScene_SetActorHidden(shadowTask->shadow->scene, actorIndex, &hidden);
    }
}
