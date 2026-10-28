#ifndef POKEBW2_FIELD_SCRCMD_PLACE_NAME_H
#define POKEBW2_FIELD_SCRCMD_PLACE_NAME_H

// Overlay 36's scrcmd_place_name.c: the script command that shows the place name banner. Name from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_PlaceName_CallPlaceNameDisp(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_PLACE_NAME_H
