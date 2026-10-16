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
    // Event flag 0x960
    u8 flag0 : 1;
    u8 flag1 : 1;
    u8 hasStartSentence : 1;
    u8 flag3 : 1;
    u32 unk4;
    // Save block 0x36
    void *unk8;
    PMSWordSave *wordSave;
    PMSData sentence;
    u16 words[PMS_SENTENCE_WORD_MAX];
    u32 unk1C;
};

static BOOL IsSameAsStart(const PMSIParam *param, const u16 *words, const PMSData *sentence);

PMSIParam *PMSIParam_Create(u32 mode, u32 unused, const PMSData *sentence, BOOL flag3, SaveControl *save, u32 heapId) {
    PMSIParam *param = GFL_HeapAllocate(heapId, sizeof(PMSIParam), TRUE, "pmsi_param.c", 86);
    int i;

    param->mode = mode;
    param->unk8 = SaveControl_GetBlockPtr(save, 0x36);
    param->wordSave = getDexBlkAddress(save);
    param->flag0 = EventWork_FlagGet(getConstDataBlock(save), 0x960);
    param->flag1 = FALSE;
    param->canceled = TRUE;
    param->changed = FALSE;
    param->flag3 = flag3;
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

void *PMSIParam_GetPokeDex(const PMSIParam *param) {
    return param->unk8;
}

PMSWordSave *PMSIParam_GetWordSave(const PMSIParam *param) {
    return param->wordSave;
}

BOOL func_02029a74(const PMSIParam *param) {
    return param->flag0;
}

BOOL PMSIParam_HasStartSentence(const PMSIParam *param) {
    return param->hasStartSentence;
}

BOOL PMSIParam_HasNumbers(const PMSIParam *param) {
    return param->flag3;
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

BOOL func_02029b40(const PMSIParam *param) {
    return func_0203d554();
}

void func_02029b48(PMSIParam *param, BOOL touch) {
    func_0203d564(touch);
}
