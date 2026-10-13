#include "types.h"
#include "app/local_tvt_local.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "constants/types.h"
#include "constants/vars.h"
#include "field/survey.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "save/event_work.h"
#include "save/save_control.h"
#include "save/survey_savedata.h"
#include "system/game_data.h"

// What the person of LOCAL_TVT_CALL_SECRET says in a call on the Xtransceiver, named after the ROM's
// "local_tvt_person_secret.c". The player learns who the person is over many calls: the count of calls made picks
// a stage, which has the kind of call, the face and how long the person lets it ring. The first calls each ask
// about an answer the player gave in a survey

// The last count of calls, which has no more to say
#define SECRET_CALL_MAX 51
// The first calls each talk about a survey answer
#define SECRET_SURVEY_CALLS 8
// The calls that talk about the player's badges and first Pokémon
#define SECRET_CALL_BADGES 8
#define SECRET_CALL_STARTER 21
// The messages of the other gender's person come this far after
#define SECRET_MSG_OTHER_GENDER 78

// The kinds of call of a stage
enum {
    SECRET_CALL_TOPICS,
    SECRET_CALL_SURVEY,
    SECRET_CALL_SHORT,
    SECRET_CALL_RANDOM,
    SECRET_CALL_4,
};

typedef struct {
    int step;
    int talkStep;
    BOOL chatted;
    u32 chatMsg;
    u32 unk10;
    GameData *gameData;
    BOOL unk18;
    u16 heapId;
    u8 *surveyAnswers;
} LocalTvtSecret;

typedef struct {
    int callCountMin;
    int callCountMax;
    u32 callType;
    u32 face;
    u32 unk10;
    u32 unk14;
    u32 ringCount;
} LocalTvtSecretStage;

static LocalTvtSecret *LocalTvtSecret_Create(GameData *gameData, BOOL unk, HeapID heapId);
static void LocalTvtSecret_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtSecret_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtSecret_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtSecret_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtSecret_TalkGreeting(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, LocalTvtSecret *secret,
                                        int *step);
static BOOL LocalTvtSecret_TalkChat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, LocalTvtSecret *secret,
                                    int *step);
static BOOL LocalTvtSecret_TalkBye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, LocalTvtSecret *secret,
                                   int *step);
static BOOL LocalTvtSecret_ShortCall(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static BOOL LocalTvtSecret_WholeCall(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
static u32 LocalTvtSecret_GetStage10(LocalTvtCallData *data);
static u32 LocalTvtSecret_GetChatMsg(u8 *surveyAnswers, LocalTvtCallData *data, LocalTvtMessage *message);
static void LocalTvtSecret_SetWords(LocalTvtCallData *data, LocalTvtMessage *message, u32 callCount, u32 answerMsg,
                                    BOOL answered);
static u16 LocalTvtSecret_GetSurveyChatMsg(LocalTvtCallData *data, u16 person, BOOL answered);
static u32 LocalTvtSecret_GetRandomChatMsg(LocalTvtCallData *data, u32 callCount);
static void LocalTvtSecret_GetSurveyAnswer(u8 *surveyAnswers, u8 callCount, u32 *answerMsg, BOOL *answered,
                                           BOOL *asked);
static u32 LocalTvtSecret_GetStageCallType(u32 stage);
static u32 LocalTvtSecret_GetStageFace(u32 stage);
static u32 LocalTvtSecret_GetStage10Of(u32 stage);
static u32 LocalTvtSecret_GetStageRingCount(u32 stage);

// The types of the first Pokémon, by the work that keeps which one the player chose
static const u32 sLocalTvtStarterTypes[] = { TYPE_GRASS, TYPE_FIRE, TYPE_WATER };

// The survey question each of the first calls talks about
static const u32 sLocalTvtSecretQuestions[SECRET_SURVEY_CALLS] = { 1, 25, 28, 20, 7, 19, 2, 12 };

static const LocalTvtSecretStage sLocalTvtSecretStages[] = {
    { 0, 8, SECRET_CALL_SURVEY, 0, 0, FALSE, 2 },   { 9, 9, SECRET_CALL_4, 0, 3, FALSE, 2 },
    { 10, 28, SECRET_CALL_TOPICS, 1, 0, TRUE, 2 },  { 29, 29, SECRET_CALL_TOPICS, 3, 1, TRUE, 2 },
    { 30, 38, SECRET_CALL_RANDOM, 3, 2, TRUE, 2 },  { 39, 39, SECRET_CALL_TOPICS, 0, 0, TRUE, 4 },
    { 40, 48, SECRET_CALL_RANDOM, 3, 2, TRUE, 2 },  { 49, 49, SECRET_CALL_SHORT, 2, 0, TRUE, 4 },
    { 50, 50, SECRET_CALL_TOPICS, 3, 0, TRUE, 2 },  { 51, 51, SECRET_CALL_RANDOM, 3, 2, TRUE, 2 },
};

void LocalTvtSecret_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, LocalTvtCallData *callData,
                          BOOL unk, HeapID heapId) {
    u32 callType;
    BOOL male;

    sys_memset(funcs, 0, sizeof(LocalTvtPersonFuncs));
    funcs->work = LocalTvtSecret_Create(gameData, unk, heapId);
    funcs->free = LocalTvtSecret_Free;
    callType = LocalTvtSecret_GetStageCallType(
        LocalTvtSecret_GetStage(LocalTvtCallData_GetCallCount(callData, LOCAL_TVT_CALL_SECRET)));
    male = LocalTvtCallData_IsPlayerMale(callData);
    if (callType == SECRET_CALL_SHORT) {
        funcs->greet = LocalTvtSecret_ShortCall;
        return;
    }
    if (callType == SECRET_CALL_4 || callType == SECRET_CALL_SURVEY) {
        funcs->greet = LocalTvtSecret_WholeCall;
        return;
    }
    funcs->greet = LocalTvtSecret_Greet;
    funcs->topics[0] = LocalTvtSecret_Chat;
    funcs->topicNames[0] = 56;
    funcs->topics[1] = LocalTvtSecret_Bye;
    funcs->topicNames[1] = 57;
    funcs->topicCount = 2;
    funcs->menuMsg = LocalTvtCallData_GetExtraMsg(callData, LOCAL_TVT_CALL_SECRET);
    if (male == TRUE) {
        funcs->menuMsg += SECRET_MSG_OTHER_GENDER;
    }
    funcs->msgData = msgData;
}

u16 LocalTvtSecret_GetFacePerson(LocalTvtCallData *data) {
    BOOL male = LocalTvtCallData_IsPlayerMale(data);
    u16 person;

    switch (LocalTvtSecret_GetStageFace(
        LocalTvtSecret_GetStage(LocalTvtCallData_GetCallCount(data, LOCAL_TVT_CALL_SECRET)))) {
    case 0:
    default:
        person = 8;
        break;
    case 1:
        person = 9;
        if (male) {
            person = 10;
        }
        break;
    case 2:
        person = 6;
        if (male) {
            person = 7;
        }
        break;
    case 3:
        person = 11;
        if (male) {
            person = 12;
        }
        break;
    }
    return person;
}

u16 LocalTvtSecret_GetNameMsg(LocalTvtCallData *data) {
    BOOL male = LocalTvtCallData_IsPlayerMale(data);
    u16 messageId;

    if (LocalTvtCallData_IsSecretUnknown(data)) {
        return 8;
    }
    messageId = 9;
    if (male) {
        messageId = 10;
    }
    return messageId;
}

static LocalTvtSecret *LocalTvtSecret_Create(GameData *gameData, BOOL unk, HeapID heapId) {
    LocalTvtSecret *secret = GFL_HeapAllocate(heapId, sizeof(LocalTvtSecret), TRUE, "local_tvt_person_secret.c", 248);

    secret->heapId = heapId;
    secret->gameData = gameData;
    secret->surveyAnswers = func_0200ec38(func_0200ec2c(GameData_GetSaveControl(gameData)));
    secret->unk18 = unk;
    return secret;
}

static void LocalTvtSecret_Free(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    GFL_HeapFree(work);
}

static BOOL LocalTvtSecret_Greet(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtSecret *secret = work;

    return LocalTvtSecret_TalkGreeting(person, setup, secret, &secret->step);
}

static BOOL LocalTvtSecret_Chat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtSecret *secret = work;

    switch (secret->step) {
    case 0:
        if (LocalTvtSecret_TalkChat(person, setup, secret, &secret->talkStep)) {
            secret->step++;
        }
        break;
    case 1:
        if (LocalTvtSecret_TalkBye(person, setup, secret, &secret->talkStep)) {
            secret->step++;
        }
        break;
    case 2:
        secret->step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtSecret_Bye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtSecret *secret = work;

    return LocalTvtSecret_TalkBye(person, setup, secret, &secret->step);
}

static BOOL LocalTvtSecret_TalkGreeting(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, LocalTvtSecret *secret,
                                        int *step) {
    u32 messageId = 0;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (*step) {
    case 0: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        BOOL male = LocalTvtCallData_IsPlayerMale(setup->callData);

        messageId = LocalTvtCallData_GetGreetingMsg(setup->callData, LOCAL_TVT_CALL_SECRET);
        LocalTvtMessage_SetPlayerName(setup->message, LocalTvtCallData_GetGameData(setup->callData));
        if (male == TRUE) {
            messageId += SECRET_MSG_OTHER_GENDER;
        }
        LocalTvtFace_Talk(face, msgData, messageId);
        (*step)++;
        break;
    }
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            (*step)++;
        }
        break;
    case 2:
        *step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtSecret_TalkChat(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, LocalTvtSecret *secret,
                                    int *step) {
    BOOL male = FALSE;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (*step) {
    case 0: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        u32 messageId;

        male = LocalTvtCallData_IsPlayerMale(setup->callData);
        LocalTvtCallData_Reset0200ffcc(setup->callData);
        // The same message again if the player asks again in this call
        if (!secret->chatted) {
            secret->chatMsg = LocalTvtSecret_GetChatMsg(secret->surveyAnswers, setup->callData, setup->message);
        }
        messageId = secret->chatMsg;
        if (male == TRUE) {
            messageId += SECRET_MSG_OTHER_GENDER;
        }
        LocalTvtFace_Talk(face, msgData, messageId);
        secret->chatted = TRUE;
        (*step)++;
        break;
    }
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            (*step)++;
        }
        break;
    case 2:
        *step = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL LocalTvtSecret_TalkBye(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, LocalTvtSecret *secret,
                                   int *step) {
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (*step) {
    case 0: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        BOOL male = LocalTvtCallData_IsPlayerMale(setup->callData);
        u16 messageId = LocalTvtCallData_GetByeMsg(setup->callData, LOCAL_TVT_CALL_SECRET);
        u32 unk10 = LocalTvtSecret_GetStage10(setup->callData);

        if ((unk10 == 1 || unk10 == 2) && secret->chatted) {
            messageId = 245;
        }
        if (male == TRUE) {
            messageId += SECRET_MSG_OTHER_GENDER;
        }
        LocalTvtFace_Talk(face, msgData, messageId);
        (*step)++;
        break;
    }
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            (*step)++;
        }
        break;
    case 2:
        *step = 0;
        // A call counts once the person has chatted
        if (secret->chatted) {
            u32 unk10;

            LocalTvtSecret_GetStage(LocalTvtCallData_GetCallCount(setup->callData, LOCAL_TVT_CALL_SECRET));
            unk10 = LocalTvtSecret_GetStage10(setup->callData);
            if (unk10 != 0) {
                LocalTvtCallData_Set0200ff68(setup->callData, unk10);
                if (unk10 == 1 || unk10 == 2) {
                    LocalTvtCallData_Set0200ff78(setup->callData);
                }
            }
            LocalTvtCallData_AddCall(setup->callData, LOCAL_TVT_CALL_SECRET, SECRET_CALL_MAX);
        }
        func_02017a78(secret->gameData);
        LocalTvtPerson_End(person);
        return TRUE;
    }
    return FALSE;
}

// A call that is over after one message
static BOOL LocalTvtSecret_ShortCall(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtSecret *secret = work;
    u32 messageId = 0;
    LocalTvtFace *face = LocalTvtChara_GetFace(setup->chara, 0);

    switch (secret->step) {
    case 0: {
        MsgData *msgData = LocalTvtMessage_GetTalkMsgData(setup->message);
        BOOL male = LocalTvtCallData_IsPlayerMale(setup->callData);

        messageId = LocalTvtCallData_GetChatMsg(setup->callData, LOCAL_TVT_CALL_SECRET);
        if (male == TRUE) {
            messageId += SECRET_MSG_OTHER_GENDER;
        }
        LocalTvtFace_Talk(face, msgData, messageId);
        secret->step++;
        break;
    }
    case 1:
        if (LocalTvtFace_UpdateTalk(face, setup->chara)) {
            secret->step++;
        }
        break;
    case 2:
        secret->step = 0;
        LocalTvtCallData_AddCall(setup->callData, LOCAL_TVT_CALL_SECRET, SECRET_CALL_MAX);
        LocalTvtPerson_End(person);
        return TRUE;
    }
    return FALSE;
}

// A call without a menu: the greeting, the chat and the goodbye
static BOOL LocalTvtSecret_WholeCall(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work) {
    LocalTvtSecret *secret = work;

    switch (secret->step) {
    case 0:
        if (LocalTvtSecret_TalkGreeting(person, setup, secret, &secret->talkStep)) {
            secret->step++;
        }
        break;
    case 1:
        if (LocalTvtSecret_TalkChat(person, setup, secret, &secret->talkStep)) {
            secret->step++;
        }
        break;
    case 2:
        if (LocalTvtSecret_TalkBye(person, setup, secret, &secret->talkStep)) {
            secret->step++;
        }
        break;
    case 3:
        secret->step = 0;
        return TRUE;
    }
    return FALSE;
}

static u32 LocalTvtSecret_GetStage10(LocalTvtCallData *data) {
    u32 unk10 = LocalTvtSecret_GetStage10Of(
        LocalTvtSecret_GetStage(LocalTvtCallData_GetCallCount(data, LOCAL_TVT_CALL_SECRET)));

    if (LocalTvtCallData_Get0200ff8c(data)) {
        unk10 = 0;
    }
    return unk10;
}

static u32 LocalTvtSecret_GetChatMsg(u8 *surveyAnswers, LocalTvtCallData *data, LocalTvtMessage *message) {
    u32 callCount;
    u32 callType;
    u32 unk10;
    BOOL answered;
    BOOL asked;
    u32 answerMsg;

    LocalTvtCallData_IsPlayerMale(data);
    callCount = LocalTvtCallData_GetCallCount(data, LOCAL_TVT_CALL_SECRET);
    callType = LocalTvtSecret_GetStageCallType(LocalTvtSecret_GetStage(callCount));
    unk10 = LocalTvtSecret_GetStage10(data);
    LocalTvtSecret_GetSurveyAnswer(surveyAnswers, callCount, &answerMsg, &answered, &asked);
    LocalTvtSecret_SetWords(data, message, callCount, answerMsg, answered);
    if (unk10 == 2) {
        return 244;
    }
    switch (callType) {
    case SECRET_CALL_TOPICS:
    case SECRET_CALL_SHORT:
    case SECRET_CALL_4:
    default:
        return LocalTvtCallData_GetChatMsg(data, LOCAL_TVT_CALL_SECRET);
    case SECRET_CALL_SURVEY:
        if (asked) {
            return LocalTvtSecret_GetSurveyChatMsg(data, LOCAL_TVT_CALL_SECRET, answered);
        }
        return LocalTvtCallData_GetChatMsg(data, LOCAL_TVT_CALL_SECRET);
    case SECRET_CALL_RANDOM:
        return LocalTvtSecret_GetRandomChatMsg(data, callCount);
    }
}

static void LocalTvtSecret_SetWords(LocalTvtCallData *data, LocalTvtMessage *message, u32 callCount, u32 answerMsg,
                                    BOOL answered) {
    GameData *gameData = LocalTvtCallData_GetGameData(data);

    LocalTvtMessage_SetPlayerName(message, gameData);
    if (callCount == SECRET_CALL_BADGES) {
        LocalTvtMessage_SetNumber(message, LocalTvtCallData_GetBadgeCount(data), 1, 1);
    } else if (callCount == SECRET_CALL_STARTER) {
        u16 *starter = EventWork_GetWkPtr(GameData_GetEventWork(gameData), EVENT_WORK_0x4030);
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TYPE_NAMES, HEAPID_TAIL(HEAPID_LOCAL_TVT));

        LocalTvtMessage_SetWord(message, msgData, sLocalTvtStarterTypes[*starter], 1);
        GFL_MsgDataFree(msgData);
    } else if (callCount < SECRET_SURVEY_CALLS && answered) {
        LocalTvtMessage_SetSurveyAnswer(message, 1, answerMsg);
    }
}

// The message after the one of the question if the player hasn't answered it
static u16 LocalTvtSecret_GetSurveyChatMsg(LocalTvtCallData *data, u16 person, BOOL answered) {
    u16 messageId = LocalTvtCallData_GetChatMsg(data, LOCAL_TVT_CALL_SECRET);

    if (!answered) {
        messageId++;
    }
    return messageId;
}

static u32 LocalTvtSecret_GetRandomChatMsg(LocalTvtCallData *data, u32 callCount) {
    if (GFL_RandomLC(2) != 0) {
        return GFL_RandomLC(9) + 223;
    }
    return GFL_RandomLC(9) + 233;
}

static void LocalTvtSecret_GetSurveyAnswer(u8 *surveyAnswers, u8 callCount, u32 *answerMsg, BOOL *answered,
                                           BOOL *asked) {
    u32 question;
    u32 answer;

    if (callCount >= SECRET_SURVEY_CALLS) {
        *asked = FALSE;
        return;
    }
    question = sLocalTvtSecretQuestions[callCount];
    answer = func_0200ec3c(surveyAnswers, question);
    *asked = TRUE;
    if (answer == 0) {
        *answerMsg = 0;
        *answered = FALSE;
        return;
    }
    *answerMsg = GetSurveyAnswerMsgID(question, answer - 1);
    *answered = TRUE;
}

u32 LocalTvtSecret_GetRingCount(LocalTvtCallData *data) {
    return LocalTvtSecret_GetStageRingCount(
        LocalTvtSecret_GetStage(LocalTvtCallData_GetCallCount(data, LOCAL_TVT_CALL_SECRET)));
}

u32 LocalTvtSecret_GetStage(int callCount) {
    u32 i;

    if (callCount >= SECRET_CALL_MAX) {
        return NELEMS(sLocalTvtSecretStages) - 1;
    }
    for (i = 0; i < NELEMS(sLocalTvtSecretStages); i++) {
        const LocalTvtSecretStage *stage = &sLocalTvtSecretStages[i];

        if (stage->callCountMin <= callCount && callCount <= stage->callCountMax) {
            return i;
        }
    }
    return 0;
}

static u32 LocalTvtSecret_GetStageCallType(u32 stage) {
    return sLocalTvtSecretStages[stage].callType;
}

static u32 LocalTvtSecret_GetStageFace(u32 stage) {
    return sLocalTvtSecretStages[stage].face;
}

static u32 LocalTvtSecret_GetStage10Of(u32 stage) {
    return sLocalTvtSecretStages[stage].unk10;
}

u32 LocalTvtSecret_GetStage0378(u32 stage) {
    return sLocalTvtSecretStages[stage].unk14;
}

static u32 LocalTvtSecret_GetStageRingCount(u32 stage) {
    return sLocalTvtSecretStages[stage].ringCount;
}

u32 LocalTvtSecret_GetStageCount(u32 stage) {
    return NELEMS(sLocalTvtSecretStages);
}
