#include "types.h"
#include "app/livebattlematch_irc.h"
#include "field/delivery_irc.h"
#include "field/net_devirc.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "system/version.h"

// The commands of the link, the game's command base 0x1f and a number
#define LIVEBATTLEMATCH_IRC_COMMAND_PLAYER_DATA 0x1f00
#define LIVEBATTLEMATCH_IRC_COMMAND_PARTY 0x1f01
#define LIVEBATTLEMATCH_IRC_COMMAND_UNUSED 0x1f02
#define LIVEBATTLEMATCH_IRC_COMMAND_READY 0x1f03
#define LIVEBATTLEMATCH_IRC_COMMAND_NUM 4

#define LIVEBATTLEMATCH_IRC_BUFFER_SIZE 0x1000
// The code and size of the p_delivery's data: the competition's data of a Download Station
#define LIVEBATTLEMATCH_IRC_DELIVERY_CODE 0x30
#define LIVEBATTLEMATCH_IRC_DELIVERY_SIZE 0x160

struct LiveBattleMatchIrc {
    // The steps of whichever function is stepping the link
    u32 state;
    HeapID heapId;
    // Where the other machine's data arrives
    u8 buffer[LIVEBATTLEMATCH_IRC_BUFFER_SIZE];
    // Whether the data of each command came, until it is taken
    BOOL playerDataArrived;
    BOOL partyArrived;
    BOOL unusedArrived;
    BOOL readyArrived;
    void *p_delivery; // the ROM's name, from its assert
    int deliveryResult;
};

static void LiveBattleMatchIrc_OnPlayerData(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *LiveBattleMatchIrc_GetBuffer(int netId, void *work, int size);
static void LiveBattleMatchIrc_OnParty(int netId, int size, const void *data, void *work, NetHandle *handle);
static void LiveBattleMatchIrc_OnUnused(int netId, int size, const void *data, void *work, NetHandle *handle);
static void LiveBattleMatchIrc_OnReady(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL LiveBattleMatchIrc_OnLinkChanged(int a, int b);

static const NetCommand sLiveBattleMatchIrcCommands[LIVEBATTLEMATCH_IRC_COMMAND_NUM] = {
    {LiveBattleMatchIrc_OnPlayerData, LiveBattleMatchIrc_GetBuffer},
    {LiveBattleMatchIrc_OnParty, LiveBattleMatchIrc_GetBuffer},
    {LiveBattleMatchIrc_OnUnused, LiveBattleMatchIrc_GetBuffer},
    {LiveBattleMatchIrc_OnReady, LiveBattleMatchIrc_GetBuffer},
};

static const GFLNetInitData sLiveBattleMatchIrcNetInit = {
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
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
    0xd,
    0xf,
    0x10,
    0xf0,
    0,
    2,
    0x64,
    0x10,
    1,
    0,
    3,
    1,
    0x1f,
    {0xff, 0xff, 0xff, 0xff},
    0,
    0,
};

// Whether the other machine is still linked, which the link reports
static int sLiveBattleMatchIrcIsLinked = TRUE;

LiveBattleMatchIrc *LiveBattleMatchIrc_Create(GameData *gameData, HeapID heapId) {
    LiveBattleMatchIrc *irc = GFL_HeapAllocate(heapId, sizeof(LiveBattleMatchIrc), FALSE, "livebattlematch_irc.c", 142);

    sys_memset(irc, 0, sizeof(LiveBattleMatchIrc));
    irc->heapId = heapId;
    if (func_02042788()) {
        func_02040c20(LIVEBATTLEMATCH_IRC_COMMAND_PLAYER_DATA, sLiveBattleMatchIrcCommands, LIVEBATTLEMATCH_IRC_COMMAND_NUM, irc);
    }
    return irc;
}

void LiveBattleMatchIrc_Delete(LiveBattleMatchIrc *irc) {
    if (func_02042788()) {
        func_02040c64(LIVEBATTLEMATCH_IRC_COMMAND_PLAYER_DATA);
    }
    GFL_HeapFree(irc);
}

void LiveBattleMatchIrc_Main(LiveBattleMatchIrc *irc) {
}

BOOL LiveBattleMatchIrc_CheckError(LiveBattleMatchIrc *irc) {
    if (GFL_NetErrCheck()) {
        if (irc->p_delivery != NULL) {
            func_ov012_02153150(irc->p_delivery);
            irc->p_delivery = NULL;
        }
        func_02012154();
        func_02011de0();
        func_02012144();
        return TRUE;
    }
    return FALSE;
}

BOOL LiveBattleMatchIrc_ResetLink(LiveBattleMatchIrc *irc) {
    irc->state = 0;
    return TRUE;
}

int LiveBattleMatchIrc_StepLink(LiveBattleMatchIrc *irc) {
    switch (irc->state) {
    case 0:
        func_020425ec(&sLiveBattleMatchIrcNetInit, NULL, NULL);
        sLiveBattleMatchIrcIsLinked = TRUE;
        irc->state = 1;
        break;
    case 1:
        if (func_02042788()) {
            irc->state = 2;
        }
        break;
    case 2:
        func_02040c20(LIVEBATTLEMATCH_IRC_COMMAND_PLAYER_DATA, sLiveBattleMatchIrcCommands, LIVEBATTLEMATCH_IRC_COMMAND_NUM, irc);
        NetDevIrc_SetPeerFilter(LiveBattleMatchIrc_OnLinkChanged);
        irc->state = 3;
        break;
    case 3:
        func_020429f8(NULL);
        irc->state = 4;
        break;
    case 4:
        if (func_02042a78() > 1) {
            irc->state = 5;
        }
        if (!sLiveBattleMatchIrcIsLinked) {
            irc->state = 6;
        }
        break;
    case 5:
        NetDevIrc_SetPeerFilter(NULL);
        return 1;
    case 6:
        NetDevIrc_SetPeerFilter(NULL);
        return 2;
    }
    return 0;
}

void LiveBattleMatchIrc_CancelLink(LiveBattleMatchIrc *irc) {
    if (irc->state >= 2) {
        func_02040c64(LIVEBATTLEMATCH_IRC_COMMAND_PLAYER_DATA);
    }
    NetDevIrc_SetPeerFilter(NULL);
    irc->state = 0;
}

BOOL LiveBattleMatchIrc_StepCancel(LiveBattleMatchIrc *irc) {
    switch (irc->state) {
    case 0:
        if (func_02042788()) {
            func_02042860(NULL);
            irc->state = 1;
        } else {
            irc->state = 2;
        }
        break;
    case 1:
        if (func_020427a4()) {
            irc->state = 2;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

BOOL LiveBattleMatchIrc_ResetEnd(LiveBattleMatchIrc *irc) {
    irc->state = 0;
    return TRUE;
}

BOOL LiveBattleMatchIrc_StepEnd(LiveBattleMatchIrc *irc) {
    switch (irc->state) {
    case 0:
        func_02040c64(LIVEBATTLEMATCH_IRC_COMMAND_PLAYER_DATA);
        func_02042860(NULL);
        irc->state = 1;
        break;
    case 1:
        if (func_020427a4()) {
            irc->state = 2;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

BOOL LiveBattleMatchIrc_SendPlayerData(LiveBattleMatchIrc *irc, void *data) {
    u8 partnerId = func_02042a6c(func_02040440()) ? 0 : 1;
    NetHandle *handle = func_02040440();

    return func_02042c18(handle, partnerId, LIVEBATTLEMATCH_IRC_COMMAND_PLAYER_DATA,
                         PokeParty_GetSaveDataSize() + LIVEBATTLEMATCH_PLAYER_HEADER_SIZE, data, TRUE, TRUE, TRUE);
}

BOOL LiveBattleMatchIrc_TakePlayerData(LiveBattleMatchIrc *irc, void **data) {
    BOOL arrived = irc->playerDataArrived;

    if (arrived) {
        irc->playerDataArrived = FALSE;
        *data = irc->buffer;
    }
    return arrived;
}

BOOL LiveBattleMatchIrc_SendParty(LiveBattleMatchIrc *irc, PokeParty *party) {
    u8 partnerId = func_02042a6c(func_02040440()) ? 0 : 1;
    NetHandle *handle = func_02040440();

    return func_02042c18(handle, partnerId, LIVEBATTLEMATCH_IRC_COMMAND_PARTY, PokeParty_GetSaveDataSize(), party, TRUE,
                         TRUE, TRUE);
}

BOOL LiveBattleMatchIrc_TakeParty(LiveBattleMatchIrc *irc, PokeParty *party) {
    BOOL arrived = irc->partyArrived;

    if (arrived) {
        irc->partyArrived = FALSE;
        sys_memcpy(irc->buffer, party, PokeParty_GetSaveDataSize());
    }
    return arrived;
}

BOOL LiveBattleMatchIrc_SendReady(LiveBattleMatchIrc *irc, const u32 *flag) {
    u8 partnerId = func_02042a6c(func_02040440()) ? 0 : 1;

    return func_02042c18(func_02040440(), partnerId, LIVEBATTLEMATCH_IRC_COMMAND_READY, sizeof(u32), flag, TRUE, TRUE,
                         TRUE);
}

BOOL LiveBattleMatchIrc_TakeReady(LiveBattleMatchIrc *irc, u32 *flag) {
    BOOL arrived = irc->readyArrived;

    if (arrived) {
        irc->readyArrived = FALSE;
        sys_memcpy(irc->buffer, flag, sizeof(u32));
    }
    return arrived;
}

void LiveBattleMatchIrc_StartDelivery(LiveBattleMatchIrc *p_wk, void *buffer) {
    DeliveryIrcInit init;

    sys_memset(&init, 0, sizeof(DeliveryIrcInit));
    init.code = LIVEBATTLEMATCH_IRC_DELIVERY_CODE;
    init.data[0].datasize = LIVEBATTLEMATCH_IRC_DELIVERY_SIZE;
    init.data[0].pData = buffer;
    init.unk04 = 0;
    init.heapId = p_wk->heapId;
    init.data[0].region = region;
    init.data[0].mask = LIVEBATTLEMATCH_VERSION_MASK;
    init.dataNum = 1;
    GFL_ASSERT(p_wk->p_delivery == NULL);
    p_wk->p_delivery = func_ov012_0215309c(&init);
    p_wk->state = 0;
}

int LiveBattleMatchIrc_StepDelivery(LiveBattleMatchIrc *irc) {
    if (irc->p_delivery != NULL) {
        func_ov012_0215313c(irc->p_delivery);
    }
    switch (irc->state) {
    case 0:
        func_ov012_021530f8(irc->p_delivery);
        irc->state++;
        break;
    case 1:
        irc->deliveryResult = func_ov012_02153130(irc->p_delivery);
        if (irc->deliveryResult != 0) {
            irc->state++;
        }
        break;
    case 2:
        func_ov012_02153150(irc->p_delivery);
        irc->p_delivery = NULL;
        irc->state++;
        break;
    case 3:
        if (func_02042ab8()) {
            switch (irc->deliveryResult) {
            case 1:
                return 1;
            case 3:
                return 3;
            case 2:
            default:
                return 2;
            }
        }
        break;
    }
    return 0;
}

void LiveBattleMatchIrc_ResetDeliveryEnd(LiveBattleMatchIrc *irc) {
    irc->state = 0;
}

BOOL LiveBattleMatchIrc_StepDeliveryEnd(LiveBattleMatchIrc *irc) {
    if (irc->p_delivery != NULL) {
        func_ov012_0215313c(irc->p_delivery);
    }
    switch (irc->state) {
    case 0:
        if (irc->p_delivery != NULL) {
            func_ov012_02153150(irc->p_delivery);
            irc->p_delivery = NULL;
        }
        irc->state++;
        break;
    case 1:
        if (func_02042ab8()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void LiveBattleMatchIrc_OnPlayerData(int netId, int size, const void *data, void *work, NetHandle *handle) {
    LiveBattleMatchIrc *irc = work;

    if (handle == func_02040440() && netId != func_02042a6c(func_02040440())) {
        irc->playerDataArrived = TRUE;
    }
}

static void LiveBattleMatchIrc_OnParty(int netId, int size, const void *data, void *work, NetHandle *handle) {
    LiveBattleMatchIrc *irc = work;

    if (handle == func_02040440() && netId != func_02042a6c(func_02040440())) {
        irc->partyArrived = TRUE;
    }
}

static void LiveBattleMatchIrc_OnUnused(int netId, int size, const void *data, void *work, NetHandle *handle) {
    LiveBattleMatchIrc *irc = work;

    if (handle == func_02040440() && netId != func_02042a6c(func_02040440())) {
        irc->unusedArrived = TRUE;
    }
}

static void LiveBattleMatchIrc_OnReady(int netId, int size, const void *data, void *work, NetHandle *handle) {
    LiveBattleMatchIrc *irc = work;

    if (handle == func_02040440() && netId != func_02042a6c(func_02040440())) {
        irc->readyArrived = TRUE;
    }
}

static void *LiveBattleMatchIrc_GetBuffer(int netId, void *work, int size) {
    LiveBattleMatchIrc *irc = work;

    if (netId == func_02042a6c(func_02040440())) {
        GFL_ASSERT(0);
        return NULL;
    }
    return irc->buffer;
}

static BOOL LiveBattleMatchIrc_OnLinkChanged(int a, int b) {
    BOOL linked = a == b;

    sLiveBattleMatchIrcIsLinked = linked;
    return linked;
}
