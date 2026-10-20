// The electric effects of Chargestone Cave (d06denki): the sparks over a stone and the glow of a stone's mark. The name
// is the ROM's own, from GFL_HeapAllocate's file argument.
#include "types.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"

#define D06DENKI_MDL_COUNT 4
#define D06DENKI_ANM_COUNT 5

typedef struct {
    FieldEffects *effects;
    void *mdlRes[D06DENKI_MDL_COUNT];
    void *anmRes[D06DENKI_ANM_COUNT];
    void *anm;
    G3DModel *mdl;
    G3DActor *actor;
} FieldEffectD06Denki;

// What the sparks over a stone start from
typedef struct {
    FieldEffectD06Denki *effect;
    VecFx32 pos;
} SparkTaskParams;

typedef struct {
    FieldEffectD06Denki *effect;
    fx32 frame;
    // Set once the sparks' animation has ended
    BOOL done;
} SparkTask;

// What a stone's effect starts from
typedef struct {
    FieldEffectD06Denki *effect;
    VecFx32 pos;
    FieldActor *actor;
} StoneTaskParams;

typedef struct {
    FieldEffectD06Denki *effect;
    VecFx32 pos;
    FieldActor *actor;
    u8 paused;
    u8 lit;
    u8 litDone;
    G3DActor *stoneActor;
    void *anms[2];
    G3DModel *stoneMdl;
    VecFx32 glowPos;
    G3DActor *glowActor;
    G3DModel *glowMdl;
} StoneTask;

typedef struct {
    FieldEffectD06Denki *effect;
    VecFx32 pos;
    u8 lit;
    G3DActor *actor;
    void *anms[2];
    G3DModel *mdl;
} MarkTask;

static void func_ov036_021c27c0(FieldEffectD06Denki *effect);
static void func_ov036_021c2858(FieldEffectD06Denki *effect);
void FieldTerrainEffectElec_Init(FieldEffectTask *task, void *work);
static void func_ov036_021c28fc(FieldEffectTask *task, void *work);
static void func_ov036_021c2900(FieldEffectTask *task, void *work);
static void func_ov036_021c2910(FieldEffectTask *task, void *work);
static void func_ov036_021c2a04(FieldEffectTask *task, void *work);
static void func_ov036_021c2a84(FieldEffectTask *task, void *work);
static void func_ov036_021c2ab0(FieldEffectTask *task, void *work);
static void func_ov036_021c2b28(FieldEffectTask *task, void *work);
static void func_ov036_021c2bd0(FieldEffectTask *task, void *work);
static void func_ov036_021c2c34(FieldEffectTask *task, void *work);
static void func_ov036_021c2c54(FieldEffectTask *task, void *work);
static void func_ov036_021c2c80(FieldEffectTask *task, void *work);

void *func_ov036_021c2788(FieldEffects *effects, HeapID heapId) {
    FieldEffectD06Denki *effect = GFL_HeapAllocate(heapId, sizeof(FieldEffectD06Denki), TRUE, "fldeff_d06denki.c", 169);

    effect->effects = effects;
    func_ov036_021c27c0(effect);
    return effect;
}

void func_ov036_021c27ac(FieldEffects *effects, void *data) {
    func_ov036_021c2858(data);
    GFL_HeapFree(data);
}

static void func_ov036_021c27c0(FieldEffectD06Denki *effect) {
    u32 mdlIds[D06DENKI_MDL_COUNT] = { 80, 78, 79, 81 };
    u32 anmIds[D06DENKI_ANM_COUNT] = { 106, 124, 125, 126, 127 };
    u32 i;
    ArcTool *arc = FieldEffects_GetArc(effect->effects);

    for (i = 0; i < D06DENKI_MDL_COUNT; i++) {
        effect->mdlRes[i] = GFL_G3DSysReadArcToolResource(arc, mdlIds[i]);
        GFL_G3DResUploadTexData(effect->mdlRes[i]);
    }
    for (i = 0; i < D06DENKI_ANM_COUNT; i++) {
        effect->anmRes[i] = GFL_G3DSysReadArcToolResource(arc, anmIds[i]);
    }
    effect->mdl = GFL_G3DMdlCreate(effect->mdlRes[0], 0, effect->mdlRes[0]);
    effect->anm = GFL_G3DAnmCreate(effect->mdl, effect->anmRes[0], 0);
    effect->actor = GFL_G3DActorCreate(effect->mdl, &effect->anm, 1);
    GFL_G3DActorBindAnm(effect->actor, 0);
}

static void func_ov036_021c2858(FieldEffectD06Denki *effect) {
    u32 i;

    GFL_G3DAnmFree(effect->anm);
    GFL_G3DActorFree(effect->actor);
    GFL_G3DMdlFree(effect->mdl);
    for (i = 0; i < D06DENKI_MDL_COUNT; i++) {
        GFL_G3DResFreeTexData(effect->mdlRes[i]);
        GFL_G3DResFree(effect->mdlRes[i]);
    }
    for (i = 0; i < D06DENKI_ANM_COUNT; i++) {
        GFL_G3DResFree(effect->anmRes[i]);
    }
}

static const FieldEffectTaskVTable STONE_TASK_VTABLE = {
    sizeof(StoneTask), func_ov036_021c2a04, func_ov036_021c2a84, func_ov036_021c2ab0, func_ov036_021c2b28,
};

static const FieldEffectTaskVTable SPARK_TASK_VTABLE = {
    sizeof(SparkTask), FieldTerrainEffectElec_Init, func_ov036_021c28fc, func_ov036_021c2900, func_ov036_021c2910,
};

static const FieldEffectTaskVTable MARK_TASK_VTABLE = {
    sizeof(MarkTask), func_ov036_021c2bd0, func_ov036_021c2c34, func_ov036_021c2c54, func_ov036_021c2c80,
};

void func_ov036_021c289c(FieldActor *actor, void *effects) {
    VecFx32 pos;
    SparkTaskParams params;

    CopyActorWPos(actor, &pos);
    params.effect = FieldEffects_GetHandleData(effects, 10);
    params.pos = pos;
    FieldEffects_TCBCreate(effects, &SPARK_TASK_VTABLE, NULL, 0, &params, 0);
}

void FieldTerrainEffectElec_Init(FieldEffectTask *task, void *work) {
    SparkTask *sparkTask = work;
    SparkTaskParams *params = func_ov036_021a3ac8(task);

    sparkTask->effect = params->effect;
    func_ov036_021a3ae8(task, &params->pos);
    GFL_SndSEPlay(1698);
}

static void func_ov036_021c28fc(FieldEffectTask *task, void *work) {
}

static void func_ov036_021c2900(FieldEffectTask *task, void *work) {
    SparkTask *sparkTask = work;

    if (sparkTask->done == TRUE) {
        func_ov036_021a3a70(task);
    }
}

static void func_ov036_021c2910(FieldEffectTask *task, void *work) {
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
        func_ov036_021a3ad4(task, &srt.translation);
        GFL_G3DSysDrawObjBBoxCull(actor, &srt);
    }
}

FieldEffectTask *func_ov036_021c2984(FieldEffects *effects, const VecFx32 *pos, FieldActor *actor) {
    StoneTaskParams params;

    params.effect = FieldEffects_GetHandleData(effects, 10);
    params.pos = *pos;
    params.actor = actor;
    return FieldEffects_TCBCreate(effects, &STONE_TASK_VTABLE, NULL, 0, &params, 0);
}

void func_ov036_021c29bc(FieldEffectTask *task) {
    if (task != NULL) {
        StoneTask *stoneTask = func_ov036_021a3afc(task);

        stoneTask->paused = TRUE;
    }
}

void func_ov036_021c29cc(FieldEffectTask *task) {
    if (task != NULL) {
        fx32 frame = 0;
        StoneTask *stoneTask = func_ov036_021a3afc(task);

        stoneTask->lit = TRUE;
        stoneTask->litDone = FALSE;
        GFL_G3DActorSetAnmFrame(stoneTask->stoneActor, 1, &frame);
    }
}

u8 func_ov036_021c29f0(FieldEffectTask *task) {
    if (task != NULL) {
        StoneTask *stoneTask = func_ov036_021a3afc(task);

        return stoneTask->litDone;
    }
    return TRUE;
}

static void func_ov036_021c2a04(FieldEffectTask *task, void *work) {
    StoneTask *stoneTask = work;
    StoneTaskParams *params = func_ov036_021a3ac8(task);

    *(StoneTaskParams *)stoneTask = *params;
    func_ov036_021a3ae8(task, &stoneTask->pos);
    stoneTask->stoneMdl = GFL_G3DMdlCreate(stoneTask->effect->mdlRes[1], 0, stoneTask->effect->mdlRes[1]);
    stoneTask->anms[0] = GFL_G3DAnmCreate(stoneTask->stoneMdl, stoneTask->effect->anmRes[1], 0);
    stoneTask->anms[1] = GFL_G3DAnmCreate(stoneTask->stoneMdl, stoneTask->effect->anmRes[2], 0);
    stoneTask->stoneActor = GFL_G3DActorCreate(stoneTask->stoneMdl, stoneTask->anms, 2);
    GFL_G3DActorBindAnm(stoneTask->stoneActor, 0);
    GFL_G3DActorBindAnm(stoneTask->stoneActor, 1);
    stoneTask->glowMdl = GFL_G3DMdlCreate(stoneTask->effect->mdlRes[3], 0, stoneTask->effect->mdlRes[3]);
    stoneTask->glowActor = GFL_G3DActorCreate(stoneTask->glowMdl, NULL, 0);
}

static void func_ov036_021c2a84(FieldEffectTask *task, void *work) {
    StoneTask *stoneTask = work;

    GFL_G3DAnmFree(stoneTask->anms[0]);
    GFL_G3DAnmFree(stoneTask->anms[1]);
    GFL_G3DActorFree(stoneTask->stoneActor);
    GFL_G3DMdlFree(stoneTask->stoneMdl);
    GFL_G3DActorFree(stoneTask->glowActor);
    GFL_G3DMdlFree(stoneTask->glowMdl);
}

static void func_ov036_021c2ab0(FieldEffectTask *task, void *work) {
    StoneTask *stoneTask = work;

    if (FldAct_IsInUse(stoneTask->actor)) {
        VecFx32 pos;

        CopyActorWPos(stoneTask->actor, &pos);
        stoneTask->glowPos = pos;
        func_ov036_021a3ae8(task, &pos);
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

static void func_ov036_021c2b28(FieldEffectTask *task, void *work) {
    SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    StoneTask *stoneTask = work;

    MTX_Identity33_(&srt.rotation);
    func_ov036_021a3ad4(task, &srt.translation);
    GFL_G3DSysDrawObjBBoxCull(stoneTask->stoneActor, &srt);
    srt.translation = stoneTask->glowPos;
    GFL_G3DSysDrawObjBBoxCull(stoneTask->glowActor, &srt);
}

FieldEffectTask *func_ov036_021c2b78(FieldEffects *effects, const VecFx32 *pos) {
    SparkTaskParams params;

    params.effect = FieldEffects_GetHandleData(effects, 10);
    params.pos = *pos;
    return FieldEffects_TCBCreate(effects, &MARK_TASK_VTABLE, NULL, 0, &params, 0);
}

void func_ov036_021c2bac(FieldEffectTask *task) {
    if (task != NULL) {
        MarkTask *markTask = func_ov036_021a3afc(task);

        if (markTask->lit == FALSE) {
            fx32 frame = 0;

            markTask->lit = TRUE;
            GFL_G3DActorSetAnmFrame(markTask->actor, 1, &frame);
        }
    }
}

static void func_ov036_021c2bd0(FieldEffectTask *task, void *work) {
    MarkTask *markTask = work;
    SparkTaskParams *params = func_ov036_021a3ac8(task);

    *(SparkTaskParams *)markTask = *params;
    func_ov036_021a3ae8(task, &markTask->pos);
    markTask->mdl = GFL_G3DMdlCreate(markTask->effect->mdlRes[2], 0, markTask->effect->mdlRes[2]);
    markTask->anms[0] = GFL_G3DAnmCreate(markTask->mdl, markTask->effect->anmRes[3], 0);
    markTask->anms[1] = GFL_G3DAnmCreate(markTask->mdl, markTask->effect->anmRes[4], 0);
    markTask->actor = GFL_G3DActorCreate(markTask->mdl, markTask->anms, 2);
    GFL_G3DActorBindAnm(markTask->actor, 0);
    GFL_G3DActorBindAnm(markTask->actor, 1);
}

static void func_ov036_021c2c34(FieldEffectTask *task, void *work) {
    MarkTask *markTask = work;

    GFL_G3DAnmFree(markTask->anms[0]);
    GFL_G3DAnmFree(markTask->anms[1]);
    GFL_G3DActorFree(markTask->actor);
    GFL_G3DMdlFree(markTask->mdl);
}

static void func_ov036_021c2c54(FieldEffectTask *task, void *work) {
    MarkTask *markTask = work;

    GFL_G3DActorStepAnmFrameLoop(markTask->actor, 0, FX32_ONE);
    if (markTask->lit == TRUE) {
        if (GFL_G3DActorStepAnmFrame(markTask->actor, 1, FX32_ONE) == FALSE) {
            markTask->lit = FALSE;
        }
    }
}

static void func_ov036_021c2c80(FieldEffectTask *task, void *work) {
    SRTMatrix srt = { { 0, 0, 0 }, { FX32_ONE, FX32_ONE, FX32_ONE } };
    MarkTask *markTask = work;

    MTX_Identity33_(&srt.rotation);
    func_ov036_021a3ad4(task, &srt.translation);
    GFL_G3DSysDrawObjBBoxCull(markTask->actor, &srt);
}
