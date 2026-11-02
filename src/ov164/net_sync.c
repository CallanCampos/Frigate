#include "types.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_sync.h"

// The steps of NetSync_Update
enum {
    STATE_SEND_TIMING,
    STATE_WAIT_TIMING,
    STATE_READY,
};

const NetSyncCommand NetSync_CommandTable = { NetSync_RecvCallback, NULL };

void NetSync_Init(NetSyncWork *work) {
    work->state = STATE_SEND_TIMING;
}

void NetSync_Exit(NetSyncWork *work) {
    NetCommand_RemoveTable(0x2d00);
}

void NetSync_Update(NetSyncWork *work) {
    switch (work->state) {
    case STATE_SEND_TIMING:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 1, 0x2d);
        work->state = STATE_WAIT_TIMING;
        break;
    case STATE_WAIT_TIMING:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 1, 0x2d) == 1) {
            NetCommand_AddTable(0x2d00, &NetSync_CommandTable, 1, work);
            work->state = STATE_READY;
        }
        break;
    case STATE_READY:
        break;
    }
    if (GFL_NetErrCheck() && work->error == 0) {
        work->error = 1;
    }
}

u32 NetSync_Send(NetSyncWork *work, u8 kind, u16 value) {
    NetHandle *handle = NetHandle_GetOwn();
    NetSyncPacket packet;
    packet.kind = kind;
    packet.value = value;
    if (work->state != STATE_READY) {
        return 0;
    }
    return GFL_NetSendCommandToMachine(handle, 0xff, 0x2d00, 4, &packet, 1, 0, 0);
}

void NetSync_RecvCallback(u32 netId, u32 size, const NetSyncPacket *packet, NetSyncWork *work) {
    GFL_NetGetHandleNetId(NetHandle_GetOwn());
    if (packet->kind == 0) {
        work->received++;
    }
}

void NetSync_StartTiming(u32 ignored, u32 timing) {
    NetHandle_SendTimingPair(NetHandle_GetOwn(), timing, 0x2d);
}

BOOL NetSync_IsTimingDone(NetSyncWork *work, u32 timing) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), timing, 0x2d) == 1) {
        return TRUE;
    }
    if (work->error == 1) {
        return TRUE;
    }
    return FALSE;
}
