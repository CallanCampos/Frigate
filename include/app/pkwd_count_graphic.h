#ifndef POKEBW2_APP_PKWD_COUNT_GRAPHIC_H
#define POKEBW2_APP_PKWD_COUNT_GRAPHIC_H

// The BGs, cell actors and palette fades of the Pokéstar Studios movie's opening (overlay 321), named after the ROM's
// "pkwd_count_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

typedef struct PkwdCountGraphic PkwdCountGraphic;

PkwdCountGraphic *PkwdCountGraphic_Create(u32 layout, HeapID heapId);
void PkwdCountGraphic_Free(PkwdCountGraphic *graphic);
void PkwdCountGraphic_Update(PkwdCountGraphic *graphic);
// Both do nothing
void PkwdCountGraphic_Unused1(PkwdCountGraphic *graphic);
void PkwdCountGraphic_Unused2(PkwdCountGraphic *graphic);
ClActUnit *PkwdCountGraphic_GetClActUnit(PkwdCountGraphic *graphic);
PaletteFade *PkwdCountGraphic_GetPaletteFade(PkwdCountGraphic *graphic);

#endif // POKEBW2_APP_PKWD_COUNT_GRAPHIC_H
