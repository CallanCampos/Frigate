#ifndef POKEBW2_APP_MANUAL_TOP_H
#define POKEBW2_APP_MANUAL_TOP_H

// The manual's first screen (overlay 319), named after the ROM's "manual_top.c": the choice between the categories
// and all the titles

#include "types.h"
#include "app/manual_common.h"

typedef struct ManualTop ManualTop;

enum {
    MANUAL_TOP_RESULT_CLOSE,
    MANUAL_TOP_RESULT_RETURN,
    MANUAL_TOP_RESULT_CATEGORY,
    MANUAL_TOP_RESULT_TITLE,
};

typedef struct {
    u8 cursor;
    u32 result;
} ManualTopParam;

ManualTop *ManualTop_Create(ManualTopParam *param, ManualCommon *common);
void ManualTop_Free(ManualTop *top);
BOOL ManualTop_Main(ManualTop *top);

#endif // POKEBW2_APP_MANUAL_TOP_H
