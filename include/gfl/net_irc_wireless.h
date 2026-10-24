#ifndef POKEBW2_GFL_NET_IRC_WIRELESS_H
#define POKEBW2_GFL_NET_IRC_WIRELESS_H

#include "types.h"
#include "gfl/net.h"
#include "struct_decls.h"

// Connecting over infrared first and then wirelessly: the machines meet over infrared, swap MAC addresses, and then
// find each other's wireless beacons by them

// Starts it in place of a GFL_NET_TYPE 4 network, returning the init data to start the infrared network with
const GFLNetInitData *NetIw_Start(const GFLNetInitData *pNetInit, void *work);
void NetIw_Free(void);
void NetIw_FreeIfIdle(void);
// Whether it is running
BOOL NetIw_IsRunning(void);
void NetIw_SetPeerFilter(GFLNetPeerFilter filter);
void (*NetIw_SetCallbacks(void (*a0)(void *work), void (*a1)(void *work, BOOL a1), void (*a2)(void *work)))(void *work);
// Adds a MAC address to the ones collected
void NetIw_AddCollectedMac(const u8 *mac);
// A collected MAC address
void NetIw_GetCollectedMac(u8 *mac, int index);
// Steps it each frame
void NetIw_Update(void);
// The command handler for the MAC addresses another machine collected
void NetIw_RecvCollectedMacs(int netId, int size, const void *data, void *work, NetHandle *handle);
BOOL NetIw_FinishCollecting(void);
void NetIw_OverrideNetId(int netId);
u8 NetIw_GetLinkedCount(void);
void NetIw_SetGameCommandBase(int gameCommandBase);

#endif // POKEBW2_GFL_NET_IRC_WIRELESS_H
