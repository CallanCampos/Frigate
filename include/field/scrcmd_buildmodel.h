#ifndef POKEBW2_FIELD_SCRCMD_BUILDMODEL_H
#define POKEBW2_FIELD_SCRCMD_BUILDMODEL_H

// Overlay 36's scrcmd_buildmodel.c: the script commands of build models. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Buildmodel_BMPlayHOFMachineSeq(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_PokecenPlayHealingSequence(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_PokecenPCOpen(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_PokecenPCIdle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_PokecenPCClose(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_BMAnmPlayInv(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_BMAnmPlayLoop(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_BMSetVisible(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_BMChangeMdlID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_BMCreateHandleByGPos(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_BMReleaseHandle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_BMHndAudioVisualAnmPlay(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_BMHndAnmPlay(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_BMHndAnmPause(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Buildmodel_BMHndAnmWait(VM *vm, FieldScriptEnv *env);

// The handle that Scrcmd_Buildmodel_BMHndAnmWait waits on, among the common symbols at the end of the .bss
extern u16 g_ScrBMAnmWaitHandleID;

#endif // POKEBW2_FIELD_SCRCMD_BUILDMODEL_H
