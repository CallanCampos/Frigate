#include "types.h"
#include "save/traded_pokemon.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "system/str_tool.h"

// The Pokémon that the player gave away in the in-game trades, which their new Trainers battle with later. The ROM
// has no string for the file, so the name is descriptive (swan's SetTradedPokemonData). Block 0x41.

#define TRADED_POKEMON_LEVEL_MIN 65
#define TRADED_POKEMON_LEVEL_MAX 100
#define TRADED_POKEMON_LEVEL_GAIN 5

u32 TradedPokemonSave_GetSize(void) {
    return sizeof(TradedPokemonSave);
}

void TradedPokemonSave_Init(TradedPokemonSave *block) {
    sys_memset(block, 0, sizeof(TradedPokemonSave));
}

BOOL TradedPokemonSave_IsTraded(TradedPokemonSave *block, u32 index) {
    return block->pokemon[index].traded;
}

u8 TradedPokemonSave_GetBattleLevel(TradedPokemonSave *block, u32 index) {
    u8 level = block->pokemon[index].level;

    if (level < TRADED_POKEMON_LEVEL_MIN) {
        level = TRADED_POKEMON_LEVEL_MIN;
    } else {
        level += TRADED_POKEMON_LEVEL_GAIN;
    }
    if (level > TRADED_POKEMON_LEVEL_MAX) {
        level = TRADED_POKEMON_LEVEL_MAX;
    }
    return level;
}

BOOL TradedPokemonSave_HasAbility(TradedPokemonSave *block, u32 index) {
    if (block->pokemon[index].ability != TRADED_POKEMON_NO_ABILITY) {
        return TRUE;
    }
    return FALSE;
}

u8 TradedPokemonSave_GetAbility(TradedPokemonSave *block, u32 index) {
    return block->pokemon[index].ability;
}

u32 TradedPokemonSave_GetSpecies(TradedPokemonSave *block, u32 index, u32 version) {
    switch (index) {
    case 0:
        return SPECIES_COTTONEE;
    case 1:
        return SPECIES_PETILIL;
    case 2:
        return SPECIES_EXCADRILL;
    case 3:
        return SPECIES_HIPPOWDON;
    }
    return SPECIES_PATRAT;
}

u32 TradedPokemonSave_GetPersonality(TradedPokemonSave *block, u32 index) {
    return block->pokemon[index].personality;
}

u16 *TradedPokemonSave_GetNickname(TradedPokemonSave *block, u32 index) {
    return block->pokemon[index].nickname;
}

u32 TradedPokemonSave_GetId(TradedPokemonSave *block, u32 index) {
    return block->pokemon[index].id;
}

void SetTradedPokemonData(TradedPokemonSave *block, u32 index, PartyPkm *pkm) {
    u16 name[TRADED_POKEMON_NAME_LENGTH];

    block->pokemon[index].level = (u8)PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
    // Whether the Pokémon has its hidden ability
    if (PokeParty_GetParam(pkm, 0x71, NULL)) {
        block->pokemon[index].ability = PokeParty_GetParam(pkm, PKM_PARAM_ABILITY, NULL);
    } else {
        block->pokemon[index].ability = TRADED_POKEMON_NO_ABILITY;
    }
    block->pokemon[index].personality = PokeParty_GetParam(pkm, PKM_PARAM_PID, NULL);
    block->pokemon[index].id = PokeParty_GetParam(pkm, PKM_PARAM_ID, NULL);
    // Whether the Pokémon has a nickname
    if (PokeParty_GetParam(pkm, 0x75, NULL)) {
        PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME_RAW, block->pokemon[index].nickname);
    } else {
        GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL), name,
                              TRADED_POKEMON_NAME_LENGTH);
        wcharsncpy(name, block->pokemon[index].nickname, TRADED_POKEMON_NAME_LENGTH);
    }
    block->pokemon[index].traded = TRUE;
}
