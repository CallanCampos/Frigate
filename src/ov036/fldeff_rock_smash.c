// The rock that breaks into pieces when Rock Smash is used on it. The ROM names it fldeff_iwakudaki.c, by
// GFL_HeapAllocate's file argument ("iwakudaki" is Rock Smash). Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"

#define ROCK_SMASH_ANM_COUNT 4

typedef struct {
    FieldEffects *effects;
    G3DResource *mdlRes;
    G3DResource *anmRes[ROCK_SMASH_ANM_COUNT];
    TCB *uploadTcb;
    // Set once the model's textures are in VRAM
    BOOL uploaded;
} FieldEffectRockSmash;

// What the breaking starts from
typedef struct {
    FieldEffectRockSmash *effect;
    VecFx32 pos;
} RockSmashTaskParams;

typedef struct {
    FieldEffectRockSmash *effect;
    G3DActor *actor;
    G3DAnimation *anms[ROCK_SMASH_ANM_COUNT];
    G3DModel *mdl;
} RockSmashTask;

static void FldeffRockSmash_LoadResources(FieldEffectRockSmash *effect);
static void FldeffRockSmash_FreeResources(FieldEffectRockSmash *effect);
static void FldeffRockSmash_UploadTexturesTask(TCB *tcb, void *data);
static void FldeffRockSmash_Init(FieldEffectTask *task, void *work);
static void FldeffRockSmash_Exit(FieldEffectTask *task, void *work);
static void FldeffRockSmash_Update(FieldEffectTask *task, void *work);
static void FldeffRockSmash_Draw(FieldEffectTask *task, void *work);

// clang-format off
static const FieldEffectTaskVTable ROCK_SMASH_TASK_VTABLE = {
    sizeof(RockSmashTask),
    FldeffRockSmash_Init,
    FldeffRockSmash_Exit,
    FldeffRockSmash_Update,
    FldeffRockSmash_Draw,
};

// clang-format on

static u32 ROCK_SMASH_ANM_IDS[ROCK_SMASH_ANM_COUNT] = { 117, 105, 155, 136 };

void *FldeffRockSmash_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectRockSmash *effect =
        GFL_HeapAllocate(heapId, sizeof(FieldEffectRockSmash), TRUE, "fldeff_iwakudaki.c", 103);

    effect->effects = effects;
    FldeffRockSmash_LoadResources(effect);
    return effect;
}

void FldeffRockSmash_Delete(FieldEffects *effects, void *data) {
    FldeffRockSmash_FreeResources(data);
    GFL_HeapFree(data);
}

static void FldeffRockSmash_LoadResources(FieldEffectRockSmash *effect) {
    int i;
    ArcTool *arc = FieldEffects_GetArc(effect->effects);

    effect->mdlRes = GFL_G3DSysReadArcToolResource(arc, 77);
    GFL_G3DResSetupTexData(effect->mdlRes);
    effect->uploadTcb = GFL_VBlankTCBAdd(FldeffRockSmash_UploadTexturesTask, effect, 0);
    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        effect->anmRes[i] = GFL_G3DSysReadArcToolResource(arc, ROCK_SMASH_ANM_IDS[i]);
    }
}

static void FldeffRockSmash_FreeResources(FieldEffectRockSmash *effect) {
    int i;

    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        GFL_G3DResFree(effect->anmRes[i]);
    }
    GFL_G3DResFreeTexData(effect->mdlRes);
    GFL_G3DResFree(effect->mdlRes);
    GFL_TCBRemove(effect->uploadTcb);
}

// Upload the model's textures in the first VBlank
static void FldeffRockSmash_UploadTexturesTask(TCB *tcb, void *data) {
    FieldEffectRockSmash *effect = data;

    if (!effect->uploaded) {
        GFL_G3DResUploadTexDataCore(effect->mdlRes);
        effect->uploaded = TRUE;
    }
}

// Break the rock in front of the actor, loading the effect first if the map hasn't
void FldeffRockSmash_Start(FieldActor *actor, FieldEffects *effects) {
    VecFx32 pos;
    RockSmashTaskParams params;
    u32 id;

    CopyActorWPos(actor, &pos);
    pos.y += FX32_ONE;
    pos.z += FX32_CONST(12);
    if (!FieldEffects_IsLoaded(effects, FLDEFF_ROCK_SMASH)) {
        id = FLDEFF_ROCK_SMASH;
        FieldEffects_Load(effects, &id, 1);
    }
    params.effect = FieldEffects_GetHandleData(effects, FLDEFF_ROCK_SMASH);
    params.pos = pos;
    FieldEffects_TCBCreate(effects, &ROCK_SMASH_TASK_VTABLE, NULL, 0, &params, 0);
}

static void FldeffRockSmash_Init(FieldEffectTask *task, void *work) {
    int i;
    RockSmashTask *rockTask = work;
    RockSmashTaskParams *params = FieldEffects_GetObjectParam(task);

    rockTask->effect = params->effect;
    FieldEffects_SetObjectPos(task, &params->pos);
    rockTask->mdl = GFL_G3DMdlCreate(rockTask->effect->mdlRes, 0, rockTask->effect->mdlRes);
    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        rockTask->anms[i] = GFL_G3DAnmCreate(rockTask->mdl, rockTask->effect->anmRes[i], 0);
    }
    rockTask->actor = GFL_G3DActorCreate(rockTask->mdl, rockTask->anms, ROCK_SMASH_ANM_COUNT);
    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        GFL_G3DActorBindAnm(rockTask->actor, i);
    }
}

static void FldeffRockSmash_Exit(FieldEffectTask *task, void *work) {
    int i;
    RockSmashTask *rockTask = work;

    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        GFL_G3DAnmFree(rockTask->anms[i]);
    }
    GFL_G3DActorFree(rockTask->actor);
    GFL_G3DMdlFree(rockTask->mdl);
}

// End once every animation has
static void FldeffRockSmash_Update(FieldEffectTask *task, void *work) {
    int i;
    RockSmashTask *rockTask = work;
    BOOL done = TRUE;

    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        if (GFL_G3DActorStepAnmFrame(rockTask->actor, i, FX32_ONE) == TRUE) {
            done = FALSE;
        }
    }
    if (done) {
        FieldEffects_DeleteObject(task);
    }
}

static void FldeffRockSmash_Draw(FieldEffectTask *task, void *work) {
    SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    RockSmashTask *rockTask = work;

    MTX_Identity33_(&srt.rotation);
    FieldEffects_GetObjectPos(task, &srt.translation);
    GFL_G3DSysDrawObjBBoxCull(rockTask->actor, &srt);
}
