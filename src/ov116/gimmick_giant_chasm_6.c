#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_exp_obj.h"
#include "field/field_task.h"
#include "field/gimmick_giant_chasm_6.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_system.h"

#define GIMMICK_WORK_ID 1

// The models of the scene
enum {
    MODEL_0,
    MODEL_1,
    MODEL_2,
    MODEL_3,
    MODEL_COUNT,
};

// Which models' animations play, and the sounds that go with them
enum {
    EFFECT_NONE,
    EFFECT_MODEL_0,
    EFFECT_MODEL_1,
    EFFECT_MODEL_2_OR_3,
};

// The effect stops after this many frames
#define EFFECT_FRAMES 500

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    u32 effect;
    u32 frame;
} GiantChasm6Work;

// The task that makes an actor hover
typedef struct {
    int frame;
    FieldActor *actor;
    Field *field;
    u32 actorId;
} HoverTaskWork;

static void GimmickGC6_Init(GiantChasm6Work *work);
static void GimmickGC6_Free(GiantChasm6Work *work);
static void GimmickGC6_StepAnimations(GiantChasm6Work *work);
static void GimmickGC6_PlayModel(FieldExpObjSystem *expObj, u16 model);
static void GimmickGC6_StartHover(GiantChasm6Work *work, FieldActor *actor, u32 actorId);
static BOOL GimmickGC6_HoverTask(void *data);

static const G3DSceneAnimationSetup sModel3Animations[] = { { 10, 0 }, { 11, 0 } };
static const G3DSceneAnimationSetup sModel0Animations[] = { { 1, 0 }, { 2, 0 } };
static const G3DSceneAnimationSetup sModel1Animations[] = { { 4, 0 }, { 5, 0 } };
static const G3DSceneAnimationSetup sModel2Animations[] = { { 7, 0 }, { 8, 0 } };

// White 2's archive has one member fewer ahead of two of the files
static const G3DSceneResourceSetup sResources[12] = {
    { ARCID_GIMMICK_GIANT_CHASM_6, 0, 0 },  { ARCID_GIMMICK_GIANT_CHASM_6, 1, 0 },
    { ARCID_GIMMICK_GIANT_CHASM_6, 2, 0 },  { ARCID_GIMMICK_GIANT_CHASM_6, 3, 0 },
#ifdef WHITE2
    { ARCID_GIMMICK_GIANT_CHASM_6, 4, 0 },  { ARCID_GIMMICK_GIANT_CHASM_6, 5, 0 },
    { ARCID_GIMMICK_GIANT_CHASM_6, 10, 0 }, { ARCID_GIMMICK_GIANT_CHASM_6, 11, 0 },
    { ARCID_GIMMICK_GIANT_CHASM_6, 12, 0 }, { ARCID_GIMMICK_GIANT_CHASM_6, 6, 0 },
    { ARCID_GIMMICK_GIANT_CHASM_6, 7, 0 },  { ARCID_GIMMICK_GIANT_CHASM_6, 8, 0 },
#else
    { ARCID_GIMMICK_GIANT_CHASM_6, 4, 0 },  { ARCID_GIMMICK_GIANT_CHASM_6, 5, 0 },
    { ARCID_GIMMICK_GIANT_CHASM_6, 10, 0 }, { ARCID_GIMMICK_GIANT_CHASM_6, 11, 0 },
    { ARCID_GIMMICK_GIANT_CHASM_6, 13, 0 }, { ARCID_GIMMICK_GIANT_CHASM_6, 6, 0 },
    { ARCID_GIMMICK_GIANT_CHASM_6, 7, 0 },  { ARCID_GIMMICK_GIANT_CHASM_6, 9, 0 },
#endif
};

static const G3DSceneActorSetup sActors[MODEL_COUNT] = {
    { 0, 0, 0, 0, sModel0Animations, NELEMS(sModel0Animations) },
    { 3, 0, 3, 0, sModel1Animations, NELEMS(sModel1Animations) },
    { 6, 0, 6, 0, sModel2Animations, NELEMS(sModel2Animations) },
    { 9, 0, 9, 0, sModel3Animations, NELEMS(sModel3Animations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

static const int sAnimationCounts[MODEL_COUNT] = { 2, 2, 2, 2 };

void GimmickGC6_Setup(Field *field) {
    GiantChasm6Work *work;

    Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(GiantChasm6Work));
    work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    work->field = field;
    work->expObj = Field_GetExpObjSystem(field);
    GimmickGC6_Init(work);
}

void GimmickGC6_End(Field *field) {
    GimmickGC6_Free(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

// Plays the sounds of the running effect, by the frame it is at
void GimmickGC6_Update(Field *field) {
    GiantChasm6Work *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    if (work->effect != EFFECT_NONE) {
        if (work->effect == EFFECT_MODEL_0) {
            switch (work->frame) {
            case 0:
                GFL_SndSEPlay(SEQ_SE_SW_GIANTHOLE_01);
                break;
            case 120:
                GFL_SndSEPlay(SEQ_SE_SW_GIANTHOLE_02);
                break;
            case 290:
                GFL_SndSEPlay(SEQ_SE_SW_GIANTHOLE_03);
                break;
            case 370:
                GFL_SndSEPlay(SEQ_SE_SW_GIANTHOLE_04);
                break;
            case 480:
                GFL_SndSEPlay(SEQ_SE_SW_GIANTHOLE_05);
                break;
            case 498:
                GFL_SndSEPlay(SEQ_SE_SW_GIANTHOLE_06);
                break;
            }
        } else if (work->effect == EFFECT_MODEL_1) {
            if (work->frame == 5) {
                GFL_SndSEPlay(SEQ_SE_SW_GIANTHOLE_09);
            }
        } else if (work->effect == EFFECT_MODEL_2_OR_3) {
            switch (work->frame) {
            case 10:
            case 35:
            case 60:
            case 85:
                GFL_SndSEPlay(SEQ_SE_FLD_167);
                break;
            case 13:
            case 38:
            case 63:
            case 88:
                GFL_SndSEPlay(SEQ_SE_FLD_168);
                break;
            }
        }
        work->frame++;
        if (work->frame > EFFECT_FRAMES) {
            work->effect = EFFECT_NONE;
        }
    }
    GimmickGC6_StepAnimations(work);
}

void GimmickGC6_ShowModel01(GameSystem *gsys, u16 which) {
    GiantChasm6Work *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    if (which == 0) {
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, FALSE);
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_1, TRUE);
    } else {
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_1, FALSE);
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, TRUE);
    }
}

void GimmickGC6_PlayModel01(GameSystem *gsys, u16 which) {
    GiantChasm6Work *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    work->effect = EFFECT_NONE;
    work->frame = 0;
    if (which == 0) {
        GimmickGC6_PlayModel(work->expObj, MODEL_0);
        work->effect = EFFECT_MODEL_0;
    } else {
        GimmickGC6_PlayModel(work->expObj, MODEL_1);
        work->effect = EFFECT_MODEL_1;
    }
}

void GimmickGC6_CallStartHover(GameSystem *gsys, u32 actorId) {
    Field *field = GSYS_GetField(gsys);
    GiantChasm6Work *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    GimmickGC6_StartHover(work, FindFieldActor(Field_GetActorSystem(field), actorId), actorId);
}

void GimmickGC6_HideModel3(GameSystem *gsys) {
    GiantChasm6Work *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    u32 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_3, TRUE);
    for (i = 0; i < 2; i++) {
        FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_3, i), TRUE);
        FieldExpObj_SetAnm(work->expObj, 0, MODEL_3, i, FALSE);
    }
}

void GimmickGC6_PlayModel23(GameSystem *gsys, BOOL which) {
    GiantChasm6Work *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    work->effect = EFFECT_MODEL_2_OR_3;
    work->frame = 0;
    if (which == TRUE) {
        GimmickGC6_PlayModel(work->expObj, MODEL_2);
    } else {
        GimmickGC6_PlayModel(work->expObj, MODEL_3);
    }
}

// Places the model, hidden, with its animations set and paused
static inline void GimmickGC6_InitModel(GiantChasm6Work *work, u16 model, fx32 x, fx32 z) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    u32 i;

    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, model);
    matrix->translation.x = x;
    matrix->translation.y = 0;
    matrix->translation.z = z;
    FieldExpObj_SetActorCulling(work->expObj, 0, model, 0);
    FieldExpObj_SetActorHidden(work->expObj, 0, model, TRUE);
    for (i = 0; i < 2; i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(work->expObj, 0, model, i, FALSE);
    }
}
static void GimmickGC6_Init(GiantChasm6Work *work) {
    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    GimmickGC6_InitModel(work, MODEL_0, FX32_CONST(256), FX32_CONST(256));
    GimmickGC6_InitModel(work, MODEL_1, FX32_CONST(256), FX32_CONST(256));
    GimmickGC6_InitModel(work, MODEL_2, FX32_CONST(248), FX32_CONST(312));
    GimmickGC6_InitModel(work, MODEL_3, FX32_CONST(248), FX32_CONST(312));
}

static void GimmickGC6_Free(GiantChasm6Work *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void GimmickGC6_StepAnimations(GiantChasm6Work *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Shows the model and plays its animations once from the start
static void GimmickGC6_PlayModel(FieldExpObjSystem *expObj, u16 model) {
    FieldExpObjAnm *anm;
    int count = sAnimationCounts[model];
    int i;

    FieldExpObj_SetActorHidden(expObj, 0, model, FALSE);
    for (i = 0; i < count; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, model, i);
        FieldExpObj_SetAnm(expObj, 0, model, i, TRUE);
        FieldExpObj_SetAnmFrame(expObj, 0, model, i, 0);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}

static void GimmickGC6_StartHover(GiantChasm6Work *work, FieldActor *actor, u32 actorId) {
    HeapID heapId = Field_GetHeapID(work->field);
    FieldTaskManager *manager = Field_GetTaskManager(work->field);
    FieldTask *task = FieldTask_Create(heapId, sizeof(HoverTaskWork), GimmickGC6_HoverTask);
    HoverTaskWork *hover = FieldTask_GetData(task);

    hover->actor = actor;
    hover->frame = 0;
    hover->field = work->field;
    hover->actorId = actorId;
    FieldTaskManager_AddTask(manager, task, 0);
}

// Bobs the actor up and down, a swing every 35 frames, until the actor is gone
static BOOL GimmickGC6_HoverTask(void *data) {
    HoverTaskWork *hover = data;
    VecFx32 offset;
    u16 angle;
    fx16 sin;

    if (FindFieldActor(Field_GetActorSystem(hover->field), hover->actorId) == NULL) {
        return TRUE;
    }
    hover->frame++;
    angle = (hover->frame << 16) / 35;
    sin = FX_SinIdx(angle);
    offset.x = 0;
    offset.y = sin * 3 + FX32_CONST(13.5);
    offset.z = 0;
    SetActorWPosOffset(hover->actor, &offset);
    return FALSE;
}
