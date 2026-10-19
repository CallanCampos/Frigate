// Shaymin's Sky Forme turns back into its Land Forme at night. The name is descriptive
#include "types.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "field/shaymin_form.h"
#include "pml/poke_party.h"
#include "save/pokedex.h"
#include "system/game_data.h"
#include "system/rtc.h"

#define SHAYMIN_FORM_SKY 1

static void RevertPartyShaymin(GameData *gameData, PokeParty *party) {
    int count = PokeParty_GetPkmCount(party);
    int i;
    PartyPkm *pkm;
    u32 species;
    u32 form;

    for (i = 0; i < count; i++) {
        pkm = PokeParty_GetPkm(party, i);
        species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        if (species == SPECIES_SHAYMIN && form == SHAYMIN_FORM_SKY) {
            PokeParty_ChangeForme(pkm, 0);
        }
    }
}

BOOL ShayminForm_CheckNightPassed(GameData *gameData, PokeParty *party, s32 minutes, const RTCTime *time, u32 season) {
    int nightStart = GetLightChangeHoursForSeasons(season, 3);
    int morning = GetLightChangeHoursForSeasons(season, 0);
    int hour;
    int elapsed;

    if (time->hour >= nightStart || time->hour < morning) {
        hour = time->hour;
        if (hour < morning) {
            hour += 24;
        }
        hour -= nightStart;
        elapsed = time->minute + hour * 60;
        if (elapsed < minutes + 1) {
            RevertPartyShaymin(gameData, party);
            return TRUE;
        }
        return FALSE;
    }
    elapsed = time->minute + (time->hour - morning) * 60;
    if (elapsed < minutes) {
        RevertPartyShaymin(gameData, party);
        return TRUE;
    }
    return FALSE;
}

BOOL ShayminForm_CheckNight(GameData *gameData, PokeParty *party, const RTCTime *time, u8 season) {
    int nightStart = GetLightChangeHoursForSeasons(season, 3);
    int morning = GetLightChangeHoursForSeasons(season, 0);
    BOOL night = FALSE;

    if (time->hour >= nightStart || time->hour < morning) {
        ShayminForm_RevertPartyAndRegister(gameData, party);
        night = TRUE;
    }
    return night;
}

void ShayminForm_RevertPartyAndRegister(GameData *gameData, PokeParty *party) {
    int count;
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);
    BOOL registered = FALSE;
    int i;
    PartyPkm *pkm;
    u32 species;
    u32 form;

    count = PokeParty_GetPkmCount(party);
    for (i = 0; i < count; i++) {
        pkm = PokeParty_GetPkm(party, i);
        species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        if (species == SPECIES_SHAYMIN && form == SHAYMIN_FORM_SKY) {
            PokeParty_ChangeForme(pkm, 0);
            if (!registered) {
                PokeDex_RegistPkm(pokedex, pkm);
                registered = TRUE;
            }
        }
    }
}
