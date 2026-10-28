#ifndef POKEBW2_FIELD_STADIUM_SCRIPT_H
#define POKEBW2_FIELD_STADIUM_SCRIPT_H

#include "types.h"
#include "struct_decls.h"

struct StadiumTrainerEntry {
    u16 unk00;
    u16 unk02;
    u16 objCode;
    u16 trainerId;
};

BOOL Scrcmd_Stadium_StadiumLoadTrainerTable(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Stadium_StadiumFreeTrainerTable(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Stadium_StadiumSetupActorSingle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Stadium_StadiumSetupActorsDouble(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Stadium_StadiumSetupActorsTriple(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Stadium_StadiumResetTrainerFlags(VM *vm, FieldScriptEnv *env);
u32 FindStadiumTrainerIndex(StadiumTrainerEntry *trainers, u16 a, u16 b);

// Picks six different numbers from 0x50 to 0x83 into the script variables
BOOL Scrcmd_Stadium_GetRandomDistinctSix(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_STADIUM_SCRIPT_H
