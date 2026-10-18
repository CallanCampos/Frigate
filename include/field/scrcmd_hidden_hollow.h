#ifndef POKEBW2_FIELD_SCRCMD_HIDDEN_HOLLOW_H
#define POKEBW2_FIELD_SCRCMD_HIDDEN_HOLLOW_H

// Overlay 36's scrcmd_hidden_hollow.c (a descriptive name), not written yet: the script commands of the Hidden
// Grottoes. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

BOOL s0294_HiddenHollowGetParam(VM *vm, FieldScriptEnv *env);
BOOL s0295_HiddenHollowReset(VM *vm, FieldScriptEnv *env);
BOOL s0293_HiddenHollowCallWarpIn(VM *vm, FieldScriptEnv *env);
BOOL s0296_HiddenHollowCallWarpOut(VM *vm, FieldScriptEnv *env);
BOOL s02E5_HiddenHollowSet(VM *vm, FieldScriptEnv *env);
BOOL s02E6_HiddenHollowCreateEvents(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_HIDDEN_HOLLOW_H
