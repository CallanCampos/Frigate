#ifndef POKEBW2_APP_NAMEIN_DATA_H
#define POKEBW2_APP_NAMEIN_DATA_H

#include "types.h"
#include "gfl/heap.h"

// namein_data.c (overlay 280): the name-entry screen's tables, loaded from its archive. Layouts
// are ours, read off the accesses

// The characters that can be entered are in keyboards, a grid each. 0x3000 (an ideographic space) fills the unused
// places of a grid, a word or a row
#define NAMEIN_EMPTY_CHAR 0x3000
// The most characters of one of a word's spellings
#define NAMEIN_WORD_CHARS 3
#define NAMEIN_KEYBOARD_COUNT 5

// A grid of characters, width by height, row by row
typedef struct {
    u16 width;
    u16 height;
    u16 chars[];
} NameinKeyboard;

typedef struct {
    NameinKeyboard *keyboards[NAMEIN_KEYBOARD_COUNT];
} NameinKeyboardSet;

// A word as it is spelled in four ways (the member of the table that it is read with picks one)
typedef struct {
    u16 forms[4][NAMEIN_WORD_CHARS];
} NameinWord;

typedef struct {
    u16 count;
    NameinWord words[];
} NameinWordTable;

// The characters that change into each other: the rows of NAMEIN_WORD_CHARS characters, whose unused places are
// NAMEIN_EMPTY_CHAR
typedef struct {
    u16 count;
    u16 chars[][NAMEIN_WORD_CHARS];
} NameinConvTable;

NameinKeyboardSet *NameinData_LoadKeyboards(HeapID heapId);
void NameinData_FreeKeyboards(NameinKeyboardSet *set);
NameinKeyboard *NameinData_GetKeyboard(const NameinKeyboardSet *set, u32 index);
u16 NameinKeyboard_GetWidth(const NameinKeyboard *keyboard);
u16 NameinKeyboard_GetHeight(const NameinKeyboard *keyboard);
u16 NameinKeyboard_GetChar(const NameinKeyboard *keyboard, u16 x, u16 y);

NameinWordTable *NameinData_LoadWordTable(u32 index, HeapID heapId);
void NameinData_FreeWordTable(NameinWordTable *table);
// The four spellings of a word, copied to dest with at most n characters. len gets how many characters before the first
// NAMEIN_EMPTY_CHAR. All but the first return whether there are any
void NameinWordTable_GetForm0(const NameinWordTable *table, u32 index, u16 *dest, u32 n, u8 *len);
BOOL NameinWordTable_GetForm1(const NameinWordTable *table, u32 index, u16 *dest, u32 n, u8 *len);
BOOL NameinWordTable_GetForm2(const NameinWordTable *table, u32 index, u16 *dest, u32 n, u8 *len);
BOOL NameinWordTable_GetForm3(const NameinWordTable *table, u32 index, u16 *dest, u32 n, u8 *len);
// The index of the word whose first spelling is str, or 0xffff
u16 NameinWordTable_Find(const NameinWordTable *table, const u16 *str);

NameinConvTable *NameinData_LoadConvTable(HeapID heapId);
void NameinData_FreeConvTable(NameinConvTable *table);
// Writes to out the character that follows in's in its row; FALSE if in has none
BOOL NameinConvTable_GetNext(const NameinConvTable *table, const u16 *in, u16 *out);

// Whether every character of name is on a keyboard or in the conversion table
BOOL NameinData_IsNameValid(const u16 *name, HeapID heapId);

#endif // POKEBW2_APP_NAMEIN_DATA_H
