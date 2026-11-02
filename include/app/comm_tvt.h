#ifndef POKEBW2_APP_COMM_TVT_H
#define POKEBW2_APP_COMM_TVT_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Video chat with the Xtransceiver
#define OVERLAY_COMM_TVT OVERLAY_ID(257)

// How the Xtransceiver starts
enum {
    COMM_TVT_START_WAIT,
    COMM_TVT_START_CALL,
    // Answering the call of parentMac
    COMM_TVT_START_ANSWER,
    // Over the connection that exists, the Wi-Fi Club's
    COMM_TVT_START_EXISTING,
};

struct CommTvtParam {
    GameData *gameData;
    // A COMM_TVT_START_*
    u32 startMode;
    u8 parentMac[6];
};

extern GameProcFunctions COMM_TVT_PROC_FUNCTIONS;

#endif // POKEBW2_APP_COMM_TVT_H
