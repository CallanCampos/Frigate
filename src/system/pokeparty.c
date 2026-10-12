#include "types.h"
#include "constants/pokemon.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/save_control.h"

// The party: a save block of six Pokémon with their count. The file's name is the ROM's string; the functions' names
// are swan's

typedef BOOL (*PokePartyMemberFunc)(PartyPkm *pkm);

static BOOL PML_PkmIsBattleReady(PartyPkm *pkm);
static BOOL PML_PkmIsEgg(PartyPkm *pkm);
static int PokeParty_FindMemberCore(PokeParty *party, PokePartyMemberFunc func);

u32 PokeParty_GetSaveDataSize(void) {
    return sizeof(PokeParty);
}

PokeParty *PokeParty_Create(HeapID heapId) {
    PokeParty *party = GFL_HeapAllocate(heapId, sizeof(PokeParty), FALSE, "pokeparty.c", 65);

    PokeParty_Init(party);
    return party;
}

void PokeParty_Init(PokeParty *party) {
    PokeParty_InitCore(party, PARTY_SIZE);
}

void PokeParty_InitCore(PokeParty *party, u32 capacity) {
    int i;

    sys_memset32_fast(0, party, sizeof(PokeParty));
    party->capacity = capacity;
    party->count = 0;
    party->slotExists = 0;
    for (i = 0; i < PARTY_SIZE; i++) {
        PokeParty_ClearPkm(&party->pkms[i]);
    }
}

BOOL PokeParty_AddPkm(PokeParty *party, PartyPkm *pkm) {
    if (party->count >= party->capacity) {
        return FALSE;
    }
    party->pkms[party->count] = *pkm;
    PokeParty_SetSlotExists(party, party->count, TRUE);
    party->count++;
    return TRUE;
}

BOOL PokeParty_RemovePkm(PokeParty *party, u32 index) {
    int i;

    for (i = index; i < party->count - 1; i++) {
        party->pkms[i] = party->pkms[i + 1];
    }
    PokeParty_ClearPkm(&party->pkms[i]);
    party->count--;
    return TRUE;
}

int PokeParty_GetCapacity(PokeParty *party) {
    return party->capacity;
}

int PokeParty_GetPkmCount(PokeParty *party) {
    return party->count;
}

int howManyPartyPokesAreNotEggs(PokeParty *party) {
    int i;
    int result = 0;
    int count = PokeParty_GetPkmCount(party);

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) != TRUE) {
            result++;
        }
    }
    return result;
}

int howManyPokesAreAbleToFight(PokeParty *party) {
    int i;
    int result = 0;
    int count = PokeParty_GetPkmCount(party);

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        pkm = PokeParty_GetPkm(party, i);
        if (PML_PkmIsBattleReady(pkm)) {
            result++;
        }
    }
    return result;
}

// The eggs that are not bad eggs
int countAllEggsInParty(PokeParty *party) {
    int i;
    int result = 0;
    int count = PokeParty_GetPkmCount(party);

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);
        u32 isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
        u32 badEgg = PokeParty_GetParam(pkm, PKM_PARAM_BAD_EGG, NULL);

        if (isEgg == TRUE && badEgg == FALSE) {
            result++;
        }
    }
    return result;
}

// The bad eggs
int countSanityEggsInParty(PokeParty *party) {
    int i;
    int result = 0;
    int count = PokeParty_GetPkmCount(party);

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_BAD_EGG, NULL) == TRUE) {
            result++;
        }
    }
    return result;
}

// The first Pokémon that can battle, or the party's count if none can
u32 PokeParty_GetFirstBattleReady(PokeParty *party) {
    return PokeParty_FindMemberCore(party, PML_PkmIsBattleReady);
}

// The first Pokémon that is not an egg, or the party's count if all are
u32 isEggInParty(PokeParty *party) {
    return PokeParty_FindMemberCore(party, PML_PkmIsEgg);
}

PartyPkm *PokeParty_GetPkm(PokeParty *party, u32 index) {
    return &party->pkms[index];
}

// Replaces the Pokémon of a slot. The count changes by the slot's validity less the new Pokémon's, which is nothing
// when one Pokémon replaces another
void copyPkmIntoPartyBlk(PokeParty *party, u32 index, PartyPkm *pkm) {
    int change = PokeParty_GetParam(&party->pkms[index], PKM_PARAM_SPECIES_VALID, NULL)
                 - PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL);

    party->pkms[index] = *pkm;
    party->count += change;
}

BOOL PokeParty_SwapPkms(PokeParty *party, u32 indexA, u32 indexB, HeapID heapId) {
    PartyPkm *temp = GFL_HeapAllocate(heapId, sizeof(PartyPkm), FALSE, "pokeparty.c", 382);
    BOOL existsA;
    BOOL existsB;

    *temp = party->pkms[indexA];
    party->pkms[indexA] = party->pkms[indexB];
    party->pkms[indexB] = *temp;
    GFL_HeapFree(temp);
    existsA = PokeParty_GetSlotExists(party, indexA);
    existsB = PokeParty_GetSlotExists(party, indexB);
    PokeParty_SetSlotExists(party, indexA, existsB);
    PokeParty_SetSlotExists(party, indexB, existsA);
    return FALSE;
}

// Reorders the party: order[i] is the slot of the Pokémon that goes to slot i. Does nothing to a full party
void func_0201fff8(PokeParty *party, u32 *order, HeapID heapId) {
    PartyPkm *pkms[PARTY_SIZE];
    BOOL exists[PARTY_SIZE];
    u32 i;

    if (party->count < PARTY_SIZE) {
        for (i = 0; i < party->count; i++) {
            pkms[i] = GFL_HeapAllocate(heapId, sizeof(PartyPkm), FALSE, "pokeparty.c", 420);
            *pkms[i] = party->pkms[i];
            exists[i] = PokeParty_GetSlotExists(party, i);
        }
        for (i = 0; i < party->count; i++) {
            copyPkmIntoPartyBlk(party, i, pkms[order[i]]);
            PokeParty_SetSlotExists(party, i, exists[order[i]]);
            sys_memset(pkms[order[i]], 0xff, sizeof(PartyPkm));
            GFL_HeapFree(pkms[order[i]]);
        }
    }
}

void PokeParty_Copy(const PokeParty *src, PokeParty *dest) {
    *dest = *src;
}

BOOL CheckSpeciesInParty(PokeParty *party, u32 species) {
    int i;

    for (i = 0; i < party->count; i++) {
        if (species == PokeParty_GetParam(&party->pkms[i], PKM_PARAM_SPECIES, NULL)) {
            break;
        }
    }
    if (i != party->count) {
        return TRUE;
    }
    return FALSE;
}

int countActivePkms(PokeParty *party) {
    int i;
    int count = PokeParty_GetPkmCount(party);
    int result = 0;

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL) != 0 && PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == FALSE) {
            result++;
        }
    }
    return result;
}

BOOL PokeParty_GetSlotExists(PokeParty *party, u32 slot) {
    return (party->slotExists & (1 << slot)) != 0;
}

void PokeParty_SetSlotExists(PokeParty *party, u32 slot, BOOL exists) {
    if (exists == TRUE) {
        party->slotExists = party->slotExists | (1 << slot);
    } else {
        party->slotExists &= 0xff - (1 << slot);
    }
}

PokeParty *SaveControl_GetPokePartySave(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, 26);
}

static BOOL PML_PkmIsBattleReady(PartyPkm *pkm) {
    u32 isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    u32 hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);

    if (isEgg != TRUE && hp != 0) {
        return TRUE;
    }
    return FALSE;
}

// Whether the Pokémon is not an egg
static BOOL PML_PkmIsEgg(PartyPkm *pkm) {
    BOOL result = FALSE;

    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) != TRUE) {
        result = TRUE;
    }
    return result;
}

// The first Pokémon for which func returns TRUE, or the party's count
static int PokeParty_FindMemberCore(PokeParty *party, PokePartyMemberFunc func) {
    int i;
    int count = PokeParty_GetPkmCount(party);

    for (i = 0; i < count; i++) {
        if (func(PokeParty_GetPkm(party, i))) {
            return i;
        }
    }
    return count;
}
