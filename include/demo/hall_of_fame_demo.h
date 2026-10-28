#ifndef POKEBW2_DEMO_HALL_OF_FAME_DEMO_H
#define POKEBW2_DEMO_HALL_OF_FAME_DEMO_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Hall of Fame entry demo that the game shows once the Champion is beaten, before the credits (overlay 265): each
// Pokémon of the team slides in over its type's particles with its name, level and cry, then the trainer stands with
// the team under falling confetti until a button is pressed. The ROM names none of its files; they are named in the
// way of the Hall of Fame PC's (hall_of_fame_pc.c, hall_of_fame_pc_main.c, ...). Our names
#define OVERLAY_HALL_OF_FAME_DEMO OVERLAY_ID(265)

// The demo's heap, a child of HEAPID_USER
#define HEAPID_HALL_OF_FAME_DEMO 0x8d

typedef struct {
    PokeParty *party;
    PlayerInfo *playerInfo;
    PlayTime *playTime;
} HallOfFameDemoParam;

extern const GameProcFunctions HALL_OF_FAME_DEMO_PROC_FUNCTIONS;

#endif // POKEBW2_DEMO_HALL_OF_FAME_DEMO_H
