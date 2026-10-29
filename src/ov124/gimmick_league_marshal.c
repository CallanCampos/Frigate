#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_exp_obj.h"
#include "field/field_map.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_task.h"
#include "field/gimmick_league_marshal.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

#define GIMMICK_WORK_ID 1
#define GIMMICK_STATE_ID 41

// The models of the scene
enum {
    MODEL_0,
    MODEL_1,
    MODEL_PLATFORM,
    MODEL_3,
};

#define PLATFORM_TOP_Y FX32_CONST(352)
#define PLATFORM_BOTTOM_Y FX32_CONST(192)

#define POS_TO_GRID(pos) ((s16)(((pos) >> 4) / FX32_ONE))

// What the save keeps of the room
typedef struct {
    // Model 0's animations have played
    BOOL model0Done;
    // The platform is down
    BOOL platformDown;
} MarshalRoomState;

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    MarshalRoomState *state;
} MarshalRoomWork;

// The platform's descent
typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    fx32 y;
    fx32 targetY;
    fx32 speed;
    FieldActor *player;
    // The bounce once it lands: the frames of half a swing, the frame and the height
    u32 bouncePeriod;
    u16 bounceFrame;
    s16 bounceHeight;
} PlatformEventWork;

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
    const VecFx32 *bind;
    const VecFx32 *refBind;
} CameraShakeTaskWork;

static void MarshalRoom_ShowSavedState(MarshalRoomWork *work);
static void MarshalRoom_LoadScene(MarshalRoomWork *work);
static void MarshalRoom_FreeScene(MarshalRoomWork *work);
static void MarshalRoom_StepAnims(MarshalRoomWork *work);
static void MarshalRoom_InitModel(FieldExpObjSystem *expObj, u16 model, int anmCount);
static void MarshalRoom_PlayAnm(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped);
static void MarshalRoom_StopAnm(FieldExpObjSystem *expObj, u16 model, u16 anm);
static void MarshalRoom_ShowModel0End(MarshalRoomWork *work);
static void MarshalRoom_ShowModel1End(MarshalRoomWork *work);
static void MarshalRoom_PlayModel3FirstAnm(MarshalRoomWork *work);
static void MarshalRoom_PlayModel3SecondAnm(MarshalRoomWork *work);
static void MarshalRoom_ShowModel3End(MarshalRoomWork *work);
static GameEvent *MarshalRoom_CreatePlatformEvent(GameSystem *gsys);
static GameEventReturnCode MarshalRoom_PlatformEvent(GameEvent *event, u32 *state, void *data);
static void MarshalRoom_SetPlatformY(PlatformEventWork *work, fx32 y);
static void MarshalRoomState_SetModel0Done(MarshalRoomState *state, BOOL done);
static BOOL MarshalRoomState_IsModel0Done(MarshalRoomState *state);
static void MarshalRoomState_SetPlatformDown(MarshalRoomState *state, BOOL down);
static BOOL MarshalRoomState_IsPlatformDown(MarshalRoomState *state);
static FieldTask *MarshalRoom_CreateCameraShakeTask(Field *field, const CameraShake *shake);
static BOOL MarshalRoom_CameraShakeTask(void *data);

static const G3DSceneAnimationSetup sModel3Animations[] = { { 9, 0 }, { 10, 0 } };
static const G3DSceneAnimationSetup sModel0Animations[] = { { 1, 0 }, { 2, 0 } };
static const G3DSceneAnimationSetup sModel1Animations[] = { { 4, 0 }, { 5, 0 }, { 6, 0 } };

// clang-format off
static const G3DSceneResourceSetup sResources[11] = {
    { ARCID_GIMMICK_LEAGUE_MARSHAL, 0, 0 }, { ARCID_GIMMICK_LEAGUE_MARSHAL, 1, 0 }, { ARCID_GIMMICK_LEAGUE_MARSHAL, 2, 0 }, { ARCID_GIMMICK_LEAGUE_MARSHAL, 3, 0 },
    { ARCID_GIMMICK_LEAGUE_MARSHAL, 4, 0 }, { ARCID_GIMMICK_LEAGUE_MARSHAL, 5, 0 }, { ARCID_GIMMICK_LEAGUE_MARSHAL, 6, 0 }, { ARCID_GIMMICK_LEAGUE_MARSHAL, 7, 0 },
    { ARCID_GIMMICK_LEAGUE_MARSHAL, 8, 0 }, { ARCID_GIMMICK_LEAGUE_MARSHAL, 9, 0 }, { ARCID_GIMMICK_LEAGUE_MARSHAL, 10, 0 },
};

// clang-format on

static const G3DSceneActorSetup sActors[4] = {
    { 0, 0, 0, 0, sModel0Animations, NELEMS(sModel0Animations) },
    { 3, 0, 3, 0, sModel1Animations, NELEMS(sModel1Animations) },
    { 7, 0, 7, 0, NULL, 0 },
    { 8, 0, 8, 0, sModel3Animations, NELEMS(sModel3Animations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void MarshalRoom_Init(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    MarshalRoomWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(MarshalRoomWork));

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    work->state = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_STATE_ID);
    MarshalRoom_LoadScene(work);
    MarshalRoom_ShowSavedState(work);
}

void MarshalRoom_End(Field *field) {
    MarshalRoom_FreeScene(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void MarshalRoom_Update(Field *field) {
    MarshalRoom_StepAnims(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
}

void MarshalRoom_PlayModel0Anims(GameSystem *gsys) {
    MarshalRoomWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    MarshalRoom_PlayAnm(work->expObj, MODEL_0, 0, FALSE);
    MarshalRoom_PlayAnm(work->expObj, MODEL_0, 1, FALSE);
    MarshalRoomState_SetModel0Done(work->state, TRUE);
}

void MarshalRoom_PlayModel3Anim(GameSystem *gsys, u16 which) {
    MarshalRoomWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    if (which == 0) {
        MarshalRoom_PlayModel3FirstAnm(work);
    } else {
        MarshalRoom_PlayModel3SecondAnm(work);
    }
}

void MarshalRoom_PlayModel1Anims(GameSystem *gsys) {
    MarshalRoomWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    u32 i;

    for (i = 0; i < 3; i++) {
        MarshalRoom_PlayAnm(work->expObj, MODEL_1, i, FALSE);
    }
}

GameEvent *MarshalRoom_LowerPlatform(GameSystem *gsys) {
    MarshalRoomWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);
    GameEvent *event = MarshalRoom_CreatePlatformEvent(gsys);

    if (event != NULL) {
        MarshalRoomState_SetPlatformDown(work->state, TRUE);
    }
    return event;
}

// Shows the models as the saved state left them
static void MarshalRoom_ShowSavedState(MarshalRoomWork *work) {
    BOOL model0Done = MarshalRoomState_IsModel0Done(work->state);
    BOOL platformDown = MarshalRoomState_IsPlatformDown(work->state);
    fx32 y = PLATFORM_TOP_Y;
    SRTMatrix *matrix;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, FALSE);
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_1, FALSE);
    if (model0Done == FALSE && platformDown == FALSE) {
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_3, FALSE);
    } else if (model0Done == TRUE && platformDown == FALSE) {
        MarshalRoom_ShowModel3End(work);
        MarshalRoom_ShowModel0End(work);
    } else if (platformDown == TRUE) {
        MarshalRoom_ShowModel0End(work);
        y = PLATFORM_BOTTOM_Y;
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_PLATFORM, FALSE);
        MarshalRoom_ShowModel1End(work);
    }
    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, MODEL_PLATFORM);
    matrix->translation.x = FX32_CONST(248);
    matrix->translation.y = y;
    matrix->translation.z = FX32_CONST(120);
}

static void MarshalRoom_LoadScene(MarshalRoomWork *work) {
    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    MarshalRoom_InitModel(work->expObj, MODEL_0, NELEMS(sModel0Animations));
    MarshalRoom_InitModel(work->expObj, MODEL_1, NELEMS(sModel1Animations));
    MarshalRoom_InitModel(work->expObj, MODEL_PLATFORM, 0);
    MarshalRoom_InitModel(work->expObj, MODEL_3, NELEMS(sModel3Animations));
}

static void MarshalRoom_FreeScene(MarshalRoomWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void MarshalRoom_StepAnims(MarshalRoomWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Places the model, hidden, with its animations set and paused
static void MarshalRoom_InitModel(FieldExpObjSystem *expObj, u16 model, int anmCount) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    int i;

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
    matrix->translation.x = FX32_CONST(256);
    matrix->translation.y = 0;
    matrix->translation.z = FX32_CONST(256);
    FieldExpObj_SetActorCulling(expObj, 0, model, 1);
    FieldExpObj_SetActorHidden(expObj, 0, model, TRUE);
    for (i = 0; i < anmCount; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, model, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(expObj, 0, model, i, FALSE);
    }
}

// Plays the animation from its first frame
static void MarshalRoom_PlayAnm(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, FALSE);
    FieldExpObjAnm_SetLooped(anmInfo, looped);
    FieldExpObj_SetAnm(expObj, 0, model, anm, TRUE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Stops the animation at its first frame
static void MarshalRoom_StopAnm(FieldExpObjSystem *expObj, u16 model, u16 anm) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, TRUE);
    FieldExpObj_SetAnm(expObj, 0, model, anm, FALSE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Shows model 0 at the end of its animations
static void MarshalRoom_ShowModel0End(MarshalRoomWork *work) {
    FieldExpObjAnm *anm;
    u32 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_0, FALSE);
    for (i = 0; i < 2; i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_0, i);
        FieldExpObj_SetAnm(work->expObj, 0, MODEL_0, i, TRUE);
        FieldExpObj_SetAnmFrame(work->expObj, 0, MODEL_0, i, FieldExpObjAnm_GetLastFrame(anm));
    }
}

// Shows model 1 at the end of its animations
static void MarshalRoom_ShowModel1End(MarshalRoomWork *work) {
    FieldExpObjAnm *anm;
    u32 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_1, FALSE);
    for (i = 0; i < 3; i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_1, i);
        FieldExpObj_SetAnm(work->expObj, 0, MODEL_1, i, TRUE);
        FieldExpObj_SetAnmFrame(work->expObj, 0, MODEL_1, i, FieldExpObjAnm_GetLastFrame(anm));
    }
}

static void MarshalRoom_PlayModel3FirstAnm(MarshalRoomWork *work) {
    MarshalRoom_PlayAnm(work->expObj, MODEL_3, 0, FALSE);
    MarshalRoom_StopAnm(work->expObj, MODEL_3, 1);
}

static void MarshalRoom_PlayModel3SecondAnm(MarshalRoomWork *work) {
    MarshalRoom_PlayAnm(work->expObj, MODEL_3, 1, FALSE);
    MarshalRoom_StopAnm(work->expObj, MODEL_3, 0);
}

// Shows model 3 at the end of its first animation
static void MarshalRoom_ShowModel3End(MarshalRoomWork *work) {
    FieldExpObjAnm *anm;
    fx32 frame;

    anm = FieldExpObj_GetAnmInfo(work->expObj, 0, MODEL_3, 0);
    FieldExpObj_SetAnm(work->expObj, 0, MODEL_3, 0, TRUE);
    frame = FieldExpObjAnm_GetLastFrame(anm);
    FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_3, FALSE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, MODEL_3, 0, frame);
}

static GameEvent *MarshalRoom_CreatePlatformEvent(GameSystem *gsys) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, MarshalRoom_PlatformEvent, sizeof(PlatformEventWork));
    PlatformEventWork *work = GameEvent_GetData(event);

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    work->player = FindFieldActor(Field_GetActorSystem(field), 0);
    FldAct_SetAcmd(work->player, 100);
    SetActorFlag(work->player, 0x10);
    work->y = PLATFORM_TOP_Y;
    work->targetY = PLATFORM_BOTTOM_Y;
    work->speed = FX32_CONST(12);
    work->bounceFrame = 0;
    work->bouncePeriod = 1;
    work->bounceHeight = 30;
    return event;
}

static GameEventReturnCode MarshalRoom_PlatformEvent(GameEvent *event, u32 *state, void *data) {
    PlatformEventWork *work = data;
    GameSystem *gsys = GameEvent_GetGameSystem(event);
    MarshalRoomWork *gimmick = Field_GetGimmickWorkBlock(work->field, GIMMICK_WORK_ID);
    CameraShake shake;
    FieldTask *task;
    fx32 y;
    fx32 targetY;
    fx32 speed;
    int frames;
    fx16 sin;
    fx32 offset;
    u16 angle;

    switch (*state) {
    case 0:
        FieldExpObj_SetActorHidden(work->expObj, 0, MODEL_PLATFORM, FALSE);
        MI_CpuFill8(&shake, 0, sizeof(shake));
        shake.amplitude = 4;
        shake.period = 3;
        shake.count = 10;
        // Starts the shake five frames before the platform lands
        frames = 0;
        y = work->y;
        targetY = work->targetY;
        speed = work->speed;
        while (targetY <= y) {
            y -= speed;
            speed = FX_Mul(speed, 0x11a0);
            frames++;
        }
        shake.delay = frames - 5;
        task = MarshalRoom_CreateCameraShakeTask(work->field, &shake);
        FieldTaskManager_AddTask(Field_GetTaskManager(work->field), task, 0);
        *state = 1;
        break;
    case 1:
        work->y -= work->speed;
        work->speed = FX_Mul(work->speed, 0x11a0);
        if (work->targetY >= work->y) {
            work->y = work->targetY;
            GFL_SndSEPlay(SEQ_SE_SW_RENBU_04);
            *state = 2;
        }
        MarshalRoom_SetPlatformY(work, work->y);
        break;
    case 2:
        work->bounceFrame++;
        angle = (u16)((work->bounceFrame << 15) / work->bouncePeriod) % 0x8000;
        sin = FX_SinIdx(angle);
        offset = work->bounceHeight * sin;
        if (sin == 0) {
            work->bounceHeight -= 11;
        }
        if (work->bounceHeight <= 0) {
            offset = 0;
            *state = 3;
        }
        MarshalRoom_SetPlatformY(work, PLATFORM_BOTTOM_Y + offset);
        break;
    case 3:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(work->field))) {
            FldAct_FreeAcmd(work->player);
            ClearActorFlag(work->player, 0x10);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

// Moves the platform and the player on it to the height
static void MarshalRoom_SetPlatformY(PlatformEventWork *work, fx32 y) {
    VecFx32 pos;

    FieldExpObj_GetActorMatrixPtr(work->expObj, 0, MODEL_PLATFORM)->translation.y = y;
    pos = *GetMModelWPosPtr(work->player);
    pos.y = y;
    SetActorGPosY(work->player, POS_TO_GRID(y));
    SetActorWPosValue(work->player, &pos);
}

static void MarshalRoomState_SetModel0Done(MarshalRoomState *state, BOOL done) {
    state->model0Done = done;
}

static BOOL MarshalRoomState_IsModel0Done(MarshalRoomState *state) {
    return state->model0Done;
}

static void MarshalRoomState_SetPlatformDown(MarshalRoomState *state, BOOL down) {
    state->platformDown = down;
}

static BOOL MarshalRoomState_IsPlatformDown(MarshalRoomState *state) {
    return state->platformDown;
}

static FieldTask *MarshalRoom_CreateCameraShakeTask(Field *field, const CameraShake *shake) {
    FieldTask *task =
        FieldTask_Create(Field_GetHeapID(field), sizeof(CameraShakeTaskWork), MarshalRoom_CameraShakeTask);
    CameraShakeTaskWork *work = FieldTask_GetData(task);

    work->field = field;
    work->camera = Field_GetCameraSystem(field);
    work->shake = *shake;
    work->state = 0;
    return task;
}

static BOOL MarshalRoom_CameraShakeTask(void *data) {
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
