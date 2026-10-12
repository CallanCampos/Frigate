#include "types.h"
#include "field/survey.h"
#include "save/survey_savedata.h"

// The tables of the passerby survey's questions: the messages of their answers, and where the answers are kept in
// the save, which survey_savedata.c and the trainer's game info read. The ROM has no string for the file, so the name
// is a guess. GetSurveyAnswerMsgID and GetSurveyAnswerMsgIDCount are swan's names.

#define SURVEY_ANSWER_MAX 17

typedef struct {
    // Whether the question has no rank of its own in the trainer's game info
    u8 unk00_0 : 1;
    u8 unk00_1 : 7;
    // How many bits the answer takes
    u8 bits;
    // The index of the first answer in the save's answer counts
    u8 firstAnswer;
    // The first bit of the player's answer
    u8 firstBit;
} SurveyQuestion;

static const u8 sAnswerMsgIdCounts[SURVEY_QUESTION_COUNT] = {
    2, 2, 2, 2, 2, 2, 2, 3, 3, 2, 3, 3, 4, 2, 4, 4, 4, 4, 4, 5, 5, 5, 6, 6, 5, 8, 8, 8, 17, 11,
};

static const SurveyQuestion sQuestions[SURVEY_QUESTION_COUNT] = {
    { 1, 3, 2, 0, 0 },
    { 0, 3, 2, 2, 2 },
    { 0, 3, 2, 4, 4 },
    { 0, 3, 2, 6, 6 },
    { 0, 3, 2, 8, 8 },
    { 0, 3, 2, 10, 10 },
    { 0, 3, 2, 12, 12 },
    { 0, 4, 2, 14, 14 },
    { 0, 4, 2, 17, 16 },
    { 1, 4, 2, 20, 18 },
    { 1, 4, 2, 23, 20 },
    { 0, 4, 2, 26, 22 },
    { 1, 5, 3, 29, 24 },
    { 1, 5, 3, 33, 27 },
    { 0, 5, 3, 37, 30 },
    { 0, 5, 3, 41, 33 },
    { 1, 5, 3, 45, 36 },
    { 1, 5, 3, 49, 39 },
    { 1, 5, 3, 53, 42 },
    { 0, 6, 3, 57, 45 },
    { 0, 6, 3, 62, 48 },
    { 0, 6, 3, 67, 51 },
    { 0, 7, 3, 72, 54 },
    { 1, 7, 3, 78, 57 },
    { 1, 7, 3, 84, 60 },
    { 0, 9, 4, 90, 63 },
    { 0, 9, 4, 98, 67 },
    { 1, 12, 4, 106, 71 },
    { 0, 18, 5, 117, 75 },
    { 1, 12, 0, 134, 80 },
};

static const u8 sAnswerMsgIds[SURVEY_QUESTION_COUNT][SURVEY_ANSWER_MAX] = {
    { 1, 2 },
    { 3, 4 },
    { 5, 6 },
    { 7, 8 },
    { 9, 10 },
    { 11, 12 },
    { 13, 14 },
    { 15, 16, 17 },
    { 18, 19, 20 },
    { 21, 22 },
    { 24, 25, 26 },
    { 27, 28, 29 },
    { 30, 31, 32, 33 },
    { 34, 35 },
    { 38, 39, 40, 41 },
    { 42, 43, 44, 45 },
    { 46, 47, 48, 49 },
    { 50, 51, 52, 53 },
    { 54, 55, 56, 57 },
    { 58, 59, 60, 61, 62 },
    { 63, 64, 65, 66, 67 },
    { 68, 69, 70, 71, 72 },
    { 73, 74, 75, 76, 77, 78 },
    { 79, 80, 81, 82, 83, 84 },
    { 85, 86, 87, 88, 89 },
    { 91, 92, 93, 94, 95, 96, 97, 98 },
    { 99, 100, 101, 102, 103, 104, 105, 106 },
    { 107, 108, 109, 110, 111, 112, 113, 114 },
    { 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134 },
    { 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145 },
};

u16 GetSurveyAnswerMsgID(u16 question, int answer) {
    return sAnswerMsgIds[question][answer];
}

int GetSurveyAnswerMsgIDCount(int question) {
    if (question > SURVEY_QUESTION_COUNT - 1) {
        return 0;
    }
    return sAnswerMsgIdCounts[question];
}

u8 func_02011450(u8 question) {
    if (question > SURVEY_QUESTION_COUNT - 1) {
        return 0;
    }
    return sQuestions[question].bits;
}

u8 func_02011464(u8 question) {
    if (question > SURVEY_QUESTION_COUNT - 1) {
        return 0;
    }
    return sQuestions[question].firstAnswer;
}

u8 func_02011478(u8 question) {
    if (question > SURVEY_QUESTION_COUNT - 1) {
        return 0;
    }
    return sQuestions[question].firstBit;
}

BOOL func_0201148c(u8 question) {
    if (question > SURVEY_QUESTION_COUNT - 1) {
        return TRUE;
    }
    return sQuestions[question].unk00_0;
}
