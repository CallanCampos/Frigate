#ifndef POKEBW2_SAVE_TRADED_POKEMON_H
#define POKEBW2_SAVE_TRADED_POKEMON_H

// The Pokémon traded in game, in save block 0x41 (GetTradedPokemonBlock), 0x20 bytes for each, of ARM9 main
// (traded_pokemon_savedata.c, a descriptive name)

#include "types.h"
#include "struct_decls.h"

#define TRADED_POKEMON_COUNT 4
#define TRADED_POKEMON_NAME_LENGTH 11
#define TRADED_POKEMON_NO_ABILITY 0xff

// A Pokémon the player gave away, which its new Trainer battles with later
typedef struct TradedPokemon {
    u32 personality;
    u16 nickname[TRADED_POKEMON_NAME_LENGTH];
    u8 traded : 1;
    u8 level : 7;
    // TRADED_POKEMON_NO_ABILITY when the Pokémon doesn't keep its own
    u8 ability;
    u32 id;
} TradedPokemon; // size: 0x20

typedef struct TradedPokemonSave {
    TradedPokemon pokemon[TRADED_POKEMON_COUNT];
} TradedPokemonSave; // size: 0x80

// The block's size and its initial state
u32 TradedPokemonSave_GetSize(void);
void TradedPokemonSave_Init(TradedPokemonSave *block);
// Whether the trade of the index is done
BOOL TradedPokemonSave_IsTraded(TradedPokemonSave *block, u32 index);
// The level, from 65 to 100
u8 TradedPokemonSave_GetBattleLevel(TradedPokemonSave *block, u32 index);
// Whether it has its own ability, and the ability
BOOL TradedPokemonSave_HasAbility(TradedPokemonSave *block, u32 index);
u8 TradedPokemonSave_GetAbility(TradedPokemonSave *block, u32 index);
// The species of the index
u32 TradedPokemonSave_GetSpecies(TradedPokemonSave *block, u32 index, u32 version);
u32 TradedPokemonSave_GetPersonality(TradedPokemonSave *block, u32 index);
// Its nickname, as characters
u16 *TradedPokemonSave_GetNickname(TradedPokemonSave *block, u32 index);
u32 TradedPokemonSave_GetId(TradedPokemonSave *block, u32 index);
// Records the Pokémon given away in the trade of the index
void SetTradedPokemonData(TradedPokemonSave *block, u32 index, PartyPkm *pkm);

#endif // POKEBW2_SAVE_TRADED_POKEMON_H
