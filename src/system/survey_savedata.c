#include "types.h"
#include "save/survey_savedata.h"
#include "gfl/std.h"
#include "save/save_control.h"

// The save block of the Passerby Analytics HQ's surveys: how many people answered each question and how, today and
// before today, the player's own answers and the questions being surveyed. The ROM has no string for the file, so the
// name is descriptive. Block 0x3d.

#define SAVE_BLOCK_SURVEY 0x3d
#define SURVEY_TODAY_COUNT_MAX 9999
#define SURVEY_TODAY_ANSWER_MAX 99
#define SURVEY_TOTAL_COUNT_MAX 999999
#define SURVEY_TOTAL_ANSWER_MAX 9999

static u32 getAnswerIndex(u16 question, int answer);

u32 func_0200ec00(void) {
    return sizeof(SurveySave);
}

void func_0200ec08(SurveySave *survey) {
    sys_memset(survey, 0, sizeof(SurveySave));
    survey->questions[0] = 0;
    survey->questions[1] = 8;
    survey->questions[2] = 29;
}

SurveySave *func_0200ec2c(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_SURVEY);
}

u8 *func_0200ec38(SurveySave *survey) {
    return survey->ownAnswers;
}

u32 func_0200ec3c(u8 *answers, u8 question) {
    u32 offset = func_02011478(question);
    u32 count = func_02011450(question);
    u32 byte = offset / 8;
    u32 bit = offset % 8;
    u32 answer = 0;
    u32 i;

    for (i = 0; count != 0; count--) {
        answer |= ((answers[byte] >> bit) & 1) << i;
        bit++;
        if (bit > 7) {
            byte++;
            bit = 0;
        }
        i++;
    }
    return answer;
}

void func_0200ec80(u8 *answers, u8 question, u32 answer) {
    u32 offset = func_02011478(question);
    u32 count = func_02011450(question);
    u32 byte = offset / 8;
    u32 bit = offset % 8;

    for (; count != 0; count--) {
        answers[byte] &= 0xff ^ (1 << bit);
        answers[byte] |= (answer & 1) << bit;
        bit++;
        if (bit > 7) {
            byte++;
            bit = 0;
        }
        answer >>= 1;
    }
}

void func_0200ecd8(SurveySave *survey, u8 question, u32 index) {
    survey->questions[index] = question;
}

u8 func_0200ece4(SurveySave *survey, u32 index) {
    return survey->questions[index];
}

u16 func_0200ecf0(SurveySave *survey, int question) {
    return survey->todayCounts[question];
}

void func_0200ecf8(SurveySave *survey, u32 question, u32 count) {
    survey->todayCounts[question] += (u16)count;
    if (survey->todayCounts[question] > SURVEY_TODAY_COUNT_MAX) {
        survey->todayCounts[question] = SURVEY_TODAY_COUNT_MAX;
    }
}

u32 func_0200ed14(SurveySave *survey, int question) {
    return survey->totalCounts[question];
}

// Answers are numbered from 1
static u32 getAnswerIndex(u16 question, int answer) {
    u32 first = func_02011464(question);

    return first + (answer - 1);
}

BOOL func_0200ed34(SurveySave *survey, u16 question, int answer) {
    if (func_0200ed48(survey, question, answer) >= SURVEY_TODAY_ANSWER_MAX) {
        return TRUE;
    }
    return FALSE;
}

u32 func_0200ed48(SurveySave *survey, u16 question, int answer) {
    if (answer == 0) {
        return 0;
    }
    return survey->todayAnswers[getAnswerIndex(question, answer)];
}

void func_0200ed64(SurveySave *survey, u16 question, int answer, int count) {
    u32 index;

    if (answer == 0) {
        return;
    }
    index = getAnswerIndex(question, answer);
    if (count + survey->todayAnswers[index] > SURVEY_TODAY_ANSWER_MAX) {
        survey->todayAnswers[index] = SURVEY_TODAY_ANSWER_MAX;
    } else {
        survey->todayAnswers[index] += count;
    }
}

u32 func_0200ed90(SurveySave *survey, u16 question, int answer) {
    if (answer == 0) {
        return 0;
    }
    return survey->totalAnswers[getAnswerIndex(question, answer)];
}

// Adds today's counts to the totals and clears them
void func_0200edb0(SurveySave *survey) {
    int i;

    for (i = 0; i < SURVEY_QUESTION_COUNT; i++) {
        survey->totalCounts[i] += survey->todayCounts[i];
        if (survey->totalCounts[i] > SURVEY_TOTAL_COUNT_MAX) {
            survey->totalCounts[i] = SURVEY_TOTAL_COUNT_MAX;
        }
        survey->todayCounts[i] = 0;
    }
    for (i = 0; i < SURVEY_ANSWER_COUNT; i++) {
        survey->totalAnswers[i] += survey->todayAnswers[i];
        if (survey->totalAnswers[i] > SURVEY_TOTAL_ANSWER_MAX) {
            survey->totalAnswers[i] = SURVEY_TOTAL_ANSWER_MAX;
        }
        survey->todayAnswers[i] = 0;
    }
}
