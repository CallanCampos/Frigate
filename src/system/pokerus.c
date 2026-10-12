#include "types.h"
#include "constants/pokemon.h"
#include "gfl/random.h"
#include "pml/poke_party.h"

// Pokérus: catching it, its days running out and its spread through the party. The file's name is a guess: the ROM has
// no string for it. It is a file of its own because nothing ties it to poke_personal.c before it, whose archives it
// doesn't touch, or to shinka_check.c after it: it only reads and writes the Pokérus field of the party's Pokémon.
// The functions' names are swan's

// The Pokérus field: the days left in the low nibble, the strain in the high one
#define POKERUS_DAYS_MASK 0xf
#define POKERUS_STRAIN_MASK 0xf0

static u32 pokerusBitmaskForPartyPKM(PokeParty *party);

void pokerusHandler(PokeParty *party) {
    u32 count = PokeParty_GetPkmCount(party);
    u16 chance = GFL_RandomMTRange(0x10000);
    u32 infected = pokerusBitmaskForPartyPKM(party);

    if (chance == 0x4000 || chance == 0x8000 || chance == 0xc000) {
        PartyPkm *pkm;
        u32 slot;
        u8 pokerus;

        do {
            slot = GFL_RandomMTRange(count);
            pkm = PokeParty_GetPkm(party, slot);
        } while (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == 0
                 || PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) != FALSE);
        if (!(infected & (1 << slot))) {
            do {
                pokerus = GFL_RandomMTRange(0x100);
            } while ((pokerus & 7) == 0);
            if (pokerus & POKERUS_STRAIN_MASK) {
                pokerus &= 7;
            }
            pokerus |= pokerus << 4;
            pokerus &= 0xf3;
            pokerus++;
            PokeParty_SetParam(pkm, PKM_PARAM_POKERUS, pokerus);
        }
    }
}

void pokerusDecay(PokeParty *party, s32 days) {
    PartyPkm *pkm;
    int i;
    int count = PokeParty_GetPkmCount(party);

    for (i = 0; i < count; i++) {
        pkm = PokeParty_GetPkm(party, i);
        if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) != 0) {
            u8 pokerus = PokeParty_GetParam(pkm, PKM_PARAM_POKERUS, NULL);

            if (pokerus & POKERUS_DAYS_MASK) {
                if ((pokerus & POKERUS_DAYS_MASK) < days || days > 4) {
                    pokerus &= POKERUS_STRAIN_MASK;
                } else {
                    pokerus -= days;
                }
                if (pokerus == 0) {
                    pokerus = 0x10;
                }
                PokeParty_SetParam(pkm, PKM_PARAM_POKERUS, pokerus);
            }
        }
    }
}

void pokerusSpread(PokeParty *party) {
    int count = PokeParty_GetPkmCount(party);
    int i;

    if (GFL_RandomMTRange(3) == 0) {
        for (i = 0; i < count; i++) {
            PartyPkm *pkm = PokeParty_GetPkm(party, i);

            if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) != 0) {
                u8 pokerus = PokeParty_GetParam(pkm, PKM_PARAM_POKERUS, NULL);

                if (pokerus & POKERUS_DAYS_MASK) {
                    if (i != 0) {
                        pkm = PokeParty_GetPkm(party, i - 1);
                        if (!(PokeParty_GetParam(pkm, PKM_PARAM_POKERUS, NULL) & POKERUS_STRAIN_MASK)) {
                            PokeParty_SetParam(pkm, PKM_PARAM_POKERUS, pokerus);
                        }
                    }
                    if (i < count - 1) {
                        pkm = PokeParty_GetPkm(party, i + 1);
                        if (!(PokeParty_GetParam(pkm, PKM_PARAM_POKERUS, NULL) & POKERUS_STRAIN_MASK)) {
                            PokeParty_SetParam(pkm, PKM_PARAM_POKERUS, pokerus);
                            i++;
                        }
                    }
                }
            }
        }
    }
}

// A bit for each Pokémon of the party that has, or has had, Pokérus
static u32 pokerusBitmaskForPartyPKM(PokeParty *party) {
    int i;
    u32 mask = 0;
    int count = PokeParty_GetPkmCount(party);

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_POKERUS, NULL) != 0) {
            mask |= (u8)(1 << i);
        }
    }
    return mask;
}

BOOL pokerusDuration(PartyPkm *pkm) {
    return doesPokerusHaveDuration(func_0201d620(pkm));
}

BOOL doesPokerusHaveDuration(BoxPkm *pkm) {
    BOOL result = FALSE;

    if (PML_PkmGetParam(pkm, PKM_PARAM_POKERUS, NULL) & POKERUS_DAYS_MASK) {
        result = TRUE;
    }
    return result;
}

BOOL pokeHasPkrs(PartyPkm *pkm) {
    return doesPokeHavePokerus(func_0201d620(pkm));
}

// Whether the Pokémon has had Pokérus and is over it
BOOL doesPokeHavePokerus(BoxPkm *pkm) {
    BOOL result = FALSE;
    u8 pokerus = PML_PkmGetParam(pkm, PKM_PARAM_POKERUS, NULL);

    if (!(pokerus & POKERUS_DAYS_MASK) && (pokerus & POKERUS_STRAIN_MASK)) {
        result = TRUE;
    }
    return result;
}
