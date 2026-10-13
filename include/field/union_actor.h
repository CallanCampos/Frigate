#ifndef POKEBW2_FIELD_UNION_ACTOR_H
#define POKEBW2_FIELD_UNION_ACTOR_H

#include "types.h"
#include "field/union_comm.h"
#include "struct_decls.h"

// union_actor.c, overlay 34 (the name is a guess, the ROM has no string of it): the actors of the Union Room's
// players heard, which union_oneself.c asks about by their id (the entry's index in the high byte, the member in the
// low one)

// Whose actor an id is: the one of an entry's parent, or of a member that joined it
typedef enum {
    UNION_ACTOR_PARENT,
    UNION_ACTOR_MEMBER,
} UnionActorKind;

// A step of an actor, for the player heard that it stands for
typedef BOOL (*UnionActorFunc)(UnionSystem *unisys, UnionActorSlot *slot, Field *field, u8 *seq);

void UnionOneself_ClearActors(UnionSystem *unisys);
UnionActorKind UnionOneself_IsMemberId(u16 id);
u16 UnionOneself_GetEntryIndex(u16 id);
u8 UnionOneself_GetActorGender(UnionSystem *unisys, u16 id);
BOOL UnionActor_IsId(u16 id);
BOOL UnionOneself_CanTalkTo(UnionSystem *unisys, u16 id);
void UnionOneself_UpdateActors(UnionSystem *unisys, GameData *gameData, Field *field);
void UnionOneself_UpdateMarks(UnionSystem *unisys, GameData *gameData, Field *field);
BOOL UnionActor_StepAppear(UnionSystem *unisys, UnionActorSlot *slot, Field *field, u8 *seq);

#endif // POKEBW2_FIELD_UNION_ACTOR_H
