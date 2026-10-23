#ifndef POKEBW2_APP_GTSNEGO_H
#define POKEBW2_APP_GTSNEGO_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

#define OVERLAY_GTSNEGO OVERLAY_ID(195)

// What the search leaves in GtsNegoParam's result
enum {
    GTSNEGO_RESULT_RETRY_LOGIN,
    GTSNEGO_RESULT_EXIT,
    // A partner is being searched for or was found: anyone, or one of the people of the Pal Pad
    GTSNEGO_RESULT_ANYONE,
    GTSNEGO_RESULT_RENDEZVOUS,
};

#define GTSNEGO_MET_COUNT 100

// What a player searched with: unk0 is the level range, unk4 the kind of their Pokémon
typedef struct {
    u32 unk0;
    u32 unk4;
} GtsNegoUnk4;

typedef struct {
    GameData *gameData;
    // For each of the two players
    GtsNegoUnk4 unk4[2];
    // Copies of the player's info
    PlayerInfo *playerInfo;
    PlayerInfo *playerInfo2;
    // The IDs of the partners found before, which are not matched again, and where the next one goes
    u32 metIds[GTSNEGO_MET_COUNT];
    int metIndex;
    // A GTSNEGO_RESULT_*
    u32 result;
} GtsNegoParam;

extern const GameProcFunctions GTSNEGO_PROC_FUNCTIONS;

#endif // POKEBW2_APP_GTSNEGO_H
