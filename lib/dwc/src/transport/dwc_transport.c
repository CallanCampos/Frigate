// DWC's transport between the machines of a match (overlay 11, Thumb): the pool of the data that GT2 could not take
// yet, the sending of the game's data in pieces, the receiving of it into the game's buffers, and the receive
// timeouts. The ROM embeds no name for the file, which takes the name TWL-DWC gives it (recalled from the SDK). See
// dwc/transport.h
#include "dwc/error.h"
#include "dwc/ext.h"
#include "dwc/memfunc.h"
#include "dwc/transport.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// GT2's send, which the pool sends again through
typedef int (*DWCTransportSendFunc)(GTI2Connection *connection, const void *message, int length, BOOL reliable);

// The header that comes before the game's data and the match's own: its size and kind, and the marks "DT"
typedef struct DWCTransportHeader {
    u32 size;
    u16 type;
    char magic[2];
} DWCTransportHeader;

// The file's own functions, which aren't static: as statics MWCC inlines the small ones into their callers
void DWCs_InitSendPool(void);
void DWCs_ClearSendPool(void);
BOOL DWCs_HasSendPoolSpace(void);
BOOL DWCs_FlushSendPool(void);
BOOL DWCs_FlushSendPoolWith(DWCTransportSendFunc send);
BOOL DWCs_AddToSendPool(GTI2Connection *connection, const void *data, int size);
BOOL DWCi_IsSendableReliable(u8 aid, int type);
void DWCs_EncodeHeader(DWCTransportHeader *header, int type, int size);
u16 DWCs_DecodeHeader(const void *data);
int DWCs_GetRequiredHeaderSize(int type);
BOOL DWC_SetConnectionKeepAliveTime(u32 timeout);
u32 DWC_GetConnectionKeepAliveTime(void);
DWCTransportConnection *DWCs_GetTransConnection(u8 aid);
u8 DWCs_GetSendState(u8 aid);
u8 DWCs_GetRecvState(u8 aid);
void DWCs_Send(u8 aid, const void *data, int size, BOOL reliable);
void DWCs_HandleReliableMessage(GTI2Connection *connection, const void *data, int size);
void DWCs_HandleUnreliableMessage(GTI2Connection *connection, const void *data, int size);
void DWCs_RecvDataHeader(u8 aid, const void *data, int size);
void DWCs_RecvDataBody(u8 aid, const void *data, int size);
void DWCs_RecvSystemDataBody(u8 aid, const void *data, int size);
int DWCs_GetOutgoingBufferFreeSize(u8 aid);
void DWCs_ResetLastRecvTime(GTI2Connection *connection);

// A copy of data that GT2 could not take yet, to be sent again to its connection
typedef struct TransportSendNode {
    GTI2Connection *connection;
    void *data;
    int size;
    struct TransportSendNode *next;
} TransportSendNode;

// The header of data that the server relays to other machines: the marks "F\xfcW", the aid that sent it and the aids
// it is for
typedef struct TransportRelayHeader {
    char magic[3];
    u8 aid;
    u32 aids;
} TransportRelayHeader;

// The data that GT2 could not take, and its total size
static struct {
    TransportSendNode *head;
    int size;
} sSendPool;
static DWCTransportInfo *sTransport;

void DWCs_InitSendPool(void) {
    sSendPool.head = NULL;
    sSendPool.size = 0;
}

void DWCs_ClearSendPool(void) {
    TransportSendNode *node = sSendPool.head;

    while (node != NULL) {
        sSendPool.head = node->next;
        DWC_Free(4, node->data, node->size);
        DWC_Free(4, node, 0);
        node = sSendPool.head;
    }
    sSendPool.head = NULL;
    sSendPool.size = 0;
}

BOOL DWCs_HasSendPoolSpace(void) {
    if (sSendPool.size < 0x6000) {
        return TRUE;
    }
    return FALSE;
}

BOOL DWCs_FlushSendPool(void) {
    return DWCs_FlushSendPoolWith(func_ov011_0217d290);
}

BOOL DWCs_FlushSendPoolWith(DWCTransportSendFunc send) {
    TransportSendNode *node = sSendPool.head;
    int tries;

    while (node != NULL) {
        for (tries = 0; tries < 3; tries++) {
            if (!send(node->connection, node->data, node->size, TRUE)) {
                sSendPool.size -= node->size;
                DWC_Free(4, node->data, node->size);
                node->data = NULL;
                node->size = 0;
                node->connection = NULL;
                break;
            }
        }
        if (tries == 3) {
            break;
        }
        node = node->next;
    }
    node = sSendPool.head;
    if (node != NULL) {
        do {
            if (node->data != NULL) {
                break;
            }
            sSendPool.head = node->next;
            DWC_Free(4, node, 0);
            node = sSendPool.head;
        } while (node != NULL);
    }
    return node == NULL;
}

BOOL DWCs_AddToSendPool(GTI2Connection *connection, const void *data, int size) {
    TransportSendNode **link = &sSendPool.head;
    TransportSendNode *node;

    if (sSendPool.size + size > 0x8000) {
        return FALSE;
    }
    for (node = sSendPool.head; node != NULL; node = node->next) {
        link = &node->next;
    }
    *link = DWC_Alloc(4, sizeof(TransportSendNode));
    if (*link == NULL) {
        return FALSE;
    }
    (*link)->data = DWC_Alloc(4, size);
    if ((*link)->data == NULL) {
        DWC_Free(4, *link, sizeof(TransportSendNode));
        *link = NULL;
        return FALSE;
    }
    (*link)->connection = connection;
    (*link)->size = size;
    (*link)->next = NULL;
    MIi_CpuCopy32(data, (*link)->data, size);
    sSendPool.size += size;
    return TRUE;
}

BOOL DWC_IsSendableReliable(u8 netId) {
    return DWCi_IsSendableReliable(netId, 1);
}

BOOL DWCi_IsSendableReliable(u8 aid, int type) {
    if (DWCi_IsError() || (type == 1 && !DWC_IsValidAID(aid))) {
        return FALSE;
    }
    if (!DWCi_IsValidAID(aid)) {
        return FALSE;
    }
    if (DWCs_GetSendState(aid) == 1) {
        return FALSE;
    }
    if (DWCs_GetOutgoingBufferFreeSize(aid) < DWCs_GetRequiredHeaderSize(type)) {
        return FALSE;
    }
    if (DWCs_HasSendPoolSpace()) {
        return TRUE;
    }
    return FALSE;
}

BOOL DWC_SendReliable(u8 aid, const void *data, int size) {
    return DWCi_SendReliable(1, aid, data, size);
}

BOOL DWCi_SendReliable(int type, u8 aid, const void *data, int size) {
    DWCTransportConnection *connection = DWCs_GetTransConnection(aid);
    int n;
    DWCTransportHeader header;

    if (!DWCi_IsSendableReliable(aid, type)) {
        return FALSE;
    }
    connection->sendState = 1;
    connection->sendBuffer = data;
    connection->sendingSize = 0;
    connection->requestSendSize = size;
    DWCs_EncodeHeader(&header, type, size);
    DWCs_Send(aid, &header, sizeof(DWCTransportHeader), TRUE);
    n = sTransport->sendSplitMax;
    if (size <= n) {
        n = size;
    }
    if (n > DWCs_GetOutgoingBufferFreeSize(aid)) {
        return TRUE;
    }
    DWCs_Send(aid, data, n, TRUE);
    connection->sendingSize += n;
    n = connection->requestSendSize;
    if (connection->sendingSize == n) {
        connection->sendState = 0;
        connection->sendBuffer = NULL;
        connection->sendingSize = 0;
        connection->requestSendSize = 0;
        if (sTransport->sendCallback != NULL && type == 1) {
            sTransport->sendCallback(n, aid, sTransport->sendParam);
        }
    }
    return TRUE;
}

u32 DWC_SendReliableBitmap(u32 netIds, const void *data, int size) {
    u8 i;

    for (i = 0; i < 32; i++) {
        u32 bit = (i != 0) ? 1 << i : 1;

        if ((netIds & bit) && i != DWC_GetMyAID()) {
            if (!DWC_SendReliable(i, data, size)) {
                netIds &= ~bit;
            }
        }
    }
    return netIds;
}

BOOL DWC_SendUnreliable(u8 aid, const void *data, int size) {
    if (DWCi_IsError()) {
        return FALSE;
    }
    if (!DWC_IsValidAID(aid)) {
        return FALSE;
    }
    if (size > 0x5b6) {
        return FALSE;
    }
    DWCi_SendUnreliable(DWC_GetMyAID(), aid, data, size, TRUE);
    if (sTransport->sendCallback != NULL) {
        sTransport->sendCallback(size, aid, sTransport->sendParam);
    }
    return TRUE;
}

BOOL DWC_SetRecvBuffer(u8 netId, void *buffer, int size) {
    DWCTransportConnection *connection = DWCs_GetTransConnection(netId);

    if (DWCs_GetRecvState(netId) == 2) {
        return FALSE;
    }
    connection->recvBuffer = buffer;
    connection->recvBufferSize = size;
    connection->recvState = 1;
    connection->recvingSize = 0;
    connection->requestRecvSize = 0;
    return TRUE;
}

BOOL DWC_SetUserSendCallback(DWCSendFunc func, void *param) {
    if (sTransport == NULL) {
        return FALSE;
    }
    sTransport->sendCallback = func;
    sTransport->sendParam = param;
    return TRUE;
}

BOOL DWC_SetUserRecvCallback(DWCRecvFunc func, void *param) {
    if (sTransport == NULL) {
        return FALSE;
    }
    sTransport->recvCallback = func;
    sTransport->recvParam = param;
    return TRUE;
}

BOOL DWC_SetUserRecvTimeoutCallback(DWCTimeoutFunc func, void *param) {
    if (sTransport == NULL) {
        return FALSE;
    }
    sTransport->recvTimeoutCallback = func;
    sTransport->recvTimeoutParam = param;
    return TRUE;
}

void DWCs_EncodeHeader(DWCTransportHeader *header, int type, int size) {
    strncpy(header->magic, "DT", 2);
    header->type = type;
    header->size = size;
}

u16 DWCs_DecodeHeader(const void *data) {
    DWCTransportHeader header;

    MI_CpuCopy8(data, &header, sizeof(DWCTransportHeader));
    if (memcmp(header.magic, "DT", 2) == 0) {
        return header.type;
    }
    return 0;
}

// The room in GT2's buffer that sending data of a kind needs
int DWCs_GetRequiredHeaderSize(int type) {
    int n;

    switch (type) {
    case 2:
        return 0x24;
    case 3:
        return 0xc;
    case 4:
        n = DWCi_GetNumNodeInfo() - 1;
        if (n == 0) {
            return 0x10;
        }
        return n * 0x18 + 0x10;
    default:
        return 8;
    }
}

BOOL DWC_SetRecvTimeoutTime(u8 netId, int timeout) {
    if (sTransport == NULL) {
        return FALSE;
    }
    sTransport->connections[netId].recvTimeout = timeout;
    sTransport->connections[netId].recvTimeoutTime = OS_GetTick();
    return TRUE;
}

BOOL DWC_SetConnectionKeepAliveTime(u32 timeout) {
    if (sTransport == NULL || timeout == 0) {
        return FALSE;
    }
    sTransport->keepAliveTimeout = timeout;
    return TRUE;
}

u32 DWC_GetConnectionKeepAliveTime(void) {
    if (sTransport != NULL) {
        return sTransport->keepAliveTimeout;
    }
    return 0;
}

void DWCi_InitTransport(DWCTransportInfo *info) {
    sTransport = info;
    MI_CpuFill8(info, 0, sizeof(DWCTransportInfo));
    sTransport->sendSplitMax = 0x5b9;
    DWC_SetConnectionKeepAliveTime(20000);
    DWCs_InitSendPool();
}

void DWCi_RecvCallback(GTI2Connection *connection, const void *message, int length, BOOL reliable) {
    if (sTransport == NULL || message == NULL || length == 0) {
        return;
    }
    DWCs_ResetLastRecvTime(connection);
    if (reliable) {
        DWCs_HandleReliableMessage(connection, message, length);
    } else {
        DWCs_HandleUnreliableMessage(connection, message, length);
    }
}

void DWCi_PingCallback(GTI2Connection *connection, u32 latency) {
    if (sTransport->pingCallback != NULL) {
        u8 aid = DWCi_GetConnectionAID(connection);

        sTransport->pingCallback(latency, aid, sTransport->pingParam);
    }
}

void DWCi_TransportProcess(void) {
    u8 *aids;
    int count;
    int i;
    u32 timeout;
    u64 now;
    u8 aid;
    DWCTransportConnection *connection;

    if (sTransport == NULL) {
        return;
    }
    count = DWC_GetAIDList(&aids);
    now = OS_GetTick();
    timeout = DWC_GetConnectionKeepAliveTime();
    if (DWC_IsServerMyself() || (DWCi_IsSvDownState() && DWCi_IsServerNodeMyself())) {
        for (i = 0; i < count; i++) {
            aid = aids[i];
            if (aid != DWC_GetMyAID()) {
                connection = DWCs_GetTransConnection(aid);
                if ((u32)OS_TicksToMilliSeconds(now - connection->lastRecvTime) > timeout) {
                    DWCi_CloseConnectionHard(aid);
                    DWCi_SendServerPoll(TRUE);
                }
            }
        }
    } else {
        aid = DWCi_GetServerOrSubServerAID();
        if (aid != 0xff && aid != DWC_GetMyAID()) {
            connection = DWCs_GetTransConnection(aid);
            if ((u32)OS_TicksToMilliSeconds(now - connection->lastRecvTime) > timeout) {
                DWCi_CloseConnectionHard(aid);
            }
        }
    }
    for (i = 0; i < count; i++) {
        aid = aids[i];
        DWCs_FlushSendPool();
        if (DWC_IsValidAID(aid)) {
            u64 current = OS_GetTick();
            u32 limit;
            DWCTimeoutFunc callback;

            connection = DWCs_GetTransConnection(aid);
            callback = sTransport->recvTimeoutCallback;
            if (callback != NULL) {
                limit = connection->recvTimeout;
                if (limit != 0 && (u32)OS_TicksToMilliSeconds(current - connection->recvTimeoutTime) > limit) {
                    callback(aid, sTransport->recvTimeoutParam);
                    connection->recvTimeoutTime = current;
                }
            }
        }
        if (aid != DWC_GetMyAID() && DWCs_GetSendState(aid) == 1) {
            int total;
            int n;

            connection = DWCs_GetTransConnection(aid);
            n = connection->requestSendSize - connection->sendingSize;
            total = sTransport->sendSplitMax;
            if (n <= total) {
                total = n;
            }
            if (DWCs_GetOutgoingBufferFreeSize(aid) >= total) {
                DWCs_Send(aid, connection->sendBuffer + connection->sendingSize, total, TRUE);
                connection->sendingSize += total;
                n = connection->requestSendSize;
                if (connection->sendingSize == n) {
                    connection->sendState = 0;
                    connection->sendBuffer = NULL;
                    connection->sendingSize = 0;
                    connection->requestSendSize = 0;
                    if (sTransport->sendCallback != NULL) {
                        sTransport->sendCallback(n, aid, sTransport->sendParam);
                    }
                }
            }
        }
    }
}

void DWCi_ClearTransConnection(u8 aid) {
    if (sTransport != NULL) {
        sTransport->connections[aid].sendingSize = 0;
        sTransport->connections[aid].recvingSize = 0;
        sTransport->connections[aid].requestSendSize = 0;
        sTransport->connections[aid].requestRecvSize = 0;
        sTransport->connections[aid].sendState = 0;
        sTransport->connections[aid].recvBuffer = NULL;
        sTransport->connections[aid].recvBufferSize = 0;
        sTransport->connections[aid].recvState = 0;
        sTransport->connections[aid].lastRecvType = 0;
    }
}

void DWCi_ShutdownTransport(void) {
    sTransport = NULL;
    DWCs_ClearSendPool();
}

void DWCi_ResetLastSomeDataRecvTimeByAid(u8 aid) {
    u64 now = OS_GetTick();

    DWCs_GetTransConnection(aid)->lastRecvTime = now;
}

DWCTransportConnection *DWCs_GetTransConnection(u8 aid) {
    return &sTransport->connections[aid];
}

u8 DWCs_GetSendState(u8 aid) {
    return sTransport->connections[aid].sendState;
}

u8 DWCs_GetRecvState(u8 aid) {
    return sTransport->connections[aid].recvState;
}

// Sends data to a machine; reliable data that GT2 can't take now goes to the pool, and is lost with an error when the
// pool is full
void DWCs_Send(u8 aid, const void *data, int size, BOOL reliable) {
    GTI2Connection *connection = DWCi_GetGT2Connection(aid);

    if (reliable) {
        if (!DWCs_FlushSendPool() || func_ov011_0217d290(connection, data, size, reliable)) {
            if (!DWCs_AddToSendPool(connection, data, size)) {
                DWCi_SetError(DWC_ERROR_NETWORK, -0x17d5e);
            }
        }
    } else {
        func_ov011_0217d290(connection, data, size, reliable);
    }
}

void DWCi_SendUnreliable(u8 from, u8 aid, const void *data, int size, BOOL relay) {
    GTI2Connection *connection;
    BOOL viaServer = FALSE;
    TransportRelayHeader *header;

    connection = DWCi_GetGT2Connection(aid);
    if (connection == NULL) {
        if (DWC_IsServerMyself()) {
            return;
        }
        connection = DWCi_GetGT2Connection(DWCi_GetNodeInfoByIndex(0)->aid);
        if (connection == NULL) {
            return;
        }
        viaServer = TRUE;
    }
    if (relay == TRUE && viaServer == TRUE) {
        header = DWC_Alloc(4, size + sizeof(TransportRelayHeader));
        if (header == NULL) {
            return;
        }
        strncpy(header->magic, "F\xfcW", 3);
        header->aid = from;
        header->aids = 1 << aid;
        MI_CpuCopy8(data, header + 1, size);
        size += sizeof(TransportRelayHeader);
        func_ov011_0217d290(connection, header, size, FALSE);
        size = 0;
        if (header != NULL) {
            DWC_Free(4, header, size);
        }
    } else {
        func_ov011_0217d290(connection, data, size, FALSE);
    }
}

void DWCs_HandleReliableMessage(GTI2Connection *connection, const void *data, int size) {
    u8 aid = DWCi_GetConnectionAID(connection);
    u16 type;

    if (aid == 0xff) {
        DWCi_SetError(DWC_ERROR_NETWORK, -0x17d68);
        return;
    }
    switch (DWCs_GetRecvState(aid)) {
    case 0:
        type = DWCs_DecodeHeader(data);
        if (type >= 2 && type < 5) {
            DWCs_RecvDataHeader(aid, data, size);
        }
        break;
    case 1:
        DWCs_RecvDataHeader(aid, data, size);
        break;
    case 2:
        DWCs_RecvDataBody(aid, data, size);
        break;
    case 3:
        DWCs_RecvSystemDataBody(aid, data, size);
        break;
    case 4:
        sTransport->connections[aid].recvState = 1;
        sTransport->connections[aid].recvingSize = 0;
        sTransport->connections[aid].requestRecvSize = 0;
        break;
    default:
        DWCi_SetError(DWC_ERROR_NETWORK, -0x17d4a);
        break;
    }
}

void DWCs_HandleUnreliableMessage(GTI2Connection *connection, const void *data, int size) {
    const u8 *bytes = data;
    u8 aid = DWCi_GetConnectionAID(connection);
    BOOL relayed = FALSE;
    BOOL forMe = FALSE;
    u32 aids;
    DWCTransportConnection *transport;
    u8 i;

    if (aid == 0xff) {
        DWCi_SetError(DWC_ERROR_NETWORK, -0x17d68);
        return;
    }
    if ((u32)size >= 0x14 && strncmp(data, "\xbbI\xccM", 4) == 0) {
        DWCi_ProcessGT2MatchCommand(bytes, size);
        return;
    }
    if (size >= 8 && memcmp(data, "F\xfcW", 3) == 0) {
        relayed = TRUE;
    }
    if (relayed == TRUE) {
        const TransportRelayHeader *header = data;

        aid = header->aid;
        MI_CpuCopy8(&header->aids, &aids, 4);
        if (!DWC_IsServerMyself()) {
            u8 own = DWC_GetMyAID();

            forMe = TRUE;
            if (!(aids & (1 << own))) {
                return;
            }
        } else {
            for (i = 0; i < 32; i++) {
                if (aids & (1 << i)) {
                    if (i == DWC_GetMyAID()) {
                        forMe = TRUE;
                    } else {
                        DWCi_SendUnreliable(aid, i, data, size, FALSE);
                    }
                }
            }
        }
        if (!forMe) {
            return;
        }
    }
    transport = &sTransport->connections[aid];
    if (relayed == TRUE) {
        bytes += sizeof(TransportRelayHeader);
        size -= sizeof(TransportRelayHeader);
    }
    if (transport->recvBuffer == NULL || transport->recvBufferSize < size) {
        return;
    }
    if (sTransport->recvCallback != NULL) {
        sTransport->recvCallback(aid, (u8 *)bytes, size, sTransport->recvParam);
    }
    if (sTransport->recvTimeoutCallback != NULL && transport->recvTimeout != 0) {
        transport->recvTimeoutTime = OS_GetTick();
    }
}

void DWCs_RecvDataHeader(u8 aid, const void *data, int size) {
    DWCTransportConnection *connection = &sTransport->connections[aid];
    DWCTransportHeader header;
    u16 type;

    connection->lastRecvState = DWCs_GetRecvState(aid);
    type = DWCs_DecodeHeader(data);
    switch (type) {
    case 1:
        if (size != sizeof(DWCTransportHeader)) {
            return;
        }
        MI_CpuCopy8(data, &header, sizeof(DWCTransportHeader));
        connection->requestRecvSize = header.size;
        connection->recvingSize = 0;
        if (connection->recvBuffer != NULL && connection->recvBufferSize >= connection->requestRecvSize) {
            connection->recvState = 2;
        } else {
            connection->recvState = 4;
        }
        break;
    case 2:
    case 3:
    case 4:
        connection->recvState = 3;
        break;
    }
    connection->lastRecvType = type;
}

void DWCs_RecvDataBody(u8 aid, const void *data, int size) {
    DWCTransportConnection *connection = &sTransport->connections[aid];
    int total;

    if (DWCs_GetRecvState(aid) == 2) {
        if (connection->recvingSize + size > connection->recvBufferSize) {
            DWCi_SetError(DWC_ERROR_NETWORK, -0x17d54);
            return;
        }
        MI_CpuCopy8(data, connection->recvBuffer + connection->recvingSize, size);
    }
    connection->recvingSize += size;
    total = connection->requestRecvSize;
    if (connection->recvingSize == total) {
        connection->recvState = 1;
        connection->recvingSize = 0;
        connection->requestRecvSize = 0;
        if (sTransport->recvCallback != NULL) {
            sTransport->recvCallback(aid, connection->recvBuffer, total, sTransport->recvParam);
        }
    }
    if (sTransport->recvTimeoutCallback != NULL && connection->recvTimeout != 0) {
        connection->recvTimeoutTime = OS_GetTick();
    }
}

void DWCs_RecvSystemDataBody(u8 aid, const void *data, int size) {
    DWCTransportConnection *connection = DWCs_GetTransConnection(aid);
    void *copy;

    connection->recvState = connection->lastRecvState;
    switch (connection->lastRecvType) {
    case 2:
    case 3:
    case 4:
        copy = DWC_Alloc(4, size);
        if (copy == NULL) {
            DWCi_SetError(DWC_ERROR_FATAL, -0x17ae9);
            return;
        }
        MI_CpuCopy8(data, copy, size);
        DWCi_ProcessMatchSynPacket(aid, connection->lastRecvType, copy);
        DWC_Free(4, copy, 0);
        break;
    }
}

// The free space of GT2's buffer to a machine, less what the transport keeps for itself
int DWCs_GetOutgoingBufferFreeSize(u8 aid) {
    int n = func_ov011_0217d394(DWCi_GetGT2Connection(aid)) - 0x207;

    if (n <= 0) {
        return 0;
    }
    return n;
}

void DWCs_ResetLastRecvTime(GTI2Connection *connection) {
    u8 aid = DWCi_GetConnectionAID(connection);

    if (aid < 0x20 && aid != DWC_GetMyAID()) {
        u64 now = OS_GetTick();

        DWCs_GetTransConnection(aid)->lastRecvTime = now;
    }
}
