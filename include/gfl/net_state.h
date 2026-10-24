#ifndef POKEBW2_GFL_NET_STATE_H
#define POKEBW2_GFL_NET_STATE_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"

// The state of the connection: a function for each state that runs every frame, for the parent and the children
// of a wireless connection, Wi-Fi, and ending the connection. The line numbers each state change passes are the
// original's

// Starts the network, calling callback when it is ready
void NetState_Start(HeapID heapId, void (*callback)(void *work));
// Connects as a child to the parent with a MAC address, scanning for it first when scanFirst is set
void NetState_ConnectMac(const u8 *mac, BOOL scanFirst);
void NetState_StartScan(void);
// Runs the state, each frame. Returns TRUE when the network is not running
BOOL NetState_Update(void);
void NetState_StartParent(HeapID unused);
void NetState_StartChangeover(int a0, void (*callback)(void *work), int a2);
void NetState_SetChangeoverMode(int mode, int a1, const u8 *mac);
// Command handlers
void NetState_RecvExit(int netId, int size, const void *data, void *work, NetHandle *handle);
void NetState_RecvExitReply(int netId, int size, const void *data, void *work, NetHandle *handle);
void NetState_RecvMPMode(int netId, int size, const void *data, void *work, NetHandle *handle);
void NetState_RecvWifiDisconnect(int netId, int size, const void *data, void *work, NetHandle *handle);
// Ends the network, calling callback when it has
void NetState_End(void (*callback)(void *work));
void NetState_CancelEnd(void);
void NetState_StartSearch(void);
BOOL NetState_ConnectWifiFriend(int a0);
BOOL NetState_WaitWifiMatch(void);
BOOL NetState_WaitGtsMatch(void);
void NetState_ConnectWifiAny(void);
// Where the Wi-Fi connection is: 0 connecting, 1 connected, 3 to 5 failed, 2 otherwise
int NetState_GetWifiStatus(void);
void NetState_WifiDisconnect(BOOL a0);
u8 NetState_IsDisconnectReceived(void);
BOOL NetState_IsWifiConnecting(void);
BOOL NetState_IsWifiReady(void);
void NetState_EndWifi(void (*callback)(void *work));
void NetState_StartWifiLogin(void);
void NetState_SetAutoExit(u8 a0);
void NetState_SetDisconnectError(int a0);
BOOL NetState_SetError(u8 a0);
void NetState_ClearError(void);
u8 NetState_GetError(void);
// Records a network error: its code, and type nonzero to set the network error
void NetState_RecordError(int code, u32 errorType, u32 error, int type);
void NetState_ClearWifiError(void);
GFLNetErrorInfo *NetState_GetErrorInfo(void);
BOOL NetState_IsWifiBattleError(void);

#endif // POKEBW2_GFL_NET_STATE_H
