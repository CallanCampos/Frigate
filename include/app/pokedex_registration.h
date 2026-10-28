#ifndef POKEBW2_APP_POKEDEX_REGISTRATION_H
#define POKEBW2_APP_POKEDEX_REGISTRATION_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "struct_decls.h"

// The Pokédex registration of a caught Pokémon, overlay 297 (pokedex_registration.c, named after its own file and its
// two strings): the Pokémon's Pokédex entry, with its cry, and then the question of whether to give it a nickname
// (pokedex_nickname.c). Queued by the returns of the wild battle and of the Entree Forest's capture
#define OVERLAY_POKEDEX_REGISTRATION OVERLAY_ID(297)

typedef struct {
    // 0 for a species caught for the first time, which shows its whole entry; otherwise only the cry is played
    u32 mode;
    PartyPkm *pkm;
    BOOL nationalDex;
    // The message about the box the Pokémon goes to, or NULL when it joins the party
    StrBuf *boxMessage;
    BoxSaveAccessor *boxes;
    u32 box;
    GameData *gameData;
    // TRUE when the player chose to give the Pokémon a nickname
    u32 nickname;
} PokedexRegistrationParam;

extern const GameProcFunctions POKEDEX_REGISTRATION_PROC_FUNCTIONS;

#endif // POKEBW2_APP_POKEDEX_REGISTRATION_H
