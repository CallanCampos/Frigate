#include "types.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "field/festival.h"
#include "field/field.h"
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

static GameEventReturnCode func_ov027_021707e8(GameEvent *event, u32 *state, void *data);

// The options a survey can pick, with the score each needs and its probability in percent
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

BOOL func_ov027_021703a8(VM *vm, FieldScriptEnv *env) {
    TrainerGameInfoSave *info;
    u16 *out;

    FieldScriptEnv_GetScriptWork(env);
    info = getTrainerGameInfoAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    out = ScriptReadVar(vm, env);
    *out = func_0200c96c(info);
    return FALSE;
}

BOOL func_ov027_021703dc(VM *vm, FieldScriptEnv *env) {
    TrainerGameInfoSave *info;
    int count;

    FieldScriptEnv_GetScriptWork(env);
    info = getTrainerGameInfoAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    count = func_0200c96c(info);
    if (count >= 5) {
        return FALSE;
    }
    func_0200c974(info, count + 1);
    GameBeaconSys_SetSurveyRank((u8)(count + 1));
    return FALSE;
}

BOOL s01FF_SurveyGetCurrentQuestionID(VM *vm, FieldScriptEnv *env) {
    TrainerGameInfoSave *info;
    u16 *out;

    FieldScriptEnv_GetScriptWork(env);
    info = getTrainerGameInfoAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    out = ScriptReadVar(vm, env);
    *out = func_0200ca7c(info);
    return FALSE;
}

BOOL s0200_SurveyGetCurrentAnswerIDs(VM *vm, FieldScriptEnv *env) {
    TrainerGameInfoSave *info;
    u16 *first;
    u16 *second;
    u16 *third;

    FieldScriptEnv_GetScriptWork(env);
    info = getTrainerGameInfoAddress(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env)));
    first = ScriptReadVar(vm, env);
    second = ScriptReadVar(vm, env);
    third = ScriptReadVar(vm, env);
    *first = func_0200ca8c(info, 0);
    *second = func_0200ca8c(info, 1);
    *third = func_0200ca8c(info, 2);
    return FALSE;
}

BOOL s0204_SurveyGetTime(VM *vm, FieldScriptEnv *env) {
    SaveControl *save;
    u16 *out;

    FieldScriptEnv_GetScriptWork(env);
    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    out = ScriptReadVar(vm, env);
    *out = detectLengthSinceLastSession(save);
    return FALSE;
}

BOOL func_ov027_021704e0(VM *vm, FieldScriptEnv *env) {
    SaveControl *save;
    TrainerGameInfoSave *info;
    u16 values[3];
    u16 first;
    int i;

    FieldScriptEnv_GetScriptWork(env);
    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    info = getTrainerGameInfoAddress(save);
    func_0200ec2c(save);
    first = ScriptReadAny(vm, env);
    values[0] = ScriptReadAny(vm, env);
    values[1] = ScriptReadAny(vm, env);
    values[2] = ScriptReadAny(vm, env);
    func_0200ca84(info, first);
    for (i = 0; i < 3; i++) {
        func_0200ca94(info, i, values[i]);
        func_0200caa8(info, i, 0);
    }
    // The original call passes the value already in r0 without setting up a new argument.
    setSecondsCurrentTimeInTrainerCard(info, RTC_ConvertSecondsCached());
    return FALSE;
}

BOOL func_ov027_02170580(VM *vm, FieldScriptEnv *env) {
    SaveControl *save;
    TrainerGameInfoSave *info;
    int i;

    FieldScriptEnv_GetScriptWork(env);
    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    info = getTrainerGameInfoAddress(save);
    func_0200ec2c(save);
    func_0200ca84(info, 0);
    for (i = 0; i < 3; i++) {
        func_0200ca94(info, i, 0xff);
    }
    GameBeaconSys_ClearNotice(0);
    return FALSE;
}

BOOL s0201_SurveyGetPopularOptionMsgID(VM *vm, FieldScriptEnv *env) {
    int i;
    u16 question;
    u16 *out;
    int best;
    u16 msgId;
    SaveControl *save;
    void *survey;
    int count;
    int total;

    FieldScriptEnv_GetScriptWork(env);
    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    survey = func_0200ec2c(save);
    question = ScriptReadAny(vm, env);
    out = ScriptReadVar(vm, env);
    count = GetSurveyAnswerMsgIDCount(question);
    best = -1;
    for (i = 0; i < count; i++) {
        total = func_0200ed90(survey, question, i + 1);
        total += func_0200ed48(survey, question, i + 1);
        if (best < total) {
            best = total;
            msgId = GetSurveyAnswerMsgID(question, i);
        }
    }
    *out = msgId;
    return FALSE;
}

BOOL func_ov027_02170650(VM *vm, FieldScriptEnv *env) {
    void *survey;
    u32 question;
    u32 answer;

    FieldScriptEnv_GetScriptWork(env);
    survey = func_0200ec38(func_0200ec2c(GameData_GetSaveControl(FieldScriptEnv_GetGameData(env))));
    question = ScriptReadAny(vm, env);
    answer = ScriptReadAny(vm, env);
    func_0200ec80(survey, question, answer);
    GameBeaconSys_SetSurveyAnswers(survey);
    return FALSE;
}

BOOL func_ov027_02170698(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameEvent *event;

    work = FieldScriptEnv_GetScriptWork(env);
    event = func_ov027_02170860(FieldScriptEnv_GetGameSystem(env));
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov027_021706bc(VM *vm, FieldScriptEnv *env) {
    SaveControl *save;
    TrainerCardSave *card;
    u8 index;
    u16 *out;

    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    card = getTrainerCardData_wrapper(save);
    index = ScriptReadAny(vm, env);
    out = ScriptReadVar(vm, env);
    if (index == func_0200ca7c(card)) {
        *out = func_ov012_02165330(save);
    } else {
        *out = func_ov012_021653d8(save, index);
    }
    return FALSE;
}

BOOL func_ov027_0217070c(VM *vm, FieldScriptEnv *env) {
    SaveControl *save;
    TrainerCardSave *card;
    u16 *out;
    u8 kind;
    u16 result;

    save = GameData_GetSaveControl(FieldScriptEnv_GetGameData(env));
    card = getTrainerCardData_wrapper(save);
    out = ScriptReadVar(vm, env);
    kind = func_ov012_021652cc(func_0200ca7c(card));
    switch (kind) {
    case 1:
        result = func_ov027_021707b8(save);
        break;
    case 0:
        result = func_ov027_02170758(save);
        break;
    }
    *out = result;
    return FALSE;
}


u16 func_ov027_02170758(SaveControl *save) {
    void *survey;
    u16 index;
    u32 maximum;
    u8 count;
    u8 answers[4];
    s32 i;
    u32 minimum;
    u32 value;

    survey = func_0200ec2c(save);
    index = func_0200ca7c(getTrainerCardData_wrapper(save));
    maximum = func_ov012_02165310(index);
    count = (u8)func_ov012_021652dc(index);
    func_ov012_021652ec(index, answers);
    minimum = 0xf694e;
    for (i = 0; i < count; i++) {
        value = func_ov012_02165480(survey, answers[i]);
        if (value < minimum) minimum = value;
    }
    return maximum - minimum;
}

u16 func_ov027_021707b8(SaveControl *save) {
    u16 index;
    u16 length;
    s32 result;

    func_0200ec2c(save);
    index = func_0200ca7c(getTrainerCardData_wrapper(save));
    length = detectLengthSinceLastSession(save);
    result = func_ov012_02165320((u8)index) - length;
    if (result < 0) result = 0;
    return result;
}

static GameEventReturnCode func_ov027_021707e8(GameEvent *event, u32 *state, void *data) {
    SurveyTextWork *work = data;
    switch (*state) {
    case 0:
        getSurveyText(work);
        func_ov027_021708e0(work);
        func_ov027_02170944(work);
        func_ov027_02170964(work);
        (*state)++;
        break;
    case 1:
        if (func_ov036_02187c70(work->window) == 1) (*state)++;
        break;
    case 2:
        if (GCTX_HIDGetPressedKeys() & 0xf3) (*state)++;
        break;
    case 3:
        func_ov027_02170a1c(work);
        func_ov027_02170954(work);
        func_ov027_02170934(work);
        func_ov027_021708d0(work);
        return TRUE;
    }
    return FALSE;
}

GameEvent *func_ov027_02170860(GameSystem *gsys) {
    GameEvent *event;
    SurveyTextWork *work;
    event = GameEvent_Create(gsys, NULL, func_ov027_021707e8, 0x20);
    work = GameEvent_GetData(event);
    func_ov027_02170884(work, gsys);
    return event;
}

void func_ov027_02170884(SurveyTextWork *work, GameSystem *gsys) {
    Field *field;
    field = GSYS_GetField(gsys);
    sys_memset(work, 0, 0x20);
    work->heapId = Field_GetHeapID(field);
    work->gameSystem = gsys;
    work->gameData = GSYS_GetGameData(gsys);
    work->field = field;
    work->msgBGSys = Field_GetMsgBGSys(field);
}

void getSurveyText(SurveyTextWork *work) {
    work->message = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_CASTELIA_CITY_13, work->heapId);
}

void func_ov027_021708d0(SurveyTextWork *work) {
    GFL_MsgDataFree(work->message);
    work->message = NULL;
}

void func_ov027_021708e0(SurveyTextWork *work) {
    u32 first;
    u32 second;
    u32 third;
    u32 mode = func_ov027_02170a38(work);

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

void func_ov027_02170934(SurveyTextWork *work) {
    func_ov036_02187c1c(work->window);
    work->window = NULL;
}

void func_ov027_02170944(SurveyTextWork *work) {
    work->wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
}

void func_ov027_02170954(SurveyTextWork *work) {
    GFL_WordSetSystemFree(work->wordSet);
    work->wordSet = NULL;
}

void func_ov027_02170964(SurveyTextWork *work) {
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
    kind = func_ov027_02170a38(work);
    num1 = func_ov027_02170a4c(work);
    num2 = func_ov027_02170a60(work);
    msgId = GetTrainerCardTextMSGID(kind);
    player = GetGameDataPlayerInfo(work->gameData);
    copyVarForText(work->wordSet, 0, player);
    WordSetNumber(work->wordSet, 1, num1, 5, 1, 1);
    WordSetNumber(work->wordSet, 2, num2, 5, 1, 1);
    GFL_MsgDataLoadStrbuf(work->message, msgId, source);
    GFL_WordSetFormatStrbuf(work->wordSet, formatted, source);
    GFL_WordSetClearAll(work->wordSet);
    func_ov036_02187c4c(work->window, 8, 0, formatted);
    window = func_ov036_02187c9c(work->window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    GFL_StrBufFree(source);
    GFL_StrBufFree(formatted);
}

void func_ov027_02170a1c(SurveyTextWork *work) {
    func_ov036_02187c7c(work->window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(func_ov036_02187c9c(work->window)));
}

u32 func_ov027_02170a38(SurveyTextWork *work) {
    return func_0200c96c(getTrainerCardData_wrapper(GameData_GetSaveControl_(work->gameData)));
}

u32 func_ov027_02170a4c(SurveyTextWork *work) {
    return func_0200c924(getTrainerCardData_wrapper(GameData_GetSaveControl_(work->gameData)));
}

u32 func_ov027_02170a60(SurveyTextWork *work) {
    return func_0200c90c(getTrainerCardData_wrapper(GameData_GetSaveControl_(work->gameData)));
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
