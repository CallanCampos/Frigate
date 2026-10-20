#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/net_irc_wireless.h"
#include "gfl/net_state.h"
#include "gfl/net_system.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "nitro/os.h"

// How many MAC addresses are collected over infrared
#define _COLLECTMAC_NUM 4

typedef struct {
    int num;
    u8 macs[_COLLECTMAC_NUM][8];
} NetIwCollectMac;

typedef struct {
    // The init data the networks are started with, the game's with the callbacks below swapped for this file's
    GFLNetInitData aNetInit;
    void *work;
    // The filter of the infrared machines to link with (NetIw_SetPeerFilter)
    void *peerFilter;
    // The game's callbacks from its init data
    void *(*getBeaconData)(void *work);
    int (*getBeaconSize)(void *work);
    void (*exitCallback)(void *work);
    // Called once the wireless network is connected
    void (*connectCallback)(void *work);
    void (*unk88)(void *work, BOOL a1);
    void (*unk8C)(void *work);
    NetIwCollectMac collect;
    // The game's beacon data, which the beacons send after the collected MAC addresses
    u8 gameBeacon[0x4c];
    // Frames left to wait
    u8 timer;
    // Set from the infrared link until the wireless network starts, while the networks are switched
    u8 switchingToWireless;
    // Set when the network has connected and when it has ended
    u8 connected;
    u8 ended;
    // The game's MP mode and machine count, which the infrared network does not use
    u8 bMPMode;
    // This machine's ID once connected over infrared, 0 for the parent
    u8 netId;
    // The ID to use in place of netId, plus 1, or 0
    u8 netIdOverride;
    u8 maxConnectNum;
    // How many machines linked over infrared
    u8 linkedCount;
    u8 state;
    // Set by NetIw_OverrideNetId
    u8 netIdOverridden;
    u8 gameCommandBase;
} NetIrcWireless;

static NetIrcWireless *NetIwSys;

static void NetIw_ConnectedCallback(void *work);
static void NetIw_EndedCallback(void *work);
static void NetIw_SetState(NetIrcWireless *sys, int state);
static void *NetIw_GetBeaconData(void *work);
static int NetIw_GetBeaconSize(void *work);

const GFLNetInitData *NetIw_Start(const GFLNetInitData *pNetInit, void *work) {
    NetIrcWireless *sys;

    if (NetIwSys != NULL && NetIwSys->switchingToWireless == TRUE) {
        return pNetInit;
    }
    sys =
        GFL_HeapAllocate(HEAPID_TAIL(pNetInit->parentHeapId), sizeof(NetIrcWireless), TRUE, "net_irc_wireless.c", 114);
    sys->aNetInit = *pNetInit;
    sys->work = work;
    sys->exitCallback = pNetInit->endCallback;
    sys->getBeaconData = pNetInit->getBeaconData;
    sys->getBeaconSize = pNetInit->getBeaconSize;
    sys->aNetInit.bNetType = 3;
    sys->aNetInit.endCallback = NetIw_EndedCallback;
    sys->aNetInit.getBeaconData = NetIw_GetBeaconData;
    sys->aNetInit.getBeaconSize = NetIw_GetBeaconSize;
    sys->bMPMode = sys->aNetInit.bMPMode;
    sys->aNetInit.bMPMode = FALSE;
    sys->maxConnectNum = sys->aNetInit.maxConnectNum;
    NetIwSys = sys;
    GCTX_HIDBlockSleep(0x20);
    return &NetIwSys->aNetInit;
}

void NetIw_Free(void) {
    if (NetIwSys != NULL) {
        GCTX_HIDUnblockSleep(0x20);
        GFL_HeapFree(NetIwSys);
        NetIwSys = NULL;
    }
}

void NetIw_FreeIfIdle(void) {
    if (NetIwSys != NULL && NetIwSys->switchingToWireless == 0) {
        NetIw_Free();
    }
}

BOOL NetIw_IsRunning(void) {
    if (NetIwSys != NULL) {
        return TRUE;
    }
    return FALSE;
}

void NetIw_SetPeerFilter(void *a0) {
    NetIwSys->peerFilter = a0;
}

void (*NetIw_SetCallbacks(void (*a0)(void *work), void (*a1)(void *work, BOOL a1),
                          void (*a2)(void *work)))(void *work) {
    if (NetIwSys == NULL) {
        return a0;
    }
    if (a0 != NULL) {
        NetIwSys->connectCallback = a0;
    }
    if (a1 != NULL) {
        NetIwSys->unk88 = a1;
    }
    if (a2 != NULL) {
        NetIwSys->unk8C = a2;
    }
    return NetIw_ConnectedCallback;
}

void NetIw_AddCollectedMac(const u8 *mac) {
    int i;
    u8 empty[6];
    u8 *p = empty;

    if (NetIwSys == NULL) {
        return;
    }
    for (i = 0; i < _COLLECTMAC_NUM; i++) {
        if (GFL_STD_MemCmp(mac, NetIwSys->collect.macs[i], 6) == 0) {
            return;
        }
    }
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    for (i = 0; i < _COLLECTMAC_NUM; i++) {
        if (GFL_STD_MemCmp(empty, NetIwSys->collect.macs[i], 6) == 0) {
            MI_CpuCopy8(mac, NetIwSys->collect.macs[i], 6);
            NetIwSys->collect.num = i;
            return;
        }
    }
    NetState_SetError(1);
}

void NetIw_GetCollectedMac(u8 *mac, int index) {
    if (NetIwSys != NULL) {
        MI_CpuCopy8(NetIwSys->collect.macs[index], mac, 6);
    } else {
        GFL_ASSERT(0);
    }
}

static void NetIw_ConnectedCallback(void *work) {
    if (NetIwSys != NULL) {
        NetIwSys->connected = TRUE;
        if (NetIwSys->netIdOverride != 0) {
            NetIwSys->netId = NetIwSys->netIdOverride - 1;
        } else {
            NetIwSys->netId = NetSystem_GetNetId();
        }
    }
}

static void NetIw_EndedCallback(void *work) {
    if (NetIwSys != NULL) {
        NetIwSys->ended = TRUE;
    }
}

static void NetIw_SetState(NetIrcWireless *sys, int state) {
    sys->state = state;
}

void NetIw_Update(void) {
    NetIrcWireless *sys = NetIwSys;

    if (sys == NULL) {
        return;
    }
    switch (sys->state) {
    case 0:
        if (sys->connected == TRUE) {
            sys->switchingToWireless = TRUE;
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 3, 0);
            NetIw_SetState(sys, 1);
        }
        break;
    case 1:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 3, 0)) {
            NetIw_SetState(sys, 2);
        }
        break;
    case 2:
        if (NetSystem_GetNetId() == 0) {
            if (GFL_NetSendCommand(NetHandle_GetOwn(), 1, 0, NULL)) {
                NetIw_SetState(sys, 3);
                sys->timer = 200;
            }
        } else if (GFL_NetSendCommand(NetHandle_GetOwn(), 16, 0, NULL)) {
            NetIw_SetState(sys, 3);
        }
        break;
    case 3:
        sys->timer--;
        if (GFL_NetIsNotInitialized()) {
            if (NetIwSys->unk8C != NULL) {
                NetIwSys->unk8C(NetIwSys->work);
            }
            if (sys->netIdOverridden && sys->aNetInit.maxConnectNum == 4 && sys->netId == 0) {
                NetIw_SetState(sys, 14);
                sys->linkedCount++;
            } else if (sys->netId == 0 && sys->aNetInit.maxConnectNum == 4) {
                sys->linkedCount++;
                GFL_NetExit(NULL);
                sys->unk88(sys->work, TRUE);
                NetIw_SetState(sys, 4);
            } else {
                sys->unk88(sys->work, FALSE);
                NetIw_SetState(sys, 9);
            }
        }
        if (sys->timer == 0) {
            GFL_NetExit(NULL);
        }
        break;
    case 4:
        if (!GFL_NetIsInitialized()) {
            sys->aNetInit.bNetType = 3;
            sys->aNetInit.endCallback = NetIw_EndedCallback;
            if (sys->gameCommandBase != 0) {
                sys->aNetInit.gameCommandBase = sys->gameCommandBase;
            }
            sys->connected = FALSE;
            sys->ended = FALSE;
            GFL_NetInit(&sys->aNetInit, NULL, sys->work);
            NetIw_SetState(sys, 5);
        }
        break;
    case 5:
        if (GFL_NetIsInitialized() == TRUE) {
            GFL_NetGetSys()->pDevTable->setPeerFilter(NetIwSys->peerFilter);
            GFL_NetChangeoverConnect(NetIw_ConnectedCallback);
            NetIw_SetState(sys, 6);
        }
        break;
    case 6:
        if (sys->connected == TRUE) {
            sys->switchingToWireless = TRUE;
            if (GFL_NetSendCommand(NetHandle_GetOwn(), 9, sizeof(NetIwCollectMac), &NetIwSys->collect)) {
                NetIw_SetState(sys, 7);
            }
        }
        break;
    case 7:
        if (sys->connected == TRUE && GFL_NetSendCommand(NetHandle_GetOwn(), 1, 0, NULL)) {
            GFL_NetSetTimeout(200);
            NetIw_SetState(sys, 8);
        }
        break;
    case 8:
        if (GFL_NetIsNotInitialized()) {
            if (NetIwSys->unk8C != NULL) {
                NetIwSys->unk8C(NetIwSys->work);
            }
            NetIw_SetState(sys, 9);
        }
        break;
    case 9:
        if (GFL_NetIsNotInitialized()) {
            if (sys->gameCommandBase != 0) {
                sys->aNetInit.negotiationCommandBase = 0;
                sys->aNetInit.gameCommandBase = sys->gameCommandBase;
            }
            sys->aNetInit.bNetType = 0;
            sys->aNetInit.endCallback = sys->exitCallback;
            sys->aNetInit.bMPMode = sys->bMPMode;
            GFL_NetInit(&sys->aNetInit, NULL, sys->work);
            NetIwSys->switchingToWireless = FALSE;
            NetIw_SetState(sys, 10);
        }
        break;
    case 10:
        if (GFL_NetIsInitialized() == TRUE) {
            if (sys->netId == 0) {
                GFL_NetStartServer();
                NetIw_SetState(sys, 12);
            } else if (sys->aNetInit.maxConnectNum == 4) {
                GFL_NetStartScan();
                NetIw_SetState(sys, 11);
            } else {
                GFL_NetStartAsParent(sys->collect.macs[0]);
                NetIw_SetState(sys, 12);
            }
        }
        break;
    case 11: {
        int index = 0;
        u8 mac[6];
        int i;

        // Find the beacon of the machine that collected this machine's MAC address
        OS_GetMacAddress(mac);
        for (;;) {
            NetIwCollectMac *collect = GFL_NetGetBeaconData(index);

            if (collect == NULL) {
                break;
            }
            for (i = 0; i < _COLLECTMAC_NUM; i++) {
                if (GFL_STD_MemCmp(mac, collect->macs[i], 6) == 0) {
                    GFL_NetStartAsChild(GFL_NetGetBeaconMac(index));
                    NetIw_SetState(sys, 12);
                    break;
                }
            }
            index++;
        }
        break;
    }
    case 12:
        if (sys->netId == 0) {
            if (NetHandle_CountNegotiated() != 0 && NetHandle_SendNegotiation() == TRUE) {
                NetIw_SetState(sys, 13);
            }
        } else if (NetHandle_SendNegotiation() == TRUE) {
            NetIw_SetState(sys, 13);
        }
        break;
    case 13:
        if (sys->netId != 0) {
            if (sys->connectCallback != NULL) {
                sys->connectCallback(sys->work);
            }
            NetIw_FreeIfIdle();
        } else if (NetHandle_CountNegotiated() >= sys->maxConnectNum) {
            if (sys->connectCallback != NULL) {
                sys->connectCallback(sys->work);
            }
            NetIw_FreeIfIdle();
        }
        break;
    case 14:
        if (!GFL_NetIsInitialized()) {
            sys->aNetInit.bNetType = 3;
            sys->aNetInit.endCallback = NetIw_EndedCallback;
            sys->connected = FALSE;
            sys->ended = FALSE;
            GFL_NetInit(&sys->aNetInit, NULL, sys->work);
            NetIw_SetState(sys, 15);
        }
        break;
    case 15:
        if (GFL_NetIsInitialized() == TRUE) {
            GFL_NetGetSys()->pDevTable->setPeerFilter(NetIwSys->peerFilter);
            GFL_NetChangeoverConnect(NetIw_ConnectedCallback);
            NetIw_SetState(sys, 16);
        }
        break;
    case 16:
        if (sys->connected == TRUE) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 3, 0);
            NetIwSys->switchingToWireless = TRUE;
            NetIw_SetState(sys, 17);
        }
        break;
    case 17:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 3, 0) &&
            GFL_NetSendCommand(NetHandle_GetOwn(), 1, 0, NULL)) {
            NetIw_SetState(sys, 18);
            sys->timer = 200;
        }
        break;
    case 18:
        sys->timer--;
        if (GFL_NetIsNotInitialized() || sys->timer == 0) {
            if (NetIwSys->unk8C != NULL) {
                NetIwSys->unk8C(NetIwSys->work);
            }
            if (sys->netIdOverridden && sys->linkedCount < 3) {
                sys->linkedCount++;
                if (sys->linkedCount >= 3) {
                    NetIw_SetState(sys, 9);
                } else {
                    NetIw_SetState(sys, 14);
                }
            } else {
                NetIw_SetState(sys, 9);
            }
        }
        break;
    }
}

static void *NetIw_GetBeaconData(void *work) {
    NetIrcWireless *sys = NetIwSys;

    if (sys == NULL) {
        return NULL;
    }
    MI_CpuCopy8(sys->getBeaconData(work), sys->gameBeacon, sizeof(sys->gameBeacon));
    return &sys->collect;
}

static int NetIw_GetBeaconSize(void *work) {
    NetIrcWireless *sys = NetIwSys;

    if (sys == NULL) {
        return 0;
    }
    return sys->getBeaconSize(work) + sizeof(NetIwCollectMac);
}

void NetIw_RecvCollectedMacs(int netId, int size, const void *data, void *work, NetHandle *handle) {
    NetIwCollectMac *pTemp = data;
    int i;
    int num;

    // clang-format off
    GFL_ASSERT(pTemp->num<_COLLECTMAC_NUM);
    // clang-format on
    num = pTemp->num;
    if (num > _COLLECTMAC_NUM) {
        num = _COLLECTMAC_NUM;
    }
    for (i = 0; i < num; i++) {
        NetIw_AddCollectedMac(pTemp->macs[i]);
    }
}

BOOL NetIw_FinishCollecting(void) {
    if (NetIwSys == NULL) {
        return FALSE;
    }
    if (NetIwSys->state == 16) {
        NetIwSys->maxConnectNum = NetIwSys->linkedCount + 1;
        GFL_NetExit(NULL);
        NetIwSys->state = 9;
        return TRUE;
    }
    return FALSE;
}

void NetIw_OverrideNetId(int netId) {
    GFL_ASSERT(NetIwSys);
    if (NetIwSys != NULL) {
        NetIwSys->netIdOverride = netId + 1;
        NetIwSys->netIdOverridden = TRUE;
    }
}

u8 NetIw_GetLinkedCount(void) {
    if (NetIwSys == NULL) {
        return 0;
    }
    return NetIwSys->linkedCount;
}

void NetIw_SetGameCommandBase(int gameCommandBase) {
    GFL_ASSERT(NetIwSys);
    if (NetIwSys != NULL) {
        NetIwSys->gameCommandBase = gameCommandBase;
    }
}
