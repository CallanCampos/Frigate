#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_PERSON_SECRET_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_PERSON_SECRET_H

#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "gfl/heap.h"
#include "struct_decls.h"

void LocalTvtSecret_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, LocalTvtCallData *callData,
                          BOOL unk, HeapID heapId);
// The face of the person, which depends on the player's gender
u16 LocalTvtSecret_GetFacePerson(LocalTvtCallData *data);
// The message of the name of the person, whose name the player learns later
u16 LocalTvtSecret_GetNameMsg(LocalTvtCallData *data);
// How often the Xtransceiver rings before the person answers
u32 LocalTvtSecret_GetRingCount(LocalTvtCallData *data);
// The stage of the calls with the person for a count of calls, and what a stage has
u32 LocalTvtSecret_GetStage(int callCount);
u32 LocalTvtSecret_GetStage0378(u32 stage);
u32 LocalTvtSecret_GetStageCount(u32 stage);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_PERSON_SECRET_H
