#ifndef POKEBW2_BATTLE_VS_MULTI_LIST_H
#define POKEBW2_BATTLE_VS_MULTI_LIST_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 171, vs_multi_list.c: the screen before a multi battle that lists the Pokémon of both teams, which the
// battle proc (overlay 10) queues

typedef struct {
    // The teams, each side's three first Pokémon showing
    PokeParty *party0;
    PokeParty *party1;
    // Whether the teams are in the opposite places, which also sends the lists in from the opposite sides
    u32 flipped;
} VsMultiListParam;

extern const GameProcFunctions VS_MULTI_LIST_PROC_FUNCTIONS;

#endif // POKEBW2_BATTLE_VS_MULTI_LIST_H
