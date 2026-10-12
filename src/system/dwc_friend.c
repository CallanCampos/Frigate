#include "types.h"
#include "system/net_define.h"
#include "dwc/dwc.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "save/wifi_list.h"
#include "system/game_data.h"

// Checks of a friend's data or friend key against the friends known, and adding a friend to the Friend Roster.
// The ROM has no string for the file, so the name is a guess.

// What addFriend does with the place
#define FRIEND_ADD_NEW 0
// Replaces the data and the player's info, but keeps the name
#define FRIEND_ADD_UPDATE 1
// Only the player's appearance
#define FRIEND_ADD_VIEW 2

static int compareFriendData(DWCFriendData *friendData, DWCUserData *userData, DWCFriendData *other);
static int findFriendData(DWCFriendData *friendData, int *index);
static void setFriendInfo(WifiList *wifiList, PlayerInfo *info, u32 index, u32 heapId, BOOL setName);
static void addFriend(GameData *gameData, PlayerInfo *info, const void *friendData, u32 index, u32 heapId,
                      int mode);

static int compareFriendData(DWCFriendData *friendData, DWCUserData *userData, DWCFriendData *other) {
    if (func_02057fd4(friendData, other)) {
        return FRIEND_CHECK_SAME;
    }
    if (DWC_GetGsProfileId(userData, friendData) > 0
        && DWC_GetGsProfileId(userData, friendData) == DWC_GetGsProfileId(userData, other)) {
        return FRIEND_CHECK_SAME_PROFILE;
    }
    return FRIEND_CHECK_NEW;
}

static int findFriendData(DWCFriendData *friendData, int *index) {
    int i;
    DWCUserData *userData = (DWCUserData *)func_02042d48();
    DWCFriendData *friends = (DWCFriendData *)func_02042d64();
    int count = func_02042d80();

    *index = -1;
    if (!DWC_IsValidFriendData(friendData)) {
        return FRIEND_CHECK_INVALID;
    }
    for (i = 0; i < count; i++) {
        if (func_02057fd4(friendData, &friends[i])) {
            *index = i;
            return FRIEND_CHECK_SAME;
        }
        if (DWC_GetGsProfileId(userData, friendData) > 0
            && DWC_GetGsProfileId(userData, friendData) == DWC_GetGsProfileId(userData, &friends[i])) {
            *index = i;
            return FRIEND_CHECK_SAME_PROFILE;
        }
        if (*index < 0 && !DWC_IsValidFriendData(&friends[i])) {
            *index = i;
        }
    }
    return FRIEND_CHECK_NEW;
}

int func_02011980(WifiList *wifiList, u64 friendKey, int *index) {
    DWCUserData *userData = func_02009f7c(wifiList);
    int i = 0;
    DWCFriendData *friends = func_0200a0e4(wifiList, 0);
    int count = func_0200a150(wifiList);
    DWCFriendData token;

    if (!func_020578cc(userData, friendKey)) {
        return FRIEND_CHECK_INVALID;
    }
    func_02057ef8(&token, friendKey);
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

BOOL func_02011a28(DWCFriendData *friendData) {
    BOOL isNew = FALSE;
    DWCFriendData *friends = (DWCFriendData *)func_02042d64();
    int index;
    int result;

    result = findFriendData(friendData, &index);
    if (result != FRIEND_CHECK_SAME) {
        if (result == FRIEND_CHECK_SAME_PROFILE) {
            sys_memcpy(friendData, &friends[index], sizeof(DWCFriendData));
        } else if (result == FRIEND_CHECK_NEW) {
            isNew = TRUE;
        }
    }
    return isNew;
}

static void setFriendInfo(WifiList *wifiList, PlayerInfo *info, u32 index, u32 heapId, BOOL setName) {
    if (setName) {
        StrBuf *name = copyTrainerNameToNewStrbuf(info->name, heapId);

        func_0200a11c(wifiList, index, name);
        GFL_StrBufFree(name);
    }
    func_0200a034(wifiList, index, WIFI_LIST_FRIEND_GENDER, getTrainerGender(info));
    func_0200a034(wifiList, index, WIFI_LIST_FRIEND_ID, getIDAsUInt(info));
    func_0200a034(wifiList, index, WIFI_LIST_FRIEND_PROFILE_ID, func_02008bdc(info));
    func_0200a034(wifiList, index, WIFI_LIST_FRIEND_TRAINER_VIEW, func_02008bf4(info));
}

static void addFriend(GameData *gameData, PlayerInfo *info, const void *friendData, u32 index, u32 heapId,
                      int mode) {
    WifiList *wifiList = GameData_GetWifiList(gameData);
    DWCFriendData *dest = func_0200a0e4(wifiList, index);

    if (mode != FRIEND_ADD_VIEW) {
        sys_memcpy(friendData, dest, sizeof(DWCFriendData));
    }
    if (mode == FRIEND_ADD_NEW) {
        setFriendInfo(wifiList, info, index, heapId, TRUE);
    } else if (mode == FRIEND_ADD_UPDATE) {
        setFriendInfo(wifiList, info, index, heapId, FALSE);
    }
    func_0200a034(wifiList, index, WIFI_LIST_FRIEND_TRAINER_VIEW, func_02008bf4(info));
}

BOOL func_02011b30(GameData *gameData, PlayerInfo *info, const void *friendData, HeapID heapId) {
    WifiList *wifiList = GameData_GetWifiList(gameData);
    int i;

    if (!DWC_IsValidFriendData(friendData)) {
        return FALSE;
    }
    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        DWCFriendData *other = func_0200a0e4(wifiList, i);
        int result;

        if (!func_0200a138(wifiList, i)) {
            addFriend(gameData, info, friendData, i, heapId, FRIEND_ADD_NEW);
            return TRUE;
        }
        result = compareFriendData((DWCFriendData *)friendData, func_02009f7c(wifiList), other);
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
