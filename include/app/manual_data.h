#ifndef POKEBW2_APP_MANUAL_DATA_H
#define POKEBW2_APP_MANUAL_DATA_H

// The manual's tables (overlay 319), named after the ROM's "manual_data.c": the titles, each with its pages, and the
// categories, each with its titles

#include "types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "struct_decls.h"

typedef struct ManualData ManualData;

// What a title needs before the manual lists it
enum {
    MANUAL_CONDITION_NONE,
    MANUAL_CONDITION_C_GEAR,
    MANUAL_CONDITION_FLAG,
    MANUAL_CONDITION_MEDAL_BOX,
    MANUAL_CONDITION_HABITAT_LIST,
};

// The picture of a page that has none
#define MANUAL_IMAGE_NONE 0xffff

ManualData *ManualData_Create(ArcTool *arc, HeapID heapId);
void ManualData_Free(ManualData *data);
u16 ManualData_GetTitleMsg(ManualData *data, u16 title);
u16 ManualData_GetTitleCondition(ManualData *data, u16 title);
// The flag of the miscellaneous save data that says the title was read
u16 ManualData_GetTitleReadFlag(ManualData *data, u16 title);
u16 ManualData_GetTitlePageCount(ManualData *data, u16 title);
u16 ManualData_GetPageMsg(ManualData *data, u16 title, u16 page);
u16 ManualData_GetPageImage(ManualData *data, u16 title, u16 page);
u16 ManualData_GetTitleCount(ManualData *data);
u16 ManualData_GetCategoryMsg(ManualData *data, u16 category);
u16 ManualData_GetCategoryTitleCount(ManualData *data, u16 category);
u16 ManualData_GetCategoryTitle(ManualData *data, u16 category, u16 index);
u16 ManualData_GetCategoryCount(ManualData *data);
BOOL ManualData_IsOpen(ManualData *data, u16 condition, GameData *gameData);
BOOL ManualData_IsRead(ManualData *data, u16 flag, GameData *gameData);
void ManualData_SetRead(ManualData *data, u16 flag, GameData *gameData);
BOOL ManualData_IsImageValid(ManualData *data, u16 image);
// The picture the upper screen shows outside of a title's pages
u16 ManualData_GetDefaultImage(ManualData *data);

#endif // POKEBW2_APP_MANUAL_DATA_H
