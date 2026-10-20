#ifndef POKEBW2_SYSTEM_PMSI_PARAM_H
#define POKEBW2_SYSTEM_PMSI_PARAM_H

#include "types.h"
#include "save/save_control.h"
#include "struct_decls.h"
#include "system/pms_data.h"
#include "system/pms_word.h"

// The parameters of the sentence input screen (pmsi_param.c): what it edits, one word, two words or a sentence, the
// starting value and, once it closes, the result

enum {
    PMSI_MODE_WORD,
    PMSI_MODE_DOUBLE_WORD,
    PMSI_MODE_SENTENCE,
};

// sentence is the starting sentence in PMSI_MODE_SENTENCE, or NULL for an empty one; unused is always 0
PMSIParam *PMSIParam_Create(u32 mode, u32 unused, const PMSData *sentence, BOOL hasNumbers, SaveControl *save,
                            u32 heapId);
void PMSIParam_Free(PMSIParam *param);
void PMSIParam_SetWord(PMSIParam *param, u16 word);
void PMSIParam_SetSentence(PMSIParam *param, const PMSData *sentence);
BOOL PMSIParam_IsCanceled(const PMSIParam *param);
BOOL PMSIParam_IsChanged(const PMSIParam *param);
u16 PMSIParam_GetWord(const PMSIParam *param);
void PMSIParam_GetWords(const PMSIParam *param, u16 *words);
void PMSIParam_GetSentence(const PMSIParam *param, PMSData *sentence);
u32 PMSIParam_GetMode(const PMSIParam *param);
void *PMSIParam_GetPokeDex(const PMSIParam *param);
PMSWordSave *PMSIParam_GetWordSave(const PMSIParam *param);
BOOL func_02029a74(const PMSIParam *param);
BOOL PMSIParam_HasStartSentence(const PMSIParam *param);
BOOL PMSIParam_HasNumbers(const PMSIParam *param);
// The result, in words or sentence by the mode
void PMSIParam_GetResult(const PMSIParam *param, u16 *words, PMSData *sentence);
// Sets the result and whether it differs from the starting value
void PMSIParam_SetResult(PMSIParam *param, const u16 *words, const PMSData *sentence);
BOOL func_02029b40(const PMSIParam *param);
void func_02029b48(PMSIParam *param, BOOL touch);

#endif // POKEBW2_SYSTEM_PMSI_PARAM_H
