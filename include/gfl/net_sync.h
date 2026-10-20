#ifndef POKEBW2_GFL_NET_SYNC_H
#define POKEBW2_GFL_NET_SYNC_H

#include "types.h"
#include "struct_decls.h"

struct NetSyncWork {
    u8 pad0[0x72];
    u8 state;
    u8 error;
    u8 pad1[0x30];
    u8 received;
};

struct NetSyncPacket {
    u16 value;
    u8 kind;
    u8 pad;
};

struct NetSyncCommand {
    void (*callback)(u32 a0, u32 a1, const NetSyncPacket *packet, NetSyncWork *work);
    void *arg;
};

extern const NetSyncCommand data_ov164_021999e8;

void NetSync_Init(NetSyncWork *work);
void NetSync_Exit(NetSyncWork *work);
void NetSync_Update(NetSyncWork *work);
u32 NetSync_Send(NetSyncWork *work, u8 kind, u16 value);
void NetSync_RecvCallback(u32 a0, u32 a1, const NetSyncPacket *packet, NetSyncWork *work);
void NetSync_StartTiming(u32 ignored, u32 a1);
BOOL NetSync_IsTimingDone(NetSyncWork *work, u32 a1);

#endif // POKEBW2_GFL_NET_SYNC_H
