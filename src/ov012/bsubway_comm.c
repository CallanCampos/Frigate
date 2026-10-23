// The Battle Subway's multi-battle communication with the partner: the beacon, and the commands that send the members,
// the trainers of the train and the partner's choices. The name is descriptive; the file has no assert to give it
#include "types.h"
#include "constants/pokemon.h"
#include "field/bsubway_scr.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_system.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "system/game_data.h"

static void *BSubwayComm_GetBeacon(void *work);
static int BSubwayComm_GetBeaconSize(void *work);
static BOOL BSubwayComm_IsMatchingBeacon(int a, int b);
static void BSubwayComm_SetMembersData(BSubwayScrWork *bsw, GameData *gameData);
static void BSubwayComm_SetTrainersData(BSubwayScrWork *bsw);
static void BSubwayComm_SetRetireData(BSubwayScrWork *bsw, u16 retire);
static void BSubwayComm_SetPlayModeData(BSubwayScrWork *bsw);
static void BSubwayComm_SetCommand5Data(BSubwayScrWork *bsw, u16 value);
static void BSubwayComm_SetCommand6Data(BSubwayScrWork *bsw, u16 value);
static void BSubwayComm_SetCommand7Data(BSubwayScrWork *bsw, u16 value);
static void BSubwayComm_RecvMembers(int netId, int size, const void *data, void *work, NetHandle *handle);
static void BSubwayComm_RecvTrainers(int netId, int size, const void *data, void *work, NetHandle *handle);
static void BSubwayComm_RecvRetire(int netId, int size, const void *data, void *work, NetHandle *handle);
static void BSubwayComm_RecvPlayerInfo(int netId, int size, const void *data, void *work, NetHandle *handle);
static void BSubwayComm_RecvPlayMode(int netId, int size, const void *data, void *work, NetHandle *handle);
static void BSubwayComm_RecvCommand5(int netId, int size, const void *data, void *work, NetHandle *handle);
static void BSubwayComm_RecvCommand6(int netId, int size, const void *data, void *work, NetHandle *handle);
static void BSubwayComm_RecvCommand7(int netId, int size, const void *data, void *work, NetHandle *handle);

// clang-format off
static const NetCommand data_ov012_0216d8fc[] = {
    {BSubwayComm_RecvMembers, NULL},
    {BSubwayComm_RecvTrainers, NULL},
    {BSubwayComm_RecvRetire, NULL},
    {BSubwayComm_RecvPlayerInfo, NULL},
    {BSubwayComm_RecvPlayMode, NULL},
    {BSubwayComm_RecvCommand5, NULL},
    {BSubwayComm_RecvCommand6, NULL},
    {BSubwayComm_RecvCommand7, NULL},
};

// clang-format on

// clang-format off
static const GFLNetInitData data_ov012_0216d93c = {
    data_ov012_0216d8fc,
    NELEMS(data_ov012_0216d8fc),
    NULL,
    NULL,
    NULL,
    NULL,
    BSubwayComm_GetBeacon,
    BSubwayComm_GetBeaconSize,
    BSubwayComm_IsMatchingBeacon,
    NULL,
    NULL,
    NULL,
    NULL,
    {0},
    NULL,
    NULL,
    0,
    {1, 0, 0, 0, 0x80, 0x13, 0, 0},
    HEAPID_USER,
    HEAPID_TAIL(0xd),
    HEAPID_TAIL(0xf),
    HEAPID_TAIL(0xd),
    0xf0,
    0,
    2,
    0x60,
    8,
    1,
    0,
    GFL_NET_TYPE_WIRELESS,
    0,
    0x2c,
    {0x2c, 1, 0, 0},
    0,
    0,
};

// clang-format on

void BSubwayComm_InitNet(BSubwayScrWork *bsw) {
    GFL_NetInit(&data_ov012_0216d93c, NULL, bsw);
}

static void *BSubwayComm_GetBeacon(void *work) {
    BSubwayScrWork *bsw = work;
    BSubwayBeacon *beacon = &bsw->beacon;

    PlayerInfo_Copy(GetGameDataPlayerInfo(bsw->gameData), &beacon->playerInfo);
    OS_GetMacAddress(beacon->mac);
    beacon->gameId = 0x3a0b;
    return beacon;
}

static int BSubwayComm_GetBeaconSize(void *work) {
    return sizeof(BSubwayBeacon);
}

static BOOL BSubwayComm_IsMatchingBeacon(int a, int b) {
    if (a == b) {
        return TRUE;
    }
    return FALSE;
}

void BSubwayComm_AddCommands(BSubwayScrWork *bsw) {
    // The count passed is the end command, not the table's length
    NetCommand_AddTable(BSUBWAY_COMM_MEMBERS, data_ov012_0216d8fc, BSUBWAY_COMM_MAX, bsw);
}

void BSubwayComm_RemoveCommands(BSubwayScrWork *bsw) {
    NetCommand_RemoveTable(BSUBWAY_COMM_MEMBERS);
}

void BSubwayComm_SendTiming(u8 timing) {
    GFL_NetSendTiming(NetHandle_GetOwn(), timing);
}

BOOL BSubwayComm_IsTimingRecv(u8 timing) {
    return GFL_NetIsTimingRecv(NetHandle_GetOwn(), timing);
}

static void BSubwayComm_SetMembersData(BSubwayScrWork *bsw, GameData *gameData) {
    PlayerInfo *info = GetGameDataPlayerInfo(gameData);
    PokeParty *party = func_ov033_0217bd60(bsw);
    int i;

    bsw->sendBuf[0] = getTrainerGender(info);
    for (i = 0; i < 2; i++) {
        bsw->sendBuf[1 + i] = PokeParty_GetParam(PokeParty_GetPkm(party, bsw->memberSlots[i]), PKM_PARAM_SPECIES, NULL);
    }
    bsw->sendBuf[3] = func_ov033_0217be1c(func_ov033_0217bd84(bsw));
}

static void BSubwayComm_SetTrainersData(BSubwayScrWork *bsw) {
    MI_CpuCopy8(bsw->unk32, bsw->sendBuf, 14 * sizeof(u16));
}

static void BSubwayComm_SetRetireData(BSubwayScrWork *bsw, u16 retire) {
    bsw->unkC_3 = retire;
    bsw->sendBuf[0] = retire;
}

static void BSubwayComm_SetPlayModeData(BSubwayScrWork *bsw) {
    *(u32 *)bsw->sendBuf = bsw->playMode;
}

static void BSubwayComm_SetCommand5Data(BSubwayScrWork *bsw, u16 value) {
    *(u32 *)bsw->sendBuf = value;
}

static void BSubwayComm_SetCommand6Data(BSubwayScrWork *bsw, u16 value) {
    *(u32 *)bsw->sendBuf = value;
}

static void BSubwayComm_SetCommand7Data(BSubwayScrWork *bsw, u16 value) {
    *(u32 *)bsw->sendBuf = value;
}

void BSubwayComm_SetSendData(BSubwayScrWork *bsw, u16 mode, u16 value) {
    GameData *gameData = bsw->gameData;

    switch (mode) {
    case 0:
        bsw->sendCommand = BSUBWAY_COMM_MEMBERS;
        BSubwayComm_SetMembersData(bsw, gameData);
        break;
    case 1:
        bsw->sendCommand = BSUBWAY_COMM_TRAINERS;
        BSubwayComm_SetTrainersData(bsw);
        break;
    case 2:
        bsw->sendCommand = BSUBWAY_COMM_RETIRE;
        BSubwayComm_SetRetireData(bsw, value);
        break;
    case 3:
        bsw->sendCommand = BSUBWAY_COMM_PLAYER_INFO;
        PlayerInfo_Copy(GetGameDataPlayerInfo(gameData), (PlayerInfo *)bsw->sendBuf);
        break;
    case 4:
        bsw->sendCommand = BSUBWAY_COMM_PLAY_MODE;
        BSubwayComm_SetPlayModeData(bsw);
        break;
    case 5:
        bsw->sendCommand = BSUBWAY_COMM_5;
        BSubwayComm_SetCommand5Data(bsw, value);
        break;
    case 6:
        bsw->sendCommand = BSUBWAY_COMM_6;
        BSubwayComm_SetCommand6Data(bsw, value);
        break;
    case 7:
        bsw->sendCommand = BSUBWAY_COMM_7;
        BSubwayComm_SetCommand7Data(bsw, value);
        break;
    default:
        bsw->sendCommand = BSUBWAY_COMM_MAX;
        break;
    }
}

BOOL BSubwayComm_Send(BSubwayScrWork *bsw) {
    u16 command;

    if (GFL_NetErrCheck()) {
        return TRUE;
    }
    command = bsw->sendCommand;
    if (command >= BSUBWAY_COMM_MAX) {
        return TRUE;
    }
    if (GFL_NetSendCommand(NetHandle_GetOwn(), command, sizeof(bsw->sendBuf), bsw->sendBuf) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

void BSubwayComm_SetRecvMode(BSubwayScrWork *bsw, u8 mode) {
    bsw->recvMode = mode;
}

BOOL BSubwayComm_CheckRecv(BSubwayScrWork *bsw, u16 *var) {
    u8 count;

    if (GFL_NetErrCheck()) {
        return TRUE;
    }
    count = 1;
    if (bsw->recvMode != 1) {
        count = 2;
    }
    if (bsw->recvCount == count) {
        bsw->recvCount = 0;
        if (var != NULL) {
            *var = bsw->recvResult;
        }
        return TRUE;
    }
    return FALSE;
}

static void BSubwayComm_RecvMembers(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;
    u16 result = 0;

    bsw->recvCount++;
    if (netId == NetSystem_GetNetId()) {
        return;
    }
    bsw->partnerGender = recv[0];
    bsw->partnerSpecies[0] = recv[1];
    bsw->partnerSpecies[1] = recv[2];
    bsw->unk18 = recv[3];
    bsw->unkC_5 = bsw->partnerGender + 5;
    if (bsw->memberSpecies[0] == bsw->partnerSpecies[0] || bsw->memberSpecies[0] == bsw->partnerSpecies[1]) {
        result += 1;
    }
    if (bsw->memberSpecies[1] == bsw->partnerSpecies[0] || bsw->memberSpecies[1] == bsw->partnerSpecies[1]) {
        result += 2;
    }
    bsw->recvResult = result;
}

static void BSubwayComm_RecvTrainers(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;

    bsw->recvCount++;
    if (netId == NetSystem_GetNetId() || NetSystem_GetNetId() == 0) {
        return;
    }
    MI_CpuCopy8(data, bsw->unk32, 14 * sizeof(u16));
}

static void BSubwayComm_RecvRetire(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;

    bsw->recvCount++;
    if (netId == NetSystem_GetNetId()) {
        return;
    }
    bsw->recvResult = 0;
    if (bsw->unkC_3 || recv[0]) {
        bsw->recvResult = 1;
    }
}

static void BSubwayComm_RecvPlayerInfo(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;

    bsw->recvCount++;
    if (netId == NetSystem_GetNetId()) {
        return;
    }
    MI_CpuCopy8(data, &bsw->partner, sizeof(PlayerInfo));
}

static void BSubwayComm_RecvPlayMode(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;

    bsw->recvCount++;
    if (netId == NetSystem_GetNetId()) {
        return;
    }
    if (bsw->playMode != recv[0]) {
        bsw->recvResult = 0;
    } else {
        bsw->recvResult = 1;
    }
}

static void BSubwayComm_RecvCommand5(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;

    bsw->recvCount++;
    if (netId == NetSystem_GetNetId()) {
        return;
    }
    bsw->recvResult = recv[0];
}

static void BSubwayComm_RecvCommand6(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;

    bsw->recvCount++;
    if (netId == NetSystem_GetNetId()) {
        return;
    }
    bsw->recvResult = recv[0];
}

static void BSubwayComm_RecvCommand7(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;

    bsw->recvCount++;
    if (netId == NetSystem_GetNetId()) {
        return;
    }
    bsw->recvResult = recv[0];
}
