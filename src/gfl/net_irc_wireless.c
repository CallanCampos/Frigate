#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/net_irc_wireless.h"
#include "gfl/net_state.h"
#include "gfl/net_system.h"
#include "gfl/std.h"
#include "gfl/ui.h"

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
    void *unk74;
    // The game's callbacks from its init data
    void *(*unk18)(void *work);
    int (*unk1C)(void *work);
    void (*exitCallback)(void *work);
    void (*unk84)(void *work);
    void (*unk88)(void *work, BOOL a1);
    void (*unk8C)(void *work);
    NetIwCollectMac collect;
    u8 unkB4[0x4c];
    // Frames left to wait
    u8 timer;
    u8 unk101;
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
    u8 unk108;
    u8 state;
    u8 unk10A;
    u8 gameCommandBase;
} NetIrcWireless;

static NetIrcWireless *NetIwSys;

static void func_02043170(void *work);
static void func_020431a8(void *work);
static void func_020431c0(NetIrcWireless *sys, int state);
static void *func_02043720(void *work);
static int func_02043748(void *work);

const GFLNetInitData *func_02042f74(const GFLNetInitData *pNetInit, void *work) {
    NetIrcWireless *sys;

    if (NetIwSys != NULL && NetIwSys->unk101 == TRUE) {
        return pNetInit;
    }
    sys = GFL_HeapAllocate(HEAPID_TAIL(pNetInit->parentHeapId), sizeof(NetIrcWireless), TRUE, "net_irc_wireless.c",
                           114);
    sys->aNetInit = *pNetInit;
    sys->work = work;
    sys->exitCallback = pNetInit->unk2C;
    sys->unk18 = pNetInit->unk18;
    sys->unk1C = pNetInit->unk1C;
    sys->aNetInit.bNetType = 3;
    sys->aNetInit.unk2C = func_020431a8;
    sys->aNetInit.unk18 = func_02043720;
    sys->aNetInit.unk1C = func_02043748;
    sys->bMPMode = sys->aNetInit.bMPMode;
    sys->aNetInit.bMPMode = FALSE;
    sys->maxConnectNum = sys->aNetInit.maxConnectNum;
    NetIwSys = sys;
    GCTX_HIDBlockSleep(0x20);
    return &NetIwSys->aNetInit;
}

void func_02043028(void) {
    if (NetIwSys != NULL) {
        GCTX_HIDUnblockSleep(0x20);
        GFL_HeapFree(NetIwSys);
        NetIwSys = NULL;
    }
}

void func_02043048(void) {
    if (NetIwSys != NULL && NetIwSys->unk101 == 0) {
        func_02043028();
    }
}

BOOL func_02043068(void) {
    if (NetIwSys != NULL) {
        return TRUE;
    }
    return FALSE;
}

void func_0204307c(void *a0) {
    NetIwSys->unk74 = a0;
}

void (*func_02043088(void (*a0)(void *work), void (*a1)(void *work, BOOL a1), void (*a2)(void *work)))(void *work) {
    if (NetIwSys == NULL) {
        return a0;
    }
    if (a0 != NULL) {
        NetIwSys->unk84 = a0;
    }
    if (a1 != NULL) {
        NetIwSys->unk88 = a1;
    }
    if (a2 != NULL) {
        NetIwSys->unk8C = a2;
    }
    return func_02043170;
}

void func_020430bc(const u8 *mac) {
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
            sys_memcpy(mac, NetIwSys->collect.macs[i], 6);
            NetIwSys->collect.num = i;
            return;
        }
    }
    func_02042454(1);
}

void func_0204313c(u8 *mac, int index) {
    if (NetIwSys != NULL) {
        sys_memcpy(NetIwSys->collect.macs[index], mac, 6);
    } else {
        GFL_ASSERT(0);
    }
}

static void func_02043170(void *work) {
    if (NetIwSys != NULL) {
        NetIwSys->connected = TRUE;
        if (NetIwSys->netIdOverride != 0) {
            NetIwSys->netId = NetIwSys->netIdOverride - 1;
        } else {
            NetIwSys->netId = func_0203ffc4();
        }
    }
}

static void func_020431a8(void *work) {
    if (NetIwSys != NULL) {
        NetIwSys->ended = TRUE;
    }
}

static void func_020431c0(NetIrcWireless *sys, int state) {
    sys->state = state;
}

void func_020431cc(void) {
    NetIrcWireless *sys = NetIwSys;

    if (sys == NULL) {
        return;
    }
    switch (sys->state) {
    case 0:
        if (sys->connected == TRUE) {
            sys->unk101 = TRUE;
            func_02040624(func_02040440(), 3, 0);
            func_020431c0(sys, 1);
        }
        break;
    case 1:
        if (func_02040664(func_02040440(), 3, 0)) {
            func_020431c0(sys, 2);
        }
        break;
    case 2:
        if (func_0203ffc4() == 0) {
            if (func_02042be8(func_02040440(), 1, 0, NULL)) {
                func_020431c0(sys, 3);
                sys->timer = 200;
            }
        } else if (func_02042be8(func_02040440(), 16, 0, NULL)) {
            func_020431c0(sys, 3);
        }
        break;
    case 3:
        sys->timer--;
        if (func_02042ab8()) {
            if (NetIwSys->unk8C != NULL) {
                NetIwSys->unk8C(NetIwSys->work);
            }
            if (sys->unk10A && sys->aNetInit.maxConnectNum == 4 && sys->netId == 0) {
                func_020431c0(sys, 14);
                sys->unk108++;
            } else if (sys->netId == 0 && sys->aNetInit.maxConnectNum == 4) {
                sys->unk108++;
                func_02042860(NULL);
                sys->unk88(sys->work, TRUE);
                func_020431c0(sys, 4);
            } else {
                sys->unk88(sys->work, FALSE);
                func_020431c0(sys, 9);
            }
        }
        if (sys->timer == 0) {
            func_02042860(NULL);
        }
        break;
    case 4:
        if (!func_02042788()) {
            sys->aNetInit.bNetType = 3;
            sys->aNetInit.unk2C = func_020431a8;
            if (sys->gameCommandBase != 0) {
                sys->aNetInit.gameCommandBase = sys->gameCommandBase;
            }
            sys->connected = FALSE;
            sys->ended = FALSE;
            func_020425ec(&sys->aNetInit, NULL, sys->work);
            func_020431c0(sys, 5);
        }
        break;
    case 5:
        if (func_02042788() == TRUE) {
            func_02042e78()->pDevTable->unkC8(NetIwSys->unk74);
            func_020429f8(func_02043170);
            func_020431c0(sys, 6);
        }
        break;
    case 6:
        if (sys->connected == TRUE) {
            sys->unk101 = TRUE;
            if (func_02042be8(func_02040440(), 9, sizeof(NetIwCollectMac), &NetIwSys->collect)) {
                func_020431c0(sys, 7);
            }
        }
        break;
    case 7:
        if (sys->connected == TRUE && func_02042be8(func_02040440(), 1, 0, NULL)) {
            func_02042a50(200);
            func_020431c0(sys, 8);
        }
        break;
    case 8:
        if (func_02042ab8()) {
            if (NetIwSys->unk8C != NULL) {
                NetIwSys->unk8C(NetIwSys->work);
            }
            func_020431c0(sys, 9);
        }
        break;
    case 9:
        if (func_02042ab8()) {
            if (sys->gameCommandBase != 0) {
                sys->aNetInit.unk6E = 0;
                sys->aNetInit.gameCommandBase = sys->gameCommandBase;
            }
            sys->aNetInit.bNetType = 0;
            sys->aNetInit.unk2C = sys->exitCallback;
            sys->aNetInit.bMPMode = sys->bMPMode;
            func_020425ec(&sys->aNetInit, NULL, sys->work);
            NetIwSys->unk101 = FALSE;
            func_020431c0(sys, 10);
        }
        break;
    case 10:
        if (func_02042788() == TRUE) {
            if (sys->netId == 0) {
                func_02042970();
                func_020431c0(sys, 12);
            } else if (sys->aNetInit.maxConnectNum == 4) {
                func_02042968();
                func_020431c0(sys, 11);
            } else {
                func_02042950(sys->collect.macs[0]);
                func_020431c0(sys, 12);
            }
        }
        break;
    case 11: {
        int index = 0;
        u8 mac[6];
        int i;

        // Find the beacon of the machine that collected this machine's MAC address
        func_0207c33c(mac);
        for (;;) {
            NetIwCollectMac *collect = func_020428a8(index);

            if (collect == NULL) {
                break;
            }
            for (i = 0; i < _COLLECTMAC_NUM; i++) {
                if (GFL_STD_MemCmp(mac, collect->macs[i], 6) == 0) {
                    func_0204295c(func_020428c8(index));
                    func_020431c0(sys, 12);
                    break;
                }
            }
            index++;
        }
        break;
    }
    case 12:
        if (sys->netId == 0) {
            if (func_02040474() != 0 && func_02040504() == TRUE) {
                func_020431c0(sys, 13);
            }
        } else if (func_02040504() == TRUE) {
            func_020431c0(sys, 13);
        }
        break;
    case 13:
        if (sys->netId != 0) {
            if (sys->unk84 != NULL) {
                sys->unk84(sys->work);
            }
            func_02043048();
        } else if (func_02040474() >= sys->maxConnectNum) {
            if (sys->unk84 != NULL) {
                sys->unk84(sys->work);
            }
            func_02043048();
        }
        break;
    case 14:
        if (!func_02042788()) {
            sys->aNetInit.bNetType = 3;
            sys->aNetInit.unk2C = func_020431a8;
            sys->connected = FALSE;
            sys->ended = FALSE;
            func_020425ec(&sys->aNetInit, NULL, sys->work);
            func_020431c0(sys, 15);
        }
        break;
    case 15:
        if (func_02042788() == TRUE) {
            func_02042e78()->pDevTable->unkC8(NetIwSys->unk74);
            func_020429f8(func_02043170);
            func_020431c0(sys, 16);
        }
        break;
    case 16:
        if (sys->connected == TRUE) {
            func_02040624(func_02040440(), 3, 0);
            NetIwSys->unk101 = TRUE;
            func_020431c0(sys, 17);
        }
        break;
    case 17:
        if (func_02040664(func_02040440(), 3, 0) && func_02042be8(func_02040440(), 1, 0, NULL)) {
            func_020431c0(sys, 18);
            sys->timer = 200;
        }
        break;
    case 18:
        sys->timer--;
        if (func_02042ab8() || sys->timer == 0) {
            if (NetIwSys->unk8C != NULL) {
                NetIwSys->unk8C(NetIwSys->work);
            }
            if (sys->unk10A && sys->unk108 < 3) {
                sys->unk108++;
                if (sys->unk108 >= 3) {
                    func_020431c0(sys, 9);
                } else {
                    func_020431c0(sys, 14);
                }
            } else {
                func_020431c0(sys, 9);
            }
        }
        break;
    }
}

static void *func_02043720(void *work) {
    NetIrcWireless *sys = NetIwSys;

    if (sys == NULL) {
        return NULL;
    }
    sys_memcpy(sys->unk18(work), sys->unkB4, sizeof(sys->unkB4));
    return &sys->collect;
}

static int func_02043748(void *work) {
    NetIrcWireless *sys = NetIwSys;

    if (sys == NULL) {
        return 0;
    }
    return sys->unk1C(work) + sizeof(NetIwCollectMac);
}

void func_02043764(int netId, int size, const void *data, void *work, NetHandle *handle) {
    NetIwCollectMac *pTemp = data;
    int i;
    int num;

    GFL_ASSERT(pTemp->num<_COLLECTMAC_NUM);
    num = pTemp->num;
    if (num > _COLLECTMAC_NUM) {
        num = _COLLECTMAC_NUM;
    }
    for (i = 0; i < num; i++) {
        func_020430bc(pTemp->macs[i]);
    }
}

BOOL func_020437a0(void) {
    if (NetIwSys == NULL) {
        return FALSE;
    }
    if (NetIwSys->state == 16) {
        NetIwSys->maxConnectNum = NetIwSys->unk108 + 1;
        func_02042860(NULL);
        NetIwSys->state = 9;
        return TRUE;
    }
    return FALSE;
}

void func_020437dc(int netId) {
    GFL_ASSERT(NetIwSys);
    if (NetIwSys != NULL) {
        NetIwSys->netIdOverride = netId + 1;
        NetIwSys->unk10A = TRUE;
    }
}

u8 func_0204381c(void) {
    if (NetIwSys == NULL) {
        return 0;
    }
    return NetIwSys->unk108;
}

void func_02043834(int gameCommandBase) {
    GFL_ASSERT(NetIwSys);
    if (NetIwSys != NULL) {
        NetIwSys->gameCommandBase = gameCommandBase;
    }
}
