// The field gimmick of the special Pokémon's zones, 53 and 614 (overlay 129), which script plugin 16 drives. The ROM
// embeds this file's name, sp_poke_gimmick.c. Its sound effects are Keldeo's form change, so the gimmick is probably
// the stage of that event. Four gimmicks (A to D here, in address order) differ in which scene of 3D models they add;
// each keeps its state in work block 1, which holds the field's expansion object system
#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_exp_obj.h"
#include "field/field_task.h"
#include "field/sp_poke_gimmick.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_event.h"
#include "system/game_system.h"

#define WORK_ID 1
#define WORK_SIZE 0x30

#define ARCID_SP_POKE_ACTORS 201
#define ARCID_SP_POKE_CURVE 305
#define ARCID_SP_POKE_SCENE_C 214
#define ARCID_SP_POKE_SCENE_D 304

// The center of a grid square, in fx32
#define GRID_CENTER(n) ((n) * FX32_CONST(16) + FX32_CONST(8))

#define ACTOR_MOVEMENT_FLAG_SP_POKE 0x8000

#define OBJ_CODE_SP_POKE_STILL 123
#define OBJ_CODE_SP_POKE_SWORD 374

#define CURVE_ARC_FILE 3
#define CURVE_TYPE 10

enum {
    CURVE_STATE_IDLE,
    CURVE_STATE_RUN,
};

// Gimmick B's work of actor 0, 0x198 bytes: it runs a Pokémon of the zone along a curve of archive 305
struct SpPokeGimmickCurve {
    FieldActor *actor;
    u8 buffer[0x180];
    G3DCurve *curve;
    VecFx32 basePos;
    u8 state;
    u8 phase;
};

// What the task that lowers the actor to the ground keeps
typedef struct {
    FieldActor *actor;
    VecFx32 start;
    VecFx32 end;
} SpPokeDropTask;

typedef struct {
    FieldExpObjSystem *system;
    Field *field;
    s32 timer;
} SpPokeFormEventWork;

static void SpPokeGimmick_StartDrop(SpPokeGimmickCurve *curve, Field *field);
static BOOL SpPokeGimmick_DropTask(void *data);
static void SpPokeGimmick_InitScene(SpPokeGimmickWork *work);
static void SpPokeGimmick_FreeScene(SpPokeGimmickWork *work);
static void SpPokeGimmick_StepAnimations(SpPokeGimmickWork *work);
static void SpPokeGimmick_PlayAnimation(FieldExpObjSystem *system, u16 actor, u16 anm);
static void SpPokeGimmick_PauseAnimation(FieldExpObjSystem *system, u16 actor, u16 anm);
static void SpPokeGimmick_ShowAndPlay01(FieldExpObjSystem *system, u16 actor);
static BOOL SpPokeGimmick_IsAnimation01Done(FieldExpObjSystem *system, u16 actor);
static void SpPokeGimmick_PlayAnimation23(FieldExpObjSystem *system, u16 actor);
static void SpPokeGimmick_PlayAnimation456(FieldExpObjSystem *system, u16 actor);
static BOOL SpPokeGimmick_IsAnimation456Done(FieldExpObjSystem *system, u16 actor);
static GameEvent *SpPokeGimmick_CreateFormEvent(GameSystem *gsys);
static GameEventReturnCode SpPokeGimmick_FormEvent(GameEvent *event, u32 *state, void *data);

static const G3DSceneAnimationSetup sActor2AnimsB[2] = { { 9, 0 }, { 10, 0 } };
static const G3DSceneAnimationSetup sActor2AnimsD[2] = { { 9, 0 }, { 10, 0 } };
static const G3DSceneAnimationSetup sActor0Anims[3] = { { 2, 0 }, { 3, 0 }, { 4, 0 } };
static const G3DSceneAnimationSetup sActor1Anims[3] = { { 5, 0 }, { 6, 0 }, { 7, 0 } };
static const G3DSceneAnimationSetup sActor2AnimsC[3] = { { 9, 0 }, { 10, 0 }, { 11, 0 } };
static const G3DSceneAnimationSetup sSwordAnims[7] = { { 14, 0 }, { 15, 0 }, { 16, 0 }, { 17, 0 },
                                                       { 18, 0 }, { 19, 0 }, { 20, 0 } };

static const G3DSceneResourceSetup sResourcesA[8] = {
    { ARCID_SP_POKE_ACTORS, 4, 0, G3D_SCENE_RES_ARCSYS }, { ARCID_SP_POKE_ACTORS, 0, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 5, 0, G3D_SCENE_RES_ARCSYS }, { ARCID_SP_POKE_ACTORS, 7, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 6, 0, G3D_SCENE_RES_ARCSYS }, { ARCID_SP_POKE_ACTORS, 1, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 3, 0, G3D_SCENE_RES_ARCSYS }, { ARCID_SP_POKE_ACTORS, 2, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sResourcesB[11] = {
    { ARCID_SP_POKE_ACTORS, 4, 0, G3D_SCENE_RES_ARCSYS }, { ARCID_SP_POKE_ACTORS, 0, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 5, 0, G3D_SCENE_RES_ARCSYS }, { ARCID_SP_POKE_ACTORS, 7, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 6, 0, G3D_SCENE_RES_ARCSYS }, { ARCID_SP_POKE_ACTORS, 1, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 3, 0, G3D_SCENE_RES_ARCSYS }, { ARCID_SP_POKE_ACTORS, 2, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_CURVE, 0, 0, G3D_SCENE_RES_ARCSYS },  { ARCID_SP_POKE_CURVE, 1, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_CURVE, 2, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sResourcesC[12] = {
    { ARCID_SP_POKE_ACTORS, 4, 0, G3D_SCENE_RES_ARCSYS },  { ARCID_SP_POKE_ACTORS, 0, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 5, 0, G3D_SCENE_RES_ARCSYS },  { ARCID_SP_POKE_ACTORS, 7, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 6, 0, G3D_SCENE_RES_ARCSYS },  { ARCID_SP_POKE_ACTORS, 1, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 3, 0, G3D_SCENE_RES_ARCSYS },  { ARCID_SP_POKE_ACTORS, 2, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_SCENE_C, 0, 0, G3D_SCENE_RES_ARCSYS }, { ARCID_SP_POKE_SCENE_C, 1, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_SCENE_C, 3, 0, G3D_SCENE_RES_ARCSYS }, { ARCID_SP_POKE_SCENE_C, 2, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sResourcesD[21] = {
    { ARCID_SP_POKE_ACTORS, 4, 0, G3D_SCENE_RES_ARCSYS },   { ARCID_SP_POKE_ACTORS, 0, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 5, 0, G3D_SCENE_RES_ARCSYS },   { ARCID_SP_POKE_ACTORS, 7, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 6, 0, G3D_SCENE_RES_ARCSYS },   { ARCID_SP_POKE_ACTORS, 1, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_ACTORS, 3, 0, G3D_SCENE_RES_ARCSYS },   { ARCID_SP_POKE_ACTORS, 2, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_SCENE_D, 0, 0, G3D_SCENE_RES_ARCSYS },  { ARCID_SP_POKE_SCENE_D, 1, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_SCENE_D, 2, 0, G3D_SCENE_RES_ARCSYS },  { ARCID_SP_POKE_SCENE_D, 3, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_SCENE_D, 4, 0, G3D_SCENE_RES_ARCSYS },  { ARCID_SP_POKE_SCENE_D, 5, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_SCENE_D, 6, 0, G3D_SCENE_RES_ARCSYS },  { ARCID_SP_POKE_SCENE_D, 7, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_SCENE_D, 8, 0, G3D_SCENE_RES_ARCSYS },  { ARCID_SP_POKE_SCENE_D, 9, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_SCENE_D, 10, 0, G3D_SCENE_RES_ARCSYS }, { ARCID_SP_POKE_SCENE_D, 11, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SP_POKE_SCENE_D, 12, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneActorSetup sActorsA[2] = {
    { 0, 0, 0, 0, sActor0Anims, 3 },
    { 1, 0, 1, 0, sActor1Anims, 3 },
};

static const G3DSceneActorSetup sActorsB[3] = {
    { 0, 0, 0, 0, sActor0Anims, 3 },
    { 1, 0, 1, 0, sActor1Anims, 3 },
    { 8, 0, 8, 0, sActor2AnimsB, 2 },
};

static const G3DSceneActorSetup sActorsC[3] = {
    { 0, 0, 0, 0, sActor0Anims, 3 },
    { 1, 0, 1, 0, sActor1Anims, 3 },
    { 8, 0, 8, 0, sActor2AnimsC, 3 },
};

static const G3DSceneActorSetup sActorsD[6] = {
    { 0, 0, 0, 0, sActor0Anims, 3 },  { 1, 0, 1, 0, sActor1Anims, 3 },  { 8, 0, 8, 0, sActor2AnimsD, 2 },
    { 11, 0, 11, 0, sSwordAnims, 7 }, { 12, 0, 12, 0, sSwordAnims, 7 }, { 13, 0, 13, 0, sSwordAnims, 7 },
};

static const G3DSceneSetup sSceneD = { sResourcesD, NELEMS(sResourcesD), sActorsD, NELEMS(sActorsD) };
static const G3DSceneSetup sSceneA = { sResourcesA, NELEMS(sResourcesA), sActorsA, NELEMS(sActorsA) };
static const G3DSceneSetup sSceneC = { sResourcesC, NELEMS(sResourcesC), sActorsC, NELEMS(sActorsC) };
static const G3DSceneSetup sSceneB = { sResourcesB, NELEMS(sResourcesB), sActorsB, NELEMS(sActorsB) };

// The animations of each actor of scene D
static const s32 sActorAnimCounts[6] = { 3, 3, 2, 7, 7, 7 };

// Where scene D's actors start
static const VecFx32 sActorPositions[6] = {
    { 0, 0, 0 },
    { 0, 0, 0 },
    { GRID_CENTER(15), 0, GRID_CENTER(13) },
    { GRID_CENTER(17), 0, GRID_CENTER(13) },
    { GRID_CENTER(15), 6 * FX32_CONST(8), GRID_CENTER(13) },
    { GRID_CENTER(13), 0, GRID_CENTER(13) },
};

// Sets up an animation of a scene 0 actor to start paused and not looping, at its first frame
static inline void SpPokeGimmick_InitAnimation(FieldExpObjSystem *system, u16 actor, u16 anm) {
    FieldExpObjAnm *info = FieldExpObj_GetAnmInfo(system, 0, actor, anm);

    FieldExpObjAnm_SetLooped(info, FALSE);
    FieldExpObjAnm_SetPaused(info, TRUE);
    FieldExpObj_SetAnm(system, 0, actor, anm, FALSE);
}

void SpPokeGimmickA_Init(Field *field) {
    SpPokeGimmickWork *work;
    int i;
    FieldExpObjSystem *system;

    system = Field_GetExpObjSystem(field);
    Field_AllocGimmickWorkBlock(field, WORK_ID, Field_GetHeapID(field), WORK_SIZE);
    work = Field_GetGimmickWorkBlock(field, WORK_ID);
    work->field = field;
    work->system = Field_GetExpObjSystem(field);
    LoadFieldExpandObjData(system, &sSceneA, 0);
    FieldExpObj_SetActorHidden(system, 0, 1, TRUE);
    FieldExpObj_SetActorHidden(system, 0, 0, TRUE);
    for (i = 0; i < 3; i++) {
        SpPokeGimmick_InitAnimation(system, 0, i);
        SpPokeGimmick_InitAnimation(system, 1, i);
    }
}

void SpPokeGimmickA_End(Field *field) {
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);

    Field_DeleteGimmickWorkBlock(field, WORK_ID);
    FieldExpObj_FreeScene(system, 0);
}

void SpPokeGimmickA_Update(Field *field) {
    FieldExpObj_StepAllAnimations(Field_GetExpObjSystem(field));
}

void SpPokeGimmickB_Init(Field *field) {
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);
    HeapID heapId = Field_GetHeapID(field);
    SpPokeGimmickWork *work;
    int i;
    u32 j;

    Field_AllocGimmickWorkBlock(field, WORK_ID, Field_GetHeapID(field), WORK_SIZE);
    work = Field_GetGimmickWorkBlock(field, WORK_ID);
    work->data.curve =
        GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(SpPokeGimmickCurve), TRUE, "sp_poke_gimmick.c", 697);
    work->field = field;
    work->system = Field_GetExpObjSystem(field);
    FieldExpObj_AddScene(system, &sSceneB, 0);
    FieldExpObj_SetActorHidden(system, 0, 1, TRUE);
    FieldExpObj_SetActorHidden(system, 0, 0, TRUE);
    for (i = 0; i < 3; i++) {
        SpPokeGimmick_InitAnimation(system, 0, i);
        SpPokeGimmick_InitAnimation(system, 1, i);
    }
    FieldExpObj_SetActorHidden(system, 0, 2, TRUE);
    for (j = 0; j < 2; j++) {
        SpPokeGimmick_InitAnimation(system, 2, j);
    }
}

void SpPokeGimmickB_End(Field *field) {
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);

    GFL_HeapFree(work->data.curve);
    Field_DeleteGimmickWorkBlock(field, WORK_ID);
    FieldExpObj_FreeScene(system, 0);
}

void SpPokeGimmickB_Update(Field *field) {
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);
    SpPokeGimmickCurve *curve = work->data.curve;
    VecFx32 pos;
    u32 i;

    switch (curve->state) {
    case CURVE_STATE_IDLE:
        break;
    case CURVE_STATE_RUN:
        if (!GFL_G3DCurveFrameStepLoop(curve->curve, FX32_ONE)) {
            GFL_G3DCurveGetNowTranslation(curve->curve, &pos);
            VEC_Add(&pos, &curve->basePos, &pos);
            SetActorWPosValue(curve->actor, &pos);
            if (curve->phase != 0) {
                SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(system, 0, 2);
                VecFx32 actorPos;

                CopyActorWPos(curve->actor, &actorPos);
                matrix->translation = actorPos;
                if (func_ov036_021b8520(system, 0, 2, 0) == FX32_CONST(12)) {
                    if (curve->phase == 2) {
                        FldAct_UpdateBlInfoForNewObjCode(curve->actor, OBJ_CODE_SP_POKE_SWORD);
                    } else {
                        FldAct_UpdateBlInfoForNewObjCode(curve->actor, OBJ_CODE_SP_POKE_STILL);
                    }
                    SetActorMovementFlag(curve->actor, ACTOR_MOVEMENT_FLAG_SP_POKE);
                }
                if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(system, 0, 2, 0)) == TRUE) {
                    FieldExpObj_SetActorHidden(system, 0, 2, TRUE);
                    curve->phase = 0;
                }
            }
            switch (GFL_G3DCurveGetNowFrame(curve->curve) >> FX32_SHIFT) {
            case 554:
                curve->phase++;
                // fall through
            case 835:
                curve->phase++;
                CopyActorWPos(curve->actor, &FieldExpObj_GetActorMatrixPtr(system, 0, 2)->translation);
                FieldExpObj_SetActorHidden(system, 0, 2, FALSE);
                for (i = 0; i < 2; i++) {
                    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(system, 0, 2, i);

                    FieldExpObjAnm_SetLooped(anm, FALSE);
                    FieldExpObjAnm_SetPaused(anm, FALSE);
                    FieldExpObj_SetAnm(system, 0, 2, i, TRUE);
                    FieldExpObj_SetAnmFrame(system, 0, 2, i, 0);
                }
                break;
            }
        } else {
            GFL_G3DCurveFree(curve->curve);
            curve->curve = NULL;
            curve->state = CURVE_STATE_IDLE;
        }
        FldAct_InvokeUpdateCallback(curve->actor);
        break;
    }
    FieldExpObj_StepAllAnimations(system);
}

void SpPokeGimmick_StartChase(Field *field, u16 a1) {
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);
    SpPokeGimmickCurve *curve = work->data.curve;
    VecFx32 pos;

    curve->curve = GFL_G3DCurveCreateToLoadBuffer(Field_GetHeapID(field), ARCID_SP_POKE_CURVE, CURVE_ARC_FILE,
                                                  CURVE_TYPE, curve->buffer, sizeof(curve->buffer));
    curve->actor = FindFieldActor(Field_GetActorSystem(field), a1);
    if (curve->actor != NULL) {
        SetActorMovementFlag(curve->actor, ACTOR_MOVEMENT_FLAG_SP_POKE);
        CopyActorWPos(curve->actor, &curve->basePos);
        GFL_G3DCurveGetNowTranslation(curve->curve, &pos);
        VEC_Add(&pos, &curve->basePos, &pos);
        SetActorWPosValue(curve->actor, &pos);
        curve->state = CURVE_STATE_RUN;
        curve->phase = 0;
    }
}

BOOL SpPokeGimmick_IsChaseDone(Field *field) {
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);
    BOOL done = TRUE;

    if (work->data.curve->state != CURVE_STATE_IDLE) {
        done = FALSE;
    }
    return done;
}

void SpPokeGimmick_DropActor(Field *field) {
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);

    SpPokeGimmick_StartDrop(work->data.curve, field);
}

// Lowers the actor from where it is to the ground of grid square (5, 7), a square at a time
static void SpPokeGimmick_StartDrop(SpPokeGimmickCurve *curve, Field *field) {
    HeapID heapId = Field_GetHeapID(field);
    VecFx32 pos;
    VecFx32 ground;
    fx32 height;
    FieldTaskManager *taskManager;
    FieldTask *task;
    SpPokeDropTask *drop;

    CopyActorWPos(curve->actor, &pos);
    height = 0;
    ground.x = GRID_CENTER(5);
    ground.y = 0;
    ground.z = GRID_CENTER(7);
    GetHeightFromMap(curve->actor, &ground, &height);
    ground.y = height;
    taskManager = Field_GetTaskManager(field);
    task = FieldTask_Create(heapId, sizeof(SpPokeDropTask), SpPokeGimmick_DropTask);
    drop = FieldTask_GetData(task);
    drop->actor = curve->actor;
    drop->start = pos;
    drop->end = ground;
    FieldTaskManager_AddTask(taskManager, task, 0);
}

static BOOL SpPokeGimmick_DropTask(void *data) {
    SpPokeDropTask *drop = data;
    VecFx32 pos;

    CopyActorWPos(drop->actor, &pos);
    if (pos.y > drop->end.y) {
        pos.y -= FX32_ONE;
        SetActorWPosValue(drop->actor, &pos);
    } else {
        pos.y = drop->end.y;
        pos.x = drop->end.x;
        pos.z = drop->end.z;
        SetActorWPosAll(drop->actor, &pos, GetActorFaceDir(drop->actor));
        ClearActorMovementFlag(drop->actor, ACTOR_MOVEMENT_FLAG_SP_POKE);
        return TRUE;
    }
    return FALSE;
}

void SpPokeGimmickC_Init(Field *field) {
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);
    u32 heapId = (Field_GetHeapID(field) & (HEAPID_TAIL_BIT - 1)) | HEAPID_TAIL_BIT;
    SpPokeGimmickWork *work;
    int i;

    Field_AllocGimmickWorkBlock(field, WORK_ID, heapId, WORK_SIZE);
    work = Field_GetGimmickWorkBlock(field, WORK_ID);
    work->data.npc = GFL_HeapAllocate(heapId, sizeof(SpPokeGimmickNpc), TRUE, "sp_poke_gimmick.c", 1144);
    work->field = field;
    work->system = Field_GetExpObjSystem(field);
    FieldExpObj_AddScene(system, &sSceneC, 0);
    FieldExpObj_SetActorHidden(system, 0, 1, TRUE);
    FieldExpObj_SetActorHidden(system, 0, 0, TRUE);
    for (i = 0; i < 3; i++) {
        SpPokeGimmick_InitAnimation(system, 0, i);
        SpPokeGimmick_InitAnimation(system, 1, i);
    }
    for (i = 0; i < 3; i++) {
        SpPokeGimmick_InitAnimation(system, 2, i);
    }
}

void SpPokeGimmickC_End(Field *field) {
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);

    GFL_HeapFree(work->data.npc);
    Field_DeleteGimmickWorkBlock(field, WORK_ID);
    FieldExpObj_FreeScene(system, 0);
}

void SpPokeGimmickC_Update(Field *field) {
    FieldExpObj_StepAllAnimations(Field_GetExpObjSystem(field));
}

void SpPokeGimmickD_Init(Field *field) {
    SpPokeGimmickWork *work;

    Field_AllocGimmickWorkBlock(field, WORK_ID, Field_GetHeapID(field), WORK_SIZE);
    work = Field_GetGimmickWorkBlock(field, WORK_ID);
    work->field = field;
    work->system = Field_GetExpObjSystem(field);
    SpPokeGimmick_InitScene(work);
}

void SpPokeGimmickD_End(Field *field) {
    SpPokeGimmick_FreeScene(Field_GetGimmickWorkBlock(field, WORK_ID));
    Field_DeleteGimmickWorkBlock(field, WORK_ID);
}

void SpPokeGimmickD_Update(Field *field) {
    SpPokeGimmick_StepAnimations(Field_GetGimmickWorkBlock(field, WORK_ID));
}

GameEvent *SpPokeGimmick_CallFormEvent(GameSystem *gsys) {
    Field_GetGimmickWorkBlock(GSYS_GetField(gsys), WORK_ID);
    return SpPokeGimmick_CreateFormEvent(gsys);
}

void SpPokeGimmick_HideActor2(GameSystem *gsys) {
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), WORK_ID);
    int i;

    FieldExpObj_SetActorHidden(work->system, 0, 2, TRUE);
    for (i = 0; i < 2; i++) {
        SpPokeGimmick_PauseAnimation(work->system, 2, i);
    }
}

static void SpPokeGimmick_InitScene(SpPokeGimmickWork *work) {
    s32 anm;
    s32 anmCount;
    u16 actor;
    SRTMatrix *matrix;

    FieldExpObj_AddScene(work->system, &sSceneD, 0);
    for (actor = 0; actor < 6; actor++) {
        anmCount = sActorAnimCounts[actor];
        matrix = FieldExpObj_GetActorMatrixPtr(work->system, 0, actor);
        matrix->translation.x = sActorPositions[actor].x;
        matrix->translation.y = sActorPositions[actor].y;
        matrix->translation.z = sActorPositions[actor].z;
        func_ov036_021b8248(work->system, 0, actor, 1);
        FieldExpObj_SetActorHidden(work->system, 0, actor, TRUE);
        for (anm = 0; anm < anmCount; anm++) {
            FieldExpObjAnm *info = FieldExpObj_GetAnmInfo(work->system, 0, actor, anm);

            FieldExpObjAnm_SetLooped(info, FALSE);
            FieldExpObjAnm_SetPaused(info, TRUE);
            FieldExpObj_SetAnm(work->system, 0, actor, anm, FALSE);
        }
    }
}

static void SpPokeGimmick_FreeScene(SpPokeGimmickWork *work) {
    FieldExpObj_FreeScene(work->system, 0);
}

static void SpPokeGimmick_StepAnimations(SpPokeGimmickWork *work) {
    FieldExpObj_StepAllAnimations(work->system);
}

// Starts an animation at its first frame, played once
static void SpPokeGimmick_PlayAnimation(FieldExpObjSystem *system, u16 actor, u16 anm) {
    FieldExpObjAnm *info = FieldExpObj_GetAnmInfo(system, 0, actor, anm);

    FieldExpObjAnm_SetLooped(info, FALSE);
    FieldExpObjAnm_SetPaused(info, FALSE);
    FieldExpObj_SetAnm(system, 0, actor, anm, TRUE);
    FieldExpObj_SetAnmFrame(system, 0, actor, anm, 0);
}

static void SpPokeGimmick_PauseAnimation(FieldExpObjSystem *system, u16 actor, u16 anm) {
    FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(system, 0, actor, anm), TRUE);
    FieldExpObj_SetAnm(system, 0, actor, anm, FALSE);
}

// Shows an actor, stopping its animations but animations 0 and 1, which play
static void SpPokeGimmick_ShowAndPlay01(FieldExpObjSystem *system, u16 actor) {
    u32 i;

    FieldExpObj_SetActorHidden(system, 0, actor, FALSE);
    for (i = 0; i < 7; i++) {
        SpPokeGimmick_PauseAnimation(system, actor, i);
    }
    SpPokeGimmick_PlayAnimation(system, actor, 0);
    SpPokeGimmick_PlayAnimation(system, actor, 1);
}

static BOOL SpPokeGimmick_IsAnimation01Done(FieldExpObjSystem *system, u16 actor) {
    if (!FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(system, 0, actor, 0))) {
        return FALSE;
    }
    if (!FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(system, 0, actor, 1))) {
        return FALSE;
    }
    return TRUE;
}

// Stops all animations but 2 and 3, which play
static void SpPokeGimmick_PlayAnimation23(FieldExpObjSystem *system, u16 actor) {
    u32 i;

    for (i = 0; i < 7; i++) {
        SpPokeGimmick_PauseAnimation(system, actor, i);
    }
    SpPokeGimmick_PlayAnimation(system, actor, 2);
    SpPokeGimmick_PlayAnimation(system, actor, 3);
}

// Stops all animations but 4, 5 and 6, which play
static void SpPokeGimmick_PlayAnimation456(FieldExpObjSystem *system, u16 actor) {
    u32 i;

    for (i = 0; i < 7; i++) {
        SpPokeGimmick_PauseAnimation(system, actor, i);
    }
    SpPokeGimmick_PlayAnimation(system, actor, 4);
    SpPokeGimmick_PlayAnimation(system, actor, 5);
    SpPokeGimmick_PlayAnimation(system, actor, 6);
}

static BOOL SpPokeGimmick_IsAnimation456Done(FieldExpObjSystem *system, u16 actor) {
    if (!FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(system, 0, actor, 4))) {
        return FALSE;
    }
    if (!FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(system, 0, actor, 5))) {
        return FALSE;
    }
    if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(system, 0, actor, 6))) {
        return TRUE;
    }
    return FALSE;
}

static GameEvent *SpPokeGimmick_CreateFormEvent(GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, SpPokeGimmick_FormEvent, sizeof(SpPokeFormEventWork));
    SpPokeFormEventWork *work = GameEvent_GetData(event);

    work->system = Field_GetExpObjSystem(field);
    work->field = field;
    work->timer = 0;
    return event;
}

// The three swordsmen (actors 5, 3 and 4) appear one after the other around actor 2, then leave together
static GameEventReturnCode SpPokeGimmick_FormEvent(GameEvent *event, u32 *state, void *data) {
    SpPokeFormEventWork *work = data;
    int i;

    Field_GetGimmickWorkBlock(GSYS_GetField(GameEvent_GetGameSystem(event)), WORK_ID);
    switch (*state) {
    case 0:
        *state = 1;
        break;
    case 1:
        FieldExpObj_SetActorHidden(work->system, 0, 2, FALSE);
        for (i = 0; i < 2; i++) {
            SpPokeGimmick_PlayAnimation(work->system, 2, i);
        }
        GFL_SndSEPlay(SEQ_SE_SW_KERTIO_FORM_01);
        work->timer = 60;
        *state = 2;
        break;
    case 2:
        if (work->timer-- <= 0) {
            *state = 3;
        }
        break;
    case 3:
        SpPokeGimmick_ShowAndPlay01(work->system, 5);
        GFL_SndSEPlay(SEQ_SE_SW_KERTIO_FORM_02);
        *state = 4;
        break;
    case 4:
        if (SpPokeGimmick_IsAnimation01Done(work->system, 5)) {
            SpPokeGimmick_PlayAnimation23(work->system, 5);
            *state = 5;
        }
        break;
    case 5:
        SpPokeGimmick_ShowAndPlay01(work->system, 3);
        GFL_SndSEPlay(SEQ_SE_SW_KERTIO_FORM_02);
        *state = 6;
        break;
    case 6:
        if (SpPokeGimmick_IsAnimation01Done(work->system, 3)) {
            SpPokeGimmick_PlayAnimation23(work->system, 3);
            *state = 7;
        }
        break;
    case 7:
        SpPokeGimmick_ShowAndPlay01(work->system, 4);
        GFL_SndSEPlay(SEQ_SE_SW_KERTIO_FORM_02);
        *state = 8;
        break;
    case 8:
        if (SpPokeGimmick_IsAnimation01Done(work->system, 4)) {
            SpPokeGimmick_PlayAnimation23(work->system, 4);
            work->timer = 20;
            *state = 9;
        }
        break;
    case 9:
        if (work->timer-- <= 0) {
            SpPokeGimmick_PlayAnimation456(work->system, 5);
            SpPokeGimmick_PlayAnimation456(work->system, 3);
            SpPokeGimmick_PlayAnimation456(work->system, 4);
            GFL_SndSEPlay(SEQ_SE_SW_KERTIO_FORM_06);
            *state = 10;
        }
        break;
    case 10:
        if (SpPokeGimmick_IsAnimation456Done(work->system, 5)) {
            FieldExpObj_SetActorHidden(work->system, 0, 5, TRUE);
            FieldExpObj_SetActorHidden(work->system, 0, 3, TRUE);
            FieldExpObj_SetActorHidden(work->system, 0, 4, TRUE);
            *state = 11;
        }
        break;
    case 11:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
