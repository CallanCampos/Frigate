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

static BOOL func_02043c1c(int size);
static BOOL func_02043d10(int type);

void func_02043868(HeapID heapId, BOOL a1) {
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

void func_020438dc(void) {
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

void func_0204393c(void *data, u32 size, int dest, BOOL fromServer) {
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

void func_020439a4(u32 bufferSize, int netId, HeapID heapId, void *buffer) {
    GFL_ASSERT(netLdataWork != NULL);
    GFL_ASSERT(netLdataWork->postWork[netId] == NULL);
    netLdataWork->postWork[netId] = GFL_HeapAllocate(heapId, sizeof(NetLdataPost), TRUE, "net_lower_data.c", 210);
    netLdataWork->postWork[netId]->buffer = buffer;
    netLdataWork->postWork[netId]->bufferSize = bufferSize;
    netLdataWork->postWork[netId]->allocated = FALSE;
}

void func_02043a1c(int netId) {
    GFL_ASSERT(netLdataWork != NULL);
    GFL_ASSERT(netLdataWork->postWork[netId] != NULL);
    if (netLdataWork->postWork[netId]->allocated == TRUE) {
        GFL_HeapFree(netLdataWork->postWork[netId]->buffer);
    }
    GFL_HeapFree(netLdataWork->postWork[netId]);
    netLdataWork->postWork[netId] = NULL;
}

void *func_02043a80(int netId) {
    GFL_ASSERT(netLdataWork != NULL);
    GFL_ASSERT(netLdataWork->postWork[netId] != NULL);
    return netLdataWork->postWork[netId]->buffer;
}

u32 func_02043ac8(int netId) {
    GFL_ASSERT(netLdataWork != NULL);
    GFL_ASSERT(netLdataWork->postWork[netId] != NULL);
    return netLdataWork->postWork[netId]->recvSize;
}

BOOL func_02043b10(void) {
    if (netLdataWork == NULL) {
        return FALSE;
    }
    return netLdataWork->sendDone;
}

BOOL func_02043b24(int netId) {
    if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
        return FALSE;
    }
    return netLdataWork->postWork[netId]->done;
}

void func_02043b44(void) {
    if (netLdataWork == NULL || netLdataWork->sendData == NULL || func_02042788() != TRUE) {
        return;
    }
    if (netLdataWork->sendStart == TRUE) {
        if (func_02043d10(NET_LDATA_HEADER_START) == TRUE) {
            netLdataWork->sendStart = FALSE;
        }
    } else if (netLdataWork->sentSize < netLdataWork->sendSize) {
        if (netLdataWork->unk3C == TRUE) {
            GFLNetInitData *ini = func_02042e84();
            int size;

            // What fits in the packet after what is already queued
            if (netLdataWork->fromServer == TRUE) {
                size = ini->unk6C - func_0203fbcc() - 16;
            } else {
                size = ini->maxSendSize - func_0203fbb8() - 16;
            }
            if (size > 16) {
                if (netLdataWork->sentSize + size > netLdataWork->sendSize) {
                    size = netLdataWork->sendSize - netLdataWork->sentSize;
                }
                if (func_02043c1c(size) == TRUE) {
                    netLdataWork->sentSize += size;
                    if (netLdataWork->unk38 == TRUE) {
                        netLdataWork->unk3C = FALSE;
                    }
                }
            }
        }
    } else if (netLdataWork->sendDone == FALSE) {
        if (func_02043d10(NET_LDATA_HEADER_END) == TRUE) {
            netLdataWork->sendDone = TRUE;
            netLdataWork->sendData = NULL;
        }
    }
    if (netLdataWork->unk40 == TRUE) {
        if (func_02043d10(NET_LDATA_HEADER_2) == TRUE) {
            netLdataWork->unk40 = FALSE;
        }
    }
}

static BOOL func_02043c1c(int size) {
    void *data = (u8 *)netLdataWork->sendData + netLdataWork->sentSize;
    NetHandle *handle;

    if (netLdataWork->fromServer == TRUE) {
        handle = func_02040414(GFL_NET_NETID_SERVER);
    } else {
        handle = func_02040440();
    }
    return func_02042c9c(handle, netLdataWork->dest, 14, size, data, 0, TRUE, TRUE);
}

void *func_02043c64(int netId, void *work, int size) {
    GFLNetInitData *ini;

    if (netId == GFL_NET_NETID_SERVER) {
        netId = 0;
    }
    if (netLdataWork != NULL && netLdataWork->postWork[netId] != NULL) {
        ini = func_02042e84();
        if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
            return NULL;
        }
        return (u8 *)netLdataWork->postWork[netId]->buffer + netLdataWork->postWork[netId]->recvSize;
    }
    return NULL;
}

void func_02043ca0(int netId, int size, const void *data, void *work, NetHandle *handle) {
    GFLNetInitData *ini;

    if (netId == GFL_NET_NETID_SERVER) {
        netId = 0;
    }
    if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
        return;
    }
    ini = func_02042e84();
    if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
        return;
    }
    if (handle != func_02040440() && !ini->bMPMode) {
        return;
    }
    netLdataWork->postWork[netId]->recvSize += size;
    if (netLdataWork->unk38 == TRUE && netId != func_02042a6c(func_02040440())) {
        netLdataWork->unk40 = TRUE;
    }
}

static BOOL func_02043d10(int type) {
    NetHandle *handle;
    NetLdataHeader header;

    if (netLdataWork->fromServer == TRUE) {
        handle = func_02040414(GFL_NET_NETID_SERVER);
    } else {
        handle = func_02040440();
    }
    header.type = type;
    header.size = netLdataWork->sendSize;
    return func_02042c9c(handle, netLdataWork->dest, 15, sizeof(NetLdataHeader), &header, 0, TRUE, FALSE);
}

void func_02043d6c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    NetLdataHeader *header = data;
    GFLNetInitData *ini;

    if (netId == GFL_NET_NETID_SERVER) {
        netId = 0;
    }
    if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
        return;
    }
    ini = func_02042e84();
    if (netLdataWork == NULL || netLdataWork->postWork[netId] == NULL) {
        return;
    }
    if (handle != func_02040440() && !ini->bMPMode) {
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
        if (netId != func_02042a6c(func_02040440())) {
            netLdataWork->unk3C = TRUE;
        }
        break;
    }
}
