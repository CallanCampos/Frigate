#ifndef POKEBW2_FIELD_SURVEY_H
#define POKEBW2_FIELD_SURVEY_H

// Survey function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).

#include "types.h"
#include "gfl/heap.h"
#include "save/survey_savedata.h"
#include "struct_decls.h"

struct SurveyProbabilityState {
    u8 selection[6];
    u8 probability[6];
    u16 score;
    u16 bonus;
};

struct SurveyProbabilityEntry {
    u8 selection;
    u8 requiredScore;
    u8 probability;
    u8 flags;
};

struct SurveyTextWork {
    HeapID heapId;
    u8 unused02[2];
    GameSystem *gameSystem;
    GameData *gameData;
    Field *field;
    FieldMsgBG *msgBGSys;
    MoneyWin *window;
    MsgData *message;
    WordSet *wordSet;
};

// The hours since the survey started, up to 24
u16 detectLengthSinceLastSession(SaveControl *save);
void TrainerGameInfo_SetSurveyQuestion(TrainerGameInfoSave *info, u8 value);
void TrainerGameInfo_SetSurveyAnswer(TrainerGameInfoSave *info, u8 index, u8 value);
void TrainerGameInfo_SetSurveyCount(TrainerGameInfoSave *info, u8 index, u16 value);
// Whether the survey's answer of the index has been given
BOOL TrainerGameInfo_GetSurveyCount(TrainerGameInfoSave *info, u8 index);
BOOL SurveyQuestion_IsNewInB2W2(u8 question);
BOOL Survey_IsCurrentDone(GameData *gameData);
// Overlay 12's survey.c: the questions of the surveys, and whether one is done
u32 Survey_GetQuestionType(u16 question);
u32 Survey_GetQuestionAnswerCount(u16 question);
void Survey_GetQuestionAnswers(u16 question, u8 *answers);
u32 Survey_GetQuestionThreshold(u16 question);
int Survey_GetQuestionHours(u8 question);
u16 Survey_GetCurrentState(SaveControl *save);
BOOL Survey_IsCurrentDone(GameData *gameData);
u16 Survey_GetQuestionState(SaveControl *save, u8 question);
u32 Survey_GetAnswerCount(SurveySave *survey, u8 answer);
u16 Survey_GetCurrentAnswersLeft(SaveControl *save);
u16 Survey_GetCurrentHoursLeft(SaveControl *save);
GameEvent *SurveyTextEvent_Create(GameSystem *gsys);
void SurveyTextWork_Init(SurveyTextWork *work, GameSystem *gsys);
int GetSurveyAnswerMsgIDCount(int question);
u16 GetSurveyAnswerMsgID(int question, int answer);

void probabilityLoop(SurveyProbabilityState *state);
void insideProbabilityLoop(SurveyProbabilityState *state, u32 selection);
void getSurveyText(SurveyTextWork *work);
void SurveyTextWork_FreeText(SurveyTextWork *work);
void SurveyTextWork_CreateWindow(SurveyTextWork *work);
void SurveyTextWork_DeleteWindow(SurveyTextWork *work);
void SurveyTextWork_CreateWordSet(SurveyTextWork *work);
void SurveyTextWork_FreeWordSet(SurveyTextWork *work);
void SurveyTextWork_PrintRankMessage(SurveyTextWork *work);
void SurveyTextWork_ClearWindow(SurveyTextWork *work);
u32 SurveyTextWork_GetRank(SurveyTextWork *work);
u32 SurveyTextWork_GetReportNumberA(SurveyTextWork *work);
u32 SurveyTextWork_GetReportNumberB(SurveyTextWork *work);
BOOL Scrcmd_Survey_SurveyGetRank(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Survey_SurveyRaiseRank(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Survey_SurveyGetCurrentQuestionID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Survey_SurveyGetCurrentAnswerIDs(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Survey_SurveyGetTime(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Survey_SurveyStart(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Survey_SurveyClear(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Survey_SurveyGetPopularOptionMsgID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Survey_SurveyWriteOwnAnswer(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Survey_SurveyShowRankMessage(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Survey_SurveyGetQuestionState(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Survey_SurveyGetCurrentRemaining(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SURVEY_H
