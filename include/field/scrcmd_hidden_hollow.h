#ifndef POKEBW2_FIELD_SCRCMD_HIDDEN_HOLLOW_H
#define POKEBW2_FIELD_SCRCMD_HIDDEN_HOLLOW_H

// Overlay 36's scrcmd_hidden_hollow.c (a descriptive name): the script commands of the Hidden Grottoes. Names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

BOOL Scrcmd_HiddenHollow_HiddenHollowGetParam(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_HiddenHollow_HiddenHollowReset(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_HiddenHollow_HiddenHollowCallWarpIn(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_HiddenHollow_HiddenHollowCallWarpOut(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_HiddenHollow_HiddenHollowSet(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_HiddenHollow_HiddenHollowCreateEvents(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_HIDDEN_HOLLOW_H
