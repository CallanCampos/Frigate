// The sparkles of the Join Avenue's festival: one model drawn at up to eight places. The ROM names it
// fldeff_fes_kira.c, by GFL_HeapAllocate's file argument ("kira" is sparkle). Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_effect.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"

#define FES_SPARKLE_MAX 8

// The model and animations of the sparkle
typedef struct {
    u8 anmCount;
    u16 mdlId;
    u16 anmIds[4];
} FesSparkleSpec;

typedef struct {
    G3DActor *actor;
    G3DAnimation *anms[4];
    G3DModel *mdl;
} FesSparkleObj;

typedef struct {
    FieldEffects *effects;
    G3DResource *mdlRes;
    G3DResource *anmRes[4];
    FesSparkleObj obj;
    FesSparkleSpec spec;
} FieldEffectFesSparkle;

// What the sparkles start from
typedef struct {
    FieldEffectFesSparkle *effect;
    MMSys *actorSys;
    u32 unk08;
} FesSparkleTaskParams;

typedef struct {
    VecFx32 pos;
    u16 active;
    u16 value;
} FesSparkle;

typedef struct {
    FesSparkleTaskParams params;
    FesSparkle sparkles[FES_SPARKLE_MAX];
} FesSparkleTask;

static FieldEffectFesSparkle *FldeffFesSparkle_CreateCore(FieldEffects *effects, HeapID heapId);
static void FldeffFesSparkle_LoadSpec(FieldEffectFesSparkle *effect);
static void FesSparkleObj_Create(FesSparkleObj *obj, FieldEffectFesSparkle *effect);
static void FesSparkleObj_Free(FesSparkleObj *obj, FieldEffectFesSparkle *effect);
static void FldeffFesSparkle_LoadModel(FieldEffectFesSparkle *effect, const FesSparkleSpec *spec, ArcTool *arc);
static void FldeffFesSparkle_FreeResources(FieldEffectFesSparkle *effect);
static void FldeffFesSparkle_Init(FieldEffectTask *task, void *work);
static void FldeffFesSparkle_Exit(FieldEffectTask *task, void *work);
static void FldeffFesSparkle_Update(FieldEffectTask *task, void *work);
static void FldeffFesSparkle_Draw(FieldEffectTask *task, void *work);

// clang-format off
static const FieldEffectTaskVTable FES_SPARKLE_TASK_VTABLE = {
    sizeof(FesSparkleTask),
    FldeffFesSparkle_Init,
    FldeffFesSparkle_Exit,
    FldeffFesSparkle_Update,
    FldeffFesSparkle_Draw,
};

// clang-format on

static FesSparkleSpec FES_SPARKLE_SPEC = { 1, 99, { 158, 0, 0, 0 } };

void *FldeffFesSparkle_Create(FieldEffects *effects, HeapID heapId) {
    return FldeffFesSparkle_CreateCore(effects, heapId);
}

void FldeffFesSparkle_Delete(FieldEffects *effects, void *data) {
    FldeffFesSparkle_FreeResources(data);
    GFL_HeapFree(data);
}

static FieldEffectFesSparkle *FldeffFesSparkle_CreateCore(FieldEffects *effects, HeapID heapId) {
    FieldEffectFesSparkle *effect =
        GFL_HeapAllocate(heapId, sizeof(FieldEffectFesSparkle), TRUE, "fldeff_fes_kira.c", 179);

    effect->effects = effects;
    FldeffFesSparkle_LoadSpec(effect);
    return effect;
}

static void FldeffFesSparkle_LoadSpec(FieldEffectFesSparkle *effect) {
    effect->spec = FES_SPARKLE_SPEC;
    FldeffFesSparkle_LoadModel(effect, &effect->spec, FieldEffects_GetArc(effect->effects));
}

static void FesSparkleObj_Create(FesSparkleObj *obj, FieldEffectFesSparkle *effect) {
    int i;

    obj->mdl = GFL_G3DMdlCreate(effect->mdlRes, 0, effect->mdlRes);
    for (i = 0; i < effect->spec.anmCount; i++) {
        obj->anms[i] = GFL_G3DAnmCreate(obj->mdl, effect->anmRes[i], 0);
    }
    obj->actor = GFL_G3DActorCreate(obj->mdl, obj->anms, effect->spec.anmCount);
    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DActorBindAnm(obj->actor, i);
    }
}

static void FesSparkleObj_Free(FesSparkleObj *obj, FieldEffectFesSparkle *effect) {
    int i;

    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DAnmFree(obj->anms[i]);
    }
    GFL_G3DActorFree(obj->actor);
    GFL_G3DMdlFree(obj->mdl);
}

static void FldeffFesSparkle_LoadModel(FieldEffectFesSparkle *effect, const FesSparkleSpec *spec, ArcTool *arc) {
    int i;

    effect->mdlRes = GFL_G3DSysReadArcToolResource(arc, spec->mdlId);
    GFL_G3DResUploadTexData(effect->mdlRes);
    for (i = 0; i < spec->anmCount; i++) {
        effect->anmRes[i] = GFL_G3DSysReadArcToolResource(arc, spec->anmIds[i]);
    }
    FesSparkleObj_Create(&effect->obj, effect);
}

static void FldeffFesSparkle_FreeResources(FieldEffectFesSparkle *effect) {
    int i;

    FesSparkleObj_Free(&effect->obj, effect);
    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DResFree(effect->anmRes[i]);
    }
    GFL_G3DResFreeTexData(effect->mdlRes);
    GFL_G3DResFree(effect->mdlRes);
}

// Start the sparkles' task, with none shown, if the map has loaded the effect
FieldEffectTask *FldeffFesSparkle_Start(FieldEffects *effects) {
    FesSparkleTaskParams params;
    Field *field = FieldEffects_GetField(effects);
    FieldEffectFesSparkle *effect = FieldEffects_GetHandleData(effects, FLDEFF_FES_SPARKLE);

    if (effect == NULL) {
        return NULL;
    }
    MI_CpuFill8(&params, 0, sizeof(FesSparkleTaskParams));
    params.effect = effect;
    params.actorSys = Field_GetActorSystem(field);
    return FieldEffects_TCBCreate(effects, &FES_SPARKLE_TASK_VTABLE, NULL, 0, &params, 0);
}

// Show sparkle idx at pos
void FldeffFesSparkle_Show(FieldEffectTask *task, u8 idx, u16 value, const VecFx32 *pos) {
    FesSparkleTask *sparkleTask = FieldEffects_GetObjectWork(task);

    if (idx < FES_SPARKLE_MAX) {
        sparkleTask->sparkles[idx].active = TRUE;
        sparkleTask->sparkles[idx].pos = *pos;
        sparkleTask->sparkles[idx].value = value;
    }
}

// Hide sparkle idx, and every sparkle
void FldeffFesSparkle_Hide(FieldEffectTask *task, u8 idx) {
    FesSparkleTask *sparkleTask = FieldEffects_GetObjectWork(task);

    if (idx < FES_SPARKLE_MAX) {
        sparkleTask->sparkles[idx].active = FALSE;
    }
}

void FldeffFesSparkle_HideAll(FieldEffectTask *task) {
    int i;
    FesSparkleTask *sparkleTask = FieldEffects_GetObjectWork(task);

    for (i = 0; i < FES_SPARKLE_MAX; i++) {
        sparkleTask->sparkles[i].active = FALSE;
    }
}

// Whether sparkle idx shows, and its value
u16 FldeffFesSparkle_IsShown(FieldEffectTask *task, u8 idx) {
    FesSparkleTask *sparkleTask = FieldEffects_GetObjectWork(task);

    if (idx >= FES_SPARKLE_MAX) {
        return 0;
    }
    return sparkleTask->sparkles[idx].active;
}

u16 FldeffFesSparkle_GetValue(FieldEffectTask *task, u8 idx) {
    FesSparkleTask *sparkleTask = FieldEffects_GetObjectWork(task);

    if (idx >= FES_SPARKLE_MAX) {
        return 0;
    }
    return sparkleTask->sparkles[idx].value;
}

static void FldeffFesSparkle_Init(FieldEffectTask *task, void *work) {
    FesSparkleTask *sparkleTask = work;

    sparkleTask->params = *(FesSparkleTaskParams *)FieldEffects_GetObjectParam(task);
    FieldEffects_UpdateObject(task);
}

static void FldeffFesSparkle_Exit(FieldEffectTask *task, void *work) {
}

static void FldeffFesSparkle_Update(FieldEffectTask *task, void *work) {
    int i;
    FesSparkleTask *sparkleTask = work;
    FieldEffectFesSparkle *effect = sparkleTask->params.effect;

    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DActorStepAnmFrameLoop(effect->obj.actor, i, FX32_ONE);
    }
}

static void FldeffFesSparkle_Draw(FieldEffectTask *task, void *work) {
    SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    int i;
    FesSparkleTask *sparkleTask = work;
    FieldEffectFesSparkle *effect = sparkleTask->params.effect;

    MTX_Identity33_(&srt.rotation);
    for (i = 0; i < FES_SPARKLE_MAX; i++) {
        if (sparkleTask->sparkles[i].active) {
            srt.translation = sparkleTask->sparkles[i].pos;
            GFL_G3DSysDrawObjBBoxCull(effect->obj.actor, &srt);
        }
    }
}
