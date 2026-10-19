#ifndef POKEBW2_FIELD_UNION_SUBDISP_H
#define POKEBW2_FIELD_UNION_SUBDISP_H

#include "types.h"
#include "gfl/overlay.h"
#include "struct_decls.h"

// The Union Room's touch screen (FLD_SUBSCREEN_ID_UNION_ROOM_STANDBY), overlay 82, named after the ROM's
// "union_subdisp.c": the icons that pick what to do, and the chat of what the players around say, in three lines that
// scroll. Overlay 36's field subscreen table calls these
#define OVERLAY_UNION_SUBDISP OVERLAY_ID(82)

typedef struct UnionSubdisp UnionSubdisp;

UnionSubdisp *UnionSubdisp_Create(GameSystem *gsys);
void UnionSubdisp_Free(UnionSubdisp *work);
// Both take whether the field lets the screen act, which is not while the Union Room system is busy
void UnionSubdisp_Update(UnionSubdisp *work, BOOL active);
void UnionSubdisp_UpdateAltFrame(UnionSubdisp *work, BOOL active);

#endif // POKEBW2_FIELD_UNION_SUBDISP_H
