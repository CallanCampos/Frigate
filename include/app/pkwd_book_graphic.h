#ifndef POKEBW2_APP_PKWD_BOOK_GRAPHIC_H
#define POKEBW2_APP_PKWD_BOOK_GRAPHIC_H

// The BGs and cell actors of Pokéstar Studios' scenario book (overlay 322), named after the ROM's
// "pkwd_book_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct PkwdBookGraphic PkwdBookGraphic;

PkwdBookGraphic *PkwdBookGraphic_Create(u32 layout, HeapID heapId);
void PkwdBookGraphic_Free(PkwdBookGraphic *graphic);
void PkwdBookGraphic_Update(PkwdBookGraphic *graphic);
// The template's 3D frame, which this screen leaves empty
void PkwdBookGraphic_Begin3D(PkwdBookGraphic *graphic);
void PkwdBookGraphic_End3D(PkwdBookGraphic *graphic);
ClActUnit *PkwdBookGraphic_GetClActUnit(PkwdBookGraphic *graphic);

#endif // POKEBW2_APP_PKWD_BOOK_GRAPHIC_H
