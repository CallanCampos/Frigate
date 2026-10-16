#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_PERSON_MOTHER_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_PERSON_MOTHER_H

#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "gfl/heap.h"
#include "struct_decls.h"

void LocalTvtMother_Setup(LocalTvtPersonFuncs *funcs, LocalTvtMessage *message, MsgData *msgData, GameData *gameData,
                          HeapID heapId);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_PERSON_MOTHER_H
