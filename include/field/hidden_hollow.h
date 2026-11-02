#ifndef POKEBW2_FIELD_HIDDEN_HOLLOW_H
#define POKEBW2_FIELD_HIDDEN_HOLLOW_H

// Overlay 12's hidden_hollow.c (a descriptive name): the Hidden Grottoes. GetHiddenHollowEntranceParam is swan's name

#include "types.h"
#include "struct_decls.h"

// GetHiddenHollowEntranceParam's params: the zone of a Hidden Grotto's entrance, and the position in it
enum {
    HIDDEN_HOLLOW_ENTRANCE_ZONE,
    HIDDEN_HOLLOW_ENTRANCE_X,
    HIDDEN_HOLLOW_ENTRANCE_Y,
    HIDDEN_HOLLOW_ENTRANCE_Z,
};

// A parameter of the entrance of a Hidden Grotto
u32 GetHiddenHollowEntranceParam(u8 hollow, u32 param);
BOOL Scrcmd_HiddenHollow_HiddenHollowSetPeriod(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_HIDDEN_HOLLOW_H
