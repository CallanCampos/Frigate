// Deerling and Sawsbuck take the form of the season. TransformVsPokePartyBySeason is swan's name; the file's name is
// descriptive
#include "types.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "pml/poke_party.h"
#include "save/pokedex.h"
#include "system/game_data.h"

void TransformVsPokePartyBySeason(GameData *gameData, PokeParty *party, u8 season) {
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);
    int count;
    u32 sawsbuckForm;
    u32 deerlingForm;
    BOOL sawsbuckRegistered = FALSE;
    BOOL deerlingRegistered = FALSE;
    int i;
    PartyPkm *pkm;
    u32 species;
    u32 form;

    switch (season) {
    case 0:
        sawsbuckForm = 0;
        deerlingForm = 0;
        break;
    case 1:
        sawsbuckForm = 1;
        deerlingForm = 1;
        break;
    case 2:
        sawsbuckForm = 2;
        deerlingForm = 2;
        break;
    case 3:
        sawsbuckForm = 3;
        deerlingForm = 3;
        break;
    default:
        sawsbuckForm = 0;
        deerlingForm = 0;
        break;
    }
    count = PokeParty_GetPkmCount(party);
    for (i = 0; i < count; i++) {
        pkm = PokeParty_GetPkm(party, i);
        species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        if (species == SPECIES_SAWSBUCK && form != sawsbuckForm) {
            PokeParty_ChangeForme(pkm, sawsbuckForm);
            if (!sawsbuckRegistered) {
                PokeDex_RegistPkm(pokedex, pkm);
                sawsbuckRegistered = TRUE;
            }
        } else if (species == SPECIES_DEERLING && form != deerlingForm) {
            PokeParty_ChangeForme(pkm, deerlingForm);
            if (!deerlingRegistered) {
                PokeDex_RegistPkm(pokedex, pkm);
                deerlingRegistered = TRUE;
            }
        }
    }
}
