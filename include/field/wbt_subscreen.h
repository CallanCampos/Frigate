#ifndef POKEBW2_FIELD_WBT_SUBSCREEN_H
#define POKEBW2_FIELD_WBT_SUBSCREEN_H

#include "types.h"
#include "struct_decls.h"

// The Pokémon World Tournament's touch screen (FLD_SUBSCREEN_ID_PWT), overlay 81, named after the ROM's
// "wbt_subscreen.c". Overlay 36's field subscreen table calls these

typedef struct WbtSubscreen WbtSubscreen;

// The first two arguments are unused
WbtSubscreen *WbtSubscreen_Create(void *saveData, FieldSubscreen *subscreen, GameSystem *gsys);
void WbtSubscreen_Update(WbtSubscreen *work);
void WbtSubscreen_Draw(WbtSubscreen *work);
void WbtSubscreen_Free(WbtSubscreen *work);

#endif // POKEBW2_FIELD_WBT_SUBSCREEN_H
