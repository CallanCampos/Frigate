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

static BOOL func_ov012_021654a4(void *survey, u16 question);
static BOOL func_ov012_021654e8(SaveControl *save);
static BOOL func_ov012_02165518(SaveControl *save);
static BOOL func_ov012_02165550(void *survey, u16 question);

u32 func_ov012_021652cc(u16 question) {
    return data_ov012_0216dad0[question].type;
}

u32 func_ov012_021652dc(u16 question) {
    return data_ov012_0216dad0[question].answerCount;
}

void func_ov012_021652ec(u16 question, u8 *answers) {
    answers[0] = data_ov012_0216dad0[question].answers[0];
    answers[1] = data_ov012_0216dad0[question].answers[1];
    answers[2] = data_ov012_0216dad0[question].answers[2];
}

u32 func_ov012_02165310(u16 question) {
    return data_ov012_0216dad0[question].threshold;
}

int func_ov012_02165320(u8 question) {
    return data_ov012_0216dad0[question].days;
}

u16 func_ov012_02165330(SaveControl *save) {
    void *survey = func_0200ec2c(save);
    u16 question = func_0200ca7c(getTrainerCardData_wrapper(save));
    u8 type = func_ov012_021652cc(question);

    if (type == 0) {
        if (func_ov012_021654a4(survey, question)) {
            return 1;
        }
        return 2;
    }
    if (type == 1) {
        if (func_ov012_021654e8(save)) {
            if (func_ov012_02165518(save)) {
                return 1;
            }
            return 4;
        }
        return 3;
    }
    return 2;
}

BOOL func_ov012_0216538c(GameData *gameData) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    TrainerCardSave *info = getTrainerCardData_wrapper(save);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    u16 question = func_0200ca7c(info);

    if (question == 0) {
        return FALSE;
    }
    if (EventWork_FlagGet(eventWork, question + FLAG_SURVEY_DONE_BASE)) {
        return FALSE;
    }
    if (func_ov012_02165330(save) == 1) {
        return TRUE;
    }
    return FALSE;
}

u16 func_ov012_021653d8(SaveControl *save, u8 question) {
    void *survey = func_0200ec2c(save);
    u8 type;

    getTrainerCardData_wrapper(save);
    type = func_ov012_021652cc(question);
    if (type == 0) {
        if (func_ov012_021654a4(survey, question)) {
            return 1;
        }
        return 2;
    }
    if (type == 1) {
        if (func_ov012_02165550(survey, question)) {
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

    if (func_0200ca7c(info) == 0) {
        return 0;
    }
    start = getSecondsFromTrainerCardData(info);
    func_0207d244(&date, &time, RTC_ConvertSecondsCached() - start);
    if (date.year != 0 || date.month > 1 || date.day > 1) {
        return 24;
    }
    return time.hour;
}

u32 func_ov012_02165480(void *survey, u8 answer) {
    u32 count = func_0200ecf0(survey, answer) + func_0200ed14(survey, answer);

    if (count > SURVEY_COUNT_MAX) {
        count = SURVEY_COUNT_MAX;
    }
    return count;
}

static BOOL func_ov012_021654a4(void *survey, u16 question) {
    u8 answers[3];
    u8 count;
    u32 threshold;
    int i;

    func_ov012_021652ec(question, answers);
    count = func_ov012_021652dc(question);
    threshold = func_ov012_02165310(question);
    for (i = 0; i < count; i++) {
        if (func_ov012_02165480(survey, answers[i]) < threshold) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL func_ov012_021654e8(SaveControl *save) {
    u16 question;
    int hours;

    func_0200ec2c(save);
    question = func_0200ca7c(getTrainerCardData_wrapper(save));
    hours = detectLengthSinceLastSession(save);
    if (func_ov012_02165320(question) > hours) {
        return FALSE;
    }
    return TRUE;
}

static BOOL func_ov012_02165518(SaveControl *save) {
    TrainerCardSave *info = getTrainerCardData_wrapper(save);
    u8 count = func_ov012_021652dc(func_0200ca7c(info));
    int i;

    for (i = 0; i < count; i++) {
        if (!func_0200ca9c(info, i)) {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL func_ov012_02165550(void *survey, u16 question) {
    u8 answers[3];
    u8 count;
    int i;

    func_ov012_021652ec(question, answers);
    count = func_ov012_021652dc(question);
    func_ov012_02165310(question);
    for (i = 0; i < count; i++) {
        if (func_ov012_02165480(survey, answers[i]) < SURVEY_COUNT_MAX) {
            return FALSE;
        }
    }
    return TRUE;
}
