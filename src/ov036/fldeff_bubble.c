// fldeff_bubble.c: the bubbles that rise from an actor in the water
#include "types.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"

#define BUBBLE_RES_COUNT 3
#define BUBBLE_ANM_COUNT 2
#define BUBBLE_COUNT 6

typedef struct {
    FieldEffects *effects;
    G3DResource *res[BUBBLE_RES_COUNT];
    G3DModel *mdl;
    G3DAnimation *anms[BUBBLE_ANM_COUNT];
    G3DActor *actor;
} FieldEffectBubble;

typedef struct {
    BOOL active;
    fx32 frame;
    VecFx32 pos;
} Bubble;

typedef struct {
    FieldEffectBubble *effect;
    FieldActor *actor;
} BubbleTaskParams;

typedef struct {
    BubbleTaskParams params;
    u8 acmdFinished;
    s8 turnTimer;
    s8 spawnTimer;
    Bubble bubbles[BUBBLE_COUNT];
} BubbleTask;

void FldeffBubble_Init(FieldEffectTask *task, BubbleTask *work);
void FldeffBubble_Exit(FieldEffectTask *task, BubbleTask *work);
void FldeffBubble_Update(FieldEffectTask *task, BubbleTask *work);
void FldeffBubble_Draw(FieldEffectTask *task, BubbleTask *work);
void FldeffBubble_ClearBubble(Bubble *bubble);

static const u32 BUBBLE_MDL_INDEX[1];
static const u32 BUBBLE_ANM_INDICES[BUBBLE_ANM_COUNT];
static const u32 BUBBLE_FILE_IDS[BUBBLE_RES_COUNT];
static const FieldEffectTaskVTable BUBBLE_TASK_VTABLE;

void FldeffBubble_Start(FieldActor *actor, FieldEffects *effects) {
    BubbleTaskParams params;
    VecFx32 pos;

    params.effect = FieldEffects_GetHandleData(effects, FLDEFF_BUBBLE);
    params.actor = actor;
    CopyActorWPos(actor, &pos);
    FieldEffects_TCBCreate(effects, &BUBBLE_TASK_VTABLE, &pos, 0, &params, 0);
}

void FldeffBubble_LoadResources(FieldEffectBubble *effect);
void FldeffBubble_FreeResources(FieldEffectBubble *effect);

void *FldeffBubble_CreateRes(FieldEffects *effects, HeapID heapId) {
    FieldEffectBubble *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectBubble), TRUE, "fldeff_bubble.c", 213);

    effect->effects = effects;
    FldeffBubble_LoadResources(effect);
    return effect;
}

void FldeffBubble_DeleteRes(FieldEffects *effects, void *effect) {
    FldeffBubble_FreeResources(effect);
    GFL_HeapFree(effect);
}

void FldeffBubble_LoadResources(FieldEffectBubble *effect) {
    int i;
    ArcTool *arc = FieldEffects_GetArc(effect->effects);

    for (i = 0; i < BUBBLE_RES_COUNT; i++) {
        effect->res[i] = GFL_G3DSysReadArcToolResource(arc, BUBBLE_FILE_IDS[i]);
        if (GFL_G3DResCheckType(effect->res[i], 2)) {
            GFL_G3DResUploadTexData(effect->res[i]);
        }
    }
    effect->mdl = GFL_G3DMdlCreate(effect->res[BUBBLE_MDL_INDEX[0]], 0, effect->res[BUBBLE_MDL_INDEX[0]]);
    for (i = 0; i < BUBBLE_ANM_COUNT; i++) {
        effect->anms[i] = GFL_G3DAnmCreate(effect->mdl, effect->res[BUBBLE_ANM_INDICES[i]], 0);
    }
    effect->actor = GFL_G3DActorCreate(effect->mdl, effect->anms, BUBBLE_ANM_COUNT);
    for (i = 0; i < BUBBLE_ANM_COUNT; i++) {
        GFL_G3DActorBindAnm(effect->actor, i);
    }
}

void FldeffBubble_FreeResources(FieldEffectBubble *effect) {
    int i;

    GFL_G3DActorFree(effect->actor);
    for (i = 0; i < BUBBLE_ANM_COUNT; i++) {
        GFL_G3DAnmFree(effect->anms[i]);
    }
    GFL_G3DMdlFree(effect->mdl);
    for (i = 0; i < BUBBLE_RES_COUNT; i++) {
        if (GFL_G3DResCheckType(effect->res[i], 2)) {
            GFL_G3DResFreeTexData(effect->res[i]);
        }
        GFL_G3DResFree(effect->res[i]);
    }
}

void FldeffBubble_Init(FieldEffectTask *task, BubbleTask *work) {
    BubbleTaskParams *params = FieldEffects_GetObjectParam(task);

    work->params = *params;
    work->spawnTimer = 0x78;
    work->acmdFinished = IsAllActorAcmdFinished(work->params.actor);
    work->turnTimer = 5;
}

void FldeffBubble_Exit(FieldEffectTask *task, BubbleTask *work) {
}

Bubble *FldeffBubble_FindFreeBubble(BubbleTask *work);
void FldeffBubble_SpawnBubble(Bubble *bubble, FieldActor *actor);
void FldeffBubble_UpdateBubble(Bubble *bubble, fx32 count);

void FldeffBubble_Update(FieldEffectTask *task, BubbleTask *work) {
    Bubble *bubble;
    fx32 count;
    int i;

    work->spawnTimer++;
    if (work->spawnTimer >= 0x78) {
        bubble = FldeffBubble_FindFreeBubble(work);
        if (bubble != NULL) {
            FldeffBubble_SpawnBubble(bubble, work->params.actor);
        }
        work->spawnTimer = 0;
    }
    if (work->acmdFinished != IsAllActorAcmdFinished(work->params.actor)) {
        work->turnTimer++;
        if (work->turnTimer >= 5) {
            bubble = FldeffBubble_FindFreeBubble(work);
            if (bubble != NULL) {
                FldeffBubble_SpawnBubble(bubble, work->params.actor);
            }
            work->turnTimer = 0;
        }
        work->acmdFinished = IsAllActorAcmdFinished(work->params.actor);
    }
    GFL_G3DActorGetAnmFrameCount(work->params.effect->actor, 0, &count);
    for (i = 0; i < BUBBLE_COUNT; i++) {
        FldeffBubble_UpdateBubble(&work->bubbles[i], count);
    }
}

void FldeffBubble_DrawBubble(Bubble *bubble, G3DActor *actor);

void FldeffBubble_Draw(FieldEffectTask *task, BubbleTask *work) {
    int i;

    for (i = 0; i < BUBBLE_COUNT; i++) {
        FldeffBubble_DrawBubble(&work->bubbles[i], work->params.effect->actor);
    }
}

Bubble *FldeffBubble_FindFreeBubble(BubbleTask *work) {
    int i;

    for (i = 0; i < BUBBLE_COUNT; i++) {
        if (!work->bubbles[i].active) {
            return &work->bubbles[i];
        }
    }
    return NULL;
}

u16 FldeffBubble_GetFacingVector(FieldActor *actor, VecFx16 *dir);

void FldeffBubble_SpawnBubble(Bubble *bubble, FieldActor *actor) {
    VecFx16 dir;
    fx32 scale;

    bubble->active = TRUE;
    bubble->frame = 0;
    CopyActorWPos(actor, &bubble->pos);
    if (FldeffBubble_GetFacingVector(actor, &dir) == 0) {
        scale = FX32_CONST(14);
    } else {
        scale = FX32_CONST(4);
    }
    bubble->pos.y += FX32_CONST(7);
    bubble->pos.x += FX_MUL(dir.x, scale);
    bubble->pos.z += FX_MUL(dir.z, scale);
}

void FldeffBubble_ClearBubble(Bubble *bubble) {
    MI_CpuFill8(bubble, 0, sizeof(Bubble));
}

void FldeffBubble_UpdateBubble(Bubble *bubble, fx32 count) {
    if (bubble->active) {
        bubble->frame += FX32_ONE;
        if (bubble->frame > count) {
            FldeffBubble_ClearBubble(bubble);
        }
    }
}

static SRTMatrix sBubbleMatrix = {
    { 0, 0, 0 },
    { FX32_ONE, FX32_ONE, FX32_ONE },
    { FX32_ONE, 0, 0, 0, FX32_ONE, 0, 0, 0, FX32_ONE },
};

void FldeffBubble_DrawBubble(Bubble *bubble, G3DActor *actor) {
    int i;

    if (bubble->active) {
        sBubbleMatrix.translation = bubble->pos;
        for (i = 0; i < BUBBLE_ANM_COUNT; i++) {
            GFL_G3DActorSetAnmFrame(actor, i, &bubble->frame);
        }
        GFL_G3DSysDrawObjBBoxCull(actor, &sBubbleMatrix);
    }
}

u16 FldeffBubble_GetFacingVector(FieldActor *actor, VecFx16 *dir) {
    u16 faceDir = GetActorFaceDir(actor);

    if (!CheckActorFlag(actor, ACTOR_FLAG_RAIL)) {
        dir->y = 0;
        dir->x = GetDirectionVectorCompX(faceDir);
        dir->z = GetDirectionVectorCompZ(faceDir);
        dir->x = dir->x << 16;
        dir->z = dir->z << 16;
    } else {
        RailActor_GetDrawOffsetByDir(actor, faceDir, dir);
        dir->y = 0;
    }
    VEC_Fx16Normalize(dir, dir);
    return faceDir;
}

static const u32 BUBBLE_MDL_INDEX[1] = { 0 };
static const u32 BUBBLE_ANM_INDICES[BUBBLE_ANM_COUNT] = { 1, 2 };
static const u32 BUBBLE_FILE_IDS[BUBBLE_RES_COUNT] = { 0x53, 0x9d, 0x8a };
static const FieldEffectTaskVTable BUBBLE_TASK_VTABLE = {
    0x84,
    (FieldEffectTaskFunc)FldeffBubble_Init,
    (FieldEffectTaskFunc)FldeffBubble_Exit,
    (FieldEffectTaskFunc)FldeffBubble_Update,
    (FieldEffectTaskFunc)FldeffBubble_Draw,
};
