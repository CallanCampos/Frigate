#include "types.h"
#include "dwc/dwc.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/rtc.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/wifi_list.h"

// The Pal Pad: the player's DWC account with the friends' data and records, block 0x1e, and the list of the
// players traded with, block 0x1f. The file's name is the ROM's; the function names from swan are marked in
// wifi_list.h, and moveFriend is ours.

// initWifi unloads the field's overlays 12 and 27 to make room for 29 and dwc_rap's 11 while the user data is made
#define OVERLAY_DWC_RAP OVERLAY_ID(11)

static void moveFriend(WifiList *wifiList, u32 dest, u32 src);

u32 getSizeofWifiData(void) {
    return sizeof(WifiList);
}

WifiList *populateWifiList(u32 heapId) {
    WifiList *wifiList = GFL_HeapAllocate((HeapID)heapId, sizeof(WifiList), TRUE, "wifilist.c", 57);
    initWifi(wifiList);
    return wifiList;
}

void copyWifilist(const WifiList *src, WifiList *dest) {
    MI_CpuCopy8(src, dest, sizeof(WifiList));
}

void initWifi(WifiList *wifiList) {
    MI_CpuFill8(wifiList, 0, sizeof(WifiList));
    GFL_OvlUnload(OVERLAY_ID(12));
    GFL_OvlUnload(OVERLAY_ID(27));
    GFL_OvlLoad(OVERLAY_ID(29));
    GFL_OvlLoad(OVERLAY_DWC_RAP);
    DWCRapCommon_ResetUserData(&wifiList->myUserData);
    GFL_OvlUnload(OVERLAY_DWC_RAP);
    GFL_OvlUnload(OVERLAY_ID(29));
    GFL_OvlLoad(OVERLAY_ID(12));
    GFL_OvlLoad(OVERLAY_ID(27));
}

DWCUserData *WifiList_GetMyUserData(WifiList *wifiList) {
    return &wifiList->myUserData;
}

u32 WifiList_GetFriendParam(WifiList *wifiList, u32 index, u32 param) {
    u32 value = 0;

    switch (param) {
    case WIFI_LIST_FRIEND_ID:
        value = wifiList->friends[index].id;
        break;
    case WIFI_LIST_FRIEND_PROFILE_ID:
        value = wifiList->friends[index].profileId;
        break;
    case WIFI_LIST_FRIEND_WINS:
        value = wifiList->friends[index].wins;
        break;
    case WIFI_LIST_FRIEND_LOSSES:
        value = wifiList->friends[index].losses;
        break;
    case WIFI_LIST_FRIEND_DRAWS:
        value = wifiList->friends[index].draws;
        break;
    case WIFI_LIST_FRIEND_LAST_YEAR:
        value = wifiList->friends[index].lastYear;
        break;
    case WIFI_LIST_FRIEND_LAST_MONTH:
        value = wifiList->friends[index].lastMonth;
        break;
    case WIFI_LIST_FRIEND_LAST_DAY:
        value = wifiList->friends[index].lastDay;
        break;
    case WIFI_LIST_FRIEND_GENDER:
        value = wifiList->friends[index].gender;
        break;
    case WIFI_LIST_FRIEND_TRAINER_VIEW:
        value = wifiList->friends[index].trainerView;
        break;
    }
    return value;
}

void WifiList_SetFriendParam(WifiList *wifiList, u32 index, u32 param, u32 value) {
    switch (param) {
    case WIFI_LIST_FRIEND_ID:
        wifiList->friends[index].id = value;
        break;
    case WIFI_LIST_FRIEND_PROFILE_ID:
        wifiList->friends[index].profileId = value;
        break;
    case WIFI_LIST_FRIEND_LAST_YEAR:
        wifiList->friends[index].lastYear = value;
        break;
    case WIFI_LIST_FRIEND_LAST_MONTH:
        wifiList->friends[index].lastMonth = value;
        break;
    case WIFI_LIST_FRIEND_LAST_DAY:
        wifiList->friends[index].lastDay = value;
        break;
    case WIFI_LIST_FRIEND_GENDER:
        wifiList->friends[index].gender = value;
        break;
    case WIFI_LIST_FRIEND_TRAINER_VIEW:
        wifiList->friends[index].trainerView = value;
        break;
    }
}

DWCFriendData *WifiList_GetFriendData(WifiList *wifiList, u32 index) {
    return &wifiList->friendData[index];
}

u16 *WifiList_GetFriendName(WifiList *wifiList, u32 index) {
    return wifiList->friends[index].name;
}

void WifiList_LoadFriendName(WifiList *wifiList, u32 index, StrBuf *dest) {
    GFL_StrBufLoadFixedString(dest, wifiList->friends[index].name, 16);
}

void WifiList_StoreFriendName(WifiList *wifiList, u32 index, const StrBuf *src) {
    GFL_StrBufStoreString(src, wifiList->friends[index].name, 16);
}

BOOL WifiList_IsFriendValid(WifiList *wifiList, u32 index) {
    if (index >= WIFI_LIST_FRIEND_MAX) {
        return FALSE;
    }
    return DWC_IsValidFriendData(&wifiList->friendData[index]);
}

u32 WifiList_CountFriends(WifiList *wifiList) {
    s32 i;
    u32 count = 0;

    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        if (WifiList_IsFriendValid(wifiList, i)) {
            count++;
        }
    }
    return count;
}

u32 WifiList_GetFriendSlotCount(WifiList *wifiList) {
    u32 last = 0;
    s32 i;

    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        if (WifiList_IsFriendValid(wifiList, i)) {
            last = i + 1;
        }
    }
    return last;
}

void WifiList_RemoveFriend(WifiList *wifiList, u32 index) {
    int i;
    s32 last = WIFI_LIST_FRIEND_MAX - 1;

    if (index >= WIFI_LIST_FRIEND_MAX) {
        return;
    }
    for (i = index; i < last; i++) {
        MI_CpuCopy8(&wifiList->friends[i + 1], &wifiList->friends[i], sizeof(WifiListFriend));
        MI_CpuCopy8(&wifiList->friendData[i + 1], &wifiList->friendData[i], sizeof(DWCFriendData));
    }
    MI_CpuFill8(&wifiList->friends[last], 0, sizeof(WifiListFriend));
    MI_CpuFill8(&wifiList->friendData[last], 0, sizeof(DWCFriendData));
}

static void moveFriend(WifiList *wifiList, u32 dest, u32 src) {
    MI_CpuCopy8(&wifiList->friends[src], &wifiList->friends[dest], sizeof(WifiListFriend));
    MI_CpuCopy8(&wifiList->friendData[src], &wifiList->friendData[dest], sizeof(DWCFriendData));
    MI_CpuFill8(&wifiList->friends[src], 0, sizeof(WifiListFriend));
    MI_CpuFill8(&wifiList->friendData[src], 0, sizeof(DWCFriendData));
}

void WifiList_CompactFriends(WifiList *wifiList) {
    s32 i;
    s32 empty = -1;

    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        if (WifiList_IsFriendValid(wifiList, i)) {
            if (empty != -1) {
                moveFriend(wifiList, empty, i);
                // Starts over from the first slot; the original writes empty from i
                i = -1;
                empty = i;
            }
        } else if (empty == -1) {
            empty = i;
        }
    }
}

void WifiList_SetFriendLastDateToNow(WifiList *wifiList, u32 index) {
    RTCDate date;

    RTC_GetCachedDate(&date);
    wifiList->friends[index].lastYear = date.year + 2000;
    wifiList->friends[index].lastMonth = date.month;
    wifiList->friends[index].lastDay = date.day;
}

void WifiList_AddFriendRecord(WifiList *wifiList, s32 index, u32 wins, u32 losses, u32 draws) {
    if (index < 0 || index >= WIFI_LIST_FRIEND_MAX) {
        return;
    }
    wifiList->friends[index].wins += wins;
    if (wifiList->friends[index].wins > WIFI_LIST_RECORD_MAX) {
        wifiList->friends[index].wins = WIFI_LIST_RECORD_MAX;
    }
    wifiList->friends[index].losses += losses;
    if (wifiList->friends[index].losses > WIFI_LIST_RECORD_MAX) {
        wifiList->friends[index].losses = WIFI_LIST_RECORD_MAX;
    }
    wifiList->friends[index].draws += draws;
    if (wifiList->friends[index].draws > WIFI_LIST_RECORD_MAX) {
        wifiList->friends[index].draws = WIFI_LIST_RECORD_MAX;
    }
    WifiList_SetFriendLastDateToNow(wifiList, index);
}

void WifiList_MergeFriendRecord(WifiList *wifiList, s32 srcIndex, s32 destIndex) {
    if (destIndex < 0 || destIndex >= WIFI_LIST_FRIEND_MAX || srcIndex < 0 || srcIndex >= WIFI_LIST_FRIEND_MAX) {
        return;
    }
    wifiList->friends[destIndex].wins += wifiList->friends[srcIndex].wins;
    if (wifiList->friends[destIndex].wins > WIFI_LIST_RECORD_MAX) {
        wifiList->friends[destIndex].wins = WIFI_LIST_RECORD_MAX;
    }
    wifiList->friends[destIndex].losses += wifiList->friends[srcIndex].losses;
    if (wifiList->friends[destIndex].losses > WIFI_LIST_RECORD_MAX) {
        wifiList->friends[destIndex].losses = WIFI_LIST_RECORD_MAX;
    }
    wifiList->friends[destIndex].draws += wifiList->friends[srcIndex].draws;
    if (wifiList->friends[destIndex].draws > WIFI_LIST_RECORD_MAX) {
        wifiList->friends[destIndex].draws = WIFI_LIST_RECORD_MAX;
    }
    MI_CpuFill8(&wifiList->friends[srcIndex], 0, sizeof(WifiListFriend));
}

void WifiList_CreateExchangeTokenFromUserData(const DWCUserData *userData, DWCFriendData *token) {
    DWC_CreateExchangeToken(userData, token);
}

void WifiList_CreateExchangeToken(WifiList *wifiList, DWCFriendData *token) {
    DWC_CreateExchangeToken(WifiList_GetMyUserData(wifiList), token);
}

s32 WifiList_GetMyGSID(WifiList *wifiList) {
    DWCUserData *userData = WifiList_GetMyUserData(wifiList);
    DWCFriendData token;

    WifiList_CreateExchangeToken(wifiList, &token);
    return DWC_GetGsProfileId(userData, &token);
}

BOOL WifiList_HasValidProfile(WifiList *wifiList) {
    DWCUserData *userData = WifiList_GetMyUserData(wifiList);

    if (DWC_CheckHasProfile(userData) && DWC_CheckValidConsole(userData)) {
        return TRUE;
    }
    return FALSE;
}

WifiList *SaveControl_GetWifiList(SaveControl *save) {
    WifiList *wifiList = SaveControl_GetBlockPtr(save, SAVE_BLOCK_WIFI_LIST);
    // The original fetches the first name and drops it
    WifiList_GetFriendName(wifiList, 0);
    return wifiList;
}

BOOL WifiList_FindFriendByPlayerInfo(WifiList *wifiList, PlayerInfo *info, u32 *friendIndex) {
    u32 id = getIDAsUInt(info);
    u32 gender = getTrainerGender(info);
    return WifiList_FindFriend(wifiList, GetPlayerName(info), id, gender, friendIndex);
}

BOOL WifiList_FindFriend(WifiList *wifiList, const u16 *name, u32 id, u32 gender, u32 *friendIndex) {
    u8 i;

    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        if (WifiList_IsFriendValid(wifiList, i) == TRUE) {
            u32 friendId = WifiList_GetFriendParam(wifiList, i, WIFI_LIST_FRIEND_ID);
            if (friendId == id) {
                u32 friendGender = WifiList_GetFriendParam(wifiList, i, WIFI_LIST_FRIEND_GENDER);
                if (friendGender == gender) {
                    if (friendIndex != NULL) {
                        *friendIndex = i;
                    }
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

u32 PalPadFriendList_GetSize(void) {
    return sizeof(PalPadFriendList);
}

void PalPadFriendList_Init(PalPadFriendList *list) {
    MI_CpuFill8(list, 0, PalPadFriendList_GetSize());
}

s32 PalPadFriendList_FindPlayer(PalPadFriendList *list, u32 id) {
    s32 i;

    for (i = 0; i < PAL_PAD_PLAYER_MAX; i++) {
        if (PlayerInfo_GetVersion(&list->players[i]) != 0 && id == getIDAsUInt(&list->players[i])) {
            return i;
        }
    }
    return -1;
}

void PalPadFriendList_AddPlayer(PalPadFriendList *list, PlayerInfo *info) {
    u32 id = getIDAsUInt(info);
    u32 size = PlayerInfo_GetSize();
    s32 index;

    if (id == 0) {
        return;
    }
    index = PalPadFriendList_FindPlayer(list, id);
    if (index != -1) {
        PlayerInfo_SetUnionView(&list->players[index], PlayerInfo_GetUnionView(info));
        return;
    }
    MI_CpuMove(list, &list->players[1], (PAL_PAD_PLAYER_MAX - 1) * size);
    MI_CpuCopy8(info, list, size);
}

u32 PalPadFriendList_GetPlayerId(PalPadFriendList *list, u32 index) {
    if (index >= PAL_PAD_PLAYER_MAX) {
        index = 0;
    }
    return getIDAsUInt(&list->players[index]);
}

PlayerInfo *PalPadFriendList_GetPlayer(PalPadFriendList *list, u32 index) {
    if (index >= PAL_PAD_PLAYER_MAX) {
        return NULL;
    }
    if (PalPadFriendList_GetPlayerId(list, index) == 0) {
        return NULL;
    }
    return &list->players[index];
}

BOOL PalPadFriendList_HasPlayer(PalPadFriendList *list, u32 id) {
    if (PalPadFriendList_FindPlayer(list, id) != -1) {
        return TRUE;
    }
    return FALSE;
}

u32 PalPadFriendList_CountPlayers(PalPadFriendList *list) {
    s32 i;

    for (i = 0; i < PAL_PAD_PLAYER_MAX; i++) {
        if (PlayerInfo_GetVersion(&list->players[i]) == 0) {
            break;
        }
    }
    return i;
}

u32 PalPadFriendList_GetTradeCount(PalPadFriendList *list) {
    return list->tradeCount;
}

void PalPadFriendList_IncrementTradeCount(PalPadFriendList *list) {
    list->tradeCount++;
    if (list->tradeCount > PAL_PAD_TRADE_COUNT_MAX) {
        list->tradeCount = PAL_PAD_TRADE_COUNT_MAX;
    }
}

PalPadFriendList *getPalPadFriendListAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_PAL_PAD);
}
