#include "types.h"
#include "app/namein_data.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "system/str_tool.h"

// The name-entry screen's tables, named after the ROM's "namein_data.c". The name-entry screen is overlay 280

// The name-entry screen's archive
#define ARCID_NAMEIN 0x78
// Its members: the keyboards, then the word tables, then the conversion table
#define NARC_NAMEIN_KEYBOARD_0 0
#define NARC_NAMEIN_WORD_TABLE_0 5
#define NARC_NAMEIN_CONV_TABLE 8

static void *NameinData_LoadKeyboard(u32 index, HeapID heapId);
static void NameinData_FreeKeyboard(NameinKeyboard *keyboard);
static u16 NameinData_WordTableCount(const NameinWordTable *table);
static BOOL NameinData_WordMatches(const u16 *word, const u16 *str);
static u16 NameinData_ConvTableCount(const NameinConvTable *table);
static u16 NameinData_ConvTableGet(const NameinConvTable *table, u16 row, u16 column);

static void *NameinData_LoadKeyboard(u32 index, HeapID heapId) {
    return GFL_ArcSysReadHeapNewLZ(ARCID_NAMEIN, NARC_NAMEIN_KEYBOARD_0 + index, FALSE, heapId);
}

static void NameinData_FreeKeyboard(NameinKeyboard *keyboard) {
    GFL_HeapFree(keyboard);
}

NameinKeyboardSet *NameinData_LoadKeyboards(HeapID heapId) {
    NameinKeyboardSet *set = GFL_HeapAllocate(heapId, sizeof(NameinKeyboardSet), FALSE, "namein_data.c", 147);
    int i;

    MI_CpuFill8(set, 0, sizeof(NameinKeyboardSet));
    for (i = 0; i < NAMEIN_KEYBOARD_COUNT; i++) {
        set->keyboards[i] = NameinData_LoadKeyboard(i, heapId);
    }
    return set;
}

void NameinData_FreeKeyboards(NameinKeyboardSet *set) {
    int i;

    for (i = 0; i < NAMEIN_KEYBOARD_COUNT; i++) {
        NameinData_FreeKeyboard(set->keyboards[i]);
    }
    GFL_HeapFree(set);
}

NameinKeyboard *NameinData_GetKeyboard(const NameinKeyboardSet *set, u32 index) {
    return set->keyboards[index];
}

u16 NameinKeyboard_GetWidth(const NameinKeyboard *keyboard) {
    return keyboard->width;
}

u16 NameinKeyboard_GetHeight(const NameinKeyboard *keyboard) {
    return keyboard->height;
}

u16 NameinKeyboard_GetChar(const NameinKeyboard *keyboard, u16 x, u16 y) {
    return keyboard->chars[x + y * NameinKeyboard_GetWidth(keyboard)];
}

NameinWordTable *NameinData_LoadWordTable(u32 index, HeapID heapId) {
    return GFL_ArcSysReadHeapNewLZ(ARCID_NAMEIN, NARC_NAMEIN_WORD_TABLE_0 + index, FALSE, heapId);
}

void NameinData_FreeWordTable(NameinWordTable *table) {
    GFL_HeapFree(table);
}

static u16 NameinData_WordTableCount(const NameinWordTable *table) {
    return table->count;
}

void NameinWordTable_GetForm0(const NameinWordTable *table, u32 index, u16 *dest, u32 n, u8 *len) {
    wcharsncpy(table->words[index].forms[0], dest, n);
    if (len != NULL) {
        for (*len = 0; *len < NAMEIN_WORD_CHARS && dest[*len] != NAMEIN_EMPTY_CHAR; (*len)++) {
        }
    }
}

BOOL NameinWordTable_GetForm1(const NameinWordTable *table, u32 index, u16 *dest, u32 n, u8 *len) {
    u8 i;

    wcharsncpy(table->words[index].forms[1], dest, n);
    for (i = 0; i < NAMEIN_WORD_CHARS && dest[i] != NAMEIN_EMPTY_CHAR; i++) {
    }
    if (len != NULL) {
        *len = i;
    }
    return i != 0;
}

BOOL NameinWordTable_GetForm2(const NameinWordTable *table, u32 index, u16 *dest, u32 n, u8 *len) {
    u8 i;

    wcharsncpy(table->words[index].forms[2], dest, n);
    for (i = 0; i < NAMEIN_WORD_CHARS && dest[i] != NAMEIN_EMPTY_CHAR; i++) {
    }
    if (len != NULL) {
        *len = i;
    }
    return i != 0;
}

BOOL NameinWordTable_GetForm3(const NameinWordTable *table, u32 index, u16 *dest, u32 n, u8 *len) {
    u8 i;

    wcharsncpy(table->words[index].forms[3], dest, n);
    for (i = 0; i < NAMEIN_WORD_CHARS && dest[i] != NAMEIN_EMPTY_CHAR; i++) {
    }
    if (len != NULL) {
        *len = i;
    }
    return i != 0;
}

u16 NameinWordTable_Find(const NameinWordTable *table, const u16 *str) {
    int count = NameinData_WordTableCount(table);
    int i;

    for (i = 0; i < count; i++) {
        if (NameinData_WordMatches(table->words[i].forms[0], str)) {
            return i;
        }
    }
    return 0xffff;
}

static BOOL NameinData_WordMatches(const u16 *word, const u16 *str) {
    u16 terminator = GFL_StrBufGetTerminator();
    int i;

    for (i = 0; i < NAMEIN_WORD_CHARS; i++, word++, str++) {
        if (*word == NAMEIN_EMPTY_CHAR && *str == terminator) {
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

NameinConvTable *NameinData_LoadConvTable(HeapID heapId) {
    return GFL_ArcSysReadHeapNewLZ(ARCID_NAMEIN, NARC_NAMEIN_CONV_TABLE, FALSE, heapId);
}

void NameinData_FreeConvTable(NameinConvTable *table) {
    GFL_HeapFree(table);
}

static u16 NameinData_ConvTableCount(const NameinConvTable *table) {
    return table->count;
}

static u16 NameinData_ConvTableGet(const NameinConvTable *table, u16 row, u16 column) {
    return table->chars[row][column];
}

BOOL NameinConvTable_GetNext(const NameinConvTable *table, const u16 *in, u16 *out) {
    int i;
    int j;

    if (in[0] == NAMEIN_EMPTY_CHAR) {
        return FALSE;
    }
    for (i = 0; i < NameinData_ConvTableCount(table); i++) {
        for (j = 0; j < NAMEIN_WORD_CHARS; j++) {
            if (in[0] == NameinData_ConvTableGet(table, i, j)) {
                do {
                    j = (j + 1) % NAMEIN_WORD_CHARS;
                    out[0] = table->chars[i][j];
                } while (out[0] == NAMEIN_EMPTY_CHAR);
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL NameinData_IsNameValid(const u16 *name, HeapID heapId) {
    NameinKeyboard *keyboards[4];
    BOOL valid;
    NameinConvTable *conv;
    int y;
    int pos;
    u32 k;
    int x;
    int j;
    int i;
    BOOL found;

    for (i = 0; i < 4; i++) {
        keyboards[i] = NameinData_LoadKeyboard(i, HEAPID_TAIL(heapId));
    }
    conv = NameinData_LoadConvTable(HEAPID_TAIL(heapId));
    valid = TRUE;
    pos = 0;
    while (name[pos] != GFL_StrBufGetTerminator()) {
        found = FALSE;
        for (k = 0; k < 4; k++) {
            for (y = 0; y < NameinKeyboard_GetHeight(keyboards[k]) && !found; y++) {
                for (x = 0; x < NameinKeyboard_GetWidth(keyboards[k]) && !found; x++) {
                    if (name[pos] == NameinKeyboard_GetChar(keyboards[k], x, y)) {
                        found = TRUE;
                    }
                }
            }
        }
        for (i = 0; i < NameinData_ConvTableCount(conv); i++) {
            for (j = 0; j < NAMEIN_WORD_CHARS; j++) {
                if (name[pos] == NameinData_ConvTableGet(conv, i, j)) {
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
    NameinData_FreeConvTable(conv);
    for (i = 0; i < 4; i++) {
        NameinData_FreeKeyboard(keyboards[i]);
    }
    return valid;
}
