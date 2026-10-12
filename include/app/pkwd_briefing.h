#ifndef POKEBW2_APP_PKWD_BRIEFING_H
#define POKEBW2_APP_PKWD_BRIEFING_H

// Pokéstar Studios' theater, overlay 324: the movie's set on the upper screen and the audience on the lower one. It
// is shown before the battle with the briefing of the scene, and after it with the movie's credits. Named after the
// ROM's "pkwd_briefing.c". Overlay 23's event_pokewood.c runs it

#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

#define OVERLAY_PKWD_BRIEFING OVERLAY_ID(324)

// The heap of the screen (ours, not from swan)
#define HEAPID_PKWD_BRIEFING 0xa4

// The seats of the audience, each a bit of PkwdBriefingParam's audience
#define PKWD_BRIEFING_SEAT_COUNT 22

typedef struct {
    s32 movie;
    // How many seats are taken. The briefing picks them
    u32 audienceCount;
    // The seats that are taken, which the briefing sets and the credits read
    u32 audience;
    PlayerInfo *playerInfo;
    SaveControl *saveControl;
    GameData *gameData;
    // The battle of the movie, for its parties and, in the credits, how it ended
    BtlSetup *setup;
    // Set to keep the B Button from skipping the screen
    u32 noSkip;
    // Set by the screen when it was skipped
    u32 skip;
} PkwdBriefingParam;

extern const GameProcFunctions PKWD_BRIEFING_PROC_FUNCTIONS;
extern const GameProcFunctions PKWD_BRIEFING_CREDITS_PROC_FUNCTIONS;

#endif // POKEBW2_APP_PKWD_BRIEFING_H
