#include "types.h"
#include "gfl/backup_card.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_irc_wireless.h"
#include "gfl/net_lower_data.h"
#include "gfl/net_state.h"
#include "gfl/net_system.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "gfl/wm_icon.h"

// The infrared packets' size limit
#define GFL_NET_IRC_SEND_MAX 0x80

static struct {
    // The heap net.c makes its heaps under, once func_020425a0 has made it
    HeapID parentHeapId;
    // The offset of BG 1 of the main engine
    int bg1Y;
    GFLNetSys *pNet;
    int bg1X;
} sNet;

static void func_020427b8(void *work);
static void func_02042ae4(int x, int y);
static void func_02042b40(void);

void func_020425a0(int a0, int a1, HeapID parentHeapId, HeapID heapId) {
    const GFLNetDevTable *pDevTable = func_020116c0(GFL_NET_TYPE_WIFI);

    if (pDevTable != NULL) {
        pDevTable->unk00(a0, a1);
    }
    func_02011778(GFL_NET_TYPE_WIFI);
    if (parentHeapId != heapId) {
        sNet.parentHeapId = heapId;
        GFL_HeapCreateChild(parentHeapId, heapId, 0x9e80);
    }
    func_0203e874(0, 0);
    func_0203e7dc();
}

void func_020425ec(const GFLNetInitData *pNetInit, void (*callback)(void *work), void *work) {
    GFLNetSys *pNet;

    if (sNet.parentHeapId != 0) {
        GFL_HeapCreateChild(sNet.parentHeapId, pNetInit->heapId, 0x7800);
    } else {
        GFL_HeapCreateChild(pNetInit->parentHeapId, pNetInit->heapId, 0x7800);
    }
    pNet = GFL_HeapAllocate(pNetInit->heapId, sizeof(GFLNetSys), TRUE, "net.c", 106);
    sNet.pNet = pNet;
    pNet->unk352 = TRUE;
    pNet->pDevTable = func_020116c0(pNetInit->bNetType);
    GFL_ASSERT(pNet->pDevTable);
    if (pNetInit->bNetType == 4) {
        pNetInit = func_02042f74(pNetInit, work);
    }
    sys_memcpy(pNetInit, &pNet->aNetInit, sizeof(GFLNetInitData));
    switch (pNetInit->bNetType) {
    case GFL_NET_TYPE_WIFI:
    case GFL_NET_TYPE_WIFI_LOBBY:
    case GFL_NET_TYPE_WIFI_GTS:
        pNet->pDevTable->unk00(pNetInit->parentHeapId, 0);
        GFL_HeapCreateChild(pNetInit->parentHeapId, pNetInit->wifiHeapId | HEAPID_TAIL_BIT, pNetInit->wifiHeapSize);
        break;
    case 3:
    case 4:
        GFL_ASSERT(pNetInit->maxSendSize <= GFL_NET_IRC_SEND_MAX);
        if (sNet.parentHeapId != 0) {
            GFL_HeapCreateChild(sNet.parentHeapId, pNetInit->ircHeapId, 0x1b00);
        } else {
            GFL_HeapCreateChild(pNetInit->parentHeapId, pNetInit->ircHeapId, 0x1b00);
        }
        break;
    case 0:
    case 5:
        break;
    default:
        GFL_ASSERT(0);
        return;
    }
    pNet->devWork = work;
    func_0204129c(pNetInit->heapId, callback);
    func_0204034c(pNet);
    func_02040b94(pNetInit->gameCommandBase << 8, pNetInit->commandTable, pNetInit->commandNum, pNet->devWork,
                  pNetInit->heapId);
    if (pNetInit->bNetType != 3 && pNetInit->bNetType != 4) {
        BOOL isWifi = FALSE;

        if (pNetInit->bNetType == GFL_NET_TYPE_WIFI || pNetInit->bNetType == GFL_NET_TYPE_WIFI_LOBBY) {
            isWifi = TRUE;
        }
        func_0203e76c(pNetInit->iconX, pNetInit->iconY, isWifi, pNetInit->heapId);
    }
}


BOOL func_02042788(void) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet == NULL) {
        return FALSE;
    }
    return pNet->pDevTable->unk6C();
}

BOOL func_020427a4(void) {
    if (func_02042e78() == NULL) {
        return TRUE;
    }
    return FALSE;
}

static void func_020427b8(void *work) {
    GFLNetSys *pNet = func_02042e78();
    HeapID heapId = pNet->aNetInit.heapId;
    HeapID wifiHeapId = pNet->aNetInit.wifiHeapId;
    HeapID ircHeapId = pNet->aNetInit.ircHeapId;
    u8 type = pNet->aNetInit.bNetType;

    if (pNet->exitCallback != NULL) {
        pNet->exitCallback(pNet->devWork);
    }
    if (pNet->aNetInit.unk2C != NULL) {
        pNet->aNetInit.unk2C(pNet->devWork);
    }
    func_020403a4(pNet);
    func_02011778(type);
    GFL_HeapFree(pNet);
    sNet.pNet = NULL;
    switch (type) {
    case GFL_NET_TYPE_WIFI:
    case GFL_NET_TYPE_WIFI_LOBBY:
    case GFL_NET_TYPE_WIFI_GTS:
        GFL_HeapDelete(wifiHeapId);
        break;
    case 3:
    case 4:
        GFL_HeapDelete(ircHeapId);
        break;
    case 0:
    case 5:
        break;
    default:
        GFL_ASSERT(0);
        return;
    }
    GFL_HeapDelete(heapId);
    GCTX_HIDUnblockSleep(4);
}

BOOL func_02042860(void (*callback)(void *work)) {
    GFLNetSys *pNet = func_02042e78();

    func_02043048();
    if (pNet != NULL) {
        u8 type = pNet->aNetInit.bNetType;

        pNet->exitCallback = callback;
        if (type == GFL_NET_TYPE_WIFI) {
            func_0204230c(func_020427b8);
        } else {
            func_02041da8(func_020427b8);
        }
        func_0203e7dc();
        return TRUE;
    }
    return FALSE;
}


void func_020428a0(void) {
    func_02041de4();
}

void *func_020428a8(int index) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet == NULL) {
        return NULL;
    }
    return pNet->pDevTable->unk28(index);
}

u8 *func_020428c8(int index) {
    GFLNetSys *pNet = func_02042e78();

    return pNet->pDevTable->unk2C(index);
}

BOOL func_020428e0(void) {
    GFLNetSys *pNet = func_02042e78();

    func_02043b44();
    func_020431cc();
    if (func_02041410()) {
        return TRUE;
    }
    if (pNet == NULL) {
        return TRUE;
    }
    func_0203e838();
    func_020406e0();
    func_0203efe8();
    return func_02040198();
}

void func_02042918(void) {
    GFLNetSys *pNet = func_02042e78();

    if (!func_02041410() && pNet != NULL && pNet->pDevTable != NULL) {
        u8 type = func_02042e84()->bNetType;

        if (type != GFL_NET_TYPE_WIFI && type != GFL_NET_TYPE_WIFI_LOBBY) {
            pNet->pDevTable->update(0);
        }
    }
}

void func_02042950(const u8 *mac) {
    func_02041354(mac, TRUE);
}

void func_0204295c(const u8 *mac) {
    func_02041354(mac, FALSE);
}

void func_02042968(void) {
    func_020413f0();
}

void func_02042970(void) {
    GFLNetSys *pNet = func_02042e78();

    switch (pNet->aNetInit.bNetType) {
    case GFL_NET_TYPE_WIFI:
    case GFL_NET_TYPE_WIFI_LOBBY:
        func_0204208c();
        break;
    case 0:
    case 5:
        func_020414c0(pNet->aNetInit.heapId);
        break;
    }
}

void func_020429a8(void (*callback)(void *work), void (*a1)(void *work, BOOL a1), void (*a2)(void *work)) {
    GFLNetSys *pNet = func_02042e78();

    if (func_02043068() == TRUE) {
        callback = func_02043088(callback, a1, a2);
    }
    func_02041a30(pNet->aNetInit.heapId, callback, 1);
}

void func_020429d8(int a0) {
    func_02043834(a0);
}

void func_020429e0(u8 *mac, int index) {
    func_0204313c(mac, index);
}

void func_020429e8(void *a0) {
    func_0204307c(a0);
}

void func_020429f0(void) {
    func_02043028();
}

void func_020429f8(void (*callback)(void *work)) {
    GFLNetSys *pNet = func_02042e78();

    func_02041a30(pNet->aNetInit.heapId, callback, 1);
}

void func_02042a10(int unused) {
    GFLNetSys *pNet = func_02042e78();

    func_02041dfc();
}

void func_02042a1c(int a0) {
    if (func_02042788()) {
        func_0203eea4(a0);
    }
}

void func_02042a30(int mode, int a1, const u8 *mac) {
    func_02041c00(mode, a1, mac);
}

BOOL func_02042a38(void) {
    return func_020437a0();
}

void func_02042a40(int a0) {
    func_020437dc(a0);
}

u8 func_02042a48(void) {
    return func_0204381c();
}

void func_02042a50(int a0) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet->pDevTable != NULL) {
        pNet->pDevTable->unkC4(a0);
    }
}

u8 func_02042a6c(NetHandle *handle) {
    return func_020401dc(handle);
}

int func_02042a78(void) {
    return func_02040474();
}

BOOL func_02042a80(int netId) {
    if (func_02042e78() == NULL) {
        return FALSE;
    }
    return func_0204044c(func_02040414(netId));
}

void func_02042a9c(NetHandle *handle, int a1) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet->pDevTable->unkB8 != NULL) {
        pNet->pDevTable->unkB8(a1);
    }
}

BOOL func_02042ab8(void) {
    if (sNet.pNet == NULL) {
        return TRUE;
    }
    if (!func_02042e78()->pDevTable->unk6C()) {
        return TRUE;
    }
    return FALSE;
}

static void func_02042ae4(int x, int y) {
    GFLNetInitData *ini = func_02042e84();

    if (ini != NULL) {
        ini->iconX = x;
        ini->iconY = y;
    }
}

BOOL func_02042b00(void) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet != NULL && (pNet->aNetInit.bNetType == 3 || pNet->aNetInit.bNetType == 4)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_02042b20(void) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet != NULL
        && (pNet->aNetInit.bNetType == GFL_NET_TYPE_WIFI || pNet->aNetInit.bNetType == GFL_NET_TYPE_WIFI_LOBBY)) {
        return TRUE;
    }
    return FALSE;
}

static void func_02042b40(void) {
    GFLNetInitData *ini = func_02042e84();
    GFLNetSys *pNet = func_02042e78();

    if (getLockIDStatus() == TRUE) {
        GFL_ASSERT(0);
        return;
    }
    if (pNet != NULL && ini->bNetType != 3) {
        BOOL isWifi = FALSE;

        if (pNet->aNetInit.bNetType == GFL_NET_TYPE_WIFI || pNet->aNetInit.bNetType == GFL_NET_TYPE_WIFI_LOBBY) {
            isWifi = TRUE;
        }
        func_0203e76c(pNet->aNetInit.iconX, pNet->aNetInit.iconY, isWifi, pNet->aNetInit.heapId);
    } else {
        func_0203e7dc();
    }
}


void func_02042ba8(BOOL top, HeapID heapId) {
    func_02042ae4(240, 0);
    func_0203e810(top, heapId);
    func_02042b40();
}

BOOL func_02042bc4(void) {
    BOOL ret = FALSE;

    if (func_0203ffc4() == 0) {
        ret = TRUE;
    }
    return ret;
}

BOOL func_02042bd8(NetHandle *handle) {
    if (handle->state == NET_HANDLE_STATE_NEGOTIATED) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_02042be8(NetHandle *handle, int command, u16 size, const void *data) {
    GFLNetInitData *ini = func_02042e84();

    return func_0203fafc(command, (u8 *)data, size, 0, func_020401dc(handle), 0xff, FALSE);
}

BOOL func_02042c18(NetHandle *handle, u8 sendID, u16 command, u32 size, const void *data, u32 a5, BOOL a6,
                   BOOL noCopy) {
    GFLNetInitData *ini = func_02042e84();
    u8 dest = 0xff;

    if (!func_02042bd8(handle) && command >= GFL_NET_CMD_BASE_COUNT) {
        return FALSE;
    }
    if (a6 && func_020400b8(command, func_020401dc(handle))) {
        return FALSE;
    }
    if (sendID != 0xff) {
        GFL_ASSERT(sendID < 7);
        dest = 1 << sendID;
    }
    return func_0203fafc(command, (u8 *)data, size, a5, func_020401dc(handle), dest, noCopy);
}

BOOL func_02042c9c(NetHandle *handle, int dest, u16 command, int size, const void *data, u32 a5, BOOL a6,
                   BOOL noCopy) {
    GFLNetInitData *ini = func_02042e84();

    if (!func_02042bd8(handle) && command >= GFL_NET_CMD_BASE_COUNT) {
        return FALSE;
    }
    if (a6 && func_020400b8(command, func_020401dc(handle))) {
        return FALSE;
    }
    return func_0203fafc(command, (u8 *)data, size, a5, func_020401dc(handle), dest, noCopy);
}

BOOL func_02042cfc(void) {
    return func_020400f0();
}

void func_02042d04(NetHandle *handle, u16 timing) {
    func_020405f8(handle, timing);
}

BOOL func_02042d0c(NetHandle *handle, u16 timing) {
    return func_02040654(handle, timing);
}

void func_02042d14(u8 gameCommandBase, u8 unused) {
    GFLNetSys *pNet = func_02042e78();

    func_02042e84()->gameCommandBase = gameCommandBase;
    func_02040b68(func_02040414(GFL_NET_NETID_SERVER), gameCommandBase);
}

u8 func_02042d34(void) {
    GFLNetInitData *ini = func_02042e84();

    if (ini != NULL) {
        return ini->gameCommandBase;
    }
    return 0;
}

int func_02042d48(void) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet->aNetInit.unk44 != NULL) {
        return pNet->aNetInit.unk44(pNet->devWork);
    }
    return 0;
}

int func_02042d64(void) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet->aNetInit.unk40 != NULL) {
        return pNet->aNetInit.unk40(pNet->devWork);
    }
    return 0;
}

u8 func_02042d80(void) {
    GFLNetSys *pNet = func_02042e78();

    return pNet->aNetInit.unk62;
}

void func_02042d8c(BOOL a0) {
    func_02040118(a0);
}

void *func_02042d94(void) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet != NULL) {
        return pNet->devWork;
    }
    return NULL;
}

void func_02042dac(void *work) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet != NULL) {
        pNet->devWork = work;
    }
}

int func_02042dc0(void) {
    GFLNetSys *pNet = func_02042e78();

    GFL_ASSERT(pNet->aNetInit.maxConnectNum <= GFL_NET_MACHINE_MAX);
    return pNet->aNetInit.maxConnectNum;
}

int func_02042de8(void) {
    GFLNetSys *pNet = func_02042e78();

    return pNet->aNetInit.maxSendSize;
}

u16 func_02042df4(void) {
    GFLNetSys *pNet = func_02042e78();

    GFL_ASSERT(pNet);
    return pNet->aNetInit.unk6C;
}

void func_02042e18(void) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet->aNetInit.unk30 != NULL) {
        pNet->aNetInit.unk30(pNet->devWork);
    }
}

void func_02042e30(int a0) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet->aNetInit.unk28 != NULL) {
        pNet->aNetInit.unk28(func_02040440(), a0, pNet->devWork);
    }
}

void func_02042e54(int a0) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet->aNetInit.unk24 != NULL) {
        pNet->aNetInit.unk24(func_02040440(), a0, pNet->devWork);
    }
}

GFLNetSys *func_02042e78(void) {
    return sNet.pNet;
}

GFLNetInitData *func_02042e84(void) {
    if (sNet.pNet != NULL) {
        return &sNet.pNet->aNetInit;
    }
    return NULL;
}

void func_02042e94(u8 a0) {
    func_02042410(a0);
}

void func_02042e9c(BOOL a0) {
    func_02042424(a0);
}

// Allocates 32-byte aligned memory, keeping the allocation just before it for func_02042ed0
void *allocConfigDSSoftwareFeature(HeapID heapId, u32 size, const char *file, u16 line) {
    void *mem = GFL_HeapAllocate(heapId, ((size + sizeof(void *) + 32) & ~31) + 32, TRUE, file, line);
    void **aligned = (void **)(((u32)mem + 32) & ~31);

    aligned[-1] = mem;
    return aligned;
}

void func_02042ed0(void *ptr) {
    if (ptr != NULL) {
        GFL_HeapFree(((void **)ptr)[-1]);
    }
}

void func_02042ee0(int x, int y) {
    sNet.bg1X = x;
    sNet.bg1Y = y;
}

void func_02042eec(int *x, int *y) {
    *x = sNet.bg1X;
    *y = sNet.bg1Y;
}

void func_02042efc(const GFLNetInitData *pNetInit) {
    sys_memcpy(pNetInit, &func_02042e78()->aNetInit, sizeof(GFLNetInitData));
}

void func_02042f10(void) {
    GFLNetSys *pNet = func_02042e78();

    if (pNet != NULL) {
        pNet->aNetInit.unk2C = NULL;
        pNet->aNetInit.unk1C = NULL;
    }
}

BOOL func_02042f24(void) {
    return func_02040078();
}

void func_02042f2c(int x, int y) {
    func_0203e874(x, y);
    func_02042b40();
}

void func_02042f40(void) {
    func_0203e874(0, 0);
    func_02042b40();
}

void func_02042f50(BOOL a0) {
    if (sNet.pNet != NULL) {
        if (a0 == TRUE) {
            sNet.pNet->unk352 = TRUE;
        } else {
            sNet.pNet->unk352 = FALSE;
        }
    }
}
