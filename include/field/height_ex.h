#ifndef POKEBW2_FIELD_HEIGHT_EX_H
#define POKEBW2_FIELD_HEIGHT_EX_H

#include "types.h"
#include "struct_decls.h"

// height_ex.c, overlay 36: the event of a communication error on the field, which fieldmap_event.c starts when the
// game's communication has failed. It shows the error and returns to the field
GameEvent *func_ov036_021ba6c8(GameSystem *gsys, Field *field);

// An entry of the list of func_ov036_021ba5d0: a rectangle of the grid, which func_ov036_021ba594 tests a position
// against
typedef struct HeightExEntry {
    s32 x;
    s32 z;
    s32 width;
    s32 depth;
    s32 value;
    BOOL inUse;
    s32 flags;
} HeightExEntry;

#endif // POKEBW2_FIELD_HEIGHT_EX_H
