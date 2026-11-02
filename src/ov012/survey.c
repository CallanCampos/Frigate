// The questions of the surveys, and whether the one asked is done: answered enough times, or run for its days. The
// name is descriptive
#include "types.h"
#include "field/survey.h"
#include "gfl/rtc_cache.h"
#include "nitro/rtc.h"
#include "save/event_work.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "system/game_data.h"

// The most an answer counts
#define SURVEY_COUNT_MAX 999999
// The flags that a survey's done event sets, from question 1
#define FLAG_SURVEY_DONE_BASE 0x91

// A question of the surveys
typedef struct {
    // A SURVEY_QUESTION_TYPE_*
    u8 type;
    u8 answerCount;
    u8 answers[3];
    u8 threshold;
    u8 hours;
} SurveyQuestion;

// clang-format off
static const SurveyQuestion SURVEY_QUESTIONS[] = {
    {SURVEY_QUESTION_TYPE_HOURS, 0, {0xff, 0xff, 0xff}, 0, 0},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {0, 0xff, 0xff}, 1, 2},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {8, 0xff, 0xff}, 1, 2},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {29, 0xff, 0xff}, 1, 4},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {1, 0xff, 0xff}, 1, 4},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {26, 0xff, 0xff}, 1, 4},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {25, 0xff, 0xff}, 1, 4},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {2, 0xff, 0xff}, 1, 8},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {7, 0xff, 0xff}, 1, 8},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {28, 0xff, 0xff}, 1, 8},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {3, 0xff, 0xff}, 1, 8},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {5, 11, 0xff}, 1, 12},
    {SURVEY_QUESTION_TYPE_HOURS, 3, {4, 6, 14}, 1, 12},
    {SURVEY_QUESTION_TYPE_HOURS, 2, {12, 13, 0xff}, 1, 12},
    {SURVEY_QUESTION_TYPE_HOURS, 2, {24, 20, 0xff}, 1, 12},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {20, 0xff, 0xff}, 1, 16},
    {SURVEY_QUESTION_TYPE_HOURS, 2, {21, 22, 0xff}, 1, 16},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {10, 0xff, 0xff}, 1, 16},
    {SURVEY_QUESTION_TYPE_HOURS, 2, {16, 17, 0xff}, 1, 16},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {23, 0xff, 0xff}, 1, 20},
    {SURVEY_QUESTION_TYPE_HOURS, 2, {19, 9, 0xff}, 1, 20},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {18, 0xff, 0xff}, 1, 20},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {27, 0xff, 0xff}, 1, 24},
    {SURVEY_QUESTION_TYPE_HOURS, 1, {15, 0xff, 0xff}, 1, 24},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {0, 0xff, 0xff}, 5, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {8, 0xff, 0xff}, 5, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {29, 0xff, 0xff}, 10, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {1, 0xff, 0xff}, 10, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {26, 0xff, 0xff}, 10, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {25, 0xff, 0xff}, 10, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {2, 0xff, 0xff}, 20, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {7, 0xff, 0xff}, 20, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {28, 0xff, 0xff}, 20, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {3, 0xff, 0xff}, 20, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 2, {5, 11, 0xff}, 30, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 3, {4, 6, 14}, 30, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 2, {12, 13, 0xff}, 30, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 2, {24, 20, 0xff}, 30, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {20, 0xff, 0xff}, 40, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 2, {21, 22, 0xff}, 40, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {10, 0xff, 0xff}, 40, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 2, {16, 17, 0xff}, 40, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {23, 0xff, 0xff}, 50, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 2, {19, 9, 0xff}, 50, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {18, 0xff, 0xff}, 50, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {27, 0xff, 0xff}, 100, 0},
    {SURVEY_QUESTION_TYPE_COUNT, 1, {15, 0xff, 0xff}, 100, 0},
};

// clang-format on

static BOOL Survey_AreAnswersAtThreshold(SurveySave *survey, u16 question);
static BOOL Survey_HasRunLongEnough(SaveControl *save);
static BOOL Survey_AreAllAnswersCounted(SaveControl *save);
static BOOL Survey_AreAnswersAtMax(SurveySave *survey, u16 question);

u32 Survey_GetQuestionType(u16 question) {
    return SURVEY_QUESTIONS[question].type;
}

u32 Survey_GetQuestionAnswerCount(u16 question) {
    return SURVEY_QUESTIONS[question].answerCount;
}

void Survey_GetQuestionAnswers(u16 question, u8 *answers) {
    answers[0] = SURVEY_QUESTIONS[question].answers[0];
    answers[1] = SURVEY_QUESTIONS[question].answers[1];
    answers[2] = SURVEY_QUESTIONS[question].answers[2];
}

u32 Survey_GetQuestionThreshold(u16 question) {
    return SURVEY_QUESTIONS[question].threshold;
}

int Survey_GetQuestionHours(u8 question) {
    return SURVEY_QUESTIONS[question].hours;
}

u16 Survey_GetCurrentState(SaveControl *save) {
    SurveySave *survey = SurveySave_Get(save);
    u16 question = TrainerGameInfo_GetSurveyQuestion(getTrainerCardData_wrapper(save));
    u8 type = Survey_GetQuestionType(question);

    if (type == SURVEY_QUESTION_TYPE_COUNT) {
        if (Survey_AreAnswersAtThreshold(survey, question)) {
            return 1;
        }
        return 2;
    }
    if (type == SURVEY_QUESTION_TYPE_HOURS) {
        if (Survey_HasRunLongEnough(save)) {
            if (Survey_AreAllAnswersCounted(save)) {
                return 1;
            }
            return 4;
        }
        return 3;
    }
    return 2;
}

BOOL Survey_IsCurrentDone(GameData *gameData) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    TrainerCardSave *info = getTrainerCardData_wrapper(save);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    u16 question = TrainerGameInfo_GetSurveyQuestion(info);

    if (question == 0) {
        return FALSE;
    }
    if (EventWork_FlagGet(eventWork, question + FLAG_SURVEY_DONE_BASE)) {
        return FALSE;
    }
    if (Survey_GetCurrentState(save) == 1) {
        return TRUE;
    }
    return FALSE;
}

u16 Survey_GetQuestionState(SaveControl *save, u8 question) {
    SurveySave *survey = SurveySave_Get(save);
    u8 type;

    getTrainerCardData_wrapper(save);
    type = Survey_GetQuestionType(question);
    if (type == SURVEY_QUESTION_TYPE_COUNT) {
        if (Survey_AreAnswersAtThreshold(survey, question)) {
            return 1;
        }
        return 2;
    }
    if (type == SURVEY_QUESTION_TYPE_HOURS) {
        if (Survey_AreAnswersAtMax(survey, question)) {
            return 1;
        }
        return 3;
    }
    return 2;
}

u16 detectLengthSinceLastSession(SaveControl *save) {
    TrainerGameInfoSave *info = getTrainerGameInfoAddress(save);
    s64 start;
    RTCDate date;
    RTCTime time;

    if (TrainerGameInfo_GetSurveyQuestion(info) == 0) {
        return 0;
    }
    start = getSecondsFromTrainerCardData(info);
    RTC_ConvertSecondToDateTime(&date, &time, RTC_ConvertSecondsCached() - start);
    if (date.year != 0 || date.month > 1 || date.day > 1) {
        return 24;
    }
    return time.hour;
}

u32 Survey_GetAnswerCount(SurveySave *survey, u8 answer) {
    u32 count = SurveySave_GetTodayCount(survey, answer) + SurveySave_GetTotalCount(survey, answer);

    if (count > SURVEY_COUNT_MAX) {
        count = SURVEY_COUNT_MAX;
    }
    return count;
}

static BOOL Survey_AreAnswersAtThreshold(SurveySave *survey, u16 question) {
    u8 answers[3];
    u8 count;
    u32 threshold;
    int i;

    Survey_GetQuestionAnswers(question, answers);
    count = Survey_GetQuestionAnswerCount(question);
    threshold = Survey_GetQuestionThreshold(question);
    for (i = 0; i < count; i++) {
        if (Survey_GetAnswerCount(survey, answers[i]) < threshold) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL Survey_HasRunLongEnough(SaveControl *save) {
    u16 question;
    int hours;

    SurveySave_Get(save);
    question = TrainerGameInfo_GetSurveyQuestion(getTrainerCardData_wrapper(save));
    hours = detectLengthSinceLastSession(save);
    if (Survey_GetQuestionHours(question) > hours) {
        return FALSE;
    }
    return TRUE;
}

static BOOL Survey_AreAllAnswersCounted(SaveControl *save) {
    TrainerCardSave *info = getTrainerCardData_wrapper(save);
    u8 count = Survey_GetQuestionAnswerCount(TrainerGameInfo_GetSurveyQuestion(info));
    int i;

    for (i = 0; i < count; i++) {
        if (!TrainerGameInfo_GetSurveyCount(info, i)) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL Survey_AreAnswersAtMax(SurveySave *survey, u16 question) {
    u8 answers[3];
    u8 count;
    int i;

    Survey_GetQuestionAnswers(question, answers);
    count = Survey_GetQuestionAnswerCount(question);
    Survey_GetQuestionThreshold(question);
    for (i = 0; i < count; i++) {
        if (Survey_GetAnswerCount(survey, answers[i]) < SURVEY_COUNT_MAX) {
            return FALSE;
        }
    }
    return TRUE;
}
