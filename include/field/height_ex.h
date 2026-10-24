#ifndef POKEBW2_FIELD_HEIGHT_EX_H
#define POKEBW2_FIELD_HEIGHT_EX_H

#include "types.h"
#include "struct_decls.h"

// height_ex.c, overlay 36: the event of a communication error on the field, which fieldmap_event.c starts when the
// game's communication has failed. It shows the error and returns to the field
GameEvent *EventCommError_Create(GameSystem *gsys, Field *field);

// An entry of the list of HeightEx_CreateTable: a rectangle of the grid, which HeightEx_IsPointInArea tests a position
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
