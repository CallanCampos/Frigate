#ifndef POKEBW2_SAVE_SURVEY_SAVEDATA_H
#define POKEBW2_SAVE_SURVEY_SAVEDATA_H

#include "types.h"
#include "struct_decls.h"

// Block 0x3d, the Passerby Analytics HQ's survey (survey_savedata.c, a descriptive name).

#define SURVEY_QUESTION_COUNT 30
#define SURVEY_ANSWER_COUNT 150
#define SURVEY_ACTIVE_QUESTIONS 3

typedef struct SurveySave {
    // How many people answered each question today, and how many gave each answer
    u16 todayCounts[SURVEY_QUESTION_COUNT];
    u8 todayAnswers[SURVEY_ANSWER_COUNT];
    // The player's own answers, each question's in its bits
    u8 ownAnswers[10];
    // The same counts before today
    u32 totalCounts[SURVEY_QUESTION_COUNT];
    u16 totalAnswers[SURVEY_ANSWER_COUNT];
    // The questions being surveyed
    u8 questions[SURVEY_ACTIVE_QUESTIONS];
} SurveySave;

// The block's size and its initial state
u32 SurveySave_GetSize(void);
void SurveySave_Init(SurveySave *survey);
SurveySave *SurveySave_Get(SaveControl *save);
// The player's own answers, which the next two read and write a question's answer of
u8 *SurveySave_GetOwnAnswers(SurveySave *survey);
u32 SurveySave_ReadAnswer(u8 *answers, u8 question);
void SurveySave_WriteAnswer(u8 *answers, u8 question, u32 answer);
// Sets the survey's question at index, and returns it
void SurveySave_SetQuestion(SurveySave *survey, u8 question, u32 index);
u8 SurveySave_GetQuestion(SurveySave *survey, u32 index);
// How many people answered a question today, which the second adds to, up to 9999, and before today
u16 SurveySave_GetTodayCount(SurveySave *survey, int question);
void SurveySave_AddTodayCount(SurveySave *survey, u32 question, u32 count);
u32 SurveySave_GetTotalCount(SurveySave *survey, int question);
// How many people gave an answer, numbered from 1, today: whether it is full, the count, and adding to it, up to 99
BOOL SurveySave_IsTodayAnswerFull(SurveySave *survey, u16 question, int answer);
u32 SurveySave_GetTodayAnswerCount(SurveySave *survey, u16 question, int answer);
void SurveySave_AddTodayAnswerCount(SurveySave *survey, u16 question, int answer, int count);
// The same count before today
u32 SurveySave_GetTotalAnswerCount(SurveySave *survey, u16 question, int answer);
// Adds today's counts to the totals and clears them
void SurveySave_FinishDay(SurveySave *survey);

// The table of the questions, of a file that is not written yet: how many bits a question's answer takes, the index
// of its first answer in the answer counts, and the first bit of the player's answer
u8 SurveyQuestion_GetBitCount(u8 question);
u8 SurveyQuestion_GetFirstAnswer(u8 question);
u8 SurveyQuestion_GetFirstBit(u8 question);

#endif // POKEBW2_SAVE_SURVEY_SAVEDATA_H
