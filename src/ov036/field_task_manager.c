// Runs the field's tasks, and holds the tasks that move the camera and actors, fade the screen and wait. Names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_animation_curve.h"
#include "field/field_camera.h"
#include "field/field_effect.h"
#include "field/field_task.h"
#include "gfl/calctool.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/fx.h"

#define CAMERA_ANGLE_YAW 0
#define CAMERA_ANGLE_PITCH 1
#define ACTOR_MOVE_FORWARD 0
#define ACTOR_MOVE_BACKWARD 1

// The steps of the tasks that move over frames: take where they start from, then move each frame until they end
enum {
    TASK_STATE_START,
    TASK_STATE_UPDATE,
};

// The steps of the camera shake: end the camera's delay, wait for it to stop and free the camera from its bind, shake
// it, and end
enum {
    SHAKE_STATE_END_DELAY,
    SHAKE_STATE_WAIT_DELAY,
    SHAKE_STATE_SHAKE,
    SHAKE_STATE_END,
};

struct FieldTaskManager {
    u8 count;
    FieldTask **tasks;
    FieldTask **parents;
};

typedef struct {
    Field *field;
    u8 state;
    u16 frame;
    u16 duration;
    fx32 start;
    fx32 target;
} FieldCameraZoomTask;

typedef struct {
    Field *field;
    FieldCamera *camera;
    u32 axis;
    u8 state;
    u16 frame;
    u16 duration;
    u32 start;
    u32 target;
} FieldCameraAngleTask;

typedef struct {
    Field *field;
    FieldActor *actor;
    u16 frame;
    u16 duration;
    u8 turns;
    u16 dir;
} FieldActorSpinTask;

typedef struct {
    u8 state;
    Field *field;
    s32 frame;
    s32 duration;
    VecFx32 start;
    VecFx32 end;
} FieldPlayerMoveTask;

typedef struct {
    u8 state;
    Field *field;
    u16 frame;
    u16 duration;
    f32 height;
    FieldActor *actor;
} FieldActorFallTask;

typedef struct {
    u16 frame;
    u16 wait;
} FieldWaitTask;

typedef struct {
    Field *field;
    u32 mode;
    s32 from;
    s32 to;
    s32 speed;
} FieldFadeTask;

typedef struct {
    u8 state;
    Field *field;
    s32 direction;
    u16 frame;
    u16 duration;
    VecFx32 offset;
    VecFx32 start;
    FieldActor *actor;
} FieldActorMoveTask;

typedef struct {
    u8 state;
    Field *field;
    s32 frame;
    s32 duration;
    VecFx32 start;
    VecFx32 end;
} FieldCameraTargetOffsTask;

typedef struct {
    u8 state;
    Field *field;
    u16 frame;
    u16 duration;
    VecFx32 from;
    VecFx32 to;
    VecFx32 delta;
    u32 transformType;
} FieldCameraTargetTask;

typedef struct {
    Field *field;
    FieldEffects *effects;
    FieldActor *actor;
    u8 state;
    u16 frame;
    u16 duration;
    u16 interval;
} FieldActorEffectTask;

typedef struct {
    Field *field;
    FieldCamera *camera;
    s32 state;
    FieldCameraShakeSetup setup;
    const VecFx32 *bind;
} FieldCameraShakeTask;

static void FieldTaskManager_SetTask(FieldTaskManager *mgr, u8 index, FieldTask *task, FieldTask *parent);
static void FieldTaskManager_EndTask(FieldTaskManager *mgr, u8 index);
static void FieldTaskManager_RunTask(FieldTaskManager *mgr, u8 index);
static void FieldTaskManager_UpdateParentControlTransfer(FieldTaskManager *mgr, u8 index);
static void FieldTaskManager_UpdateEndTasks(FieldTaskManager *mgr, u8 index);
static BOOL FieldCameraMoveTaskZoom_Calc(void *taskData);
static void FieldCameraMoveTaskAngle_CalcStartValues(FieldCameraAngleTask *data);
static void FieldCameraMoveTaskAngle_Calc(FieldCameraAngleTask *data);
static BOOL FieldCameraMoveTaskAngle_Callback(void *work);
static u16 FieldActorSpinTask_CalcSpinDirAcmd(f32 elapsed, f32 duration, f32 turns);
static u16 FieldActorSpinTask_CalcSpinDirAcmdQuad(f32 elapsed, f32 duration, f32 turns);
static u16 FieldActorSpinTask_CalcSpinDirAcmdSqrt(f32 elapsed, f32 duration, f32 turns);
static BOOL FieldActorSpinTask_Callback(void *work);
static BOOL FieldActorSpinTask_CallbackAccel(void *work);
static BOOL FieldActorSpinTask_CallbackDeaccel(void *work);
static BOOL FieldPlayerMoveTask_Callback(void *work);
static void FieldActorFallTask_CalcFallPos(VecFx32 *pos, f32 elapsed, f32 duration, f32 height);
static BOOL FieldActorFallTask_Callback(void *work);
static BOOL FieldWaitTask_Callback(void *work);
static BOOL FieldFadeTask_Callback(void *work);
static void FieldActorMoveTask_InterpVector(VecFx32 *dest, u32 elapsed, VecFx32 *vec, u32 duration);
static BOOL FieldActorMoveTask_CallbackRel(void *work);
static BOOL FieldActorMoveTask_CallbackAbs(void *work);
static BOOL FieldCameraTargetMoveTaskTargetOffs_Callback(void *work);
static BOOL FieldCameraMoveTaskTarget_Callback(void *work);
static BOOL FieldActorEffectTask_Callback(void *work);
static BOOL FieldCameraShakeTask_Callback(void *work);

FieldTaskManager *FieldTaskManager_Create(u32 count, HeapID heapId) {
    FieldTaskManager *mgr = GFL_HeapAllocate(heapId, sizeof(FieldTaskManager), FALSE, "field_task_manager.c", 52);

    mgr->count = count;
    mgr->tasks = GFL_HeapAllocate(heapId, count * sizeof(FieldTask *), TRUE, "field_task_manager.c", 54);
    mgr->parents = GFL_HeapAllocate(heapId, count * sizeof(FieldTask *), TRUE, "field_task_manager.c", 55);
    return mgr;
}

void FieldTaskManager_Free(FieldTaskManager *mgr) {
    int i;

    for (i = 0; i < mgr->count; i++) {
        FieldTaskManager_EndTask(mgr, i);
    }
    GFL_HeapFree(mgr->tasks);
    GFL_HeapFree(mgr->parents);
    GFL_HeapFree(mgr);
}

void FieldTaskManager_Update(FieldTaskManager *mgr) {
    int i;

    for (i = 0; i < mgr->count; i++) {
        FieldTaskManager_RunTask(mgr, i);
    }
    for (i = 0; i < mgr->count; i++) {
        FieldTaskManager_UpdateParentControlTransfer(mgr, i);
    }
    for (i = 0; i < mgr->count; i++) {
        FieldTaskManager_UpdateEndTasks(mgr, i);
    }
}

void FieldTaskManager_AddTask(FieldTaskManager *mgr, FieldTask *task, FieldTask *parent) {
    int i;

    for (i = 0; i < mgr->count; i++) {
        if (mgr->tasks[i] == NULL) {
            FieldTaskManager_SetTask(mgr, i, task, parent);
            if (parent == NULL) {
                FieldTask_SetStarted(task);
            }
            return;
        }
    }
    FieldTask_Free(task);
}

u32 FieldTaskManager_GetActiveTaskCount(FieldTaskManager *mgr) {
    int i;
    u8 count = 0;

    for (i = 0; i < mgr->count; i++) {
        if (mgr->tasks[i] != NULL) {
            count++;
        }
    }
    return count;
}

BOOL FieldTaskManager_IsIdle(FieldTaskManager *mgr) {
    if (FieldTaskManager_GetActiveTaskCount(mgr) == 0) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldTaskManager_CheckCallbackRegistered(FieldTaskManager *mgr, FieldTaskCallback callback) {
    int i;

    for (i = 0; i < mgr->count; i++) {
        if (mgr->tasks[i] != NULL && FieldTask_GetCallback(mgr->tasks[i]) == callback) {
            return TRUE;
        }
    }
    return FALSE;
}

static void FieldTaskManager_SetTask(FieldTaskManager *mgr, u8 index, FieldTask *task, FieldTask *parent) {
    if (mgr->tasks[index] == NULL) {
        mgr->tasks[index] = task;
        mgr->parents[index] = parent;
    }
}

static void FieldTaskManager_EndTask(FieldTaskManager *mgr, u8 index) {
    if (mgr->tasks[index] != NULL) {
        FieldTask_Free(mgr->tasks[index]);
        mgr->tasks[index] = NULL;
        mgr->parents[index] = NULL;
    }
}

static void FieldTaskManager_RunTask(FieldTaskManager *mgr, u8 index) {
    if (mgr->tasks[index] != NULL) {
        FieldTask_Run(mgr->tasks[index]);
    }
}

static void FieldTaskManager_UpdateParentControlTransfer(FieldTaskManager *mgr, u8 index) {
    if (mgr->tasks[index] != NULL && mgr->parents[index] != NULL) {
        if (FieldTask_GetState(mgr->tasks[index]) == FIELD_TASK_STATE_WAIT &&
            FieldTask_GetState(mgr->parents[index]) == FIELD_TASK_STATE_END) {
            FieldTask_SetStarted(mgr->tasks[index]);
        }
    }
}

static void FieldTaskManager_UpdateEndTasks(FieldTaskManager *mgr, u8 index) {
    if (mgr->tasks[index] != NULL) {
        if (FieldTask_CheckStateReqEnd(mgr->tasks[index]) == TRUE) {
            FieldTaskManager_EndTask(mgr, index);
        }
    }
}

FieldTask *FieldCameraMoveTaskZoom_Create(Field *field, u16 duration, fx32 zoom) {
    FieldTask *task =
        FieldTask_Create(Field_GetHeapID(field), sizeof(FieldCameraZoomTask), FieldCameraMoveTaskZoom_Calc);
    FieldCameraZoomTask *data = FieldTask_GetData(task);

    data->state = TASK_STATE_START;
    data->field = field;
    data->frame = 0;
    data->duration = duration;
    data->target = zoom;
    return task;
}

static BOOL FieldCameraMoveTaskZoom_Calc(void *taskData) {
    FieldCameraZoomTask *data = taskData;
    FieldCamera *camera = Field_GetCameraSystem(data->field);

    switch (data->state) {
    case TASK_STATE_START:
        data->start = FieldCamera_CoordsGetZoom(camera);
        data->state++;
        break;
    case TASK_STATE_UPDATE: {
        u32 type;
        f32 ratio;
        f32 start;
        f32 target;
        f32 zoom;

        data->frame++;
        type = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        ratio = (f32)data->frame / (f32)data->duration;
        start = FX_FX32_TO_F32(data->start);
        target = FX_FX32_TO_F32(data->target);
        zoom = (1.0f - ratio) * start + ratio * target;
        FieldCamera_CoordsSetZoom(camera, FX32_CONST(zoom));
        FieldCamera_ChangeTransformType(camera, type);
        if (data->duration <= data->frame) {
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

FieldTask *FieldCameraMoveTaskYaw_Callback(Field *field, u32 duration, u16 yaw) {
    FieldTask *task =
        FieldTask_Create(Field_GetHeapID(field), sizeof(FieldCameraAngleTask), FieldCameraMoveTaskAngle_Callback);
    FieldCameraAngleTask *data = FieldTask_GetData(task);

    data->field = field;
    data->camera = Field_GetCameraSystem(field);
    data->state = TASK_STATE_START;
    data->axis = CAMERA_ANGLE_YAW;
    data->frame = 0;
    data->duration = duration;
    data->target = yaw;
    return task;
}

FieldTask *FieldCameraMoveTaskPitch_Callback(Field *field, u32 duration, u16 pitch) {
    FieldTask *task =
        FieldTask_Create(Field_GetHeapID(field), sizeof(FieldCameraAngleTask), FieldCameraMoveTaskAngle_Callback);
    FieldCameraAngleTask *data = FieldTask_GetData(task);

    data->field = field;
    data->camera = Field_GetCameraSystem(field);
    data->state = TASK_STATE_START;
    data->axis = CAMERA_ANGLE_PITCH;
    data->frame = 0;
    data->duration = duration;
    data->target = pitch;
    return task;
}

static void FieldCameraMoveTaskAngle_CalcStartValues(FieldCameraAngleTask *data) {
    FieldCamera *camera = data->camera;
    u32 start;
    u32 target = data->target;

    switch (data->axis) {
    case CAMERA_ANGLE_YAW:
    default:
        start = FieldCamera_CoordsGetYaw(camera);
        break;
    case CAMERA_ANGLE_PITCH:
        start = FieldCamera_CoordsGetPitch(camera);
        break;
    }
    if (start < target) {
        if (target - start > 0x8000) {
            start += 0x10000;
        }
    } else if (target < start) {
        if (start - target > 0x8000) {
            target += 0x10000;
        }
    }
    data->start = start;
    data->target = target;
}

static void FieldCameraMoveTaskAngle_Calc(FieldCameraAngleTask *data) {
    u16 duration = data->duration;
    FieldCamera *camera = data->camera;

    data->frame++;
    if (data->frame <= duration) {
        f32 ratio = (f32)data->frame / (f32)duration;
        f32 smooth = ratio * ratio * (-2.0f * ratio + 3.0f);
        u16 angle = (1.0f - smooth) * (f32)data->start + smooth * (f32)data->target;
        u32 type = FieldCamera_GetTransformType(camera);

        FieldCamera_ChangeTransformType(camera, 0);
        switch (data->axis) {
        case CAMERA_ANGLE_YAW:
            FieldCamera_CoordsSetYaw(camera, angle);
            break;
        case CAMERA_ANGLE_PITCH:
            FieldCamera_CoordsSetPitch(camera, angle);
            break;
        }
        FieldCamera_ChangeTransformType(camera, type);
    }
}

static BOOL FieldCameraMoveTaskAngle_Callback(void *work) {
    FieldCameraAngleTask *data = work;

    switch (data->state) {
    case TASK_STATE_START:
        FieldCameraMoveTaskAngle_CalcStartValues(data);
        data->state++;
        break;
    case TASK_STATE_UPDATE:
        FieldCameraMoveTaskAngle_Calc(data);
        if (data->duration <= data->frame) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

FieldTask *FieldActorSpinTask_CreatePlayer(Field *field, u32 duration, u32 turns) {
    HeapID heapId = Field_GetHeapID(field);
    FieldPlayer *player = Field_GetPlayer(field);
    FieldTask *task = FieldTask_Create(heapId, sizeof(FieldActorSpinTask), FieldActorSpinTask_Callback);
    FieldActorSpinTask *data = FieldTask_GetData(task);

    data->field = field;
    data->actor = FieldPlayer_GetActor(player);
    data->frame = 0;
    data->duration = duration;
    data->turns = turns;
    data->dir = GetActorFaceDir(data->actor);
    return task;
}

FieldTask *FieldActorSpinTask_Create(Field *field, u32 duration, u32 turns, FieldActor *actor) {
    FieldTask *task = FieldTask_Create(Field_GetHeapID(field), sizeof(FieldActorSpinTask), FieldActorSpinTask_Callback);
    FieldActorSpinTask *data = FieldTask_GetData(task);

    data->field = field;
    data->actor = actor;
    data->frame = 0;
    data->duration = duration;
    data->turns = turns;
    data->dir = GetActorFaceDir(actor);
    return task;
}

FieldTask *FieldActorSpinTask_CreatePlayerAccel(Field *field, u32 duration, u32 turns) {
    HeapID heapId = Field_GetHeapID(field);
    FieldPlayer *player = Field_GetPlayer(field);
    FieldTask *task = FieldTask_Create(heapId, sizeof(FieldActorSpinTask), FieldActorSpinTask_CallbackAccel);
    FieldActorSpinTask *data = FieldTask_GetData(task);

    data->field = field;
    data->actor = FieldPlayer_GetActor(player);
    data->frame = 0;
    data->duration = duration;
    data->turns = turns;
    data->dir = GetActorFaceDir(data->actor);
    return task;
}

FieldTask *FieldActorSpinTask_CreatePlayerDeaccel(Field *field, u32 duration, u32 turns, u32 dir) {
    HeapID heapId = Field_GetHeapID(field);
    FieldPlayer *player = Field_GetPlayer(field);
    FieldTask *task = FieldTask_Create(heapId, sizeof(FieldActorSpinTask), FieldActorSpinTask_CallbackDeaccel);
    FieldActorSpinTask *data = FieldTask_GetData(task);

    data->field = field;
    data->actor = FieldPlayer_GetActor(player);
    data->frame = 0;
    data->duration = duration;
    data->turns = turns;
    data->dir = dir;
    return task;
}

// The animation command that turns the actor to the direction it has after elapsed of duration frames of a spin of
// the given number of turns, steady
static u16 FieldActorSpinTask_CalcSpinDirAcmd(f32 elapsed, f32 duration, f32 turns) {
    u16 acmds[4] = { ACMD_FACE + DIR_DOWN, ACMD_FACE + DIR_RIGHT, ACMD_FACE + DIR_UP, ACMD_FACE + DIR_LEFT };
    f32 rate = 4.0f * turns / duration;

    return acmds[(s32)(rate * elapsed) % 4];
}

// The same, speeding up
static u16 FieldActorSpinTask_CalcSpinDirAcmdQuad(f32 elapsed, f32 duration, f32 turns) {
    u16 acmds[4] = { ACMD_FACE + DIR_DOWN, ACMD_FACE + DIR_RIGHT, ACMD_FACE + DIR_UP, ACMD_FACE + DIR_LEFT };

    f32 rate = 4.0f * turns / (duration * duration);

    return acmds[(s32)(rate * elapsed * elapsed) % 4];
}

// The same, slowing down
static u16 FieldActorSpinTask_CalcSpinDirAcmdSqrt(f32 elapsed, f32 duration, f32 turns) {
    u16 acmds[4] = { ACMD_FACE + DIR_DOWN, ACMD_FACE + DIR_RIGHT, ACMD_FACE + DIR_UP, ACMD_FACE + DIR_LEFT };
    f32 distance = 4.0f * turns;
    f32 rate = distance * distance / duration;
    f32 value = FieldAnimationCurve_Sqrt(rate, elapsed);

    return acmds[(s32)value % 4];
}

static BOOL FieldActorSpinTask_Callback(void *work) {
    FieldActorSpinTask *data = work;
    FieldActor *actor = data->actor;
    u16 acmd;

    FldAct_SetAcmd(actor, FieldActorSpinTask_CalcSpinDirAcmd(data->frame, data->duration, data->turns));
    data->frame++;
    if (data->duration < data->frame) {
        switch (data->dir) {
        case DIR_UP:
        default:
            acmd = ACMD_FACE + DIR_UP;
            break;
        case DIR_DOWN:
            acmd = ACMD_FACE + DIR_DOWN;
            break;
        case DIR_LEFT:
            acmd = ACMD_FACE + DIR_LEFT;
            break;
        case DIR_RIGHT:
            acmd = ACMD_FACE + DIR_RIGHT;
            break;
        }
        FldAct_SetAcmd(actor, acmd);
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldActorSpinTask_CallbackAccel(void *work) {
    FieldActorSpinTask *data = work;
    FieldActor *actor = data->actor;
    u16 acmd;

    FldAct_SetAcmd(actor, FieldActorSpinTask_CalcSpinDirAcmdQuad(data->frame, data->duration, data->turns));
    data->frame++;
    if (data->duration < data->frame) {
        switch (data->dir) {
        case DIR_UP:
        default:
            acmd = ACMD_FACE + DIR_UP;
            break;
        case DIR_DOWN:
            acmd = ACMD_FACE + DIR_DOWN;
            break;
        case DIR_LEFT:
            acmd = ACMD_FACE + DIR_LEFT;
            break;
        case DIR_RIGHT:
            acmd = ACMD_FACE + DIR_RIGHT;
            break;
        }
        FldAct_SetAcmd(actor, acmd);
        return TRUE;
    }
    return FALSE;
}

static BOOL FieldActorSpinTask_CallbackDeaccel(void *work) {
    FieldActorSpinTask *data = work;
    FieldActor *actor = data->actor;

    FldAct_SetAcmd(actor, FieldActorSpinTask_CalcSpinDirAcmdSqrt(data->frame, data->duration, data->turns));
    data->frame++;
    if (data->duration < data->frame) {
        CheckSetActorFaceDir(actor, data->dir);
        return TRUE;
    }
    return FALSE;
}

FieldTask *FieldPlayerMoveTask_Create(Field *field, u32 duration, VecFx32 *pos) {
    FieldTask *task =
        FieldTask_Create(Field_GetHeapID(field), sizeof(FieldPlayerMoveTask), FieldPlayerMoveTask_Callback);
    FieldPlayerMoveTask *data = FieldTask_GetData(task);

    data->state = TASK_STATE_START;
    data->field = field;
    data->frame = 0;
    data->duration = duration;
    VEC_Set(&data->end, pos->x, pos->y, pos->z);
    return task;
}

static BOOL FieldPlayerMoveTask_Callback(void *work) {
    FieldPlayerMoveTask *data = work;
    FieldPlayer *player = Field_GetPlayer(data->field);
    FieldActor *actor = FieldPlayer_GetActor(player);

    switch (data->state) {
    case TASK_STATE_START:
        FieldPlayer_GetWPos(player, &data->start);
        data->state++;
        break;
    case TASK_STATE_UPDATE: {
        s32 duration = data->duration;
        s32 frame = data->frame + 1;
        fx32 ratio;
        VecFx32 startPart;
        VecFx32 endPart;
        VecFx32 pos;
        VecFx32 current;

        data->frame = frame;
        ratio = FX32_CONST((f32)frame / (f32)duration);
        vecfx_mul(&data->start, FX32_ONE - ratio, &startPart);
        vecfx_mul(&data->end, ratio, &endPart);
        VEC_Add(&startPart, &endPart, &pos);
        FieldPlayer_GetWPos(player, &current);
        pos.y = current.y;
        FieldPlayer_SetWPos(player, &pos);
        CheckRecalcActorY(actor);
        FieldAcmdSyncBillboardPos(actor);
        if (data->duration <= data->frame) {
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

FieldTask *FieldActorFallTask_Create(Field *field, FieldActor *actor, u32 duration, s32 height) {
    FieldTask *task = FieldTask_Create(Field_GetHeapID(field), sizeof(FieldActorFallTask), FieldActorFallTask_Callback);
    FieldActorFallTask *data = FieldTask_GetData(task);

    data->state = TASK_STATE_START;
    data->field = field;
    data->frame = 0;
    data->duration = duration;
    data->height = height;
    data->actor = actor;
    return task;
}

static void FieldActorFallTask_CalcFallPos(VecFx32 *pos, f32 elapsed, f32 duration, f32 height) {
    f32 accel = height / (duration * duration);
    f32 y = height - accel * elapsed * elapsed;

    pos->x = 0;
    pos->y = FX32_CONST(y);
    pos->z = 0;
}

static BOOL FieldActorFallTask_Callback(void *work) {
    FieldActorFallTask *data = work;
    Field_GetPlayer(data->field);
    FieldActor *actor = data->actor;
    VecFx32 pos;

    switch (data->state) {
    case TASK_STATE_START:
        FieldActorFallTask_CalcFallPos(&pos, data->frame, data->duration, data->height);
        SetActorWPosOffset(actor, &pos);
        GFL_SndSEPlay(SEQ_SE_FLD_16);
        data->state++;
        break;
    case TASK_STATE_UPDATE:
        data->frame++;
        FieldActorFallTask_CalcFallPos(&pos, data->frame, data->duration, data->height);
        SetActorWPosOffset(actor, &pos);
        if (data->duration <= data->frame) {
            FldeffDust_Start(actor, Field_GetFieldEffects(data->field));
            GFL_SndSEPlay(SEQ_SE_FLD_17);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

FieldTask *FieldWaitTask_Create(Field *field, u16 wait) {
    FieldTask *task = FieldTask_Create(Field_GetHeapID(field), sizeof(FieldWaitTask), FieldWaitTask_Callback);
    FieldWaitTask *data = FieldTask_GetData(task);

    data->frame = 0;
    data->wait = wait;
    return task;
}

static BOOL FieldWaitTask_Callback(void *work) {
    FieldWaitTask *data = work;

    data->frame++;
    if (data->wait <= data->frame) {
        return TRUE;
    }
    return FALSE;
}

FieldTask *FieldFadeTask_Create(Field *field, u32 mode, u32 from, u32 to, u32 speed) {
    FieldTask *task = FieldTask_Create(Field_GetHeapID(field), sizeof(FieldFadeTask), FieldFadeTask_Callback);
    FieldFadeTask *data = FieldTask_GetData(task);

    data->field = field;
    data->mode = mode;
    data->from = from;
    data->to = to;
    data->speed = speed;
    return task;
}

static BOOL FieldFadeTask_Callback(void *work) {
    FieldFadeTask *data = work;

    GFL_FadeSet(data->mode, data->from, data->to, data->speed);
    return TRUE;
}

FieldTask *FieldActorMoveTask_CreatePlayer(Field *field, u32 duration, const VecFx32 *offset) {
    return FieldActorMoveTask_CreateRel(field, duration, offset, FieldPlayer_GetActor(Field_GetPlayer(field)));
}

FieldTask *FieldActorMoveTask_CreateRel(Field *field, s32 duration, const VecFx32 *offset, FieldActor *actor) {
    FieldTask *task =
        FieldTask_Create(Field_GetHeapID(field), sizeof(FieldActorMoveTask), FieldActorMoveTask_CallbackRel);
    FieldActorMoveTask *data = FieldTask_GetData(task);

    data->state = 0;
    data->field = field;
    data->actor = actor;
    CopyActorWPos(actor, &data->start);
    if (duration >= 0) {
        data->direction = ACTOR_MOVE_FORWARD;
        data->frame = 0;
        data->duration = duration;
    } else {
        data->direction = ACTOR_MOVE_BACKWARD;
        data->frame = 0;
        data->duration = -duration;
    }
    VEC_Set(&data->offset, offset->x, offset->y, offset->z);
    return task;
}

FieldTask *FieldActorMoveTask_CreateAbs(Field *field, s32 duration, const VecFx32 *pos, FieldActor *actor) {
    FieldTask *task =
        FieldTask_Create(Field_GetHeapID(field), sizeof(FieldActorMoveTask), FieldActorMoveTask_CallbackAbs);
    FieldActorMoveTask *data = FieldTask_GetData(task);

    data->state = 0;
    data->field = field;
    data->actor = actor;
    CopyActorWPos(actor, &data->start);
    if (duration >= 0) {
        data->direction = ACTOR_MOVE_FORWARD;
        data->frame = 0;
        data->duration = duration;
    } else {
        data->direction = ACTOR_MOVE_BACKWARD;
        data->frame = 0;
        data->duration = -duration;
    }
    VEC_Set(&data->offset, pos->x, pos->y, pos->z);
    return task;
}

static void FieldActorMoveTask_InterpVector(VecFx32 *dest, u32 elapsed, VecFx32 *vec, u32 duration) {
    fx32 ratio = FX32_CONST((f32)elapsed / (f32)duration);

    dest->x = FX_Mul(vec->x, ratio);
    dest->y = FX_Mul(vec->y, ratio);
    dest->z = FX_Mul(vec->z, ratio);
}

static BOOL FieldActorMoveTask_CallbackRel(void *work) {
    FieldActorMoveTask *data = work;
    VecFx32 offset;
    FieldActor *actor = data->actor;

    if (data->state == 0) {
        data->frame++;
        if (data->direction == ACTOR_MOVE_FORWARD) {
            FieldActorMoveTask_InterpVector(&offset, data->frame, &data->offset, data->duration);
        } else if (data->direction == ACTOR_MOVE_BACKWARD) {
            FieldActorMoveTask_InterpVector(&offset, (u16)(data->duration - data->frame), &data->offset,
                                            data->duration);
        }
        SetActorWPosOffset(actor, &offset);
        if (data->duration <= data->frame) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FieldActorMoveTask_CallbackAbs(void *work) {
    FieldActorMoveTask *data = work;
    VecFx32 pos;
    FieldActor *actor = data->actor;

    if (data->state == 0) {
        data->frame++;
        if (data->direction == ACTOR_MOVE_FORWARD) {
            FieldActorMoveTask_InterpVector(&pos, data->frame, &data->offset, data->duration);
        } else if (data->direction == ACTOR_MOVE_BACKWARD) {
            FieldActorMoveTask_InterpVector(&pos, (u16)(data->duration - data->frame), &data->offset, data->duration);
        }
        VEC_Add(&pos, &data->start, &pos);
        SetActorWPosValue(actor, &pos);
        if (data->duration <= data->frame) {
            return TRUE;
        }
    }
    return FALSE;
}

FieldTask *FieldCameraMoveTaskTargetOffs_Create(Field *field, u32 duration, const VecFx32 *offset) {
    FieldTask *task = FieldTask_Create(Field_GetHeapID(field), sizeof(FieldCameraTargetOffsTask),
                                       FieldCameraTargetMoveTaskTargetOffs_Callback);
    FieldCameraTargetOffsTask *data = FieldTask_GetData(task);

    data->state = TASK_STATE_START;
    data->field = field;
    data->frame = 0;
    data->duration = duration;
    VEC_Set(&data->end, offset->x, offset->y, offset->z);
    return task;
}

static BOOL FieldCameraTargetMoveTaskTargetOffs_Callback(void *work) {
    FieldCameraTargetOffsTask *data = work;
    FieldCamera *camera = Field_GetCameraSystem(data->field);

    switch (data->state) {
    case TASK_STATE_START:
        FieldCamera_CoordsGetTargetOffset(camera, &data->start);
        data->state++;
        break;
    case TASK_STATE_UPDATE: {
        s32 duration = data->duration;
        s32 frame = data->frame + 1;
        fx32 ratio;
        VecFx32 startPart;
        VecFx32 endPart;
        VecFx32 offset;

        data->frame = frame;
        ratio = FX32_CONST((f32)frame / (f32)duration);
        vecfx_mul(&data->start, FX32_ONE - ratio, &startPart);
        vecfx_mul(&data->end, ratio, &endPart);
        VEC_Add(&startPart, &endPart, &offset);
        FieldCamera_CoordsSetTargetOffset(camera, &offset);
        if (data->duration <= data->frame) {
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

FieldTask *FieldCameraMoveTaskTarget_Create(Field *field, u16 duration, const VecFx32 *from, const VecFx32 *to) {
    HeapID heapId = Field_GetHeapID(field);
    FieldTask *task;
    FieldCameraTargetTask *data;

    Field_GetCameraSystem(field);
    task = FieldTask_Create(heapId, sizeof(FieldCameraTargetTask), FieldCameraMoveTaskTarget_Callback);
    data = FieldTask_GetData(task);
    data->state = TASK_STATE_START;
    data->field = field;
    data->frame = 0;
    data->duration = duration;
    data->from = *from;
    data->to = *to;
    VEC_Subtract(&data->to, &data->from, &data->delta);
    return task;
}

static BOOL FieldCameraMoveTaskTarget_Callback(void *work) {
    FieldCameraTargetTask *data = work;
    FieldCamera *camera = Field_GetCameraSystem(data->field);

    switch (data->state) {
    case TASK_STATE_START:
        data->transformType = FieldCamera_GetTransformType(camera);
        FieldCamera_ChangeTransformType(camera, 0);
        data->state++;
        break;
    case TASK_STATE_UPDATE: {
        f32 progress;
        fx32 ratio;
        VecFx32 pos;

        data->frame++;
        progress = (f32)data->frame / (f32)data->duration;
        ratio = FX32_CONST(progress);
        VEC_MultAdd(ratio, &data->delta, &data->from, &pos);
        FieldCamera_CoordsSetTarget(camera, &pos);
        if (data->duration <= data->frame) {
            FieldCamera_ChangeTransformType(camera, data->transformType);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

FieldTask *FieldActorEffectTask_Create(Field *field, FieldActor *actor, u16 duration, u16 interval) {
    HeapID heapId = Field_GetHeapID(field);
    FieldPlayer *player = Field_GetPlayer(field);
    FieldTask *task = FieldTask_Create(heapId, sizeof(FieldActorEffectTask), FieldActorEffectTask_Callback);
    FieldActorEffectTask *data = FieldTask_GetData(task);

    data->field = field;
    data->effects = Field_GetFieldEffects(field);
    // BUG: The supplied actor is ignored in favor of the player's actor.
#ifdef BUGFIX
    data->actor = actor;
#else
    data->actor = FieldPlayer_GetActor(player);
#endif
    data->state = 0;
    data->frame = 0;
    data->duration = duration;
    data->interval = interval;
    return task;
}

static BOOL FieldActorEffectTask_Callback(void *work) {
    FieldActorEffectTask *data = work;

    if (data->state == 0) {
        if (data->frame % data->interval == 0) {
            FldeffRockSmash_Start(data->actor, data->effects);
        }
        if (data->duration <= data->frame++) {
            return TRUE;
        }
    }
    return FALSE;
}

FieldTask *FieldCameraShakeTask_Create(Field *field, const FieldCameraShakeSetup *setup) {
    FieldTask *task =
        FieldTask_Create(Field_GetHeapID(field), sizeof(FieldCameraShakeTask), FieldCameraShakeTask_Callback);
    FieldCameraShakeTask *data = FieldTask_GetData(task);

    data->field = field;
    data->camera = Field_GetCameraSystem(field);
    data->setup = *setup;
    data->state = SHAKE_STATE_END_DELAY;
    return task;
}

static BOOL FieldCameraShakeTask_Callback(void *work) {
    FieldCameraShakeTask *data = work;
    BOOL finished = FALSE;
    FieldCameraShakeSetup *setup = &data->setup;
    FieldCamera *camera = Field_GetCameraSystem(data->field);

    if (setup->delay > 0) {
        setup->delay--;
        return finished;
    }
    switch (data->state) {
    case SHAKE_STATE_END_DELAY:
        FieldCamera_FinishDelay(camera);
        data->state++;
        break;
    case SHAKE_STATE_WAIT_DELAY:
        if (FieldCamera_IsDelayActive(data->camera)) {
            break;
        }
        FieldCamera_EVCameraInit(data->camera);
        data->bind = FieldCamera_GetBind(camera);
        FieldCamera_ClearBind(camera);
        data->state++;
    case SHAKE_STATE_SHAKE: {
        u16 angle;

        setup->frame++;
        angle = (setup->frame << 16) / setup->period;
        if (setup->decayStart != 0 && setup->totalSteps - setup->steps >= setup->decayStart) {
            setup->decayTimer++;
            if (setup->decayTimer >= setup->decayInterval) {
                setup->decayTimer = 0;
                setup->amplitudeY -= setup->decayY;
                setup->amplitudeX -= setup->decayX;
                if (setup->amplitudeY <= 0) {
                    setup->amplitudeY = 1;
                }
                if (setup->amplitudeX <= 0) {
                    setup->amplitudeX = 1;
                }
            }
        }
        if (setup->frame >= setup->period) {
            setup->steps--;
            setup->frame = 0;
        }
        if (setup->steps == 0) {
            setup->frame = 0;
            finished = TRUE;
        }
        if (finished) {
            VecFx32 zero = { 0, 0, 0 };

            FieldCamera_CoordsSetEyeOffset(camera, &zero);
            FieldCameraAnm_EVCameraEnd(camera);
            FieldCamera_SetBind(camera, data->bind);
            FieldCamera_EnableDelay(camera);
            data->state++;
        } else {
            VecFx32 target;
            VecFx32 eyeOffset;
            fx32 offsetX = setup->amplitudeX * FX_SinIdx(angle);
            fx32 offsetY = setup->amplitudeY * FX_SinIdx(angle);
            u8 mode;

            target = *data->bind;
            target.x += offsetX;
            target.y += offsetY;
            eyeOffset.x = offsetX;
            eyeOffset.y = offsetY;
            eyeOffset.z = 0;
            mode = setup->mode;
            if (mode <= CAMERA_SHAKE_TARGET) {
                FieldCamera_CoordsSetTarget(camera, &target);
            }
            if (mode == CAMERA_SHAKE_BOTH || mode == CAMERA_SHAKE_EYE) {
                FieldCamera_CoordsSetEyeOffset(camera, &eyeOffset);
            }
        }
        break;
    }
    case SHAKE_STATE_END:
        return TRUE;
    }
    return FALSE;
}
