#ifndef POKEBW2_SYSTEM_COMM_PLAYER_SUPPORT_H
#define POKEBW2_SYSTEM_COMM_PLAYER_SUPPORT_H

#include "types.h"
#include "gfl/heap.h"
#include "save/player_info.h"
#include "struct_decls.h"

// The support that another player gives over communication, which the game data keeps: the next battle heals one of
// the player's Pokémon once, and names the player who gave it. Our names

// The support given
enum {
    COMM_PLAYER_SUPPORT_NONE,
    // Heals half or all of a Pokémon's HP
    COMM_PLAYER_SUPPORT_HEAL_HALF,
    COMM_PLAYER_SUPPORT_HEAL_FULL,
    // The battle has used it
    COMM_PLAYER_SUPPORT_USED,
};

CommPlayerSupport *CommPlayerSupport_Create(HeapID heapId);
void CommPlayerSupport_Free(CommPlayerSupport *support);
void CommPlayerSupport_Init(CommPlayerSupport *support);
// The player who gave the support
PlayerInfo *CommPlayerSupport_GetSupporter(CommPlayerSupport *support);
u32 CommPlayerSupport_GetType(CommPlayerSupport *support);
// Marks the support used, keeping what it was and who gave it
void CommPlayerSupport_SetUsed(CommPlayerSupport *support);
// Clears the support after a battle, keeping what it was
void CommPlayerSupport_EndBattle(CommPlayerSupport *support);

#endif // POKEBW2_SYSTEM_COMM_PLAYER_SUPPORT_H
