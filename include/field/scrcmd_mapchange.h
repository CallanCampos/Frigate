#ifndef POKEBW2_FIELD_SCRCMD_MAPCHANGE_H
#define POKEBW2_FIELD_SCRCMD_MAPCHANGE_H

// Overlay 36's scrcmd_mapchange.c: the script commands that change the map. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Mapchange_MapChangeWarp(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Mapchange_MapChangeFake(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Mapchange_MapChangeQuicksand(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Mapchange_MapChangeWarpPad(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Mapchange_MapChangeUnionRoom(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Mapchange_MapChangeCore(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Mapchange_MapChangeWarpRail(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Mapchange_MapChangeRail(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Mapchange_MapChangeFlyWarp(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_MAPCHANGE_H
