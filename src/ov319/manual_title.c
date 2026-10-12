#include "types.h"
#include "app/manual_common.h"
#include "app/manual_data.h"
#include "app/manual_list.h"
#include "app/manual_title.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"

// The manual's list of titles, named after the ROM's "manual_title.c": the titles the player can read, of a category
// or of them all, with a mark on those not read yet

struct ManualTitle {
    ManualTitleParam *param;
    ManualCommon *common;
    ManualListParam listParam;
    ManualList *list;
    TCB *vblankTask;
};

static void ManualTitle_VBlank(TCB *tcb, void *data);

ManualTitle *ManualTitle_Create(ManualTitleParam *param, ManualCommon *common) {
    u16 i;
    ManualTitle *title = GFL_HeapAllocate(common->heapId, sizeof(ManualTitle), TRUE, "manual_title.c", 97);

    title->param = param;
    param->result = MANUAL_LIST_RESULT_RETURN;
    title->param->selected = 0;
    title->common = common;
    if (title->param->mode == MANUAL_TITLE_MODE_CATEGORY) {
        u16 category;
        u16 count;

        title->listParam.type = MANUAL_LIST_TITLE;
        category = title->param->category;
        count = ManualData_GetCategoryTitleCount(title->common->data, category);
        title->listParam.count = 0;
        title->listParam.items =
            GFL_HeapAllocate(title->common->heapId, count * sizeof(ManualListItem), TRUE, "manual_title.c", 121);
        for (i = 0; i < count; i++) {
            u16 id = ManualData_GetCategoryTitle(title->common->data, category, i);
            u16 condition = ManualData_GetTitleCondition(title->common->data, id);

            if (ManualData_IsOpen(title->common->data, condition, title->common->gameData)) {
                u16 flag = ManualData_GetTitleReadFlag(title->common->data, id);
                BOOL read = ManualData_IsRead(title->common->data, flag, title->common->gameData);

                title->listParam.items[title->listParam.count].id = id;
                title->listParam.items[title->listParam.count].msg = ManualData_GetTitleMsg(title->common->data, id);
                title->listParam.items[title->listParam.count].mark =
                    read ? MANUAL_LIST_MARK_NONE : MANUAL_LIST_MARK_NEW;
                title->listParam.count++;
            }
        }
        title->listParam.scroll = title->param->scroll;
        title->listParam.cursor = title->param->cursor;
    } else {
        u16 count;

        title->listParam.type = MANUAL_LIST_TITLE;
        count = ManualData_GetTitleCount(title->common->data);
        title->listParam.count = 0;
        title->listParam.items =
            GFL_HeapAllocate(title->common->heapId, count * sizeof(ManualListItem), TRUE, "manual_title.c", 178);
        for (i = 0; i < count; i++) {
            u16 condition = ManualData_GetTitleCondition(title->common->data, i);

            if (ManualData_IsOpen(title->common->data, condition, title->common->gameData)) {
                u16 flag = ManualData_GetTitleReadFlag(title->common->data, i);
                BOOL read = ManualData_IsRead(title->common->data, flag, title->common->gameData);

                title->listParam.items[title->listParam.count].id = i;
                title->listParam.items[title->listParam.count].msg = ManualData_GetTitleMsg(title->common->data, i);
                title->listParam.items[title->listParam.count].mark =
                    read ? MANUAL_LIST_MARK_NONE : MANUAL_LIST_MARK_NEW;
                title->listParam.count++;
            }
        }
        title->listParam.scroll = title->param->scroll;
        title->listParam.cursor = title->param->cursor;
    }
    title->list = ManualList_Create(&title->listParam, common);
    title->vblankTask = GFL_VBlankTCBAdd(ManualTitle_VBlank, title, 1);
    return title;
}

void ManualTitle_Free(ManualTitle *title) {
    GFL_TCBRemove(title->vblankTask);
    title->param->scroll = title->listParam.scroll;
    title->param->cursor = title->listParam.cursor;
    if (title->listParam.result == MANUAL_LIST_RESULT_RETURN) {
        title->param->result = MANUAL_LIST_RESULT_RETURN;
    } else if (title->listParam.result == MANUAL_LIST_RESULT_DECIDE) {
        title->param->result = MANUAL_LIST_RESULT_DECIDE;
        title->param->selected = title->listParam.items[title->param->cursor].id;
    }
    ManualList_Free(title->list);
    GFL_HeapFree(title->listParam.items);
    GFL_HeapFree(title);
}

// Returns whether the screen has ended
BOOL ManualTitle_Main(ManualTitle *title) {
    return ManualList_Main(title->list);
}

static void ManualTitle_VBlank(TCB *tcb, void *data) {
}
