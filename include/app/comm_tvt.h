#ifndef POKEBW2_APP_COMM_TVT_H
#define POKEBW2_APP_COMM_TVT_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Video chat with the Xtransceiver
#define OVERLAY_COMM_TVT OVERLAY_ID(257)

struct CommTvtParam {
    GameData *gameData;
    // How it starts: 0 waiting for calls, 1 calling, 2 answering the call of parentMac, 3 over the connection that
    // exists (the Wi-Fi Club's)
    u32 unk4;
    u8 parentMac[6];
};

extern const GameProcFunctions COMM_TVT_PROC_FUNCTIONS;

#endif // POKEBW2_APP_COMM_TVT_H
