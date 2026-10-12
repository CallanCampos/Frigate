#ifndef POKEBW2_APP_BEACON_DETAIL_H
#define POKEBW2_APP_BEACON_DETAIL_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The screen of a beacon's details, overlay 308 (beacon_detail_gra.c)
#define OVERLAY_BEACON_DETAIL OVERLAY_ID(308)

typedef struct {
    GameSystem *gsys;
    GameData *gameData;
    u8 subscreen;
    u8 mode;
    // Set when the field is to return to the default subscreen
    u8 unk0A;
    u8 unk0B;
} BeaconDetailParam;

extern const GameProcFunctions BeaconDetail_ProcFunctions;

#endif // POKEBW2_APP_BEACON_DETAIL_H
