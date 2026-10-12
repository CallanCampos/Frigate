#include "types.h"
#include "save/hall_of_fame.h"
#include "constants/pokemon.h"
#include "gfl/std.h"
#include "pml/poke_party.h"

// The Hall of Fame's save data: the last 15 teams that entered it, each with the date, kept in extra save block 8,
// and a single team in block 0x40 of the main save. The ROM has no string for the file, so the name is a guess.

static void HallOfFameTeam_Set(HallOfFameTeam *team, PokeParty *party, const RTCDate *date, HeapID heapId);

u32 HallOfFameSave_GetSize(void) {
    return sizeof(HallOfFameSave);
}

u32 HallOfFameTeam_GetSize(void) {
    return sizeof(HallOfFameTeam);
}

void HallOfFameSave_Init(HallOfFameSave *save) {
    sys_memset(save, 0, sizeof(HallOfFameSave));
}

void HallOfFameTeam_Init(HallOfFameTeam *team) {
    sys_memset(team, 0, sizeof(HallOfFameTeam));
}

// Copies the party's Pokémon into the team, skipping the eggs
static void HallOfFameTeam_Set(HallOfFameTeam *team, PokeParty *party, const RTCDate *date, HeapID heapId) {
    int count;
    int i;
    int slot;
    BOOL wasEncrypted;
    PartyPkm *pkm;
    HallOfFameSavePokemon *pokemon;
    StrBuf *strbuf;

    count = PokeParty_GetPkmCount(party);
    strbuf = GFL_StrBufCreate(HALL_OF_FAME_NICKNAME_LENGTH, heapId);
    sys_memset(team->pokemon, 0, sizeof(team->pokemon));
    for (i = 0, slot = 0; i < count; i++) {
        pkm = PokeParty_GetPkm(party, i);
        wasEncrypted = PokeParty_DecryptPkm(pkm);
        if (!PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
            pokemon = &team->pokemon[slot];
            pokemon->species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
            pokemon->level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
            pokemon->form = (u8)PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
            pokemon->sex = (u8)PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
            pokemon->personality = PokeParty_GetParam(pkm, PKM_PARAM_PID, NULL);
            pokemon->trainerId = PokeParty_GetParam(pkm, PKM_PARAM_ID, NULL);
            pokemon->moves[0] = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1, NULL);
            pokemon->moves[1] = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + 1, NULL);
            pokemon->moves[2] = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + 2, NULL);
            pokemon->moves[3] = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + 3, NULL);
            if (strbuf != NULL) {
                PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME, strbuf);
                GFL_StrBufStoreString(strbuf, pokemon->nickname, HALL_OF_FAME_NICKNAME_LENGTH);
                PokeParty_GetParam(pkm, PKM_PARAM_OT_NAME, strbuf);
                GFL_StrBufStoreString(strbuf, pokemon->trainerName, HALL_OF_FAME_TRAINER_NAME_LENGTH);
            } else {
                pokemon->nickname[0] = GFL_StrBufGetTerminator();
                pokemon->trainerName[0] = GFL_StrBufGetTerminator();
            }
            slot++;
        }
        PokeParty_EncryptPkm(pkm, wasEncrypted);
    }
    team->year = date->year;
    team->month = date->month;
    team->day = date->day;
    if (strbuf != NULL) {
        GFL_StrBufFree(strbuf);
    }
}

void HallOfFameSave_AddTeam(HallOfFameSave *save, PokeParty *party, const RTCDate *date, HeapID heapId) {
    if (save->count < HALL_OF_FAME_MAX_COUNT) {
        HallOfFameTeam_Set(&save->teams[save->next], party, date, heapId);
        save->next++;
        if (save->next >= HALL_OF_FAME_TEAMS) {
            save->next = 0;
        }
        save->count++;
    }
}

s32 HallOfFameTeam_GetPokemonCount(HallOfFameTeam *team) {
    s32 count;

    for (count = 0; count < HALL_OF_FAME_TEAM_SIZE; count++) {
        if (team->pokemon[count].species == 0) {
            break;
        }
    }
    return count;
}

void HallOfFameTeam_GetPokemon(const HallOfFameTeam *team, s32 slot, HallOfFamePokemon *pokemon) {
    s32 i;
    const HallOfFameSavePokemon *saved = &team->pokemon[slot];

    pokemon->species = saved->species;
    pokemon->level = saved->level;
    pokemon->personality = saved->personality;
    pokemon->trainerId = saved->trainerId;
    pokemon->form = saved->form;
    pokemon->sex = saved->sex;
    GFL_StrBufLoadFixedString(pokemon->nickname, saved->nickname, HALL_OF_FAME_NICKNAME_LENGTH);
    GFL_StrBufLoadFixedString(pokemon->trainerName, saved->trainerName, HALL_OF_FAME_TRAINER_NAME_LENGTH);
    for (i = 0; i < 4; i++) {
        pokemon->moves[i] = saved->moves[i];
    }
}

void HallOfFameTeam_GetDate(const HallOfFameTeam *team, RTCDate *date) {
    date->year = team->year;
    date->month = team->month;
    date->day = team->day;
    date->week = RTC_WEEK_SUNDAY;
}

u32 HallOfFameSave_GetTeamCount(HallOfFameSave *save) {
    if (save->count < HALL_OF_FAME_TEAMS) {
        return save->count;
    }
    return HALL_OF_FAME_TEAMS;
}

u32 HallOfFameSave_GetEntryNumber(HallOfFameSave *save, u32 team) {
    return save->count - team;
}

s32 HallOfFameSave_GetPokemonCount(HallOfFameSave *save, s32 team) {
    s32 index = save->next - 1 - team;

    if (index < 0) {
        index += HALL_OF_FAME_TEAMS;
    }
    return HallOfFameTeam_GetPokemonCount(&save->teams[index]);
}

void HallOfFameSave_GetPokemon(HallOfFameSave *save, s32 team, s32 slot, HallOfFamePokemon *pokemon) {
    s32 index = save->next - 1 - team;

    if (index < 0) {
        index += HALL_OF_FAME_TEAMS;
    }
    HallOfFameTeam_GetPokemon(&save->teams[index], slot, pokemon);
}

void HallOfFameSave_GetDate(HallOfFameSave *save, s32 team, RTCDate *date) {
    s32 index = save->next - 1 - team;

    if (index < 0) {
        index += HALL_OF_FAME_TEAMS;
    }
    HallOfFameTeam_GetDate(&save->teams[index], date);
}
