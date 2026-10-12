#include "types.h"
#include "app/manual_category.h"
#include "app/manual_common.h"
#include "app/manual_data.h"
#include "app/manual_list.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"

// The manual's list of categories, named after the ROM's "manual_category.c": the categories that have a title the
// player can read

struct ManualCategory {
    ManualCategoryParam *param;
    ManualCommon *common;
    ManualListParam listParam;
    ManualList *list;
    TCB *vblankTask;
};

static void ManualCategory_VBlank(TCB *tcb, void *data);

ManualCategory *ManualCategory_Create(ManualCategoryParam *param, ManualCommon *common) {
    u16 i;
    u16 count;
    ManualCategory *category = GFL_HeapAllocate(common->heapId, sizeof(ManualCategory), TRUE, "manual_category.c", 92);

    category->param = param;
    param->result = MANUAL_LIST_RESULT_RETURN;
    category->param->selected = 0;
    category->common = common;
    category->listParam.type = MANUAL_LIST_CATEGORY;
    count = ManualData_GetCategoryCount(common->data);
    category->listParam.count = 0;
    category->listParam.items =
        GFL_HeapAllocate(category->common->heapId, count * sizeof(ManualListItem), TRUE, "manual_category.c", 113);
    for (i = 0; i < count; i++) {
        u16 open = 0;
        u16 titleCount = ManualData_GetCategoryTitleCount(category->common->data, i);
        u16 j;

        for (j = 0; j < titleCount; j++) {
            u16 title = ManualData_GetCategoryTitle(category->common->data, i, j);
            u16 condition = ManualData_GetTitleCondition(category->common->data, title);

            if (ManualData_IsOpen(category->common->data, condition, category->common->gameData)) {
                open++;
            }
        }
        if (open != 0) {
            category->listParam.items[category->listParam.count].id = i;
            category->listParam.items[category->listParam.count].msg =
                ManualData_GetCategoryMsg(category->common->data, i);
            category->listParam.items[category->listParam.count].mark = MANUAL_LIST_MARK_NONE;
            category->listParam.count++;
        }
    }
    category->listParam.scroll = category->param->scroll;
    category->listParam.cursor = category->param->cursor;
    category->list = ManualList_Create(&category->listParam, common);
    category->vblankTask = GFL_VBlankTCBAdd(ManualCategory_VBlank, category, 1);
    return category;
}

void ManualCategory_Free(ManualCategory *category) {
    GFL_TCBRemove(category->vblankTask);
    category->param->scroll = category->listParam.scroll;
    category->param->cursor = category->listParam.cursor;
    if (category->listParam.result == MANUAL_LIST_RESULT_RETURN) {
        category->param->result = MANUAL_LIST_RESULT_RETURN;
    } else if (category->listParam.result == MANUAL_LIST_RESULT_DECIDE) {
        category->param->result = MANUAL_LIST_RESULT_DECIDE;
        category->param->selected = category->listParam.items[category->param->cursor].id;
    }
    ManualList_Free(category->list);
    GFL_HeapFree(category->listParam.items);
    GFL_HeapFree(category);
}

// Returns whether the screen has ended
BOOL ManualCategory_Main(ManualCategory *category) {
    return ManualList_Main(category->list);
}

static void ManualCategory_VBlank(TCB *tcb, void *data) {
}
