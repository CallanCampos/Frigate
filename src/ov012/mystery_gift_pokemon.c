// Makes the Pokémon of a Pokémon gift. The file's name is descriptive
#include "types.h"
#include "app/mystery/mystery_gift_data.h"
#include "field/mystery_gift_pokemon.h"

// The body is the maker that overlay 197 also inlines: its early returns all leave through one exit, which the
// original only gets from inlining
PartyPkm *func_ov012_02153160(const MysteryGift *gift, HeapID heapId, GameData *gameData) {
    return Mystery_CreateGiftPokemon(gift, heapId, gameData);
}
