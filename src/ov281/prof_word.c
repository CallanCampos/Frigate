#include "types.h"
#include "system/prof_word.h"
#include "constants/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"

// The lists of profane words, each one an encrypted run of PROF_WORD_LEN-character words
#define PROF_WORD_SEED 0x72012891

static BOOL ProfWord_CheckList(const u16 *name, u32 listId, HeapID heapId, const u16 *list, u32 size);
static BOOL ProfWord_Compare(const u16 *name, const u16 *word);

ProfWordLists *ProfWord_LoadLists(HeapID heapId) {
    int i;
    ProfWordLists *lists = GFL_HeapAllocate(heapId, sizeof(ProfWordLists), TRUE, "prof_word.c", 56);

    for (i = 0; i < PROF_WORD_LIST_COUNT; i++) {
        lists->lists[i] = GFL_ArcSysReadHeapNewLZGetLen(ARCID_PROF_WORD, i, FALSE, heapId, &lists->sizes[i]);
        _decryptData(lists->lists[i], lists->sizes[i], PROF_WORD_SEED);
    }
    return lists;
}

void ProfWord_FreeLists(ProfWordLists *lists) {
    int i;

    for (i = 0; i < PROF_WORD_LIST_COUNT; i++) {
        GFL_HeapFree(lists->lists[i]);
    }
    GFL_HeapFree(lists);
}

BOOL ProfWord_Check(ProfWordLists *lists, const u16 *name, HeapID heapId) {
    u16 buf[PROF_WORD_LEN] = { 0 };
    u16 *list;
    u32 size;
    BOOL found;
    int i;

    sys_memcpy(name, buf, sizeof(buf));
    // Fold katakana to hiragana and every Latin letter to ASCII capitals
    for (i = 0; i < PROF_WORD_LEN; i++) {
        u16 c = buf[i];
        if (c >= 0x30a1 && c <= 0x30f4) {
            buf[i] = c - 0x60;
        } else if (c >= 'a' && c <= 'z') {
            buf[i] = c - 0x20;
        } else if (c >= 0xff41 && c <= 0xff5a) {
            buf[i] = c - 0xff41 + 'A';
        } else if (c >= 0xff21 && c <= 0xff3a) {
            buf[i] = c - 0xff21 + 'A';
        } else if (c >= 0xe0 && c <= 0xfe) {
            buf[i] = c - 0x20;
        }
    }

    for (i = 0; i < PROF_WORD_LIST_COUNT; i++) {
        if (lists == NULL) {
            list = GFL_ArcSysReadHeapNewLZGetLen(ARCID_PROF_WORD, i, FALSE, heapId, &size);
            _decryptData(list, size, PROF_WORD_SEED);
        } else {
            list = lists->lists[i];
            size = lists->sizes[i];
        }
        found = ProfWord_CheckList(buf, i, heapId, list, size);
        if (lists == NULL) {
            GFL_HeapFree(list);
        }
        if (found == TRUE) {
            break;
        }
    }
    return found;
}

static BOOL ProfWord_CheckList(const u16 *name, u32 listId, HeapID heapId, const u16 *list, u32 size) {
    BOOL found;
    int i;
    int count = (u16)(size / sizeof(u16[PROF_WORD_LEN]));

    for (i = 0; i < count; i++) {
        found = ProfWord_Compare(name, &list[i * PROF_WORD_LEN]);
        if (found == TRUE) {
            break;
        }
    }
    return found;
}

static BOOL ProfWord_Compare(const u16 *name, const u16 *word) {
    int i;

    for (i = 0; i < PROF_WORD_LEN; i++) {
        if (name[i] != word[i]) {
            return FALSE;
        }
        if (name[i] == GFL_StrBufGetTerminator() && word[i] == GFL_StrBufGetTerminator()) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ProfWord_HasManyDigits(const u16 *name, u32 len, HeapID unused) {
    u16 buf[PROF_WORD_LEN] = { 0 };
    int digits = 0;
    int i;

    sys_memcpy(name, buf, len * sizeof(u16));
    for (i = 0; i < PROF_WORD_LEN; i++) {
        if ((buf[i] >= '0' && buf[i] <= '9') || (buf[i] >= 0xff10 && buf[i] <= 0xff19)) {
            digits++;
        }
        if (digits > 4) {
            return TRUE;
        }
    }
    return FALSE;
}
