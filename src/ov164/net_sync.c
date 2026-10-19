#include "types.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_sync.h"

const NetSyncCommand data_ov164_021999e8 = { NetSync_RecvCallback, NULL };

void NetSync_Init(NetSyncWork *work) {
    work->state = 0;
}

void NetSync_Exit(NetSyncWork *work) {
    NetCommand_RemoveTable(0x2d00);
}

void NetSync_Update(NetSyncWork *work) {
    switch (work->state) {
    case 0:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 1, 0x2d);
        work->state = 1;
        break;
    case 1:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 1, 0x2d) == 1) {
            NetCommand_AddTable(0x2d00, &data_ov164_021999e8, 1, work);
            work->state = 2;
        }
        break;
    case 2:
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
    if (work->state != 2) {
        return 0;
    }
    return GFL_NetSendCommandToMachine(handle, 0xff, 0x2d00, 4, &packet, 1, 0, 0);
}

void NetSync_RecvCallback(u32 a0, u32 a1, const NetSyncPacket *packet, NetSyncWork *work) {
    GFL_NetGetHandleNetId(NetHandle_GetOwn());
    if (packet->kind == 0) {
        work->received++;
    }
}

void NetSync_StartTiming(u32 ignored, u32 a1) {
    NetHandle_SendTimingPair(NetHandle_GetOwn(), a1, 0x2d);
}

BOOL NetSync_IsTimingDone(NetSyncWork *work, u32 a1) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), a1, 0x2d) == 1) {
        return TRUE;
    }
    if (work->error == 1) {
        return TRUE;
    }
    return FALSE;
}
