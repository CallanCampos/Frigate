#include "types.h"
#include "field/resort.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "save/join_avenue.h"
#include "system/resort_binary.h"

#define RESORT_DATA_COUNT 40
#define RESORT_KIND_COUNT 5

#define RESORT_SLOT_ROWS 3
#define RESORT_SLOTS 64
#define RESORT_SLOT_NO_KIND 5
#define RESORT_SLOT_NO_INDEX 0xff

// The functions of a type of data, called with the data's person, entry or record. The people's functions take a
// JoinAvenuePerson, so the table casts them
typedef void *(*ResortPersonCreateFunc)(HeapID heapId);
typedef void (*ResortPersonFunc)(void *person);
typedef BOOL (*ResortPersonIsEmptyFunc)(void *person);
typedef u32 (*ResortPersonGetParamFunc)(void *person, u32 param, void *buffer);
typedef void (*ResortPersonSetParamFunc)(void *person, u32 param, u32 value);
typedef u32 (*ResortPersonUnk1cFunc)(void *person, u32 a1, u32 a2);

typedef struct {
    ResortPersonCreateFunc create;
    ResortPersonFunc free;
    ResortPersonFunc clear;
    ResortPersonIsEmptyFunc isEmpty;
    ResortPersonGetParamFunc getParam;
    ResortPersonSetParamFunc setParam;
    void *unk18;
    ResortPersonUnk1cFunc unk1c;
} ResortPersonFuncs;

struct ResortPersonData {
    void *person;
    const ResortPersonFuncs *funcs;
    u32 kind;
    u32 index;
    u32 type;
};

// The data at the slots of a row, by kind and index
typedef struct {
    u8 index[RESORT_SLOTS];
    u8 kind[RESORT_SLOTS];
} ResortSlotRow;

struct ResortSlots {
    ResortSlotRow rows[RESORT_SLOT_ROWS];
    ResortPersonData **datas;
};

// The data of each kind, among the 40
typedef struct {
    u16 start;
    u16 end;
    u32 count;
} ResortDataRange;

static void ResortData_Init(ResortPersonData *data, void *person, u32 kind, u16 index);

static const u32 sUnk021f55a8[] = { 0, 4, 2, 1 };
static const u32 sKindsOfScript[] = { 0, 4, 2, 1 };
static const u32 sTypes[RESORT_KIND_COUNT] = { 0, 3, 2, 2, 1 };
static const u32 sKinds[RESORT_KIND_COUNT] = { 0, 1, 2, 3, 4 };
// clang-format off
static const ResortDataRange sRanges[RESORT_KIND_COUNT] = {
    {0, 8, 8},
    {8, 12, 4},
    {12, 20, 8},
    {20, 28, 8},
    {28, 40, 12},
};

// clang-format on
static const ResortPersonFuncs sFuncs[] = {
    {
        (ResortPersonCreateFunc)JoinAvenuePerson_Create,
        (ResortPersonFunc)JoinAvenuePerson_Free,
        (ResortPersonFunc)JoinAvenuePerson_Clear,
        (ResortPersonIsEmptyFunc)JoinAvenuePerson_IsEmpty,
        (ResortPersonGetParamFunc)joinAveTextHandler,
        (ResortPersonSetParamFunc)JoinAvenuePerson_SetParam,
        NULL,
        (ResortPersonUnk1cFunc)JoinAvenuePerson_Random,
    },
    {
        (ResortPersonCreateFunc)JoinAvenueEntry_Create,
        (ResortPersonFunc)JoinAvenueEntry_Free,
        (ResortPersonFunc)JoinAvenueEntry_Clear,
        (ResortPersonIsEmptyFunc)JoinAvenueEntry_IsEmpty,
        (ResortPersonGetParamFunc)JoinAvenueEntry_GetParam,
        (ResortPersonSetParamFunc)JoinAvenueEntry_SetParam,
        NULL,
        (ResortPersonUnk1cFunc)JoinAvenueEntry_Random,
    },
    {
        (ResortPersonCreateFunc)JoinAvenuePerson_Create,
        (ResortPersonFunc)JoinAvenuePerson_Free,
        (ResortPersonFunc)JoinAvenuePerson_Clear,
        (ResortPersonIsEmptyFunc)JoinAvenuePerson_IsEmpty,
        (ResortPersonGetParamFunc)joinAveTextHandler,
        (ResortPersonSetParamFunc)JoinAvenuePerson_SetParam,
        NULL,
        (ResortPersonUnk1cFunc)JoinAvenuePerson_Random,
    },
    {
        (ResortPersonCreateFunc)JoinAvenueRecord_Create,
        (ResortPersonFunc)JoinAvenueRecord_Free,
        (ResortPersonFunc)JoinAvenueRecord_Clear,
        (ResortPersonIsEmptyFunc)JoinAvenueRecord_IsEmpty,
        (ResortPersonGetParamFunc)JoinAvenueRecord_GetParam,
        (ResortPersonSetParamFunc)JoinAvenueRecord_SetParam,
        NULL,
        (ResortPersonUnk1cFunc)JoinAvenueRecord_Random,
    },
};

ResortSlots *ResortSlots_Create(ResortPersonData **datas, HeapID heapId) {
    ResortSlots *slots = GFL_HeapAllocate(heapId, sizeof(ResortSlots), TRUE, "resort_data_manager.c", 79);
    slots->datas = datas;
    ResortSlots_Fill(slots);
    return slots;
}

void ResortSlots_Free(ResortSlots *slots) {
    GFL_HeapFree(slots);
}

void ResortSlots_Fill(ResortSlots *slots) {
    int row;
    int pos;
    ResortDataIter iter;
    ResortPersonData *data;

    for (row = 0; row < RESORT_SLOT_ROWS; row++) {
        ResortSlotRow *slotRow = &slots->rows[row];

        for (pos = 0; pos < RESORT_SLOTS; pos++) {
            slotRow->kind[pos] = RESORT_SLOT_NO_KIND;
            slotRow->index[pos] = RESORT_SLOT_NO_INDEX;
        }
    }
    iter = ResortDatas_StartIter(slots->datas, sKinds, RESORT_KIND_COUNT);
    while (TRUE) {
        u16 slot;
        int dataRow;

        data = ResortDatas_NextIter(slots->datas, &iter, sKinds, RESORT_KIND_COUNT);
        if (data == NULL) {
            break;
        }
        if (ResortData_IsEmpty(data)) {
            continue;
        }
        slot = ResortData_GetParam(data, JOIN_AVE_PARAM_SLOT, NULL);
        dataRow = ResortData_GetParam(data, JOIN_AVE_PARAM_ROW, NULL);
        if (dataRow < 1 || dataRow >= RESORT_SLOT_ROWS) {
            continue;
        }
        slots->rows[dataRow].kind[slot] = ResortData_GetKind(data);
        slots->rows[dataRow].index[slot] = ResortData_GetIndex(data);
    }
}

u16 ResortData_GetTypeOfKind(u32 kind) {
    return sTypes[kind];
}

u32 ResortData_GetMakeKind(u32 kind) {
    return sUnk021f55a8[kind];
}

BOOL ResortSlots_IsEmpty(ResortSlots *slots, u32 row, u32 pos) {
    BOOL empty = FALSE;
    if (slots->rows[row].index[pos] == RESORT_SLOT_NO_INDEX && slots->rows[row].kind[pos] == RESORT_SLOT_NO_KIND) {
        empty = TRUE;
    }
    return empty;
}

ResortPersonData *ResortSlots_GetData(ResortSlots *slots, u32 row, u32 pos) {
    u8 kind = slots->rows[row].kind[pos];
    u8 index = slots->rows[row].index[pos];
    if (kind == RESORT_SLOT_NO_KIND || index == RESORT_SLOT_NO_INDEX) {
        return NULL;
    }
    return ResortDatas_Get(slots->datas, kind, index);
}

u32 ResortSlots_FindEmpty(ResortSlots *slots, u32 row) {
    u16 pos;
    for (pos = 0; pos < RESORT_SLOTS; pos++) {
        if (ResortSlots_IsEmpty(slots, row, pos)) {
            return pos;
        }
    }
    return 0xffff;
}

u16 ResortData_RaffleShopItem(ResortPersonData *data, void *a1, void *a2, JoinAvenueInfo *info) {
    u32 value = ResortData_Roll(data, 0, 100);
    u32 row = ResortBinary_FindRange(a2, JoinAvenue_GetParam(info, 2, 0));
    if (value < ResortBinary_Get(a2, row, 8)) {
        u16 shopRow = ResortBinary_Get(a2, row, 7);
        u32 prize = join_ave_raffle_shop(a1, shopRow, ResortData_Roll(data, 1, ResortBinary_Get(a1, shopRow, 0)));
        if (prize != 10) {
            return ResortBinary_Get(a1, shopRow, prize * 2 + 3);
        }
    }
    return 0;
}

ResortPersonData *ResortData_Create(HeapID heapId, void *person, u32 kind, u16 index) {
    ResortPersonData *data = GFL_HeapAllocate(heapId, sizeof(ResortPersonData), TRUE, "resort_data_manager.c", 467);
    ResortData_Init(data, person, kind, index);
    return data;
}

void ResortData_Free(ResortPersonData *data) {
    GFL_HeapFree(data);
}

static void ResortData_Init(ResortPersonData *data, void *person, u32 kind, u16 index) {
    data->kind = kind;
    data->index = index;
    data->type = ResortData_GetTypeOfKind(kind);
    data->funcs = &sFuncs[data->type];
    if (person != NULL) {
        data->person = person;
    }
}

void ResortData_Clear(ResortPersonData *data) {
    data->funcs->clear(data->person);
}

BOOL ResortData_IsEmpty(ResortPersonData *data) {
    return data->funcs->isEmpty(data->person);
}

u32 ResortData_GetParam(ResortPersonData *data, JoinAvenuePersonParam param, void *buffer) {
    return data->funcs->getParam(data->person, param, buffer);
}

void ResortData_SetParam(ResortPersonData *data, JoinAvenuePersonParam param, u32 value) {
    data->funcs->setParam(data->person, param, value);
}

void *ResortData_GetPerson(ResortPersonData *data) {
    return data->person;
}

u32 ResortData_GetType(ResortPersonData *data) {
    return data->type;
}

u32 ResortData_GetKind(ResortPersonData *data) {
    return data->kind;
}

u32 ResortData_GetIndex(ResortPersonData *data) {
    return data->index;
}

u32 ResortData_Roll(ResortPersonData *data, u32 a1, u32 a2) {
    return data->funcs->unk1c(data->person, a1, a2);
}

u32 ResortData_RollPair(ResortPersonData *data, ResortPersonData *other, u32 a2, u32 a3) {
    return JoinAvenue_RandomFromSeed(ResortData_GetParam(data, JOIN_AVE_PARAM_SEED, NULL) +
                                         ResortData_GetParam(other, JOIN_AVE_PARAM_SEED, NULL),
                                     a2, a3);
}

ResortPersonData **ResortDatas_Create(const ResortSysSetup *setup, HeapID heapId) {
    int i;
    ResortPersonData **datas =
        GFL_HeapAllocate(heapId, sizeof(ResortPersonData *) * RESORT_DATA_COUNT, TRUE, "resort_data_manager.c", 735);

    for (i = 0; i < 8; i++) {
        u16 index = i;

        datas[i] = ResortData_Create(heapId, JoinAvenueOccupants_GetPerson(setup->occupants, index), 0, index);
    }
    for (i = 8; i < 12; i++) {
        u16 index = i - 8;

        datas[i] = ResortData_Create(heapId, JoinAvenueOccupants_GetRecord(setup->occupants, index), 1, index);
    }
    for (i = 12; i < 20; i++) {
        u16 index = i - 12;

        datas[i] = ResortData_Create(heapId, JoinAvenuePersonList_Get(setup->list, index), 2, index);
    }
    if (setup->list2 != NULL) {
        for (i = 20; i < 28; i++) {
            u16 index = i - 20;

            datas[i] = ResortData_Create(heapId, JoinAvenuePersonList_Get(setup->list2, index), 3, index);
        }
    }
    for (i = 28; i < 40; i++) {
        u16 index = i - 28;

        datas[i] = ResortData_Create(heapId, JoinAvenueEntryList_Get(setup->entries, index), 4, index);
    }
    return datas;
}

void ResortDatas_Free(ResortPersonData **datas) {
    int i;
    for (i = 0; i < RESORT_DATA_COUNT; i++) {
        if (datas[i] != NULL) {
            ResortData_Free(datas[i]);
        }
    }
    GFL_HeapFree(datas);
}

ResortDataIter ResortDatas_StartIter(ResortPersonData **datas, const u32 *kinds, u32 count) {
    ResortDataIter iter;
    MI_CpuFill8(&iter, 0, sizeof(ResortDataIter));
    iter.kindIndex = 0;
    iter.pos = sRanges[kinds[0]].start;
    return iter;
}

ResortPersonData *ResortDatas_NextIter(ResortPersonData **datas, ResortDataIter *iter, const u32 *kinds, u32 count) {
    if (iter->kindIndex < count) {
        u32 kind = kinds[iter->kindIndex];
        if (sRanges[kind].start <= iter->pos && iter->pos < sRanges[kind].end) {
            return datas[iter->pos++];
        }
        iter->kindIndex++;
        if (iter->kindIndex < count) {
            iter->pos = sRanges[kinds[iter->kindIndex]].start;
            return datas[iter->pos++];
        }
    }
    return NULL;
}

ResortPersonData *ResortDatas_FindByPerson(ResortPersonData **datas, JoinAvenuePerson *person) {
    int i;
    for (i = 0; i < RESORT_DATA_COUNT; i++) {
        if (datas[i] != NULL && person == ResortData_GetPerson(datas[i])) {
            return datas[i];
        }
    }
    return NULL;
}

ResortPersonData *ResortDatas_Get(ResortPersonData **datas, u32 kind, u16 index) {
    return datas[sRanges[kind].start + index];
}

ResortPersonData *ResortDatas_GetByScript(ResortPersonData **datas, u32 which, u16 index) {
    u32 kind;
    if (which == 2) {
        if (index < 8) {
            kind = 2;
        } else {
            kind = 3;
            index -= 8;
        }
    } else {
        kind = sKindsOfScript[which];
    }
    return ResortDatas_Get(datas, kind, index);
}

ResortPersonData *ResortDatas_FindEmpty(ResortPersonData **datas, u32 kind) {
    ResortDataIter iter = ResortDatas_StartIter(datas, &kind, 1);
    ResortPersonData *data;

    while (TRUE) {
        data = ResortDatas_NextIter(datas, &iter, &kind, 1);
        if (data == NULL) {
            return NULL;
        }
        if (ResortData_IsEmpty(data)) {
            return data;
        }
    }
}
