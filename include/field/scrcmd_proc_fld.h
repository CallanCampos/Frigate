#ifndef POKEBW2_FIELD_SCRCMD_PROC_FLD_H
#define POKEBW2_FIELD_SCRCMD_PROC_FLD_H

// Overlay 36's scrcmd_proc_fld.c: the script commands that open other screens from the field. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except our descriptive ones of commands 0x231, 0x265 and 0x28F

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s014F_CallPC(VM *vm, FieldScriptEnv *env);
BOOL s0205_CallGreetingPhraseInput(VM *vm, FieldScriptEnv *env);
BOOL s0206_CallThanksPhraseInput(VM *vm, FieldScriptEnv *env);
BOOL s02CA_CallHappyPhraseInput(VM *vm, FieldScriptEnv *env);
BOOL s0156_CallGameClear(VM *vm, FieldScriptEnv *env);
BOOL s0197_CGearPowerOn(VM *vm, FieldScriptEnv *env);
BOOL s0152_CallGeonet(VM *vm, FieldScriptEnv *env);
BOOL s014D_CallRecordSystem(VM *vm, FieldScriptEnv *env);
BOOL s0231_CallGameManual(VM *vm, FieldScriptEnv *env);
BOOL s0155_CallXTransceiver(VM *vm, FieldScriptEnv *env);
BOOL s0265_CallXTransceiverSecret(VM *vm, FieldScriptEnv *env);
BOOL s0284_CallPlaceSelect(VM *vm, FieldScriptEnv *env);
BOOL s028F_CallRivalNameInput(VM *vm, FieldScriptEnv *env);
BOOL s0285_CallWordSetPokeNameInput(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_PROC_FLD_H
