#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CALLSELECT_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CALLSELECT_H

#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// local_tvt_callselect.c: the list of the persons to call on the lower screen

#define LOCAL_TVT_SELECT_MAX 6

// The results of the list
enum {
    LOCAL_TVT_SELECT_NONE,
    LOCAL_TVT_SELECT_CALL,
    LOCAL_TVT_SELECT_EXIT,
};

struct LocalTvtCallSelectSetup {
    RivalDataSave *rival;
    LocalTvtMessage *message;
    ClActUnit *unit;
    MsgData *msgData;
    // The messages of the names
    u32 names[LOCAL_TVT_SELECT_MAX];
    BOOL enabled[LOCAL_TVT_SELECT_MAX];
    int count;
};

LocalTvtCallSelect *LocalTvtCallSelect_Create(const LocalTvtCallSelectSetup *setup, HeapID heapId);
void LocalTvtCallSelect_Free(LocalTvtCallSelect *select);
void LocalTvtCallSelect_Main(LocalTvtCallSelect *select);
u32 LocalTvtCallSelect_GetResult(LocalTvtCallSelect *select);
int LocalTvtCallSelect_GetSelected(LocalTvtCallSelect *select);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CALLSELECT_H
