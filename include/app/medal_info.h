#ifndef POKEBW2_APP_MEDAL_INFO_H
#define POKEBW2_APP_MEDAL_INFO_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "struct_decls.h"

// Overlay 187's screen of a Join Avenue visitor's medals, whose files are medal_info_beacon.c and
// medal_info_viewer.c
#define OVERLAY_MEDAL_INFO OVERLAY_ID(187)

// What the screen of a Join Avenue visitor's medals (medal_info_beacon.c's first table) gets
typedef struct {
    // 0 from the Join Avenue, 1 from the Trainer Card, which adds a button to return to the field
    u32 mode;
    // The heap that the screen's own heap is a child of
    u16 parentHeapId;
    StrBuf *name;
    u16 medal;
    u16 medalCount;
    u8 rank;
    // The date, as the visitor's data packs it
    u8 year;
    u8 month;
    u8 day;
    // Set to 1 when the player chose to return to the field
    u32 result;
    GameSystem *gsys;
    GameData *gameData;
    StrBuf *nameBuf;
} MedalInfoParam;

// The screen's process, suggested name MedalInfo_ProcFunctions
extern GameProcFunctions MedalInfo_ProcFunctions;
// The first table's init function
BOOL MedalInfo_Init(GameProc *proc, u32 *state, void *param, void *work);
// The second table's main function
BOOL MedalInfoBeacon_LogMain(GameProc *proc, u32 *state, void *param, void *work);

// What the screen of the medals of the senders of the beacon log (medal_info_beacon.c's second table) gets
typedef struct {
    GameSystem *gsys;
    GameData *gameData;
    // The beacon of the log that the screen shows first
    u8 beaconIndex;
    // Set when the player chose to return to the field
    u8 returnToField;
    // The row of the log that the beacon was in; the log is scrolled to keep that beacon in view
    u8 logRow;
    u8 unk0B;
} MedalInfoBeaconParam;

// The beacon log's screen, suggested name MedalInfoBeacon_ProcFunctions
extern GameProcFunctions MedalInfoBeacon_ProcFunctions;

#endif // POKEBW2_APP_MEDAL_INFO_H
