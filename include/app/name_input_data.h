#ifndef POKEBW2_APP_NAME_INPUT_DATA_H
#define POKEBW2_APP_NAME_INPUT_DATA_H

#include "types.h"
#include "gfl/heap.h"

// name_input_data.c (overlay 280): the name-entry screen's tables, loaded from its archive. Layouts
// are ours, read off the accesses

// The characters that can be entered are in keyboards, a grid each. 0x3000 (an ideographic space) fills the unused
// places of a grid, a word or a row
#define NAME_INPUT_EMPTY_CHAR 0x3000
// The most characters of one of a word's spellings
#define NAME_INPUT_WORD_CHARS 3
#define NAME_INPUT_KEYBOARD_COUNT 5

// A grid of characters, width by height, row by row
typedef struct {
    u16 width;
    u16 height;
    u16 chars[];
} NameInputKeyboard;

typedef struct {
    NameInputKeyboard *keyboards[NAME_INPUT_KEYBOARD_COUNT];
} NameInputKeyboardSet;

// A word as it is spelled in four ways (the member of the table that it is read with picks one)
typedef struct {
    u16 forms[4][NAME_INPUT_WORD_CHARS];
} NameInputWord;

typedef struct {
    u16 count;
    NameInputWord words[];
} NameInputWordTable;

// The characters that change into each other: the rows of NAME_INPUT_WORD_CHARS characters, whose unused places are
// NAME_INPUT_EMPTY_CHAR
typedef struct {
    u16 count;
    u16 chars[][NAME_INPUT_WORD_CHARS];
} NameInputConvTable;

NameInputKeyboardSet *NameInputData_LoadKeyboards(HeapID heapId);
void NameInputData_FreeKeyboards(NameInputKeyboardSet *set);
NameInputKeyboard *NameInputData_GetKeyboard(const NameInputKeyboardSet *set, u32 index);
u16 NameInputKeyboard_GetWidth(const NameInputKeyboard *keyboard);
u16 NameInputKeyboard_GetHeight(const NameInputKeyboard *keyboard);
u16 NameInputKeyboard_GetChar(const NameInputKeyboard *keyboard, u16 x, u16 y);

// The word tables: the characters that take a dakuten, those that take a handakuten, and the sequences of characters
// that compose into one
enum {
    NAME_INPUT_WORD_TABLE_DAKUTEN,
    NAME_INPUT_WORD_TABLE_HANDAKUTEN,
    NAME_INPUT_WORD_TABLE_COMPOSE,
    NAME_INPUT_WORD_TABLE_MAX,
};

NameInputWordTable *NameInputData_LoadWordTable(u32 index, HeapID heapId);
void NameInputData_FreeWordTable(NameInputWordTable *table);
// The four spellings of a word, copied to dest with at most n characters. len gets how many characters before the first
// NAME_INPUT_EMPTY_CHAR. All but the first return whether there are any
void NameInputWordTable_GetForm0(const NameInputWordTable *table, u32 index, u16 *dest, u32 n, u8 *len);
BOOL NameInputWordTable_GetForm1(const NameInputWordTable *table, u32 index, u16 *dest, u32 n, u8 *len);
BOOL NameInputWordTable_GetForm2(const NameInputWordTable *table, u32 index, u16 *dest, u32 n, u8 *len);
BOOL NameInputWordTable_GetForm3(const NameInputWordTable *table, u32 index, u16 *dest, u32 n, u8 *len);
// The index of the word whose first spelling is str, or 0xffff
u16 NameInputWordTable_Find(const NameInputWordTable *table, const u16 *str);

NameInputConvTable *NameInputData_LoadConvTable(HeapID heapId);
void NameInputData_FreeConvTable(NameInputConvTable *table);
// Writes to out the character that follows in's in its row; FALSE if in has none
BOOL NameInputConvTable_GetNext(const NameInputConvTable *table, const u16 *in, u16 *out);

// Whether every character of name is on a keyboard or in the conversion table
BOOL NameInputData_IsNameValid(const u16 *name, HeapID heapId);

#endif // POKEBW2_APP_NAME_INPUT_DATA_H
