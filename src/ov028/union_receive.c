// The Union Room's net commands from 0x1400 that the machines send each other, with the senders for them. The file's
// name is a guess: the ROM only shows that it is a file of its own, between union_main.c and union_subproc.c, by the
// assert file name ("") that its asserts share
#include "field/union_main.h"
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

void func_ov028_02171138(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    UnionComm_ActivateGroup(unisys);
    unisys->situ.mycomm.doneMask |= (u8)(1 << netId);
}

BOOL func_ov028_0217115c(void) {
    return func_02042be8(func_02040440(), UNION_NET_CMD_UNK0, 0, NULL);
}

void func_ov028_02171170(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;
    const u32 *value = data;

    if (netId != func_02042a6c(func_02040440())) {
        self->mycomm.unk19 = *value;
    }
}

BOOL func_ov028_02171194(u32 value) {
    return func_02042be8(func_02040440(), UNION_NET_CMD_UNK1, sizeof(u32), &value);
}

void func_ov028_021711b4(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;
    const u32 *value = data;

    if (netId != func_02042a6c(func_02040440())) {
        self->mycomm.unk1A = *value;
    }
}

BOOL func_ov028_021711d8(u32 value) {
    return func_02042be8(func_02040440(), UNION_NET_CMD_UNK2, sizeof(u32), &value);
}

void func_ov028_021711f8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;

    if (netId != func_02042a6c(func_02040440())) {
        sys_memcpy(data, self->mycomm.unk10, 8);
    }
}

BOOL func_ov028_02171220(void) {
    u8 mac[8];

    sys_memset(mac, 0, sizeof(mac));
    func_0207c33c(mac);
    mac[6] = 1;
    return func_02042be8(func_02040440(), UNION_NET_CMD_MAC, sizeof(mac), mac);
}

void func_ov028_02171254(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;
    const u32 *value = data;

    if (netId != func_02042a6c(func_02040440())) {
        self->mycomm.unk46_2 = *value;
    }
}

BOOL func_ov028_0217128c(u32 value) {
    return func_02042be8(func_02040440(), UNION_NET_CMD_UNK4, sizeof(u32), &value);
}

void func_ov028_021712ac(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    func_02008b34(data, func_02017378(unisys->param->gameData, netId));
    unisys->situ.mycomm.unk47 |= (u8)(1 << netId);
}

BOOL func_ov028_021712dc(UnionSystem *unisys) {
    return func_02042be8(func_02040440(), UNION_NET_CMD_PLAYER_INFO, PlayerInfo_GetSize(),
                         GetGameDataPlayerInfo(unisys->param->gameData));
}

void func_ov028_02171308(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;

    if (netId != func_02042a6c(func_02040440())) {
        self->mycomm.recvFlag = 1;
        sys_memcpy(data, unisys->alloc.target_card, size);
    }
}

BOOL func_ov028_02171340(UnionSystem *unisys) {
    GFL_ASSERT(unisys->alloc.my_card != NULL);
    return func_02042c18(func_02040440(), 0xff, UNION_NET_CMD_CARD_A, 0x6c4, unisys->alloc.my_card, TRUE, FALSE, TRUE);
}

BOOL func_ov028_02171390(UnionSystem *unisys) {
    GFL_ASSERT(unisys->alloc.my_card != NULL);
    return func_02042c18(func_02040440(), 0xff, UNION_NET_CMD_CARD_A, 0x678, unisys->alloc.my_card, TRUE, FALSE, TRUE);
}

void func_ov028_021713e0(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;
    GameData *gameData = GSYS_GetGameData(unisys->param->gsys);
    u8 mac[8];

    if (colosseum == NULL || colosseum->commPlayer == NULL || colosseum->unk178 == NULL) {
        GFL_ASSERT(0);
        return;
    }
    func_0207c33c(mac);
    func_ov036_021c3f34(colosseum->unk178, netId, GetGameDataPlayerInfo(gameData), ((const u8 *)data)[0x27], mac);
}

void func_ov028_0217144c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;
    const u32 *value = data;

    if (colosseum == NULL) {
        GFL_ASSERT(0);
        return;
    }
    colosseum->unkDE = *value;
}

void func_ov028_02171478(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum == NULL) {
        GFL_ASSERT(0);
        return;
    }
    colosseum->unkCA = 1;
}

BOOL func_ov028_021714a4(void) {
    return func_02042be8(func_02040440(), UNION_NET_CMD_UNK9, 0, NULL);
}

void func_ov028_021714bc(int netId, int size, const void *data, void *work, NetHandle *handle) {
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

BOOL func_ov028_0217152c(UnionSystem *unisys) {
    NetHandle *handle;
    u32 size;

    unisys->entryReply = 0;
    handle = func_02040440();
    size = PlayerInfo_GetSize();
    return func_02042c18(handle, 0, UNION_NET_CMD_ENTRY_REQUEST, size, GetGameDataPlayerInfo(unisys->param->gameData),
                         TRUE, FALSE, FALSE);
}

void func_ov028_02171570(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    unisys->entryReply = 2;
}

BOOL func_ov028_0217157c(u8 sendTo) {
    return func_02042c9c(func_02040440(), sendTo, UNION_NET_CMD_ENTRY_REPLY_2, 0, NULL, TRUE, FALSE, FALSE);
}

void func_ov028_021715a4(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    unisys->entryReply = 1;
}

BOOL func_ov028_021715b0(u8 sendTo) {
    return func_02042c9c(func_02040440(), sendTo, UNION_NET_CMD_ENTRY_REPLY_1, 0, NULL, TRUE, FALSE, FALSE);
}

void func_ov028_021715d8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (func_02042bc4()) {
        if (unisys->alloc.uniapp == NULL) {
            GFL_ASSERT(unisys->alloc.uniapp != NULL);
            return;
        }
        UnionApp_Enqueue(unisys->alloc.uniapp, netId);
    }
}

BOOL UnionMain_SendEnqueue(void) {
    return func_02042c18(func_02040440(), 0, UNION_NET_CMD_ENQUEUE, 0, NULL, TRUE, FALSE, FALSE);
}

void func_ov028_02171638(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (unisys->alloc.uniapp == NULL) {
        GFL_ASSERT(unisys->alloc.uniapp != NULL);
        return;
    }
    func_ov069_0217cd5c(unisys->alloc.uniapp);
}

BOOL func_ov028_02171664(u8 netId) {
    return func_02042c18(func_02040440(), netId, UNION_NET_CMD_UNKE, 0, NULL, TRUE, FALSE, FALSE);
}

void func_ov028_0217168c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (func_02042bc4()) {
        if (unisys->alloc.uniapp == NULL) {
            GFL_ASSERT(unisys->alloc.uniapp != NULL);
            return;
        }
        UnionApp_RequestSendStatus(unisys->alloc.uniapp, netId);
    }
}

BOOL UnionMain_SendRequestStatus(void) {
    return func_02042c18(func_02040440(), 0, UNION_NET_CMD_REQUEST_STATUS, 0, NULL, TRUE, FALSE, FALSE);
}

void func_ov028_021716ec(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (func_02042bc4() != TRUE) {
        if (unisys->alloc.uniapp == NULL) {
            GFL_ASSERT(0);
            return;
        }
        UnionApp_ReceiveStatus(unisys->alloc.uniapp, data);
    }
}

BOOL func_ov028_02171724(const UnionAppStatus *status, u8 sendTo) {
    NetHandle *handle = func_02040440();

    return func_02042c9c(handle, sendTo, UNION_NET_CMD_STATUS, UnionApp_GetStatusSize(), status, TRUE, FALSE, FALSE);
}

void func_ov028_02171758(int netId, int size, const void *data, void *work, NetHandle *handle) {
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

    sys_memset(&member, 0, sizeof(UnionAppMember));
    member.info = *info;
    func_0207c33c(member.mac);
    return func_02042c9c(func_02040440(), sendTo, UNION_NET_CMD_APP_PROFILE, sizeof(UnionAppMember), &member, TRUE,
                         FALSE, FALSE);
}

void func_ov028_021717e8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (unisys->alloc.uniapp == NULL) {
        GFL_ASSERT(0);
        return;
    }
    UnionApp_SetMemberProfile(unisys->alloc.uniapp, unisys, netId, (UnionAppMember *)data);
}

BOOL func_ov028_0217181c(u8 sendTo, const UnionAppMember *member) {
    return func_02042c9c(func_02040440(), sendTo, UNION_NET_CMD_PROFILE, sizeof(UnionAppMember), member, TRUE, FALSE,
                         FALSE);
}

void func_ov028_02171848(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;

    if (netId != func_02042a6c(func_02040440())) {
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

    sys_memset(&member, 0, sizeof(UnionAppMember));
    member.info = *info;
    func_0207c33c(member.mac);
    return func_02042c9c(func_02040440(), sendTo, UNION_NET_CMD_CONFIRM_PROFILE, sizeof(UnionAppMember), &member, TRUE,
                         FALSE, FALSE);
}

void func_ov028_021718e4(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    const u8 *leftNetId = data;

    if (netId != func_02042a6c(func_02040440())) {
        if (unisys->alloc.uniapp == NULL) {
            GFL_ASSERT(0);
            return;
        }
        UnionApp_OnLeave(unisys->alloc.uniapp, *leftNetId);
    }
}

void func_ov028_02171920(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionSelf *self = &unisys->situ;
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(unisys->param->gameData));

    if (netId != func_02042a6c(func_02040440())) {
        self->mycomm.unk4C = 1;
        JoinAvenuePerson_SetParam((JoinAvenuePerson *)data, JOIN_AVE_PARAM_UNK_26, 3);
        func_02010078(joinAvenue, unisys->param->gameData, (void *)data, TRUE);
    }
}

BOOL func_ov028_0217196c(u8 sendTo, u8 leftNetId) {
    return func_02042c9c(func_02040440(), sendTo, UNION_NET_CMD_LEAVE, 1, &leftNetId, TRUE, FALSE, FALSE);
}

BOOL func_ov028_0217199c(UnionSystem *unisys) {
    func_02036dc0(&unisys->joinPerson, GSYS_GetGameData(unisys->param->gsys));
    return func_02042c18(func_02040440(), 0xff, UNION_NET_CMD_JOIN_AVENUE, sizeof(JoinAvenuePerson), &unisys->joinPerson,
                         TRUE, FALSE, TRUE);
}
