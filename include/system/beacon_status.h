#ifndef POKEBW2_SYSTEM_BEACON_STATUS_H
#define POKEBW2_SYSTEM_BEACON_STATUS_H

#include "types.h"
#include "struct_decls.h"
#include "system/game_beacon.h"

// The game's beacon status (beacon_status.c), kept in GameData: among others, the greeting that the player's beacons
// send. msgHeapId is the heap the default greeting's message data is loaded with

BeaconStatus *BeaconStatus_Create(HeapID heapId, HeapID msgHeapId);
void BeaconStatus_Free(BeaconStatus *status);
// Returns the status, which the log of received beacons (game_beacon.h's GameBeaconInfoTable_GetCount) is at the start
// of
GameBeaconInfoTable *BeaconStatus_GetInfoTable(BeaconStatus *status);
u8 BeaconStatus_GetLogTop(BeaconStatus *status);
void BeaconStatus_SetLogTop(BeaconStatus *status, u8 value);
StrBuf *BeaconStatus_GetGreeting(BeaconStatus *status);
u8 *BeaconStatus_GetGreetingChanged(BeaconStatus *status);

#endif // POKEBW2_SYSTEM_BEACON_STATUS_H
