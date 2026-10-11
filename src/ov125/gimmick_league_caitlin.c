#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_exp_obj.h"
#include "field/field_map.h"
#include "field/field_sound.h"
#include "field/field_task.h"
#include "field/gimmick_league_caitlin.h"
#include "gfl/g3d.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

#define GIMMICK_WORK_ID 1
#define GIMMICK_STATE_ID 42
// The archive of the room's models, a/2/8/5
#define GIMMICK_ARCID 0x117
// The room's ambient sound
#define AMBIENCE_SE 0x88f

// The models of the scene
enum {
    MODEL_0,
    MODEL_1,
    MODEL_2,
    MODEL_3,
    MODEL_4,
    MODEL_5,
};

// What the save keeps of the room
typedef struct {
    // Models 3 and 4 have played
    BOOL model34Played;
    // Model 5 has played
    BOOL model5Played;
} CaitlinRoomState;

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    CaitlinRoomState *state;
} CaitlinRoomWork;

// The drop of an actor
typedef struct {
    // The height where it lands
    fx32 targetY;
    fx32 speed;
    FieldActor *actor;
    CaitlinRoomWork *room;
} ActorDropTaskWork;

static void GimmickCaitlin_ShowSavedState(CaitlinRoomWork *work);
static void GimmickCaitlin_Init(CaitlinRoomWork *work);
static void GimmickCaitlin_Free(CaitlinRoomWork *work);
static void GimmickCaitlin_StepAnimations(CaitlinRoomWork *work);
static void GimmickCaitlin_InitModel(FieldExpObjSystem *expObj, u16 model, int animCount);
static void GimmickCaitlin_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped);
static void GimmickCaitlin_StopAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm);
static void GimmickCaitlin_PlayModel0(CaitlinRoomWork *work);
static void GimmickCaitlin_ShowModel0(CaitlinRoomWork *work);
static void GimmickCaitlin_PlayModel1(CaitlinRoomWork *work);
static void GimmickCaitlin_ShowModel1(CaitlinRoomWork *work);
static BOOL GimmickCaitlin_IsModel1Done(CaitlinRoomWork *work);
static void GimmickCaitlin_PlayModel2(CaitlinRoomWork *work);
static void GimmickCaitlin_ShowModel3(CaitlinRoomWork *work);
static void GimmickCaitlin_PlayModel3Rest(CaitlinRoomWork *work);
static void GimmickCaitlin_PlayModel4(CaitlinRoomWork *work);
static void GimmickCaitlin_ShowModel5(CaitlinRoomWork *work);
static void GimmickCaitlin_PlayModel5(CaitlinRoomWork *work);
static GameEventReturnCode GimmickCaitlin_Model1Event(GameEvent *event, u32 *state, void *data);
static void GimmickCaitlin_StartActorDrop(CaitlinRoomWork *work, FieldActor *actor, fx32 speed);
static BOOL GimmickCaitlin_ActorDropTask(void *data);
static void GimmickCaitlin_SetModel34Played(CaitlinRoomState *state, BOOL played);
static BOOL GimmickCaitlin_GetModel34Played(CaitlinRoomState *state);
static void GimmickCaitlin_SetModel5Played(CaitlinRoomState *state, BOOL played);
static BOOL GimmickCaitlin_GetModel5Played(CaitlinRoomState *state);

static const G3DSceneAnimationSetup sModel2Animations[] = { { 11, 0 } };
static const G3DSceneAnimationSetup sModel5Animations[] = { { 20, 0 }, { 21, 0 } };
static const G3DSceneAnimationSetup sModel4Animations[] = { { 17, 0 }, { 18, 0 } };
static const G3DSceneAnimationSetup sModel3Animations[] = { { 13, 0 }, { 14, 0 }, { 15, 0 } };
static const G3DSceneAnimationSetup sModel0Animations[] = { { 1, 0 }, { 2, 0 }, { 3, 0 } };
static const G3DSceneAnimationSetup sModel1Animations[] = { { 5, 0 }, { 6, 0 }, { 7, 0 }, { 8, 0 }, { 9, 0 } };

static const G3DSceneResourceSetup sResources[22] = {
    { GIMMICK_ARCID, 0, 0 },  { GIMMICK_ARCID, 1, 0 },  { GIMMICK_ARCID, 2, 0 },  { GIMMICK_ARCID, 3, 0 },
    { GIMMICK_ARCID, 4, 0 },  { GIMMICK_ARCID, 5, 0 },  { GIMMICK_ARCID, 6, 0 },  { GIMMICK_ARCID, 7, 0 },
    { GIMMICK_ARCID, 8, 0 },  { GIMMICK_ARCID, 9, 0 },  { GIMMICK_ARCID, 10, 0 }, { GIMMICK_ARCID, 11, 0 },
    { GIMMICK_ARCID, 12, 0 }, { GIMMICK_ARCID, 13, 0 }, { GIMMICK_ARCID, 14, 0 }, { GIMMICK_ARCID, 15, 0 },
    { GIMMICK_ARCID, 16, 0 }, { GIMMICK_ARCID, 17, 0 }, { GIMMICK_ARCID, 18, 0 }, { GIMMICK_ARCID, 19, 0 },
    { GIMMICK_ARCID, 20, 0 }, { GIMMICK_ARCID, 21, 0 },
};

static const G3DSceneActorSetup sActors[6] = {
    { 0, 0, 0, 0, sModel0Animations, NELEMS(sModel0Animations) },
    { 4, 0, 4, 0, sModel1Animations, NELEMS(sModel1Animations) },
    { 10, 0, 10, 0, sModel2Animations, NELEMS(sModel2Animations) },
    { 12, 0, 12, 0, sModel3Animations, NELEMS(sModel3Animations) },
    { 16, 0, 16, 0, sModel4Animations, NELEMS(sModel4Animations) },
    { 19, 0, 19, 0, sModel5Animations, NELEMS(sModel5Animations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void GimmickCaitlin_Setup(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    CaitlinRoomWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(CaitlinRoomWork));

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    work->state = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_STATE_ID);
    GimmickCaitlin_Init(work);
    GimmickCaitlin_ShowSavedState(work);
}

void GimmickCaitlin_End(Field *field) {
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(GSYS_GetGameData(Field_GetGameSystem(field)));
    CaitlinRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    FieldSnd_StopAmbience(fieldSound, AMBIENCE_SE);
    GimmickCaitlin_Free(work);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void GimmickCaitlin_Update(Field *field) {
    GimmickCaitlin_StepAnimations(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
}

void GimmickCaitlin_PlayModels34Or5(GameSystem *gsys, u16 which) {
    CaitlinRoomWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    if (which == 0) {
        GimmickCaitlin_PlayModel3Rest(work);
        GimmickCaitlin_PlayModel4(work);
    } else {
        GimmickCaitlin_PlayModel5(work);
    }
    if (which == 0) {
        GimmickCaitlin_SetModel34Played(work->state, TRUE);
    } else {
        GimmickCaitlin_SetModel5Played(work->state, TRUE);
    }
}

GameEvent *GimmickCaitlin_CreateModel1Event(GameSystem *gsys) {
    CaitlinRoomWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    return GameEvent_Create(gsys, NULL, GimmickCaitlin_Model1Event, 0);
}

void GimmickCaitlin_CallPlayModel0(GameSystem *gsys) {
    GimmickCaitlin_PlayModel0(Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID));
}

void GimmickCaitlin_DropActor(GameSystem *gsys, u32 actorId, u16 speed) {
    Field *field = GSYS_GetField(gsys);
    CaitlinRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    FieldActor *actor = FindFieldActor(Field_GetActorSystem(field), actorId);

    SetActorHidden(actor, FALSE);
    GimmickCaitlin_StartActorDrop(work, actor, speed << 8);
}

// Shows the models as the saved state left them
static void GimmickCaitlin_ShowSavedState(CaitlinRoomWork *work) {
    BOOL model34Played = GimmickCaitlin_GetModel34Played(work->state);
    BOOL model5Played = GimmickCaitlin_GetModel5Played(work->state);

    if (model5Played == FALSE) {
        SetActorHidden(FindFieldActor(Field_GetActorSystem(work->field), 0), TRUE);
        GimmickCaitlin_ShowModel5(work);
        GimmickCaitlin_ShowModel1(work);
        if (model34Played == FALSE) {
            GimmickCaitlin_ShowModel3(work);
            FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_4, FALSE);
        }
    } else {
        GimmickCaitlin_ShowModel0(work);
    }
}

static void GimmickCaitlin_Init(CaitlinRoomWork *work) {
    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    GimmickCaitlin_InitModel(work->expObj, MODEL_0, NELEMS(sModel0Animations));
    GimmickCaitlin_InitModel(work->expObj, MODEL_1, NELEMS(sModel1Animations));
    GimmickCaitlin_InitModel(work->expObj, MODEL_2, NELEMS(sModel2Animations));
    GimmickCaitlin_InitModel(work->expObj, MODEL_3, NELEMS(sModel3Animations));
    GimmickCaitlin_InitModel(work->expObj, MODEL_4, NELEMS(sModel4Animations));
    GimmickCaitlin_InitModel(work->expObj, MODEL_5, NELEMS(sModel5Animations));
}

static void GimmickCaitlin_Free(CaitlinRoomWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void GimmickCaitlin_StepAnimations(CaitlinRoomWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Places the model, hidden, with its animations set and paused
static void GimmickCaitlin_InitModel(FieldExpObjSystem *expObj, u16 model, int animCount) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    int i;

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
    matrix->translation.x = FX32_CONST(256);
    matrix->translation.y = 0;
    matrix->translation.z = FX32_CONST(256);
    func_ov036_021b8248(expObj, 0, model, 1);
    FieldExpObj_SetActorHidden(expObj, 0, model, TRUE);
    for (i = 0; i < animCount; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, model, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(expObj, 0, model, i, FALSE);
    }
}

// Plays the animation from its first frame
static void GimmickCaitlin_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, FALSE);
    FieldExpObjAnm_SetLooped(anmInfo, looped);
    FieldExpObj_SetAnm(expObj, 0, model, anm, TRUE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Stops the animation at its first frame
static void GimmickCaitlin_StopAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, TRUE);
    FieldExpObj_SetAnm(expObj, 0, model, anm, FALSE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Shows model 0 and plays its animations, the last once
static void GimmickCaitlin_PlayModel0(CaitlinRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, FALSE);
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_0, 0, TRUE);
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_0, 1, TRUE);
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_0, 2, FALSE);
}

// Shows model 0 with its last animation stopped
static void GimmickCaitlin_ShowModel0(CaitlinRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, FALSE);
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_0, 0, TRUE);
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_0, 1, TRUE);
    GimmickCaitlin_StopAnimation(work->expObj, MODEL_0, 2);
}

// Shows model 1 and plays its first four animations once, the last stopped
static void GimmickCaitlin_PlayModel1(CaitlinRoomWork *work) {
    u32 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_1, FALSE);
    for (i = 0; i < 4; i++) {
        GimmickCaitlin_PlayAnimation(work->expObj, MODEL_1, i, FALSE);
    }
    GimmickCaitlin_StopAnimation(work->expObj, MODEL_1, 4);
}

// Shows model 1 with its first four animations stopped, and the last playing
static void GimmickCaitlin_ShowModel1(CaitlinRoomWork *work) {
    u32 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_1, FALSE);
    for (i = 0; i < 4; i++) {
        GimmickCaitlin_StopAnimation(work->expObj, MODEL_1, i);
    }
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_1, 4, TRUE);
}

static BOOL GimmickCaitlin_IsModel1Done(CaitlinRoomWork *work) {
    return FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_1, 4));
}

// Shows model 2 and plays its animation
static void GimmickCaitlin_PlayModel2(CaitlinRoomWork *work) {
    int i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_2, FALSE);
    for (i = 0; i < NELEMS(sModel2Animations); i++) {
        GimmickCaitlin_PlayAnimation(work->expObj, MODEL_2, i, FALSE);
    }
}

// Shows model 3 with its first animation looping and its others stopped
static void GimmickCaitlin_ShowModel3(CaitlinRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_3, FALSE);
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_3, 0, TRUE);
    GimmickCaitlin_StopAnimation(work->expObj, MODEL_3, 1);
    GimmickCaitlin_StopAnimation(work->expObj, MODEL_3, 2);
}

// Plays the second and third animations of model 3
static void GimmickCaitlin_PlayModel3Rest(CaitlinRoomWork *work) {
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_3, 1, FALSE);
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_3, 2, FALSE);
}

// Shows model 4 and plays its animations
static void GimmickCaitlin_PlayModel4(CaitlinRoomWork *work) {
    u32 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_4, FALSE);
    for (i = 0; i < 2; i++) {
        GimmickCaitlin_PlayAnimation(work->expObj, MODEL_4, i, FALSE);
    }
}

// Shows model 5 with its first animation looping and its second stopped
static void GimmickCaitlin_ShowModel5(CaitlinRoomWork *work) {
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_5, FALSE);
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_5, 0, TRUE);
    GimmickCaitlin_StopAnimation(work->expObj, MODEL_5, 1);
}

// Plays the first animation of model 5 looping, and its second once
static void GimmickCaitlin_PlayModel5(CaitlinRoomWork *work) {
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_5, 0, TRUE);
    GimmickCaitlin_PlayAnimation(work->expObj, MODEL_5, 1, FALSE);
}

// Waits for model 1's last animation, then plays its others
static GameEventReturnCode GimmickCaitlin_Model1Event(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    CaitlinRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    switch (*state) {
    case 0:
        if (GimmickCaitlin_IsModel1Done(work)) {
            (*state)++;
        }
        break;
    case 1:
        GimmickCaitlin_PlayModel1(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Drops the actor, speed a frame, from where it is to the player's height
static void GimmickCaitlin_StartActorDrop(CaitlinRoomWork *work, FieldActor *actor, fx32 speed) {
    HeapID heapId = Field_GetHeapID(work->field);
    VecFx32 playerPos;
    FieldTaskManager *manager;
    FieldTask *task;
    ActorDropTaskWork *drop;

    FieldPlayer_GetWPos(Field_GetPlayer(work->field), &playerPos);
    manager = Field_GetTaskManager(work->field);
    task = FieldTask_Create(heapId, sizeof(ActorDropTaskWork), GimmickCaitlin_ActorDropTask);
    drop = FieldTask_GetData(task);
    drop->actor = actor;
    drop->targetY = playerPos.y;
    drop->speed = speed;
    drop->room = work;
    FieldTaskManager_AddTask(manager, task, 0);
}

static BOOL GimmickCaitlin_ActorDropTask(void *data) {
    ActorDropTaskWork *drop = data;
    VecFx32 pos;

    CopyActorWPos(drop->actor, &pos);
    pos.y -= drop->speed;
    SetActorWPosValue(drop->actor, &pos);
    if (pos.y <= drop->targetY) {
        CheckRecalcActorY(drop->actor);
        GimmickCaitlin_PlayModel2(drop->room);
        return TRUE;
    }
    return FALSE;
}

static void GimmickCaitlin_SetModel34Played(CaitlinRoomState *state, BOOL played) {
    state->model34Played = played;
}

static BOOL GimmickCaitlin_GetModel34Played(CaitlinRoomState *state) {
    return state->model34Played;
}

static void GimmickCaitlin_SetModel5Played(CaitlinRoomState *state, BOOL played) {
    state->model5Played = played;
}

static BOOL GimmickCaitlin_GetModel5Played(CaitlinRoomState *state) {
    return state->model5Played;
}
