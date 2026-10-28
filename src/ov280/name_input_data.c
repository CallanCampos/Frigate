#include "types.h"
#include "app/name_input_data.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "system/str_tool.h"

// The name-entry screen's tables, named after the ROM's "namein_data.c". The name-entry screen is overlay 280

// The name-entry screen's archive
#define ARCID_NAMEIN 0x78
// Its members: the keyboards, then the word tables, then the conversion table
#define NARC_NAME_INPUT_KEYBOARD_0 0
#define NARC_NAME_INPUT_WORD_TABLE_0 5
#define NARC_NAME_INPUT_CONV_TABLE 8

static NameInputKeyboard *NameInputData_LoadKeyboard(u32 index, HeapID heapId);
static void NameInputData_FreeKeyboard(NameInputKeyboard *keyboard);
static u16 NameInputData_WordTableCount(const NameInputWordTable *table);
static BOOL NameInputData_WordMatches(const u16 *word, const u16 *str);
static u16 NameInputData_ConvTableCount(const NameInputConvTable *table);
static u16 NameInputData_ConvTableGet(const NameInputConvTable *table, u16 row, u16 column);

static NameInputKeyboard *NameInputData_LoadKeyboard(u32 index, HeapID heapId) {
    return GFL_ArcSysReadHeapNewLZ(ARCID_NAMEIN, NARC_NAME_INPUT_KEYBOARD_0 + index, FALSE, heapId);
}

static void NameInputData_FreeKeyboard(NameInputKeyboard *keyboard) {
    GFL_HeapFree(keyboard);
}

NameInputKeyboardSet *NameInputData_LoadKeyboards(HeapID heapId) {
    NameInputKeyboardSet *set = GFL_HeapAllocate(heapId, sizeof(NameInputKeyboardSet), FALSE, "namein_data.c", 147);
    int i;

    MI_CpuFill8(set, 0, sizeof(NameInputKeyboardSet));
    for (i = 0; i < NAME_INPUT_KEYBOARD_COUNT; i++) {
        set->keyboards[i] = NameInputData_LoadKeyboard(i, heapId);
    }
    return set;
}

void NameInputData_FreeKeyboards(NameInputKeyboardSet *set) {
    int i;

    for (i = 0; i < NAME_INPUT_KEYBOARD_COUNT; i++) {
        NameInputData_FreeKeyboard(set->keyboards[i]);
    }
    GFL_HeapFree(set);
}

NameInputKeyboard *NameInputData_GetKeyboard(const NameInputKeyboardSet *set, u32 index) {
    return set->keyboards[index];
}

u16 NameInputKeyboard_GetWidth(const NameInputKeyboard *keyboard) {
    return keyboard->width;
}

u16 NameInputKeyboard_GetHeight(const NameInputKeyboard *keyboard) {
    return keyboard->height;
}

u16 NameInputKeyboard_GetChar(const NameInputKeyboard *keyboard, u16 x, u16 y) {
    return keyboard->chars[x + y * NameInputKeyboard_GetWidth(keyboard)];
}

NameInputWordTable *NameInputData_LoadWordTable(u32 index, HeapID heapId) {
    return GFL_ArcSysReadHeapNewLZ(ARCID_NAMEIN, NARC_NAME_INPUT_WORD_TABLE_0 + index, FALSE, heapId);
}

void NameInputData_FreeWordTable(NameInputWordTable *table) {
    GFL_HeapFree(table);
}

static u16 NameInputData_WordTableCount(const NameInputWordTable *table) {
    return table->count;
}

void NameInputWordTable_GetForm0(const NameInputWordTable *table, u32 index, u16 *dest, u32 n, u8 *len) {
    wcharsncpy(table->words[index].forms[0], dest, n);
    if (len != NULL) {
        for (*len = 0; *len < NAME_INPUT_WORD_CHARS && dest[*len] != NAME_INPUT_EMPTY_CHAR; (*len)++) {
        }
    }
}

BOOL NameInputWordTable_GetForm1(const NameInputWordTable *table, u32 index, u16 *dest, u32 n, u8 *len) {
    u8 i;

    wcharsncpy(table->words[index].forms[1], dest, n);
    for (i = 0; i < NAME_INPUT_WORD_CHARS && dest[i] != NAME_INPUT_EMPTY_CHAR; i++) {
    }
    if (len != NULL) {
        *len = i;
    }
    return i != 0;
}

BOOL NameInputWordTable_GetForm2(const NameInputWordTable *table, u32 index, u16 *dest, u32 n, u8 *len) {
    u8 i;

    wcharsncpy(table->words[index].forms[2], dest, n);
    for (i = 0; i < NAME_INPUT_WORD_CHARS && dest[i] != NAME_INPUT_EMPTY_CHAR; i++) {
    }
    if (len != NULL) {
        *len = i;
    }
    return i != 0;
}

BOOL NameInputWordTable_GetForm3(const NameInputWordTable *table, u32 index, u16 *dest, u32 n, u8 *len) {
    u8 i;

    wcharsncpy(table->words[index].forms[3], dest, n);
    for (i = 0; i < NAME_INPUT_WORD_CHARS && dest[i] != NAME_INPUT_EMPTY_CHAR; i++) {
    }
    if (len != NULL) {
        *len = i;
    }
    return i != 0;
}

u16 NameInputWordTable_Find(const NameInputWordTable *table, const u16 *str) {
    int count = NameInputData_WordTableCount(table);
    int i;

    for (i = 0; i < count; i++) {
        if (NameInputData_WordMatches(table->words[i].forms[0], str)) {
            return i;
        }
    }
    return 0xffff;
}

static BOOL NameInputData_WordMatches(const u16 *word, const u16 *str) {
    u16 terminator = GFL_StrBufGetTerminator();
    int i;

    for (i = 0; i < NAME_INPUT_WORD_CHARS; i++, word++, str++) {
        if (*word == NAME_INPUT_EMPTY_CHAR && *str == terminator) {
            return TRUE;
        }
        if (*word != *str) {
            return FALSE;
        }
        if (*str == terminator) {
            return FALSE;
        }
    }
    return TRUE;
}

NameInputConvTable *NameInputData_LoadConvTable(HeapID heapId) {
    return GFL_ArcSysReadHeapNewLZ(ARCID_NAMEIN, NARC_NAME_INPUT_CONV_TABLE, FALSE, heapId);
}

void NameInputData_FreeConvTable(NameInputConvTable *table) {
    GFL_HeapFree(table);
}

static u16 NameInputData_ConvTableCount(const NameInputConvTable *table) {
    return table->count;
}

static u16 NameInputData_ConvTableGet(const NameInputConvTable *table, u16 row, u16 column) {
    return table->chars[row][column];
}

BOOL NameInputConvTable_GetNext(const NameInputConvTable *table, const u16 *in, u16 *out) {
    int i;
    int j;

    if (in[0] == NAME_INPUT_EMPTY_CHAR) {
        return FALSE;
    }
    for (i = 0; i < NameInputData_ConvTableCount(table); i++) {
        for (j = 0; j < NAME_INPUT_WORD_CHARS; j++) {
            if (in[0] == NameInputData_ConvTableGet(table, i, j)) {
                do {
                    j = (j + 1) % NAME_INPUT_WORD_CHARS;
                    out[0] = table->chars[i][j];
                } while (out[0] == NAME_INPUT_EMPTY_CHAR);
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL NameInputData_IsNameValid(const u16 *name, HeapID heapId) {
    NameInputKeyboard *keyboards[4];
    BOOL valid;
    NameInputConvTable *conv;
    int y;
    int pos;
    u32 k;
    int x;
    int j;
    int i;
    BOOL found;

    for (i = 0; i < 4; i++) {
        keyboards[i] = NameInputData_LoadKeyboard(i, HEAPID_TAIL(heapId));
    }
    conv = NameInputData_LoadConvTable(HEAPID_TAIL(heapId));
    valid = TRUE;
    pos = 0;
    while (name[pos] != GFL_StrBufGetTerminator()) {
        found = FALSE;
        for (k = 0; k < 4; k++) {
            for (y = 0; y < NameInputKeyboard_GetHeight(keyboards[k]) && !found; y++) {
                for (x = 0; x < NameInputKeyboard_GetWidth(keyboards[k]) && !found; x++) {
                    if (name[pos] == NameInputKeyboard_GetChar(keyboards[k], x, y)) {
                        found = TRUE;
                    }
                }
            }
        }
        for (i = 0; i < NameInputData_ConvTableCount(conv); i++) {
            for (j = 0; j < NAME_INPUT_WORD_CHARS; j++) {
                if (name[pos] == NameInputData_ConvTableGet(conv, i, j)) {
                    found = TRUE;
                }
            }
        }
        if (!found) {
            valid = FALSE;
            break;
        }
        pos++;
    }
    NameInputData_FreeConvTable(conv);
    for (i = 0; i < 4; i++) {
        NameInputData_FreeKeyboard(keyboards[i]);
    }
    return valid;
}
