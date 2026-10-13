#include "types.h"
#include "app/local_tvt_local.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "system/game_data.h"

// What the person of LOCAL_TVT_CALL_CHELEN says in a call on the Xtransceiver, named after the ROM's
// "local_tvt_person_chelen.c": what the types of a Pokémon of the party are weak to, and what its Ability does

// The types a Pokémon can have; a pair of types is counted with the smaller type first
#define TYPE_COUNT 17

typedef struct {
    int step;
    u32 timer;
    LocalTvtPokeSelect *select;
    GameData *gameData;
    u16 heapId;
    // The banks with a message for each pair of types and for each Ability
    MsgData *typeMsgData;
    MsgData *abilityMsgData;
} LocalTvtChelen;

static LocalTvtChelen *LocalTvtChelen_Create(GameData *gameData, HeapID heapId);
static void LocalTvtChelen_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtChelen_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtChelen_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtChelen_Types(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtChelen_Ability(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtChelen_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static u16 LocalTvtChelen_GetTypesMsg(u16 type1, u16 type2);

void LocalTvtChelen_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId) {
    sys_memset(funcs, 0, sizeof(LocalTvtPersonFuncs));
    funcs->work = LocalTvtChelen_Create(gameData, heapId);
    funcs->msgData = msgData;
    funcs->free = LocalTvtChelen_Free;
    funcs->greet = LocalTvtChelen_Greet;
    funcs->topics[0] = LocalTvtChelen_Chat;
    funcs->topicNames[0] = 19;
    funcs->topics[1] = LocalTvtChelen_Types;
    funcs->topicNames[1] = 20;
    funcs->topics[2] = LocalTvtChelen_Ability;
    funcs->topicNames[2] = 21;
    funcs->topics[3] = LocalTvtChelen_Bye;
    funcs->topicNames[3] = 22;
    funcs->topicCount = 4;
    funcs->menuMsg = 25;
}

static LocalTvtChelen *LocalTvtChelen_Create(GameData *gameData, HeapID heapId) {
    LocalTvtChelen *chelen = GFL_HeapAllocate(heapId, sizeof(LocalTvtChelen), TRUE, "local_tvt_person_chelen.c", 120);

    chelen->heapId = heapId;
    chelen->gameData = gameData;
    chelen->typeMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0397, heapId);
    chelen->abilityMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0396, heapId);
    return chelen;
}

static void LocalTvtChelen_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtChelen *chelen = work;

    GFL_MsgDataFree(chelen->abilityMsgData);
    GFL_MsgDataFree(chelen->typeMsgData);
    GFL_HeapFree(chelen);
}

static BOOL LocalTvtChelen_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtChelen *chelen = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (chelen->step) {
    case 0:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetGreetingMsg(setup->callData, LOCAL_TVT_CALL_CHELEN));
        chelen->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            chelen->step++;
        }
        break;
    case 2:
        chelen->step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtChelen_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtChelen *chelen = work;
    MsgData *msgData = NULL;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (chelen->step) {
    case 0:
        msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetChatMsg(setup->callData, LOCAL_TVT_CALL_CHELEN));
        chelen->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            chelen->step++;
        }
        break;
    case 2:
        chelen->step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtChelen_Types(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtChelen *chelen = work;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (chelen->step) {
    case 0:
        LocalTvtFace_Talk(face, LocalTvtMessage_GetTalkMsgData(setup->message), 23);
        chelen->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            chelen->timer = 0;
            chelen->step++;
        }
        break;
    case 2:
        if (chelen->timer++ > 15) {
            chelen->step++;
        }
        break;
    case 3:
        if (chelen->select != NULL) {
            LocalTvtPokeSelect_SetActive(chelen->select, TRUE);
            LocalTvtPokeSelect_Restart(chelen->select);
        } else {
            LocalTvtPokeSelectSetup selectSetup;

            sys_memset(&selectSetup, 0, sizeof(selectSetup));
            selectSetup.message = setup->message;
            selectSetup.unit = setup->unit;
            selectSetup.party = GameData_GetParty(chelen->gameData);
            chelen->select = LocalTvtPokeSelect_Create(&selectSetup, chelen->heapId);
        }
        chelen->step++;
        break;
    case 4:
        LocalTvtPokeSelect_Main(chelen->select);
        switch (LocalTvtPokeSelect_GetResult(chelen->select)) {
        case LOCAL_TVT_POKE_SELECT_DECIDE:
            LocalTvtPokeSelect_SetActive(chelen->select, FALSE);
            chelen->step = 6;
            break;
        case LOCAL_TVT_POKE_SELECT_CANCEL:
            chelen->step = 5;
            break;
        }
        break;
    case 5:
        LocalTvtPokeSelect_Free(chelen->select);
        chelen->select = NULL;
        chelen->step = 0;
        return TRUE;
    case 6: {
        PartyPkm *pkm = LocalTvtPokeSelect_GetPkm(chelen->select);
        u32 type1 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE1, NULL);
        u32 type2 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE2, NULL);
        u16 messageId = LocalTvtChelen_GetTypesMsg(type1, type2);

        LocalTvtMessage_SetPlayerName(setup->message, chelen->gameData);
        LocalTvtMessage_SetPokemonNickname(setup->message, pkm, 1);
        LocalTvtFace_Talk(face, chelen->typeMsgData, messageId);
        chelen->step++;
        break;
    }
    case 7:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            chelen->step = 0;
        }
        break;
    }
    return FALSE;
}

static BOOL LocalTvtChelen_Ability(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtChelen *chelen = work;
    u16 messageId = 0;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (chelen->step) {
    case 0:
        LocalTvtFace_Talk(face, LocalTvtMessage_GetTalkMsgData(setup->message), 24);
        chelen->step++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            chelen->timer = 0;
            chelen->step++;
        }
        break;
    case 2:
        if (chelen->timer++ > 15) {
            chelen->step++;
        }
        break;
    case 3:
        if (chelen->select != NULL) {
            LocalTvtPokeSelect_SetActive(chelen->select, TRUE);
            LocalTvtPokeSelect_Restart(chelen->select);
        } else {
            LocalTvtPokeSelectSetup selectSetup;

            sys_memset(&selectSetup, 0, sizeof(selectSetup));
            selectSetup.message = setup->message;
            selectSetup.unit = setup->unit;
            selectSetup.party = GameData_GetParty(chelen->gameData);
            chelen->select = LocalTvtPokeSelect_Create(&selectSetup, chelen->heapId);
        }
        chelen->step++;
        break;
    case 4:
        LocalTvtPokeSelect_Main(chelen->select);
        switch (LocalTvtPokeSelect_GetResult(chelen->select)) {
        case LOCAL_TVT_POKE_SELECT_DECIDE:
            LocalTvtPokeSelect_SetActive(chelen->select, FALSE);
            chelen->step = 6;
            break;
        case LOCAL_TVT_POKE_SELECT_CANCEL:
            chelen->step = 5;
            break;
        }
        break;
    case 5:
        LocalTvtPokeSelect_Free(chelen->select);
        chelen->select = NULL;
        chelen->step = 0;
        return TRUE;
    case 6: {
        PartyPkm *pkm = LocalTvtPokeSelect_GetPkm(chelen->select);

        messageId = PokeParty_GetParam(pkm, PKM_PARAM_ABILITY, NULL);
        LocalTvtMessage_SetPlayerName(setup->message, chelen->gameData);
        LocalTvtMessage_SetPokemonNickname(setup->message, pkm, 1);
        LocalTvtFace_Talk(face, chelen->abilityMsgData, messageId);
        chelen->step++;
        break;
    }
    case 7:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            chelen->step = 0;
        }
        break;
    }
    return FALSE;
}

static BOOL LocalTvtChelen_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtChelen *chelen = work;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (chelen->step) {
    case 0: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);

        LocalTvtFace_Talk(face, msgData, LocalTvtCallData_GetByeMsg(setup->callData, LOCAL_TVT_CALL_CHELEN));
        chelen->step++;
        break;
    }
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            chelen->step++;
        }
        break;
    case 2:
        chelen->step = 0;
        LocalTvtPerson_End(person);
        return TRUE;
    }
    return FALSE;
}

// The index of a pair of types among the pairs with the smaller type first
static u16 LocalTvtChelen_GetTypesMsg(u16 type1, u16 type2) {
    int i;
    int index = 0;

    if (type1 > type2) {
        int type = type1;

        type1 = type2;
        type2 = type;
    }
    for (i = 0; i < type1; i++) {
        index += TYPE_COUNT - i;
    }
    return index + (type2 - i);
}
