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
// Changes the darkness at the next update: 0 dark, 1 widening as Flash is used, 2 lit, 3 off. The state is 3 once the
// widening has ended
void func_ov036_021c1c68(FieldFlash *flash, u32 nextState);
u16 func_ov036_021c1c6c(FieldFlash *flash);

#endif // POKEBW2_FIELD_FIELD_FLASH_H
