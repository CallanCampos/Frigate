#ifndef POKEBW2_APP_WIFI_STATUS_H
#define POKEBW2_APP_WIFI_STATUS_H

// What a player of the Wi-Fi Club tells the friends through the friend list's status data (overlay 173).
// event_wificlub.c allocates the player's own and keeps it over the Wi-Fi Club's applications. The ROM has no name for
// the file: the names follow Black and White's wifi_status.c as far as known, and which of the two bytes is the status
// and which the game mode is a guess

#include "types.h"

#define WIFI_STATUS_MAC_SIZE 6

typedef struct {
    // The GameSpy profile ID
    s32 profileId;
    u32 trainerId;
    // The MAC address of the friend the player is in a voice chat with
    u8 vchatMac[WIFI_STATUS_MAC_SIZE];
    u8 myMac[WIFI_STATUS_MAC_SIZE];
    u8 trainerView;
    u8 version;
    u8 language;
    u8 gameMode;
    u8 wifiStatus;
    u8 country;
    u8 province;
    u8 unk1B_0 : 1;
    u8 vchatStatus : 1;
    u8 gender : 2;
    u8 useVChat : 1;
    u8 unk1B_5 : 3;
    // How many times the player has set a voice chat partner
    u8 callCounter;
    u8 unk1D[3];
} WIFI_STATUS;

u32 WIFI_STATUS_GetWifiStatus(const WIFI_STATUS *status);
void WIFI_STATUS_SetWifiStatus(WIFI_STATUS *status, u8 wifiStatus);
u32 WIFI_STATUS_GetGameMode(const WIFI_STATUS *status);
void WIFI_STATUS_SetGameMode(WIFI_STATUS *status, u8 gameMode);
u8 WIFI_STATUS_GetVChatStatus(const WIFI_STATUS *status);
void WIFI_STATUS_SetVChatStatus(WIFI_STATUS *status, u8 vchatStatus);
// Whether the friend is in a voice chat with the player
BOOL WIFI_STATUS_IsVChatMac(const WIFI_STATUS *my, const WIFI_STATUS *friendStatus);
void WIFI_STATUS_SetVChatMac(WIFI_STATUS *my, const WIFI_STATUS *friendStatus);
void WIFI_STATUS_ResetVChatMac(WIFI_STATUS *my);
void WIFI_STATUS_SetMyMac(WIFI_STATUS *my);
BOOL WIFI_STATUS_GetUseVChat(const WIFI_STATUS *status);
void WIFI_STATUS_SetUseVChat(WIFI_STATUS *status, u8 useVChat);
u32 WIFI_STATUS_GetCallCounter(const WIFI_STATUS *status);

#endif // POKEBW2_APP_WIFI_STATUS_H
