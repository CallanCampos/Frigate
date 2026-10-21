#include "system/ctvt_beacon.h"
#include "types.h"
#include "app/comm_tvt/ctvt_comm.h"
#include "save/player_info.h"
#include "save/wifi_list.h"

// What the main module reads of an Xtransceiver's beacon, as it finds the machines calling this one. The file's
// name is a guess

BOOL CtvtBeacon_IsFriendInvite(CtvtCommBeacon *beacon, WifiList *wifiList, const u8 *mac) {
    u8 i;
    u8 j;

    if (wifiList == NULL) {
        return FALSE;
    }
    for (i = 0; i < 3; i++) {
        BOOL match = TRUE;

        for (j = 0; j < 6; j++) {
            if (beacon->inviteMacs[i][j] != mac[j]) {
                match = FALSE;
                break;
            }
        }
        if (match == TRUE) {
            return WifiList_FindFriendByPlayerInfo(wifiList, &beacon->player, NULL);
        }
    }
    return FALSE;
}

u16 *CtvtBeacon_GetPlayerName(CtvtCommBeacon *beacon) {
    return GetPlayerName(&beacon->player);
}

u16 CtvtBeacon_GetTrainerID(CtvtCommBeacon *beacon) {
    return getTrainerID(&beacon->player);
}
