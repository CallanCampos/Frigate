#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CALLMODE_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CALLMODE_H

#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// local_tvt_callmode.c: the calls the player makes

// Whether the player picks the person, or the call is with LOCAL_TVT_CALL_SECRET at once
enum {
    LOCAL_TVT_CALL_MODE_SELECT,
    LOCAL_TVT_CALL_MODE_SECRET,
    LOCAL_TVT_CALL_MODE_2,
};

struct LocalTvtCallModeSetup {
    GameSystem *gsys;
    GameData *gameData;
    LocalTvtChara *chara;
    LocalTvtMessage *message;
    ClActUnit *unit;
    // LOCAL_TVT_CALL_MODE_*
    u32 mode;
};

LocalTvtCallMode *LocalTvtCallMode_Create(const LocalTvtCallModeSetup *setup, HeapID heapId);
void LocalTvtCallMode_Free(LocalTvtCallMode *mode);
BOOL LocalTvtCallMode_Main(LocalTvtCallMode *mode);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CALLMODE_H
