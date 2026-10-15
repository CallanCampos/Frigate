// Burmy's form after a battle, from the terrain it was fought on. The name is descriptive: the ROM has no string for
// this file, which the order of overlay 12's .rodata separates from event_battle.c (its 140-byte table comes before
// that file's smaller ones, which one file's size-sorted data can't do). Function name from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "field/burmy_form.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "pml/poke_party.h"
#include "save/pokedex.h"
#include "system/game_data.h"

// Burmy's form for each terrain
typedef struct {
    u16 terrain;
    u16 form;
} BurmyForm;

// clang-format off
static const BurmyForm data_ov012_0216dc90[35] = {
    {0x00, 1}, {0x01, 1}, {0x02, 2}, {0x03, 2}, {0x04, 1}, {0x05, 0}, {0x06, 0}, {0x07, 1}, {0x08, 0},
    {0x09, 0}, {0x0a, 1}, {0x0b, 1}, {0x0c, 0}, {0x0d, 0}, {0x0e, 2}, {0x0f, 2}, {0x10, 2}, {0x11, 2},
    {0x12, 2}, {0x13, 2}, {0x14, 2}, {0x15, 1}, {0x16, 0}, {0x17, 0}, {0x18, 2}, {0x19, 1}, {0x1a, 2},
    {0x1b, 2}, {0x1c, 2}, {0x1d, 2}, {0x1e, 2}, {0x1f, 2}, {0x20, 2}, {0x21, 2}, {0x22, 2},
};

// clang-format on

void burmyTransform(GameData *gameData, PartyPkm *pkm, u32 terrain) {
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);
    u32 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    int i;

    for (i = 0; i < 35; i++) {
        if (terrain == data_ov012_0216dc90[i].terrain) {
            u32 form = data_ov012_0216dc90[i].form;

            if (species == SPECIES_BURMY) {
                PokeParty_ChangeForme(pkm, form);
                PokeDex_RegistPkm(pokedex, pkm);
            }
            return;
        }
    }
}
