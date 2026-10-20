#ifndef POKEBW2_FIELD_FIELD_COMM_ACTOR_H
#define POKEBW2_FIELD_FIELD_COMM_ACTOR_H

// Overlay 36's actors of the other players in a field communication. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

FldCommActSys *FldCommActSys_Create(u8 count, MMSys *actorSystem, HeapID heapId, BOOL a3);
void FldCommActSys_Free(FldCommActSys *sys);
void FldCommActSys_CreateActor(FldCommActSys *sys, u32 netId, u16 modelId, const u16 *dir, const VecFx32 *pos,
                               const u32 *hidden);
void FldCommActSys_DeleteActor(FldCommActSys *sys, u32 netId);
// Whether an actor is at a grid position, with its network ID; *index is the entry to look from, and moves past it
BOOL FldCommActSys_FindActor(FldCommActSys *sys, s16 x, s16 y, s16 z, u32 *netId, u32 *index);
// The init, delete and update functions of the move code of these actors, which follow their player's direction,
// position and visibility
void func_ov036_0218b044(FieldActor *actor);
void func_ov036_0218b050(FieldActor *actor);
void func_ov036_0218b060(FieldActor *actor);

#endif // POKEBW2_FIELD_FIELD_COMM_ACTOR_H
