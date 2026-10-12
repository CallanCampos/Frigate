#ifndef POKEBW2_APP_PKWD_SELECTSCREEN_H
#define POKEBW2_APP_PKWD_SELECTSCREEN_H

// Pokéstar Studios' movie select screen, overlay 323: the list of the eight screens of the theater and the movies
// that play on them, to watch one or to take one out. Named after the ROM's "pkwd_selectscreen.c". Overlay 23's
// event_pokewood.c runs it

#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

#define OVERLAY_PKWD_SELECTSCREEN OVERLAY_ID(323)

// The heap of the screen (ours, not from swan)
#define HEAPID_PKWD_SELECTSCREEN 0xa1

// What the screen leaves in the result when the player backs out, rather than a screen of the theater (0 to 7)
#define PKWD_SELECTSCREEN_CANCEL 8

// Whether a picked movie is watched, or removed from the theater
enum {
    PKWD_SELECTSCREEN_MODE_WATCH,
    PKWD_SELECTSCREEN_MODE_REMOVE,
};

typedef struct {
    PlayerInfo *playerInfo;
    SaveControl *saveControl;
    // The screen of the picked movie, or PKWD_SELECTSCREEN_CANCEL
    u16 *result;
    PokewoodSave *save;
    u32 mode;
} PkwdSelectScreenParam;

extern const GameProcFunctions PKWD_SELECTSCREEN_PROC_FUNCTIONS;

#endif // POKEBW2_APP_PKWD_SELECTSCREEN_H
