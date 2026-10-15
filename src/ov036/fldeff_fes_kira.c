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
    void *anms[4];
    G3DModel *mdl;
} FesKiraObj;

typedef struct {
    FieldEffects *effects;
    void *mdlRes;
    void *anmRes[4];
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

static void *func_ov036_021a5a34(FieldEffects *effects, HeapID heapId);
static void func_ov036_021a5a58(FieldEffectFesKira *effect);
static void func_ov036_021a5a90(FesKiraObj *obj, FieldEffectFesKira *effect);
static void func_ov036_021a5afc(FesKiraObj *obj, FieldEffectFesKira *effect);
static void func_ov036_021a5b34(FieldEffectFesKira *effect, const FesKiraSpec *spec, ArcTool *arc);
static void func_ov036_021a5b78(FieldEffectFesKira *effect);
static void func_ov036_021a5c8c(FieldEffectTask *task, void *work);
static void func_ov036_021a5ca8(FieldEffectTask *task, void *work);
static void func_ov036_021a5cac(FieldEffectTask *task, void *work);
static void func_ov036_021a5cdc(FieldEffectTask *task, void *work);

static const FieldEffectTaskVTable FES_KIRA_TASK_VTABLE = {
    sizeof(FesKiraTask),
    func_ov036_021a5c8c,
    func_ov036_021a5ca8,
    func_ov036_021a5cac,
    func_ov036_021a5cdc,
};

static FesKiraSpec FES_KIRA_SPEC = { 1, 99, { 158, 0, 0, 0 } };

void *func_ov036_021a5a18(FieldEffects *effects, HeapID heapId) {
    return func_ov036_021a5a34(effects, heapId);
}

void func_ov036_021a5a20(FieldEffects *effects, void *data) {
    func_ov036_021a5b78(data);
    GFL_HeapFree(data);
}

static void *func_ov036_021a5a34(FieldEffects *effects, HeapID heapId) {
    FieldEffectFesKira *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectFesKira), TRUE, "fldeff_fes_kira.c", 179);

    effect->effects = effects;
    func_ov036_021a5a58(effect);
    return effect;
}

static void func_ov036_021a5a58(FieldEffectFesKira *effect) {
    effect->spec = FES_KIRA_SPEC;
    func_ov036_021a5b34(effect, &effect->spec, FieldEffects_GetArc(effect->effects));
}

static void func_ov036_021a5a90(FesKiraObj *obj, FieldEffectFesKira *effect) {
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

static void func_ov036_021a5afc(FesKiraObj *obj, FieldEffectFesKira *effect) {
    int i;

    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DAnmFree(obj->anms[i]);
    }
    GFL_G3DActorFree(obj->actor);
    GFL_G3DMdlFree(obj->mdl);
}

static void func_ov036_021a5b34(FieldEffectFesKira *effect, const FesKiraSpec *spec, ArcTool *arc) {
    int i;

    effect->mdlRes = GFL_G3DSysReadArcToolResource(arc, spec->mdlId);
    GFL_G3DResUploadTexData(effect->mdlRes);
    for (i = 0; i < spec->anmCount; i++) {
        effect->anmRes[i] = GFL_G3DSysReadArcToolResource(arc, spec->anmIds[i]);
    }
    func_ov036_021a5a90(&effect->obj, effect);
}

static void func_ov036_021a5b78(FieldEffectFesKira *effect) {
    int i;

    func_ov036_021a5afc(&effect->obj, effect);
    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DResFree(effect->anmRes[i]);
    }
    GFL_G3DResFreeTexData(effect->mdlRes);
    GFL_G3DResFree(effect->mdlRes);
}

// Start the sparkles' task, with none shown, if the map has loaded the effect
FieldEffectTask *func_ov036_021a5bb4(FieldEffects *effects) {
    FesKiraTaskParams params;
    Field *field = FieldEffects_GetField(effects);
    FieldEffectFesKira *effect = FieldEffects_GetHandleData(effects, 18);

    if (effect == NULL) {
        return NULL;
    }
    sys_memset(&params, 0, sizeof(FesKiraTaskParams));
    params.effect = effect;
    params.actorSys = Field_GetActorSystem(field);
    return FieldEffects_TCBCreate(effects, &FES_KIRA_TASK_VTABLE, NULL, 0, &params, 0);
}

// Show sparkle idx at pos
void func_ov036_021a5c04(FieldEffectTask *task, u8 idx, u16 value, const VecFx32 *pos) {
    FesKiraTask *kiraTask = func_ov036_021a3afc(task);

    if (idx < FES_KIRA_MAX) {
        kiraTask->kiras[idx].active = TRUE;
        kiraTask->kiras[idx].pos = *pos;
        kiraTask->kiras[idx].value = value;
    }
}

// Hide sparkle idx, and every sparkle
void func_ov036_021a5c2c(FieldEffectTask *task, u8 idx) {
    FesKiraTask *kiraTask = func_ov036_021a3afc(task);

    if (idx < FES_KIRA_MAX) {
        kiraTask->kiras[idx].active = FALSE;
    }
}

void func_ov036_021a5c44(FieldEffectTask *task) {
    int i;
    FesKiraTask *kiraTask = func_ov036_021a3afc(task);

    for (i = 0; i < FES_KIRA_MAX; i++) {
        kiraTask->kiras[i].active = FALSE;
    }
}

// Whether sparkle idx shows, and its value
u16 func_ov036_021a5c5c(FieldEffectTask *task, u8 idx) {
    FesKiraTask *kiraTask = func_ov036_021a3afc(task);

    if (idx >= FES_KIRA_MAX) {
        return 0;
    }
    return kiraTask->kiras[idx].active;
}

u16 func_ov036_021a5c74(FieldEffectTask *task, u8 idx) {
    FesKiraTask *kiraTask = func_ov036_021a3afc(task);

    if (idx >= FES_KIRA_MAX) {
        return 0;
    }
    return kiraTask->kiras[idx].value;
}

static void func_ov036_021a5c8c(FieldEffectTask *task, void *work) {
    FesKiraTask *kiraTask = work;

    kiraTask->params = *(FesKiraTaskParams *)func_ov036_021a3ac8(task);
    func_ov036_021a3a94(task);
}

static void func_ov036_021a5ca8(FieldEffectTask *task, void *work) {
}

static void func_ov036_021a5cac(FieldEffectTask *task, void *work) {
    int i;
    FesKiraTask *kiraTask = work;
    FieldEffectFesKira *effect = kiraTask->params.effect;

    for (i = 0; i < effect->spec.anmCount; i++) {
        GFL_G3DActorStepAnmFrameLoop(effect->obj.actor, i, FX32_ONE);
    }
}

static void func_ov036_021a5cdc(FieldEffectTask *task, void *work) {
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
