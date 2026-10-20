// The Battle Recorder's menus: the buttons of each menu, which this file copies and marks as valid by what is saved.
// The name is the ROM's string, from GFL_HeapAllocate's asserts

#include "types.h"
#include "app/battle_recorder/br_btn_data.h"
#include "app/battle_recorder/br_core.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "system/wordset.h"

// The menus that lead to a menu, as the menu and the button pressed in each, ending in BR_BTN_LINK_END
typedef struct {
    u32 menuID;
    u32 btnID;
} BrBtnLink;

#define BR_BTN_LINK_END 0xffffffff

typedef struct {
    u16 max;
    const BrBtnData *cp_data;
    const BrBtnLink *cp_link;
} BrBtnDataTbl;

struct BrBtnDataSys {
    BrBtnDataSetup setup;
    BrBtnData data[];
};

static void BrBtnData_Load(BrBtnDataSys *p_wk);

// Declared in the order that gives the ROM's layout, which MWCC sorts by size
static const BrBtnLink sc_link_menu00[] = {
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnLink sc_link_menu01[] = {
    { 0, 0 },
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnLink sc_link_menu02[] = {
    { 0, 0 },
    { 1, 1 },
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnLink sc_link_menu03[] = {
    { 0, 0 },
    { 1, 2 },
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnLink sc_link_menu07[] = {
    { 5, 0 },
    { 6, 0 },
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnLink sc_link_menu04[] = {
    { 0, 0 },
    { 1, 2 },
    { 3, 1 },
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnLink sc_link_menu09[] = {
    { 5, 2 },
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnLink sc_link_menu06[] = {
    { 5, 0 },
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnLink sc_link_menu10[] = {
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnLink sc_link_menu05[] = {
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnLink sc_link_menu08[] = {
    { 5, 1 },
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnLink sc_link_menu11[] = {
    { 10, 2 },
    { BR_BTN_LINK_END, BR_BTN_LINK_END },
};

static const BrBtnData sc_btn_menu00[] = {
    { { 42, 41, 0, 0, 7, 0, 1, 0, 0, 0, 0, 0 } },
    { { 42, 77, 0, 1, 3, 4, 3, 0, 0, 0, 0, 0 } },
    { { 42, 113, 0, 2, 9, 4, 4, 0, 0, 0, 0, 0 } },
    { { 42, 149, 0, 3, 0, 3, 0, 0, 0, 0, 0, 0 } },
};

static const BrBtnData sc_btn_menu01[] = {
    { { 42, 41, 1, 5, 10, 4, 2, 0, 0, 1, 308, 1 } },
    { { 42, 77, 1, 6, 11, 0, 2, 0, 0, 0, 0, 0 } },
    { { 42, 113, 1, 7, 12, 0, 3, 0, 0, 0, 0, 0 } },
    { { 42, 149, 1, 4, 1, 2, 0, 0, 0, 0, 0, 0 } },
};

static const BrBtnData sc_btn_menu05[] = {
    { { 42, 57, 5, 95, 7, 0, 6, 0, 0, 0, 0, 0 } },
    { { 42, 93, 5, 96, 8, 1, 8, 340, 0, 1, 121, 1 } },
    { { 42, 129, 5, 3, 0, 1, 9, 229, 0, 0, 0, 0 } },
};

static const BrBtnData sc_btn_menu06[] = {
    { { 42, 41, 6, 31, 13, 0, 7, 0, 0, 0, 0, 0 } },
    { { 42, 77, 6, 34, 5, 4, 6, 0, 0, 0, 0, 0 } },
    { { 42, 113, 6, 35, 6, 4, 7, 0, 0, 0, 0, 0 } },
    { { 42, 149, 6, 4, 1, 2, 5, 0, 0, 0, 0, 0 } },
};

static const BrBtnData sc_btn_menu03[] = {
    { { 42, 57, 3, 27, 10, 4, 9, 0, 0, 1, 308, 1 } },
    { { 42, 93, 3, 28, 11, 0, 4, 0, 0, 0, 0, 0 } },
    { { 42, 129, 3, 4, 1, 2, 1, 0, 0, 0, 0, 0 } },
};

static const BrBtnData sc_btn_menu04[] = {
    { { 42, 41, 4, 9, 11, 4, 9, 1, 0, 1, 308, 2 } },
    { { 42, 77, 4, 9, 11, 4, 9, 2, 0, 1, 308, 3 } },
    { { 42, 113, 4, 9, 11, 4, 9, 3, 0, 1, 308, 4 } },
    { { 42, 149, 4, 4, 1, 2, 3, 0, 0, 0, 0, 0 } },
};

static const BrBtnData sc_btn_menu07[] = {
    { { 42, 41, 7, 33, 4, 4, 5, 0, 0, 0, 0, 0 } },
    { { 42, 77, 7, 39, 14, 4, 5, 1, 0, 0, 0, 0 } },
    { { 42, 113, 7, 40, 3, 4, 5, 2, 0, 0, 0, 0 } },
    { { 42, 149, 7, 4, 1, 2, 6, 0, 0, 0, 0, 0 } },
};

static const BrBtnData sc_btn_menu02[] = {
    { { 42, 41, 2, 9, 11, 4, 2, 1, 0, 1, 308, 2 } },
    { { 42, 77, 2, 9, 11, 4, 2, 2, 0, 1, 308, 3 } },
    { { 42, 113, 2, 9, 11, 4, 2, 3, 0, 1, 308, 4 } },
    { { 42, 149, 2, 4, 1, 2, 1, 0, 0, 0, 0, 0 } },
};

static const BrBtnData sc_btn_menu10[] = {
    { { 42, 57, 10, 42, 0, 4, 11, 0, 0, 0, 0, 0 } },
    { { 42, 93, 10, 41, 1, 4, 12, 0, 0, 1, 329, 5 } },
    { { 42, 129, 10, 3, 2, 1, 11, 229, 0, 0, 0, 0 } },
};

static const BrBtnData sc_btn_menu09[] = {
    { { 42, 73, 9, 161, 2, 3, 0, 0, 0, 0, 0, 0 } },
    { { 42, 109, 9, 162, 1, 2, 5, 0, 0, 0, 0, 0 } },
};

static const BrBtnData sc_btn_menu08[] = {
    { { 42, 73, 8, 161, 2, 4, 8, 0, 0, 0, 0, 0 } },
    { { 42, 109, 8, 162, 1, 2, 5, 0, 0, 0, 0, 0 } },
};

static const BrBtnData sc_btn_menu11[] = {
    { { 42, 73, 11, 161, 3, 3, 0, 0, 0, 0, 0, 0 } },
    { { 42, 109, 11, 162, 4, 2, 10, 0, 0, 0, 0, 0 } },
};

static const BrBtnDataTbl sc_btn_data_tbl[BR_MENUID_MAX] = {
    { NELEMS(sc_btn_menu00), sc_btn_menu00, sc_link_menu00 }, { NELEMS(sc_btn_menu01), sc_btn_menu01, sc_link_menu01 },
    { NELEMS(sc_btn_menu02), sc_btn_menu02, sc_link_menu02 }, { NELEMS(sc_btn_menu03), sc_btn_menu03, sc_link_menu03 },
    { NELEMS(sc_btn_menu04), sc_btn_menu04, sc_link_menu04 }, { NELEMS(sc_btn_menu05), sc_btn_menu05, sc_link_menu05 },
    { NELEMS(sc_btn_menu06), sc_btn_menu06, sc_link_menu06 }, { NELEMS(sc_btn_menu07), sc_btn_menu07, sc_link_menu07 },
    { NELEMS(sc_btn_menu08), sc_btn_menu08, sc_link_menu08 }, { NELEMS(sc_btn_menu09), sc_btn_menu09, sc_link_menu09 },
    { NELEMS(sc_btn_menu10), sc_btn_menu10, sc_link_menu10 }, { NELEMS(sc_btn_menu11), sc_btn_menu11, sc_link_menu11 },
};

BrBtnDataSys *BrBtnData_Init(const BrBtnDataSetup *cp_setup, HeapID heapId) {
    BrBtnDataSys *p_wk;
    u32 size;
    u32 num = 0;
    int i;

    for (i = 0; i < BR_MENUID_MAX; i++) {
        num += sc_btn_data_tbl[i].max;
    }
    size = sizeof(BrBtnDataSys) + sizeof(BrBtnData) * num;
    p_wk = GFL_HeapAllocate(heapId, size, FALSE, "br_btn_data.c", 1065);
    MI_CpuFill8(p_wk, 0, size);
    p_wk->setup = *cp_setup;
    BrBtnData_Load(p_wk);
    return p_wk;
}

void BrBtnData_Exit(BrBtnDataSys *p_wk) {
    GFL_HeapFree(p_wk);
}

// Copies the buttons and marks those of saved videos and photos valid when they are saved
static void BrBtnData_Load(BrBtnDataSys *p_wk) {
    const BrRecordInfo *cp_info = p_wk->setup.recordInfo;
    int i;
    u32 j;
    int idx = 0;

    for (i = 0; i < BR_MENUID_MAX; i++) {
        u32 max = sc_btn_data_tbl[i].max;
        const BrBtnData *cp_data = sc_btn_data_tbl[i].cp_data;

        for (j = 0; j < max; j++) {
            p_wk->data[idx] = cp_data[j];
            switch (p_wk->data[idx].param[BR_BTN_DATA_PARAM_VALID_TYPE]) {
            case BR_BTN_VALID_TYPE_MINE:
                p_wk->data[idx].param[BR_BTN_DATA_PARAM_VALID] = cp_info->isValid[0];
                break;
            case BR_BTN_VALID_TYPE_OTHER1:
            case BR_BTN_VALID_TYPE_OTHER2:
            case BR_BTN_VALID_TYPE_OTHER3: {
                u32 id = p_wk->data[idx].param[BR_BTN_DATA_PARAM_VALID_TYPE] - BR_BTN_VALID_TYPE_OTHER1;

                GFL_ASSERT(id < 4);
                p_wk->data[idx].param[BR_BTN_DATA_PARAM_VALID] = cp_info->isValid[1 + id];
                if (p_wk->data[idx].param[BR_BTN_DATA_PARAM_VALID]) {
                    p_wk->data[idx].param[BR_BTN_DATA_PARAM_MSGID] = 26;
                }
                break;
            }
            case BR_BTN_VALID_TYPE_MUSICAL:
                p_wk->data[idx].param[BR_BTN_DATA_PARAM_VALID] = cp_info->hasMusicalShot;
                break;
            default:
                p_wk->data[idx].param[BR_BTN_DATA_PARAM_VALID] = TRUE;
                break;
            }
            idx++;
        }
    }
}

const BrBtnData *BrBtnData_GetData(const BrBtnDataSys *cp_wk, int menuID, u16 btnID) {
    int i;
    u32 idx;

    GFL_ASSERT(menuID < BR_MENUID_MAX);
    GFL_ASSERT(btnID < sc_btn_data_tbl[menuID].max);
    idx = 0;
    for (i = 0; i < menuID; i++) {
        idx += sc_btn_data_tbl[i].max;
    }
    return &cp_wk->data[idx + btnID];
}

u16 BrBtnData_GetNum(const BrBtnDataSys *cp_wk, int menuID) {
    GFL_ASSERT(menuID < BR_MENUID_MAX);
    return sc_btn_data_tbl[menuID].max;
}

u32 BrBtnData_GetMaxNum(const BrBtnDataSys *cp_wk) {
    u32 max = 0;
    int i;

    for (i = 0; i < BR_MENUID_MAX; i++) {
        if (max < BrBtnData_GetNum(cp_wk, i)) {
            max = BrBtnData_GetNum(cp_wk, i);
        }
    }
    return max;
}

u16 BrBtnData_GetParam(const BrBtnData *cp_data, int paramID) {
    GFL_ASSERT(paramID < BR_BTN_DATA_PARAM_MAX);
    return cp_data->param[paramID];
}

StrBuf *BrBtnData_CreateStr(const BrBtnDataSys *cp_wk, const BrBtnData *cp_data, MsgData *msg, HeapID heapId) {
    StrBuf *str;
    StrBuf *src;
    WordSet *wordset;
    int validType = BrBtnData_GetParam(cp_data, BR_BTN_DATA_PARAM_VALID_TYPE);
    BOOL isName = FALSE;
    u32 id;

    if (validType >= BR_BTN_VALID_TYPE_MINE && validType <= BR_BTN_VALID_TYPE_OTHER3) {
        id = validType - BR_BTN_VALID_TYPE_MINE;
        isName = TRUE;
    }

    if (isName) {
        if (cp_wk->setup.recordInfo->isValid[id]) {
            wordset = GFL_WordSetSystemCreateDefault(HEAPID_TAIL(heapId));
            GFL_WordSetSetStrbuf(wordset, 0, cp_wk->setup.recordInfo->name[id], cp_wk->setup.recordInfo->sex[id], TRUE,
                                 2);
            src = GFL_MsgDataLoadStrbufNew(msg, BrBtnData_GetParam(cp_data, BR_BTN_DATA_PARAM_MSGID));
            str = GFL_StrBufCreate(GFL_StrBufGetCharCount(src) + 7, heapId);
            GFL_WordSetFormatStrbuf(wordset, str, src);
            GFL_WordSetSystemFree(wordset);
            GFL_StrBufFree(src);
        } else {
            str = GFL_MsgDataLoadStrbufNew(msg, BrBtnData_GetParam(cp_data, BR_BTN_DATA_PARAM_MSGID));
        }
    } else {
        str = GFL_MsgDataLoadStrbufNew(msg, BrBtnData_GetParam(cp_data, BR_BTN_DATA_PARAM_MSGID));
    }
    return str;
}
