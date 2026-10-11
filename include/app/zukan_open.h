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
#define ZUKAN_OPEN_RESULT_BACK 0
#define ZUKAN_OPEN_RESULT_CLOSE 1
#define ZUKAN_OPEN_RESULT_DONE 2

typedef struct {
    GameData *gameData;
    PlayerInfo *playerInfo;
    u32 result;
} ZukanOpenParam;

extern const GameProcFunctions ZUKAN_OPEN_PROC_FUNCTIONS;

// zukan_common.c of overlay 302, which has no header yet: wipes in and out of the Pokédex's screens
void func_ov302_021ad78c(HeapID heapId);
void func_ov302_021ad7a8(HeapID heapId);

#endif // POKEBW2_APP_ZUKAN_OPEN_H
