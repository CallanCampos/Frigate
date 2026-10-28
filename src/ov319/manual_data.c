#include "types.h"
#include "app/manual.h"
#include "app/manual_data.h"
#include "constants/flags.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "save/c_gear_savedata.h"
#include "save/event_work.h"
#include "save/medal_box.h"
#include "save/misc_savedata.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The manual's tables, named after the ROM's "manual_data.c". The titles and the categories are each a file of
// records of 16-bit words, with a file of the records' offsets after their count

// The words of a title: after these, the message and the picture of each page
enum {
    TITLE_MSG = 3,
    TITLE_CONDITION,
    TITLE_READ_FLAG,
    TITLE_PAGE_COUNT,
    TITLE_PAGES,
};

// The words of a category: after these, its titles
enum {
    CATEGORY_MSG = 1,
    CATEGORY_TITLE_COUNT,
    CATEGORY_TITLES,
};

// The picture of the manual's cover
#define DEFAULT_IMAGE 85

// The offsets of a file's records
typedef struct {
    u16 count;
    u16 offsets[1];
} ManualOffsets;

struct ManualData {
    u8 *titles;
    ManualOffsets *titleOffsets;
    u8 *categories;
    ManualOffsets *categoryOffsets;
};

static u16 *ManualData_GetTitle(ManualData *data, u16 title);
static u16 ManualData_GetTitleOffset(ManualData *data, u16 title);
static u16 *ManualData_GetCategory(ManualData *data, u16 category);
static u16 ManualData_GetCategoryOffset(ManualData *data, u16 category);

ManualData *ManualData_Create(ArcTool *arc, HeapID heapId) {
    u32 size;
    ManualData *data = GFL_HeapAllocate(heapId, sizeof(ManualData), TRUE, "manual_data.c", 139);

    data->titles = GFL_ArcToolReadHeapNewLZGetLen(arc, MANUAL_ARC_TITLE_DATA, FALSE, heapId, &size);
    data->titleOffsets = GFL_ArcToolReadHeapNewLZGetLen(arc, MANUAL_ARC_TITLE_OFFSETS, FALSE, heapId, &size);
    data->categories = GFL_ArcToolReadHeapNewLZGetLen(arc, MANUAL_ARC_CATEGORY_DATA, FALSE, heapId, &size);
    data->categoryOffsets = GFL_ArcToolReadHeapNewLZGetLen(arc, MANUAL_ARC_CATEGORY_OFFSETS, FALSE, heapId, &size);
    return data;
}

void ManualData_Free(ManualData *data) {
    GFL_HeapFree(data->categoryOffsets);
    GFL_HeapFree(data->categories);
    GFL_HeapFree(data->titleOffsets);
    GFL_HeapFree(data->titles);
    GFL_HeapFree(data);
}

u16 ManualData_GetTitleMsg(ManualData *data, u16 title) {
    return ManualData_GetTitle(data, title)[TITLE_MSG];
}

u16 ManualData_GetTitleCondition(ManualData *data, u16 title) {
    return ManualData_GetTitle(data, title)[TITLE_CONDITION];
}

u16 ManualData_GetTitleReadFlag(ManualData *data, u16 title) {
    return ManualData_GetTitle(data, title)[TITLE_READ_FLAG];
}

u16 ManualData_GetTitlePageCount(ManualData *data, u16 title) {
    return ManualData_GetTitle(data, title)[TITLE_PAGE_COUNT];
}

u16 ManualData_GetPageMsg(ManualData *data, u16 title, u16 page) {
    return ManualData_GetTitle(data, title)[TITLE_PAGES + page * 2];
}

u16 ManualData_GetPageImage(ManualData *data, u16 title, u16 page) {
    return ManualData_GetTitle(data, title)[TITLE_PAGES + 1 + page * 2];
}

u16 ManualData_GetTitleCount(ManualData *data) {
    return data->titleOffsets->count;
}

u16 ManualData_GetCategoryMsg(ManualData *data, u16 category) {
    return ManualData_GetCategory(data, category)[CATEGORY_MSG];
}

u16 ManualData_GetCategoryTitleCount(ManualData *data, u16 category) {
    return ManualData_GetCategory(data, category)[CATEGORY_TITLE_COUNT];
}

u16 ManualData_GetCategoryTitle(ManualData *data, u16 category, u16 index) {
    return ManualData_GetCategory(data, category)[CATEGORY_TITLES + index];
}

u16 ManualData_GetCategoryCount(ManualData *data) {
    return data->categoryOffsets->count;
}

BOOL ManualData_IsOpen(ManualData *data, u16 condition, GameData *gameData) {
    BOOL open = FALSE;

    switch (condition) {
    case MANUAL_CONDITION_NONE:
        open = TRUE;
        break;
    case MANUAL_CONDITION_C_GEAR:
        if (CGearSettings_IsOn(CGearSettings_GetFromSave(GameData_GetSaveControl(gameData)))) {
            open = TRUE;
        }
        break;
    case MANUAL_CONDITION_FLAG:
        if (EventWork_FlagGet(GameData_GetEventWork(gameData), EVENT_FLAG_0x0960)) {
            open = TRUE;
        }
        break;
    case MANUAL_CONDITION_MEDAL_BOX:
        if (MedalBox_GetMedalStatus(SaveControl_GetMedalBox(GameData_GetSaveControl(gameData)), 0) == 4) {
            open = TRUE;
        }
        break;
    case MANUAL_CONDITION_HABITAT_LIST:
        if (PokeDex_IsHabitatListEnabled(GameData_GetPokedex(gameData)) == TRUE) {
            open = TRUE;
        }
        break;
    }
    return open;
}

BOOL ManualData_IsRead(ManualData *data, u16 flag, GameData *gameData) {
    return Misc_GetFlag(getHollow_RivalData(GameData_GetSaveControl(gameData)), flag);
}

void ManualData_SetRead(ManualData *data, u16 flag, GameData *gameData) {
    Misc_SetFlag(getHollow_RivalData(GameData_GetSaveControl(gameData)), flag);
}

BOOL ManualData_IsImageValid(ManualData *data, u16 image) {
    if (image != MANUAL_IMAGE_NONE) {
        return TRUE;
    }
    return FALSE;
}

u16 ManualData_GetDefaultImage(ManualData *data) {
    return DEFAULT_IMAGE;
}

static u16 *ManualData_GetTitle(ManualData *data, u16 title) {
    return (u16 *)(data->titles + ManualData_GetTitleOffset(data, title));
}

static u16 ManualData_GetTitleOffset(ManualData *data, u16 title) {
    return data->titleOffsets->offsets[title];
}

static u16 *ManualData_GetCategory(ManualData *data, u16 category) {
    return (u16 *)(data->categories + ManualData_GetCategoryOffset(data, category));
}

static u16 ManualData_GetCategoryOffset(ManualData *data, u16 category) {
    return data->categoryOffsets->offsets[category];
}
