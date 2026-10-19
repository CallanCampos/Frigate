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
    // 0 when the answers must reach a count, 1 when the survey runs for days
    u8 type;
    u8 answerCount;
    u8 answers[3];
    u8 threshold;
    u8 days;
} SurveyQuestion;

// clang-format off
static const SurveyQuestion data_ov012_0216dad0[] = {
    {1, 0, {0xff, 0xff, 0xff}, 0, 0},
    {1, 1, {0, 0xff, 0xff}, 1, 2},
    {1, 1, {8, 0xff, 0xff}, 1, 2},
    {1, 1, {29, 0xff, 0xff}, 1, 4},
    {1, 1, {1, 0xff, 0xff}, 1, 4},
    {1, 1, {26, 0xff, 0xff}, 1, 4},
    {1, 1, {25, 0xff, 0xff}, 1, 4},
    {1, 1, {2, 0xff, 0xff}, 1, 8},
    {1, 1, {7, 0xff, 0xff}, 1, 8},
    {1, 1, {28, 0xff, 0xff}, 1, 8},
    {1, 1, {3, 0xff, 0xff}, 1, 8},
    {1, 1, {5, 11, 0xff}, 1, 12},
    {1, 3, {4, 6, 14}, 1, 12},
    {1, 2, {12, 13, 0xff}, 1, 12},
    {1, 2, {24, 20, 0xff}, 1, 12},
    {1, 1, {20, 0xff, 0xff}, 1, 16},
    {1, 2, {21, 22, 0xff}, 1, 16},
    {1, 1, {10, 0xff, 0xff}, 1, 16},
    {1, 2, {16, 17, 0xff}, 1, 16},
    {1, 1, {23, 0xff, 0xff}, 1, 20},
    {1, 2, {19, 9, 0xff}, 1, 20},
    {1, 1, {18, 0xff, 0xff}, 1, 20},
    {1, 1, {27, 0xff, 0xff}, 1, 24},
    {1, 1, {15, 0xff, 0xff}, 1, 24},
    {0, 1, {0, 0xff, 0xff}, 5, 0},
    {0, 1, {8, 0xff, 0xff}, 5, 0},
    {0, 1, {29, 0xff, 0xff}, 10, 0},
    {0, 1, {1, 0xff, 0xff}, 10, 0},
    {0, 1, {26, 0xff, 0xff}, 10, 0},
    {0, 1, {25, 0xff, 0xff}, 10, 0},
    {0, 1, {2, 0xff, 0xff}, 20, 0},
    {0, 1, {7, 0xff, 0xff}, 20, 0},
    {0, 1, {28, 0xff, 0xff}, 20, 0},
    {0, 1, {3, 0xff, 0xff}, 20, 0},
    {0, 2, {5, 11, 0xff}, 30, 0},
    {0, 3, {4, 6, 14}, 30, 0},
    {0, 2, {12, 13, 0xff}, 30, 0},
    {0, 2, {24, 20, 0xff}, 30, 0},
    {0, 1, {20, 0xff, 0xff}, 40, 0},
    {0, 2, {21, 22, 0xff}, 40, 0},
    {0, 1, {10, 0xff, 0xff}, 40, 0},
    {0, 2, {16, 17, 0xff}, 40, 0},
    {0, 1, {23, 0xff, 0xff}, 50, 0},
    {0, 2, {19, 9, 0xff}, 50, 0},
    {0, 1, {18, 0xff, 0xff}, 50, 0},
    {0, 1, {27, 0xff, 0xff}, 100, 0},
    {0, 1, {15, 0xff, 0xff}, 100, 0},
};

// clang-format on

static BOOL Survey_AreAnswersAtThreshold(void *survey, u16 question);
static BOOL Survey_HasRunLongEnough(SaveControl *save);
static BOOL Survey_AreAllAnswersCounted(SaveControl *save);
static BOOL Survey_AreAnswersAtMax(void *survey, u16 question);

u32 Survey_GetQuestionType(u16 question) {
    return data_ov012_0216dad0[question].type;
}

u32 Survey_GetQuestionAnswerCount(u16 question) {
    return data_ov012_0216dad0[question].answerCount;
}

void Survey_GetQuestionAnswers(u16 question, u8 *answers) {
    answers[0] = data_ov012_0216dad0[question].answers[0];
    answers[1] = data_ov012_0216dad0[question].answers[1];
    answers[2] = data_ov012_0216dad0[question].answers[2];
}

u32 Survey_GetQuestionThreshold(u16 question) {
    return data_ov012_0216dad0[question].threshold;
}

int Survey_GetQuestionHours(u8 question) {
    return data_ov012_0216dad0[question].days;
}

u16 Survey_GetCurrentState(SaveControl *save) {
    void *survey = SurveySave_Get(save);
    u16 question = TrainerGameInfo_GetSurveyQuestion(getTrainerCardData_wrapper(save));
    u8 type = Survey_GetQuestionType(question);

    if (type == 0) {
        if (Survey_AreAnswersAtThreshold(survey, question)) {
            return 1;
        }
        return 2;
    }
    if (type == 1) {
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
    void *survey = SurveySave_Get(save);
    u8 type;

    getTrainerCardData_wrapper(save);
    type = Survey_GetQuestionType(question);
    if (type == 0) {
        if (Survey_AreAnswersAtThreshold(survey, question)) {
            return 1;
        }
        return 2;
    }
    if (type == 1) {
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

u32 Survey_GetAnswerCount(void *survey, u8 answer) {
    u32 count = SurveySave_GetTodayCount(survey, answer) + SurveySave_GetTotalCount(survey, answer);

    if (count > SURVEY_COUNT_MAX) {
        count = SURVEY_COUNT_MAX;
    }
    return count;
}

static BOOL Survey_AreAnswersAtThreshold(void *survey, u16 question) {
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

static BOOL Survey_AreAnswersAtMax(void *survey, u16 question) {
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
