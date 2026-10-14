// The rock that breaks into pieces when Rock Smash is used on it. The name is the ROM's own, from GFL_HeapAllocate's
// file argument ("iwakudaki" is Rock Smash). Function names from swan
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
    void *mdlRes;
    void *anmRes[ROCK_SMASH_ANM_COUNT];
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
    void *anms[ROCK_SMASH_ANM_COUNT];
    G3DModel *mdl;
} RockSmashTask;

static void func_ov036_021a563c(FieldEffectRockSmash *effect);
static void func_ov036_021a5684(FieldEffectRockSmash *effect);
static void func_ov036_021a56b0(TCB *tcb, void *data);
static void func_ov036_021a5734(FieldEffectTask *task, void *work);
static void func_ov036_021a5798(FieldEffectTask *task, void *work);
static void func_ov036_021a57bc(FieldEffectTask *task, void *work);
static void func_ov036_021a57ec(FieldEffectTask *task, void *work);

static const FieldEffectTaskVTable ROCK_SMASH_TASK_VTABLE = {
    sizeof(RockSmashTask),
    func_ov036_021a5734,
    func_ov036_021a5798,
    func_ov036_021a57bc,
    func_ov036_021a57ec,
};

static u32 ROCK_SMASH_ANM_IDS[ROCK_SMASH_ANM_COUNT] = { 117, 105, 155, 136 };

void *func_ov036_021a5604(FieldEffects *effects, HeapID heapId) {
    FieldEffectRockSmash *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectRockSmash), TRUE, "fldeff_iwakudaki.c",
                                                    103);

    effect->effects = effects;
    func_ov036_021a563c(effect);
    return effect;
}

void func_ov036_021a5628(FieldEffects *effects, void *data) {
    func_ov036_021a5684(data);
    GFL_HeapFree(data);
}

static void func_ov036_021a563c(FieldEffectRockSmash *effect) {
    int i;
    ArcTool *arc = FieldEffects_GetArc(effect->effects);

    effect->mdlRes = GFL_G3DSysReadArcToolResource(arc, 77);
    GFL_G3DResSetupTexData(effect->mdlRes);
    effect->uploadTcb = GFL_VBlankTCBAdd(func_ov036_021a56b0, effect, 0);
    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        effect->anmRes[i] = GFL_G3DSysReadArcToolResource(arc, ROCK_SMASH_ANM_IDS[i]);
    }
}

static void func_ov036_021a5684(FieldEffectRockSmash *effect) {
    int i;

    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        GFL_G3DResFree(effect->anmRes[i]);
    }
    GFL_G3DResFreeTexData(effect->mdlRes);
    GFL_G3DResFree(effect->mdlRes);
    GFL_TCBRemove(effect->uploadTcb);
}

// Upload the model's textures in the first VBlank
static void func_ov036_021a56b0(TCB *tcb, void *data) {
    FieldEffectRockSmash *effect = data;

    if (!effect->uploaded) {
        GFL_G3DResUploadTexDataCore(effect->mdlRes);
        effect->uploaded = TRUE;
    }
}

// Break the rock in front of the actor, loading the effect first if the map hasn't
void func_ov036_021a56c8(FieldActor *actor, FieldEffects *effects) {
    VecFx32 pos;
    RockSmashTaskParams params;
    u32 id;

    CopyActorWPos(actor, &pos);
    pos.y += FX32_ONE;
    pos.z += FX32_CONST(12);
    if (!FieldEffects_IsLoaded(effects, 9)) {
        id = 9;
        FieldEffects_Load(effects, &id, 1);
    }
    params.effect = FieldEffects_GetHandleData(effects, 9);
    params.pos = pos;
    FieldEffects_TCBCreate(effects, &ROCK_SMASH_TASK_VTABLE, NULL, 0, &params, 0);
}

static void func_ov036_021a5734(FieldEffectTask *task, void *work) {
    int i;
    RockSmashTask *rockTask = work;
    RockSmashTaskParams *params = func_ov036_021a3ac8(task);

    rockTask->effect = params->effect;
    func_ov036_021a3ae8(task, &params->pos);
    rockTask->mdl = GFL_G3DMdlCreate(rockTask->effect->mdlRes, 0, rockTask->effect->mdlRes);
    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        rockTask->anms[i] = GFL_G3DAnmCreate(rockTask->mdl, rockTask->effect->anmRes[i], 0);
    }
    rockTask->actor = GFL_G3DActorCreate(rockTask->mdl, rockTask->anms, ROCK_SMASH_ANM_COUNT);
    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        GFL_G3DActorBindAnm(rockTask->actor, i);
    }
}

static void func_ov036_021a5798(FieldEffectTask *task, void *work) {
    int i;
    RockSmashTask *rockTask = work;

    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        GFL_G3DAnmFree(rockTask->anms[i]);
    }
    GFL_G3DActorFree(rockTask->actor);
    GFL_G3DMdlFree(rockTask->mdl);
}

// End once every animation has
static void func_ov036_021a57bc(FieldEffectTask *task, void *work) {
    int i;
    RockSmashTask *rockTask = work;
    BOOL done = TRUE;

    for (i = 0; i < ROCK_SMASH_ANM_COUNT; i++) {
        if (GFL_G3DActorStepAnmFrame(rockTask->actor, i, FX32_ONE) == TRUE) {
            done = FALSE;
        }
    }
    if (done) {
        func_ov036_021a3a70(task);
    }
}

static void func_ov036_021a57ec(FieldEffectTask *task, void *work) {
    SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    RockSmashTask *rockTask = work;

    MTX_Identity33_(&srt.rotation);
    func_ov036_021a3ad4(task, &srt.translation);
    GFL_G3DSysDrawObjBBoxCull(rockTask->actor, &srt);
}
