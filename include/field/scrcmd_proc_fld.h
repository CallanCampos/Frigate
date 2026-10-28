#ifndef POKEBW2_FIELD_SCRCMD_PROC_FLD_H
#define POKEBW2_FIELD_SCRCMD_PROC_FLD_H

// Overlay 36's scrcmd_proc_fld.c: the script commands that open other screens from the field. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except our descriptive ones of commands 0x231, 0x265 and 0x28F

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_ProcFld_CallPC(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallGreetingPhraseInput(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallThanksPhraseInput(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallHappyPhraseInput(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallGameClear(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CGearPowerOn(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallGeonet(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallRecordSystem(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallGameManual(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallXTransceiver(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallXTransceiverSecret(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallPlaceSelect(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallRivalNameInput(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_ProcFld_CallWordSetPokeNameInput(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_PROC_FLD_H
