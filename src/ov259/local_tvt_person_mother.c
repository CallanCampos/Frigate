#include "types.h"
#include "app/local_tvt_local.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "system/game_data.h"

// What the player's mother says in a call on the Xtransceiver, named after the ROM's "local_tvt_person_mother.c":
// she tells about the place the player is at

typedef struct {
    int step;
    u32 unk4;
    GameData *gameData;
    // The bank of what she says about each place, by the message of the place's name
    MsgData *placeMsgData;
    u16 heapId;
} LocalTvtMother;

static LocalTvtMother *LocalTvtMother_Create(GameData *gameData, HeapID heapId);
static void LocalTvtMother_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtMother_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtMother_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtMother_Place(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtMother_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);

void LocalTvtMother_Setup(LocalTvtPersonFuncs *funcs, LocalTvtMessage *message, MsgData *msgData, GameData *gameData,
                          HeapID heapId) {
    sys_memset(funcs, 0, sizeof(LocalTvtPersonFuncs));
    funcs->work = LocalTvtMother_Create(gameData, heapId);
    funcs->free = LocalTvtMother_Free;
    funcs->greet = LocalTvtMother_Greet;
    funcs->msgData = msgData;
    funcs->topics[0] = LocalTvtMother_Chat;
    funcs->topicNames[0] = 52;
    funcs->topics[1] = LocalTvtMother_Place;
    funcs->topicNames[1] = 53;
    funcs->topics[2] = LocalTvtMother_Bye;
    funcs->topicNames[2] = 54;
    funcs->topicCount = 3;
    funcs->menuMsg = 55;
    LocalTvtMessage_SetPlaceName(message, gameData, 0);
}

static LocalTvtMother *LocalTvtMother_Create(GameData *gameData, HeapID heapId) {
    LocalTvtMother *mother = GFL_HeapAllocate(heapId, sizeof(LocalTvtMother), TRUE, "local_tvt_person_mother.c", 122);

    mother->heapId = heapId;
    mother->gameData = gameData;
    mother->placeMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_STARTMENU_6, heapId);
    return mother;
}

static void LocalTvtMother_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtMother *mother = work;

    GFL_MsgDataFree(mother->placeMsgData);
    GFL_HeapFree(mother);
}

static BOOL LocalTvtMother_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtMother *mother = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (mother->step) {
    case 0:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetGreetingMsg(setup->callData, LOCAL_TVT_CALL_MOTHER));
        mother->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            mother->step++;
        }
        break;
    case 2:
        mother->step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtMother_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtMother *mother = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (mother->step) {
    case 0:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetChatMsg(setup->callData, LOCAL_TVT_CALL_MOTHER));
        mother->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            mother->step++;
        }
        break;
    case 2:
        mother->step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtMother_Place(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtMother *mother = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (mother->step) {
    case 0:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(
            face, mother->placeMsgData,
            ZoneData_GetPlaceNameID(PlayerState_GetZoneID(GameData_GetPlayerState(mother->gameData))));
        mother->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            mother->step++;
        }
        break;
    case 2:
        mother->step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtMother_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtMother *mother = work;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (mother->step) {
    case 0: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);

        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetByeMsg(setup->callData, LOCAL_TVT_CALL_MOTHER));
        mother->step++;
        break;
    }
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            mother->step++;
        }
        break;
    case 2:
        mother->step = 0;
        LocalTvtPerson_End(person);
        return TRUE;
    }
    return FALSE;
}
