#include "types.h"
#include "dwc/dwc.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "save/wifi_list.h"
#include "system/game_data.h"
#include "system/net_define.h"

// Checks of a friend's data or friend key against the friends known, and adding a friend to the Friend Roster.
// The ROM has no string for the file, so the name is a guess.

// What addFriend does with the place
enum {
    FRIEND_ADD_NEW,
    // Replaces the data and the player's info, but keeps the name
    FRIEND_ADD_UPDATE,
    // Only the player's appearance
    FRIEND_ADD_VIEW,
};

static int compareFriendData(DWCFriendData *friendData, DWCUserData *userData, DWCFriendData *other);
static int findFriendData(DWCFriendData *friendData, int *index);
static void setFriendInfo(WifiList *wifiList, PlayerInfo *info, u32 index, u32 heapId, BOOL setName);
static void addFriend(GameData *gameData, PlayerInfo *info, const void *friendData, u32 index, u32 heapId, int mode);

static int compareFriendData(DWCFriendData *friendData, DWCUserData *userData, DWCFriendData *other) {
    if (DWC_IsEqualFriendData(friendData, other)) {
        return FRIEND_CHECK_SAME;
    }
    if (DWC_GetGsProfileId(userData, friendData) > 0 &&
        DWC_GetGsProfileId(userData, friendData) == DWC_GetGsProfileId(userData, other)) {
        return FRIEND_CHECK_SAME_PROFILE;
    }
    return FRIEND_CHECK_NEW;
}

static int findFriendData(DWCFriendData *friendData, int *index) {
    int i;
    DWCUserData *userData = (DWCUserData *)GFL_NetGetUserData();
    DWCFriendData *friends = (DWCFriendData *)GFL_NetGetFriendData();
    int count = func_02042d80();

    *index = -1;
    if (!DWC_IsValidFriendData(friendData)) {
        return FRIEND_CHECK_INVALID;
    }
    for (i = 0; i < count; i++) {
        if (DWC_IsEqualFriendData(friendData, &friends[i])) {
            *index = i;
            return FRIEND_CHECK_SAME;
        }
        if (DWC_GetGsProfileId(userData, friendData) > 0 &&
            DWC_GetGsProfileId(userData, friendData) == DWC_GetGsProfileId(userData, &friends[i])) {
            *index = i;
            return FRIEND_CHECK_SAME_PROFILE;
        }
        if (*index < 0 && !DWC_IsValidFriendData(&friends[i])) {
            *index = i;
        }
    }
    return FRIEND_CHECK_NEW;
}

int WifiList_FindFriendKey(WifiList *wifiList, u64 friendKey, int *index) {
    DWCUserData *userData = WifiList_GetMyUserData(wifiList);
    int i = 0;
    DWCFriendData *friends = WifiList_GetFriendData(wifiList, 0);
    int count = WifiList_CountFriends(wifiList);
    DWCFriendData token;

    if (!DWC_CheckFriendKey(userData, friendKey)) {
        return FRIEND_CHECK_INVALID;
    }
    DWC_CreateFriendKeyToken(&token, friendKey);
    if (DWC_GetGsProfileId(userData, &token) <= 0) {
        return FRIEND_CHECK_INVALID;
    }
    *index = -1;
    for (; i < count; i++) {
        if (DWC_GetGsProfileId(userData, &token) == DWC_GetGsProfileId(userData, &friends[i])) {
            *index = i;
            return FRIEND_CHECK_SAME;
        }
        if (*index < 0 && !DWC_IsValidFriendData(&friends[i])) {
            *index = i;
        }
    }
    return FRIEND_CHECK_NEW;
}

BOOL DWCFriend_UpdateFriendData(DWCFriendData *friendData) {
    BOOL isNew = FALSE;
    DWCFriendData *friends = (DWCFriendData *)GFL_NetGetFriendData();
    int index;
    int result;

    result = findFriendData(friendData, &index);
    if (result != FRIEND_CHECK_SAME) {
        if (result == FRIEND_CHECK_SAME_PROFILE) {
            MI_CpuCopy8(friendData, &friends[index], sizeof(DWCFriendData));
        } else if (result == FRIEND_CHECK_NEW) {
            isNew = TRUE;
        }
    }
    return isNew;
}

static void setFriendInfo(WifiList *wifiList, PlayerInfo *info, u32 index, u32 heapId, BOOL setName) {
    if (setName) {
        StrBuf *name = copyTrainerNameToNewStrbuf(info->name, heapId);

        WifiList_StoreFriendName(wifiList, index, name);
        GFL_StrBufFree(name);
    }
    WifiList_SetFriendParam(wifiList, index, WIFI_LIST_FRIEND_GENDER, getTrainerGender(info));
    WifiList_SetFriendParam(wifiList, index, WIFI_LIST_FRIEND_ID, getIDAsUInt(info));
    WifiList_SetFriendParam(wifiList, index, WIFI_LIST_FRIEND_PROFILE_ID, PlayerInfo_GetProfileId(info));
    WifiList_SetFriendParam(wifiList, index, WIFI_LIST_FRIEND_TRAINER_VIEW, PlayerInfo_GetUnionView(info));
}

static void addFriend(GameData *gameData, PlayerInfo *info, const void *friendData, u32 index, u32 heapId, int mode) {
    WifiList *wifiList = GameData_GetWifiList(gameData);
    DWCFriendData *dest = WifiList_GetFriendData(wifiList, index);

    if (mode != FRIEND_ADD_VIEW) {
        MI_CpuCopy8(friendData, dest, sizeof(DWCFriendData));
    }
    if (mode == FRIEND_ADD_NEW) {
        setFriendInfo(wifiList, info, index, heapId, TRUE);
    } else if (mode == FRIEND_ADD_UPDATE) {
        setFriendInfo(wifiList, info, index, heapId, FALSE);
    }
    WifiList_SetFriendParam(wifiList, index, WIFI_LIST_FRIEND_TRAINER_VIEW, PlayerInfo_GetUnionView(info));
}

BOOL WifiList_AddFriend(GameData *gameData, PlayerInfo *info, const DWCFriendData *friendData, HeapID heapId) {
    WifiList *wifiList = GameData_GetWifiList(gameData);
    int i;

    if (!DWC_IsValidFriendData(friendData)) {
        return FALSE;
    }
    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        DWCFriendData *other = WifiList_GetFriendData(wifiList, i);
        int result;

        if (!WifiList_IsFriendValid(wifiList, i)) {
            addFriend(gameData, info, friendData, i, heapId, FRIEND_ADD_NEW);
            return TRUE;
        }
        result = compareFriendData((DWCFriendData *)friendData, WifiList_GetMyUserData(wifiList), other);
        if (result == FRIEND_CHECK_SAME) {
            addFriend(gameData, info, friendData, i, heapId, FRIEND_ADD_VIEW);
            return FALSE;
        }
        if (result == FRIEND_CHECK_SAME_PROFILE) {
            addFriend(gameData, info, friendData, i, heapId, FRIEND_ADD_UPDATE);
            return FALSE;
        }
    }
    return FALSE;
}
