#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/net_lower_data.h"
#include "gfl/net_system.h"
#include "gfl/std.h"

enum {
    NET_LDATA_HEADER_START,
    NET_LDATA_HEADER_END,
    NET_LDATA_HEADER_2,
};

// The header command, which starts and ends the data
typedef struct {
    u32 type : 2;
    u32 size : 30;
} NetLdataHeader;

// Where a machine's data is received
typedef struct {
    void *buffer;
    u32 bufferSize;
    u32 size;
    u32 recvSize;
    u8 done : 1;
    // Whether buffer is freed with this
    u8 allocated : 1;
    u8 unk11[0x37];
} NetLdataPost;

typedef struct {
    NetLdataPost *postWork[GFL_NET_MACHINE_MAX];
    void *sendData;
    u32 sendSize;
    u8 dest;
    BOOL fromServer;
    BOOL sendDone;
    // Whether the start header still has to be sent
    BOOL sendStart;
    BOOL unk38;
    BOOL unk3C;
    BOOL unk40;
    u32 sentSize;
} NetLdataWork;

static NetLdataWork *netLdataWork;

static BOOL NetLdata_SendPiece(int size);
static BOOL NetLdata_SendHeader(int type);

void NetLdata_Init(HeapID heapId, BOOL a1) {
    u8 i;

    GFL_ASSERT(netLdataWork == NULL);
    netLdataWork = GFL_HeapAllocate(heapId, sizeof(NetLdataWork), TRUE, "net_lower_data.c", 103);
    netLdataWork->sendData = NULL;
    netLdataWork->sendSize = 0;
    netLdataWork->sentSize = 0;
    netLdataWork->sendDone = FALSE;
    netLdataWork->sendStart = FALSE;
    netLdataWork->unk38 = a1;
    netLdataWork->unk3C = TRUE;
    netLdataWork->unk40 = FALSE;
    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        netLdataWork->postWork[i] = NULL;
    }
}

void NetLdata_Exit(void) {
    u8 i;

    GFL_ASSERT(netLdataWork != NULL);
    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        if (netLdataWork->postWork[i] != NULL) {
            if (netLdataWork->postWork[i]->allocated == TRUE) {
                GFL_HeapFree(netLdataWork->postWork[i]->buffer);
            }
            GFL_HeapFree(netLdataWork->postWork[i]);
        }
    }
    GFL_HeapFree(netLdataWork);
    netLdataWork = NULL;
}

void NetLdata_StartSend(void *data, u32 size, int dest, BOOL fromServer) {
    GFL_ASSERT(netLdataWork != NULL);
    GFL_ASSERT(netLdataWork->sendData == NULL);
    netLdataWork->sendData = data;
    netLdataWork->sendSize = size;
    netLdataWork->dest = dest;
    netLdataWork->fromServer = fromServer;
    netLdataWork->sentSize = 0;
    netLdataWork->sendStart = TRUE;
    netLdataWork->sendDone = FALSE;
}

void NetLdata_SetRecvBuffer(u32 bufferSize, int netId, HeapID heapId, void *buffer) {
    GFL_ASSERT(netLdataWork != NULL);
    GFL_ASSERT(netLdataWork->postWork[netId] == NULL);
    netLdataWork->postWork[netId] = GFL_HeapAllocate(heapId, sizeof(NetLdataPost), TRUE, "net_lower_data.c", 210);
    netLdataWork->postWork[netId]->buffer = buffer;
    netLdataWork->postWork[netId]->bufferSize = bufferSize;
    netLdataWork->postWork[netId]->allocated = FALSE;
}

void NetLdata_FreeRecvBuffer(int netId) {
    GFL_ASSERT(netLdataWork != NULL);
    GFL_ASSERT(netLdataWork->postWork[netId] != NULL);
    if (netLdataWork->postWork[netId]->allocated == TRUE) {
        GFL_HeapFree(netLdataWork->postWork[netId]->buffer);
    }
    GFL_HeapFree(netLdataWork->postWork[netId]);
    netLdataWork->postWork[netId] = NULL;
}

void *NetLdata_GetRecvBuffer(int netId) {
    GFL_ASSERT(netLdataWork != NULL);
    GFL_ASSERT(netLdataWork->postWork[netId] != NULL);
    return netLdataWork->postWork[netId]->buffer;
}

u32 NetLdata_GetRecvSize(int netId) {
    GFL_ASSERT(netLdataWork != NULL);
    GFL_ASSERT(netLdataWork->postWork[netId] != NULL);
    return netLdataWork->postWork[netId]->recvSize;
}

BOOL NetLdata_IsSendDone(void) {
    if (netLdataWork == NULL) {
        return FALSE;
    }
    return netLdataWork->sendDone;
}

BOOL NetLdata_IsRecvDone(int netId) {
    if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
        return FALSE;
    }
    return netLdataWork->postWork[netId]->done;
}

void NetLdata_Update(void) {
    if (netLdataWork == NULL || netLdataWork->sendData == NULL || GFL_NetIsInitialized() != TRUE) {
        return;
    }
    if (netLdataWork->sendStart == TRUE) {
        if (NetLdata_SendHeader(NET_LDATA_HEADER_START) == TRUE) {
            netLdataWork->sendStart = FALSE;
        }
    } else if (netLdataWork->sentSize < netLdataWork->sendSize) {
        if (netLdataWork->unk3C == TRUE) {
            GFLNetInitData *ini = GFL_NetGetInitData();
            int size;

            // What fits in the packet after what is already queued
            if (netLdataWork->fromServer == TRUE) {
                size = ini->unk6C - NetSystem_GetMPRingUsed() - 16;
            } else {
                size = ini->maxSendSize - NetSystem_GetSendRingUsed() - 16;
            }
            if (size > 16) {
                if (netLdataWork->sentSize + size > netLdataWork->sendSize) {
                    size = netLdataWork->sendSize - netLdataWork->sentSize;
                }
                if (NetLdata_SendPiece(size) == TRUE) {
                    netLdataWork->sentSize += size;
                    if (netLdataWork->unk38 == TRUE) {
                        netLdataWork->unk3C = FALSE;
                    }
                }
            }
        }
    } else if (netLdataWork->sendDone == FALSE) {
        if (NetLdata_SendHeader(NET_LDATA_HEADER_END) == TRUE) {
            netLdataWork->sendDone = TRUE;
            netLdataWork->sendData = NULL;
        }
    }
    if (netLdataWork->unk40 == TRUE) {
        if (NetLdata_SendHeader(NET_LDATA_HEADER_2) == TRUE) {
            netLdataWork->unk40 = FALSE;
        }
    }
}

static BOOL NetLdata_SendPiece(int size) {
    void *data = (u8 *)netLdataWork->sendData + netLdataWork->sentSize;
    NetHandle *handle;

    if (netLdataWork->fromServer == TRUE) {
        handle = NetHandle_Get(GFL_NET_NETID_SERVER);
    } else {
        handle = NetHandle_GetOwn();
    }
    return GFL_NetSendCommandToBits(handle, netLdataWork->dest, 14, size, data, 0, TRUE, TRUE);
}

void *NetLdata_GetRecvPtr(int netId, void *work, int size) {
    GFLNetInitData *ini;

    if (netId == GFL_NET_NETID_SERVER) {
        netId = 0;
    }
    if (netLdataWork != NULL && netLdataWork->postWork[netId] != NULL) {
        ini = GFL_NetGetInitData();
        if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
            return NULL;
        }
        return (u8 *)netLdataWork->postWork[netId]->buffer + netLdataWork->postWork[netId]->recvSize;
    }
    return NULL;
}

void NetLdata_RecvData(int netId, int size, const void *data, void *work, NetHandle *handle) {
    GFLNetInitData *ini;

    if (netId == GFL_NET_NETID_SERVER) {
        netId = 0;
    }
    if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
        return;
    }
    ini = GFL_NetGetInitData();
    if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
        return;
    }
    if (handle != NetHandle_GetOwn() && !ini->bMPMode) {
        return;
    }
    netLdataWork->postWork[netId]->recvSize += size;
    if (netLdataWork->unk38 == TRUE && netId != GFL_NetGetHandleNetId(NetHandle_GetOwn())) {
        netLdataWork->unk40 = TRUE;
    }
}

static BOOL NetLdata_SendHeader(int type) {
    NetHandle *handle;
    NetLdataHeader header;

    if (netLdataWork->fromServer == TRUE) {
        handle = NetHandle_Get(GFL_NET_NETID_SERVER);
    } else {
        handle = NetHandle_GetOwn();
    }
    header.type = type;
    header.size = netLdataWork->sendSize;
    return GFL_NetSendCommandToBits(handle, netLdataWork->dest, 15, sizeof(NetLdataHeader), &header, 0, TRUE, FALSE);
}

void NetLdata_RecvHeader(int netId, int size, const void *data, void *work, NetHandle *handle) {
    NetLdataHeader *header = data;
    GFLNetInitData *ini;

    if (netId == GFL_NET_NETID_SERVER) {
        netId = 0;
    }
    if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
        return;
    }
    ini = GFL_NetGetInitData();
    if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
        return;
    }
    if (handle != NetHandle_GetOwn() && !ini->bMPMode) {
        return;
    }
    switch (header->type) {
    case NET_LDATA_HEADER_START:
        // clang-format off
        GFL_ASSERT_MSG(header->size <= netLdataWork->postWork[netId]->bufferSize, "BufferSize is not enough!![%x][%x]\n",
                       netLdataWork->postWork[netId]->bufferSize, header->size);
        // clang-format on
        netLdataWork->postWork[netId]->size = header->size;
        netLdataWork->postWork[netId]->recvSize = 0;
        netLdataWork->postWork[netId]->done = FALSE;
        break;
    case NET_LDATA_HEADER_END:
        if (netLdataWork->postWork[netId]->recvSize == netLdataWork->postWork[netId]->size) {
            netLdataWork->postWork[netId]->done = TRUE;
        }
        break;
    case NET_LDATA_HEADER_2:
        if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn())) {
            netLdataWork->unk3C = TRUE;
        }
        break;
    }
}
