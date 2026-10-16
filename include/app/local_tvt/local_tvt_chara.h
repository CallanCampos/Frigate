#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CHARA_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CHARA_H

#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// local_tvt_chara.c: the faces of the upper screen

// What a face shows
struct LocalTvtFaceSetup {
    u16 person;
    u16 background;
    u16 object;
    // A message of the names' bank, or LOCAL_TVT_NAME_DEFAULT
    u16 nameMsg;
};

struct LocalTvtCharaSetup {
    ClActUnit *unit;
    PlayerInfo *playerInfo;
    RivalDataSave *rival;
    LocalTvtMessage *message;
    u32 time;
};

LocalTvtChara *LocalTvtChara_Create(const LocalTvtCharaSetup *setup, HeapID heapId);
void LocalTvtChara_Free(LocalTvtChara *chara);
void LocalTvtChara_Update(LocalTvtChara *chara);
// Starts loading the faces of a call, one step a frame, until LocalTvtChara_IsReady
void LocalTvtChara_Start(LocalTvtChara *chara, const LocalTvtFaceSetup *setups, int count);
BOOL LocalTvtChara_IsReady(LocalTvtChara *chara);
void LocalTvtChara_SetVisible(LocalTvtChara *chara, BOOL visible);
LocalTvtFace *LocalTvtChara_GetFace(LocalTvtChara *chara, int index);
// The face says a message, and LocalTvtFace_UpdateTalk returns TRUE once it is over. With a key wait the last page
// stays until the player presses a key
void LocalTvtFace_Talk(LocalTvtFace *face, MsgData *msgData, u16 messageId);
void LocalTvtFace_TalkKeyWait(LocalTvtFace *face, MsgData *msgData, u16 messageId);
BOOL LocalTvtFace_UpdateTalk(LocalTvtFace *face, LocalTvtChara *chara);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_CHARA_H
