#ifndef POKEBW2_GFL_NET_SYSTEM_H
#define POKEBW2_GFL_NET_SYSTEM_H

#include "types.h"
#include "gfl/heap.h"

// The packets between the machines: queuing commands to send, writing them into packets with a CRC and a sequence
// number, and reading the commands in the packets that arrive

BOOL NetSystem_StartWireless(int size, HeapID heapId);
int NetSystem_CanConnectFriend(int a0);
BOOL NetSystem_StartInfrared(int a0);
int NetSystem_StartParent(int a0, int size, int a2, BOOL (*callback)(void));
BOOL NetSystem_StartScan(int a0, int size);
BOOL NetSystem_StartSearch(int size);
void NetSystem_HoldScan(int a0);
int NetSystem_StartChild(BOOL a0, const u8 *mac, int size, void (*callback)(void));
void NetSystem_Free(void);
// Forgets what has arrived from a machine, as when it disconnects
void NetSystem_ForgetMachine(u32 netId);
// Sends and receives, each frame
BOOL NetSystem_Update(void);
void NetSystem_ResetBuffersIfAllocated(void);
BOOL NetSystem_QueueCommand(int command, u8 *data, int size, u32 unused, int dest, int netId, BOOL noCopy);
int NetSystem_GetSendRingUsed(void);
int NetSystem_GetMPRingUsed(void);
// Whether a machine is connected
BOOL NetSystem_IsMachineConnected(u16 netId);
// The number of machines connected
int NetSystem_CountConnected(void);
BOOL NetSystem_IsDeviceReady(void);
// This machine's network ID
u16 NetSystem_GetNetId(void);
// The connected machines but the parent, as bits
u32 NetSystem_GetChildBits(void);
BOOL NetSystem_HasError(void);
BOOL NetSystem_HasErrorChecked(void);
// Whether a command is waiting to be sent
BOOL NetSystem_HasQueuedCommand(u16 command, int dest);
BOOL NetSystem_IsSendQueueEmpty(void);
void NetSystem_SetWifiConnected(u8 a0);
void NetSystem_SetMPSendEnabled(BOOL a0);
// Sets and clears the network error
void NetSystem_SetError(void);
void NetSystem_ClearError(void);
void NetSystem_Stop(void);
BOOL NetSystem_IsDataShareOk(void);
// A network ID, or GFL_NET_NETID_SERVER for one past the last machine
int NetSystem_ClampNetId(int netId);

#endif // POKEBW2_GFL_NET_SYSTEM_H
