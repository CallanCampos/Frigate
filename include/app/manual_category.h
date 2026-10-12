#ifndef POKEBW2_APP_MANUAL_CATEGORY_H
#define POKEBW2_APP_MANUAL_CATEGORY_H

// The manual's list of categories (overlay 319), named after the ROM's "manual_category.c"

#include "types.h"
#include "app/manual_common.h"

typedef struct ManualCategory ManualCategory;

typedef struct {
    u16 scroll;
    u16 cursor;
    // A MANUAL_LIST_RESULT_*, and the category that was picked
    u32 result;
    u16 selected;
} ManualCategoryParam;

ManualCategory *ManualCategory_Create(ManualCategoryParam *param, ManualCommon *common);
void ManualCategory_Free(ManualCategory *category);
BOOL ManualCategory_Main(ManualCategory *category);

#endif // POKEBW2_APP_MANUAL_CATEGORY_H
