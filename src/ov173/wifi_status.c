#include "types.h"
#include "app/wifi_status.h"
#include "gfl/net.h"
#include "gfl/std.h"

// The status a player of the Wi-Fi Club shows the friends. The ROM has no name for this file: it is named after
// Black and White's wifi_status.c, and the functions' names are ours

u32 WIFI_STATUS_GetWifiStatus(const WIFI_STATUS *status) {
    return status->wifiStatus;
}

void WIFI_STATUS_SetWifiStatus(WIFI_STATUS *status, u8 wifiStatus) {
    status->wifiStatus = wifiStatus;
}

u32 WIFI_STATUS_GetGameMode(const WIFI_STATUS *status) {
    return status->gameMode;
}

void WIFI_STATUS_SetGameMode(WIFI_STATUS *status, u8 gameMode) {
    status->gameMode = gameMode;
}

u8 WIFI_STATUS_GetVChatStatus(const WIFI_STATUS *status) {
    return status->vchatStatus;
}

void WIFI_STATUS_SetVChatStatus(WIFI_STATUS *status, u8 vchatStatus) {
    status->vchatStatus = vchatStatus;
}

BOOL WIFI_STATUS_IsVChatMac(const WIFI_STATUS *my, const WIFI_STATUS *friendStatus) {
    if (GFL_STD_MemCmp(my->myMac, friendStatus->vchatMac, WIFI_STATUS_MAC_SIZE) == 0) {
        return TRUE;
    }
    return FALSE;
}

void WIFI_STATUS_SetVChatMac(WIFI_STATUS *my, const WIFI_STATUS *friendStatus) {
    my->callCounter++;
    sys_memcpy(friendStatus->myMac, my->vchatMac, WIFI_STATUS_MAC_SIZE);
}

void WIFI_STATUS_ResetVChatMac(WIFI_STATUS *my) {
    u8 mac[WIFI_STATUS_MAC_SIZE] = { 0, 0, 0, 0, 0, 0 };

    sys_memcpy(mac, my->vchatMac, WIFI_STATUS_MAC_SIZE);
}

void WIFI_STATUS_SetMyMac(WIFI_STATUS *my) {
    func_0207c33c(my->myMac);
}

BOOL WIFI_STATUS_GetUseVChat(const WIFI_STATUS *status) {
    return status->useVChat;
}

void WIFI_STATUS_SetUseVChat(WIFI_STATUS *status, u8 useVChat) {
    status->useVChat = useVChat;
}

u32 WIFI_STATUS_GetCallCounter(const WIFI_STATUS *status) {
    return status->callCounter;
}
