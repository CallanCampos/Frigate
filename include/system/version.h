#ifndef POKEBW2_SYSTEM_VERSION_H
#define POKEBW2_SYSTEM_VERSION_H

#include "types.h"
#include "constants/version.h"

// VERSION_BLACK2 or VERSION_WHITE2
u32 getGameVersion(void);
extern const u8 region;
// The version that made the save's Pokémon, as their origin game
extern const u8 game_version;

#endif // POKEBW2_SYSTEM_VERSION_H
