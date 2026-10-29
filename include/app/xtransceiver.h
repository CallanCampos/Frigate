#ifndef POKEBW2_APP_XTRANSCEIVER_H
#define POKEBW2_APP_XTRANSCEIVER_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 259: the Xtransceiver's calls with the game's characters ("local_tvt" in the ROM's file names), which a
// script starts or the player makes from the menu

#define OVERLAY_XTRANSCEIVER OVERLAY_ID(259)

// What the Xtransceiver does: play a call's script, let the player pick whom to call, or call the person whose
// name the player learns later
enum {
    LOCAL_TVT_MODE_SCRIPT,
    LOCAL_TVT_MODE_CALL,
    LOCAL_TVT_MODE_CALL_SECRET,
};

// The Xtransceiver's parameters
struct LocalTvtParam {
    u32 mode;
    // Whether the background takes the palette of the other sex's player (LocalTvtBG_Create)
    u32 swapPalette;
    // The script of LOCAL_TVT_MODE_SCRIPT
    u16 scriptId;
    GameData *gameData;
    GameSystem *gsys;
};

extern const GameProcFunctions XTRANSCEIVER_PROC_FUNCTIONS;

#endif // POKEBW2_APP_XTRANSCEIVER_H
