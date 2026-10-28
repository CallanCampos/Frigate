#ifndef POKEBW2_FIELD_SCRCMD_ENTREE_FOREST_H
#define POKEBW2_FIELD_SCRCMD_ENTREE_FOREST_H

// Overlay 12's scrcmd_entree_forest.c (a descriptive name): the Entree Forest's script commands. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_EntreeForest_EntreeForestStartBattle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_EntreeForest_EntreeForestSpawnAllPkm(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_EntreeForest_EntreeForestRemovePkm(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02164bcc(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_EntreeForest_WordSetLoadEntreeForestPkmName(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02164c68(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_EntreeForest_MapChangeEntreeForest(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_ENTREE_FOREST_H
