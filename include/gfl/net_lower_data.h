#ifndef POKEBW2_GFL_NET_LOWER_DATA_H
#define POKEBW2_GFL_NET_LOWER_DATA_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Sending data larger than a packet in pieces, a little each frame, to buffers the receiving machines set up

void NetLdata_Init(HeapID heapId, BOOL a1);
void NetLdata_Exit(void);
// Starts sending, to the machines in dest, from the server's handle or this machine's
void NetLdata_StartSend(void *data, u32 size, int dest, BOOL fromServer);
// Sets up and frees the buffer for what a machine sends
void NetLdata_SetRecvBuffer(u32 bufferSize, int netId, HeapID heapId, void *buffer);
void NetLdata_FreeRecvBuffer(int netId);
void *NetLdata_GetRecvBuffer(int netId);
u32 NetLdata_GetRecvSize(int netId);
// Whether all of the data has been sent, and whether all of a machine's has arrived
BOOL NetLdata_IsSendDone(void);
BOOL NetLdata_IsRecvDone(int netId);
// Sends the next piece each frame
void NetLdata_Update(void);
// The command handlers for the data and for its header
void *NetLdata_GetRecvPtr(int netId, void *work, int size);
void NetLdata_RecvData(int netId, int size, const void *data, void *work, NetHandle *handle);
void NetLdata_RecvHeader(int netId, int size, const void *data, void *work, NetHandle *handle);

#endif // POKEBW2_GFL_NET_LOWER_DATA_H
