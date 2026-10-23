#ifndef POKEBW2_APP_PASS_POWER_H
#define POKEBW2_APP_PASS_POWER_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Pass Power screen (overlay 328, gpower_app.c, a guessed name): the running Pass Powers, and the ones the player
// can use for Pass Orbs

#define OVERLAY_PASS_POWER_APP OVERLAY_ID(328)

// What the screen returns in PassPowerParam's result
enum {
    PASS_POWER_APP_NONE,
    // A Pass Power was chosen, the param's passPower
    PASS_POWER_APP_USE,
    PASS_POWER_APP_ENTRALINK,
    // The game system ended the screen
    PASS_POWER_APP_ABORTED,
};

struct PassPowerEntry {
    u8 id;
    u8 pad;
    u16 seconds;
};

struct PassPowerParam {
    GameSystem *gsys;
    // 1 leaves out the way to the Entralink
    u32 argument;
    u32 highLinkIds[3];
    u8 highLinkCount;
    u8 powerCount;
    u16 itemCount;
    PassPowerEntry powers[10];
    // Set by the screen: a PASS_POWER_APP_*, and the Pass Power chosen
    u32 result;
    u32 passPower;
};

extern const GameProcFunctions GPOWER_APP_PROC_FUNCTIONS;

#endif // POKEBW2_APP_PASS_POWER_H
