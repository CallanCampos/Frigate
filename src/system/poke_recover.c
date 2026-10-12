#include "types.h"
#include "constants/pokemon.h"
#include "pml/poke_party.h"
#include "pml/waza.h"

// Restoring a Pokémon's PP, and healing a Pokémon and a party. The file's name is a guess, from what it does: the
// ROM has no string for it. The functions' names are swan's (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

// The amount of PokeParty_AddPP that restores all of a move's PP
#define PP_RESTORE_ALL 127

u8 PokeParty_CheckPPNeedsReplenish(PartyPkm *pkm, u32 slot) {
    u16 move;
    u8 pp;
    u8 ppUp;
    BOOL result = FALSE;

    move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + slot, NULL);
    if (move == 0) {
        return FALSE;
    }
    pp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP + slot, NULL);
    ppUp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP_UP + slot, NULL);
    if (pp < PML_MoveGetMaxPP(move, ppUp)) {
        result = TRUE;
    }
    return result;
}

BOOL PokeParty_AddPP(PartyPkm *pkm, u32 slot, u32 amount) {
    u16 move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + slot, NULL);
    u8 pp;
    u8 maxPP;

    if (move == 0) {
        return FALSE;
    }
    pp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP + slot, NULL);
    maxPP = PML_MoveGetMaxPP(move, PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP_UP + slot, NULL));
    if (pp < maxPP) {
        if (amount == PP_RESTORE_ALL) {
            pp = maxPP;
        } else {
            pp += (u8)amount;
            if (pp > maxPP) {
                pp = maxPP;
            }
        }
        PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP + slot, pp);
        return TRUE;
    }
    return FALSE;
}

void PokeParty_Recover(PartyPkm *pkm) {
    int i;

    PokeParty_SetParam(pkm, PKM_PARAM_HP, PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL));
    PokeParty_SetParam(pkm, PKM_PARAM_STATUS, 0);
    for (i = 0; i < 4; i++) {
        if (PokeParty_CheckPPNeedsReplenish(pkm, i) == TRUE) {
            PokeParty_AddPP(pkm, i, PP_RESTORE_ALL);
        }
    }
}

void PokeParty_RecoverAll(PokeParty *party) {
    int count = PokeParty_GetPkmCount(party);
    int i;
    PartyPkm *pkm;

    for (i = 0; i < count; i++) {
        pkm = PokeParty_GetPkm(party, i);
        if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            PokeParty_Recover(pkm);
        }
    }
}
