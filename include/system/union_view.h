#ifndef POKEBW2_SYSTEM_UNION_VIEW_H
#define POKEBW2_SYSTEM_UNION_VIEW_H

#include "types.h"

// The looks a player can have in the Union Room (union_view.c, a guessed name). The names are ours. A view is 0 to 7
// for a male player and 8 to 15 for a female one

// The trainer type of a view, the first view's for one out of range
u16 UnionView_GetTrainerType(u32 view);
// A value of a view, 2 out of range; UnionView_FindTrainerClass finds the view with a value
u8 UnionView_GetTrainerClass(u32 view);
// A second value of a view, 2 out of range. It equals UnionView_GetTrainerClass's for every view
u8 UnionView_GetClassNameId(u32 view);
// The view with a trainer type, or 0
u32 UnionView_FindTrainerType(u32 trainerType);
// The view whose UnionView_GetTrainerClass value is the given one, or 0
u32 UnionView_FindTrainerClass(u32 value);

#endif // POKEBW2_SYSTEM_UNION_VIEW_H
