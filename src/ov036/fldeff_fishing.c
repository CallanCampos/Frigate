// The ripples where the fishing line meets the water. The name is the ROM's own, from GFL_HeapAllocate's file
// argument. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_effect.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

typedef struct {
    FieldEffects *effects;
    G3DResource *mdlRes;
    G3DResource *anmRes;
    G3DActor *actor;
    G3DAnimation *anm;
    G3DModel *mdl;
} FieldEffectFishing;

// What the ripples start from
typedef struct {
    FieldEffectFishing *effect;
    VecFx32 pos;
} FishingTaskParams;

typedef struct {
    FieldEffectFishing *effect;
    // How fast the ripples play, once set
    u16 speed;
    u16 speedSet;
} FishingTask;

static void FldeffFishing_LoadResources(FieldEffectFishing *effect);
static void FldeffFishing_FreeResources(FieldEffectFishing *effect);
static void FldeffFishing_Init(FieldEffectTask *task, void *work);
static void FldeffFishing_Exit(FieldEffectTask *task, void *work);
static void FldeffFishing_Update(FieldEffectTask *task, void *work);
static void FldeffFishing_Draw(FieldEffectTask *task, void *work);

// clang-format off
static const FieldEffectTaskVTable FISHING_TASK_VTABLE = {
    sizeof(FishingTask),
    FldeffFishing_Init,
    FldeffFishing_Exit,
    FldeffFishing_Update,
    FldeffFishing_Draw,
};

// clang-format on

static u16 FISHING_ANM_ID = 107;

void *FldeffFishing_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectFishing *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectFishing), TRUE, "fldeff_fishing.c", 96);

    effect->effects = effects;
    FldeffFishing_LoadResources(effect);
    return effect;
}

void FldeffFishing_Delete(FieldEffects *effects, void *data) {
    FldeffFishing_FreeResources(data);
    GFL_HeapFree(data);
}

static void FldeffFishing_LoadResources(FieldEffectFishing *effect) {
    ArcTool *arc = FieldEffects_GetArc(effect->effects);

    effect->mdlRes = GFL_G3DSysReadArcToolResource(arc, 84);
    GFL_G3DResUploadTexData(effect->mdlRes);
    effect->anmRes = GFL_G3DSysReadArcToolResource(arc, FISHING_ANM_ID);
    effect->mdl = GFL_G3DMdlCreate(effect->mdlRes, 0, effect->mdlRes);
    effect->anm = GFL_G3DAnmCreate(effect->mdl, effect->anmRes, 0);
    effect->actor = GFL_G3DActorCreate(effect->mdl, &effect->anm, 1);
    GFL_G3DActorBindAnm(effect->actor, 0);
}

static void FldeffFishing_FreeResources(FieldEffectFishing *effect) {
    GFL_G3DActorFree(effect->actor);
    GFL_G3DAnmFree(effect->anm);
    GFL_G3DMdlFree(effect->mdl);
    GFL_G3DResFree(effect->anmRes);
    GFL_G3DResFreeTexData(effect->mdlRes);
    GFL_G3DResFree(effect->mdlRes);
}

// Ripples at the end of the line, cast in dir from pos, a little further away when the water is lower
FieldEffectTask *FldeffFishing_Start(FieldEffects *effects, const VecFx32 *pos, u32 dir, u32 sameHeight) {
    u8 zOffsets[2][4] = { { 0, 0, 0, 0 }, { 0, 0, 3, 3 } };
    FishingTaskParams params;

    params.effect = FieldEffects_GetHandleData(effects, FLDEFF_FISHING);
    params.pos = *pos;
    params.pos.z += FX32_CONST(zOffsets[sameHeight][dir]);
    return FieldEffects_TCBCreate(effects, &FISHING_TASK_VTABLE, NULL, 0, &params, 0);
}

void FldeffFishing_SetAnimeSpeed(FieldEffectTask *task, u16 speed) {
    if (task != NULL) {
        FishingTask *fishingTask = FieldEffects_GetObjectWork(task);

        fishingTask->speedSet = TRUE;
        fishingTask->speed = speed;
    }
}

static void FldeffFishing_Init(FieldEffectTask *task, void *work) {
    FishingTask *fishingTask = work;
    FishingTaskParams *params = FieldEffects_GetObjectParam(task);

    fishingTask->effect = params->effect;
    FieldEffects_SetObjectPos(task, &params->pos);
}

static void FldeffFishing_Exit(FieldEffectTask *task, void *work) {
}

static void FldeffFishing_Update(FieldEffectTask *task, void *work) {
    FishingTask *fishingTask = work;
    FieldEffectFishing *effect = fishingTask->effect;

    GFL_G3DActorStepAnmFrameLoop(effect->actor, 0, FX32_CONST(fishingTask->speed * 2 + 1));
}

static void FldeffFishing_Draw(FieldEffectTask *task, void *work) {
    FishingTask *fishingTask = work;
    FieldEffectFishing *effect = fishingTask->effect;
    SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };

    MTX_Identity33_(&srt.rotation);
    FieldEffects_GetObjectPos(task, &srt.translation);
    GFL_G3DSysDrawObjBBoxCull(effect->actor, &srt);
}
