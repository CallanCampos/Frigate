#include "types.h"
#include "system/union_view.h"

// The 16 looks a player can have in the Union Room, 8 for each gender, and their trainer types. The file's name is a
// guess: the ROM has no string for it

typedef struct {
    u16 trainerType;
    // The message of the trainer class's name, and the trainer class of the graphics
    u8 classNameId;
    u8 trainerClass;
} UnionView;

static const UnionView sUnionViews[] = {
    { 11, 2, 2 },   { 30, 50, 50 },  { 36, 24, 24 }, { 34, 17, 17 },  { 73, 52, 52 }, { 64, 74, 74 },
    { 61, 57, 57 }, { 153, 15, 15 }, { 15, 3, 3 },   { 31, 49, 49 },  { 37, 25, 25 }, { 35, 18, 18 },
    { 74, 46, 46 }, { 84, 65, 65 },  { 51, 62, 62 }, { 154, 14, 14 },
};

u16 UnionView_GetTrainerType(u32 view) {
    if (view >= NELEMS(sUnionViews)) {
        return 11;
    }
    return sUnionViews[view].trainerType;
}

u8 UnionView_GetTrainerClass(u32 view) {
    if (view >= NELEMS(sUnionViews)) {
        return 2;
    }
    return sUnionViews[view].trainerClass;
}

u8 UnionView_GetClassNameId(u32 view) {
    if (view >= NELEMS(sUnionViews)) {
        return 2;
    }
    return sUnionViews[view].classNameId;
}

u32 UnionView_FindTrainerType(u32 trainerType) {
    u32 i;

    for (i = 0; i < NELEMS(sUnionViews); i++) {
        if (trainerType == sUnionViews[i].trainerType) {
            return i;
        }
    }
    return 0;
}

u32 UnionView_FindTrainerClass(u32 value) {
    u32 i;

    for (i = 0; i < NELEMS(sUnionViews); i++) {
        if (value == sUnionViews[i].trainerClass) {
            return i;
        }
    }
    return 0;
}
