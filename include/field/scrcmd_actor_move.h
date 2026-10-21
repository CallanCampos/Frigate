#ifndef POKEBW2_FIELD_SCRCMD_ACTOR_MOVE_H
#define POKEBW2_FIELD_SCRCMD_ACTOR_MOVE_H

// Overlay 12's scrcmd_actor_move.c (a descriptive name): the script commands that raise or lower an actor over frames
// and hold it in place

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s0212_ActorMoveToHeight(VM *vm, FieldScriptEnv *env);
BOOL s0213_ActorHoldPos(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_ACTOR_MOVE_H
