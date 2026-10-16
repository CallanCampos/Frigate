#ifndef POKEBW2_DPW_INTERNAL_H
#define POKEBW2_DPW_INTERNAL_H

#include "types.h"
#include "dwc/connectinet.h"
#include "dwc/error.h"
#include <string.h>

// What the sources of the dpw library call outside themselves, none of it with a name in the ROM

// The key of the game statistics client: the game's secret, four numbers of its scrambling, and its name
#define DPW_GAMESTATS_KEY "HZEdGCzcGGLvguqUEKQN0001d93500002dd5000000082db842b2syachi2ds"

// The friend key, as the two words it is kept in
typedef struct DpwFriendKey {
    u32 low;
    u32 high;
} DpwFriendKey;

#endif // POKEBW2_DPW_INTERNAL_H
