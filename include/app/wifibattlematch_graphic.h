#ifndef POKEBW2_APP_WIFIBATTLEMATCH_GRAPHIC_H
#define POKEBW2_APP_WIFIBATTLEMATCH_GRAPHIC_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

// Overlay 262, the code that the live and Wi-Fi competition screens of overlay 261 (battle_championship_flow.c,
// digitalcardcheck.c, livebattlematch_flow.c) share. Its three files are named after the ROM's strings. The BGs and
// the cell actor system of the screens

typedef struct WifiBattleMatchGraphic WifiBattleMatchGraphic;

// Sets up the VRAM, the BGs and the cell actor system, and the VBlank task that updates them
WifiBattleMatchGraphic *WifiBattleMatchGraphic_Create(u32 layout, HeapID heapId);
void WifiBattleMatchGraphic_Free(WifiBattleMatchGraphic *graphic);
void WifiBattleMatchGraphic_Update(WifiBattleMatchGraphic *graphic);
// The unit that the screens' cell actors are made in
ClActUnit *WifiBattleMatchGraphic_GetClActUnit(WifiBattleMatchGraphic *graphic);

#endif // POKEBW2_APP_WIFIBATTLEMATCH_GRAPHIC_H
