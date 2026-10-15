#ifndef POKEBW2_FIELD_UNION_EFF_H
#define POKEBW2_FIELD_UNION_EFF_H

#include "types.h"
#include "field/colosseum_tool.h"
#include "field/union_comm.h"
#include "field/field_actor.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// union_eff.c, overlay 34, which the ROM names: the Union Room's effects on the field, the focus on a player and
// the marks over them (the field keeps its work). The colosseum's checks of who is ready are colosseum_tool.c's,
// whose header this one includes

// Whether the player stands on the Union Room's spot (the one entry of its table)
BOOL UnionEff_IsPlayerOnSpot(Field *field);
// A step of the Union Room's actors told in kinds: 1 for 4 to 0x17, 4 for 3, 2 for 0x18, 5 for 0x19, 3 for 0x1a
int UnionEff_GetKindStep(int kind);
// Whether the beacon's player is one of the wifi list's friends
BOOL UnionEff_IsFriendBeacon(WifiList *wifiList, const UnionBeacon *beacon);
void *UnionEff_Create(HeapID heapId);
void UnionEff_Free(void *work);
void UnionEff_Update(void *work);
void UnionEff_Draw(void *work);
// Focus on an actor
void UnionEff_AddFocus(void *work, FieldActor *actor);
// Put a mark over an actor, the asonda one when asonda is TRUE and else the tomo one
void UnionEff_AddMark(void *work, FieldActor *actor, BOOL asonda);
// Take the mark off an actor
void UnionEff_RemoveMark(void *work, FieldActor *actor);

#endif // POKEBW2_FIELD_UNION_EFF_H
