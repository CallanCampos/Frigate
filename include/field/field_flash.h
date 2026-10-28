#ifndef POKEBW2_FIELD_FIELD_FLASH_H
#define POKEBW2_FIELD_FIELD_FLASH_H

// Overlay 36's field_flash.c: the darkness of a dark cave around the player, a sphere that Flash widens and fades

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

FieldFlash *FieldFlash_Create(HeapID heapId);
void FieldFlash_Free(FieldFlash *flash);
void FieldFlash_Update(FieldFlash *flash);
void FieldFlash_Draw(FieldFlash *flash);
// What FieldFlash_RequestState asks for, at the next update
enum {
    FIELD_FLASH_REQUEST_DARK,
    FIELD_FLASH_REQUEST_WIDEN,
    FIELD_FLASH_REQUEST_LIT,
    FIELD_FLASH_REQUEST_OFF,
    FIELD_FLASH_REQUEST_NONE,
};

// FieldFlash_GetState's result: the widening ends lit
enum {
    FIELD_FLASH_STATE_OFF,
    FIELD_FLASH_STATE_DARK,
    FIELD_FLASH_STATE_WIDENING,
    FIELD_FLASH_STATE_LIT,
};

// Changes the darkness at the next update: dark, widening as Flash is used, lit, or off
void FieldFlash_RequestState(FieldFlash *flash, u32 request);
u16 FieldFlash_GetState(FieldFlash *flash);

#endif // POKEBW2_FIELD_FIELD_FLASH_H
