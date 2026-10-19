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
    void *msgBGSys;
    void *window;
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
BOOL func_0201148c(u8 question);
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
u32 Survey_GetAnswerCount(void *survey, u8 answer);
u16 func_ov027_02170758(SaveControl *save);
u16 func_ov027_021707b8(SaveControl *save);
GameEvent *func_ov027_02170860(GameSystem *gsys);
void func_ov027_02170884(SurveyTextWork *work, GameSystem *gsys);
int GetSurveyAnswerMsgIDCount(int question);
u16 GetSurveyAnswerMsgID(int question, int answer);

void probabilityLoop(SurveyProbabilityState *state);
void insideProbabilityLoop(SurveyProbabilityState *state, u32 selection);
void getSurveyText(SurveyTextWork *work);
void func_ov027_021708d0(SurveyTextWork *work);
void func_ov027_021708e0(SurveyTextWork *work);
void func_ov027_02170934(SurveyTextWork *work);
void func_ov027_02170944(SurveyTextWork *work);
void func_ov027_02170954(SurveyTextWork *work);
void func_ov027_02170964(SurveyTextWork *work);
void func_ov027_02170a1c(SurveyTextWork *work);
u32 func_ov027_02170a38(SurveyTextWork *work);
u32 func_ov027_02170a4c(SurveyTextWork *work);
u32 func_ov027_02170a60(SurveyTextWork *work);
BOOL func_ov027_021703a8(VM *vm, FieldScriptEnv *env);
BOOL func_ov027_021703dc(VM *vm, FieldScriptEnv *env);
BOOL s01FF_SurveyGetCurrentQuestionID(VM *vm, FieldScriptEnv *env);
BOOL s0200_SurveyGetCurrentAnswerIDs(VM *vm, FieldScriptEnv *env);
BOOL s0204_SurveyGetTime(VM *vm, FieldScriptEnv *env);
BOOL func_ov027_021704e0(VM *vm, FieldScriptEnv *env);
BOOL func_ov027_02170580(VM *vm, FieldScriptEnv *env);
BOOL s0201_SurveyGetPopularOptionMsgID(VM *vm, FieldScriptEnv *env);
BOOL func_ov027_02170650(VM *vm, FieldScriptEnv *env);
BOOL func_ov027_02170698(VM *vm, FieldScriptEnv *env);
BOOL func_ov027_021706bc(VM *vm, FieldScriptEnv *env);
BOOL func_ov027_0217070c(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SURVEY_H
