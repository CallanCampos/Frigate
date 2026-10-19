// The Union Room's net commands from 0x1400 that the machines send each other, with the senders for them. The file's
// name is a guess: the ROM only shows that it is a file of its own, between union_main.c and union_subproc.c, by the
// assert file name ("") that its asserts share
#include "types.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "app/pokemon_trade.h"
#include "constants/zones.h"
#include "demo/shinka_demo.h"
#include "field/colosseum.h"
#include "field/event_colosseum_battle.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "field/field_controller.h"
#include "field/field_event.h"
#include "field/field_sound.h"
#include "field/union_app.h"
#include "field/union_comm.h"
#include "field/union_main.h"
#include "field/union_msg.h"
#include "field/union_oneself.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_sync.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "save/join_avenue.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The net commands, from UNION_NET_CMD_BASE
#define UNION_NET_CMD_BASE 0x1400
enum {
    UNION_NET_CMD_UNK0 = UNION_NET_CMD_BASE,
    UNION_NET_CMD_UNK1,
    UNION_NET_CMD_UNK2,
    UNION_NET_CMD_MAC,
    UNION_NET_CMD_UNK4,
    UNION_NET_CMD_PLAYER_INFO,
    UNION_NET_CMD_CARD_A,
    UNION_NET_CMD_UNK7,
    UNION_NET_CMD_UNK8,
    UNION_NET_CMD_UNK9,
    UNION_NET_CMD_ENTRY_REQUEST,
    UNION_NET_CMD_ENTRY_REPLY_2,
    UNION_NET_CMD_ENTRY_REPLY_1,
    UNION_NET_CMD_ENQUEUE,
    UNION_NET_CMD_UNKE,
    UNION_NET_CMD_REQUEST_STATUS,
    UNION_NET_CMD_STATUS,
    UNION_NET_CMD_APP_PROFILE,
    UNION_NET_CMD_PROFILE,
    UNION_NET_CMD_CONFIRM_PROFILE,
    UNION_NET_CMD_LEAVE,
    UNION_NET_CMD_JOIN_AVENUE,
};

void *UnionNet_GetRecvBuffer(int netId, void *work, int size) {
    UnionSystem *unisys = work;

    GFL_ASSERT_MSG(size <= 0x800, "size=%x, recv_size=%x\n", size, 0x800);
    return unisys->recvBuf[netId];
}

void UnionNet_RecvGroupDone(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    UnionComm_ActivateGroup(unisys);
    unisys->situ.mycomm.doneMask |= (u8)(1 << netId);
}

BOOL UnionMain_SendGroupDone(void) {
    return GFL_NetSendCommand(NetHandle_GetOwn(), UNION_NET_CMD_UNK0, 0, NULL);
}

void UnionNet_RecvUnk19(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;
    const u32 *value = data;

    if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn())) {
        self->mycomm.unk19 = *value;
    }
}

BOOL UnionMain_SendUnk19(u32 value) {
    return GFL_NetSendCommand(NetHandle_GetOwn(), UNION_NET_CMD_UNK1, sizeof(u32), &value);
}

void UnionNet_RecvUnk1A(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;
    const u32 *value = data;

    if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn())) {
        self->mycomm.unk1A = *value;
    }
}

BOOL UnionMain_SendUnk1A(u32 value) {
    return GFL_NetSendCommand(NetHandle_GetOwn(), UNION_NET_CMD_UNK2, sizeof(u32), &value);
}

void UnionNet_RecvMac(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;

    if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn())) {
        MI_CpuCopy8(data, self->mycomm.unk10, 8);
    }
}

BOOL UnionMain_SendMac(void) {
    u8 mac[8];

    MI_CpuFill8(mac, 0, sizeof(mac));
    OS_GetMacAddress(mac);
    mac[6] = 1;
    return GFL_NetSendCommand(NetHandle_GetOwn(), UNION_NET_CMD_MAC, sizeof(mac), mac);
}

void UnionNet_RecvUnk46_2(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;
    const u32 *value = data;

    if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn())) {
        self->mycomm.unk46_2 = *value;
    }
}

BOOL UnionMain_SendUnk46_2(u32 value) {
    return GFL_NetSendCommand(NetHandle_GetOwn(), UNION_NET_CMD_UNK4, sizeof(u32), &value);
}

void UnionNet_RecvPlayerInfo(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    PlayerInfo_Copy(data, GameData_GetPlayerInfoByNetID(unisys->param->gameData, netId));
    unisys->situ.mycomm.unk47 |= (u8)(1 << netId);
}

BOOL UnionMain_SendPlayerInfo(UnionSystem *unisys) {
    return GFL_NetSendCommand(NetHandle_GetOwn(), UNION_NET_CMD_PLAYER_INFO, PlayerInfo_GetSize(),
                              GetGameDataPlayerInfo(unisys->param->gameData));
}

void UnionNet_RecvCard(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;

    if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn())) {
        self->mycomm.recvFlag = 1;
        MI_CpuCopy8(data, unisys->alloc.target_card, size);
    }
}

BOOL UnionMain_SendCard(UnionSystem *unisys) {
    GFL_ASSERT(unisys->alloc.my_card != NULL);
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0xff, UNION_NET_CMD_CARD_A, 0x6c4, unisys->alloc.my_card,
                                       TRUE, FALSE, TRUE);
}

BOOL UnionMain_SendCardHead(UnionSystem *unisys) {
    GFL_ASSERT(unisys->alloc.my_card != NULL);
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0xff, UNION_NET_CMD_CARD_A, 0x678, unisys->alloc.my_card,
                                       TRUE, FALSE, TRUE);
}

void UnionNet_RecvColosseumMember(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;
    GameData *gameData = GSYS_GetGameData(unisys->param->gsys);
    u8 mac[8];

    if (colosseum == NULL || colosseum->commPlayer == NULL || colosseum->unk178 == NULL) {
        GFL_ASSERT(0);
        return;
    }
    OS_GetMacAddress(mac);
    func_ov036_021c3f34(colosseum->unk178, netId, GetGameDataPlayerInfo(gameData), ((const u8 *)data)[0x27], mac);
}

void UnionNet_RecvColosseumUnkDE(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;
    const u32 *value = data;

    if (colosseum == NULL) {
        GFL_ASSERT(0);
        return;
    }
    colosseum->unkDE = *value;
}

void UnionNet_RecvColosseumUnkCA(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum == NULL) {
        GFL_ASSERT(0);
        return;
    }
    colosseum->unkCA = 1;
}

BOOL UnionMain_SendColosseumUnkCA(void) {
    return GFL_NetSendCommand(NetHandle_GetOwn(), UNION_NET_CMD_UNK9, 0, NULL);
}

void UnionNet_RecvEntryRequest(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (unisys->alloc.uniapp == NULL) {
        GFL_ASSERT(0);
        unisys->entryRepliedTo |= (u8)(1 << netId);
        return;
    }
    if (UnionApp_RequestEntry(unisys->alloc.uniapp, netId) == TRUE) {
        unisys->entryAcceptedTo |= (u8)(1 << netId);
    } else {
        unisys->entryRepliedTo |= (u8)(1 << netId);
    }
}

BOOL UnionMain_SendEntryRequest(UnionSystem *unisys) {
    NetHandle *handle;
    u32 size;

    unisys->entryReply = 0;
    handle = NetHandle_GetOwn();
    size = PlayerInfo_GetSize();
    return GFL_NetSendCommandToMachine(handle, 0, UNION_NET_CMD_ENTRY_REQUEST, size,
                                       GetGameDataPlayerInfo(unisys->param->gameData), TRUE, FALSE, FALSE);
}

void UnionNet_RecvEntryReply2(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    unisys->entryReply = 2;
}

BOOL UnionMain_SendEntryReply2(u8 sendTo) {
    return GFL_NetSendCommandToBits(NetHandle_GetOwn(), sendTo, UNION_NET_CMD_ENTRY_REPLY_2, 0, NULL, TRUE, FALSE,
                                    FALSE);
}

void UnionNet_RecvEntryReply1(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    unisys->entryReply = 1;
}

BOOL UnionMain_SendEntryReply1(u8 sendTo) {
    return GFL_NetSendCommandToBits(NetHandle_GetOwn(), sendTo, UNION_NET_CMD_ENTRY_REPLY_1, 0, NULL, TRUE, FALSE,
                                    FALSE);
}

void UnionNet_RecvEnqueue(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (GFL_NetIsParent()) {
        if (unisys->alloc.uniapp == NULL) {
            GFL_ASSERT(unisys->alloc.uniapp != NULL);
            return;
        }
        UnionApp_Enqueue(unisys->alloc.uniapp, netId);
    }
}

BOOL UnionMain_SendEnqueue(void) {
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0, UNION_NET_CMD_ENQUEUE, 0, NULL, TRUE, FALSE, FALSE);
}

void UnionNet_RecvUnkE(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (unisys->alloc.uniapp == NULL) {
        GFL_ASSERT(unisys->alloc.uniapp != NULL);
        return;
    }
    UnionApp_SetTurnReceived(unisys->alloc.uniapp);
}

BOOL UnionMain_SendUnkE(u8 netId) {
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), netId, UNION_NET_CMD_UNKE, 0, NULL, TRUE, FALSE, FALSE);
}

void UnionNet_RecvRequestStatus(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (GFL_NetIsParent()) {
        if (unisys->alloc.uniapp == NULL) {
            GFL_ASSERT(unisys->alloc.uniapp != NULL);
            return;
        }
        UnionApp_RequestSendStatus(unisys->alloc.uniapp, netId);
    }
}

BOOL UnionMain_SendRequestStatus(void) {
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0, UNION_NET_CMD_REQUEST_STATUS, 0, NULL, TRUE, FALSE,
                                       FALSE);
}

void UnionNet_RecvStatus(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (GFL_NetIsParent() != TRUE) {
        if (unisys->alloc.uniapp == NULL) {
            GFL_ASSERT(0);
            return;
        }
        UnionApp_ReceiveStatus(unisys->alloc.uniapp, data);
    }
}

BOOL UnionMain_SendStatus(const UnionAppStatus *status, u8 sendTo) {
    NetHandle *handle = NetHandle_GetOwn();

    return GFL_NetSendCommandToBits(handle, sendTo, UNION_NET_CMD_STATUS, UnionApp_GetStatusSize(), status, TRUE, FALSE,
                                    FALSE);
}

void UnionNet_RecvAppProfile(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (unisys->alloc.uniapp == NULL) {
        GFL_ASSERT(0);
        return;
    }
    UnionApp_SetMemberProfile(unisys->alloc.uniapp, unisys, netId, (UnionAppMember *)data);
    UnionApp_RequestSendProfile(unisys->alloc.uniapp, netId);
}

BOOL UnionMain_SendAppProfile(u8 sendTo, const PlayerInfo *info) {
    UnionAppMember member;

    MI_CpuFill8(&member, 0, sizeof(UnionAppMember));
    member.info = *info;
    OS_GetMacAddress(member.mac);
    return GFL_NetSendCommandToBits(NetHandle_GetOwn(), sendTo, UNION_NET_CMD_APP_PROFILE, sizeof(UnionAppMember),
                                    &member, TRUE, FALSE, FALSE);
}

void UnionNet_RecvProfile(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (unisys->alloc.uniapp == NULL) {
        GFL_ASSERT(0);
        return;
    }
    UnionApp_SetMemberProfile(unisys->alloc.uniapp, unisys, netId, (UnionAppMember *)data);
}

BOOL UnionMain_SendProfile(u8 sendTo, const UnionAppMember *member) {
    return GFL_NetSendCommandToBits(NetHandle_GetOwn(), sendTo, UNION_NET_CMD_PROFILE, sizeof(UnionAppMember), member,
                                    TRUE, FALSE, FALSE);
}

void UnionNet_RecvConfirmProfile(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn())) {
        if (unisys->alloc.uniapp == NULL) {
            GFL_ASSERT(0);
            return;
        }
        UnionApp_SetMemberProfile(unisys->alloc.uniapp, unisys, netId, (UnionAppMember *)data);
        UnionApp_ConfirmEntry(unisys->alloc.uniapp, netId);
    }
}

BOOL UnionMain_SendConfirmProfile(u8 sendTo, const PlayerInfo *info) {
    UnionAppMember member;

    MI_CpuFill8(&member, 0, sizeof(UnionAppMember));
    member.info = *info;
    OS_GetMacAddress(member.mac);
    return GFL_NetSendCommandToBits(NetHandle_GetOwn(), sendTo, UNION_NET_CMD_CONFIRM_PROFILE, sizeof(UnionAppMember),
                                    &member, TRUE, FALSE, FALSE);
}

void UnionNet_RecvLeave(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    const u8 *leftNetId = data;

    if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn())) {
        if (unisys->alloc.uniapp == NULL) {
            GFL_ASSERT(0);
            return;
        }
        UnionApp_OnLeave(unisys->alloc.uniapp, *leftNetId);
    }
}

void UnionNet_RecvJoinAvenue(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(unisys->param->gameData));

    if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn())) {
        self->mycomm.unk4C = 1;
        JoinAvenuePerson_SetParam((JoinAvenuePerson *)data, JOIN_AVE_PARAM_UNK_26, 3);
        JoinAvenue_Add(joinAvenue, unisys->param->gameData, (void *)data, TRUE);
    }
}

BOOL UnionMain_SendLeave(u8 sendTo, u8 leftNetId) {
    return GFL_NetSendCommandToBits(NetHandle_GetOwn(), sendTo, UNION_NET_CMD_LEAVE, 1, &leftNetId, TRUE, FALSE, FALSE);
}

BOOL UnionMain_SendJoinAvenue(UnionSystem *unisys) {
    JoinAvenuePerson_InitPlayer(&unisys->joinPerson, GSYS_GetGameData(unisys->param->gsys));
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0xff, UNION_NET_CMD_JOIN_AVENUE, sizeof(JoinAvenuePerson),
                                       &unisys->joinPerson, TRUE, FALSE, TRUE);
}
