#include "app/pmsi_search.h"
#include "types.h"
#include "app/ui/msgsearch.h"
#include "app/pms_input_data.h"
#include "app/pmsi_initial_data.h"
#include "constants/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"

// The phrase input's search: the letters typed are kept as initials, and the words of the first letter's initial
// whose names start with them are listed. The names are ours, guessed

// The words of the others' initial that the search leaves out
#define PMSI_SEARCH_EXCLUDED_COUNT 10
// How many words a search lists
#define PMSI_SEARCH_RESULT_MAX 264

struct PMSISearch {
    const PMSInputWork *mwk;
    const PMSInputData *dwk;
    u16 heapId;
    // The names of the words of each initial
    MsgData *msgData[PMSI_INITIAL_COUNT];
    MsgSearch *search;
    MsgSearchResult results[PMSI_SEARCH_RESULT_MAX];
    u16 words[PMSI_SEARCH_RESULT_MAX + 2];
    StrBuf *inputStr;
    u16 input[PMSI_SEARCH_INPUT_MAX + 1];
    u8 inputLen;
    u8 resultCount;
};

static const u16 sPMSISearchExcludedWords[PMSI_SEARCH_EXCLUDED_COUNT] = {
    1732, 1733, 1734, 1735, 1736, 1737, 1738, 1739, 1740, 1741,
};

// The message files of the words of each initial
static const u16 sPMSISearchInitialMsgFiles[PMSI_INITIAL_COUNT] = {
    113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
    127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
};

PMSISearch *PMSISearch_Create(const PMSInputWork *mwk, const PMSInputData *dwk, HeapID heapId) {
    int i;
    PMSISearch *ss;

    ss = GFL_HeapAllocate(heapId, sizeof(PMSISearch), TRUE, "pmsi_search.c", 137);
    ss->mwk = mwk;
    ss->dwk = dwk;
    ss->heapId = heapId;
    PMSISearch_Reset(ss);
    for (i = 0; i < PMSI_INITIAL_COUNT; i++) {
        ss->msgData[i] = GFL_MsgSysLoadData(TRUE, ARCID_SYSTEM_MESSAGE, sPMSISearchInitialMsgFiles[i], heapId);
    }
    ss->search = MsgSearch_Create(ss->msgData, PMSI_INITIAL_COUNT, ss->heapId);
    ss->inputStr = GFL_StrBufCreate(27, heapId);
    return ss;
}

void PMSISearch_Delete(PMSISearch *ss) {
    int i;

    GFL_StrBufFree(ss->inputStr);
    MsgSearch_Free(ss->search);
    for (i = 0; i < PMSI_INITIAL_COUNT; i++) {
        GFL_MsgDataFree(ss->msgData[i]);
    }
    GFL_HeapFree(ss);
}

void PMSISearch_AddChar(PMSISearch *ss, u16 initial) {
    int len;

    ss->input[ss->inputLen] = initial;
    len = ss->inputLen + 1;
    ss->inputLen = len > PMSI_SEARCH_INPUT_MAX - 1 ? PMSI_SEARCH_INPUT_MAX - 1 : (len < 0 ? 0 : len);
}

BOOL PMSISearch_DelChar(PMSISearch *ss) {
    if (ss->inputLen == 0) {
        return FALSE;
    }
    if (ss->input[PMSI_SEARCH_INPUT_MAX - 1] == PMSI_SEARCH_INPUT_NONE) {
        ss->inputLen--;
    }
    ss->input[ss->inputLen] = PMSI_SEARCH_INPUT_NONE;
    return TRUE;
}

u8 PMSISearch_GetInputLen(PMSISearch *ss) {
    return ss->inputLen;
}

void PMSISearch_Reset(PMSISearch *ss) {
    int i;

    for (i = 0; i < PMSI_SEARCH_INPUT_MAX; i++) {
        ss->input[i] = PMSI_SEARCH_INPUT_NONE;
    }
    ss->inputLen = 0;
    ss->resultCount = 0;
}

void PMSISearch_GetInputStr(PMSISearch *ss, StrBuf *buf) {
    u16 str[PMSI_SEARCH_INPUT_MAX + 1];
    int i;

    for (i = 0; i < PMSI_SEARCH_INPUT_MAX; i++) {
        if (ss->input[i] == PMSI_SEARCH_INPUT_NONE) {
            str[i] = GFL_StrBufGetTerminator();
            break;
        }
        str[i] = PMSIInitial_GetCode(ss->input[i]);
    }
    str[PMSI_SEARCH_INPUT_MAX] = GFL_StrBufGetTerminator();
    GFL_StrBufLoadString(buf, str);
}

BOOL PMSISearch_Search(PMSISearch *ss) {
    u32 count;
    u32 initial;
    u32 i;

    initial = ss->input[0];
    if (initial >= PMSI_INITIAL_COUNT) {
        ss->resultCount = 0;
        return FALSE;
    }
    GFL_StrBufClear(ss->inputStr);
    PMSISearch_GetInputStr(ss, ss->inputStr);
    if (initial == PMSI_INITIAL_COUNT - 1) {
        count = GFL_MsgDataGetLineCount(ss->msgData[initial]);
        u32 n = 0;
        u16 words[PMSI_SEARCH_RESULT_MAX + 1] = { 0 };

        for (i = 0; i < count; i++) {
            u16 word = PMSIData_GetInitialWordCode(ss->dwk, initial, i);
            BOOL excluded = FALSE;
            u32 j;

            for (j = 0; j < PMSI_SEARCH_EXCLUDED_COUNT; j++) {
                if (word == sPMSISearchExcludedWords[j]) {
                    excluded = TRUE;
                    break;
                }
            }
            if (!excluded) {
                ss->results[n].file = initial;
                ss->results[n].line = i;
                words[n] = word;
                n++;
            }
        }
        words[n] = PMSIData_GetEndWord(ss->dwk);
        ss->resultCount = PMSIData_CountTableWords(ss->dwk, words, ss->words);
        if (ss->resultCount) {
            return TRUE;
        }
        return FALSE;
    } else {
        u16 words[PMSI_SEARCH_RESULT_MAX + 1] = { 0 };
        u32 n;

        n = MsgSearch_Find(ss->search, initial, 0, ss->inputStr, ss->results, PMSI_SEARCH_RESULT_MAX);
        for (i = 0; i < n; i++) {
            words[i] = PMSIData_GetInitialWordCode(ss->dwk, initial, ss->results[i].line);
        }
        words[n] = PMSIData_GetEndWord(ss->dwk);
        ss->resultCount = PMSIData_CountTableWords(ss->dwk, words, ss->words);
        if (ss->resultCount) {
            return TRUE;
        }
        return FALSE;
    }
}

u8 PMSISearch_GetResultCount(PMSISearch *ss) {
    return ss->resultCount;
}

void PMSISearch_GetResultStr(PMSISearch *ss, u8 index, StrBuf *buf) {
    PMSIData_GetWordStr(ss->dwk, ss->words[index], buf);
}

u16 PMSISearch_GetResultWord(PMSISearch *ss, u32 index) {
    return ss->words[index];
}
