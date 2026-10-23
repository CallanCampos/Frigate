#ifndef POKEBW2_APP_COMM_TVT_FIELD_H
#define POKEBW2_APP_COMM_TVT_FIELD_H

#include "types.h"
#include "app/comm_tvt.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// comm_tvt_field.c, overlay 198: the Xtransceiver's screen in the field, where the player starts or answers a call.
// The ROM has no string for the file, so the name is a guess. The names here are ours

#define OVERLAY_COMM_TVT_FIELD_APP OVERLAY_ID(198)

// What event_comm_tvt.c (overlay 17) passes: the game, the parameter of the video chat that the screen fills in, and
// what is left to the event
typedef struct CommTvtFieldParam CommTvtFieldParam;

struct CommTvtFieldParam {
    GameSystem *gsys;
    GameData *gameData;
    SaveControl *save;
    // The parameter of the video chat (overlay 257): the screen sets unk4 to 1 to call or 2 to answer, and parentMac
    CommTvtParam commParam;
    u32 unk1C;
    // 0 if the screen left to wait, 1 if it left to start the video chat
    u32 mode;
    u32 bgm;
    u32 exitArgument;
    u32 unused2C;
    u32 unused30;
};

// The symbol is still COMM_TVT_FIELD_PROC_FUNCTIONS until it is renamed to COMM_TVT_FIELD_PROC_FUNCTIONS, the name in
// the C file
extern const GameProcFunctions COMM_TVT_FIELD_PROC_FUNCTIONS;

// The index of the friend who calls, or -1
int WihDwc_FindFriendInvite(WifiList *wifiList);

#endif // POKEBW2_APP_COMM_TVT_FIELD_H
