#include "types.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_msgbg.h"
#include "field/field_script.h"
#include "field/survey.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/rtc_cache.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/wordset.h"

static GameEventReturnCode SurveyTextEvent_Run(GameEvent *event, u32 *state, void *data);

// The options a survey can pick, with the score each needs and its probability in percent
// clang-format off
static const SurveyProbabilityEntry sProbabilities[0xe0] = {
    { 1, 48, 25, 4 }, { 1, 48, 25, 3 }, { 1, 38, 25, 4 }, { 1, 38, 25, 3 },
    { 1, 32, 25, 4 }, { 1, 32, 25, 3 }, { 1, 24, 25, 4 }, { 1, 24, 25, 3 },
    { 1, 17, 33, 4 }, { 1, 17, 33, 3 }, { 1, 14, 33, 4 }, { 1, 14, 33, 3 },
    { 1, 11, 33, 4 }, { 1, 11, 33, 3 }, { 1, 8, 50, 4 }, { 1, 8, 50, 3 },
    { 1, 5, 50, 4 }, { 1, 5, 50, 3 }, { 1, 2, 50, 4 }, { 1, 2, 50, 3 },
    { 1, 0, 50, 4 }, { 1, 0, 50, 3 }, { 1, 0, 50, 4 }, { 1, 0, 50, 3 },
    { 1, 0, 100, 4 }, { 1, 0, 100, 3 }, { 2, 50, 25, 4 }, { 2, 50, 25, 3 },
    { 2, 40, 25, 4 }, { 2, 40, 25, 3 }, { 2, 34, 25, 4 }, { 2, 34, 25, 3 },
    { 2, 25, 30, 4 }, { 2, 25, 30, 3 }, { 2, 19, 30, 4 }, { 2, 19, 30, 3 },
    { 2, 16, 30, 4 }, { 2, 16, 30, 3 }, { 2, 13, 30, 4 }, { 2, 13, 30, 3 },
    { 2, 10, 50, 4 }, { 2, 10, 50, 3 }, { 2, 7, 50, 4 }, { 2, 7, 50, 3 },
    { 2, 4, 50, 4 }, { 2, 4, 50, 3 }, { 2, 0, 30, 4 }, { 2, 0, 30, 3 },
    { 2, 0, 50, 4 }, { 2, 0, 50, 3 }, { 2, 0, 100, 4 }, { 2, 0, 100, 3 },
    { 0, 42, 20, 4 }, { 0, 42, 20, 3 }, { 0, 34, 20, 4 }, { 0, 34, 20, 3 },
    { 0, 26, 25, 4 }, { 0, 26, 25, 3 }, { 0, 18, 25, 4 }, { 0, 18, 25, 3 },
    { 0, 10, 33, 4 }, { 0, 10, 33, 3 }, { 0, 5, 33, 4 }, { 0, 5, 33, 3 },
    { 0, 0, 50, 4 }, { 0, 0, 50, 3 }, { 0, 0, 100, 4 }, { 0, 0, 100, 3 },
    { 5, 25, 33, 4 }, { 5, 25, 33, 3 }, { 5, 15, 50, 4 }, { 5, 15, 50, 3 },
    { 5, 5, 50, 4 }, { 5, 5, 50, 3 }, { 5, 0, 100, 4 }, { 5, 0, 100, 3 },
    { 4, 48, 15, 4 }, { 4, 48, 15, 3 }, { 4, 48, 20, 4 }, { 4, 48, 20, 3 },
    { 4, 48, 25, 4 }, { 4, 48, 25, 3 }, { 4, 43, 15, 4 }, { 4, 43, 15, 3 },
    { 4, 43, 20, 4 }, { 4, 43, 20, 3 }, { 4, 43, 25, 4 }, { 4, 43, 25, 3 },
    { 4, 38, 15, 4 }, { 4, 38, 15, 3 }, { 4, 38, 20, 4 }, { 4, 38, 20, 3 },
    { 4, 38, 25, 4 }, { 4, 38, 25, 3 }, { 4, 33, 15, 4 }, { 4, 33, 15, 3 },
    { 4, 33, 20, 4 }, { 4, 33, 20, 3 }, { 4, 33, 25, 4 }, { 4, 33, 25, 3 },
    { 4, 28, 15, 4 }, { 4, 28, 15, 3 }, { 4, 28, 20, 4 }, { 4, 28, 20, 3 },
    { 4, 28, 25, 4 }, { 4, 28, 25, 3 }, { 4, 23, 20, 4 }, { 4, 23, 20, 3 },
    { 4, 23, 25, 4 }, { 4, 23, 25, 3 }, { 4, 23, 30, 4 }, { 4, 23, 30, 3 },
    { 4, 18, 20, 4 }, { 4, 18, 20, 3 }, { 4, 18, 25, 4 }, { 4, 18, 25, 3 },
    { 4, 18, 30, 4 }, { 4, 18, 30, 3 }, { 4, 13, 20, 4 }, { 4, 13, 20, 3 },
    { 4, 13, 25, 4 }, { 4, 13, 25, 3 }, { 4, 13, 30, 4 }, { 4, 13, 30, 3 },
    { 4, 8, 20, 4 }, { 4, 8, 20, 3 }, { 4, 8, 25, 4 }, { 4, 8, 25, 3 },
    { 4, 8, 30, 4 }, { 4, 8, 30, 3 }, { 4, 4, 20, 4 }, { 4, 4, 20, 3 },
    { 4, 4, 25, 4 }, { 4, 4, 25, 3 }, { 4, 4, 30, 4 }, { 4, 4, 30, 3 },
    { 4, 4, 50, 4 }, { 4, 4, 50, 3 }, { 4, 0, 20, 4 }, { 4, 0, 20, 3 },
    { 4, 0, 25, 4 }, { 4, 0, 25, 3 }, { 4, 0, 33, 4 }, { 4, 0, 33, 3 },
    { 4, 0, 50, 4 }, { 4, 0, 50, 3 }, { 4, 0, 100, 4 }, { 4, 0, 100, 3 },
    { 3, 54, 15, 4 }, { 3, 54, 15, 3 }, { 3, 54, 20, 4 }, { 3, 54, 20, 3 },
    { 3, 54, 25, 4 }, { 3, 54, 25, 3 }, { 3, 54, 30, 4 }, { 3, 54, 30, 3 },
    { 3, 49, 15, 4 }, { 3, 49, 15, 3 }, { 3, 49, 20, 4 }, { 3, 49, 20, 3 },
    { 3, 49, 33, 4 }, { 3, 49, 33, 3 }, { 3, 44, 15, 4 }, { 3, 44, 15, 3 },
    { 3, 44, 20, 4 }, { 3, 44, 20, 3 }, { 3, 44, 33, 4 }, { 3, 44, 33, 3 },
    { 3, 39, 15, 4 }, { 3, 39, 15, 3 }, { 3, 39, 20, 4 }, { 3, 39, 20, 3 },
    { 3, 39, 33, 4 }, { 3, 39, 33, 3 }, { 3, 34, 15, 4 }, { 3, 34, 15, 3 },
    { 3, 34, 20, 4 }, { 3, 34, 20, 3 }, { 3, 34, 33, 4 }, { 3, 34, 33, 3 },
    { 3, 29, 20, 4 }, { 3, 29, 20, 3 }, { 3, 29, 30, 4 }, { 3, 29, 30, 3 },
    { 3, 29, 50, 4 }, { 3, 29, 50, 3 }, { 3, 24, 20, 4 }, { 3, 24, 20, 3 },
    { 3, 24, 30, 4 }, { 3, 24, 30, 3 }, { 3, 24, 50, 4 }, { 3, 24, 50, 3 },
    { 3, 19, 20, 4 }, { 3, 19, 20, 3 }, { 3, 19, 30, 4 }, { 3, 19, 30, 3 },
    { 3, 19, 50, 4 }, { 3, 19, 50, 3 }, { 3, 14, 20, 4 }, { 3, 14, 20, 3 },
    { 3, 14, 30, 4 }, { 3, 14, 30, 3 }, { 3, 14, 50, 4 }, { 3, 14, 50, 3 },
    { 3, 8, 20, 4 }, { 3, 8, 20, 3 }, { 3, 8, 30, 4 }, { 3, 8, 30, 3 },
    { 3, 8, 50, 4 }, { 3, 8, 50, 3 }, { 3, 3, 20, 4 }, { 3, 3, 20, 3 },
    { 3, 3, 30, 4 }, { 3, 3, 30, 3 }, { 3, 3, 50, 4 }, { 3, 3, 50, 3 },
    { 3, 0, 25, 4 }, { 3, 0, 25, 3 }, { 3, 0, 30, 4 }, { 3, 0, 30, 3 },
    { 3, 0, 50, 4 }, { 3, 0, 50, 3 }, { 3, 0, 100, 4 }, { 3, 0, 100, 3 },
};

// clang-format on

void probabilityLoop(SurveyProbabilityState *state) {
    int i;
    for (i = 0; i < 6; i++) {
        state->probability[i] = 0;
        insideProbabilityLoop(state, i);
    }
}

void insideProbabilityLoop(SurveyProbabilityState *state, u32 selection) {
    s32 score = state->score + state->bonus;
    u32 i;
    for (i = 0; i < NELEMS(sProbabilities); i++) {
        const SurveyProbabilityEntry *entry = &sProbabilities[i];
        if (selection == entry->selection && entry->requiredScore <= score && (entry->flags & 4)) {
            u32 probability = entry->probability;
            if (probability == 100 || probability >= GFL_RandomLC(101)) {
                state->selection[selection] = i;
                return;
            }
        }
    }
}

BOOL Scrcmd_Survey_SurveyGetRank(VM *vm, FieldScriptEnv *env) {
    TrainerGameInfoSave *info;
    u16 *out;

    FieldScriptEnv_GetScriptWork(env);
    info = getTrainerGameInfoAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    out = ScriptReadVar(vm, env);
    *out = TrainerGameInfo_GetSurveyRank(info);
    return FALSE;
}

BOOL Scrcmd_Survey_SurveyRaiseRank(VM *vm, FieldScriptEnv *env) {
    TrainerGameInfoSave *info;
    int count;

    FieldScriptEnv_GetScriptWork(env);
    info = getTrainerGameInfoAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    count = TrainerGameInfo_GetSurveyRank(info);
    if (count >= 5) {
        return FALSE;
    }
    TrainerGameInfo_SetSurveyRank(info, count + 1);
    GameBeaconSys_SetSurveyRank((u8)(count + 1));
    return FALSE;
}

BOOL Scrcmd_Survey_SurveyGetCurrentQuestionID(VM *vm, FieldScriptEnv *env) {
    TrainerGameInfoSave *info;
    u16 *out;

    FieldScriptEnv_GetScriptWork(env);
    info = getTrainerGameInfoAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    out = ScriptReadVar(vm, env);
    *out = TrainerGameInfo_GetSurveyQuestion(info);
    return FALSE;
}

BOOL Scrcmd_Survey_SurveyGetCurrentAnswerIDs(VM *vm, FieldScriptEnv *env) {
    TrainerGameInfoSave *info;
    u16 *first;
    u16 *second;
    u16 *third;

    FieldScriptEnv_GetScriptWork(env);
    info = getTrainerGameInfoAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    first = ScriptReadVar(vm, env);
    second = ScriptReadVar(vm, env);
    third = ScriptReadVar(vm, env);
    *first = TrainerGameInfo_GetSurveyAnswer(info, 0);
    *second = TrainerGameInfo_GetSurveyAnswer(info, 1);
    *third = TrainerGameInfo_GetSurveyAnswer(info, 2);
    return FALSE;
}

BOOL Scrcmd_Survey_SurveyGetTime(VM *vm, FieldScriptEnv *env) {
    SaveControl *save;
    u16 *out;

    FieldScriptEnv_GetScriptWork(env);
    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    out = ScriptReadVar(vm, env);
    *out = detectLengthSinceLastSession(save);
    return FALSE;
}

BOOL Scrcmd_Survey_SurveyStart(VM *vm, FieldScriptEnv *env) {
    SaveControl *save;
    TrainerGameInfoSave *info;
    u16 values[3];
    u16 first;
    int i;

    FieldScriptEnv_GetScriptWork(env);
    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    info = getTrainerGameInfoAddress(save);
    SurveySave_Get(save);
    first = ScriptReadAny(vm, env);
    values[0] = ScriptReadAny(vm, env);
    values[1] = ScriptReadAny(vm, env);
    values[2] = ScriptReadAny(vm, env);
    TrainerGameInfo_SetSurveyQuestion(info, first);
    for (i = 0; i < 3; i++) {
        TrainerGameInfo_SetSurveyAnswer(info, i, values[i]);
        TrainerGameInfo_SetSurveyCount(info, i, 0);
    }
    // The original call passes the value already in r0 without setting up a new argument.
    setSecondsCurrentTimeInTrainerCard(info, RTC_ConvertSecondsCached());
    return FALSE;
}

BOOL Scrcmd_Survey_SurveyClear(VM *vm, FieldScriptEnv *env) {
    SaveControl *save;
    TrainerGameInfoSave *info;
    int i;

    FieldScriptEnv_GetScriptWork(env);
    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    info = getTrainerGameInfoAddress(save);
    SurveySave_Get(save);
    TrainerGameInfo_SetSurveyQuestion(info, 0);
    for (i = 0; i < 3; i++) {
        TrainerGameInfo_SetSurveyAnswer(info, i, 0xff);
    }
    GameBeaconSys_ClearNotice(0);
    return FALSE;
}

BOOL Scrcmd_Survey_SurveyGetPopularOptionMsgID(VM *vm, FieldScriptEnv *env) {
    int i;
    u16 question;
    u16 *out;
    int best;
    u16 msgId;
    SaveControl *save;
    SurveySave *survey;
    int count;
    int total;

    FieldScriptEnv_GetScriptWork(env);
    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    survey = SurveySave_Get(save);
    question = ScriptReadAny(vm, env);
    out = ScriptReadVar(vm, env);
    count = GetSurveyAnswerMsgIDCount(question);
    best = -1;
    for (i = 0; i < count; i++) {
        total = SurveySave_GetTotalAnswerCount(survey, question, i + 1);
        total += SurveySave_GetTodayAnswerCount(survey, question, i + 1);
        if (best < total) {
            best = total;
            msgId = GetSurveyAnswerMsgID(question, i);
        }
    }
    *out = msgId;
    return FALSE;
}

BOOL Scrcmd_Survey_SurveyWriteOwnAnswer(VM *vm, FieldScriptEnv *env) {
    u8 *answers;
    u32 question;
    u32 answer;

    FieldScriptEnv_GetScriptWork(env);
    answers = SurveySave_GetOwnAnswers(SurveySave_Get(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env))));
    question = ScriptReadAny(vm, env);
    answer = ScriptReadAny(vm, env);
    SurveySave_WriteAnswer(answers, question, answer);
    GameBeaconSys_SetSurveyAnswers(answers);
    return FALSE;
}

BOOL Scrcmd_Survey_SurveyShowRankMessage(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameEvent *event;

    work = FieldScriptEnv_GetScriptWork(env);
    event = SurveyTextEvent_Create(FieldScriptEnv_GetGameSystem(env));
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Survey_SurveyGetQuestionState(VM *vm, FieldScriptEnv *env) {
    SaveControl *save;
    TrainerCardSave *card;
    u8 index;
    u16 *out;

    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    card = getTrainerCardData_wrapper(save);
    index = ScriptReadAny(vm, env);
    out = ScriptReadVar(vm, env);
    if (index == TrainerGameInfo_GetSurveyQuestion(card)) {
        *out = Survey_GetCurrentState(save);
    } else {
        *out = Survey_GetQuestionState(save, index);
    }
    return FALSE;
}

BOOL Scrcmd_Survey_SurveyGetCurrentRemaining(VM *vm, FieldScriptEnv *env) {
    SaveControl *save;
    TrainerCardSave *card;
    u16 *out;
    u8 kind;
    u16 result;

    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    card = getTrainerCardData_wrapper(save);
    out = ScriptReadVar(vm, env);
    kind = Survey_GetQuestionType(TrainerGameInfo_GetSurveyQuestion(card));
    switch (kind) {
    case 1:
        result = Survey_GetCurrentHoursLeft(save);
        break;
    case 0:
        result = Survey_GetCurrentAnswersLeft(save);
        break;
    }
    *out = result;
    return FALSE;
}

u16 Survey_GetCurrentAnswersLeft(SaveControl *save) {
    SurveySave *survey;
    u16 index;
    u32 maximum;
    u8 count;
    u8 answers[4];
    s32 i;
    u32 minimum;
    u32 value;

    survey = SurveySave_Get(save);
    index = TrainerGameInfo_GetSurveyQuestion(getTrainerCardData_wrapper(save));
    maximum = Survey_GetQuestionThreshold(index);
    count = (u8)Survey_GetQuestionAnswerCount(index);
    Survey_GetQuestionAnswers(index, answers);
    minimum = 0xf694e;
    for (i = 0; i < count; i++) {
        value = Survey_GetAnswerCount(survey, answers[i]);
        if (value < minimum)
            minimum = value;
    }
    return maximum - minimum;
}

u16 Survey_GetCurrentHoursLeft(SaveControl *save) {
    u16 index;
    u16 length;
    s32 result;

    SurveySave_Get(save);
    index = TrainerGameInfo_GetSurveyQuestion(getTrainerCardData_wrapper(save));
    length = detectLengthSinceLastSession(save);
    result = Survey_GetQuestionHours((u8)index) - length;
    if (result < 0)
        result = 0;
    return result;
}

static GameEventReturnCode SurveyTextEvent_Run(GameEvent *event, u32 *state, void *data) {
    SurveyTextWork *work = data;
    switch (*state) {
    case 0:
        getSurveyText(work);
        SurveyTextWork_CreateWindow(work);
        SurveyTextWork_CreateWordSet(work);
        SurveyTextWork_PrintRankMessage(work);
        (*state)++;
        break;
    case 1:
        if (MsgBGPrinter_IsMsgWinTransDone(work->window) == 1)
            (*state)++;
        break;
    case 2:
        if (GCTX_HIDGetPressedKeys() & 0xf3)
            (*state)++;
        break;
    case 3:
        SurveyTextWork_ClearWindow(work);
        SurveyTextWork_FreeWordSet(work);
        SurveyTextWork_DeleteWindow(work);
        SurveyTextWork_FreeText(work);
        return TRUE;
    }
    return FALSE;
}

GameEvent *SurveyTextEvent_Create(GameSystem *gsys) {
    GameEvent *event;
    SurveyTextWork *work;
    event = GameEvent_Create(gsys, NULL, SurveyTextEvent_Run, 0x20);
    work = GameEvent_GetData(event);
    SurveyTextWork_Init(work, gsys);
    return event;
}

void SurveyTextWork_Init(SurveyTextWork *work, GameSystem *gsys) {
    Field *field;
    field = GSYS_GetField(gsys);
    MI_CpuFill8(work, 0, 0x20);
    work->heapId = Field_GetHeapID(field);
    work->gameSystem = gsys;
    work->gameData = GSYS_GetGameData(gsys);
    work->field = field;
    work->msgBGSys = Field_GetMsgBGSys(field);
}

void getSurveyText(SurveyTextWork *work) {
    work->message = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_CASTELIA_CITY_13, work->heapId);
}

void SurveyTextWork_FreeText(SurveyTextWork *work) {
    GFL_MsgDataFree(work->message);
    work->message = NULL;
}

void SurveyTextWork_CreateWindow(SurveyTextWork *work) {
    u32 first;
    u32 second;
    u32 third;
    u32 mode = SurveyTextWork_GetRank(work);

    switch (mode) {
    case 0:
        first = 1;
        second = 30;
        third = 4;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        first = 1;
        second = 30;
        third = 8;
        break;
    }
    work->window = FieldMsgBG_CreateMoneyWin(work->msgBGSys, work->message, first, first, second, third);
}

void SurveyTextWork_DeleteWindow(SurveyTextWork *work) {
    MsgBGPrinter_DeleteMsgWin(work->window);
    work->window = NULL;
}

void SurveyTextWork_CreateWordSet(SurveyTextWork *work) {
    work->wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
}

void SurveyTextWork_FreeWordSet(SurveyTextWork *work) {
    GFL_WordSetSystemFree(work->wordSet);
    work->wordSet = NULL;
}

void SurveyTextWork_PrintRankMessage(SurveyTextWork *work) {
    StrBuf *source;
    StrBuf *formatted;
    u32 kind;
    u32 num1;
    u32 num2;
    u32 msgId;
    PlayerInfo *player;
    BmpWin *window;

    source = GFL_StrBufCreate(256, work->heapId);
    formatted = GFL_StrBufCreate(256, work->heapId);
    kind = SurveyTextWork_GetRank(work);
    num1 = SurveyTextWork_GetReportNumberA(work);
    num2 = SurveyTextWork_GetReportNumberB(work);
    msgId = GetTrainerCardTextMSGID(kind);
    player = GetGameDataPlayerInfo(work->gameData);
    copyVarForText(work->wordSet, 0, player);
    WordSetNumber(work->wordSet, 1, num1, 5, 1, 1);
    WordSetNumber(work->wordSet, 2, num2, 5, 1, 1);
    GFL_MsgDataLoadStrbuf(work->message, msgId, source);
    GFL_WordSetFormatStrbuf(work->wordSet, formatted, source);
    GFL_WordSetClearAll(work->wordSet);
    MsgBGPrinter_MsgWinPrintStr(work->window, 8, 0, formatted);
    window = MsgBGPrinter_GetMsgWinBmpWin(work->window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    GFL_StrBufFree(source);
    GFL_StrBufFree(formatted);
}

void SurveyTextWork_ClearWindow(SurveyTextWork *work) {
    MsgBGPrinter_ClearMsgWin(work->window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(MsgBGPrinter_GetMsgWinBmpWin(work->window)));
}

u32 SurveyTextWork_GetRank(SurveyTextWork *work) {
    return TrainerGameInfo_GetSurveyRank(getTrainerCardData_wrapper(GameData_GetSaveControl_(work->gameData)));
}

u32 SurveyTextWork_GetReportNumberA(SurveyTextWork *work) {
    return TrainerCard_GetPasserbyCount(getTrainerCardData_wrapper(GameData_GetSaveControl_(work->gameData)));
}

u32 SurveyTextWork_GetReportNumberB(SurveyTextWork *work) {
    return TrainerCard_GetThankedCount(getTrainerCardData_wrapper(GameData_GetSaveControl_(work->gameData)));
}

u32 GetTrainerCardTextMSGID(u32 type) {
    switch (type) {
    case 0:
        return 0x179;
    case 1:
        return 0x17a;
    case 2:
        return 0x17b;
    case 3:
        return 0x17c;
    case 4:
        return 0x17d;
    case 5:
        return 0x17e;
    }
    return 0;
}
