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

#define SAVE_BLOCK_WIFI_LIST 0x1e
#define SAVE_BLOCK_PAL_PAD 0x1f

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
    sys_memcpy(src, dest, sizeof(WifiList));
}

void initWifi(WifiList *wifiList) {
    sys_memset(wifiList, 0, sizeof(WifiList));
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

DWCUserData *func_02009f7c(WifiList *wifiList) {
    return &wifiList->myUserData;
}

u32 func_02009f80(WifiList *wifiList, u32 index, u32 param) {
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

void func_0200a034(WifiList *wifiList, u32 index, u32 param, u32 value) {
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

DWCFriendData *func_0200a0e4(WifiList *wifiList, u32 index) {
    return &wifiList->friendData[index];
}

u16 *func_0200a0f0(WifiList *wifiList, u32 index) {
    return wifiList->friends[index].name;
}

void func_0200a100(WifiList *wifiList, u32 index, StrBuf *dest) {
    GFL_StrBufLoadFixedString(dest, wifiList->friends[index].name, 16);
}

void func_0200a11c(WifiList *wifiList, u32 index, const StrBuf *src) {
    GFL_StrBufStoreString(src, wifiList->friends[index].name, 16);
}

BOOL func_0200a138(WifiList *wifiList, u32 index) {
    if (index >= WIFI_LIST_FRIEND_MAX) {
        return FALSE;
    }
    return DWC_IsValidFriendData(&wifiList->friendData[index]);
}

u32 func_0200a150(WifiList *wifiList) {
    s32 i;
    u32 count = 0;

    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        if (func_0200a138(wifiList, i)) {
            count++;
        }
    }
    return count;
}

u32 func_0200a170(WifiList *wifiList) {
    u32 last = 0;
    s32 i;

    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        if (func_0200a138(wifiList, i)) {
            last = i + 1;
        }
    }
    return last;
}

void func_0200a190(WifiList *wifiList, u32 index) {
    s32 i;
    s32 last = WIFI_LIST_FRIEND_MAX - 1;

    if (index >= WIFI_LIST_FRIEND_MAX) {
        return;
    }
    for (i = index; i < last; i++) {
        sys_memcpy(&wifiList->friends[i + 1], &wifiList->friends[i], sizeof(WifiListFriend));
        sys_memcpy(&wifiList->friendData[i + 1], &wifiList->friendData[i], sizeof(DWCFriendData));
    }
    sys_memset(&wifiList->friends[last], 0, sizeof(WifiListFriend));
    sys_memset(&wifiList->friendData[last], 0, sizeof(DWCFriendData));
}

static void moveFriend(WifiList *wifiList, u32 dest, u32 src) {
    sys_memcpy(&wifiList->friends[src], &wifiList->friends[dest], sizeof(WifiListFriend));
    sys_memcpy(&wifiList->friendData[src], &wifiList->friendData[dest], sizeof(DWCFriendData));
    sys_memset(&wifiList->friends[src], 0, sizeof(WifiListFriend));
    sys_memset(&wifiList->friendData[src], 0, sizeof(DWCFriendData));
}

void func_0200a258(WifiList *wifiList) {
    s32 i;
    s32 empty = -1;

    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        if (func_0200a138(wifiList, i)) {
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

void func_0200a29c(WifiList *wifiList, u32 index) {
    RTCDate date;

    RTC_GetCachedDate(&date);
    wifiList->friends[index].lastYear = date.year + 2000;
    wifiList->friends[index].lastMonth = date.month;
    wifiList->friends[index].lastDay = date.day;
}

void func_0200a2d4(WifiList *wifiList, s32 index, u32 wins, u32 losses, u32 draws) {
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
    func_0200a29c(wifiList, index);
}

void func_0200a340(WifiList *wifiList, s32 srcIndex, s32 destIndex) {
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
    sys_memset(&wifiList->friends[srcIndex], 0, sizeof(WifiListFriend));
}

void func_0200a3c4(const DWCUserData *userData, DWCFriendData *token) {
    DWC_CreateExchangeToken(userData, token);
}

void func_0200a3cc(WifiList *wifiList, DWCFriendData *token) {
    DWC_CreateExchangeToken(func_02009f7c(wifiList), token);
}

s32 WifiList_GetMyGSID(WifiList *wifiList) {
    DWCUserData *userData = func_02009f7c(wifiList);
    DWCFriendData token;

    func_0200a3cc(wifiList, &token);
    return DWC_GetGsProfileId(userData, &token);
}

BOOL func_0200a400(WifiList *wifiList) {
    DWCUserData *userData = func_02009f7c(wifiList);

    if (DWC_CheckHasProfile(userData) && DWC_CheckValidConsole(userData)) {
        return TRUE;
    }
    return FALSE;
}

WifiList *func_0200a424(SaveControl *save) {
    WifiList *wifiList = SaveControl_GetBlockPtr(save, SAVE_BLOCK_WIFI_LIST);
    // The original fetches the first name and drops it
    func_0200a0f0(wifiList, 0);
    return wifiList;
}

BOOL func_0200a438(WifiList *wifiList, PlayerInfo *info, u32 *friendIndex) {
    u32 id = getIDAsUInt(info);
    u32 gender = getTrainerGender(info);
    return func_0200a46c(wifiList, GetPlayerName(info), id, gender, friendIndex);
}

BOOL func_0200a46c(WifiList *wifiList, const u16 *name, u32 id, u32 gender, u32 *friendIndex) {
    u8 i;

    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        if (func_0200a138(wifiList, i) == TRUE) {
            u32 friendId = func_02009f80(wifiList, i, WIFI_LIST_FRIEND_ID);
            if (friendId == id) {
                u32 friendGender = func_02009f80(wifiList, i, WIFI_LIST_FRIEND_GENDER);
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

u32 func_0200a4b8(void) {
    return sizeof(PalPadFriendList);
}

void func_0200a4c0(PalPadFriendList *list) {
    sys_memset(list, 0, func_0200a4b8());
}

s32 func_0200a4d4(PalPadFriendList *list, u32 id) {
    s32 i;

    for (i = 0; i < PAL_PAD_PLAYER_MAX; i++) {
        if (func_02008bfc(&list->players[i]) != 0 && id == getIDAsUInt(&list->players[i])) {
            return i;
        }
    }
    return -1;
}

void func_0200a504(PalPadFriendList *list, PlayerInfo *info) {
    u32 id = getIDAsUInt(info);
    u32 size = PlayerInfo_GetSize();
    s32 index;

    if (id == 0) {
        return;
    }
    index = func_0200a4d4(list, id);
    if (index != -1) {
        func_02008bf8(&list->players[index], func_02008bf4(info));
        return;
    }
    sys_memcpy_ex(list, &list->players[1], (PAL_PAD_PLAYER_MAX - 1) * size);
    sys_memcpy(info, list, size);
}

u32 func_0200a55c(PalPadFriendList *list, u32 index) {
    if (index >= PAL_PAD_PLAYER_MAX) {
        index = 0;
    }
    return getIDAsUInt(&list->players[index]);
}

PlayerInfo *func_0200a570(PalPadFriendList *list, u32 index) {
    if (index >= PAL_PAD_PLAYER_MAX) {
        return NULL;
    }
    if (func_0200a55c(list, index) == 0) {
        return NULL;
    }
    return &list->players[index];
}

BOOL func_0200a590(PalPadFriendList *list, u32 id) {
    if (func_0200a4d4(list, id) != -1) {
        return TRUE;
    }
    return FALSE;
}

u32 func_0200a5a8(PalPadFriendList *list) {
    s32 i;

    for (i = 0; i < PAL_PAD_PLAYER_MAX; i++) {
        if (func_02008bfc(&list->players[i]) == 0) {
            break;
        }
    }
    return i;
}

u32 func_0200a5c4(PalPadFriendList *list) {
    return list->tradeCount;
}

void func_0200a5cc(PalPadFriendList *list) {
    list->tradeCount++;
    if (list->tradeCount > PAL_PAD_TRADE_COUNT_MAX) {
        list->tradeCount = PAL_PAD_TRADE_COUNT_MAX;
    }
}

PalPadFriendList *getPalPadFriendListAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_PAL_PAD);
}
