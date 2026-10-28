#ifndef POKEBW2_FIELD_FIELD_MOVE_SCRIPTS_H
#define POKEBW2_FIELD_FIELD_MOVE_SCRIPTS_H

#include "field/field_script.h"

BOOL ScriptNative_SurfTCBWait(VM *vm, void *env);
BOOL Scrcmd_FieldMove_CallSurf(VM *vm, FieldScriptEnv *env);
BOOL ScriptNative_WaterfallTCBWait(VM *vm, void *env);
BOOL Scrcmd_FieldMove_CallWaterfall(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldMove_CallCut(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FieldMove_CallDiving(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_FIELD_MOVE_SCRIPTS_H
