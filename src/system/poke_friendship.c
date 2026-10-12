#include "types.h"
#include "pml/poke_friendship.h"
#include "constants/hold_effects.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "field/zone.h"
#include "gfl/random.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "save/high_link.h"

// The changes of a Pokémon's friendship. The file's name is a guess, from what it does: the ROM has no string for
// it. The functions' names are swan's (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

// The friendship from which the second and the third change of a reason apply
#define FRIENDSHIP_TIER_2 100
#define FRIENDSHIP_TIER_3 200
#define FRIENDSHIP_TIER_COUNT 3
#define FRIENDSHIP_MAX 255

static void happinessIncrement(PartyPkm *pkm, const s8 *deltas, u16 zoneId, HeapID heapId);

// The change of each reason, by the tier of the Pokémon's friendship
static const s8 sFriendshipDeltas[FRIENDSHIP_REASON_COUNT][FRIENDSHIP_TIER_COUNT] = {
    { 5, 4, 3 },
    { 0, 0, 0 },
    { 5, 4, 3 },
    { 2, 2, 1 },
    { -1, -1, -1 },
    { -5, -5, -10 },
    { 4, 3, 2 },
    { 1, 1, 0 },
};

void FriendshipManagerCalc(PartyPkm *pkm, u32 reason, u16 zoneId, u16 heapId) {
    happinessIncrement(pkm, sFriendshipDeltas[reason], zoneId, heapId);
}

// The change of an item's use, which the item's data holds
void FriendshipManagerCalcItem(PartyPkm *pkm, u16 item, u16 zoneId, HeapID heapId) {
    s8 deltas[FRIENDSHIP_TIER_COUNT];
    int i;

    for (i = 0; i < FRIENDSHIP_TIER_COUNT; i++) {
        deltas[i] = GetItemParam(item, (u16)(ITEM_PARAM_FRIENDSHIP_1 + i), heapId);
    }
    happinessIncrement(pkm, deltas, zoneId, heapId);
}

void happinessAdd(PartyPkm *pkm, u16 zoneId, HeapID heapId) {
    if (GFL_RandomMTRange(2) & 1) {
        return;
    }
    happinessIncrement(pkm, sFriendshipDeltas[FRIENDSHIP_REASON_WALK], zoneId, heapId);
}

void FriendshipManagerCalcParty(PokeParty *party, u16 zoneId, HeapID heapId) {
    int i;

    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        happinessIncrement(PokeParty_GetPkm(party, i), sFriendshipDeltas[FRIENDSHIP_REASON_LEVEL_UP_PARTY], zoneId,
                           heapId);
    }
}

// Changes the friendship by the change of its tier. A gain is one more in a Luxury Ball and one more in the place
// where the Pokémon was met, and half as much again with a Soothe Bell
static void happinessIncrement(PartyPkm *pkm, const s8 *deltas, u16 zoneId, HeapID heapId) {
    int friendship;
    int delta;
    u16 placeName = ZoneData_GetPlaceNameID(zoneId);
    u8 tier = 0;
    u16 species = PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
    u8 holdEffect;

    if (species == 0 || species == SPECIES_EGG) {
        return;
    }
    holdEffect = GetItemParam(PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL), ITEM_PARAM_HOLD_EFFECT, heapId);
    friendship = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);
    if (friendship >= FRIENDSHIP_TIER_2) {
        tier++;
    }
    if (friendship >= FRIENDSHIP_TIER_3) {
        tier++;
    }
    delta = deltas[tier];
    if (delta > 0 && PokeParty_GetParam(pkm, PKM_PARAM_POKEBALL, NULL) == ITEM_LUXURY_BALL) {
        delta++;
    }
    if (placeName != 0 && delta > 0 && placeName == PokeParty_GetParam(pkm, PKM_PARAM_MET_LOCATION, NULL)) {
        delta++;
    }
    if (delta > 0 && holdEffect == HOLD_EFFECT_FRIENDSHIP_UP) {
        delta = delta * 150 / 100;
    }
    friendship += PassPower_ApplyBefriend(delta);
    if (friendship < 0) {
        friendship = 0;
    }
    if (friendship > FRIENDSHIP_MAX) {
        friendship = FRIENDSHIP_MAX;
    }
    PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, friendship);
}
