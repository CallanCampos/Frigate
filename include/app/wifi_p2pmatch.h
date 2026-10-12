#ifndef POKEBW2_APP_WIFI_P2PMATCH_H
#define POKEBW2_APP_WIFI_P2PMATCH_H

// The Wi-Fi Club's match board (overlay 173). The ROM has no name for the file: it follows the older games'
// wifi_p2pmatch.c. This header only has what the overlay's other files need of it; its proc table,
// WIFICLUB_PROC_FUNCTIONS, is in app/wificlub.h

#include "types.h"
#include "struct_decls.h"

// The handlers of the match board's commands (wifi_p2pmatch_net.c has the table), with the work as the match board's
void WifiP2PMatch_RecvPlayerInfo(int netId, int size, const void *data, void *work, NetHandle *handle);
void WifiP2PMatch_RecvParty(int netId, int size, const void *data, void *work, NetHandle *handle);
void WifiP2PMatch_RecvCancel(int netId, int size, const void *data, void *work, NetHandle *handle);
// Where the data of WifiP2PMatch_RecvParty's command is received
void *WifiP2PMatch_GetPartyRecvBuffer(int netId, void *work, int size);
void WifiP2PMatch_RecvNextSeq(int netId, int size, const void *data, void *work, NetHandle *handle);
void WifiP2PMatch_RecvRegulation(int netId, int size, const void *data, void *work, NetHandle *handle);
void WifiP2PMatch_RecvConnectData(int netId, int size, const void *data, void *work, NetHandle *handle);

// A constant of wifi_p2pmatch.c that no code of the ROM reads. It lies among the file's data that code refers to,
// which MWCC does for a const global only when some code reads it before its definition, even code it never emits:
// this accessor is reconstructed for that
extern const u16 WIFIP2PMATCH_UNK_100;

static inline u16 WifiP2PMatch_GetUnk100(void) {
    return WIFIP2PMATCH_UNK_100;
}

#endif // POKEBW2_APP_WIFI_P2PMATCH_H
