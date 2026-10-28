#ifndef POKEBW2_FIELD_SCRCMD_CAMERA_H
#define POKEBW2_FIELD_SCRCMD_CAMERA_H

// Overlay 36's scrcmd_camera.c: the script commands of the event camera. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/field_script.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Camera_EvCameraInit(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Camera_EvCameraEnd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Camera_EvCameraUnbind(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Camera_EvCameraRebind(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Camera_EvCameraMoveTo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Camera_EvCameraMoveToCommon(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Camera_EvCameraReturn(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Camera_EvCameraMoveToDefault(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Camera_EvCameraWait(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Camera_EvCameraShake(VM *vm, FieldScriptEnv *env);
// FIELD_SCRIPT_SUB_EVENT_FINISH_FUNCS's entry for the event camera
BOOL FieldScriptSubEventFinish_EvCamera(FinishScriptSubEventsWork *work, u32 *state);

#endif // POKEBW2_FIELD_SCRCMD_CAMERA_H
