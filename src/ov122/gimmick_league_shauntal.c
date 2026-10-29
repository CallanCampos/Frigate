#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_exp_obj.h"
#include "field/field_fog.h"
#include "field/field_map.h"
#include "field/gimmick_league_shauntal.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/wipe.h"

#define GIMMICK_WORK_ID 1
#define GIMMICK_STATE_ID 39

// The models of the scene. Model 0 is the room, 1 to 5 the five that play on a script's command, and 6 to 9 those of
// its last events
enum {
    MODEL_0,
    MODEL_1,
    MODEL_2,
    MODEL_3,
    MODEL_4,
    MODEL_5,
    MODEL_6,
    MODEL_7,
    MODEL_8,
    MODEL_9,
};

// What the save keeps of the room
typedef struct {
    // Model 8 has played
    BOOL model8Played;
    // The room has changed
    BOOL roomChanged;
} ShauntalRoomState;

// What a task that shows a model once it has played keeps, in the room's work
typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    // Which of models 1 to 5
    u32 index;
} ModelTaskWork;

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    ShauntalRoomState *state;
    ModelTaskWork tasks[5];
    // The fog is kept off until it is
    BOOL fogKeptOff;
} ShauntalRoomWork;

// The room change's event
typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    // Frames to wait
    s32 wait;
    FieldActor *player;
} RoomChangeEventWork;

static void GimmickShauntal_ShowSavedState(ShauntalRoomWork *work);
static void GimmickShauntal_Init(ShauntalRoomWork *work);
static void GimmickShauntal_Free(ShauntalRoomWork *work);
static void GimmickShauntal_StepAnimations(ShauntalRoomWork *work);
static void GimmickShauntal_InitModel(FieldExpObjSystem *expObj, u16 model, int animCount);
static void GimmickShauntal_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped);
static void GimmickShauntal_StopAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm);
static void GimmickShauntal_ShowModel0Idle(ShauntalRoomWork *work);
static void GimmickShauntal_PlayModel0Finale(ShauntalRoomWork *work);
static void GimmickShauntal_ShowModel0End(ShauntalRoomWork *work);
static void GimmickShauntal_PlayModel0Opening(ShauntalRoomWork *work);
static BOOL GimmickShauntal_IsModel0OpeningDone(ShauntalRoomWork *work);
static void GimmickShauntal_PlayModel1(ShauntalRoomWork *work);
static BOOL GimmickShauntal_IsModel1Done(ShauntalRoomWork *work);
static void GimmickShauntal_ShowModel1Idle(ShauntalRoomWork *work);
static void GimmickShauntal_PlayModelFrom2(ShauntalRoomWork *work, int index);
static BOOL GimmickShauntal_IsModelFrom2Done(ShauntalRoomWork *work, int index);
static void GimmickShauntal_ShowModelFrom2Idle(ShauntalRoomWork *work, int index);
static void GimmickShauntal_ShowModel6Idle(ShauntalRoomWork *work);
static void GimmickShauntal_PlayModel6Transition(ShauntalRoomWork *work);
static BOOL GimmickShauntal_IsModel6TransitionDone(ShauntalRoomWork *work);
static void GimmickShauntal_ShowModel6End(ShauntalRoomWork *work);
static void GimmickShauntal_PlayModel7(ShauntalRoomWork *work);
static void GimmickShauntal_ShowModel8Idle(ShauntalRoomWork *work);
static BOOL GimmickShauntal_IsModel8OpeningDone(ShauntalRoomWork *work);
static void GimmickShauntal_PlayModel8Finale(ShauntalRoomWork *work);
static void GimmickShauntal_PlayModel9(ShauntalRoomWork *work);
static BOOL GimmickShauntal_IsModel9Done(ShauntalRoomWork *work);
static void GimmickShauntal_ShowModel9End(ShauntalRoomWork *work);
static GameEvent *GimmickShauntal_CreateModel8Event(GameEvent *parent, GameSystem *gsys);
static GameEventReturnCode GimmickShauntal_Model8Event(GameEvent *event, u32 *state, void *data);
static void GimmickShauntal_Model1Task(TCB *tcb, void *data);
static void GimmickShauntal_ModelFrom2Task(TCB *tcb, void *data);
static GameEvent *GimmickShauntal_CreateRoomChangeEvent(GameSystem *gsys, u16 wait);
static GameEventReturnCode GimmickShauntal_RoomChangeEvent(GameEvent *event, u32 *state, void *data);
static GameEvent *GimmickShauntal_CreateModels6And9Event(GameEvent *parent, GameSystem *gsys);
static GameEventReturnCode GimmickShauntal_Models6And9Event(GameEvent *event, u32 *state, void *data);
static void GimmickShauntal_SetModel8Played(ShauntalRoomState *state, BOOL played);
static BOOL GimmickShauntal_GetModel8Played(ShauntalRoomState *state);
static void GimmickShauntal_SetRoomChanged(ShauntalRoomState *state, BOOL changed);
static BOOL GimmickShauntal_GetRoomChanged(ShauntalRoomState *state);

static const G3DSceneAnimationSetup sModel0Animations[] = {
    { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 }, { 5, 0 }, { 6, 0 }, { 7, 0 }, { 8, 0 },
};
static const G3DSceneAnimationSetup sModel1Animations[] = {
    { 10, 0 }, { 11, 0 }, { 12, 0 }, { 13, 0 }, { 14, 0 }, { 15, 0 }, { 16, 0 }, { 17, 0 }, { 18, 0 },
};
// Models 2 to 5 share their model and animations
static const G3DSceneAnimationSetup sModelFrom2Animations[] = {
    { 20, 0 }, { 21, 0 }, { 22, 0 }, { 23, 0 }, { 24, 0 }, { 25, 0 }, { 26, 0 }, { 27, 0 }, { 28, 0 },
};
static const G3DSceneAnimationSetup sModel6Animations[] = {
    { 30, 0 }, { 31, 0 }, { 32, 0 }, { 33, 0 }, { 34, 0 }, { 35, 0 },
};
static const G3DSceneAnimationSetup sModel7Animations[] = { { 37, 0 }, { 38, 0 } };
static const G3DSceneAnimationSetup sModel9Animations[] = { { 47, 0 }, { 48, 0 }, { 49, 0 }, { 50, 0 } };
static const G3DSceneAnimationSetup sModel8Animations[] = {
    { 40, 0 }, { 41, 0 }, { 42, 0 }, { 43, 0 }, { 44, 0 }, { 45, 0 },
};

static const G3DSceneResourceSetup sResources[51] = {
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 0, 0 },  { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 1, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 2, 0 },  { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 3, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 4, 0 },  { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 5, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 6, 0 },  { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 7, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 8, 0 },  { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 9, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 10, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 11, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 12, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 13, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 14, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 15, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 16, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 17, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 18, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 19, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 20, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 21, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 22, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 23, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 24, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 25, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 26, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 27, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 28, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 29, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 30, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 31, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 32, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 33, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 34, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 35, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 36, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 37, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 38, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 39, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 40, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 41, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 42, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 43, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 44, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 45, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 46, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 47, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 48, 0 }, { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 49, 0 },
    { ARCID_GIMMICK_LEAGUE_SHAUNTAL, 50, 0 },
};

static const G3DSceneActorSetup sActors[10] = {
    { 0, 0, 0, 0, sModel0Animations, NELEMS(sModel0Animations) },
    { 9, 0, 9, 0, sModel1Animations, NELEMS(sModel1Animations) },
    { 19, 0, 19, 0, sModelFrom2Animations, NELEMS(sModelFrom2Animations) },
    { 19, 0, 19, 0, sModelFrom2Animations, NELEMS(sModelFrom2Animations) },
    { 19, 0, 19, 0, sModelFrom2Animations, NELEMS(sModelFrom2Animations) },
    { 19, 0, 19, 0, sModelFrom2Animations, NELEMS(sModelFrom2Animations) },
    { 29, 0, 29, 0, sModel6Animations, NELEMS(sModel6Animations) },
    { 36, 0, 36, 0, sModel7Animations, NELEMS(sModel7Animations) },
    { 39, 0, 39, 0, sModel8Animations, NELEMS(sModel8Animations) },
    { 46, 0, 46, 0, sModel9Animations, NELEMS(sModel9Animations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void GimmickShauntal_Setup(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    ShauntalRoomWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(ShauntalRoomWork));

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    work->state = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_STATE_ID);
    GimmickShauntal_Init(work);
    GimmickShauntal_ShowSavedState(work);
}

void GimmickShauntal_End(Field *field) {
    ShauntalRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    GimmickShauntal_Free(work);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void GimmickShauntal_Update(Field *field) {
    ShauntalRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    if (work->fogKeptOff) {
        FieldFog *fog = Field_GetFog(work->field);

        if (!FieldFog_IsEnabled(fog)) {
            work->fogKeptOff = FALSE;
        }
        FieldFog_SetEnabled(fog, FALSE);
    }
    GimmickShauntal_StepAnimations(work);
}

GameEvent *GimmickShauntal_CallModel8Event(GameSystem *gsys) {
    ShauntalRoomWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    GameEvent *event = GimmickShauntal_CreateModel8Event(NULL, gsys);

    if (event != NULL) {
        GimmickShauntal_SetModel8Played(work->state, TRUE);
    }
    return event;
}

void GimmickShauntal_PlayModelFromIndex(GameSystem *gsys, u16 index) {
    Field *field = GSYS_GetField(gsys);
    ShauntalRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    TCBManager *tcbManager = Field_GetTCBMgr(field);
    ModelTaskWork *task = &work->tasks[index];

    task->expObj = Field_GetExpObjSystem(field);
    task->field = field;
    task->index = index;
    if (index == 0) {
        GimmickShauntal_PlayModel1(work);
        GimmickShauntal_PlayModel7(work);
        GFL_TCBMgrAddTask(tcbManager, GimmickShauntal_Model1Task, task, 0);
        GFL_SndSEPlay(SEQ_SE_SW_SHIKIMI_04);
    } else {
        GimmickShauntal_PlayModelFrom2(work, index - 1);
        GFL_TCBMgrAddTask(tcbManager, GimmickShauntal_ModelFrom2Task, task, 0);
        GFL_SndSEPlay(SEQ_SE_SW_SHIKIMI_02);
    }
}

GameEvent *GimmickShauntal_CallRoomChangeEvent(GameSystem *gsys, u16 wait) {
    ShauntalRoomWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    GameEvent *event = GimmickShauntal_CreateRoomChangeEvent(gsys, wait);

    if (event != NULL) {
        GimmickShauntal_SetRoomChanged(work->state, TRUE);
    }
    return event;
}

GameEvent *GimmickShauntal_CallModels6And9Event(GameSystem *gsys) {
    Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    return GimmickShauntal_CreateModels6And9Event(NULL, gsys);
}

// Shows the models as the saved state left them
static void GimmickShauntal_ShowSavedState(ShauntalRoomWork *work) {
    BOOL model8Played = GimmickShauntal_GetModel8Played(work->state);
    BOOL roomChanged = GimmickShauntal_GetRoomChanged(work->state);

    if (model8Played == FALSE && roomChanged == FALSE) {
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_7, FALSE);
        GimmickShauntal_ShowModel0Idle(work);
        GimmickShauntal_ShowModel6Idle(work);
        GimmickShauntal_ShowModel8Idle(work);
        work->fogKeptOff = TRUE;
    } else if (model8Played == TRUE && roomChanged == FALSE) {
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_7, FALSE);
        GimmickShauntal_ShowModel0Idle(work);
        GimmickShauntal_ShowModel6Idle(work);
        GimmickShauntal_ShowModel1Idle(work);
        GimmickShauntal_ShowModelFrom2Idle(work, 0);
        GimmickShauntal_ShowModelFrom2Idle(work, 1);
        GimmickShauntal_ShowModelFrom2Idle(work, 2);
        GimmickShauntal_ShowModelFrom2Idle(work, 3);
        work->fogKeptOff = TRUE;
    } else if (roomChanged == TRUE) {
        GimmickShauntal_ShowModel0End(work);
        GimmickShauntal_ShowModel6End(work);
        GimmickShauntal_ShowModel9End(work);
    }
}

static void GimmickShauntal_Init(ShauntalRoomWork *work) {
    int i;

    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    GimmickShauntal_InitModel(work->expObj, MODEL_0, NELEMS(sModel0Animations));
    GimmickShauntal_InitModel(work->expObj, MODEL_1, NELEMS(sModel1Animations));
    GimmickShauntal_InitModel(work->expObj, MODEL_2, NELEMS(sModelFrom2Animations));
    GimmickShauntal_InitModel(work->expObj, MODEL_3, NELEMS(sModelFrom2Animations));
    GimmickShauntal_InitModel(work->expObj, MODEL_4, NELEMS(sModelFrom2Animations));
    GimmickShauntal_InitModel(work->expObj, MODEL_5, NELEMS(sModelFrom2Animations));
    GimmickShauntal_InitModel(work->expObj, MODEL_6, NELEMS(sModel6Animations));
    GimmickShauntal_InitModel(work->expObj, MODEL_7, NELEMS(sModel7Animations));
    GimmickShauntal_InitModel(work->expObj, MODEL_8, NELEMS(sModel8Animations));
    GimmickShauntal_InitModel(work->expObj, MODEL_9, NELEMS(sModel9Animations));
    VecFx32 offsets[4] = {
        { 0, 0, FX32_CONST(-8) },
        { 0, FX32_CONST(40), FX32_CONST(-48) },
        { 0, FX32_CONST(80), FX32_CONST(-88) },
        { 0, FX32_CONST(120), FX32_CONST(-128) },
    };
    for (i = 0; i < 4; i++) {
        SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, MODEL_2 + i);

        matrix->translation.x = offsets[i].x + FX32_CONST(256);
        matrix->translation.y = offsets[i].y;
        matrix->translation.z = offsets[i].z + FX32_CONST(256);
    }
}

static void GimmickShauntal_Free(ShauntalRoomWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void GimmickShauntal_StepAnimations(ShauntalRoomWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Places the model, hidden, with its animations set and paused
static void GimmickShauntal_InitModel(FieldExpObjSystem *expObj, u16 model, int animCount) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    int i;

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
    matrix->translation.x = FX32_CONST(256);
    matrix->translation.y = 0;
    matrix->translation.z = FX32_CONST(256);
    FieldExpObj_SetActorCulling(expObj, 0, model, 1);
    FieldExpObj_SetActorHidden(expObj, 0, model, TRUE);
    for (i = 0; i < animCount; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, model, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(expObj, 0, model, i, FALSE);
    }
}

// Plays the animation from its first frame
static void GimmickShauntal_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, FALSE);
    FieldExpObjAnm_SetLooped(anmInfo, looped);
    FieldExpObj_SetAnm(expObj, 0, model, anm, TRUE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Stops the animation at its first frame
static void GimmickShauntal_StopAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, TRUE);
    FieldExpObj_SetAnm(expObj, 0, model, anm, FALSE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Shows model 0 with its first three animations looping and its others stopped
static void GimmickShauntal_ShowModel0Idle(ShauntalRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_0, 0, TRUE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_0, 1, TRUE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_0, 2, TRUE);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 3);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 4);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 5);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 6);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 7);
}

// Plays the last three animations of model 0 once and stops its others
static void GimmickShauntal_PlayModel0Finale(ShauntalRoomWork *work) {
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 0);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 1);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 2);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 3);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 4);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_0, 5, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_0, 6, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_0, 7, FALSE);
}

// Shows model 0 with its animation 5 at its last frame
static void GimmickShauntal_ShowModel0End(ShauntalRoomWork *work) {
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_0, 5);
    fx32 lastFrame;

    FieldExpObj_SetAnm(work->expObj, 0, MODEL_0, 5, TRUE);
    lastFrame = FieldExpObjAnm_GetLastFrame(anm);
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, FALSE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, MODEL_0, 5, lastFrame);
}

// Shows model 0 and plays its animations 3 and 4 once, with its others stopped
static void GimmickShauntal_PlayModel0Opening(ShauntalRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_0, 3, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_0, 4, FALSE);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 0);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 1);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 2);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 5);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 6);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_0, 7);
}

static BOOL GimmickShauntal_IsModel0OpeningDone(ShauntalRoomWork *work) {
    return FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_0, 3));
}

// Shows model 1 and plays its first five animations once, with its others stopped
static void GimmickShauntal_PlayModel1(ShauntalRoomWork *work) {
    int i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_1, FALSE);
    for (i = 0; i <= 4; i++) {
        GimmickShauntal_PlayAnimation(work->expObj, MODEL_1, i, FALSE);
    }
    for (i = 5; i <= 8; i++) {
        GimmickShauntal_StopAnimation(work->expObj, MODEL_1, i);
    }
}

static BOOL GimmickShauntal_IsModel1Done(ShauntalRoomWork *work) {
    int i;

    for (i = 0; i <= 4; i++) {
        if (!FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_1, i))) {
            return FALSE;
        }
    }
    return TRUE;
}

// Shows model 1 with its first five animations stopped and its others looping
static void GimmickShauntal_ShowModel1Idle(ShauntalRoomWork *work) {
    int i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_1, FALSE);
    for (i = 0; i <= 4; i++) {
        GimmickShauntal_StopAnimation(work->expObj, MODEL_1, i);
    }
    for (i = 5; i <= 8; i++) {
        GimmickShauntal_PlayAnimation(work->expObj, MODEL_1, i, TRUE);
    }
}

// The same for model index + 2
static void GimmickShauntal_PlayModelFrom2(ShauntalRoomWork *work, int index) {
    u16 model = index + MODEL_2;
    int i;

    FieldExpObj_SetActorHidden(work->expObj, 0, model, FALSE);
    for (i = 0; i <= 4; i++) {
        GimmickShauntal_PlayAnimation(work->expObj, model, i, FALSE);
    }
    for (i = 5; i <= 8; i++) {
        GimmickShauntal_StopAnimation(work->expObj, model, i);
    }
}

static BOOL GimmickShauntal_IsModelFrom2Done(ShauntalRoomWork *work, int index) {
    int i = 0;
    u16 model = index + MODEL_2;

    for (; i <= 4; i++) {
        if (!FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(work->expObj, 0, model, i))) {
            return FALSE;
        }
    }
    return TRUE;
}

static void GimmickShauntal_ShowModelFrom2Idle(ShauntalRoomWork *work, int index) {
    u16 model = index + MODEL_2;
    int i;

    FieldExpObj_SetActorHidden(work->expObj, 0, model, FALSE);
    for (i = 0; i <= 4; i++) {
        GimmickShauntal_StopAnimation(work->expObj, model, i);
    }
    for (i = 5; i <= 8; i++) {
        GimmickShauntal_PlayAnimation(work->expObj, model, i, TRUE);
    }
}

// Shows model 6 with its first two animations looping and its others stopped
static void GimmickShauntal_ShowModel6Idle(ShauntalRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_6, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_6, 0, TRUE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_6, 1, TRUE);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 2);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 3);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 4);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 5);
}

// Plays the animations 2 and 3 of model 6 once and stops its others
static void GimmickShauntal_PlayModel6Transition(ShauntalRoomWork *work) {
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_6, 2, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_6, 3, FALSE);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 0);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 1);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 4);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 5);
}

static BOOL GimmickShauntal_IsModel6TransitionDone(ShauntalRoomWork *work) {
    int i;

    for (i = 2; i <= 3; i++) {
        if (!FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_6, i))) {
            return FALSE;
        }
    }
    return TRUE;
}

// Shows model 6 with its animations 4 and 5 looping and its others stopped
static void GimmickShauntal_ShowModel6End(ShauntalRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_6, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_6, 4, TRUE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_6, 5, TRUE);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 0);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 1);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 2);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_6, 3);
}

// Shows model 7 and plays its animations once
static void GimmickShauntal_PlayModel7(ShauntalRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_7, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_7, 0, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_7, 1, FALSE);
}

// Shows model 8 with its first three animations looping and its others stopped
static void GimmickShauntal_ShowModel8Idle(ShauntalRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_8, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_8, 0, TRUE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_8, 1, TRUE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_8, 2, TRUE);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_8, 3);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_8, 4);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_8, 5);
}

static BOOL GimmickShauntal_IsModel8OpeningDone(ShauntalRoomWork *work) {
    return FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_8, 2));
}

// Plays the last three animations of model 8 once and stops its others
static void GimmickShauntal_PlayModel8Finale(ShauntalRoomWork *work) {
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_8, 3, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_8, 4, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_8, 5, FALSE);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_8, 0);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_8, 1);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_8, 2);
}

// Shows model 9 and plays its first two animations once, with its others stopped
static void GimmickShauntal_PlayModel9(ShauntalRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_9, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_9, 0, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_9, 1, FALSE);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_9, 2);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_9, 3);
}

static BOOL GimmickShauntal_IsModel9Done(ShauntalRoomWork *work) {
    int i;

    for (i = 0; i <= 1; i++) {
        if (!FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_9, i))) {
            return FALSE;
        }
    }
    return TRUE;
}

// Shows model 9 with its last two animations looping and its others stopped
static void GimmickShauntal_ShowModel9End(ShauntalRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_9, FALSE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_9, 2, TRUE);
    GimmickShauntal_PlayAnimation(work->expObj, MODEL_9, 3, TRUE);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_9, 0);
    GimmickShauntal_StopAnimation(work->expObj, MODEL_9, 1);
}

static GameEvent *GimmickShauntal_CreateModel8Event(GameEvent *parent, GameSystem *gsys) {
    return GameEvent_Create(gsys, parent, GimmickShauntal_Model8Event, 0);
}

// Waits for model 8's first animations, then plays its others
static GameEventReturnCode GimmickShauntal_Model8Event(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    ShauntalRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    switch (*state) {
    case 0:
        if (GimmickShauntal_IsModel8OpeningDone(work)) {
            (*state)++;
        }
        break;
    case 1:
        GimmickShauntal_PlayModel8Finale(work);
        GFL_SndSEPlay(SEQ_SE_SW_SHIKIMI_01);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Shows model 1 idle once it has played
static void GimmickShauntal_Model1Task(TCB *tcb, void *data) {
    ModelTaskWork *task = data;
    ShauntalRoomWork *work = Field_GetGimmickWorkBlock(task->field, GIMMICK_WORK_ID);

    if (GimmickShauntal_IsModel1Done(work)) {
        GimmickShauntal_ShowModel1Idle(work);
        GFL_TCBRemove(tcb);
    }
}

// Shows the model idle once it has played
static void GimmickShauntal_ModelFrom2Task(TCB *tcb, void *data) {
    ModelTaskWork *task = data;
    ShauntalRoomWork *work = Field_GetGimmickWorkBlock(task->field, GIMMICK_WORK_ID);

    if (GimmickShauntal_IsModelFrom2Done(work, task->index - 1)) {
        GimmickShauntal_ShowModelFrom2Idle(work, task->index - 1);
        GFL_TCBRemove(tcb);
    }
}

static GameEvent *GimmickShauntal_CreateRoomChangeEvent(GameSystem *gsys, u16 wait) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, GimmickShauntal_RoomChangeEvent, sizeof(RoomChangeEventWork));
    RoomChangeEventWork *work = GameEvent_GetData(event);

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    work->wait = wait;
    work->player = FindFieldActor(Field_GetActorSystem(field), 0);
    return event;
}

// Opens the wall, then fades out, shows the room as it is after the change, and fades back in
static GameEventReturnCode GimmickShauntal_RoomChangeEvent(GameEvent *event, u32 *state, void *data) {
    RoomChangeEventWork *eventWork = data;
    ShauntalRoomWork *work;
    FieldFog *fog;
    GameSystem *gsys = GameEvent_GetGameSystem(event);

    work = Field_GetGimmickWorkBlock(eventWork->field, GIMMICK_WORK_ID);
    switch (*state) {
    case 0:
        FldAct_SetAcmd(eventWork->player, 0x4b);
        *state = 1;
        break;
    case 1:
        if (FldAct_CheckAcmdEnd(eventWork->player)) {
            GimmickShauntal_PlayModel0Opening(work);
            GFL_SndSEPlay(SEQ_SE_SW_SHIKIMI_03);
            *state = 2;
        }
        break;
    case 2:
        if (GimmickShauntal_IsModel0OpeningDone(work)) {
            FldAct_SetAcmd(eventWork->player, 0x21);
            GimmickShauntal_PlayModel0Finale(work);
            GFL_SndSEPlay(SEQ_SE_SW_SHIKIMI_06);
            *state = 4;
        }
        break;
    case 4:
        eventWork->wait--;
        if (eventWork->wait <= 0) {
            *state = 5;
        }
        break;
    case 5:
        GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1,
                    Field_GetHeapID(eventWork->field));
        *state = 6;
        eventWork->wait = 15;
        break;
    case 6:
        if (GFL_WipeIsFinished()) {
            eventWork->wait--;
            if (eventWork->wait <= 0) {
                fog = Field_GetFog(eventWork->field);
                FieldFog_SetEnabled(fog, TRUE);
                FieldFog_Flush(fog);
                FieldExpObj_SetActorHidden(eventWork->expObj, 0, MODEL_9, FALSE);
                *state = 7;
            }
        }
        break;
    case 7:
        GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1,
                    Field_GetHeapID(eventWork->field));
        *state = 8;
        break;
    case 8:
        if (GFL_WipeIsFinished() == TRUE) {
            *state = 9;
        }
        break;
    case 9:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *GimmickShauntal_CreateModels6And9Event(GameEvent *parent, GameSystem *gsys) {
    return GameEvent_Create(gsys, parent, GimmickShauntal_Models6And9Event, 0);
}

// Plays the animations of models 6 and 9, and when they are done shows their last
static GameEventReturnCode GimmickShauntal_Models6And9Event(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    ShauntalRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    switch (*state) {
    case 0:
        GimmickShauntal_PlayModel6Transition(work);
        GimmickShauntal_PlayModel9(work);
        GFL_SndSEPlay(SEQ_SE_SW_SHIKIMI_05);
        (*state)++;
        break;
    case 1:
        if (GimmickShauntal_IsModel6TransitionDone(work) && GimmickShauntal_IsModel9Done(work)) {
            (*state)++;
        }
        break;
    case 2:
        GimmickShauntal_ShowModel6End(work);
        GimmickShauntal_ShowModel9End(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void GimmickShauntal_SetModel8Played(ShauntalRoomState *state, BOOL played) {
    state->model8Played = played;
}

static BOOL GimmickShauntal_GetModel8Played(ShauntalRoomState *state) {
    return state->model8Played;
}

static void GimmickShauntal_SetRoomChanged(ShauntalRoomState *state, BOOL changed) {
    state->roomChanged = changed;
}

static BOOL GimmickShauntal_GetRoomChanged(ShauntalRoomState *state) {
    return state->roomChanged;
}
