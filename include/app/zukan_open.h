#ifndef POKEBW2_APP_ZUKAN_OPEN_H
#define POKEBW2_APP_ZUKAN_OPEN_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Pokédex's opening, overlay 301 (zukan_open.c): the Pokédex's cover, with the downloaded skin if there is one,
// whose halves slide apart once the player presses A, touches the screen or waits. B or X leaves it. The Pokédex
// (overlay 302, zukan.c) queues it. The name is a guess: the ROM has no string for it
#define OVERLAY_ZUKAN_OPEN OVERLAY_ID(301)

// What the Pokédex ends the opening with: B, X or the end of the opening. The Pokédex tells by the result whether to
// go on
enum {
    ZUKAN_OPEN_RESULT_BACK,
    ZUKAN_OPEN_RESULT_CLOSE,
    ZUKAN_OPEN_RESULT_DONE,
};

typedef struct {
    GameData *gameData;
    PlayerInfo *playerInfo;
    // A ZUKAN_OPEN_RESULT_*
    u32 result;
} ZukanOpenParam;

extern const GameProcFunctions ZUKAN_OPEN_PROC_FUNCTIONS;

#endif // POKEBW2_APP_ZUKAN_OPEN_H
