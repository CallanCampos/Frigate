// The Battle Subway's trains. The name is the ROM's own, from GFL_HeapAllocate's file argument. Function names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_effect.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"

#define BTRAIN_COUNT 8
#define BTRAIN_ANM_COUNT 4

typedef struct {
    FieldEffects *effects;
    G3DResource *mdlRes[BTRAIN_COUNT];
    G3DResource *anmRes[BTRAIN_ANM_COUNT];
    G3DModel *mdl[BTRAIN_COUNT];
} FieldEffectBTrain;

// What a train starts from
typedef struct {
    FieldEffectBTrain *effect;
    u32 trainIdx;
} BTrainTaskParams;

typedef struct {
    u8 unk0;
    u8 hidden;
    u8 anmIdx;
    u8 anmDone;
    FieldEffectBTrain *effect;
    u32 trainIdx;
    G3DActor *actor;
    G3DAnimation *anm[BTRAIN_ANM_COUNT];
} BTrainTask;

static void FldeffBtrain_LoadResources(FieldEffectBTrain *effect, u32 trainIdx);
static void FldeffBtrain_FreeResources(FieldEffectBTrain *effect);
static void FldeffBtrain_Init(FieldEffectTask *task, void *work);
static void FldeffBtrain_Exit(FieldEffectTask *task, void *work);
static void FldeffBtrain_Update(FieldEffectTask *task, void *work);
static void FldeffBtrain_Draw(FieldEffectTask *task, void *work);

static const u16 BTRAIN_ANM_DATIDS[BTRAIN_ANM_COUNT] = { 0x80, 0x81, 0x82, 0x83 };

static const u16 BATTLE_SUBWAY_TRAIN_EFFECT_DATIDS[BTRAIN_COUNT] = { 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c };

static const FieldEffectTaskVTable BTRAIN_TASK_VTABLE = {
    sizeof(BTrainTask), FldeffBtrain_Init, FldeffBtrain_Exit, FldeffBtrain_Update, FldeffBtrain_Draw,
};

static const SRTMatrix BTRAIN_SRT = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };

void *FieldEffect_BTrain_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectBTrain *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectBTrain), TRUE, "fldeff_btrain.c", 95);

    effect->effects = effects;
    return effect;
}

void FieldEffect_BTrain_Free(FieldEffects *effects, void *data) {
    FldeffBtrain_FreeResources(data);
    GFL_HeapFree(data);
}

static void FldeffBtrain_LoadResources(FieldEffectBTrain *effect, u32 trainIdx) {
    ArcTool *arc = FieldEffects_GetArc(effect->effects);
    const u16 *trainIds = BATTLE_SUBWAY_TRAIN_EFFECT_DATIDS;
    const u16 *anmIds;
    int i;

    if (effect->mdlRes[trainIdx] == NULL) {
        effect->mdlRes[trainIdx] = GFL_G3DSysReadArcToolResource(arc, trainIds[trainIdx]);
        GFL_G3DResUploadTexData(effect->mdlRes[trainIdx]);
        effect->mdl[trainIdx] = GFL_G3DMdlCreate(effect->mdlRes[trainIdx], 0, effect->mdlRes[trainIdx]);
    }
    anmIds = BTRAIN_ANM_DATIDS;
    for (i = 0; i < BTRAIN_ANM_COUNT; i++, anmIds++) {
        if (effect->anmRes[i] == NULL) {
            effect->anmRes[i] = GFL_G3DSysReadArcToolResource(arc, *anmIds);
        }
    }
}

static void FldeffBtrain_FreeResources(FieldEffectBTrain *effect) {
    int i;

    for (i = 0; i < BTRAIN_COUNT; i++) {
        if (effect->mdlRes[i] != NULL) {
            GFL_G3DMdlFree(effect->mdl[i]);
            GFL_G3DResFreeTexData(effect->mdlRes[i]);
            GFL_G3DResFree(effect->mdlRes[i]);
        }
    }
    for (i = 0; i < BTRAIN_ANM_COUNT; i++) {
        if (effect->anmRes[i] != NULL) {
            GFL_G3DResFree(effect->anmRes[i]);
        }
    }
}

// A train of the given kind, which starts at pos
FieldEffectTask *FldeffBtrain_Start(FieldEffects *effects, u32 trainIdx, const VecFx32 *pos) {
    BTrainTaskParams params;

    params.effect = FieldEffects_GetHandleData(effects, FLDEFF_BTRAIN);
    params.trainIdx = trainIdx;
    FldeffBtrain_LoadResources(params.effect, trainIdx);
    return FieldEffects_TCBCreate(effects, &BTRAIN_TASK_VTABLE, pos, 0, &params, 0);
}

// Plays the animation anmIdx from its start
void FldeffBtrain_SetAnime(FieldEffectTask *task, u16 anmIdx) {
    if (task != NULL) {
        int i;
        BTrainTask *trainTask = FieldEffects_GetObjectWork(task);

        for (i = 0; i < BTRAIN_ANM_COUNT; i++) {
            GFL_G3DActorResetAnmFrame(trainTask->actor, i);
            if (i == anmIdx) {
                GFL_G3DActorBindAnm(trainTask->actor, i);
            } else {
                GFL_G3DActorUnbindAnm(trainTask->actor, i);
            }
        }
        trainTask->anmIdx = anmIdx;
        trainTask->anmDone = FALSE;
    }
}

void FldeffBtrain_SetHidden(FieldEffectTask *task, u16 hidden) {
    if (task != NULL) {
        BTrainTask *trainTask = FieldEffects_GetObjectWork(task);

        trainTask->hidden = hidden;
    }
}

static void FldeffBtrain_Init(FieldEffectTask *task, void *work) {
    BTrainTask *trainTask = work;
    BTrainTaskParams *params = FieldEffects_GetObjectParam(task);
    FieldEffectBTrain *effect = params->effect;
    u32 trainIdx = params->trainIdx;
    int i;
    int j;

    trainTask->effect = effect;
    trainTask->trainIdx = trainIdx;
    for (i = 0; i < BTRAIN_ANM_COUNT; i++) {
        trainTask->anm[i] = GFL_G3DAnmCreate(effect->mdl[trainIdx], effect->anmRes[i], 0);
    }
    trainTask->anmIdx = BTRAIN_ANM_COUNT;
    trainTask->actor = GFL_G3DActorCreate(effect->mdl[trainIdx], trainTask->anm, BTRAIN_ANM_COUNT);
    for (j = 0; j < BTRAIN_ANM_COUNT; j++) {
        GFL_G3DActorUnbindAnm(trainTask->actor, j);
    }
}

static void FldeffBtrain_Exit(FieldEffectTask *task, void *work) {
    BTrainTask *trainTask = work;
    int i;

    GFL_G3DActorFree(trainTask->actor);
    for (i = 0; i < BTRAIN_ANM_COUNT; i++) {
        GFL_G3DAnmFree(trainTask->anm[i]);
    }
}

static void FldeffBtrain_Update(FieldEffectTask *task, void *work) {
    BTrainTask *trainTask = work;

    if (trainTask->anmIdx != BTRAIN_ANM_COUNT) {
        trainTask->anmDone = FALSE;
        if (!GFL_G3DActorStepAnmFrame(trainTask->actor, trainTask->anmIdx, FX32_ONE)) {
            trainTask->anmDone = TRUE;
        }
    }
}

static void FldeffBtrain_Draw(FieldEffectTask *task, void *work) {
    BTrainTask *trainTask = work;
    SRTMatrix srt;

    if (trainTask->hidden == 0) {
        srt = BTRAIN_SRT;
        MTX_Identity33_(&srt.rotation);
        FieldEffects_GetObjectPos(task, &srt.translation);
        GFL_G3DSysDrawObjBBoxCull(trainTask->actor, &srt);
    }
}
