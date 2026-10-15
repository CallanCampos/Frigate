#include "types.h"
#include "app/compatible_irc_sys.h"
#include "app/error_window_screen.h"
#include "constants/pokemon.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_system.h"
#include "gfl/std.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "system/game_data.h"
#include "system/game_system.h"

// compatible_irc_sys.c, a ROM name: the infrared link under the compatibility checker's screens. It sets up the
// network, negotiates with the other player, and exchanges the numbers and the profiles that the checker compares. The
// names are descriptive guesses

// The commands of the link: the other player's id, a reply to it, and the count. They go after the base 0xd00
#define CMD_ID 0xd00
#define CMD_ID_REPLY 0xd01
#define CMD_COUNT 0xd02
// And those of the profile exchange, after the base 0x1000
#define CMD_SELECTION 0x1000
#define CMD_PING 0x1001
#define CMD_PROFILE 0x1003

// What is exchanged besides the profiles: a selection of two numbers, and a ping
typedef struct {
    CompatibleIrcSys *sys;
    int state;
    u8 recvSelection[8];
    u32 sentSelection[2];
    BOOL selectionReceived;
    BOOL pingReceived;
    BOOL unk20;
    BOOL profileReceived;
    CompatibleIrcProfile *ownProfile;
    CompatibleIrcProfile *recvProfileOwn;
    CompatibleIrcProfile *recvProfilePeer;
} CompatibleIrcSync;

struct CompatibleIrcSys {
    GameData *gameData;
    CompatibleIrcSync sync;
    int sentCount;
    int receivedCount;
    int unk40;
    int state;
    u8 unk48[4];
    u32 negotiatedBits;
    BOOL connected;
    BOOL netEnded;
    u8 unk58[0x10];
    u32 ids[2];
    BOOL idsReplied;
    BOOL syncStarted;
    BOOL disabled;
    u8 unk7C[6];
    u16 timer;
    BOOL negotiating;
    u16 heapId;
    u8 unk8A[2];
};

static BOOL CompatibleIrcSys_Send(CompatibleIrcSys *sys, int command, u16 size, const void *data);
static void CompatibleIrcSys_OnNegotiated(void *work, int netId);
static void *CompatibleIrcSys_GetInfo(void *work);
static int CompatibleIrcSys_GetInfoSize(void *work);
static BOOL CompatibleIrcSys_IsSameGame(u32 base, u32 beaconBase);
static void CompatibleIrcSys_OnDisconnect(void *work);
static void CompatibleIrcSys_NetInitDone(void *work);
static void CompatibleIrcSys_NetExitDone(void *work);
static void CompatibleIrcSys_RecvId(int netId, int size, const void *data, void *work, NetHandle *handle);
static void CompatibleIrcSys_RecvIdReply(int netId, int size, const void *data, void *work, NetHandle *handle);
static void CompatibleIrcSys_RecvCount(int netId, int size, const void *data, void *work, NetHandle *handle);
static void CompatibleIrcSync_Init(CompatibleIrcSync *sync, CompatibleIrcSys *sys, HeapID heapId);
static void CompatibleIrcSync_Exit(CompatibleIrcSync *sync);
static BOOL CompatibleIrcSync_SendSelection(CompatibleIrcSync *sync, u32 a, u32 b);
static BOOL CompatibleIrcSync_PollSelection(CompatibleIrcSync *sync);
static BOOL CompatibleIrcSync_SendPing(CompatibleIrcSync *sync);
static BOOL CompatibleIrcSync_PollPing(CompatibleIrcSync *sync);
static BOOL CompatibleIrcSync_ExchangeProfiles(CompatibleIrcSync *sync, GameSystem *gsys);
static void CompatibleIrcSync_GetPeerProfile(CompatibleIrcSync *sync, CompatibleIrcProfile *dest);
static void CompatibleIrcSync_RecvSelection(int netId, int size, const void *data, void *work, NetHandle *handle);
static void CompatibleIrcSync_RecvPing(int netId, int size, const void *data, void *work, NetHandle *handle);
static void CompatibleIrcSync_RecvUnk20(int netId, int size, const void *data, void *work, NetHandle *handle);
static void CompatibleIrcSync_RecvProfile(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *CompatibleIrcSync_GetProfileBuffer(int netId, void *work, int size);

// The commands of the link, and then of the exchanges
static const NetCommand sCompatibleIrcSysCommands[] = {
    { CompatibleIrcSys_RecvId, NULL },
    { CompatibleIrcSys_RecvIdReply, NULL },
    { CompatibleIrcSys_RecvCount, NULL },
};

static const NetCommand sCompatibleIrcSyncCommands[] = {
    { CompatibleIrcSync_RecvSelection, NULL },
    { CompatibleIrcSync_RecvPing, NULL },
    { CompatibleIrcSync_RecvUnk20, NULL },
    { CompatibleIrcSync_RecvProfile, CompatibleIrcSync_GetProfileBuffer },
};

static const GFLNetInitData sCompatibleIrcSysNetInit = {
    sCompatibleIrcSysCommands,
    NELEMS(sCompatibleIrcSysCommands),
    NULL,
    CompatibleIrcSys_OnNegotiated,
    NULL,
    NULL,
    CompatibleIrcSys_GetInfo,
    CompatibleIrcSys_GetInfoSize,
    CompatibleIrcSys_IsSameGame,
    NULL,
    (void (*)(NetHandle *, int, void *))ErrorWindowScreen_Show,
    CompatibleIrcSys_OnDisconnect,
    NULL,
    { 0 },
    NULL,
    NULL,
    0,
    { 1, 0, 0, 0, 0x32, 0x05, 0, 0 },
    HEAPID_USER,
    0xd,
    0xf,
    0x10,
    0xf0,
    0,
    2,
    0x20,
    0x10,
    1,
    0,
    3,
    1,
    0xd,
    { 0x2c, 1, 0, 0 },
    0,
    0,
};

// What the other player's beacon says of this game
static u32 sCompatibleIrcSysInfo = 13;

CompatibleIrcSys *CompatibleIrcSys_Create(int unk40, GameData *gameData, HeapID heapId, BOOL disabled) {
    CompatibleIrcSys *sys = GFL_HeapAllocate(heapId, sizeof(CompatibleIrcSys), FALSE, "compatible_irc_sys.c", 275);

    sys_memset(sys, 0, sizeof(CompatibleIrcSys));
    sys->unk40 = unk40;
    sys->heapId = heapId;
    sys->syncStarted = FALSE;
    sys->disabled = disabled;
    sys->gameData = gameData;
    return sys;
}

void CompatibleIrcSys_Free(CompatibleIrcSys *sys) {
    GFL_HeapFree(sys);
}

BOOL CompatibleIrcSys_StartNet(CompatibleIrcSys *sys) {
    GFLNetInitData init;

    if (sys->disabled) {
        return TRUE;
    }
    switch (sys->state) {
    case 0:
        // The first branch's state is overwritten by the state 1 below, as in the original
        if (func_02042788()) {
            sys->state = 2;
        } else {
            init = sCompatibleIrcSysNetInit;
            init.unk68.word = sys->unk40;
            func_020425ec(&init, CompatibleIrcSys_NetInitDone, sys);
        }
        sys->state = 1;
        break;
    case 1:
        if (func_02042788()) {
            sys->state = 2;
        }
        break;
    case 2:
        CompatibleIrcSync_Init(&sys->sync, sys, sys->heapId);
        sys->syncStarted = TRUE;
        sys->state = 3;
        break;
    case 3:
        sys->state = 0;
        return TRUE;
    }
    return FALSE;
}

BOOL CompatibleIrcSys_EndNet(CompatibleIrcSys *sys) {
    if (sys->disabled) {
        return TRUE;
    }
    switch (sys->state) {
    case 0:
        if (sys->syncStarted) {
            CompatibleIrcSync_Exit(&sys->sync);
            sys->syncStarted = FALSE;
        }
        sys->netEnded = FALSE;
        sys->state = 1;
        break;
    case 1:
        if (func_02042860(CompatibleIrcSys_NetExitDone)) {
            sys->state = 2;
        } else {
            sys->state = 3;
        }
        break;
    case 2:
        if (sys->netEnded) {
            sys->state = 3;
        }
        break;
    case 3:
        sys->state = 0;
        return TRUE;
    }
    return FALSE;
}

BOOL CompatibleIrcSys_StartNegotiation(CompatibleIrcSys *sys) {
    if (sys->disabled) {
        return TRUE;
    }
    switch (sys->state) {
    case 0:
        func_020429f8(NULL);
        sys->state = 1;
        sys->negotiating = TRUE;
        sys->connected = FALSE;
        sys->negotiatedBits = 0;
        break;
    case 1:
        if (sys->connected) {
            sys->negotiating = FALSE;
            sys->state = 2;
        }
        break;
    case 2:
        sys->state = 0;
        return TRUE;
    }
    return FALSE;
}

BOOL CompatibleIrcSys_Sync(CompatibleIrcSys *sys) {
    if (sys->disabled) {
        return TRUE;
    }
    switch (sys->state) {
    case 0:
        func_02040624(func_02040440(), 0xe, 0xd);
        sys->state = 1;
        break;
    case 1:
        if (func_02040664(func_02040440(), 0xe, 0xd)) {
            sys->state = 2;
        }
        break;
    case 2:
        sys->state = 0;
        return TRUE;
    }
    return FALSE;
}

BOOL CompatibleIrcSys_IsConnected(CompatibleIrcSys *sys) {
    if (sys->disabled) {
        return TRUE;
    }
    return sys->connected;
}

BOOL CompatibleIrcSys_ExchangeIds(CompatibleIrcSys *sys, u32 value) {
    int i;

    if (sys->disabled) {
        return TRUE;
    }
    switch (sys->state) {
    case 0:
        sys->idsReplied = FALSE;
        if (CompatibleIrcSys_Send(sys, CMD_ID, 4, &value)) {
            sys->state = 1;
        }
        break;
    case 1:
        if (func_02042bc4()) {
            for (i = 0; i < 2; i++) {
                if (value != sys->ids[i]) {
                    break;
                }
            }
            if (i == 2) {
                sys->state = 2;
            }
        } else {
            sys->state = 3;
        }
        break;
    case 2:
        if (CompatibleIrcSys_Send(sys, CMD_ID_REPLY, 4, &value)) {
            sys->state = 4;
        }
        break;
    case 3:
        if (sys->idsReplied) {
            sys->state = 4;
        }
        break;
    case 4:
        sys->state = 0;
        return TRUE;
    }
    return FALSE;
}

void CompatibleIrcSys_ResetState(CompatibleIrcSys *sys) {
    sys->state = 0;
}

void CompatibleIrcSys_ResetCounts(CompatibleIrcSys *sys) {
    sys->receivedCount = 0;
    sys->sentCount = 0;
}

void CompatibleIrcSys_SetCount(CompatibleIrcSys *sys, int count) {
    sys->sentCount = count;
}

BOOL CompatibleIrcSys_Update(CompatibleIrcSys *sys) {
    if (sys->disabled) {
        return TRUE;
    }
    if (sys->timer++ > 180) {
        sys->timer = 0;
        if (!func_020400b8(CMD_COUNT, func_02042a6c(func_02040440()))) {
            return CompatibleIrcSys_SendCount(sys);
        }
    }
    return FALSE;
}

BOOL CompatibleIrcSys_SendCount(CompatibleIrcSys *sys) {
    int dest;
    u8 sendId;

    if (sys->disabled) {
        return TRUE;
    }
    dest = func_02042a6c(func_02040440());
    if (dest == 0) {
        sendId = TRUE;
    } else {
        sendId = FALSE;
    }
    if (!func_020400b8(CMD_COUNT, dest)) {
        return CompatibleIrcSys_SendEx(sys, CMD_COUNT, 4, &sys->sentCount, sendId, 0, 0, TRUE);
    }
    return FALSE;
}

int CompatibleIrcSys_GetPendingCount(CompatibleIrcSys *sys) {
    if (sys->disabled) {
        return 0;
    }
    return sys->sentCount - sys->receivedCount;
}

static BOOL CompatibleIrcSys_Send(CompatibleIrcSys *sys, int command, u16 size, const void *data) {
    if (sys->disabled) {
        return TRUE;
    }
    return func_02042be8(func_02040440(), command, size, data);
}

BOOL CompatibleIrcSys_SendEx(CompatibleIrcSys *sys, u16 command, u32 size, const void *data, u8 sendId, u32 a5, BOOL a6,
                             BOOL noCopy) {
    if (sys->disabled) {
        return TRUE;
    }
    return func_02042c18(func_02040440(), sendId, command, size, data, a5, a6, noCopy);
}

void CompatibleIrcSys_AddCommands(CompatibleIrcSys *sys, int base, const void *list, int count, void *work) {
    if (!sys->disabled) {
        func_02040c20(base, list, count, work);
    }
}

void CompatibleIrcSys_RemoveCommands(CompatibleIrcSys *sys, int base) {
    if (!sys->disabled) {
        func_02040c64(base);
    }
}

BOOL CompatibleIrcSys_SendSelection(CompatibleIrcSys *sys, u32 a, u32 b) {
    if (sys->disabled) {
        return TRUE;
    }
    return CompatibleIrcSync_SendSelection(&sys->sync, a, b);
}

BOOL CompatibleIrcSys_PollSelection(CompatibleIrcSys *sys) {
    if (sys->disabled) {
        return TRUE;
    }
    return CompatibleIrcSync_PollSelection(&sys->sync);
}

BOOL CompatibleIrcSys_SendPing(CompatibleIrcSys *sys) {
    if (sys->disabled) {
        return TRUE;
    }
    return CompatibleIrcSync_SendPing(&sys->sync);
}

BOOL CompatibleIrcSys_PollPing(CompatibleIrcSys *sys) {
    if (sys->disabled) {
        return TRUE;
    }
    return CompatibleIrcSync_PollPing(&sys->sync);
}

BOOL CompatibleIrcSys_ExchangeProfiles(CompatibleIrcSys *sys, GameSystem *gsys) {
    if (sys->disabled) {
        return TRUE;
    }
    return CompatibleIrcSync_ExchangeProfiles(&sys->sync, gsys);
}

void CompatibleIrcSys_GetPeerProfile(CompatibleIrcSys *sys, CompatibleIrcProfile *dest) {
    if (!sys->disabled) {
        CompatibleIrcSync_GetPeerProfile(&sys->sync, dest);
    }
}

void CompatibleIrcSys_BuildProfile(GameSystem *gsys, CompatibleIrcProfile *profile) {
    OSOwnerInfo owner;
    GameData *gameData;
    PlayerInfo *trainer;
    PokeParty *party;
    PartyPkm *pkm;
    RTCDate date;
    BOOL isFourteenth;

    OS_GetOwnerInfo(&owner);
    gameData = GSYS_GetGameData(gsys);
    trainer = GetGameDataPlayerInfo(gameData);
    profile->birthMonth = owner.birthday.month;
    profile->birthDay = owner.birthday.day;
    func_02008b34(trainer, &profile->trainer);
    party = GameData_GetParty(gameData);
    isFourteenth = FALSE;
    pkm = PokeParty_GetPkm(party, 0);
    profile->species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    profile->form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    profile->sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
    profile->isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    RTC_GetCachedDate(&date);
    if (date.day == 14) {
        isFourteenth = TRUE;
    }
    profile->isFourteenth = isFourteenth;
    profile->date = (date.year << 24) | ((date.month & 0xff) << 16) | ((date.day & 0xff) << 8) | date.week;
}

static void CompatibleIrcSys_OnNegotiated(void *work, int netId) {
    CompatibleIrcSys *sys = work;

    sys->negotiatedBits |= 1 << netId;
    if (MATH_CountPopulation(sys->negotiatedBits) >= 2) {
        sys->connected = TRUE;
    }
}

static void *CompatibleIrcSys_GetInfo(void *work) {
    return &sCompatibleIrcSysInfo;
}

static int CompatibleIrcSys_GetInfoSize(void *work) {
    return 4;
}

static BOOL CompatibleIrcSys_IsSameGame(u32 base, u32 beaconBase) {
    if (base == beaconBase) {
        return TRUE;
    }
    return FALSE;
}

static void CompatibleIrcSys_OnDisconnect(void *work) {
    CompatibleIrcSys *sys = work;

    sys->connected = FALSE;
}

static void CompatibleIrcSys_NetInitDone(void *work) {
}

static void CompatibleIrcSys_NetExitDone(void *work) {
    CompatibleIrcSys *sys = work;

    sys->netEnded = TRUE;
}

static void CompatibleIrcSys_RecvId(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CompatibleIrcSys *sys = work;
    u32 id;

    if (func_02042bc4()) {
        sys_memcpy(data, &id, 4);
        sys->ids[netId] = id;
    }
}

static void CompatibleIrcSys_RecvIdReply(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CompatibleIrcSys *sys = work;

    if (handle == func_02040440()) {
        sys->idsReplied = TRUE;
    }
}

static void CompatibleIrcSys_RecvCount(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CompatibleIrcSys *sys = work;

    if (handle == func_02040440()) {
        if (netId != func_0203ffc4()) {
            sys_memcpy(data, &sys->receivedCount, 4);
        }
    }
}

static void CompatibleIrcSync_Init(CompatibleIrcSync *sync, CompatibleIrcSys *sys, HeapID heapId) {
    sys_memset(sync, 0, sizeof(CompatibleIrcSync));
    sync->sys = sys;
    sync->ownProfile = GFL_HeapAllocate(heapId, sizeof(CompatibleIrcProfile), FALSE, "compatible_irc_sys.c", 1403);
    sys_memset(sync->ownProfile, 0, sizeof(CompatibleIrcProfile));
    sync->recvProfileOwn = GFL_HeapAllocate(heapId, sizeof(CompatibleIrcProfile), FALSE, "compatible_irc_sys.c", 1405);
    sys_memset(sync->recvProfileOwn, 0, sizeof(CompatibleIrcProfile));
    sync->recvProfilePeer = GFL_HeapAllocate(heapId, sizeof(CompatibleIrcProfile), FALSE, "compatible_irc_sys.c", 1407);
    sys_memset(sync->recvProfilePeer, 0, sizeof(CompatibleIrcProfile));
    CompatibleIrcSys_AddCommands(sys, 0x1000, sCompatibleIrcSyncCommands, NELEMS(sCompatibleIrcSyncCommands), sync);
}

static void CompatibleIrcSync_Exit(CompatibleIrcSync *sync) {
    CompatibleIrcSys_RemoveCommands(sync->sys, 0x1000);
    GFL_HeapFree(sync->ownProfile);
    GFL_HeapFree(sync->recvProfileOwn);
    GFL_HeapFree(sync->recvProfilePeer);
    sys_memset(sync, 0, sizeof(CompatibleIrcSync));
}

static BOOL CompatibleIrcSync_SendSelection(CompatibleIrcSync *sync, u32 a, u32 b) {
    sync->sentSelection[1] = a;
    sync->sentSelection[0] = b;
    if (CompatibleIrcSys_Send(sync->sys, CMD_SELECTION, 8, sync->sentSelection)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL CompatibleIrcSync_PollSelection(CompatibleIrcSync *sync) {
    if (sync->selectionReceived == TRUE) {
        sync->selectionReceived = FALSE;
        return TRUE;
    }
    return FALSE;
}

static BOOL CompatibleIrcSync_SendPing(CompatibleIrcSync *sync) {
    // The ping carries no data, only its arrival counts
    u32 ping;

    if (CompatibleIrcSys_Send(sync->sys, CMD_PING, 4, &ping)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL CompatibleIrcSync_PollPing(CompatibleIrcSync *sync) {
    if (sync->pingReceived == TRUE) {
        sync->pingReceived = FALSE;
        return TRUE;
    }
    return FALSE;
}

static BOOL CompatibleIrcSync_ExchangeProfiles(CompatibleIrcSync *sync, GameSystem *gsys) {
    switch (sync->state) {
    case 0:
        if (gsys != NULL) {
            CompatibleIrcSys_BuildProfile(gsys, sync->ownProfile);
        } else {
            return TRUE;
        }
        if (CompatibleIrcSys_SendEx(sync->sys, CMD_PROFILE, sizeof(CompatibleIrcProfile), sync->ownProfile, 0xff, 0, 0,
                                    TRUE)) {
            sync->state = 1;
        }
        break;
    case 1:
        if (sync->profileReceived) {
            sync->profileReceived = FALSE;
            sync->state = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void CompatibleIrcSync_GetPeerProfile(CompatibleIrcSync *sync, CompatibleIrcProfile *dest) {
    *dest = *sync->recvProfilePeer;
}

static void CompatibleIrcSync_RecvSelection(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CompatibleIrcSync *sync = work;

    if (handle == func_02040440()) {
        if (netId != func_0203ffc4()) {
            sys_memcpy(data, sync->recvSelection, 8);
            sync->selectionReceived = TRUE;
        }
    }
}

static void CompatibleIrcSync_RecvPing(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CompatibleIrcSync *sync = work;

    if (handle == func_02040440()) {
        if (netId != func_0203ffc4()) {
            sync->pingReceived = TRUE;
        }
    }
}

static void CompatibleIrcSync_RecvUnk20(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CompatibleIrcSync *sync = work;

    if (handle == func_02040440()) {
        if (netId != func_0203ffc4()) {
            sync->unk20 = TRUE;
        }
    }
}

static void CompatibleIrcSync_RecvProfile(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CompatibleIrcSync *sync = work;
    const CompatibleIrcProfile *profile = data;

    if (sync->sys->gameData != NULL) {
        PlayerInfo *info = func_02017378(sync->sys->gameData, netId);

        sys_memcpy(&profile->trainer, info, PlayerInfo_GetSize());
    }
    if (handle == func_02040440()) {
        if (netId != func_0203ffc4()) {
            sync->profileReceived = TRUE;
        }
    }
}

static void *CompatibleIrcSync_GetProfileBuffer(int netId, void *work, int size) {
    CompatibleIrcSync *sync = work;

    if (netId == func_0203ffc4()) {
        return sync->recvProfileOwn;
    }
    return sync->recvProfilePeer;
}
