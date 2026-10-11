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

typedef struct {
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
    void *unk88;
} BeaconSearchWork;

static void func_ov012_0215f5f4(void *work);
static void func_ov012_0215f64c(void *work);
static BeaconSearchFound *func_ov012_0215f6c8(BeaconSearchWork *work, int *index);
static BeaconSearchBeacon *func_ov012_0215f760(BeaconSearchWork *work, int *index);
static void *func_ov012_0215f7e8(void *work);
static void func_ov012_0215f810(BeaconSearchBeacon *beacon, GameData *gameData, GameCommSys *commSys);
static int func_ov012_0215f910(void *work);
static BOOL func_ov012_0215f914(u32 a, u32 b);
static void func_ov012_0215f918(NetHandle *handle, int a1, void *work);
static void func_ov012_0215f920(void *work);
static BeaconSearchBeacon *func_ov012_0215f924(BeaconSearchBeacon *beacon, BeaconSearchBeacon *other);

static const GFLNetInitData data_ov012_0216d6a8 = {
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    func_ov012_0215f7e8,
    func_ov012_0215f910,
    func_ov012_0215f914,
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
    0,
    0,
    3,
    {0x2c, 1, 0, 0},
    0,
    0,
};

void *func_ov012_0215f55c(u32 *seq, void *param) {
    GameSystem *gsys = param;
    GameData *gameData = GSYS_GetGameData(gsys);
    GameCommSys *commSys = GSYS_GetGameCommSystem(gsys);
    BeaconSearchWork *work =
        GFL_HeapAllocate(HEAPID_TRIAL_HOUSE, sizeof(BeaconSearchWork), TRUE, "game_beacon_search.c", 157);

    work->gameData = gameData;
    work->commSys = commSys;
    work->unkE = 0x267;
    if (GameCommSys_GetLastCommNo(commSys) == 2 && func_0202be08(commSys) == 5) {
        func_ov012_0215f94c(work);
    }
    return work;
}

BOOL func_ov012_0215f5b4(u32 *seq, void *param, void *work) {
    GameSystem *gsys = param;
    BeaconSearchWork *search = work;

    switch (*seq) {
    case 0:
        GFL_HeapDumpOnFailure(HEAPID_DLP);
        func_020425ec(&data_ov012_0216d6a8, func_ov012_0215f5f4, search);
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

static void func_ov012_0215f5f4(void *work) {
    BeaconSearchWork *search = work;

    search->state = 1;
    search->unk88 = func_02012908(HEAPID_GAMEEVENT, HEAPID_TAIL(HEAPID_TRIAL_HOUSE));
}

BOOL func_ov012_0215f610(u32 *seq, void *param, void *work) {
    GameSystem *gsys = param;
    BeaconSearchWork *search = work;

    func_02012994(search->unk88);
    func_02042860(func_ov012_0215f64c);
    return TRUE;
}

BOOL func_ov012_0215f628(u32 *seq, void *param, void *work) {
    GameSystem *gsys = param;
    BeaconSearchWork *search = work;

    if (search->state == 0) {
        GFL_HeapFree(search);
        if (GFL_NetErrCheck()) {
            func_02012144();
        }
        return TRUE;
    }
    return FALSE;
}

static void func_ov012_0215f64c(void *work) {
    BeaconSearchWork *search = work;

    search->state = 0;
}

void func_ov012_0215f654(u32 *seq, void *param, void *work) {
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
    found = func_ov012_0215f6c8(search, &index);
    if (found != NULL) {
        found->type = 0;
    }
    func_02012a4c();
}

static BeaconSearchFound *func_ov012_0215f6c8(BeaconSearchWork *work, int *index) {
    BeaconSearchBeacon *beacon;
    const u8 *mac;
    int i;
    BeaconSearchFound *found = &work->found;

    if (work->state == 0) {
        return NULL;
    }
    if (work->state == 1) {
        func_02042a10(0);
        work->state = 2;
        return NULL;
    }
    if (found->type == 0) {
        beacon = func_ov012_0215f760(work, index);
        if (beacon != NULL) {
            if (beacon->unk2_0) {
                func_ov030_02173bec(*index);
            } else if (beacon->unk2_1 == 1) {
                sys_memset(found, 0, sizeof(BeaconSearchFound));
                found->type = beacon->type;
                found->connectNum = beacon->connectNum;
                found->connectMax = beacon->connectMax;
                mac = func_020428c8(*index);
                for (i = 0; i < 6; i++) {
                    found->mac[i] = mac[i];
                }
            } else if (beacon->unk2_1 == 2) {
                GameBeaconSys_Receive(&beacon->data);
                func_ov030_02173bec(*index);
            }
        }
    }
    if (found->type == 0) {
        found = NULL;
    }
    return found;
}

// The beacon of the game to connect to: the one with the most players that has room
static BeaconSearchBeacon *func_ov012_0215f760(BeaconSearchWork *work, int *index) {
    int best = -1;
    int i;
    u8 type;
    BeaconSearchBeacon *beacon;

    for (i = 0; i < 16; i++) {
        type = func_ov030_021740a4(i);
        if (type != 0x35 && type != 3) {
            continue;
        }
        if (type == 0x35 && work->timer != 0) {
            continue;
        }
        beacon = func_020428a8(i);
        if (beacon == NULL) {
            continue;
        }
        if (beacon->connectNum < beacon->connectMax) {
            if (best == -1) {
                best = i;
            } else if (func_ov012_0215f924(beacon, func_020428a8(best)) == beacon) {
                best = i;
            }
        } else {
            func_ov030_02173bec(i);
        }
    }
    if (best != -1) {
        *index = best;
        return func_020428a8(best);
    }
    return NULL;
}

static void *func_ov012_0215f7e8(void *work) {
    BeaconSearchWork *search = work;

    func_ov012_0215f810(&search->beacon, search->gameData, search->commSys);
    GameBeaconSys_GetSendBeacon(&search->beacon.data);
    search->beacon.unk2_1 = 2;
    return &search->beacon;
}

static void func_ov012_0215f810(BeaconSearchBeacon *beacon, GameData *gameData, GameCommSys *commSys) {
    BOOL lidOpen = FALSE;
    BOOL flag;

    sys_memset(beacon, 0, sizeof(BeaconSearchBeacon));
    beacon->type = 3;
    beacon->connectNum = func_02042a78();
    beacon->connectMax = 3;
    beacon->unk2_0 = func_02040078();
    beacon->unk2_1 = 0;
    beacon->unk3_0 = func_0202be3c(commSys);
    beacon->trainerId = getIDAsUInt(GetGameDataPlayerInfo(gameData));
    beacon->isDSi = hw_isDSi();
    beacon->unk2_6 = FALSE;
    beacon->unk3_1 = TRUE;
    if (PAD_DetectFold() == FALSE) {
        lidOpen = TRUE;
    }
    beacon->lidOpen = lidOpen;
    if (beacon->isDSi == TRUE) {
        if (func_0207c4b4()->unk0_0 && func_0207c4b4()->unk0_5) {
            flag = TRUE;
        } else {
            flag = FALSE;
        }
        beacon->unk2_6 = flag;
    }
}

static int func_ov012_0215f910(void *work) {
    return sizeof(BeaconSearchBeacon);
}

static BOOL func_ov012_0215f914(u32 a, u32 b) {
    return TRUE;
}

static void func_ov012_0215f918(NetHandle *handle, int a1, void *work) {
    BeaconSearchWork *search = work;

    search->unkC = TRUE;
}

static void func_ov012_0215f920(void *work) {
}

static BeaconSearchBeacon *func_ov012_0215f924(BeaconSearchBeacon *beacon, BeaconSearchBeacon *other) {
    if (beacon->unk2_0) {
        return other;
    }
    if (beacon->connectNum < beacon->connectMax && beacon->connectNum >= other->connectNum) {
        return beacon;
    }
    return other;
}

void func_ov012_0215f94c(void *work) {
    BeaconSearchWork *search = work;

    search->timer = 1800;
}
