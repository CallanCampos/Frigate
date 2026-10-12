#ifndef POKEBW2_APP_WBT_LIST_H
#define POKEBW2_APP_WBT_LIST_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// The Pokémon World Tournament's list of Trainers, overlay 320 (which names wbt_list_graphic.c). It runs with a
// WbtSetup of field/wbt.h
#define OVERLAY_WBT_LIST OVERLAY_ID(320)

// Not from swan: the screen's heap
#define HEAPID_WBT_LIST 0x9d

extern const GameProcFunctions WBT_LIST_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WBT_LIST_H
