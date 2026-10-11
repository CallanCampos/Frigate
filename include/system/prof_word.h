#ifndef POKEBW2_SYSTEM_PROF_WORD_H
#define POKEBW2_SYSTEM_PROF_WORD_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"

// The check of names against the lists of profane words (prof_word.c), in overlay 281, used by name entry
#define OVERLAY_PROF_WORD OVERLAY_ID(281)

#define PROF_WORD_LIST_COUNT 7
// The length of a name and of a word in the lists, terminator included
#define PROF_WORD_LEN 32

// The lists, decrypted from archive 216, kept loaded to check several names
typedef struct ProfWordLists {
    u16 *lists[PROF_WORD_LIST_COUNT];
    u32 sizes[PROF_WORD_LIST_COUNT];
} ProfWordLists;

ProfWordLists *ProfWord_LoadLists(HeapID heapId);
void ProfWord_FreeLists(ProfWordLists *lists);
// Returns TRUE if the name, with its case and kana folded, is in a list; lists may be NULL to read them one at a time
BOOL ProfWord_Check(ProfWordLists *lists, const u16 *name, HeapID heapId);
// Returns TRUE if the first len characters of the name hold more than 4 digits, which could be a phone number
BOOL ProfWord_HasManyDigits(const u16 *name, u32 len, HeapID unused);

#endif // POKEBW2_SYSTEM_PROF_WORD_H
