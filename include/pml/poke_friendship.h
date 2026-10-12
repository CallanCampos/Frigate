#ifndef POKEBW2_PML_POKE_FRIENDSHIP_H
#define POKEBW2_PML_POKE_FRIENDSHIP_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The changes of a Pokémon's friendship (poke_friendship.c, a guessed name: the ROM has no string for it).
// FriendshipManagerCalc, FriendshipManagerCalcItem and FriendshipManagerCalcParty are declared in pml/poke_party.h. The names are swan's
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

// The reasons of FriendshipManagerCalc, rows of its table. Only those the code shows are named
#define FRIENDSHIP_REASON_LEVEL_UP_PARTY 2
#define FRIENDSHIP_REASON_WALK 3
#define FRIENDSHIP_REASON_COUNT 8

// The friendship of walking, which a Pokémon gets half the times
void happinessAdd(PartyPkm *pkm, u16 zoneId, HeapID heapId);

#endif // POKEBW2_PML_POKE_FRIENDSHIP_H
