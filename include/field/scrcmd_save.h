#ifndef POKEBW2_FIELD_SCRCMD_SAVE_H
#define POKEBW2_FIELD_SCRCMD_SAVE_H

// Overlay 36's scrcmd_save.c: the script command that saves the game. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Save_SaveDataWrite(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_SAVE_H
