#ifndef POKEBW2_FIELD_SCRCMD_HOLLOW_RIVAL_H
#define POKEBW2_FIELD_SCRCMD_HOLLOW_RIVAL_H

// Overlay 12's scrcmd_hollow_rival.c (a descriptive name): the script commands of the rival's hollows

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_HollowRival_HollowRivalSetValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_HollowRival_HollowRivalGetValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_HollowRival_HollowRivalSetFlag(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_0216a748(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_HollowRival_HollowRivalGetCounter(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_0216a7cc(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_0216a7f4(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_HOLLOW_RIVAL_H
