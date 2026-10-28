#ifndef POKEBW2_FIELD_EVENT_WILD_BATTLE_H
#define POKEBW2_FIELD_EVENT_WILD_BATTLE_H

#include "types.h"
#include "struct_decls.h"

// How a random encounter starts: by the player's steps, from an effect encounter's shaking grass or rippling water,
// or by Sweet Scent. The last two skip the encounter rate
enum {
    ENCOUNT_MODE_NORMAL,
    ENCOUNT_MODE_EFFECT,
    ENCOUNT_MODE_SWEET_SCENT,
    // The fishing event's mode for a bite off a calm spot, which takes the water's effect and BGM; a bite off a
    // rippling spot is ENCOUNT_MODE_EFFECT
    ENCOUNT_MODE_FISHING = 4,
};

GameEvent *EventWildBattleCall_CreateRandom(EncountSystem *encountSystem, u32 mode);

#endif // POKEBW2_FIELD_EVENT_WILD_BATTLE_H
