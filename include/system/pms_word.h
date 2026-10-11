#ifndef POKEBW2_SYSTEM_PMS_WORD_H
#define POKEBW2_SYSTEM_PMS_WORD_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "save/save_control.h"
#include "struct_decls.h"

// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except the ones config/names.txt records

// The words that fill sentences (pms_word.c): each is a message of one of the word categories' message files,
// numbered across the categories in order and named in constants/pms_words.h, and the save block of sentences and
// language flags

// No word
#define PMS_WORD_NULL 0xffff
// A word with this bit set is a number, in its low bits, rather than a message
#define PMS_WORD_NUMBER_SHIFT 11
#define PMS_WORD_INDEX_MASK 0x7ff

// The word categories, each with a message file
#define PMS_WORD_CATEGORY_COUNT 13

// The save block of sentences
#define SAVE_BLOCK_PMS 0x27

// The message files of the word categories, loaded for a while
PMSWordBank *PMSWordBank_Create(u32 heapId);
void PMSWordBank_Free(PMSWordBank *bank);
void PMSWordBank_LoadWord(PMSWordBank *bank, u16 word, StrBuf *strbuf);
// Loads one word, or clears the string for PMS_WORD_NULL
void loadSayingToString(u16 saying, StrBuf *strbuf, HeapID heapId);
// The word that message index of message file fileId is, or PMS_WORD_NULL
u16 PMSWord_FromMessage(u16 fileId, u16 index);
// The category and message index of a word, FALSE if it is past the last category
BOOL PMSWord_GetMessage(u32 word, u32 *category, u32 *index);
PMSData *PMSWordSave_GetSentence(PMSWordSave *save, int index);
void PMSWordSave_SetSentence(PMSWordSave *save, int index, const PMSData *sentence);
u32 PMSWordSave_GetSize(void);
void PMSWordSave_Init(PMSWordSave *save);
PMSWordSave *getDexBlkAddress(SaveControl *save);
// A flag for each of seven languages, by their order in the save's language table; a new save sets the game's own
BOOL PMSWordSave_GetLanguageFlag(PMSWordSave *save, u32 bit);
void PMSWordSave_SetLanguageFlag(PMSWordSave *save, u32 bit);
// Whether two words are the same word, or two of a group of words that count as the same
BOOL PMSWord_AreEquivalent(u16 word, u16 other);

#endif // POKEBW2_SYSTEM_PMS_WORD_H
