#include "types.h"
#include "app/local_tvt_local.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// What the rival says in a call on the Xtransceiver, named after the ROM's "local_tvt_person_rival.c"

typedef struct {
    int step;
    u32 unk4;
    GameData *gameData;
    u16 heapId;
} LocalTvtRival;

static LocalTvtRival *LocalTvtRival_Create(GameData *gameData, HeapID heapId);
static void LocalTvtRival_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtRival_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtRival_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtRival_Extra(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtRival_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);

void LocalTvtRival_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId) {
    sys_memset(funcs, 0, sizeof(LocalTvtPersonFuncs));
    funcs->work = LocalTvtRival_Create(gameData, heapId);
    funcs->msgData = msgData;
    funcs->free = LocalTvtRival_Free;
    funcs->greet = LocalTvtRival_Greet;
    funcs->topics[0] = LocalTvtRival_Chat;
    funcs->topicNames[0] = 15;
    funcs->topics[1] = LocalTvtRival_Extra;
    funcs->topicNames[1] = 16;
    funcs->topics[2] = LocalTvtRival_Bye;
    funcs->topicNames[2] = 17;
    funcs->topicCount = 3;
    funcs->menuMsg = 18;
}

static LocalTvtRival *LocalTvtRival_Create(GameData *gameData, HeapID heapId) {
    LocalTvtRival *rival = GFL_HeapAllocate(heapId, sizeof(LocalTvtRival), TRUE, "local_tvt_person_rival.c", 108);

    rival->heapId = heapId;
    rival->gameData = gameData;
    return rival;
}

static void LocalTvtRival_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    GFL_HeapFree(work);
}

static BOOL LocalTvtRival_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtRival *rival = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (rival->step) {
    case 0:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetGreetingMsg(setup->callData, LOCAL_TVT_CALL_RIVAL));
        rival->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            rival->step++;
        }
        break;
    case 2:
        rival->step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtRival_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtRival *rival = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (rival->step) {
    case 0:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetChatMsg(setup->callData, LOCAL_TVT_CALL_RIVAL));
        rival->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            rival->step++;
        }
        break;
    case 2:
        rival->step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtRival_Extra(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtRival *rival = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (rival->step) {
    case 0:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetExtraMsg(setup->callData, LOCAL_TVT_CALL_RIVAL));
        rival->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            rival->step++;
        }
        break;
    case 2:
        rival->step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtRival_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtRival *rival = work;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (rival->step) {
    case 0: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);

        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetByeMsg(setup->callData, LOCAL_TVT_CALL_RIVAL));
        rival->step++;
        break;
    }
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            rival->step++;
        }
        break;
    case 2:
        rival->step = 0;
        LocalTvtPerson_End(person);
        return TRUE;
    }
    return FALSE;
}
