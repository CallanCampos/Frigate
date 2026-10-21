#ifndef POKEBW2_GFL_NET_WHPIPE_H
#define POKEBW2_GFL_NET_WHPIPE_H

#include "types.h"
#include "gfl/wih.h"

// net_whpipe.c, in overlay 30: the GFL net's wireless pipe, over wih.c: beacons, scanning and the connection

// The wireless pipe's state machine: its work begins with the state function that runs next, which takes the work
typedef void (*NetWhpipeStateFunc)(void *work);
typedef struct {
    NetWhpipeStateFunc state;
} NetWhpipeState;

// Sets the state that the pipe's work runs next, as the pipe's files do to move on
void NetWhpipe_SetState(NetWhpipeState *work, NetWhpipeStateFunc state);

// The game service ID of a beacon the scan found
u8 NetWhpipe_GetBeaconService(int index);

// The Funfest's beacon
void NetWhpipe_SetSearchPaused(u32 enabled);
// Whether the Funfest's beacon updates are sent (WH_GetSystemState returns 2)
BOOL NetWhpipe_IsScanning(void);
// Sends the game's beacon
void NetWhpipe_UpdateGameBeacon(void);

// The beacons that deliveries come by
void *NetWhpipe_GetBeaconData(int index);
void NetWhpipe_RemoveBeacon(int index);
void NetWhpipe_RemoveAllBeacons(void);
void NetWhpipe_SetScanTarget(int index);
u8 NetWhpipe_GetBeaconCommandBase(u8 index);
void NetWhpipe_ConsumeBeacon(int index);

// What the device table's functions in net_devwireless.c are built on
BOOL NetWhpipe_Init(HeapID heapId, WHCallback callback, void *work, BOOL flag);
BOOL NetWhpipe_IsIdle(void);
void NetWhpipe_Update(u16 connectBits);
BOOL NetWhpipe_Finalize(void);
void NetWhpipe_ClearBeacons(void);
u8 *NetWhpipe_GetBeaconMac(int index);
BOOL NetWhpipe_HasBeacons(void);
BOOL NetWhpipe_StartParent(BOOL a0);
BOOL NetWhpipe_StartScan(BOOL a0);
BOOL NetWhpipe_StartChild(BOOL a0, const u8 *mac, int index, void (*callback)(void));
BOOL NetWhpipe_SendData(u8 *data, int size, BOOL (*callback)(BOOL ok));
void NetWhpipe_SetRecvCallback(BOOL (*callback)(u16 netId, u8 *data, u16 size));
BOOL NetWhpipe_IsConnected(void);
BOOL NetWhpipe_IsInitialized(void);
int NetWhpipe_GetBitmap(void);
int NetWhpipe_GetAid(void);
BOOL NetWhpipe_IsStopped(void);
void NetWhpipe_SetLockEnabled(int value);
void NetWhpipe_SetScanAcceptAll(BOOL a0);
BOOL NetWhpipe_StartSearch(void);
void NetWhpipe_HoldScan(int unused);
BOOL NetWhpipe_End(WHCallback callback);

// Stops the beacon when it was told to lock
void NetWhpipe_StopIfLocked(void);
// Sets what the scan calls with each beacon it finds and the game's argument
void NetWhpipe_SetScanCallback(void (*callback)(void *arg, const void *beacon, u8 gameCommandBase, u8 childCount),
                               void *arg);

#endif // POKEBW2_GFL_NET_WHPIPE_H
