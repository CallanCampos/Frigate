#include "types.h"
#include "app/local_tvt_local.h"
#include "constants/flags.h"
#include "constants/pokemon.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/event_work.h"
#include "system/game_data.h"

// What the person of LOCAL_TVT_CALL_SUPPORT says in a call on the Xtransceiver, named after the ROM's
// "local_tvt_person_support.c": how friendly a Pokémon of the party is and, once a flag is set, how its effort
// values are

#define EV_STAT_COUNT 6
#define EV_TOTAL_MAX 510
#define EV_STAT_MAX 252

typedef struct {
    int step;
    u32 timer;
    LocalTvtPokeSelect *select;
    GameData *gameData;
    u16 heapId;
} LocalTvtSupport;

static LocalTvtSupport *LocalTvtSupport_Create(GameData *gameData, HeapID heapId);
static void LocalTvtSupport_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtSupport_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtSupport_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtSupport_Happiness(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtSupport_EffortValues(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtSupport_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);

void LocalTvtSupport_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId) {
    sys_memset(funcs, 0, sizeof(LocalTvtPersonFuncs));
    funcs->work = LocalTvtSupport_Create(gameData, heapId);
    funcs->free = LocalTvtSupport_Free;
    funcs->greet = LocalTvtSupport_Greet;
    funcs->topics[0] = LocalTvtSupport_Chat;
    funcs->topicNames[0] = 26;
    funcs->topics[1] = LocalTvtSupport_Happiness;
    funcs->topicNames[1] = 27;
    if (EventWork_FlagGet(GameData_GetEventWork(gameData), EVENT_FLAG_0x0960)) {
        funcs->topics[2] = LocalTvtSupport_EffortValues;
        funcs->topicNames[2] = 28;
        funcs->topics[3] = LocalTvtSupport_Bye;
        funcs->topicNames[3] = 29;
        funcs->topicCount = 4;
    } else {
        funcs->topics[2] = LocalTvtSupport_Bye;
        funcs->topicNames[2] = 29;
        funcs->topicCount = 3;
    }
    funcs->menuMsg = 51;
    funcs->msgData = msgData;
}

static LocalTvtSupport *LocalTvtSupport_Create(GameData *gameData, HeapID heapId) {
    LocalTvtSupport *support = GFL_HeapAllocate(heapId, sizeof(LocalTvtSupport), TRUE, "local_tvt_person_support.c", 130);

    support->heapId = heapId;
    support->gameData = gameData;
    return support;
}

static void LocalTvtSupport_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    GFL_HeapFree(work);
}

static BOOL LocalTvtSupport_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtSupport *support = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (support->step) {
    case 0:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetGreetingMsg(setup->callData, LOCAL_TVT_CALL_SUPPORT));
        support->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            support->step++;
        }
        break;
    case 2:
        support->step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtSupport_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtSupport *support = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (support->step) {
    case 0:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetChatMsg(setup->callData, LOCAL_TVT_CALL_SUPPORT));
        support->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            support->step++;
        }
        break;
    case 2:
        support->step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtSupport_Happiness(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtSupport *support = work;
    u16 messageId = 0;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (support->step) {
    case 0:
        LocalTvtFace_Talk(face, LocalTvtMessage_GetTalkMsgData(setup->message), 30);
        support->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            support->timer = 0;
            support->step++;
        }
        break;
    case 2:
        if (support->timer++ > 15) {
            support->step++;
        }
        break;
    case 3:
        if (support->select != NULL) {
            LocalTvtPokeSelect_SetActive(support->select, TRUE);
            LocalTvtPokeSelect_Restart(support->select);
        } else {
            LocalTvtPokeSelectSetup selectSetup;

            sys_memset(&selectSetup, 0, sizeof(selectSetup));
            selectSetup.message = setup->message;
            selectSetup.unit = setup->unit;
            selectSetup.party = GameData_GetParty(support->gameData);
            support->select = LocalTvtPokeSelect_Create(&selectSetup, support->heapId);
        }
        support->step++;
        break;
    case 4:
        LocalTvtPokeSelect_Main(support->select);
        switch (LocalTvtPokeSelect_GetResult(support->select)) {
        case LOCAL_TVT_POKE_SELECT_DECIDE:
            LocalTvtPokeSelect_SetActive(support->select, FALSE);
            support->step = 6;
            break;
        case LOCAL_TVT_POKE_SELECT_CANCEL:
            support->step = 5;
            break;
        }
        break;
    case 5:
        LocalTvtPokeSelect_Free(support->select);
        support->select = NULL;
        support->step = 0;
        return TRUE;
    case 6: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        PartyPkm *pkm = LocalTvtPokeSelect_GetPkm(support->select);
        u32 happiness = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);

        if (happiness == 0) {
            messageId = 31;
        } else if (happiness >= 1 && happiness <= 49) {
            messageId = 32;
        } else if (happiness >= 50 && happiness <= 99) {
            messageId = 33;
        } else if (happiness >= 100 && happiness <= 149) {
            messageId = 34;
        } else if (happiness >= 150 && happiness <= 199) {
            messageId = 35;
        } else if (happiness >= 200 && happiness <= 254) {
            messageId = 36;
        } else {
            messageId = 37;
        }
        LocalTvtMessage_SetPlayerName(setup->message, support->gameData);
        LocalTvtMessage_SetPokemonNickname(setup->message, pkm, 1);
        LocalTvtFace_Talk(face, msgData, messageId);
        support->step++;
        break;
    }
    case 7:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            support->step = 0;
        }
        break;
    }
    return FALSE;
}

static BOOL LocalTvtSupport_EffortValues(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtSupport *support = work;
    int count = 0;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);
    int i;

    switch (support->step) {
    case 0:
        LocalTvtFace_Talk(face, LocalTvtMessage_GetTalkMsgData(setup->message), 38);
        support->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            support->timer = 0;
            support->step++;
        }
        break;
    case 2:
        if (support->timer++ > 15) {
            support->step++;
        }
        break;
    case 3:
        if (support->select != NULL) {
            LocalTvtPokeSelect_SetActive(support->select, TRUE);
            LocalTvtPokeSelect_Restart(support->select);
        } else {
            LocalTvtPokeSelectSetup selectSetup;

            sys_memset(&selectSetup, 0, sizeof(selectSetup));
            selectSetup.message = setup->message;
            selectSetup.unit = setup->unit;
            selectSetup.party = GameData_GetParty(support->gameData);
            support->select = LocalTvtPokeSelect_Create(&selectSetup, support->heapId);
        }
        support->step++;
        break;
    case 4:
        LocalTvtPokeSelect_Main(support->select);
        switch (LocalTvtPokeSelect_GetResult(support->select)) {
        case LOCAL_TVT_POKE_SELECT_DECIDE:
            LocalTvtPokeSelect_SetActive(support->select, FALSE);
            support->step = 6;
            break;
        case LOCAL_TVT_POKE_SELECT_CANCEL:
            support->step = 5;
            break;
        }
        break;
    case 5:
        LocalTvtPokeSelect_Free(support->select);
        support->select = NULL;
        support->step = 0;
        return TRUE;
    case 6: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        PartyPkm *pkm = LocalTvtPokeSelect_GetPkm(support->select);

        // Whether it can still get effort values
        for (i = 0; i < EV_STAT_COUNT; i++) {
            count += PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + i, NULL);
        }
        if (count == EV_TOTAL_MAX) {
            count = 40;
        } else {
            count = 39;
        }
        LocalTvtMessage_SetPlayerName(setup->message, support->gameData);
        LocalTvtMessage_SetPokemonNickname(setup->message, pkm, 1);
        LocalTvtFace_Talk(face, msgData, count);
        support->step++;
        break;
    }
    case 7:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            support->step = 8;
        }
        break;
    case 8: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        PartyPkm *pkm = LocalTvtPokeSelect_GetPkm(support->select);
        int total = 0;
        u32 messageId;

        // The stats that are full, of which there can be two
        for (i = 0; i < EV_STAT_COUNT; i++) {
            u32 value = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + i, NULL);
            int statMsg = 43 + i;

            total += value;
            if (value >= EV_STAT_MAX) {
                LocalTvtMessage_SetWord(setup->message, msgData, statMsg, count);
                count++;
            }
        }
        if (count == 1) {
            if (total == EV_TOTAL_MAX) {
                messageId = 41;
            } else {
                messageId = 49;
            }
        } else if (count == 2) {
            if (total == EV_TOTAL_MAX) {
                messageId = 42;
            } else {
                messageId = 50;
            }
        } else {
            support->step = 0;
            break;
        }
        LocalTvtFace_Talk(face, msgData, messageId);
        support->step++;
        break;
    }
    case 9:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            support->step = 0;
        }
        break;
    }
    return FALSE;
}

static BOOL LocalTvtSupport_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtSupport *support = work;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (support->step) {
    case 0: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);

        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetByeMsg(setup->callData, LOCAL_TVT_CALL_SUPPORT));
        support->step++;
        break;
    }
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            support->step++;
        }
        break;
    case 2:
        support->step = 0;
        LocalTvtPerson_End(person);
        return TRUE;
    }
    return FALSE;
}
