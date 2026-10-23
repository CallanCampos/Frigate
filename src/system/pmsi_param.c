#include "types.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "save/event_work.h"
#include "save/save_control.h"
#include "system/pms_data.h"
#include "system/pms_word.h"
#include "system/pmsi_param.h"

// The parameters of the sentence input screen

struct PMSIParam {
    u8 mode;
    // Set until a result is set
    u8 canceled;
    // Whether the result differs from the starting value
    u8 changed;
    // Whether moves can be picked as words, from event flag 0x960
    u8 movesEnabled : 1;
    u8 flag1 : 1;
    u8 hasStartSentence : 1;
    u8 hasNumbers : 1;
    u32 unk4;
    // Save block 0x36, the Pokédex
    PokeDexSave *pokedex;
    PMSWordSave *wordSave;
    PMSData sentence;
    u16 words[PMS_SENTENCE_WORD_MAX];
    u32 unk1C;
};

static BOOL IsSameAsStart(const PMSIParam *param, const u16 *words, const PMSData *sentence);

PMSIParam *PMSIParam_Create(u32 mode, u32 unused, const PMSData *sentence, BOOL hasNumbers, SaveControl *save,
                            u32 heapId) {
    PMSIParam *param = GFL_HeapAllocate(heapId, sizeof(PMSIParam), TRUE, "pmsi_param.c", 86);
    int i;

    param->mode = mode;
    param->pokedex = SaveControl_GetBlockPtr(save, SAVE_BLOCK_POKEDEX);
    param->wordSave = getDexBlkAddress(save);
    param->movesEnabled = EventWork_FlagGet(getConstDataBlock(save), 0x960);
    param->flag1 = FALSE;
    param->canceled = TRUE;
    param->changed = FALSE;
    param->hasNumbers = hasNumbers;
    if (mode == PMSI_MODE_SENTENCE) {
        if (sentence == NULL) {
            PMSData_Init(&param->sentence, 0);
        } else {
            MI_CpuCopy8(sentence, &param->sentence, sizeof(PMSData));
            param->hasStartSentence = TRUE;
        }
    } else {
        for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
            param->words[i] = PMS_WORD_NULL;
        }
    }
    return param;
}

void PMSIParam_Free(PMSIParam *param) {
    GFL_HeapFree(param);
}

void PMSIParam_SetWord(PMSIParam *param, u16 word) {
    param->words[0] = word;
}

void PMSIParam_SetSentence(PMSIParam *param, const PMSData *sentence) {
    param->sentence = *sentence;
}

BOOL PMSIParam_IsCanceled(const PMSIParam *param) {
    return param->canceled;
}

BOOL PMSIParam_IsChanged(const PMSIParam *param) {
    return param->changed;
}

u16 PMSIParam_GetWord(const PMSIParam *param) {
    return param->words[0];
}

void PMSIParam_GetWords(const PMSIParam *param, u16 *words) {
    words[0] = param->words[0];
    words[1] = param->words[1];
}

void PMSIParam_GetSentence(const PMSIParam *param, PMSData *sentence) {
    PMSData_Copy(sentence, &param->sentence);
}

u32 PMSIParam_GetMode(const PMSIParam *param) {
    return param->mode;
}

PokeDexSave *PMSIParam_GetPokeDex(const PMSIParam *param) {
    return param->pokedex;
}

PMSWordSave *PMSIParam_GetWordSave(const PMSIParam *param) {
    return param->wordSave;
}

BOOL PMSIParam_AreMovesEnabled(const PMSIParam *param) {
    return param->movesEnabled;
}

BOOL PMSIParam_HasStartSentence(const PMSIParam *param) {
    return param->hasStartSentence;
}

BOOL PMSIParam_HasNumbers(const PMSIParam *param) {
    return param->hasNumbers;
}

void PMSIParam_GetResult(const PMSIParam *param, u16 *words, PMSData *sentence) {
    switch (param->mode) {
    case PMSI_MODE_WORD:
        words[0] = param->words[0];
        break;
    case PMSI_MODE_DOUBLE_WORD:
        words[0] = param->words[0];
        words[1] = param->words[1];
        break;
    case PMSI_MODE_SENTENCE:
        *sentence = param->sentence;
        break;
    }
}

static BOOL IsSameAsStart(const PMSIParam *param, const u16 *words, const PMSData *sentence) {
    switch (param->mode) {
    case PMSI_MODE_WORD:
        if (words[0] == param->words[0]) {
            return TRUE;
        }
        return FALSE;
    case PMSI_MODE_DOUBLE_WORD:
        if (words[0] == param->words[0] && words[1] == param->words[1]) {
            return TRUE;
        }
        return FALSE;
    case PMSI_MODE_SENTENCE:
    default:
        return PMSData_Equals(&param->sentence, sentence);
    }
}

void PMSIParam_SetResult(PMSIParam *param, const u16 *words, const PMSData *sentence) {
    int i;

    param->changed = !IsSameAsStart(param, words, sentence);
    param->canceled = FALSE;
    for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
        param->words[i] = words[i];
    }
    param->sentence = *sentence;
}

BOOL PMSIParam_IsUsingTouch(const PMSIParam *param) {
    return GCTX_HIDIsUsingTouch();
}

void PMSIParam_SetUsingTouch(PMSIParam *param, BOOL touch) {
    GCTX_HIDSetUsingTouch(touch);
}
