#include "types.h"
#include "app/wifi_p2pmatch_net.h"
#include "app/wifi_p2pmatch.h"
#include "gfl/net_command.h"
#include "struct_decls.h"

// The command table of the Wi-Fi Club's match board. The ROM has no name for this file, and the name is a guess; the
// handlers that do something are in wifi_p2pmatch.c

enum {
    WIFIP2PMATCH_NET_CMD_UNUSED0,
    WIFIP2PMATCH_NET_CMD_UNUSED1,
    WIFIP2PMATCH_NET_CMD_UNUSED2,
    WIFIP2PMATCH_NET_CMD_3,
    WIFIP2PMATCH_NET_CMD_4,
    WIFIP2PMATCH_NET_CMD_5,
    WIFIP2PMATCH_NET_CMD_6,
    WIFIP2PMATCH_NET_CMD_7,
    WIFIP2PMATCH_NET_CMD_8,
    WIFIP2PMATCH_NET_CMD_NUM,
};

static const NetCommand *WifiP2PMatchNet_CommandTableGet(void);
static int WifiP2PMatchNet_CommandNumGet(void);
static void WifiP2PMatchNet_RecvUnused0(int netId, int size, const void *data, void *work, NetHandle *handle);
static void WifiP2PMatchNet_RecvUnused1(int netId, int size, const void *data, void *work, NetHandle *handle);
static void WifiP2PMatchNet_RecvUnused2(int netId, int size, const void *data, void *work, NetHandle *handle);

static const NetCommand sWifiP2PMatchNetCommands[WIFIP2PMATCH_NET_CMD_NUM] = {
    { WifiP2PMatchNet_RecvUnused0, NULL },
    { WifiP2PMatchNet_RecvUnused1, NULL },
    { WifiP2PMatchNet_RecvUnused2, NULL },
    { WifiP2PMatch_RecvConnectData, NULL },
    { WifiP2PMatch_RecvPlayerInfo, NULL },
    { WifiP2PMatch_RecvNextSeq, NULL },
    { WifiP2PMatch_RecvRegulation, NULL },
    { WifiP2PMatch_RecvParty, WifiP2PMatch_GetPartyRecvBuffer },
    { WifiP2PMatch_RecvCancel, NULL },
};

static const NetCommand *WifiP2PMatchNet_CommandTableGet(void) {
    return sWifiP2PMatchNetCommands;
}

static int WifiP2PMatchNet_CommandNumGet(void) {
    return WIFIP2PMATCH_NET_CMD_NUM;
}

void WifiP2PMatchNet_CommandInit(void *work) {
    const NetCommand *commands = WifiP2PMatchNet_CommandTableGet();
    int num = WifiP2PMatchNet_CommandNumGet();

    func_02040c20(WIFIP2PMATCH_NET_CMD_BASE, commands, num, work);
}

void WifiP2PMatchNet_CommandExit(void *work) {
    func_02040c64(WIFIP2PMATCH_NET_CMD_BASE);
}

static void WifiP2PMatchNet_RecvUnused0(int netId, int size, const void *data, void *work, NetHandle *handle) {
    if (work == NULL) {
        return;
    }
}

static void WifiP2PMatchNet_RecvUnused1(int netId, int size, const void *data, void *work, NetHandle *handle) {
    if (work == NULL) {
        return;
    }
}

static void WifiP2PMatchNet_RecvUnused2(int netId, int size, const void *data, void *work, NetHandle *handle) {
    if (work == NULL) {
        return;
    }
}
