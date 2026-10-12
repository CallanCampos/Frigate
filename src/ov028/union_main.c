// The Union Room's field side (the file's name is from its assert strings): it boots the communication, runs a command
// of the Union Room (a menu's choice, such as a trade or a battle) as a field event, and handles the net commands from
// 0x1400 that the machines send each other, with the senders for them
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

// What a command does, and the activity that this machine is in while it does: UNION_ACTIVITY_NONE leaves it as it is
#define UNION_ACTIVITY_NONE 0x1b

// What a command's handler gets: the GameSystem, the UnionSystem, the Field, the command's argument, the event that
// runs it, a place for an event to run before the next call, and the step, which starts at 0. It returns whether it is
// done
typedef BOOL (*UnionCommandFunc)(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                 GameEvent **next, u8 *step);

// A row of the command table
typedef struct {
    UnionCommandFunc func;
    u8 startActivity;
    u8 endActivity;
} UnionCommandInfo;

// The event that runs a command
typedef struct {
    GameSystem *gsys;
    Field *field;
    UnionSystem *unisys;
} UnionCommandEvent;

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

static BOOL UnionMain_SendEnqueue(void);
static BOOL UnionMain_SendRequestStatus(void);
static BOOL UnionMain_SendAppProfile(u8 sendTo, const PlayerInfo *info);
static BOOL UnionMain_SendConfirmProfile(u8 sendTo, const PlayerInfo *info);
static GameEventReturnCode UnionMain_RunCommand(GameEvent *event, u32 *seq, void *work);
static BOOL UnionCommand_TrainerCard(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                     GameEvent **next, u8 *step);
static BOOL UnionCommand_Trade(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step);
static BOOL UnionCommand_App(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                             GameEvent **next, u8 *step);
static BOOL UnionCommand_WarpA(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step);
static BOOL UnionCommand_WarpB(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step);
static BOOL UnionCommand_WarpRoom(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                  GameEvent **next, u8 *step);
static BOOL UnionCommand_Colosseum(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                   GameEvent **next, u8 *step);

void UnionMain_Boot(GameSystem *gsys) {
    GameCommSys *game_comm = GSYS_GetGameCommSystem(gsys);
    GameData *gameData = GSYS_GetGameData(gsys);
    UnionBootParam *param;

    GFL_ASSERT(GameCommSys_BootCheck(game_comm) == GAME_COMM_NO_NULL);
    param = GFL_HeapAllocate(5, sizeof(UnionBootParam), TRUE, "union_main.c", 55);
    param->playerInfo = GetGameDataPlayerInfo(gameData);
    param->comm = game_comm;
    param->gameData = gameData;
    param->gsys = gsys;
    GameCommSys_Boot(game_comm, GAME_COMM_NO_UNION, param);
}

void UnionMain_Update(GameCommSys *comm, Field *field) {
    UnionSystem *unisys;
    u8 activity;

    if (GameCommSys_BootCheck(comm) == GAME_COMM_NO_UNION) {
        unisys = GameCommSys_GetWork(comm);
        GFL_ASSERT(unisys != NULL);
        if (func_02016b14(Field_GetGameSystem(field)) == TRUE) {
            return;
        }
        if (GSYS_CheckNowEvent(unisys->param->gsys) == TRUE) {
            return;
        }
        if (UnionMain_IsFieldReady(unisys) == TRUE) {
            activity = unisys->situ.activity;
            if (activity >= 4 && activity <= 0x17) {
                if (UnionCommand_IsActive(unisys)) {
                    return;
                }
                UnionOneself_RunStep(unisys, field);
                return;
            }
            switch (activity) {
            case 0:
            case 1:
                if (UnionCommand_IsActive(unisys)) {
                    return;
                }
                UnionOneself_CheckEntries(unisys);
                UnionOneself_RunStep(unisys, field);
                UnionOneself_UpdateActors(unisys, unisys->param->gameData, field);
                break;
            }
        }
    }
}

BOOL UnionMain_IsFieldReady(UnionSystem *unisys) {
    if (unisys->fieldActive == TRUE && GSYS_CheckField(unisys->param->gsys) == TRUE &&
        Field_CheckMapLoadFinished(GSYS_GetField(unisys->param->gsys)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

void UnionMain_OnFieldIn(void *param, void *work, Field *field) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    unisys->fieldActive = TRUE;
    if (unisys->updateDisabled == TRUE) {
        FieldmapCtrlGrid_SetUpdateDisable(field, TRUE);
    }
    if (GetZoneIsUnionRoom(Field_GetPlayerStateZoneID(field)) == TRUE) {
        UnionOneself_UpdateMarks(unisys, GSYS_GetGameData(unisys->param->gsys), field);
    }
    if (colosseum != NULL && colosseum->unkC8_0 == TRUE) {
        func_ov012_021615a4(colosseum->commPlayer);
    }
}

void UnionMain_OnFieldOut(void *param, void *work, Field *field) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        if (colosseum->unkC9 == 1) {
            Colosseum_Free(colosseum, unisys->param->gsys);
            unisys->colosseum_sys = NULL;
        } else if (colosseum->unkC8_0 == TRUE) {
            func_ov012_02161544(colosseum->commPlayer);
        }
    }
    unisys->fieldActive = FALSE;
}

u32 func_ov028_021710a4(UnionSystem *unisys) {
    return unisys->unk3630;
}

u32 func_ov028_021710b0(UnionSystem *unisys) {
    return unisys->unk3634;
}

void func_ov028_021710bc(UnionSystem *unisys, u8 value) {
    unisys->situ.unk02 = value;
}

u8 func_ov028_021710c8(UnionSystem *unisys) {
    return unisys->situ.unk02;
}

BOOL UnionMain_IsIdle(GameSystem *gsys) {
    UnionSystem *unisys = GameCommSys_GetWork(GSYS_GetGameCommSystem(gsys));

    if (unisys == NULL) {
        return FALSE;
    }
    if ((unisys->situ.step == 0 || unisys->situ.step == 0x2a) && unisys->updateDisabled == 0) {
        return TRUE;
    }
    return FALSE;
}

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

static BOOL UnionMain_SendEnqueue(void) {
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

static BOOL UnionMain_SendRequestStatus(void) {
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

static BOOL UnionMain_SendAppProfile(u8 sendTo, const PlayerInfo *info) {
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

static BOOL UnionMain_SendConfirmProfile(u8 sendTo, const PlayerInfo *info) {
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

BOOL func_ov028_0217196c(u8 sendTo, u8 leftNetId) {
    return func_02042c9c(func_02040440(), sendTo, UNION_NET_CMD_LEAVE, 1, &leftNetId, TRUE, FALSE, FALSE);
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

BOOL func_ov028_0217199c(UnionSystem *unisys) {
    func_02036dc0(&unisys->joinPerson, GSYS_GetGameData(unisys->param->gsys));
    return func_02042c18(func_02040440(), 0xff, UNION_NET_CMD_JOIN_AVENUE, sizeof(JoinAvenuePerson), &unisys->joinPerson,
                         TRUE, FALSE, TRUE);
}

GameEvent *UnionCommand_CreateEvent(GameSystem *gsys, Field *field, UnionSystem *unisys) {
    GameEvent *event = GameEvent_Create(gsys, NULL, UnionMain_RunCommand, sizeof(UnionCommandEvent));
    UnionCommandEvent *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->field = field;
    data->unisys = unisys;
    unisys->subproc.active = TRUE;
    return event;
}

// The commands, by the id that UnionCommand_Set starts one with. Each is run as an event by UnionMain_RunCommand:
// 1 the trainer card, 2 a trade, 3 and 4 the app's procs, 5 to 20 and 21 to 24 the warps into a room of a seat, 25 a
// warp, 26 the colosseum's party select, 28 the trainer card again, and 27 and 29 are colosseum.c's
static const UnionCommandInfo sUnionCommands[] = {
    { NULL, 0, 0 },
    { UnionCommand_TrainerCard, 2, 1 },
    { UnionCommand_Trade, 0x18, 1 },
    { UnionCommand_App, 3, 0 },
    { UnionCommand_App, 0x19, 0 },
    { UnionCommand_WarpA, 4, 4 },
    { UnionCommand_WarpA, 5, 5 },
    { UnionCommand_WarpA, 6, 6 },
    { UnionCommand_WarpA, 7, 7 },
    { UnionCommand_WarpA, 8, 8 },
    { UnionCommand_WarpA, 9, 9 },
    { UnionCommand_WarpA, 10, 10 },
    { UnionCommand_WarpA, 11, 11 },
    { UnionCommand_WarpA, 12, 12 },
    { UnionCommand_WarpA, 13, 13 },
    { UnionCommand_WarpA, 14, 14 },
    { UnionCommand_WarpA, 15, 15 },
    { UnionCommand_WarpA, 16, 16 },
    { UnionCommand_WarpA, 17, 17 },
    { UnionCommand_WarpA, 18, 18 },
    { UnionCommand_WarpA, 19, 19 },
    { UnionCommand_WarpB, 20, 20 },
    { UnionCommand_WarpB, 21, 21 },
    { UnionCommand_WarpB, 22, 22 },
    { UnionCommand_WarpB, 23, 23 },
    { UnionCommand_WarpRoom, UNION_ACTIVITY_NONE, UNION_ACTIVITY_NONE },
    { UnionCommand_Colosseum, UNION_ACTIVITY_NONE, UNION_ACTIVITY_NONE },
    { Colosseum_CommandBattle, UNION_ACTIVITY_NONE, UNION_ACTIVITY_NONE },
    { UnionCommand_TrainerCard, UNION_ACTIVITY_NONE, UNION_ACTIVITY_NONE },
    { Colosseum_CommandPmsSelect, UNION_ACTIVITY_NONE, UNION_ACTIVITY_NONE },
};

static GameEventReturnCode UnionMain_RunCommand(GameEvent *event, u32 *seq, void *work) {
    UnionCommandEvent *data = work;
    GameSystem *gsys = data->gsys;
    UnionSystem *unisys = data->unisys;
    UnionSubproc *subproc = &unisys->subproc;
    GameEvent *next = NULL;
    BOOL done;

    switch (*seq) {
    case 0:
        UnionMsg_FreeAll(unisys);
        if (sUnionCommands[subproc->id].startActivity != UNION_ACTIVITY_NONE) {
            UnionComm_SetSelf(unisys, 4, sUnionCommands[subproc->id].startActivity);
        }
        (*seq)++;
        // fallthrough
    case 1:
        done = sUnionCommands[subproc->id].func(gsys, unisys, data->field, subproc->arg, event, &next, &subproc->step);
        if (next != NULL) {
            GameEvent_ChainNext(event, next);
        } else if (done == TRUE) {
            (*seq)++;
        }
        break;
    case 2:
        if (sUnionCommands[subproc->id].endActivity != UNION_ACTIVITY_NONE) {
            UnionComm_SetSelf(unisys, 4, sUnionCommands[subproc->id].endActivity);
        }
        subproc->id = 0;
        subproc->arg = NULL;
        subproc->step = 0;
        subproc->active = FALSE;
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void UnionCommand_Set(UnionSystem *unisys, u32 id, void *arg) {
    GFL_ASSERT(unisys->subproc.id == UNION_SUBPROC_ID_NULL && unisys->subproc.active == FALSE);
    unisys->subproc.id = id;
    unisys->subproc.arg = arg;
    unisys->subproc.step = 0;
}

BOOL UnionCommand_IsActive(UnionSystem *unisys) {
    if (unisys->subproc.id == UNION_SUBPROC_ID_NULL && unisys->subproc.active == FALSE) {
        return FALSE;
    }
    return TRUE;
}

static BOOL UnionCommand_TrainerCard(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                     GameEvent **next, u8 *step) {
    if (*step == 0) {
        *next = EventFieldSubprocessTransition_Create(gsys, field, OVERLAY_ID(186), &data_ov012_0216dd6c, arg);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}

static BOOL UnionCommand_Trade(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step) {
    PokemonTradeParam *trade = arg;
    ShinkaDemoParam *evolution;

    switch (*step) {
    case 0:
        *next = CallFieldMapEntranceOutTransitionDefault(gsys, field, 0, 0);
        (*step)++;
        break;
    case 1:
        *next = CreateFieldCloseEvent(gsys, field);
        (*step)++;
        break;
    case 2:
        GSYS_QueueProc(gsys, OVERLAY_POKEMONTRADE, &data_ov194_021c640c, trade);
        (*step)++;
        break;
    case 3:
        if (GSYS_GetProcMgrState(gsys) != 0) {
            break;
        }
        if (trade->next == TRADE_NEXT_EVOLVE) {
            *step = 4;
        } else {
            *step = 6;
        }
        break;
    case 4:
        evolution = trade->evolution;
        evolution->gameData = trade->gameData;
        evolution->party = trade->party;
        evolution->species = trade->evolveSpecies;
        evolution->partyIndex = 0;
        evolution->method = trade->evolveMethod;
        evolution->unkC = 1;
        evolution->canCancel = FALSE;
        trade->evolution = evolution;
        GSYS_QueueProc(gsys, OVERLAY_SHINKA_DEMO, &SHINKA_DEMO_PROC_FUNCTIONS, evolution);
        (*step)++;
        break;
    case 5:
        if (GSYS_GetProcMgrState(gsys) != 0) {
            break;
        }
        if (GFL_NetErrCheck()) {
            *step = 6;
        } else {
            trade->next = TRADE_NEXT_EVOLVE;
            *step = 2;
        }
        break;
    case 6:
        *next = EventFieldOpen_CreateHeadless(gsys);
        *step = 7;
        break;
    case 7:
        *next = CallFieldMapEntranceInTransition(gsys, field, 0, 0, 1, 0, 0);
        *step = 8;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL UnionCommand_App(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                             GameEvent **next, u8 *step) {
    UnionSelf *self = &unisys->situ;
    UnionProcParam *param;
    GameEvent *chain;
    u32 joined;

    if (GFL_NetErrCheck() && *step > 2 && *step <= 13) {
        *step = 16;
    }
    if (unisys->alloc.uniapp != NULL) {
        UnionApp_Update(unisys->alloc.uniapp, unisys);
    }
    switch (*step) {
    case 0:
        chain = CallFieldMapEntranceOutTransitionDefault(gsys, field, 0, 0);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case 1:
        chain = CreateFieldCloseEvent(gsys, field);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case 2:
        GFL_OvlLoad(OVERLAY_ID(69));
        unisys->alloc.uniapp = UnionApp_Create(unisys, 3, 5, GetGameDataPlayerInfo(unisys->param->gameData));
        if (self->mycomm.unk46_1 == 1) {
            *step = 5;
        } else {
            (*step)++;
        }
        break;
    case 3:
        func_02040624(func_02040440(), 9, 0x14);
        (*step)++;
        break;
    case 4:
        if (func_02040664(func_02040440(), 9, 0x14) == TRUE) {
            *step = 7;
        }
        break;
    case 5:
        if (UnionMain_SendEnqueue() == TRUE) {
            (*step)++;
        }
        break;
    case 6:
        if (func_ov069_0217cd64(unisys->alloc.uniapp) == TRUE) {
            *step = 7;
        }
        break;
    case 7:
        if (UnionMain_SendRequestStatus() == TRUE) {
            (*step)++;
        }
        break;
    case 8:
        if (UnionApp_HasMembers(unisys->alloc.uniapp) == TRUE) {
            (*step)++;
        }
        break;
    case 9:
        joined = UnionApp_GetJoined(unisys->alloc.uniapp);
        if (UnionMain_SendAppProfile(joined, GetGameDataPlayerInfo(unisys->param->gameData)) == TRUE) {
            (*step)++;
        }
        break;
    case 10:
        if (UnionApp_AllProfilesReceived(unisys->alloc.uniapp) == TRUE) {
            (*step)++;
        }
        break;
    case 11:
        if (self->mycomm.unk46_1 == 1) {
            joined = UnionApp_GetJoined(unisys->alloc.uniapp);
            if (UnionMain_SendConfirmProfile(joined, GetGameDataPlayerInfo(unisys->param->gameData)) == TRUE) {
                UnionApp_JoinSelf(unisys->alloc.uniapp);
                *step = 13;
            }
        } else {
            UnionComm_SetCommandBase(self->activity);
            func_02040624(func_02040440(), 10, 0x14);
            *step = 12;
        }
        break;
    case 12:
        if (func_02040664(func_02040440(), 10, 0x14) == TRUE) {
            func_02042a9c(func_02040440(), TRUE);
            *step = 13;
        }
        break;
    case 13:
        func_02042e9c(FALSE);
        param = unisys->subproc.arg;
        if (self->activity == 3) {
            param->app = unisys->alloc.uniapp;
            GSYS_QueueProc(gsys, OVERLAY_ID(70), &OEKAKI_PROC_FUNCTIONS, param);
        } else {
            param->app = unisys->alloc.uniapp;
            GSYS_QueueProc(gsys, OVERLAY_ID(216), &GURU2_PROC_FUNCTIONS, param);
        }
        (*step)++;
        break;
    case 14:
        if (GSYS_GetProcMgrState(gsys) != 0) {
            break;
        }
        param = unisys->subproc.arg;
        if (self->activity == 0x19) {
            if (param->unk14 == 1) {
                func_02005e08((u16)GetMapBGMIDByPlayerState2(unisys->param->gameData, ZONE_UNION_ROOM,
                                                             GameData_GetSeason(unisys->param->gameData)),
                              0xffff, 6, 0x3c);
                *step = 15;
            } else {
                *step = 16;
            }
        } else {
            *step = 16;
        }
        break;
    case 15:
        if (GFL_SndBGMIsFading()) {
            break;
        }
        *step = 16;
        break;
    case 16:
        if (unisys->alloc.uniapp != NULL) {
            UnionApp_Free(unisys->alloc.uniapp);
            unisys->alloc.uniapp = NULL;
            GFL_OvlUnload(OVERLAY_ID(69));
        }
        *step = 17;
        break;
    case 17:
        chain = EventFieldOpen_CreateHeadless(gsys);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case 18:
        chain = CallFieldMapEntranceInTransition(gsys, field, 0, 0, 1, 0, 0);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case 19:
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL UnionCommand_WarpA(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step) {
    VecFx32 pos;

    if (*step == 0) {
        pos.x = (func_02042a6c(func_02040440()) * 16 + 88) << FX32_SHIFT;
        pos.y = 0;
        pos.z = FX32_CONST(152);
        *next = EventMapChangeWarp_CreateGrid(gsys, field, ZONE_150, &pos, 0, FALSE);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}

static BOOL UnionCommand_WarpB(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                               GameEvent **next, u8 *step) {
    VecFx32 pos;

    if (*step == 0) {
        pos.x = (func_02042a6c(func_02040440()) * 16 + 88) << FX32_SHIFT;
        pos.y = 0;
        pos.z = FX32_CONST(152);
        *next = EventMapChangeWarp_CreateGrid(gsys, field, ZONE_151, &pos, 0, FALSE);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}

static BOOL UnionCommand_WarpRoom(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                  GameEvent **next, u8 *step) {
    VecFx32 pos;

    if (*step == 0) {
        pos.x = FX32_CONST(184);
        pos.y = 0;
        pos.z = FX32_CONST(232);
        *next = EventMapChangeWarp_CreateGrid(gsys, field, ZONE_UNION_ROOM, &pos, 0, FALSE);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}

static BOOL UnionCommand_Colosseum(GameSystem *gsys, UnionSystem *unisys, Field *field, void *arg, GameEvent *event,
                                   GameEvent **next, u8 *step) {
    UnionColosseumParam *param = arg;
    UnionColosseum *colosseum = unisys->colosseum_sys;
    PStatusParam *status = &param->status;
    ColosseumBattleParam *battle = param->battle;
    u32 selfId = func_02042a6c(func_02040440());
    PokeParty *tmpParty;
    PokeParty *party;
    GameEvent *chain;
    WifiList *wifiList;
    u32 friendIndex;
    int count;
    u32 isRedTeam;
    int i;

    if (*step > 2 && *step < 7) {
        func_ov164_021998d4((NetSyncWork *)param);
    }
    switch (*step) {
    case 0:
        chain = CallFieldMapEntranceOutTransitionDefault(gsys, field, 0, 0);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case 1:
        chain = CreateFieldCloseEvent(gsys, field);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case 2:
        GFL_OvlLoad(OVERLAY_ID(164));
        func_ov164_021998c0((NetSyncWork *)param);
        (*step)++;
        break;
    case 3:
        GSYS_QueueProc(gsys, OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, param);
        (*step)++;
        break;
    case 4:
        if (GSYS_GetProcMgrState(gsys) != 0) {
            break;
        }
        if (param->list.result == 1) {
            *step = 5;
        } else {
            *step = 7;
        }
        break;
    case 5:
        status->party = param->list.party;
        status->trainerData = param->list.trainerData;
        status->gameData = GSYS_GetGameData(gsys);
        status->dataType = PSTATUS_DATA_PARTY;
        status->partyCount = PokeParty_GetPkmCount(param->list.party);
        status->mode = PSTATUS_MODE_NORMAL;
        status->partyIndex = param->list.index;
        status->page = PSTATUS_PAGE_INFO;
        status->isNationalDex = PokeDex_IsNationalObtained(GameData_GetPokedex(status->gameData));
        GSYS_QueueProc(gsys, OVERLAY_PSTATUS, &PSTATUS_PROC_FUNCTIONS, status);
        (*step)++;
        break;
    case 6:
        if (GSYS_GetProcMgrState(gsys) != 0) {
            break;
        }
        param->list.index = status->partyIndex;
        *step = 3;
        break;
    case 7:
        func_ov164_021998c8((NetSyncWork *)param);
        GFL_OvlUnload(OVERLAY_ID(164));
        *step = 8;
        break;
    case 8:
        if (GFL_NetErrCheck()) {
            *step = 17;
            break;
        }
        GFL_ASSERT_MSG(param->list.result == 0, "plist->ret_mode \x95\x73\x90\xb3 %d\n", param->list.result);
        GFL_ASSERT_MSG(param->list.index == 6, "ret_sel=%d\n", param->list.index);
        party = colosseum->party[selfId];
        tmpParty = PokeParty_Create(0x41);
        PokeParty_Copy(party, tmpParty);
        PokeParty_Init(party);
        for (i = 0; i < 6; i++) {
            if (param->list.picked[i] == 0) {
                break;
            }
            PokeParty_AddPkm(party, PokeParty_GetPkm(tmpParty, param->list.picked[i] - 1));
        }
        GFL_HeapFree(tmpParty);
        func_ov028_02172b4c(colosseum, TRUE);
        func_02040624(func_02040440(), 22, 0x14);
        (*step)++;
        break;
    case 9:
        if (GFL_NetErrCheck()) {
            *step = 17;
            break;
        }
        if (func_02040664(func_02040440(), 22, 0x14) == TRUE) {
            (*step)++;
        }
        break;
    case 10:
        if (GFL_NetErrCheck()) {
            *step = 17;
            break;
        }
        if (Colosseum_SendParty(colosseum->party[selfId]) == TRUE) {
            (*step)++;
        }
        break;
    case 11:
        if (GFL_NetErrCheck()) {
            *step = 17;
            break;
        }
        if (func_ov028_02172b20(colosseum) == TRUE) {
            (*step)++;
        }
        break;
    case 12:
        battle->party = colosseum->party[selfId];
        battle->unk12 = colosseum->unk16C[selfId];
        battle->regulation = unisys->alloc.regulation;
        if (selfId == 0 || (colosseum->unk16C[0] & 2) == (colosseum->unk16C[selfId] & 2)) {
            battle->bgm = 0x489;
        } else {
            battle->bgm = 0x48a;
        }
        Colosseum_BuildBattlePlayers(colosseum, gsys, &battle->players, 0x41);
        *next = func_ov012_02152704(gsys, field, unisys->situ.activity, battle);
        (*step)++;
        break;
    case 13:
        if (!GFL_NetErrCheck()) {
            wifiList = GameData_GetWifiList(GSYS_GetGameData(gsys));
            count = func_02042a78();
            isRedTeam = colosseum->unk16C[selfId] & 1;
            for (i = 0; i < count; i++) {
                if (i == selfId) {
                    continue;
                }
                if (func_0200a438(wifiList, &colosseum->members[i].info, &friendIndex) != TRUE) {
                    continue;
                }
                if (isRedTeam == (colosseum->unk16C[i] & 1)) {
                    func_0200a29c(wifiList, friendIndex);
                } else {
                    switch (battle->players.battle.result) {
                    case 0:
                        func_0200a2d4(wifiList, friendIndex, 1, 0, 0);
                        break;
                    case 1:
                        func_0200a2d4(wifiList, friendIndex, 0, 1, 0);
                        break;
                    case 2:
                        func_0200a29c(wifiList, friendIndex);
                        break;
                    }
                }
            }
        }
        func_ov028_02172c60(&battle->players);
        *step = 16;
        break;
    case 14:
        chain = EventFieldOpen_CreateHeadless(gsys);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case 15:
        chain = CallFieldMapEntranceInTransition(gsys, field, 0, 0, 1, 0, 0);
        GameEvent_ChainNext(event, chain);
        (*step)++;
        break;
    case 17:
        *step = 14;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}
