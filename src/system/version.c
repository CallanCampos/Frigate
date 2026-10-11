#include "types.h"
#include "system/version.h"

// The game's version and region: a Pokémon's origin game and where the cartridge was sold (the ROM has no string for
// the file, so the name is a guess).

u32 getGameVersion(void) {
    return GAME_VERSION;
}

const u8 game_version = GAME_VERSION;
const u8 region = 2;
