#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_SCRIPTMODE_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_SCRIPTMODE_H

#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// local_tvt_scriptmode.c: plays a call's script

LocalTvtScriptMode *LocalTvtScriptMode_Create(HeapID heapId, GameSystem *gsys, GameData *gameData, u16 scriptId,
                                              LocalTvtChara *chara);
void LocalTvtScriptMode_Free(LocalTvtScriptMode *mode);
BOOL LocalTvtScriptMode_Main(LocalTvtScriptMode *mode);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_SCRIPTMODE_H
