#ifndef POKEBW2_FIELD_MYSTERY_GIFT_POKEMON_H
#define POKEBW2_FIELD_MYSTERY_GIFT_POKEMON_H

// Overlay 12's mystery_gift_pokemon.c (a descriptive name): makes the Pokémon of a Pokémon gift

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// NULL when the gift's Pokémon is not valid
PartyPkm *func_ov012_02153160(const MysteryGift *gift, HeapID heapId, GameData *gameData);

#endif // POKEBW2_FIELD_MYSTERY_GIFT_POKEMON_H
