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
// Connects to the parent with a MAC address, or becomes the parent
void NetState_ConnectMac(const u8 *mac, BOOL parent);
void func_020413f0(void);
// Runs the state, each frame. Returns TRUE when the network is not running
BOOL NetState_Update(void);
void func_020414c0(HeapID unused);
void func_02041a30(int a0, void (*callback)(void *work), int a2);
void func_02041c00(int mode, int a1, const u8 *mac);
// Command handlers
void NetState_RecvExit(int netId, int size, const void *data, void *work, NetHandle *handle);
void NetState_RecvExitReply(int netId, int size, const void *data, void *work, NetHandle *handle);
void NetState_RecvMPMode(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_020421f8(int netId, int size, const void *data, void *work, NetHandle *handle);
// Ends the network, calling callback when it has
void NetState_End(void (*callback)(void *work));
void NetState_CancelEnd(void);
void func_02041dfc(void);
BOOL func_02041fd0(int a0);
BOOL func_0204200c(void);
BOOL func_0204204c(void);
void func_0204208c(void);
// Where the Wi-Fi connection is: 0 connecting, 1 connected, 3 to 5 failed, 2 otherwise
int NetState_GetWifiStatus(void);
void func_020421ac(BOOL a0);
u8 func_02042210(void);
BOOL NetState_IsWifiConnecting(void);
BOOL func_0204223c(void);
void func_0204230c(void (*callback)(void *work));
void func_020423e0(void);
void func_02042410(u8 a0);
void func_02042424(int a0);
BOOL NetState_SetError(u8 a0);
void NetState_ClearError(void);
u8 NetState_GetError(void);
// Records a network error: its code, and type nonzero to set the network error
void NetState_RecordError(int code, u32 errorType, u32 error, int type);
void NetState_ClearWifiError(void);
GFLNetErrorInfo *NetState_GetErrorInfo(void);
BOOL NetState_IsWifiBattleError(void);

#endif // POKEBW2_GFL_NET_STATE_H
