#ifndef POKEBW2_APP_MANUAL_TITLE_H
#define POKEBW2_APP_MANUAL_TITLE_H

// The manual's list of titles (overlay 319), named after the ROM's "manual_title.c"

#include "types.h"
#include "app/manual_common.h"

typedef struct ManualTitle ManualTitle;

// Which titles the list has: those of a category, or all
enum {
    MANUAL_TITLE_MODE_CATEGORY,
    MANUAL_TITLE_MODE_ALL,
};

typedef struct {
    u32 mode;
    u16 category;
    u16 scroll;
    u16 cursor;
    // A MANUAL_LIST_RESULT_*, and the title that was picked
    u32 result;
    u16 selected;
} ManualTitleParam;

ManualTitle *ManualTitle_Create(ManualTitleParam *param, ManualCommon *common);
void ManualTitle_Free(ManualTitle *title);
BOOL ManualTitle_Main(ManualTitle *title);

#endif // POKEBW2_APP_MANUAL_TITLE_H
