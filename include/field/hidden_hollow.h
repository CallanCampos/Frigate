#ifndef POKEBW2_FIELD_HIDDEN_HOLLOW_H
#define POKEBW2_FIELD_HIDDEN_HOLLOW_H

// Overlay 12's hidden_hollow.c (a descriptive name): the Hidden Grottoes. GetHiddenHollowEntranceParam is swan's name

#include "types.h"
#include "struct_decls.h"

// A parameter of the entrance of a Hidden Grotto: 0 is its zone
u32 GetHiddenHollowEntranceParam(u8 hollow, u32 param);
BOOL Scrcmd_HiddenHollow_HiddenHollowSetPeriod(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_HIDDEN_HOLLOW_H
