#include "types.h"
#include "field/delivery_beacon.h"
#include "field/game_beacon_search.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_system.h"
#include "gfl/net_whpipe.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "nitro/os.h"
#include "save/player_info.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The beacon this game sends, and reads from others
typedef struct {
    u8 type;
    u8 connectNum : 4;
    u8 connectMax : 4;
    u8 unk2_0 : 1;
    u8 unk2_1 : 5;
    u8 unk2_6 : 1;
    u8 isDSi : 1;
    u8 unk3_0 : 1;
    u8 unk3_1 : 1;
    u8 lidOpen : 1;
    u8 unk3_3 : 5;
    u32 trainerId;
    GameBeacon data;
} BeaconSearchBeacon;

// A beacon that was found
typedef struct {
    u8 type;
    u8 connectNum;
    u8 connectMax;
    u8 mac[6];
} BeaconSearchFound;

struct BeaconSearchWork {
    GameData *gameData;
    GameCommSys *commSys;
    // 0 once the network ended, 1 once it started, 2 once the scan is set up
    int state;
    u8 unkC;
    u16 unkE;
    // While nonzero, beacons of type 0x35 are not taken
    u32 timer;
    BeaconSearchBeacon beacon;
    BeaconSearchFound found;
    WihDwcWork *unk88;
};

static void GameBeaconSearch_OnNetInit(void *work);
static void GameBeaconSearch_OnNetExit(void *work);
static BeaconSearchFound *GameBeaconSearch_ScanBeacons(BeaconSearchWork *work, int *index);
static BeaconSearchBeacon *GameBeaconSearch_FindJoinableBeacon(BeaconSearchWork *work, int *index);
static void *GameBeaconSearch_GetSendBeacon(void *work);
static void GameBeaconSearch_SetupBeacon(BeaconSearchBeacon *beacon, GameData *gameData, GameCommSys *commSys);
static int GameBeaconSearch_GetBeaconSize(void *work);
static BOOL GameBeaconSearch_AcceptAnyBeacon(u32 a, u32 b);
static void func_ov012_0215f918(NetHandle *handle, int a1, void *work);
static void func_ov012_0215f920(void *work);
static BeaconSearchBeacon *GameBeaconSearch_PickBetterBeacon(BeaconSearchBeacon *beacon, BeaconSearchBeacon *other);

// clang-format off
static const GFLNetInitData data_ov012_0216d6a8 = {
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    GameBeaconSearch_GetSendBeacon,
    GameBeaconSearch_GetBeaconSize,
    GameBeaconSearch_AcceptAnyBeacon,
    func_ov012_0215f918,
    NULL,
    func_ov012_0215f920,
    NULL,
    {0},
    NULL,
    NULL,
    0,
    {1, 0, 0, 0, 0x80, 0x13, 0, 0},
    HEAPID_TAIL(HEAPID_USER),
    HEAPID_TAIL(0xd),
    HEAPID_TAIL(0xf),
    HEAPID_TAIL(0xd),
    0xf0,
    0,
    4,
    0x30,
    4,
    1,
    0,
    GFL_NET_TYPE_WIRELESS,
    0,
    3,
    {0x2c, 1, 0, 0},
    0,
    0,
};

// clang-format on

void *GameBeaconSearch_Init(u32 *seq, void *param) {
    GameSystem *gsys = param;
    GameData *gameData = GSYS_GetGameData(gsys);
    GameCommSys *commSys = GSYS_GetGameCommSystem(gsys);
    BeaconSearchWork *work =
        GFL_HeapAllocate(HEAPID_TRIAL_HOUSE, sizeof(BeaconSearchWork), TRUE, "game_beacon_search.c", 157);

    work->gameData = gameData;
    work->commSys = commSys;
    work->unkE = 0x267;
    if (GameCommSys_GetLastCommNo(commSys) == GAME_COMM_NO_INTRUDE && func_0202be08(commSys) == 5) {
        GameBeaconSearch_StartType35Wait(work);
    }
    return work;
}

BOOL GameBeaconSearch_InitWait(u32 *seq, void *param, void *work) {
    GameSystem *gsys = param;
    BeaconSearchWork *search = work;

    switch (*seq) {
    case 0:
        GFL_HeapDumpOnFailure(HEAPID_DLP);
        GFL_NetInit(&data_ov012_0216d6a8, GameBeaconSearch_OnNetInit, search);
        (*seq)++;
        break;
    case 1:
        if (search->state >= 1) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void GameBeaconSearch_OnNetInit(void *work) {
    BeaconSearchWork *search = work;

    search->state = 1;
    search->unk88 = WihDwc_Create(4, HEAPID_TAIL(HEAPID_TRIAL_HOUSE));
}

BOOL GameBeaconSearch_Exit(u32 *seq, void *param, void *work) {
    GameSystem *gsys = param;
    BeaconSearchWork *search = work;

    WihDwc_Destroy(search->unk88);
    GFL_NetExit(GameBeaconSearch_OnNetExit);
    return TRUE;
}

BOOL GameBeaconSearch_ExitWait(u32 *seq, void *param, void *work) {
    GameSystem *gsys = param;
    BeaconSearchWork *search = work;

    if (search->state == 0) {
        GFL_HeapFree(search);
        if (GFL_NetErrCheck()) {
            GFL_NetErrClear();
        }
        return TRUE;
    }
    return FALSE;
}

static void GameBeaconSearch_OnNetExit(void *work) {
    BeaconSearchWork *search = work;

    search->state = 0;
}

void GameBeaconSearch_Main(u32 *seq, void *param, void *work) {
    GameSystem *gsys = param;
    BeaconSearchWork *search = work;
    GameCommSys *commSys = GSYS_GetGameCommSystem(gsys);
    GameData *gameData = GSYS_GetGameData(gsys);
    int index = 0;
    BeaconSearchFound *found;

    if (GameData_CheckEventsPaused(gameData) == TRUE) {
        return;
    }
    if (GFL_NetErrCheck()) {
        GameCommSys_ExitReq(commSys);
        return;
    }
    if (GCTX_HIDIsLidClosed() == TRUE && GetIsShouldUICallback() == TRUE) {
        GameCommSys_ExitReq(commSys);
        return;
    }
    if (search->timer != 0) {
        search->timer--;
    }
    GameBeaconSys_SendIfUpdated();
    found = GameBeaconSearch_ScanBeacons(search, &index);
    if (found != NULL) {
        found->type = 0;
    }
    WihDwc_TickTimers();
}

static BeaconSearchFound *GameBeaconSearch_ScanBeacons(BeaconSearchWork *work, int *index) {
    BeaconSearchBeacon *beacon;
    const u8 *mac;
    int i;
    BeaconSearchFound *found = &work->found;

    if (work->state == 0) {
        return NULL;
    }
    if (work->state == 1) {
        GFL_NetStartSearch(0);
        work->state = 2;
        return NULL;
    }
    if (found->type == 0) {
        beacon = GameBeaconSearch_FindJoinableBeacon(work, index);
        if (beacon != NULL) {
            if (beacon->unk2_0) {
                NetWhpipe_ConsumeBeacon(*index);
            } else if (beacon->unk2_1 == 1) {
                MI_CpuFill8(found, 0, sizeof(BeaconSearchFound));
                found->type = beacon->type;
                found->connectNum = beacon->connectNum;
                found->connectMax = beacon->connectMax;
                mac = GFL_NetGetBeaconMac(*index);
                for (i = 0; i < 6; i++) {
                    found->mac[i] = mac[i];
                }
            } else if (beacon->unk2_1 == 2) {
                GameBeaconSys_Receive(&beacon->data);
                NetWhpipe_ConsumeBeacon(*index);
            }
        }
    }
    if (found->type == 0) {
        found = NULL;
    }
    return found;
}

// The beacon of the game to connect to: the one with the most players that has room
static BeaconSearchBeacon *GameBeaconSearch_FindJoinableBeacon(BeaconSearchWork *work, int *index) {
    int best = -1;
    int i;
    u8 type;
    BeaconSearchBeacon *beacon;

    for (i = 0; i < 16; i++) {
        type = NetWhpipe_GetBeaconCommandBase(i);
        if (type != 0x35 && type != 3) {
            continue;
        }
        if (type == 0x35 && work->timer != 0) {
            continue;
        }
        beacon = GFL_NetGetBeaconData(i);
        if (beacon == NULL) {
            continue;
        }
        if (beacon->connectNum < beacon->connectMax) {
            if (best == -1) {
                best = i;
            } else if (GameBeaconSearch_PickBetterBeacon(beacon, GFL_NetGetBeaconData(best)) == beacon) {
                best = i;
            }
        } else {
            NetWhpipe_ConsumeBeacon(i);
        }
    }
    if (best != -1) {
        *index = best;
        return GFL_NetGetBeaconData(best);
    }
    return NULL;
}

static void *GameBeaconSearch_GetSendBeacon(void *work) {
    BeaconSearchWork *search = work;

    GameBeaconSearch_SetupBeacon(&search->beacon, search->gameData, search->commSys);
    GameBeaconSys_GetSendBeacon(&search->beacon.data);
    search->beacon.unk2_1 = 2;
    return &search->beacon;
}

static void GameBeaconSearch_SetupBeacon(BeaconSearchBeacon *beacon, GameData *gameData, GameCommSys *commSys) {
    BOOL lidOpen = FALSE;
    BOOL flag;

    MI_CpuFill8(beacon, 0, sizeof(BeaconSearchBeacon));
    beacon->type = 3;
    beacon->connectNum = GFL_NetGetNegotiatedCount();
    beacon->connectMax = 3;
    beacon->unk2_0 = NetSystem_HasErrorChecked();
    beacon->unk2_1 = 0;
    beacon->unk3_0 = GameCommSys_IsInResearchGraphOrMission(commSys);
    beacon->trainerId = getIDAsUInt(GetGameDataPlayerInfo(gameData));
    beacon->isDSi = OS_IsRunOnTwl();
    beacon->unk2_6 = FALSE;
    beacon->unk3_1 = TRUE;
    if (PAD_DetectFold() == FALSE) {
        lidOpen = TRUE;
    }
    beacon->lidOpen = lidOpen;
    if (beacon->isDSi == TRUE) {
        if (OS_GetParentalControlInfoPtr()->unk0_0 && OS_GetParentalControlInfoPtr()->unk0_5) {
            flag = TRUE;
        } else {
            flag = FALSE;
        }
        beacon->unk2_6 = flag;
    }
}

static int GameBeaconSearch_GetBeaconSize(void *work) {
    return sizeof(BeaconSearchBeacon);
}

static BOOL GameBeaconSearch_AcceptAnyBeacon(u32 a, u32 b) {
    return TRUE;
}

static void func_ov012_0215f918(NetHandle *handle, int a1, void *work) {
    BeaconSearchWork *search = work;

    search->unkC = TRUE;
}

static void func_ov012_0215f920(void *work) {
}

static BeaconSearchBeacon *GameBeaconSearch_PickBetterBeacon(BeaconSearchBeacon *beacon, BeaconSearchBeacon *other) {
    if (beacon->unk2_0) {
        return other;
    }
    if (beacon->connectNum < beacon->connectMax && beacon->connectNum >= other->connectNum) {
        return beacon;
    }
    return other;
}

void GameBeaconSearch_StartType35Wait(BeaconSearchWork *search) {
    search->timer = 1800;
}
