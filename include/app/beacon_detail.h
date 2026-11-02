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
    // The beacon's place in the log
    u8 logIndex;
    // The row of the log's list that the beacon was in
    u8 logRow;
    // Set when the screen closed for the low battery that turns the C-Gear off: the field then returns to the
    // default subscreen
    u8 lowBatteryExit;
    u8 unk0B;
} BeaconDetailParam;

extern const GameProcFunctions BeaconDetail_ProcFunctions;

#endif // POKEBW2_APP_BEACON_DETAIL_H
