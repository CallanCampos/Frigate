#include "types.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_sync.h"

const NetSyncCommand data_ov164_021999e8 = { func_ov164_02199984, NULL };

void func_ov164_021998c0(NetSyncWork *work) {
    work->state = 0;
}

void func_ov164_021998c8(NetSyncWork *work) {
    func_02040c64(0x2d00);
}

void func_ov164_021998d4(NetSyncWork *work) {
    switch (work->state) {
    case 0:
        func_02040624(func_02040440(), 1, 0x2d);
        work->state = 1;
        break;
    case 1:
        if (func_02040664(func_02040440(), 1, 0x2d) == 1) {
            func_02040c20(0x2d00, &data_ov164_021999e8, 1, work);
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

u32 func_ov164_02199944(NetSyncWork *work, u8 kind, u16 value) {
    NetHandle *handle = func_02040440();
    NetSyncPacket packet;
    packet.kind = kind;
    packet.value = value;
    if (work->state != 2) {
        return 0;
    }
    return func_02042c18(handle, 0xff, 0x2d00, 4, &packet, 1, 0, 0);
}

void func_ov164_02199984(u32 a0, u32 a1, const NetSyncPacket *packet, NetSyncWork *work) {
    func_02042a6c(func_02040440());
    if (packet->kind == 0) {
        work->received++;
    }
}

void func_ov164_021999a8(u32 ignored, u32 a1) {
    func_02040624(func_02040440(), a1, 0x2d);
}

BOOL func_ov164_021999bc(NetSyncWork *work, u32 a1) {
    if (func_02040664(func_02040440(), a1, 0x2d) == 1) {
        return TRUE;
    }
    if (work->error == 1) {
        return TRUE;
    }
    return FALSE;
}
