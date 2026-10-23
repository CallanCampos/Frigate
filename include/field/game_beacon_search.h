#ifndef POKEBW2_FIELD_GAME_BEACON_SEARCH_H
#define POKEBW2_FIELD_GAME_BEACON_SEARCH_H

// Overlay 12's game_beacon_search.c: the game communication that looks for nearby players' beacons while the player is
// in the field, which the communication table of ARM9 main runs

#include "types.h"
#include "struct_decls.h"

void *GameBeaconSearch_Init(u32 *seq, void *param);
BOOL GameBeaconSearch_InitWait(u32 *seq, void *param, void *work);
BOOL GameBeaconSearch_Exit(u32 *seq, void *param, void *work);
BOOL GameBeaconSearch_ExitWait(u32 *seq, void *param, void *work);
void GameBeaconSearch_Main(u32 *seq, void *param, void *work);
// Waits a while before taking the beacons of type 0x35 again
void GameBeaconSearch_StartType35Wait(BeaconSearchWork *work);

#endif // POKEBW2_FIELD_GAME_BEACON_SEARCH_H
