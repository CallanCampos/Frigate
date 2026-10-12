#ifndef POKEBW2_APP_PKWD_RECORD_MONITOR_H
#define POKEBW2_APP_PKWD_RECORD_MONITOR_H

// Pokéstar Studios' record monitor, overlay 325: the list of the movies the player has filmed, with the best box
// office, the cast and the stars of the movie under the cursor. Named after the ROM's
// "pkwd_record_monitor_graphic.c", the other file of the overlay (a guess). Overlay 23's event_pokewood.c runs it

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

#define OVERLAY_PKWD_RECORD_MONITOR OVERLAY_ID(325)

// The heap of the screen (ours, not from swan)
#define HEAPID_PKWD_RECORD_MONITOR 0xa9

typedef struct {
    PlayerInfo *playerInfo;
    PokewoodSave *save;
} PkwdRecordMonitorParam;

extern const GameProcFunctions PKWD_RECORD_MONITOR_PROC_FUNCTIONS;

#endif // POKEBW2_APP_PKWD_RECORD_MONITOR_H
