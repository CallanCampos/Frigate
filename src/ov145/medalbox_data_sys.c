#include "types.h"
#include "app/medalbox_data_sys.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "save/medal_box.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The medals of the Medal Box screen and their order, named by the ROM's string; the function names are ours

// The medals' definitions, one file each
#define ARCID_MEDAL_DEF 235

// Added to the sort key of a hint medal, which puts them after the medals
#define SORT_KEY_HINT 0xfffffff

// What a medal's file of ARCID_MEDAL_DEF holds, as far as the box reads it
typedef struct {
    u32 threshold;
    int check;
    u8 unk8;
    // The sequence of the medal's cell actor
    u8 sequence;
    u8 kind;
    u8 rank;
} MedalBoxMedalDef;

static MedalBoxEntry *MedalBoxDataSys_GetEntry(MedalBoxDataSys *data, u8 column, u8 slot);
static u64 MedalBoxDataSys_GetDateKey(MedalBoxEntry *entry, BOOL newest);
static void MedalBoxDataSys_ClearOrder(MedalBoxDataSys *data);

// The medals in the order of their names
static int sMedalBoxNameOrder[MEDAL_COUNT] = {
    173, 221, 150, 46,  74,  222, 151, 75,  152, 47,  48,  153, 180, 57,  229, 230, 145, 91,  103, 194, 202, 201,
    199, 200, 160, 105, 131, 106, 107, 108, 233, 161, 198, 169, 218, 112, 49,  129, 102, 176, 38,  247, 12,  50,
    95,  211, 205, 132, 220, 42,  251, 90,  83,  101, 122, 60,  62,  61,  63,  81,  41,  250, 133, 20,  86,  87,
    88,  30,  239, 183, 3,   235, 56,  55,  53,  54,  127, 13,  128, 175, 189, 98,  210, 179, 181, 33,  242, 28,
    237, 219, 174, 0,   36,  245, 163, 231, 214, 40,  249, 177, 232, 24,  191, 18,  31,  240, 104, 68,  192, 164,
    35,  244, 185, 21,  89,  99,  76,  9,   215, 204, 85,  139, 10,  212, 58,  32,  241, 69,  138, 137, 5,   7,
    109, 111, 110, 93,  52,  113, 4,   217, 234, 8,   141, 26,  196, 197, 195, 225, 226, 135, 144, 67,  207, 123,
    171, 64,  45,  114, 203, 182, 115, 27,  236, 162, 190, 23,  253, 165, 227, 228, 1,   77,  78,  79,  80,  22,
    166, 156, 157, 34,  243, 15,  94,  216, 37,  246, 92,  158, 159, 96,  66,  134, 143, 168, 213, 39,  248, 209,
    2,   100, 120, 208, 223, 224, 187, 188, 186, 17,  16,  121, 140, 82,  71,  178, 19,  43,  252, 11,  118, 117,
    119, 130, 116, 70,  25,  170, 254, 84,  72,  167, 65,  126, 125, 124, 73,  6,   146, 147, 59,  148, 149, 172,
    193, 206, 154, 155, 44,  136, 51,  184, 97,  29,  238, 14,  142,
};

void MedalBoxDataSys_Init(MedalBoxDataSys *data, GameData *gameData, HeapID heapId) {
    MedalBox *box = SaveControl_GetMedalBox(GameData_GetSaveControl(gameData));
    u16 i;
    u16 count;
    ArcTool *arc;
    MedalBoxMedalDef def;
    u8 year;
    u8 month;
    u8 day;

    data->count = MedalBox_GetObtainedCount(box, TRUE);
    count = 0;
    data->entries = GFL_HeapAllocate(heapId, data->count * sizeof(MedalBoxEntry), FALSE, "medalbox_data_sys.c", 45);
    MI_CpuFill8(data->entries, 0, data->count * sizeof(MedalBoxEntry));
    MedalBoxDataSys_ClearOrder(data);
    arc = GFL_ArcSysCreateFileHandle(ARCID_MEDAL_DEF, heapId);
    for (i = 0; i < MEDAL_COUNT; i++) {
        u8 status = MedalBox_GetMedalStatus(box, i);

        // A medal that was earned on a day is a hint medal until Mr. Medal hands it over
        if (status == MEDAL_STATUS_EARNED) {
            MedalBox_GetMedalDate(box, i, &year, &month, &day);
            if (year + month + day != 0) {
                status = MEDAL_STATUS_DISCOVERED;
            }
        }
        if (status == MEDAL_STATUS_DISCOVERED || status == MEDAL_STATUS_OBTAINED) {
            MedalBoxEntry *entry = &data->entries[count];

            entry->status = status;
            entry->id = i;
            entry->special = MedalBox_IsMedalSpecial(box, i);
            MedalBox_GetMedalDate(box, i, &entry->year, &entry->month, &entry->day);
            GFL_ArcToolRead(arc, i, &def);
            entry->sequence = def.sequence;
            data->order[count] = count;
            count++;
        }
    }
    GFL_ArcToolFree(arc);
    data->columnCount = data->count / MEDALBOX_COLUMN_SIZE;
    if (data->count % MEDALBOX_COLUMN_SIZE != 0) {
        data->columnCount++;
    }
    if (data->columnCount < MEDALBOX_COLUMN_MIN) {
        data->columnCount = MEDALBOX_COLUMN_MIN;
    }
    data->favoriteMedal = MedalBox_GetLastMedal(box);
}

void MedalBoxDataSys_Free(MedalBoxDataSys *data) {
    GFL_HeapFree(data->entries);
}

void MedalBoxDataSys_SaveFavorite(MedalBoxDataSys *data, GameData *gameData, HeapID heapId) {
    MedalBox_SetLastMedal(SaveControl_GetMedalBox(GameData_GetSaveControl(gameData)), data->favoriteMedal);
}

void MedalBoxDataSys_GetColumn(MedalBoxDataSys *data, MedalBoxColumn *column, int index) {
    int i;

    column->column = index;
    column->count = 0;
    for (i = 0; i < MEDALBOX_COLUMN_SIZE; i++) {
        column->medals[i] = MedalBoxDataSys_GetEntry(data, (u8)index, i);
        if (column->medals[i] != NULL) {
            column->count++;
        }
    }
}

static MedalBoxEntry *MedalBoxDataSys_GetEntry(MedalBoxDataSys *data, u8 column, u8 slot) {
    int index = slot + column * MEDALBOX_COLUMN_SIZE;

    if (index >= MEDAL_COUNT) {
        return NULL;
    }
    if (data->order[index] == 0xff) {
        return NULL;
    }
    return &data->entries[data->order[index]];
}

void MedalBoxDataSys_Sort(MedalBoxDataSys *data, int sort) {
    int i;
    int j;
    int count;

    data->sort = sort;
    MedalBoxDataSys_ClearOrder(data);
    i = 0;
    count = 0;
    switch (data->sort) {
    case MEDALBOX_SORT_OLDEST: {
        for (i = 0; i < data->count; i++) {
            data->order[i] = i;
        }
        for (i = 0; i < data->count; i++) {
            for (j = i + 1; j < data->count; j++) {
                MedalBoxEntry *entries = data->entries;
                MedalBoxEntry *b = &entries[data->order[j]];
                MedalBoxEntry *a = &entries[data->order[i]];

                if (MedalBoxDataSys_GetDateKey(a, FALSE) > MedalBoxDataSys_GetDateKey(b, FALSE)) {
                    u16 temp = data->order[i];

                    data->order[i] = data->order[j];
                    data->order[j] = temp;
                }
            }
        }
        break;
    }
    case MEDALBOX_SORT_MEDALS_ONLY: {
        for (i = 0; i < data->count; i++) {
            if (data->entries[i].status == MEDAL_STATUS_OBTAINED) {
                data->order[count] = i;
                count++;
            }
        }
        for (i = 0; i < count - 1; i++) {
            for (j = i + 1; j < count; j++) {
                MedalBoxEntry *entries = data->entries;
                MedalBoxEntry *b = &entries[data->order[j]];
                MedalBoxEntry *a = &entries[data->order[i]];

                if (MedalBoxDataSys_GetDateKey(a, FALSE) > MedalBoxDataSys_GetDateKey(b, FALSE)) {
                    u16 temp = data->order[i];

                    data->order[i] = data->order[j];
                    data->order[j] = temp;
                }
            }
        }
        break;
    }
    case MEDALBOX_SORT_NEWEST: {
        for (i = 0; i < data->count; i++) {
            data->order[i] = i;
        }
        for (i = 0; i < data->count; i++) {
            for (j = i + 1; j < data->count; j++) {
                MedalBoxEntry *entries = data->entries;
                MedalBoxEntry *b = &entries[data->order[j]];
                MedalBoxEntry *a = &entries[data->order[i]];

                if (MedalBoxDataSys_GetDateKey(a, TRUE) < MedalBoxDataSys_GetDateKey(b, TRUE)) {
                    u16 temp = data->order[i];

                    data->order[i] = data->order[j];
                    data->order[j] = temp;
                }
            }
        }
        break;
    }
    case MEDALBOX_SORT_HINTS_ONLY: {
        for (i = 0; i < data->count; i++) {
            if (data->entries[i].status == MEDAL_STATUS_DISCOVERED) {
                data->order[count] = i;
                count++;
            }
        }
        break;
    }
    case MEDALBOX_SORT_TYPE: {
        for (i = 0; i < data->count; i++) {
            data->order[i] = i;
        }
        for (i = 0; i < data->count; i++) {
            for (j = i + 1; j < data->count; j++) {
                MedalBoxEntry *a = &data->entries[data->order[i]];
                MedalBoxEntry *b = &data->entries[data->order[j]];
                u32 keyA = a->id;
                u32 keyB = b->id;

                if (a->status == MEDAL_STATUS_DISCOVERED) {
                    keyA += SORT_KEY_HINT;
                }
                if (b->status == MEDAL_STATUS_DISCOVERED) {
                    keyB += SORT_KEY_HINT;
                }
                if (keyA > keyB) {
                    u16 temp = data->order[i];

                    data->order[i] = data->order[j];
                    data->order[j] = temp;
                }
            }
        }
        break;
    }
    case MEDALBOX_SORT_NAME: {
        for (i = 0; i < data->count; i++) {
            data->order[i] = i;
        }
        for (i = 0; i < data->count; i++) {
            MedalBoxEntry *entry = &data->entries[i];
            int k;

            for (k = 0; k < MEDAL_COUNT; k++) {
                if (entry->id == sMedalBoxNameOrder[k]) {
                    entry->nameOrder = k;
                    break;
                }
            }
        }
        for (i = 0; i < data->count; i++) {
            for (j = i + 1; j < data->count; j++) {
                MedalBoxEntry *a = &data->entries[data->order[i]];
                MedalBoxEntry *b = &data->entries[data->order[j]];
                u32 keyA = a->nameOrder;
                u32 keyB = b->nameOrder;

                if (a->status == MEDAL_STATUS_DISCOVERED) {
                    keyA += SORT_KEY_HINT;
                }
                if (b->status == MEDAL_STATUS_DISCOVERED) {
                    keyB += SORT_KEY_HINT;
                }
                if (keyA > keyB) {
                    u16 temp = data->order[i];

                    data->order[i] = data->order[j];
                    data->order[j] = temp;
                }
            }
        }
        break;
    }
    }
}

// A number that orders the medals by the day they were obtained, and within a day by their number
static u64 MedalBoxDataSys_GetDateKey(MedalBoxEntry *entry, BOOL newest) {
    u64 key;

    if (newest == FALSE) {
        key = (u64)entry->year * 1000000000 + (u64)entry->month * 100000 + (u64)entry->day * 1000 + (u32)entry->id;
    } else {
        key = (u64)entry->year * 1000000000 + (u64)entry->month * 100000 + (u64)entry->day * 1000 +
              ((u64)0xff - entry->id);
    }
    return key;
}

static void MedalBoxDataSys_ClearOrder(MedalBoxDataSys *data) {
    u16 i;

    for (i = 0; i < MEDAL_COUNT; i++) {
        data->order[i] = 0xff;
    }
}

u8 MedalBoxDataSys_FindMedal(MedalBoxDataSys *data, int medal) {
    int column;
    int slot;
    int index = 0;

    for (column = 0; column < data->columnCount; column++) {
        for (slot = 0; slot < MEDALBOX_COLUMN_SIZE; slot++) {
            MedalBoxEntry *entry = MedalBoxDataSys_GetEntry(data, column, slot);

            if (entry != NULL) {
                if (entry->id == medal) {
                    return index;
                }
                index++;
            }
        }
    }
    return 0xff;
}
