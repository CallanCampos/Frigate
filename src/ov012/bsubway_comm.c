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
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "system/game_data.h"

static void *func_ov012_02161858(void *work);
static int func_ov012_02161884(void *work);
static BOOL func_ov012_02161888(int a, int b);
static void func_ov012_021618d8(BSubwayScrWork *bsw, GameData *gameData);
static void func_ov012_02161930(BSubwayScrWork *bsw);
static void func_ov012_02161948(BSubwayScrWork *bsw, u16 retire);
static void func_ov012_02161960(BSubwayScrWork *bsw);
static void func_ov012_0216196c(BSubwayScrWork *bsw, u16 value);
static void func_ov012_02161978(BSubwayScrWork *bsw, u16 value);
static void func_ov012_02161984(BSubwayScrWork *bsw, u16 value);
static void func_ov012_02161ad8(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov012_02161b4c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov012_02161b80(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov012_02161bbc(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov012_02161be8(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov012_02161c1c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov012_02161c40(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov012_02161c64(int netId, int size, const void *data, void *work, NetHandle *handle);

// clang-format off
static const NetCommand data_ov012_0216d8fc[] = {
    {func_ov012_02161ad8, NULL},
    {func_ov012_02161b4c, NULL},
    {func_ov012_02161b80, NULL},
    {func_ov012_02161bbc, NULL},
    {func_ov012_02161be8, NULL},
    {func_ov012_02161c1c, NULL},
    {func_ov012_02161c40, NULL},
    {func_ov012_02161c64, NULL},
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
    func_ov012_02161858,
    func_ov012_02161884,
    func_ov012_02161888,
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
    0,
    0,
    0x2c,
    {0x2c, 1, 0, 0},
    0,
    0,
};

// clang-format on

void func_ov012_02161844(BSubwayScrWork *bsw) {
    func_020425ec(&data_ov012_0216d93c, NULL, bsw);
}

static void *func_ov012_02161858(void *work) {
    BSubwayScrWork *bsw = work;
    BSubwayBeacon *beacon = &bsw->beacon;

    func_02008b34(GetGameDataPlayerInfo(bsw->gameData), &beacon->playerInfo);
    func_0207c33c(beacon->mac);
    beacon->gameId = 0x3a0b;
    return beacon;
}

static int func_ov012_02161884(void *work) {
    return sizeof(BSubwayBeacon);
}

static BOOL func_ov012_02161888(int a, int b) {
    if (a == b) {
        return TRUE;
    }
    return FALSE;
}

void func_ov012_02161894(BSubwayScrWork *bsw) {
    // The count passed is the end command, not the table's length
    func_02040c20(BSUBWAY_COMM_MEMBERS, data_ov012_0216d8fc, BSUBWAY_COMM_MAX, bsw);
}

void func_ov012_021618ac(BSubwayScrWork *bsw) {
    func_02040c64(BSUBWAY_COMM_MEMBERS);
}

void func_ov012_021618b8(u8 timing) {
    func_02042d04(func_02040440(), timing);
}

BOOL func_ov012_021618c8(u8 timing) {
    return func_02042d0c(func_02040440(), timing);
}

static void func_ov012_021618d8(BSubwayScrWork *bsw, GameData *gameData) {
    PlayerInfo *info = GetGameDataPlayerInfo(gameData);
    PokeParty *party = func_ov033_0217bd60(bsw);
    int i;

    bsw->sendBuf[0] = getTrainerGender(info);
    for (i = 0; i < 2; i++) {
        bsw->sendBuf[1 + i] = PokeParty_GetParam(PokeParty_GetPkm(party, bsw->memberSlots[i]), PKM_PARAM_SPECIES, NULL);
    }
    bsw->sendBuf[3] = func_ov033_0217be1c(func_ov033_0217bd84(bsw));
}

static void func_ov012_02161930(BSubwayScrWork *bsw) {
    sys_memcpy(bsw->unk32, bsw->sendBuf, 14 * sizeof(u16));
}

static void func_ov012_02161948(BSubwayScrWork *bsw, u16 retire) {
    bsw->unkC_3 = retire;
    bsw->sendBuf[0] = retire;
}

static void func_ov012_02161960(BSubwayScrWork *bsw) {
    *(u32 *)bsw->sendBuf = bsw->playMode;
}

static void func_ov012_0216196c(BSubwayScrWork *bsw, u16 value) {
    *(u32 *)bsw->sendBuf = value;
}

static void func_ov012_02161978(BSubwayScrWork *bsw, u16 value) {
    *(u32 *)bsw->sendBuf = value;
}

static void func_ov012_02161984(BSubwayScrWork *bsw, u16 value) {
    *(u32 *)bsw->sendBuf = value;
}

void func_ov012_02161990(BSubwayScrWork *bsw, u16 mode, u16 value) {
    GameData *gameData = bsw->gameData;

    switch (mode) {
    case 0:
        bsw->sendCommand = BSUBWAY_COMM_MEMBERS;
        func_ov012_021618d8(bsw, gameData);
        break;
    case 1:
        bsw->sendCommand = BSUBWAY_COMM_TRAINERS;
        func_ov012_02161930(bsw);
        break;
    case 2:
        bsw->sendCommand = BSUBWAY_COMM_RETIRE;
        func_ov012_02161948(bsw, value);
        break;
    case 3:
        bsw->sendCommand = BSUBWAY_COMM_PLAYER_INFO;
        func_02008b34(GetGameDataPlayerInfo(gameData), (PlayerInfo *)bsw->sendBuf);
        break;
    case 4:
        bsw->sendCommand = BSUBWAY_COMM_PLAY_MODE;
        func_ov012_02161960(bsw);
        break;
    case 5:
        bsw->sendCommand = BSUBWAY_COMM_5;
        func_ov012_0216196c(bsw, value);
        break;
    case 6:
        bsw->sendCommand = BSUBWAY_COMM_6;
        func_ov012_02161978(bsw, value);
        break;
    case 7:
        bsw->sendCommand = BSUBWAY_COMM_7;
        func_ov012_02161984(bsw, value);
        break;
    default:
        bsw->sendCommand = BSUBWAY_COMM_MAX;
        break;
    }
}

BOOL func_ov012_02161a48(BSubwayScrWork *bsw) {
    u16 command;

    if (GFL_NetErrCheck()) {
        return TRUE;
    }
    command = bsw->sendCommand;
    if (command >= BSUBWAY_COMM_MAX) {
        return TRUE;
    }
    if (func_02042be8(func_02040440(), command, sizeof(bsw->sendBuf), bsw->sendBuf) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

void func_ov012_02161a88(BSubwayScrWork *bsw, u8 mode) {
    bsw->recvMode = mode;
}

BOOL func_ov012_02161a94(BSubwayScrWork *bsw, u16 *var) {
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

static void func_ov012_02161ad8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;
    u16 result = 0;

    bsw->recvCount++;
    if (netId == func_0203ffc4()) {
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

static void func_ov012_02161b4c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;

    bsw->recvCount++;
    if (netId == func_0203ffc4() || func_0203ffc4() == 0) {
        return;
    }
    sys_memcpy(data, bsw->unk32, 14 * sizeof(u16));
}

static void func_ov012_02161b80(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;

    bsw->recvCount++;
    if (netId == func_0203ffc4()) {
        return;
    }
    bsw->recvResult = 0;
    if (bsw->unkC_3 || recv[0]) {
        bsw->recvResult = 1;
    }
}

static void func_ov012_02161bbc(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;

    bsw->recvCount++;
    if (netId == func_0203ffc4()) {
        return;
    }
    sys_memcpy(data, &bsw->partner, sizeof(PlayerInfo));
}

static void func_ov012_02161be8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;

    bsw->recvCount++;
    if (netId == func_0203ffc4()) {
        return;
    }
    if (bsw->playMode != recv[0]) {
        bsw->recvResult = 0;
    } else {
        bsw->recvResult = 1;
    }
}

static void func_ov012_02161c1c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;

    bsw->recvCount++;
    if (netId == func_0203ffc4()) {
        return;
    }
    bsw->recvResult = recv[0];
}

static void func_ov012_02161c40(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;

    bsw->recvCount++;
    if (netId == func_0203ffc4()) {
        return;
    }
    bsw->recvResult = recv[0];
}

static void func_ov012_02161c64(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BSubwayScrWork *bsw = work;
    const u16 *recv = data;

    bsw->recvCount++;
    if (netId == func_0203ffc4()) {
        return;
    }
    bsw->recvResult = recv[0];
}
