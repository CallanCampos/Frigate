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

void func_ov036_021c6a88(FieldEffectTask *task, BubbleTask *work);
void func_ov036_021c6aa8(FieldEffectTask *task, BubbleTask *work);
void func_ov036_021c6aac(FieldEffectTask *task, BubbleTask *work);
void func_ov036_021c6b34(FieldEffectTask *task, BubbleTask *work);
void func_ov036_021c6c0c(Bubble *bubble);

static const u32 BUBBLE_MDL_INDEX[1];
static const u32 BUBBLE_ANM_INDICES[BUBBLE_ANM_COUNT];
static const u32 BUBBLE_FILE_IDS[BUBBLE_RES_COUNT];
static const FieldEffectTaskVTable BUBBLE_TASK_VTABLE;

void func_ov036_021c6934(FieldActor *actor, FieldEffects *effects) {
    BubbleTaskParams params;
    VecFx32 pos;

    params.effect = FieldEffects_GetHandleData(effects, 15);
    params.actor = actor;
    CopyActorWPos(actor, &pos);
    FieldEffects_TCBCreate(effects, &BUBBLE_TASK_VTABLE, &pos, 0, &params, 0);
}

void func_ov036_021c69a4(FieldEffectBubble *effect);
void func_ov036_021c6a3c(FieldEffectBubble *effect);

void *func_ov036_021c696c(FieldEffects *effects, HeapID heapId) {
    FieldEffectBubble *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectBubble), TRUE, "fldeff_bubble.c", 213);

    effect->effects = effects;
    func_ov036_021c69a4(effect);
    return effect;
}

void func_ov036_021c6990(FieldEffects *effects, void *effect) {
    func_ov036_021c6a3c(effect);
    GFL_HeapFree(effect);
}

void func_ov036_021c69a4(FieldEffectBubble *effect) {
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

void func_ov036_021c6a3c(FieldEffectBubble *effect) {
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

void func_ov036_021c6a88(FieldEffectTask *task, BubbleTask *work) {
    BubbleTaskParams *params = func_ov036_021a3ac8(task);

    work->params = *params;
    work->spawnTimer = 0x78;
    work->acmdFinished = IsAllActorAcmdFinished(work->params.actor);
    work->turnTimer = 5;
}

void func_ov036_021c6aa8(FieldEffectTask *task, BubbleTask *work) {
}

Bubble *func_ov036_021c6b58(BubbleTask *work);
void func_ov036_021c6b80(Bubble *bubble, FieldActor *actor);
void func_ov036_021c6c18(Bubble *bubble, fx32 count);

void func_ov036_021c6aac(FieldEffectTask *task, BubbleTask *work) {
    Bubble *bubble;
    fx32 count;
    int i;

    work->spawnTimer++;
    if (work->spawnTimer >= 0x78) {
        bubble = func_ov036_021c6b58(work);
        if (bubble != NULL) {
            func_ov036_021c6b80(bubble, work->params.actor);
        }
        work->spawnTimer = 0;
    }
    if (work->acmdFinished != IsAllActorAcmdFinished(work->params.actor)) {
        work->turnTimer++;
        if (work->turnTimer >= 5) {
            bubble = func_ov036_021c6b58(work);
            if (bubble != NULL) {
                func_ov036_021c6b80(bubble, work->params.actor);
            }
            work->turnTimer = 0;
        }
        work->acmdFinished = IsAllActorAcmdFinished(work->params.actor);
    }
    GFL_G3DActorGetAnmFrameCount(work->params.effect->actor, 0, &count);
    for (i = 0; i < BUBBLE_COUNT; i++) {
        func_ov036_021c6c18(&work->bubbles[i], count);
    }
}

void func_ov036_021c6c34(Bubble *bubble, G3DActor *actor);

void func_ov036_021c6b34(FieldEffectTask *task, BubbleTask *work) {
    int i;

    for (i = 0; i < BUBBLE_COUNT; i++) {
        func_ov036_021c6c34(&work->bubbles[i], work->params.effect->actor);
    }
}

Bubble *func_ov036_021c6b58(BubbleTask *work) {
    int i;

    for (i = 0; i < BUBBLE_COUNT; i++) {
        if (!work->bubbles[i].active) {
            return &work->bubbles[i];
        }
    }
    return NULL;
}

u16 func_ov036_021c6c70(FieldActor *actor, VecFx16 *dir);

void func_ov036_021c6b80(Bubble *bubble, FieldActor *actor) {
    VecFx16 dir;
    fx32 scale;

    bubble->active = TRUE;
    bubble->frame = 0;
    CopyActorWPos(actor, &bubble->pos);
    if (func_ov036_021c6c70(actor, &dir) == 0) {
        scale = FX32_CONST(14);
    } else {
        scale = FX32_CONST(4);
    }
    bubble->pos.y += FX32_CONST(7);
    bubble->pos.x += FX_MUL(dir.x, scale);
    bubble->pos.z += FX_MUL(dir.z, scale);
}

void func_ov036_021c6c0c(Bubble *bubble) {
    MI_CpuFill8(bubble, 0, sizeof(Bubble));
}

void func_ov036_021c6c18(Bubble *bubble, fx32 count) {
    if (bubble->active) {
        bubble->frame += FX32_ONE;
        if (bubble->frame > count) {
            func_ov036_021c6c0c(bubble);
        }
    }
}

static SRTMatrix sBubbleMatrix = {
    { 0, 0, 0 },
    { FX32_ONE, FX32_ONE, FX32_ONE },
    { FX32_ONE, 0, 0, 0, FX32_ONE, 0, 0, 0, FX32_ONE },
};

void func_ov036_021c6c34(Bubble *bubble, G3DActor *actor) {
    int i;

    if (bubble->active) {
        sBubbleMatrix.translation = bubble->pos;
        for (i = 0; i < BUBBLE_ANM_COUNT; i++) {
            GFL_G3DActorSetAnmFrame(actor, i, &bubble->frame);
        }
        GFL_G3DSysDrawObjBBoxCull(actor, &sBubbleMatrix);
    }
}

u16 func_ov036_021c6c70(FieldActor *actor, VecFx16 *dir) {
    u16 faceDir = GetActorFaceDir(actor);

    if (!CheckActorFlag(actor, 0x2000)) {
        dir->y = 0;
        dir->x = GetDirectionVectorCompX(faceDir);
        dir->z = GetDirectionVectorCompZ(faceDir);
        dir->x = dir->x << 16;
        dir->z = dir->z << 16;
    } else {
        func_ov036_02195a78(actor, faceDir, dir);
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
    (FieldEffectTaskFunc)func_ov036_021c6a88,
    (FieldEffectTaskFunc)func_ov036_021c6aa8,
    (FieldEffectTaskFunc)func_ov036_021c6aac,
    (FieldEffectTaskFunc)func_ov036_021c6b34,
};
