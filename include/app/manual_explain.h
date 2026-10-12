#ifndef POKEBW2_APP_MANUAL_EXPLAIN_H
#define POKEBW2_APP_MANUAL_EXPLAIN_H

// The pages of a title of the manual (overlay 319), named after the ROM's "manual_explain.c"

#include "types.h"
#include "app/manual_common.h"

typedef struct ManualExplain ManualExplain;

#define MANUAL_EXPLAIN_PAGE_MAX 9

typedef struct {
    u16 image;
    u16 msg;
} ManualExplainPage;

typedef struct {
    u8 pageCount;
    u16 titleMsg;
    ManualExplainPage pages[MANUAL_EXPLAIN_PAGE_MAX];
    u32 result;
} ManualExplainParam;

ManualExplain *ManualExplain_Create(ManualExplainParam *param, ManualCommon *common);
void ManualExplain_Free(ManualExplain *explain);
BOOL ManualExplain_Main(ManualExplain *explain);

#endif // POKEBW2_APP_MANUAL_EXPLAIN_H
