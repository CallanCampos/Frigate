#ifndef POKEBW2_FIELD_PLACE_NAME_LETTER_H
#define POKEBW2_FIELD_PLACE_NAME_LETTER_H

// Overlay 36's place_name_letter.c, not written yet: the two animated parts of the place name banner, the zone's name
// and its frame. Create one of a type, free it, set it up, update it, slide it in, slide it out, hide it, and whether
// it has finished

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

PlaceNameLetter *func_ov036_021b6824(HeapID heapId, u32 type, void *setup, ClActUnit *unit, ArcTool *arc,
                                     u16 bgPriority, u16 palette, u16 chars, u16 cellAnims);
void func_ov036_021b68bc(PlaceNameLetter *letter);
void func_ov036_021b68d8(PlaceNameLetter *letter, void *setup);
void func_ov036_021b68e8(PlaceNameLetter *letter);
void func_ov036_021b6900(PlaceNameLetter *letter, s16 param0, u16 param1);
void func_ov036_021b6910(PlaceNameLetter *letter, s16 param0, u16 param1);
void func_ov036_021b6920(PlaceNameLetter *letter);
BOOL func_ov036_021b6940(PlaceNameLetter *letter);

#endif // POKEBW2_FIELD_PLACE_NAME_LETTER_H
