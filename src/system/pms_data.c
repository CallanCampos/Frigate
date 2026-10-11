#include "types.h"
#include "constants/abilities.h"
#include "constants/arc.h"
#include "constants/moves.h"
#include "constants/pms_words.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "system/pms_data.h"
#include "system/pms_word.h"
#include "system/printsys.h"
#include "system/wordset.h"

// Sentences with words filled in. The file's name is a guess: the ROM has no string for it

// The word a word that isn't valid is replaced with
#define PMS_WORD_DEFAULT PMS_WORD_TERM_POKEMON
// The numbers a number word can be, from PMS_NUMBER_MIN to below PMS_NUMBER_END
#define PMS_NUMBER_MIN 1
#define PMS_NUMBER_END 11

static u32 GetSentenceWordCount(u16 type, u16 id, HeapID heapId);
static BOOL IsExcludedWord(u16 word);

static const u8 sSentenceCounts[PMS_SENTENCE_TYPE_COUNT] = { 20, 20, 20, 20, 20, 21, 2 };

static const u16 sSentenceMsgFiles[PMS_SENTENCE_TYPE_COUNT] = {
    TEXT_BANK_0171, TEXT_BANK_0173, TEXT_BANK_0176, TEXT_BANK_0170, TEXT_BANK_0175, TEXT_BANK_0172, TEXT_BANK_0174,
};

void PMSData_Clear(PMSData *data) {
    int i;

    data->type = PMS_WORD_NULL;
    for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
        data->words[i] = PMS_WORD_NULL;
    }
}

void PMSData_Init(PMSData *data, u16 type) {
    int i;

    data->type = type;
    data->id = 0;
    for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
        data->words[i] = PMS_WORD_NULL;
    }
}

void PMSData_InitWithSentence(PMSData *data, u16 type, u16 id) {
    PMSData_Init(data, type);
    PMSData_SetSentence(data, type, id);
}

void func_02029bfc(PMSData *data, u32 preset) {
    switch (preset) {
    case 0:
        PMSData_Init(data, 1);
        data->id = 0;
        data->words[0] = PMS_WORD_GREETINGS_LETS_GO;
        break;
    case 1:
        PMSData_Init(data, 2);
        data->id = 0;
        data->words[0] = PMS_WORD_GREETINGS_THANKS;
        break;
    case 2:
        PMSData_Init(data, 3);
        data->id = 0;
        data->words[0] = PMS_WORD_FEELINGS_REGRET;
        break;
    case 3:
        PMSData_Init(data, 2);
        data->id = 13;
        data->words[0] = PMS_WORD_TRAINER_NO1;
        break;
    }
}

void func_02029c68(PMSData *data) {
    PMSData_Init(data, 0);
    data->id = 1;
    data->words[0] = PMS_WORD_TRAINER_TRAINER;
}

StrBuf *PMSData_ToString(const PMSData *data, u32 heapId) {
    return PMSData_ToStringWithWords(data, heapId, PMS_SENTENCE_WORD_MAX);
}

StrBuf *PMSData_ToStringWithWords(const PMSData *data, u32 heapId, int wordCount) {
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(heapId);
    int i;
    MsgData *msgData;
    StrBuf *strbuf;
    StrBuf *sentence;

    for (i = 0; i < wordCount; i++) {
        if (data->words[i] != PMS_WORD_NULL) {
            if ((data->words[i] >> PMS_WORD_NUMBER_SHIFT) & 1) {
                func_02024574(wordSet, i, data->words[i] & PMS_WORD_INDEX_MASK);
            } else {
                loadSayingForDisplay(wordSet, i, data->words[i]);
            }
        } else {
            func_02024574(wordSet, i, 1);
        }
    }
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, sSentenceMsgFiles[data->type], heapId);
    strbuf = GFL_StrBufCreate(0x100, heapId);
    sentence = GFL_MsgDataLoadStrbufNew(msgData, data->id);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, sentence);
    GFL_StrBufFree(sentence);
    GFL_MsgDataFree(msgData);
    GFL_WordSetSystemFree(wordSet);
    return strbuf;
}

StrBuf *PMSData_GetSentenceString(const PMSData *data, u32 heapId) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, sSentenceMsgFiles[data->type], heapId);
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(msgData, data->id);

    GFL_MsgDataFree(msgData);
    return strbuf;
}

BOOL PMSData_IsNotEmpty(const PMSData *data) {
    return data->type != PMS_WORD_NULL;
}

BOOL PMSData_IsComplete(const PMSData *data, HeapID heapId) {
    u32 count = GetSentenceWordCount(data->type, data->id, heapId);
    u32 i;

    for (i = 0; i < count; i++) {
        if (data->words[i] == PMS_WORD_NULL) {
            return FALSE;
        }
    }
    return TRUE;
}

static u32 GetSentenceWordCount(u16 type, u16 id, HeapID heapId) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, sSentenceMsgFiles[type], heapId);
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(msgData, id);
    u32 count;

    GFL_MsgDataFree(msgData);
    count = GFL_StrCmdGetWordSetCommandCount(strbuf);
    GFL_StrBufFree(strbuf);
    return count;
}

u16 func_02029df8(const PMSData *data, u32 index) {
    u16 word = data->words[index];

    if (word == PMS_WORD_NULL) {
        return PMS_WORD_NULL;
    }
    return word & PMS_WORD_INDEX_MASK;
}

u16 PMSData_GetWord(const PMSData *data, u32 index) {
    return data->words[index];
}

BOOL func_02029e1c(const PMSData *data, u32 index) {
    return PMSWord_IsNumber(data->words[index]);
}

BOOL PMSWord_IsNumber(u16 word) {
    if (word == PMS_WORD_NULL) {
        return FALSE;
    }
    return (word >> PMS_WORD_NUMBER_SHIFT) & 1;
}

int PMSWord_GetNumber(u16 word) {
    return word & PMS_WORD_INDEX_MASK;
}

u16 PMSData_GetType(const PMSData *data) {
    return data->type;
}

u16 PMSData_GetID(const PMSData *data) {
    return data->id;
}

BOOL PMSData_Equals(const PMSData *data, const PMSData *other) {
    int i;

    if (data->type != other->type || data->id != other->id) {
        return FALSE;
    }
    for (i = 0; i < PMS_SENTENCE_WORD_MAX; i++) {
        if (data->words[i] != other->words[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

void PMSData_Copy(PMSData *dest, const PMSData *src) {
    *dest = *src;
}

u32 PMSData_GetSentenceCount(u32 type) {
    if (type < PMS_SENTENCE_TYPE_COUNT) {
        return sSentenceCounts[type];
    }
    return 0;
}

void PMSData_SetSentence(PMSData *data, u32 type, u32 id) {
    data->type = type;
    data->id = id;
}

void PMSData_SetWord(PMSData *data, u32 index, u16 word) {
    if (index < PMS_SENTENCE_WORD_MAX) {
        data->words[index] = word;
    }
}

void PMSData_ClearUnusedWords(PMSData *data, HeapID heapId) {
    u32 i;

    for (i = GetSentenceWordCount(data->type, data->id, heapId); i < PMS_SENTENCE_WORD_MAX; i++) {
        data->words[i] = PMS_WORD_NULL;
    }
}

BOOL PMSData_IsValid(const PMSData *data, u32 heapId) {
    u16 id;
    u32 count;
    u32 i;
    BOOL valid;
    u32 index;
    u32 category;

    if (data->type < PMS_SENTENCE_TYPE_COUNT) {
        id = data->id;
        if (id < PMSData_GetSentenceCount(data->type)) {
            count = GetSentenceWordCount(data->type, id, HEAPID_TAIL(heapId));
            valid = TRUE;
            for (i = 0; i < count; i++) {
                if (PMSWord_IsNumber(data->words[i])) {
                    int number = PMSWord_GetNumber(data->words[i]);

                    if (number < PMS_NUMBER_MIN || number >= PMS_NUMBER_END) {
                        valid = FALSE;
                        break;
                    }
                } else if (!PMSWord_GetMessage(data->words[i], &category, &index)) {
                    valid = FALSE;
                    break;
                }
            }
            return valid;
        }
    }
    return FALSE;
}

BOOL PMSData_Validate(PMSData *data, BOOL allowEmpty, HeapID heapId) {
    BOOL valid = TRUE;
    u32 count;
    u32 i;

    if (data->type >= PMS_SENTENCE_TYPE_COUNT || data->id >= PMSData_GetSentenceCount(data->type)) {
        valid = FALSE;
        data->type = 0;
        data->id = 0;
    }
    count = GetSentenceWordCount(data->type, data->id, HEAPID_TAIL(heapId));
    if (count > PMS_SENTENCE_WORD_MAX) {
        count = PMS_SENTENCE_WORD_MAX;
    }
    for (i = 0; i < count; i++) {
        if (!PMSWord_Validate(&data->words[i], allowEmpty, TRUE)) {
            valid = FALSE;
        }
    }
    return valid;
}

BOOL PMSWord_Validate(u16 *word, BOOL allowEmpty, BOOL allowNumber) {
    BOOL valid = TRUE;
    u32 index;
    u32 category;

    if (allowEmpty) {
        if (*word == PMS_WORD_NULL) {
            return valid;
        }
    } else if (*word == PMS_WORD_NULL) {
        *word = PMS_WORD_DEFAULT;
        return FALSE;
    }
    if (PMSWord_IsNumber(*word)) {
        if (allowNumber) {
            int number = PMSWord_GetNumber(*word);

            if (number < PMS_NUMBER_MIN || number >= PMS_NUMBER_END) {
                *word = PMS_WORD_DEFAULT;
                valid = FALSE;
            }
        } else {
            *word = PMS_WORD_DEFAULT;
            valid = FALSE;
        }
    } else if (IsExcludedWord(*word) || !PMSWord_GetMessage(*word, &category, &index)) {
        *word = PMS_WORD_DEFAULT;
        valid = FALSE;
    }
    return valid;
}

BOOL PMSNumber_Validate(int *number, BOOL allowZero) {
    BOOL valid = TRUE;

    if (allowZero) {
        if (*number == 0) {
            return valid;
        }
    } else if (*number == 0) {
        *number = PMS_NUMBER_MIN;
        return FALSE;
    }
    if (*number < PMS_NUMBER_MIN || *number >= PMS_NUMBER_END) {
        *number = PMS_NUMBER_MIN;
        valid = FALSE;
    }
    return valid;
}

// Words that sentences can't use
static BOOL IsExcludedWord(u16 word) {
    const u16 excluded[] = {
        PMS_WORD_SPECIES(SPECIES_NONE),   PMS_WORD_SPECIES_BAD_EGG,         PMS_WORD_SPECIES_EGG,
        PMS_WORD_MOVE(MOVE_NONE),         PMS_WORD_MOVE(MOVE_TECHNO_BLAST), PMS_WORD_MOVE(MOVE_RELIC_SONG),
        PMS_WORD_MOVE(MOVE_SECRET_SWORD), PMS_WORD_MOVE(MOVE_FREEZE_SHOCK), PMS_WORD_MOVE(MOVE_ICE_BURN),
        PMS_WORD_MOVE(MOVE_SNARL),        PMS_WORD_MOVE(MOVE_V_CREATE),     PMS_WORD_ABILITY(ABILITY_NONE)
    };
    u8 i;

    for (i = 0; i < NELEMS(excluded); i++) {
        if (word == excluded[i]) {
            return TRUE;
        }
    }
    return FALSE;
}
