#ifndef POKEBW2_APP_TOWNMAP_H
#define POKEBW2_APP_TOWNMAP_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// Overlay 144, the town map (townmap.c, townmap_grh.c, and townmap_data.c in app/townmap/, whose table of places other
// overlays load too). The ROM names townmap.c and townmap_grh.c; the names of their functions and types are ours

#define OVERLAY_TOWNMAP OVERLAY_ID(144)

// The town map's heap, a child of HEAPID_USER
#define HEAPID_TOWNMAP 0x44

// What the town map is opened for, the mode of its parameters
enum {
    // Picks where to fly
    TOWNMAP_MODE_FLY,
    // Shows the map
    TOWNMAP_MODE_VIEW,
    // Picks where to fly among all the places, arrived at or not
    TOWNMAP_MODE_FLY_ALL,
};

// How the town map was left, the result in its parameters
enum {
    // With the return button
    TOWNMAP_RESULT_RETURN,
    // With the close button
    TOWNMAP_RESULT_CLOSE,
    // With a place to fly to
    TOWNMAP_RESULT_FLY,
};

// townmap.c
extern const GameProcFunctions TOWNMAP_PROC_FUNCTIONS;

// townmap_grh.c: the town map's BGs and cell actors

typedef struct TownMapGraphic TownMapGraphic;

// The BGs, as TownMapGraphic_GetBG numbers them
enum {
    // The touch bar's, BG0
    TOWNMAP_BG_BAR,
    // The place's name's and the message's, BG1
    TOWNMAP_BG_TEXT,
    // The two layers of the map, BG2 and BG3, which zoom and scroll together
    TOWNMAP_BG_MAP_FRONT,
    TOWNMAP_BG_MAP_BACK,
    // The upper screen's: BG6, which nothing draws on, the place's description on BG4, and the background on BG5
    TOWNMAP_BG_SUB_FRONT,
    TOWNMAP_BG_INFO,
    TOWNMAP_BG_SUB_BACK,
    TOWNMAP_BG_COUNT
};

enum {
    TOWNMAP_UNIT_MAIN,
    TOWNMAP_UNIT_COUNT
};

// The actors the graphics create, all hidden at first
enum {
    // The plate behind the place's name
    TOWNMAP_ACTOR_NAME_PLATE,
    TOWNMAP_ACTOR_CURSOR,
    // The ring around the place the cursor points at
    TOWNMAP_ACTOR_RING,
    // In the player marker's animation; townmap.c makes its own
    TOWNMAP_ACTOR_PLAYER,
    TOWNMAP_ACTOR_COUNT
};

TownMapGraphic *TownMapGraphic_Create(HeapID heapId);
void TownMapGraphic_Free(TownMapGraphic *graphic);
void TownMapGraphic_Update(TownMapGraphic *graphic);
// The BG of a TOWNMAP_BG_*
u8 TownMapGraphic_GetBG(TownMapGraphic *graphic, u32 index);
ClActUnit *TownMapGraphic_GetUnit(TownMapGraphic *graphic, u32 index);
ClActor *TownMapGraphic_GetActor(TownMapGraphic *graphic, u32 index);
// The resources of the map's actors, for more actors of them
void TownMapGraphic_GetRes(TownMapGraphic *graphic, u32 *chars, u32 *cellAnims, u32 *pltt);

#endif // POKEBW2_APP_TOWNMAP_H
