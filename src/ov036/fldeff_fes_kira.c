// The sparkles of the Join Avenue's festival: one model drawn at up to eight places. The name is the ROM's own, from
// GFL_HeapAllocate's file argument ("kira" is sparkle). Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_effect.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"

#define FES_KIRA_MAX 8

// The model and animations of the sparkle
typedef struct {
    u8 anmCount;
    u16 mdlId;
    u16 anmIds[4];
} FesKiraSpec;

typedef struct {
    G3DActor *actor;
    G3DAnimation *anms[4];
    G3DModel *mdl;
} FesKiraObj;

typedef struct {
    FieldEffects *effects;
    G3DResource *mdlRes;
    G3DResource *anmRes[4];
    FesKiraObj obj;
    FesKiraSpec spec;
} FieldEffectFesKira;

// What the sparkles start from
typedef struct {
    FieldEffectFesKira *effect;
    MMSys *actorSys;
    u32 unk08;
} FesKiraTaskParams;

typedef struct {
    VecFx32 pos;
    u16 active;
    u16 value;
} FesKira;

typedef struct {
    FesKiraTaskParams params;
    FesKira kiras[FES_KIRA_MAX];
} FesKiraTask;

static FieldEffectFesKira *FldeffFesKira_CreateCore(FieldEffects *effects, HeapID heapId);
static void FldeffFesKira_LoadSpec(FieldEffectFesKira *effect);
static void FesKiraObj_Create(FesKiraObj *obj, FieldEffectFesKira *effect);
static void FesKiraObj_Free(FesKiraObj *obj, FieldEffectFesKira *effect);
static void FldeffFesKira_LoadModel(FieldEffectFesKira *effect, const FesKiraSpec *spec, ArcTool *arc);
static void FldeffFesKira_FreeResources(FieldEffectFesKira *effect);
static void FldeffFesKira_Init(FieldEffectTask *task, void *work);
static void FldeffFesKira_Exit(FieldEffectTask *task, void *work);
static void FldeffFesKira_Update(FieldEffectTask *task, void *work);
static void FldeffFesKira_Draw(FieldEffectTask *task, void *work);

// clang-format off
static const FieldEffectTaskVTable FES_KIRA_TASK_VTABLE = {
    sizeof(FesKiraTask),
    FldeffFesKira_Init,
    FldeffFesKira_Exit,
    FldeffFesKira_Update,
    FldeffFesKira_Draw,
};

// clang-format on

static FesKiraSpec FES_KIRA_SPEC = { 1, 99, { 158, 0, 0, 0 } };

void *FldeffFesKira_Create(FieldEffects *effects, HeapID heapId) {
    return FldeffFesKira_CreateCore(effects, heapId);
}

void FldeffFesKira_Delete(FieldEffects *effects, void *data) {
    FldeffFesKira_FreeResources(data);
    GFL_HeapFree(data);
}

static FieldEffectFesKira *FldeffFesKira_CreateCore(FieldEffects *effects, HeapID heapId) {
    FieldEffectFesKira *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectFesKira), TRUE, "fldeff_fes_kira.c", 179);

    effect->effects = effects;
    FldeffFesKira_LoadSpec(effect);
    return effect;
}

static void FldeffFesKira_LoadSpec(FieldEffectFesKira *effect) {
    effect->spec = FES_KIRA_SPEC;
    FldeffFesKira_LoadModel(effect, &effect->spec, FieldEffects_GetArc(effect->effects));
}

static void FesKiraObj_Create(FesKiraObj *obj, FieldEffectFesKira *effect) {
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

static void FesKiraObj_Free(FesKiraObj *obj, FieldEffectFesKira *effect) {
    int i;

    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DAnmFree(obj->anms[i]);
    }
    GFL_G3DActorFree(obj->actor);
    GFL_G3DMdlFree(obj->mdl);
}

static void FldeffFesKira_LoadModel(FieldEffectFesKira *effect, const FesKiraSpec *spec, ArcTool *arc) {
    int i;

    effect->mdlRes = GFL_G3DSysReadArcToolResource(arc, spec->mdlId);
    GFL_G3DResUploadTexData(effect->mdlRes);
    for (i = 0; i < spec->anmCount; i++) {
        effect->anmRes[i] = GFL_G3DSysReadArcToolResource(arc, spec->anmIds[i]);
    }
    FesKiraObj_Create(&effect->obj, effect);
}

static void FldeffFesKira_FreeResources(FieldEffectFesKira *effect) {
    int i;

    FesKiraObj_Free(&effect->obj, effect);
    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DResFree(effect->anmRes[i]);
    }
    GFL_G3DResFreeTexData(effect->mdlRes);
    GFL_G3DResFree(effect->mdlRes);
}

// Start the sparkles' task, with none shown, if the map has loaded the effect
FieldEffectTask *FldeffFesKira_Start(FieldEffects *effects) {
    FesKiraTaskParams params;
    Field *field = FieldEffects_GetField(effects);
    FieldEffectFesKira *effect = FieldEffects_GetHandleData(effects, 18);

    if (effect == NULL) {
        return NULL;
    }
    MI_CpuFill8(&params, 0, sizeof(FesKiraTaskParams));
    params.effect = effect;
    params.actorSys = Field_GetActorSystem(field);
    return FieldEffects_TCBCreate(effects, &FES_KIRA_TASK_VTABLE, NULL, 0, &params, 0);
}

// Show sparkle idx at pos
void FldeffFesKira_Show(FieldEffectTask *task, u8 idx, u16 value, const VecFx32 *pos) {
    FesKiraTask *kiraTask = FieldEffects_GetObjectWork(task);

    if (idx < FES_KIRA_MAX) {
        kiraTask->kiras[idx].active = TRUE;
        kiraTask->kiras[idx].pos = *pos;
        kiraTask->kiras[idx].value = value;
    }
}

// Hide sparkle idx, and every sparkle
void FldeffFesKira_Hide(FieldEffectTask *task, u8 idx) {
    FesKiraTask *kiraTask = FieldEffects_GetObjectWork(task);

    if (idx < FES_KIRA_MAX) {
        kiraTask->kiras[idx].active = FALSE;
    }
}

void FldeffFesKira_HideAll(FieldEffectTask *task) {
    int i;
    FesKiraTask *kiraTask = FieldEffects_GetObjectWork(task);

    for (i = 0; i < FES_KIRA_MAX; i++) {
        kiraTask->kiras[i].active = FALSE;
    }
}

// Whether sparkle idx shows, and its value
u16 FldeffFesKira_IsShown(FieldEffectTask *task, u8 idx) {
    FesKiraTask *kiraTask = FieldEffects_GetObjectWork(task);

    if (idx >= FES_KIRA_MAX) {
        return 0;
    }
    return kiraTask->kiras[idx].active;
}

u16 FldeffFesKira_GetValue(FieldEffectTask *task, u8 idx) {
    FesKiraTask *kiraTask = FieldEffects_GetObjectWork(task);

    if (idx >= FES_KIRA_MAX) {
        return 0;
    }
    return kiraTask->kiras[idx].value;
}

static void FldeffFesKira_Init(FieldEffectTask *task, void *work) {
    FesKiraTask *kiraTask = work;

    kiraTask->params = *(FesKiraTaskParams *)FieldEffects_GetObjectParam(task);
    FieldEffects_UpdateObject(task);
}

static void FldeffFesKira_Exit(FieldEffectTask *task, void *work) {
}

static void FldeffFesKira_Update(FieldEffectTask *task, void *work) {
    int i;
    FesKiraTask *kiraTask = work;
    FieldEffectFesKira *effect = kiraTask->params.effect;

    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DActorStepAnmFrameLoop(effect->obj.actor, i, FX32_ONE);
    }
}

static void FldeffFesKira_Draw(FieldEffectTask *task, void *work) {
    SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    int i;
    FesKiraTask *kiraTask = work;
    FieldEffectFesKira *effect = kiraTask->params.effect;

    MTX_Identity33_(&srt.rotation);
    for (i = 0; i < FES_KIRA_MAX; i++) {
        if (kiraTask->kiras[i].active) {
            srt.translation = kiraTask->kiras[i].pos;
            GFL_G3DSysDrawObjBBoxCull(effect->obj.actor, &srt);
        }
    }
}
