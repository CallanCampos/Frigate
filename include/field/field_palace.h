#ifndef POKEBW2_FIELD_FIELD_PALACE_H
#define POKEBW2_FIELD_FIELD_PALACE_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

typedef struct FieldPalaceSys {
    GameSystem *gsys;
    Field *field;
    u8 *luminanceTable;
} FieldPalaceSys;

FieldPalaceSys *FieldPalaceSys_Create(HeapID heapId, GameSystem *gsys, Field *field, u16 zoneId);
void FieldPalaceSys_InitPostFX(FieldPalaceSys *sys, u16 zoneId, HeapID heapId);
void FieldPalaceSys_LoadLuminanceTable(FieldPalaceSys *sys, u32 fileId, u16 zoneId, u32 season, HeapID heapId);
void FieldPalaceSys_Free(FieldPalaceSys *sys);
u8 *FieldPalaceSys_GetLuminanceTable(FieldPalaceSys *sys);
BOOL FieldPalaceSys_CheckEventFlag(GameData *gameData, u16 zoneId);

// Overlay 12

#endif // POKEBW2_FIELD_FIELD_PALACE_H
