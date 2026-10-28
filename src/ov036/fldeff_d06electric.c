// The electric effects of Chargestone Cave (d06denki): the sparks over a stone and the glow of a stone's mark. The ROM
// names it fldeff_d06denki.c, by GFL_HeapAllocate's file argument. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0). Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/sound.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"

#define D06ELECTRIC_MDL_COUNT 4
#define D06ELECTRIC_ANM_COUNT 5

typedef struct {
    FieldEffects *effects;
    G3DResource *mdlRes[D06ELECTRIC_MDL_COUNT];
    G3DResource *anmRes[D06ELECTRIC_ANM_COUNT];
    G3DAnimation *anm;
    G3DModel *mdl;
    G3DActor *actor;
} FieldEffectD06Electric;

// What the sparks over a stone start from
typedef struct {
    FieldEffectD06Electric *effect;
    VecFx32 pos;
} SparkTaskParams;

typedef struct {
    FieldEffectD06Electric *effect;
    fx32 frame;
    // Set once the sparks' animation has ended
    BOOL done;
} SparkTask;

// What a stone's effect starts from
typedef struct {
    FieldEffectD06Electric *effect;
    VecFx32 pos;
    FieldActor *actor;
} StoneTaskParams;

typedef struct {
    FieldEffectD06Electric *effect;
    VecFx32 pos;
    FieldActor *actor;
    u8 paused;
    u8 lit;
    u8 litDone;
    G3DActor *stoneActor;
    G3DAnimation *anms[2];
    G3DModel *stoneMdl;
    VecFx32 glowPos;
    G3DActor *glowActor;
    G3DModel *glowMdl;
} StoneTask;

typedef struct {
    FieldEffectD06Electric *effect;
    VecFx32 pos;
    u8 lit;
    G3DActor *actor;
    G3DAnimation *anms[2];
    G3DModel *mdl;
} MarkTask;

static void D06Electric_LoadResources(FieldEffectD06Electric *effect);
static void D06Electric_FreeResources(FieldEffectD06Electric *effect);
void FieldTerrainEffectElec_Init(FieldEffectTask *task, void *work);
static void SparkTask_Exit(FieldEffectTask *task, void *work);
static void SparkTask_Update(FieldEffectTask *task, void *work);
static void SparkTask_Draw(FieldEffectTask *task, void *work);
static void StoneTask_Init(FieldEffectTask *task, void *work);
static void StoneTask_Delete(FieldEffectTask *task, void *work);
static void StoneTask_Update(FieldEffectTask *task, void *work);
static void StoneTask_Draw(FieldEffectTask *task, void *work);
static void MarkTask_Init(FieldEffectTask *task, void *work);
static void MarkTask_Delete(FieldEffectTask *task, void *work);
static void MarkTask_Update(FieldEffectTask *task, void *work);
static void MarkTask_Draw(FieldEffectTask *task, void *work);

void *StoneTask_Create(FieldEffects *effects, HeapID heapId) {
    FieldEffectD06Electric *effect =
        GFL_HeapAllocate(heapId, sizeof(FieldEffectD06Electric), TRUE, "fldeff_d06denki.c", 169);

    effect->effects = effects;
    D06Electric_LoadResources(effect);
    return effect;
}

void D06Electric_Delete(FieldEffects *effects, void *data) {
    D06Electric_FreeResources(data);
    GFL_HeapFree(data);
}

static void D06Electric_LoadResources(FieldEffectD06Electric *effect) {
    u32 mdlIds[D06ELECTRIC_MDL_COUNT] = { 80, 78, 79, 81 };
    u32 anmIds[D06ELECTRIC_ANM_COUNT] = { 106, 124, 125, 126, 127 };
    u32 i;
    ArcTool *arc = FieldEffects_GetArc(effect->effects);

    for (i = 0; i < D06ELECTRIC_MDL_COUNT; i++) {
        effect->mdlRes[i] = GFL_G3DSysReadArcToolResource(arc, mdlIds[i]);
        GFL_G3DResUploadTexData(effect->mdlRes[i]);
    }
    for (i = 0; i < D06ELECTRIC_ANM_COUNT; i++) {
        effect->anmRes[i] = GFL_G3DSysReadArcToolResource(arc, anmIds[i]);
    }
    effect->mdl = GFL_G3DMdlCreate(effect->mdlRes[0], 0, effect->mdlRes[0]);
    effect->anm = GFL_G3DAnmCreate(effect->mdl, effect->anmRes[0], 0);
    effect->actor = GFL_G3DActorCreate(effect->mdl, &effect->anm, 1);
    GFL_G3DActorBindAnm(effect->actor, 0);
}

static void D06Electric_FreeResources(FieldEffectD06Electric *effect) {
    u32 i;

    GFL_G3DAnmFree(effect->anm);
    GFL_G3DActorFree(effect->actor);
    GFL_G3DMdlFree(effect->mdl);
    for (i = 0; i < D06ELECTRIC_MDL_COUNT; i++) {
        GFL_G3DResFreeTexData(effect->mdlRes[i]);
        GFL_G3DResFree(effect->mdlRes[i]);
    }
    for (i = 0; i < D06ELECTRIC_ANM_COUNT; i++) {
        GFL_G3DResFree(effect->anmRes[i]);
    }
}

static const FieldEffectTaskVTable STONE_TASK_VTABLE = {
    sizeof(StoneTask), StoneTask_Init, StoneTask_Delete, StoneTask_Update, StoneTask_Draw,
};

static const FieldEffectTaskVTable SPARK_TASK_VTABLE = {
    sizeof(SparkTask), FieldTerrainEffectElec_Init, SparkTask_Exit, SparkTask_Update, SparkTask_Draw,
};

static const FieldEffectTaskVTable MARK_TASK_VTABLE = {
    sizeof(MarkTask), MarkTask_Init, MarkTask_Delete, MarkTask_Update, MarkTask_Draw,
};

void D06Electric_StartSpark(FieldActor *actor, FieldEffects *effects) {
    VecFx32 pos;
    SparkTaskParams params;

    CopyActorWPos(actor, &pos);
    params.effect = FieldEffects_GetHandleData(effects, FLDEFF_D06_ELECTRIC);
    params.pos = pos;
    FieldEffects_TCBCreate(effects, &SPARK_TASK_VTABLE, NULL, 0, &params, 0);
}

void FieldTerrainEffectElec_Init(FieldEffectTask *task, void *work) {
    SparkTask *sparkTask = work;
    SparkTaskParams *params = FieldEffects_GetObjectParam(task);

    sparkTask->effect = params->effect;
    FieldEffects_SetObjectPos(task, &params->pos);
    GFL_SndSEPlay(SEQ_SE_FLD_49);
}

static void SparkTask_Exit(FieldEffectTask *task, void *work) {
}

static void SparkTask_Update(FieldEffectTask *task, void *work) {
    SparkTask *sparkTask = work;

    if (sparkTask->done == TRUE) {
        FieldEffects_DeleteObject(task);
    }
}

static void SparkTask_Draw(FieldEffectTask *task, void *work) {
    SparkTask *sparkTask = work;

    if (sparkTask->done == FALSE) {
        SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
        G3DActor *actor = sparkTask->effect->actor;

        GFL_G3DActorSetAnmFrame(actor, 0, &sparkTask->frame);
        if (GFL_G3DActorStepAnmFrame(actor, 0, FX32_ONE) == FALSE) {
            sparkTask->done = TRUE;
        }
        if (GFL_G3DActorGetAnmFrame(actor, 0, &sparkTask->frame) == FALSE) {
            sparkTask->done = TRUE;
        }
        MTX_Identity33_(&srt.rotation);
        FieldEffects_GetObjectPos(task, &srt.translation);
        GFL_G3DSysDrawObjBBoxCull(actor, &srt);
    }
}

FieldEffectTask *StoneTask_StartFollow(FieldEffects *effects, const VecFx32 *pos, FieldActor *actor) {
    StoneTaskParams params;

    params.effect = FieldEffects_GetHandleData(effects, FLDEFF_D06_ELECTRIC);
    params.pos = *pos;
    params.actor = actor;
    return FieldEffects_TCBCreate(effects, &STONE_TASK_VTABLE, NULL, 0, &params, 0);
}

void StoneTask_FollowFreezeLoop(FieldEffectTask *task) {
    if (task != NULL) {
        StoneTask *stoneTask = FieldEffects_GetObjectWork(task);

        stoneTask->paused = TRUE;
    }
}

void StoneTask_FollowStartEnding(FieldEffectTask *task) {
    if (task != NULL) {
        fx32 frame = 0;
        StoneTask *stoneTask = FieldEffects_GetObjectWork(task);

        stoneTask->lit = TRUE;
        stoneTask->litDone = FALSE;
        GFL_G3DActorSetAnmFrame(stoneTask->stoneActor, 1, &frame);
    }
}

u8 StoneTask_FollowIsEnded(FieldEffectTask *task) {
    if (task != NULL) {
        StoneTask *stoneTask = FieldEffects_GetObjectWork(task);

        return stoneTask->litDone;
    }
    return TRUE;
}

static void StoneTask_Init(FieldEffectTask *task, void *work) {
    StoneTask *stoneTask = work;
    StoneTaskParams *params = FieldEffects_GetObjectParam(task);

    *(StoneTaskParams *)stoneTask = *params;
    FieldEffects_SetObjectPos(task, &stoneTask->pos);
    stoneTask->stoneMdl = GFL_G3DMdlCreate(stoneTask->effect->mdlRes[1], 0, stoneTask->effect->mdlRes[1]);
    stoneTask->anms[0] = GFL_G3DAnmCreate(stoneTask->stoneMdl, stoneTask->effect->anmRes[1], 0);
    stoneTask->anms[1] = GFL_G3DAnmCreate(stoneTask->stoneMdl, stoneTask->effect->anmRes[2], 0);
    stoneTask->stoneActor = GFL_G3DActorCreate(stoneTask->stoneMdl, stoneTask->anms, 2);
    GFL_G3DActorBindAnm(stoneTask->stoneActor, 0);
    GFL_G3DActorBindAnm(stoneTask->stoneActor, 1);
    stoneTask->glowMdl = GFL_G3DMdlCreate(stoneTask->effect->mdlRes[3], 0, stoneTask->effect->mdlRes[3]);
    stoneTask->glowActor = GFL_G3DActorCreate(stoneTask->glowMdl, NULL, 0);
}

static void StoneTask_Delete(FieldEffectTask *task, void *work) {
    StoneTask *stoneTask = work;

    GFL_G3DAnmFree(stoneTask->anms[0]);
    GFL_G3DAnmFree(stoneTask->anms[1]);
    GFL_G3DActorFree(stoneTask->stoneActor);
    GFL_G3DMdlFree(stoneTask->stoneMdl);
    GFL_G3DActorFree(stoneTask->glowActor);
    GFL_G3DMdlFree(stoneTask->glowMdl);
}

static void StoneTask_Update(FieldEffectTask *task, void *work) {
    StoneTask *stoneTask = work;

    if (FldAct_IsInUse(stoneTask->actor)) {
        VecFx32 pos;

        CopyActorWPos(stoneTask->actor, &pos);
        stoneTask->glowPos = pos;
        FieldEffects_SetObjectPos(task, &pos);
        stoneTask->glowPos.y -= FX32_CONST(4);
        stoneTask->glowPos.z += FX32_CONST(2.5);
        if (stoneTask->paused == FALSE) {
            GFL_G3DActorStepAnmFrameLoop(stoneTask->stoneActor, 0, FX32_ONE);
        }
        if (stoneTask->lit == TRUE && stoneTask->litDone == FALSE) {
            if (GFL_G3DActorStepAnmFrame(stoneTask->stoneActor, 1, FX32_ONE) == FALSE) {
                stoneTask->litDone = TRUE;
            }
        }
    }
}

static void StoneTask_Draw(FieldEffectTask *task, void *work) {
    SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    StoneTask *stoneTask = work;

    MTX_Identity33_(&srt.rotation);
    FieldEffects_GetObjectPos(task, &srt.translation);
    GFL_G3DSysDrawObjBBoxCull(stoneTask->stoneActor, &srt);
    srt.translation = stoneTask->glowPos;
    GFL_G3DSysDrawObjBBoxCull(stoneTask->glowActor, &srt);
}

FieldEffectTask *StoneTask_StartPulse(FieldEffects *effects, const VecFx32 *pos) {
    SparkTaskParams params;

    params.effect = FieldEffects_GetHandleData(effects, FLDEFF_D06_ELECTRIC);
    params.pos = *pos;
    return FieldEffects_TCBCreate(effects, &MARK_TASK_VTABLE, NULL, 0, &params, 0);
}

void StoneTask_PulseTrigger(FieldEffectTask *task) {
    if (task != NULL) {
        MarkTask *markTask = FieldEffects_GetObjectWork(task);

        if (markTask->lit == FALSE) {
            fx32 frame = 0;

            markTask->lit = TRUE;
            GFL_G3DActorSetAnmFrame(markTask->actor, 1, &frame);
        }
    }
}

static void MarkTask_Init(FieldEffectTask *task, void *work) {
    MarkTask *markTask = work;
    SparkTaskParams *params = FieldEffects_GetObjectParam(task);

    *(SparkTaskParams *)markTask = *params;
    FieldEffects_SetObjectPos(task, &markTask->pos);
    markTask->mdl = GFL_G3DMdlCreate(markTask->effect->mdlRes[2], 0, markTask->effect->mdlRes[2]);
    markTask->anms[0] = GFL_G3DAnmCreate(markTask->mdl, markTask->effect->anmRes[3], 0);
    markTask->anms[1] = GFL_G3DAnmCreate(markTask->mdl, markTask->effect->anmRes[4], 0);
    markTask->actor = GFL_G3DActorCreate(markTask->mdl, markTask->anms, 2);
    GFL_G3DActorBindAnm(markTask->actor, 0);
    GFL_G3DActorBindAnm(markTask->actor, 1);
}

static void MarkTask_Delete(FieldEffectTask *task, void *work) {
    MarkTask *markTask = work;

    GFL_G3DAnmFree(markTask->anms[0]);
    GFL_G3DAnmFree(markTask->anms[1]);
    GFL_G3DActorFree(markTask->actor);
    GFL_G3DMdlFree(markTask->mdl);
}

static void MarkTask_Update(FieldEffectTask *task, void *work) {
    MarkTask *markTask = work;

    GFL_G3DActorStepAnmFrameLoop(markTask->actor, 0, FX32_ONE);
    if (markTask->lit == TRUE) {
        if (GFL_G3DActorStepAnmFrame(markTask->actor, 1, FX32_ONE) == FALSE) {
            markTask->lit = FALSE;
        }
    }
}

static void MarkTask_Draw(FieldEffectTask *task, void *work) {
    SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    MarkTask *markTask = work;

    MTX_Identity33_(&srt.rotation);
    FieldEffects_GetObjectPos(task, &srt.translation);
    GFL_G3DSysDrawObjBBoxCull(markTask->actor, &srt);
}
