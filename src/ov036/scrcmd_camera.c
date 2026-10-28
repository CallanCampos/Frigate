// The script commands of the event camera: starting and ending it, binding it, moving it to set or stored positions,
// returning it, waiting for it and shaking it. The ROM has no name for the file; scrcmd_camera.c is descriptive.
// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_camera.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_script.h"
#include "field/scrcmd_camera.h"
#include "gfl/arc.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

// A stored camera position, from archive 161
typedef struct {
    u16 pitch;
    u16 yaw;
    fx32 distance;
    VecFx32 targetPos;
    FieldEvCameraAnimationFlags flags;
} EvCameraMoveData;

static GameEventReturnCode EventWaitCameraSetEvCamera_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFieldCameraWait_Callback(GameEvent *event, u32 *state, void *data);

static void CloseEventCamera(Field *field, FieldCamera *camera) {
    FieldCameraAnm_EVCameraEnd(camera);
    if (FieldCamera_SupportsDelay(camera)) {
        FieldCamera_EnableDelay(camera);
    }
    if (Field_GetResolvedControllerTypeID(field) == 1) {
        FieldNoGridMapper_SetCameraAreaEnabled(Field_GetNoGridMapper(field), TRUE);
    }
    FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_EV_CAMERA);
}

BOOL FieldScriptSubEventFinish_EvCamera(FinishScriptSubEventsWork *work, u32 *state) {
    Field *field = GSYS_GetField(work->gsys);

    CloseEventCamera(field, Field_GetCameraSystem(field));
    return TRUE;
}

BOOL Scrcmd_Camera_EvCameraInit(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    FieldCamera *camera = Field_GetCameraSystem(field);

    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_EV_CAMERA);
    if (FieldCamera_SupportsDelay(camera)) {
        // Let the camera catch up first
        FieldCamera_FinishDelay(camera);
        ScriptWork_CallEvent(FieldScriptEnv_GetScriptWork(env),
                             GameEvent_Create(gsys, NULL, EventWaitCameraSetEvCamera_Callback, 0));
        if (Field_GetResolvedControllerTypeID(field) == 1) {
            FieldNoGridMapper_SetCameraAreaEnabled(Field_GetNoGridMapper(field), FALSE);
        }
    } else {
        FieldCamera_EVCameraInit(camera);
    }
    return TRUE;
}

BOOL Scrcmd_Camera_EvCameraEnd(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));

    CloseEventCamera(field, Field_GetCameraSystem(field));
    return FALSE;
}

BOOL Scrcmd_Camera_EvCameraUnbind(VM *vm, FieldScriptEnv *env) {
    FieldCamera_ClearBind(Field_GetCameraSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
    return FALSE;
}

BOOL Scrcmd_Camera_EvCameraRebind(VM *vm, FieldScriptEnv *env) {
    FieldCamera_ResetBind(Field_GetCameraSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
    return FALSE;
}

BOOL Scrcmd_Camera_EvCameraMoveTo(VM *vm, FieldScriptEnv *env) {
    FieldEvCameraAnimationSetup setup;
    FieldCamera *camera = Field_GetCameraSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));

    setup.targetCoords.pitch = VM_Read16(vm);
    setup.targetCoords.yaw = VM_Read16(vm);
    setup.targetCoords.distance = VM_Read32(vm);
    setup.targetCoords.targetPos.x = VM_Read32(vm);
    setup.targetCoords.targetPos.y = VM_Read32(vm);
    setup.targetCoords.targetPos.z = VM_Read32(vm);
    setup.flags.animateExtraTranslation = FALSE;
    setup.flags.animatePitch = TRUE;
    setup.flags.animateYaw = TRUE;
    setup.flags.animateTargetDistance = TRUE;
    setup.flags.animateFOV = FALSE;
    setup.flags.animateTargetPos = TRUE;
    FieldCameraAnm_SetAnimation(camera, &setup, VM_Read16(vm));
    return FALSE;
}

BOOL Scrcmd_Camera_EvCameraMoveToCommon(VM *vm, FieldScriptEnv *env) {
    FieldEvCameraAnimationSetup setup;
    EvCameraMoveData data;
    FieldCamera *camera = Field_GetCameraSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    u16 dataId = VM_Read16(vm);
    u16 frames = VM_Read16(vm);

    GFL_ArcSysRead(&data, 161, dataId);
    setup.targetCoords.pitch = data.pitch;
    setup.targetCoords.yaw = data.yaw;
    setup.targetCoords.distance = data.distance;
    setup.targetCoords.targetPos = data.targetPos;
    setup.flags = data.flags;
    FieldCameraAnm_SetAnimation(camera, &setup, frames);
    return FALSE;
}

BOOL Scrcmd_Camera_EvCameraReturn(VM *vm, FieldScriptEnv *env) {
    FieldEvCameraAnimationFlags flags;
    FieldCamera *camera = Field_GetCameraSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    u16 frames = VM_Read16(vm);

    flags.animateExtraTranslation = TRUE;
    flags.animatePitch = TRUE;
    flags.animateYaw = TRUE;
    flags.animateTargetDistance = TRUE;
    flags.animateFOV = TRUE;
    flags.animateTargetPos = TRUE;
    FieldCameraAnm_SetReturnAnimation(camera, &flags, frames);
    return FALSE;
}

BOOL Scrcmd_Camera_EvCameraMoveToDefault(VM *vm, FieldScriptEnv *env) {
    FieldCamera *camera = Field_GetCameraSystem(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));

    FieldCameraAnm_SetLoadDefaultsAnimation(camera, VM_Read16(vm));
    return FALSE;
}

BOOL Scrcmd_Camera_EvCameraWait(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work,
                         GameEvent_Create(FieldScriptEnv_GetGameSystem(env), NULL, EventFieldCameraWait_Callback, 0));
    return TRUE;
}

BOOL Scrcmd_Camera_EvCameraShake(VM *vm, FieldScriptEnv *env) {
    FieldCameraShakeSetup shake;
    u16 amplitudeX = ScriptReadAny(vm, env);
    u16 amplitudeY = ScriptReadAny(vm, env);
    u16 period = ScriptReadAny(vm, env);
    u16 steps = ScriptReadAny(vm, env);
    u16 decayX = ScriptReadAny(vm, env);
    u16 decayY = ScriptReadAny(vm, env);
    u16 decayStart = ScriptReadAny(vm, env);
    u16 decayInterval = ScriptReadAny(vm, env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    shake.amplitudeX = amplitudeX;
    shake.amplitudeY = amplitudeY;
    shake.period = period;
    shake.steps = steps;
    shake.totalSteps = steps;
    shake.frame = 0;
    shake.decayX = decayX;
    shake.decayY = decayY;
    shake.decayStart = decayStart;
    shake.decayTimer = 0;
    shake.decayInterval = decayInterval;
    ScriptWork_CallEvent(work, EventEvCameraShake_Create(gsys, &shake));
    return TRUE;
}

static GameEventReturnCode EventWaitCameraSetEvCamera_Callback(GameEvent *event, u32 *state, void *data) {
    FieldCamera *camera = Field_GetCameraSystem(GSYS_GetField(GameEvent_GetGameSystem(event)));

    if (!FieldCamera_IsDelayActive(camera)) {
        FieldCamera_EVCameraInit(camera);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode EventFieldCameraWait_Callback(GameEvent *event, u32 *state, void *data) {
    if (FieldCamera_IsAnimating(Field_GetCameraSystem(GSYS_GetField(GameEvent_GetGameSystem(event))))) {
        return GAMEEVENT_CONTINUE;
    }
    return GAMEEVENT_DONE;
}
