#ifndef POKEBW2_FIELD_SCRCMD_MISC_H
#define POKEBW2_FIELD_SCRCMD_MISC_H

// Overlay 36's scrcmd_misc.c: Castelia's crowds, elevators, item collectors and Anville's daily train.
// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except the descriptive Anville train names.

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

// A floor of an elevator: its name, and where it goes. A script's table of them follows Scrcmd_Misc_ElevatorSetTablePtr
struct ElevatorFloor {
    u16 messageId;
    u16 zoneId;
    u16 x;
    u16 y;
    u16 z;
};

BOOL Scrcmd_Misc_CasteliaRushInit(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Misc_ElevatorRegisterBirthdayEntry(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Misc_ElevatorGetMysteryEntryCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Misc_ElevatorSetTablePtr(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Misc_ElevatorBuildListMenu(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Misc_ElevatorChangeMap(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Misc_ItemCollectorGetPrice(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Misc_ItemCollectorCheckGroup(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Misc_WordSetLoadItemCollectorPrice(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Misc_ItemCollectorSell(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Misc_AnvilleTrainGetModelID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Misc_AnvilleTrainGetDailyIndex(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_MISC_H
