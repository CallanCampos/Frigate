#ifndef POKEBW2_SYSTEM_BEACON_STATUS_H
#define POKEBW2_SYSTEM_BEACON_STATUS_H

#include "types.h"
#include "struct_decls.h"
#include "system/game_beacon.h"

// The game's beacon status (beacon_status.c), kept in GameData: among others, the greeting that the player's beacons
// send. msgHeapId is the heap the default greeting's message data is loaded with

#define BEACON_STATUS_RECORD_MAX 8

struct BeaconStatus {
    GameBeaconInfoTable table;
    // Set by the phrase input when the player enters a new greeting, read as a u16
    u8 greetingChanged[2];
    // The top row of the beacon view's list
    u8 logTop;
    // The records reached whose Pass Powers are still to be offered, the oldest first (the beacon view's
    // BeaconStatus_AddRecord, BeaconStatus_GetRecord and BeaconStatus_PopRecord)
    u8 recordCount;
    u8 records[BEACON_STATUS_RECORD_MAX];
    // The Pass Power that a beacon gave and its giver's name
    u32 givenPassPower;
    StrBuf *givenName;
    StrBuf *greeting;
};

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
