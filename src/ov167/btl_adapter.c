#include "types.h"
#include "battle/btl_adapter.h"
#include "battle/btl_net.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// A command and its data, as the server sends it to a client
typedef struct {
    u16 cmd;
    u16 size;
    u8 data[0xbbc];
} BtlAdapterCmd;

struct BtlAdapter {
    NetHandle *netHandle;
    // The client's reply to the last command, and its size
    void *returnData;
    u32 returnSize;
    // 1 once the reply has arrived
    u8 returned : 4;
    // The commands go over the link
    u8 isNet : 4;
    u8 clientId;
    // Where the last command is in its exchange: 0 idle, 1 to send, 3 sent and waiting for the reply, 4 replied
    u8 state;
    u8 cmd;
    BtlAdapterCmd sendCmd;
};

static BOOL SendCmd(BtlAdapter *adapter);
static BOOL IsReplyReceived(BtlAdapter *adapter);

// The battle's communication mode (BtlFieldSituation.unk18): while it is 0, the server and the client share one adapter
// per client ID; otherwise each caller gets its own
static u32 sCommMode;
static BtlAdapter *sAdapters[4];

void BtlAdapter_InitSystem(u8 commMode) {
    int i;

    for (i = 0; i < 4; i++) {
        sAdapters[i] = NULL;
    }
    sCommMode = commMode;
}

void BtlAdapter_QuitSystem(void) {
}

BtlAdapter *BtlAdapter_Create(NetHandle *netHandle, u8 clientId, BOOL flag, HeapID heapId) {
    BtlAdapter *adapter = NULL;

    if (sAdapters[clientId] == NULL) {
        adapter = GFL_HeapAllocate(heapId, sizeof(BtlAdapter), TRUE, "btl_adapter.c", 0x7e);
        adapter->netHandle = netHandle;
        adapter->clientId = clientId;
        adapter->isNet = flag;
        adapter->state = 0;
        adapter->cmd = 0;
        adapter->returned = 0;
    }
    if (sCommMode == 0) {
        if (sAdapters[clientId] == NULL) {
            sAdapters[clientId] = adapter;
        }
        return sAdapters[clientId];
    }
    sAdapters[clientId] = adapter;
    return adapter;
}

void BtlAdapter_ResetState(BtlAdapter *adapter) {
    adapter->state = 0;
    adapter->cmd = 0;
    adapter->returned = 0;
}

void BtlAdapter_Delete(BtlAdapter *adapter) {
    GFL_HeapFree(adapter);
}

void BtlAdapter_ClearNetReplies(void) {
    if (sCommMode != 0) {
        BtlNet_ResetReplies();
    }
}

void func_ov167_021d4ae8(void) {
}

void BtlAdapter_SetCmd(BtlAdapter *adapter, u32 cmd, const void *data, u32 size) {
    adapter->state = 1;
    adapter->cmd = cmd;
    adapter->sendCmd.cmd = cmd;
    adapter->sendCmd.size = size;
    MI_CpuCopy8(data, adapter->sendCmd.data, size);
    adapter->returnData = NULL;
    adapter->returnSize = 0;
    adapter->returned = 0;
}

BOOL BtlAdapter_WaitCmd(BtlAdapter *adapter) {
    switch (adapter->state) {
    case 1:
        if (!SendCmd(adapter)) {
            break;
        }
        adapter->state = 3;
        // fallthrough
    case 3:
        if (!IsReplyReceived(adapter)) {
            break;
        }
        adapter->state = 4;
        // fallthrough
    case 4:
        return TRUE;
    }
    return FALSE;
}

void *BtlAdapter_GetReturnData(BtlAdapter *adapter, u32 *size) {
    if (size != NULL) {
        *size = adapter->returnSize;
    }
    return adapter->returnData;
}

void BtlAdapter_SetIdle(BtlAdapter *adapter) {
    adapter->state = 0;
    if (adapter->isNet) {
        BtlNet_ResetReplies();
    }
}

static BOOL SendCmd(BtlAdapter *adapter) {
    if (adapter->isNet) {
        BtlAdapterCmd *cmd = &adapter->sendCmd;
        return BtlNet_SendServerCmd(adapter->clientId, cmd, cmd->size + 4);
    }
    return TRUE;
}

static BOOL IsReplyReceived(BtlAdapter *adapter) {
    if (adapter->isNet) {
        if (BtlNet_AreRepliesReceived()) {
            adapter->returnSize = BtlNet_GetReply(adapter->clientId, &adapter->returnData);
            adapter->returned = 1;
            return TRUE;
        }
        return FALSE;
    }
    return adapter->returned;
}

void BtlAdapter_ClearSendCmd(BtlAdapter *adapter) {
    adapter->sendCmd.cmd = 0;
    adapter->sendCmd.size = 4;
}

u8 BtlAdapter_GetRecvCmd(BtlAdapter *adapter) {
    if (adapter->isNet) {
        if (BtlNet_IsServerCmdReceived()) {
            void *buf;
            BtlAdapterCmd *recv;
            BtlNet_GetServerCmd(&buf);
            recv = buf;
            return recv->cmd;
        }
        return 0;
    }
    if (adapter->returned == 0) {
        return adapter->sendCmd.cmd;
    }
    return 0;
}

u16 BtlAdapter_GetRecvData(BtlAdapter *adapter, const void **data) {
    if (adapter->isNet) {
        void *buf;
        BtlAdapterCmd *recv;
        BtlNet_GetServerCmd(&buf);
        recv = buf;
        *data = recv->data;
        return recv->size;
    }
    *data = adapter->sendCmd.data;
    return adapter->sendCmd.size;
}

BOOL BtlAdapter_SetReturnData(BtlAdapter *adapter, const void *data, u32 size) {
    if (adapter->isNet) {
        return BtlNet_SendReply(data, size);
    }
    if (!adapter->returned) {
        adapter->returnData = data;
        adapter->returnSize = size;
        adapter->returned = 1;
    }
    return TRUE;
}
