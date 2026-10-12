#ifndef POKEBW2_APP_MANUAL_LIST_H
#define POKEBW2_APP_MANUAL_LIST_H

// The manual's list of buttons with a scroll bar (overlay 319), named after the ROM's "manual_list.c". The category
// and title screens are each one

#include "types.h"
#include "app/manual_common.h"

typedef struct ManualList ManualList;

enum {
    MANUAL_LIST_CATEGORY,
    MANUAL_LIST_TITLE,
};

enum {
    MANUAL_LIST_RESULT_RETURN,
    MANUAL_LIST_RESULT_DECIDE,
};

enum {
    MANUAL_LIST_MARK_NONE,
    MANUAL_LIST_MARK_NEW,
};

typedef struct {
    u16 id;
    u16 msg;
    u32 mark;
} ManualListItem;

typedef struct {
    u32 type;
    u16 count;
    ManualListItem *items;
    u16 scroll;
    u16 cursor;
    u32 result;
} ManualListParam;

ManualList *ManualList_Create(ManualListParam *param, ManualCommon *common);
void ManualList_Free(ManualList *list);
// Returns whether the list has ended
BOOL ManualList_Main(ManualList *list);

#endif // POKEBW2_APP_MANUAL_LIST_H
