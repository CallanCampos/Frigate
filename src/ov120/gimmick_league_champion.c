#include "types.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/field_map.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_task.h"
#include "field/gimmick_league_champion.h"
#include "gfl/g3d.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

#define GIMMICK_WORK_ID 1
#define GIMMICK_STATE_ID 46
// The archive of the room's models, a/2/8/0
#define GIMMICK_ARCID 0x122
// The camera's defaults once the room has changed
#define CHANGED_CAMERA_INDEX 43

// The models of the scene. Models 0 and 2 show before the room changes, and models 1 and 3 after
enum {
    MODEL_0,
    MODEL_1,
    MODEL_2,
    MODEL_3,
    MODEL_4,
};

// What the save keeps of the room
typedef struct {
    // The room has changed
    BOOL changed;
    // The room is open
    BOOL open;
} ChampionRoomState;

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    ChampionRoomState *state;
} ChampionRoomWork;

// A shake of the camera's target up and down
typedef struct {
    s16 amplitude;
    u8 count;
    u32 period;
    u32 frame;
    // Frames to wait before it starts
    s16 delay;
} CameraShake;

typedef struct {
    Field *field;
    FieldCamera *camera;
    u32 state;
    CameraShake shake;
    VecFx32 *bind;
    void *refBind;
} CameraShakeTaskWork;

static void GimmickChampion_ShowSavedState(ChampionRoomWork *work);
static void GimmickChampion_Init(ChampionRoomWork *work);
static void GimmickChampion_Free(ChampionRoomWork *work);
static void GimmickChampion_StepAnimations(ChampionRoomWork *work);
static void GimmickChampion_InitModel(FieldExpObjSystem *expObj, u16 model, int animCount, BOOL hidden);
static void GimmickChampion_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, fx32 frame, BOOL looped);
static void GimmickChampion_StopAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm);
static void GimmickChampion_PlayModel4(ChampionRoomWork *work);
static BOOL GimmickChampion_IsModel4Done(ChampionRoomWork *work);
static void GimmickChampion_ShowModel4End(ChampionRoomWork *work);
static GameEvent *GimmickChampion_CreateChangeEvent(GameSystem *gsys);
static GameEventReturnCode GimmickChampion_ChangeEvent(GameEvent *event, u32 *state, void *data);
static GameEvent *GimmickChampion_CreateOpenEvent(GameSystem *gsys);
static GameEventReturnCode GimmickChampion_OpenEvent(GameEvent *event, u32 *state, void *data);
static void GimmickChampion_SetChanged(ChampionRoomState *state, BOOL changed);
static BOOL GimmickChampion_GetChanged(ChampionRoomState *state);
static void GimmickChampion_SetOpen(ChampionRoomState *state, BOOL open);
static BOOL GimmickChampion_GetOpen(ChampionRoomState *state);
static FieldTask *GimmickChampion_CreateShakeTask(Field *field, const CameraShake *shake);
static BOOL GimmickChampion_ShakeTask(void *data);

static const G3DSceneAnimationSetup sModel4Animations[] = { { 10, 0 }, { 11, 0 } };
static const G3DSceneAnimationSetup sModel1Animations[] = { { 2, 0 }, { 3, 0 } };
static const G3DSceneAnimationSetup sModel2And3Animations[] = { { 6, 0 }, { 7, 0 }, { 8, 0 } };

static const G3DSceneResourceSetup sResources[12] = {
    { GIMMICK_ARCID, 0, 0 }, { GIMMICK_ARCID, 1, 0 }, { GIMMICK_ARCID, 2, 0 },  { GIMMICK_ARCID, 3, 0 },
    { GIMMICK_ARCID, 4, 0 }, { GIMMICK_ARCID, 5, 0 }, { GIMMICK_ARCID, 6, 0 },  { GIMMICK_ARCID, 7, 0 },
    { GIMMICK_ARCID, 8, 0 }, { GIMMICK_ARCID, 9, 0 }, { GIMMICK_ARCID, 10, 0 }, { GIMMICK_ARCID, 11, 0 },
};

static const G3DSceneActorSetup sActors[5] = {
    { 0, 0, 0, 0, NULL, 0 },
    { 1, 0, 1, 0, sModel1Animations, NELEMS(sModel1Animations) },
    { 4, 0, 4, 0, sModel2And3Animations, NELEMS(sModel2And3Animations) },
    { 5, 0, 5, 0, sModel2And3Animations, NELEMS(sModel2And3Animations) },
    { 9, 0, 9, 0, sModel4Animations, NELEMS(sModel4Animations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void GimmickChampion_Setup(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    ChampionRoomWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(ChampionRoomWork));

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    work->state = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_STATE_ID);
    GimmickChampion_Init(work);
    GimmickChampion_ShowSavedState(work);
}

void GimmickChampion_End(Field *field) {
    GimmickChampion_Free(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void GimmickChampion_Update(Field *field) {
    GimmickChampion_StepAnimations(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
}

GameEvent *GimmickChampion_CallChangeEvent(GameSystem *gsys) {
    return GimmickChampion_CreateChangeEvent(gsys);
}

GameEvent *GimmickChampion_CallOpenEvent(GameSystem *gsys) {
    return GimmickChampion_CreateOpenEvent(gsys);
}

void GimmickChampion_HideActorShadow(GameSystem *gsys, u32 actorId) {
    FldAct_SetForceOffShadow(FindFieldActor(Field_GetActorSystem(GSYS_GetField(gsys)), actorId), TRUE);
}

void GimmickChampion_ShowActorShadow(GameSystem *gsys, u32 actorId) {
    FldAct_SetForceOffShadow(FindFieldActor(Field_GetActorSystem(GSYS_GetField(gsys)), actorId), FALSE);
}

void GimmickChampion_CallSetOpen(GameSystem *gsys) {
    ChampionRoomWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    GimmickChampion_SetOpen(work->state, TRUE);
}

// Shows the models as the saved state left them
static void GimmickChampion_ShowSavedState(ChampionRoomWork *work) {
    BOOL changed = GimmickChampion_GetChanged(work->state);
    BOOL open = GimmickChampion_GetOpen(work->state);

    if (changed == TRUE) {
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, TRUE);
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_1, FALSE);
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_2, TRUE);
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_3, FALSE);
        GimmickChampion_ShowModel4End(work);
        FieldCamera_SetDefaultsIndex(Field_GetCameraSystem(work->field), CHANGED_CAMERA_INDEX);
    }
    if (open == TRUE) {
        FieldExpObj_SetAnm(work->expObj, 0, MODEL_3, 1, TRUE);
        FieldExpObj_SetAnm(work->expObj, 0, MODEL_3, 2, TRUE);
        FieldExpObj_SetAnmFrame(work->expObj, 0, MODEL_3, 1, 0);
        FieldExpObj_SetAnmFrame(work->expObj, 0, MODEL_3, 2, 0);
    } else {
        GimmickChampion_PlayAnimation(work->expObj, MODEL_3, 0, 0, TRUE);
    }
}

static void GimmickChampion_Init(ChampionRoomWork *work) {
    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    GimmickChampion_InitModel(work->expObj, MODEL_0, 0, FALSE);
    GimmickChampion_InitModel(work->expObj, MODEL_1, NELEMS(sModel1Animations), TRUE);
    GimmickChampion_InitModel(work->expObj, MODEL_2, NELEMS(sModel2And3Animations), FALSE);
    GimmickChampion_PlayAnimation(work->expObj, MODEL_2, 0, 0, TRUE);
    GimmickChampion_InitModel(work->expObj, MODEL_3, NELEMS(sModel2And3Animations), TRUE);
    GimmickChampion_InitModel(work->expObj, MODEL_4, NELEMS(sModel4Animations), TRUE);
    func_ov036_021b8248(work->expObj, 0, MODEL_4, 0);
}

static void GimmickChampion_Free(ChampionRoomWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void GimmickChampion_StepAnimations(ChampionRoomWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Places the model, with its animations set and paused
static void GimmickChampion_InitModel(FieldExpObjSystem *expObj, u16 model, int animCount, BOOL hidden) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    int i;

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
    matrix->translation.x = FX32_CONST(256);
    matrix->translation.y = 0;
    matrix->translation.z = FX32_CONST(768);
    func_ov036_021b8248(expObj, 0, model, 1);
    FieldExpObj_SetActorHidden(expObj, 0, model, hidden);
    if (animCount != 0) {
        for (i = 0; i < animCount; i++) {
            anm = FieldExpObj_GetAnmInfo(expObj, 0, model, i);
            FieldExpObjAnm_SetLooped(anm, FALSE);
            FieldExpObjAnm_SetPaused(anm, TRUE);
            FieldExpObj_SetAnm(expObj, 0, model, i, FALSE);
        }
    }
}

// Plays the animation from the frame
static void GimmickChampion_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, fx32 frame, BOOL looped) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, FALSE);
    FieldExpObjAnm_SetLooped(anmInfo, looped);
    FieldExpObj_SetAnm(expObj, 0, model, anm, TRUE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, frame);
}

// Stops the animation at its first frame
static void GimmickChampion_StopAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, TRUE);
    FieldExpObj_SetAnm(expObj, 0, model, anm, FALSE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Shows model 4 and plays its two animations
static void GimmickChampion_PlayModel4(ChampionRoomWork *work) {
    u32 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_4, FALSE);
    for (i = 0; i < 2; i++) {
        GimmickChampion_PlayAnimation(work->expObj, MODEL_4, i, 0, FALSE);
    }
}

static BOOL GimmickChampion_IsModel4Done(ChampionRoomWork *work) {
    BOOL done = FALSE;
    fx32 frame = func_ov036_021b8520(work->expObj, 0, MODEL_4, 0);

    if (frame >= FX32_CONST(8)) {
        done = TRUE;
    }
    return done;
}

// Shows model 4 at the end of its animations
static void GimmickChampion_ShowModel4End(ChampionRoomWork *work) {
    FieldExpObjAnm *anm;
    u32 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_4, FALSE);
    for (i = 0; i < 2; i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_4, i);
        FieldExpObj_SetAnm(work->expObj, 0, MODEL_4, i, TRUE);
        FieldExpObj_SetAnmFrame(work->expObj, 0, MODEL_4, i, func_ov036_021b8580(anm));
    }
}

static GameEvent *GimmickChampion_CreateChangeEvent(GameSystem *gsys) {
    return GameEvent_Create(gsys, NULL, GimmickChampion_ChangeEvent, 0);
}

// Plays model 4, then swaps the models of the room for the changed ones
static GameEventReturnCode GimmickChampion_ChangeEvent(GameEvent *event, u32 *state, void *data) {
    GameSystem *gsys = GameEvent_GetGameSystem(event);
    Field *field = GSYS_GetField(gsys);
    ChampionRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    switch (*state) {
    case 0:
        *state = 1;
        break;
    case 1:
        GimmickChampion_PlayModel4(work);
        *state = 2;
        break;
    case 2:
        if (GimmickChampion_IsModel4Done(work) == TRUE) {
            *state = 3;
        }
        break;
    case 3:
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, TRUE);
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_1, FALSE);
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_2, TRUE);
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_3, FALSE);
        FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_2, 0);
        GimmickChampion_PlayAnimation(work->expObj, MODEL_3, 0, func_ov036_021b8520(work->expObj, 0, MODEL_2, 0), TRUE);
        GimmickChampion_StopAnimation(work->expObj, MODEL_2, 0);
        GimmickChampion_SetChanged(work->state, TRUE);
        FieldCamera_SetDefaultsIndex(Field_GetCameraSystem(field), CHANGED_CAMERA_INDEX);
        *state = 4;
        break;
    case 4:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *GimmickChampion_CreateOpenEvent(GameSystem *gsys) {
    return GameEvent_Create(gsys, NULL, GimmickChampion_OpenEvent, 0);
}

// Plays the animations that open the room, with a shake of the camera
static GameEventReturnCode GimmickChampion_OpenEvent(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    ChampionRoomWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);
    CameraShake shake;
    FieldTask *task;

    switch (*state) {
    case 0:
        GimmickChampion_StopAnimation(work->expObj, MODEL_3, 0);
        GimmickChampion_PlayAnimation(work->expObj, MODEL_3, 1, 0, FALSE);
        GimmickChampion_PlayAnimation(work->expObj, MODEL_3, 2, 0, FALSE);
        GimmickChampion_PlayAnimation(work->expObj, MODEL_1, 0, 0, FALSE);
        GimmickChampion_PlayAnimation(work->expObj, MODEL_1, 1, 0, FALSE);
        MI_CpuFill8(&shake, 0, sizeof(shake));
        shake.amplitude = 1;
        shake.period = 3;
        shake.count = 22;
        shake.delay = 149;
        task = GimmickChampion_CreateShakeTask(field, &shake);
        FieldTaskManager_AddTask(Field_GetTaskManager(field), task, 0);
        *state = 1;
        break;
    case 1:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void GimmickChampion_SetChanged(ChampionRoomState *state, BOOL changed) {
    state->changed = changed;
}

static BOOL GimmickChampion_GetChanged(ChampionRoomState *state) {
    return state->changed;
}

static void GimmickChampion_SetOpen(ChampionRoomState *state, BOOL open) {
    state->open = open;
}

static BOOL GimmickChampion_GetOpen(ChampionRoomState *state) {
    return state->open;
}

static FieldTask *GimmickChampion_CreateShakeTask(Field *field, const CameraShake *shake) {
    FieldTask *task = FieldTask_Create(Field_GetHeapID(field), sizeof(CameraShakeTaskWork), GimmickChampion_ShakeTask);
    CameraShakeTaskWork *work = FieldTask_GetData(task);

    work->field = field;
    work->camera = Field_GetCameraSystem(field);
    work->shake = *shake;
    work->state = 0;
    return task;
}

static BOOL GimmickChampion_ShakeTask(void *data) {
    CameraShakeTaskWork *work = data;
    BOOL done = FALSE;
    CameraShake *shake = &work->shake;
    FieldCamera *camera = Field_GetCameraSystem(work->field);
    u16 angle;

    if (shake->delay > 0) {
        shake->delay--;
        return FALSE;
    }
    switch (work->state) {
    case 0:
        FieldCamera_FinishDelay(camera);
        work->state++;
        break;
    case 1:
        if (FieldCamera_IsDelayActive(work->camera)) {
            break;
        }
        FieldCamera_EVCameraInit(work->camera);
        work->bind = FieldCamera_GetBind(camera);
        if (FieldNoGridMapper_HasRailData(Field_GetNoGridMapper(work->field))) {
            work->refBind = FieldCamera_GetRefBind(camera);
            FieldCamera_SetRefBind(camera, NULL);
        }
        FieldCamera_ClearBind(camera);
        work->state++;
        // fallthrough
    case 2:
        shake->frame++;
        angle = (shake->frame << 16) / shake->period;
        if (shake->frame >= shake->period) {
            shake->count--;
            shake->frame = 0;
        }
        if (shake->count == 0) {
            shake->frame = 0;
            done = TRUE;
        }
        if (done) {
            VecFx32 zero = { 0, 0, 0 };

            FieldCamera_CoordsSetEyeOffset(camera, &zero);
            FieldCameraAnm_EVCameraEnd(camera);
            if (FieldNoGridMapper_HasRailData(Field_GetNoGridMapper(work->field))) {
                FieldCamera_SetRefBind(camera, work->refBind);
            }
            FieldCamera_SetBind(camera, work->bind);
            FieldCamera_EnableDelay(camera);
            work->state++;
        } else {
            fx32 offset = shake->amplitude * FX_SinIdx(angle);
            VecFx32 target = *work->bind;

            target.y += offset;
            FieldCamera_CoordsSetTarget(camera, &target);
        }
        break;
    case 3:
        return TRUE;
    }
    return FALSE;
}
