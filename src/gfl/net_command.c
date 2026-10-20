#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_irc_wireless.h"
#include "gfl/net_lower_data.h"
#include "gfl/net_state.h"
#include "gfl/net_system.h"
#include "gfl/std.h"

#define COMMAND_TABLE_MAX 4
#define HUGE_SEND_MAX 5
// The packet header and the command's header, which a chunk leaves room for
#define _HUGEDATA_DIFFBYTE 12

enum {
    NET_CMD_HUGE_DATA = 12,
    NET_CMD_HUGE_HEADER,
};

typedef struct {
    const NetCommand *list;
    int listNum;
    void *work;
    int base;
} NetCommandTable;

// Data in chunks being sent
typedef struct {
    int netId;
    u32 size;
    u8 *data;
    u16 command;
    u16 chunkSize;
    u16 lastSize;
    u16 chunksSent;
    u16 chunkCount;
    u8 netIds;
    // Whether chunks are being sent, and whether the header is to be sent
    u8 sending : 4;
    u8 headerPending : 4;
} NetHugeSend;

// What precedes data in chunks
typedef struct {
    u32 size;
    u16 chunkCount;
    u16 command;
    u16 chunkSize;
    u16 netIds;
} NetHugeHeader;

// Data in chunks arriving from a machine
typedef struct {
    u32 size;
    u8 *data;
    u16 command;
    u16 chunkSize;
    u16 chunksReceived;
    u16 chunkCount;
    u16 netIds;
    u8 used;
} NetHugeRecv;

typedef struct {
    NetCommandTable tables[COMMAND_TABLE_MAX];
    NetHugeSend hugeSends[HUGE_SEND_MAX];
    NetHugeRecv aNetHugeRecv[GFL_NET_HANDLE_MAX];
    u8 unk16C[0xb4];
    u8 unk220[8];
    u8 unk228;
} NetCommandWork;

static NetCommandTable *NetCommand_FindTable(int command);
static void *NetCommand_GetCallback(int command);
static void *NetCommand_GetWork(int command);
static void *NetCommand_GetBufferFunc(int command);
static int NetCommand_SendHugeHeader(void);

// clang-format off
static const NetCommand sBaseCommands[GFL_NET_CMD_BASE_COUNT] = {
    { (void *)func_02040e0c, NULL },
    { NetState_RecvExit, NULL },
    { NetState_RecvExitReply, NULL },
    { (void *)NetHandle_RecvNegotiation, NULL },
    { (void *)NetHandle_RecvNegotiationReply, NULL },
    { (void *)NetHandle_RecvForget, NULL },
    { NetState_RecvMPMode, NULL },
    { NetHandle_RecvTimingFromMachine, NULL },
    { NetHandle_RecvTimingFromServer, NULL },
    { NetIw_RecvCollectedMacs, NULL },
    { NetHandle_RecvData, NULL },
    { func_020421f8, NULL },
    { NetCommand_RecvHugeChunk, NULL },
    { NetCommand_RecvHugeHeader, NULL },
    { NetLdata_RecvData, NetLdata_GetRecvPtr },
    { NetLdata_RecvHeader, NULL },
    { (void *)func_02040e0c, NULL },
};

// clang-format on

static NetCommandTable _BaseTable = { sBaseCommands, GFL_NET_CMD_BASE_COUNT };

static NetCommandWork *_pCommandWork;

void NetCommand_Init(int base, const void *list, int listNum, void *work, HeapID unused) {
    int i;

    if (_pCommandWork == NULL) {
        _pCommandWork = GFL_HeapAllocate(HEAPID_SYSTEM, sizeof(NetCommandWork), TRUE, "net_command.c", 150);
    }
    NetCommand_AddTable(base, list, listNum, work);
    _BaseTable.work = work;
    for (i = 0; i < 8; i++) {
        _pCommandWork->unk220[i] = 0;
    }
    _pCommandWork->unk228 = 0;
}

void NetCommand_Exit(void) {
    if (_pCommandWork != NULL) {
        MI_CpuFill8(_pCommandWork, 0, sizeof(NetCommandWork));
        GFL_HeapFree(_pCommandWork);
        _pCommandWork = NULL;
    }
}

void NetCommand_AddTable(int base, const void *list, int listNum, void *work) {
    NetCommandWork *commandWork = _pCommandWork;

    if (commandWork != NULL) {
        int i;

        for (i = 0; i < COMMAND_TABLE_MAX; i++) {
            if (commandWork->tables[i].base == 0 || base == commandWork->tables[i].base) {
                commandWork->tables[i].base = base;
                _pCommandWork->tables[i].list = list;
                _pCommandWork->tables[i].listNum = listNum;
                _pCommandWork->tables[i].work = work;
                return;
            }
        }
    }
}

void NetCommand_RemoveTable(int base) {
    NetCommandWork *commandWork = _pCommandWork;

    if (commandWork != NULL) {
        int i;

        for (i = 0; i < COMMAND_TABLE_MAX; i++) {
            if (base == commandWork->tables[i].base) {
                MI_CpuFill8(&commandWork->tables[i], 0, sizeof(NetCommandTable));
                return;
            }
        }
    }
}

BOOL NetCommand_IsForThisMachine(int netIds) {
    if (netIds == GFL_NET_NETID_SERVER) {
        return TRUE;
    }
    if (netIds & (1 << NetSystem_GetNetId())) {
        return TRUE;
    }
    if (NetSystem_GetNetId() == 0 && (netIds & 0x80)) {
        return TRUE;
    }
    return FALSE;
}

static NetCommandTable *NetCommand_FindTable(int command) {
    int i;
    NetCommandWork *commandWork;
    int base = command & 0xff00;
    int co = (u8)command;

    if (base == 0) {
        GFL_ASSERT(_BaseTable.listNum > co);
        return &_BaseTable;
    }
    commandWork = _pCommandWork;
    for (i = 0; i < COMMAND_TABLE_MAX; i++) {
        NetCommandTable *table = &commandWork->tables[i];

        if (base == table->base) {
            if (table->listNum > co) {
                return table;
            }
            return NULL;
        }
    }
    return NULL;
}

static void *NetCommand_GetCallback(int command) {
    NetCommandTable *table = NetCommand_FindTable(command);

    if (table != NULL) {
        return table->list[(u8)command].callback;
    }
    return NULL;
}

static void *NetCommand_GetWork(int command) {
    NetCommandTable *table = NetCommand_FindTable(command);

    if (table != NULL) {
        return table->work;
    }
    return NULL;
}

static void *NetCommand_GetBufferFunc(int command) {
    NetCommandTable *table = NetCommand_FindTable(command);

    if (table != NULL) {
        return table->list[(u8)command].getBuffer;
    }
    return NULL;
}

void NetCommand_Dispatch(int netId, int netIds, int command, int size, void *data, NetHandle *handle) {
    if (NetCommand_IsForThisMachine(netIds)) {
        BOOL ok = TRUE;
        void (*callback)(int netId, int size, const void *data, void *work, NetHandle *handle);

        if (!NetHandle_IsNegotiated(handle) && command >= GFL_NET_CMD_BASE_COUNT) {
            ok = FALSE;
        }
        if (ok) {
            callback = NetCommand_GetCallback(command);
            if (callback != NULL) {
                callback(netId, size, data, NetCommand_GetWork(command), handle);
            }
        }
    }
}

BOOL NetCommand_HasCallback(int command) {
    if (NetCommand_GetCallback(command) != NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL NetCommand_HasBufferFunc(int command) {
    if (NetCommand_GetBufferFunc(command) != NULL) {
        return TRUE;
    }
    return FALSE;
}

void *NetCommand_GetRecvBuffer(int command, int netId, int size) {
    void *(*getBuffer)(int netId, void *work, int size) = NetCommand_GetBufferFunc(command);

    if (getBuffer != NULL) {
        return getBuffer(netId, NetCommand_GetWork(command), size);
    }
    return NULL;
}

void func_02040e0c(void) {
}

void NetCommand_RecvHugeChunk(int netId, int size, const void *data, void *work, NetHandle *handle) {
    int index = netId;

    if (netId == GFL_NET_NETID_SERVER) {
        index = GFL_NET_HANDLE_MAX - 1;
    }
    if (handle == NetHandle_GetOwn() && _pCommandWork->aNetHugeRecv[index].used) {
        if (_pCommandWork->aNetHugeRecv[index].data != NULL) {
            MI_CpuCopy8(data,
                        _pCommandWork->aNetHugeRecv[index].data + _pCommandWork->aNetHugeRecv[index].chunksReceived *
                                                                      _pCommandWork->aNetHugeRecv[index].chunkSize,
                        size);
        }
        _pCommandWork->aNetHugeRecv[index].chunksReceived++;
        if (_pCommandWork->aNetHugeRecv[index].chunksReceived == _pCommandWork->aNetHugeRecv[index].chunkCount) {
            if (_pCommandWork->aNetHugeRecv[index].data != NULL) {
                NetCommand_Dispatch(netId, _pCommandWork->aNetHugeRecv[index].netIds,
                                    _pCommandWork->aNetHugeRecv[index].command, _pCommandWork->aNetHugeRecv[index].size,
                                    _pCommandWork->aNetHugeRecv[index].data, handle);
            }
            _pCommandWork->aNetHugeRecv[index].used = FALSE;
        }
    }
}

void NetCommand_RecvHugeHeader(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const NetHugeHeader *header = data;
    int index = netId;

    if (netId == GFL_NET_NETID_SERVER) {
        index = GFL_NET_HANDLE_MAX - 1;
    }
    if (handle == NetHandle_GetOwn() && NetCommand_GetRecvBuffer(header->command, netId, header->size)) {
        // clang-format off
        GFL_ASSERT(_pCommandWork->aNetHugeRecv[index].used==FALSE);
        // clang-format on
        _pCommandWork->aNetHugeRecv[index].used = TRUE;
        _pCommandWork->aNetHugeRecv[index].size = header->size;
        _pCommandWork->aNetHugeRecv[index].data = NetCommand_GetRecvBuffer(header->command, netId, header->size);
        GFL_ASSERT(_pCommandWork->aNetHugeRecv[index].data);
        _pCommandWork->aNetHugeRecv[index].command = header->command;
        _pCommandWork->aNetHugeRecv[index].chunkSize = header->chunkSize;
        _pCommandWork->aNetHugeRecv[index].netIds = header->netIds;
        _pCommandWork->aNetHugeRecv[index].chunkCount = header->chunkCount;
        _pCommandWork->aNetHugeRecv[index].chunksReceived = 0;
    }
}

int NetCommand_SendHuge(NetHandle *handle, u8 netIds, u16 command, u32 size, void *data) {
    GFLNetInitData *pNetIni = GFL_NetGetInitData();
    NetHugeSend *send;
    int i;

    for (i = 0; i < HUGE_SEND_MAX; i++) {
        if (_pCommandWork->hugeSends[i].headerPending == 0 && _pCommandWork->hugeSends[i].sending == 0) {
            break;
        }
    }
    if (i == HUGE_SEND_MAX) {
        return 0;
    }
    send = &_pCommandWork->hugeSends[i];
    send->size = size;
    // clang-format off
    GFL_ASSERT((pNetIni->maxSendSize - _HUGEDATA_DIFFBYTE)>0);
    // clang-format on
    send->chunkSize = pNetIni->maxSendSize - _HUGEDATA_DIFFBYTE;
    send->chunkCount = size / send->chunkSize;
    send->lastSize = size % send->chunkSize;
    if (send->lastSize != 0) {
        send->chunkCount++;
    }
    send->chunksSent = 0;
    send->command = command;
    send->netIds = netIds;
    send->data = data;
    send->netId = NetHandle_GetNetId(handle);
    send->headerPending = TRUE;
    return send->headerPending;
}

static int NetCommand_SendHugeHeader(void) {
    int i;

    for (i = 0; i < HUGE_SEND_MAX; i++) {
        NetHugeSend *send = &_pCommandWork->hugeSends[i];

        if (send->headerPending) {
            NetHugeHeader header;

            header.chunkCount = send->chunkCount;
            header.size = send->size;
            header.command = send->command;
            header.chunkSize = send->chunkSize;
            header.netIds = send->netIds;
            send->sending = NetSystem_QueueCommand(NET_CMD_HUGE_HEADER, (u8 *)&header, sizeof(header), 1, send->netId,
                                                   send->netIds, FALSE);
            send->headerPending = 1 - send->sending;
            return send->sending;
        }
    }
    return 0;
}

void NetCommand_UpdateHugeSend(void) {
    int i;

    for (i = 0; i < HUGE_SEND_MAX; i++) {
        if (_pCommandWork->hugeSends[i].sending) {
            NetHugeSend *send = &_pCommandWork->hugeSends[i];
            u16 len = send->chunkSize;
            u32 offset;
            u8 *sendData = send->data;

            offset = len * send->chunksSent;

            // BUG: Data that is a whole number of chunks has no remainder, and its last chunk is sent empty
#ifdef BUGFIX
            if (send->chunksSent + 1 == send->chunkCount && send->lastSize != 0) {
#else
            if (send->chunksSent + 1 == send->chunkCount) {
#endif
                len = send->lastSize;
            }
            if (NetSystem_QueueCommand(NET_CMD_HUGE_DATA, sendData + offset, len, 1, send->netId, send->netIds, TRUE)) {
                send->chunksSent++;
            }
            if (send->chunksSent == send->chunkCount) {
                send->sending = FALSE;
            }
            return;
        }
    }
    NetCommand_SendHugeHeader();
}
