// DWC's friend list (TWL-DWC's dwc_friend.c, overlay 11, Thumb): the texts of the friends' statuses, and the friend
// list that DWC keeps in step with GameSpy's buddy list. Its .data holds the status keys and the profile search's
// texts, and its .bss the five statics below, which the code reaches from one base, before dwc_login.c's login.
// The layouts are in dwc/friend.h. The static DWCi_HandleGPError of the SDK's dwc_friend.c and dwc_login.c is
// DWCi_HandleFriendGPError and DWCi_HandleLoginGPError here, to keep their symbols apart.
// The ROM names none of DWC's files. Black 1's DWC, whose functions aletheia matched to TWL-DWC's library objects
// (layout/symbols_sdk.txt), has overlay 11's run of them in libdwcnonport's dwc_nonport.o, libdwcnas's dwc_nasfunc.o
// and dwc_nastime.o, and libdwcmatch's dwc_common.o, dwc_friend.o and dwc_login.o, in this order and with the same
// functions, and each file's .data and .bss follow in the same order.

#include "dwc/base64.h"
#include "dwc/common.h"
#include "dwc/dwc.h"
#include "dwc/ext.h"
#include "dwc/friend.h"
#include "dwc/gpi.h"
#include "dwc/login.h"
#include "dwc/memfunc.h"
#include "nitro/mi.h"

static u8 DWC_GetFriendStatus(const DWCFriendData *friendData, char *location);
static u8 DWC_GetFriendStatusSC(const DWCFriendData *friendData, u8 *scm, u8 *scn, char *location);
static u8 DWC_GetFriendStatusDataSC(const DWCFriendData *friendData, u8 *scm, u8 *scn, char *statusData, int *size);
static void DWCi_CallBuddyFriendCallback(int index);
static int DWCi_GPProcess(void);
static void DWCi_CloseFriendProcess(void);
static void DWCi_UpdateFriendReq(DWCFriendData *friendList, int friendListLen);
static void DWCi_EndUpdateServers(void);
static void DWCi_DeleteFriendFromList(DWCFriendData *friendList, int index, int srcIndex);
static BOOL DWCi_RefreshFriendListForth(DWCFriendData *friendList, int index, int profileId);
static int DWCi_RefreshFriendListAll(DWCFriendData *friendList, int friendListLen, int profileId);
static int DWCi_GPSendBuddyRequest(int profileId);
static BOOL DWCi_GetFriendBuddyStatus(const DWCFriendData *friendData, GPBuddyStatus *status);
static int DWCi_HandleFriendGPError(int result);
static void DWCi_GPProfileSearchCallback(GPConnection *connection, GPProfileSearchResponseArg *arg, void *param);
static void DWCi_GPGetInfoCallback_RecvBuddyRequest(GPConnection *connection, GPGetInfoResponseArg *arg, void *param);
static void DWCi_GPGetInfoCallback_RecvAuthMessage(GPConnection *connection, GPGetInfoResponseArg *arg, void *param);
static int DWCi_GetPersCallbackLevel(void);
static void DWCi_GetReverseBuddiesCallback(GPConnection *connection, GPGetReverseBuddiesListResponseArg *arg,
                                           void *param);

// The friend list's control; a flag that no code of this version reaches; whether PersistThink runs, and whether
// the stats server is to be closed once it returns; and whether DWC_ShutdownFriendsMatch is closing the stats
// server. Each is its own zero-initialized static, which MWCC reaches from the section's base as the ROM does, in
// the order that its size sort gives the ROM's addresses (sCloseStats, sInPersistThink, sUnk08, sFriendControl,
// sInCloseStats)
static DWCFriendControl *sFriendControl = NULL;
static u32 sUnk08 = 0;
static BOOL sInPersistThink = FALSE;
static BOOL sCloseStats = FALSE;
static BOOL sInCloseStats = FALSE;

static u8 DWC_GetFriendStatus(const DWCFriendData *friendData, char *location) {
    return DWC_GetFriendStatusSC(friendData, NULL, NULL, location);
}

// A friend's status, and from a status text of the matching (status 6) its SCM and SCN values
static u8 DWC_GetFriendStatusSC(const DWCFriendData *friendData, u8 *scm, u8 *scn, char *location) {
    char number[4];
    GPBuddyStatus status;
    u8 result;

    if (DWCi_GetFriendBuddyStatus(friendData, &status)) {
        if (status.status == 6) {
            if (scm != NULL) {
                if (DWC_GetCommonValueString("SCM", number, status.statusString, '/') > 0) {
                    *scm = strtoul(number, NULL, 10);
                } else {
                    *scm = 0;
                }
            }
            if (scn != NULL) {
                if (DWC_GetCommonValueString("SCN", number, status.statusString, '/') > 0) {
                    *scn = strtoul(number, NULL, 10);
                } else {
                    *scn = 0;
                }
            }
        } else {
            if (scm != NULL) {
                *scm = 0;
            }
            if (scn != NULL) {
                *scn = 0;
            }
        }
        if (location != NULL) {
            strcpy(location, status.locationString);
        }
        result = status.status;
        if (DWCi_GetBuddyStatusLCK(&status)) {
            result = 2;
        }
        return result;
    }
    if (scm != NULL) {
        *scm = 0;
    }
    if (scn != NULL) {
        *scn = 0;
    }
    return 0;
}

u8 DWC_GetFriendStatusData(const DWCFriendData *friendData, char *statusData, int *size) {
    return DWC_GetFriendStatusDataSC(friendData, NULL, NULL, statusData, size);
}

static u8 DWC_GetFriendStatusDataSC(const DWCFriendData *friendData, u8 *scm, u8 *scn, char *statusData, int *size) {
    char encoded[0x100];
    u8 status = DWC_GetFriendStatusSC(friendData, scm, scn, encoded);

    if (status == 0) {
        *size = -1;
        return status;
    }
    *size = DWC_Base64Decode(encoded, strlen(encoded), NULL, 0);
    if (statusData == NULL || *size == -1) {
        return status;
    }
    DWC_Base64Decode(encoded, strlen(encoded), statusData, *size);
    return status;
}

BOOL DWC_SetOwnStatusData(const char *statusData, u32 size) {
    char encoded[0x100];
    int length;
    BOOL result;

    if (sFriendControl == NULL || !DWCi_CheckLogin()) {
        return FALSE;
    }
    length = DWC_Base64Encode(statusData, size, encoded, 0xff);
    if (length == -1) {
        return FALSE;
    }
    result = FALSE;
    encoded[length] = 0;
    if (DWCi_SetGPStatus(-1, 0, encoded) == 0) {
        result = TRUE;
    }
    return result;
}

BOOL DWC_SetBuddyFriendCallback(DWCBuddyFriendCallback callback, void *param) {
    if (sFriendControl == NULL) {
        return FALSE;
    }
    sFriendControl->buddyCallback = callback;
    sFriendControl->buddyParam = param;
    return TRUE;
}

void DWCi_FriendInit(DWCFriendControl *control, GPConnection *connection, const u16 *playerName,
                     DWCFriendData *friendList, int friendListLen) {
    int i = 0;

    sFriendControl = control;
    control->state = 0;
    sFriendControl->pGpObj = connection;
    sFriendControl->gpProcessCount = 0;
    sFriendControl->lastGpProcess = 0;
    sFriendControl->friendListLen = friendListLen;
    sFriendControl->friendList = friendList;
    sFriendControl->buddyUpdateIdx = 0;
    sFriendControl->friendListChanged = 0;
    sFriendControl->buddyUpdateState = 0;
    sFriendControl->updateSteps = 0;
    sFriendControl->persLogin = 0;
    sFriendControl->profileID = 0;
    sFriendControl->playerName = playerName;
    sFriendControl->updateCallback = NULL;
    sFriendControl->updateParam = NULL;
    sFriendControl->statusCallback = NULL;
    sFriendControl->statusParam = NULL;
    sFriendControl->deleteCallback = NULL;
    sFriendControl->deleteParam = NULL;
    sFriendControl->buddyCallback = NULL;
    sFriendControl->buddyParam = NULL;
    sFriendControl->persLoginCallback = NULL;
    sFriendControl->persLoginParam = NULL;
    sFriendControl->saveCallback = NULL;
    sFriendControl->loadCallback = NULL;
    sFriendControl->profileIds = NULL;
    for (; i < friendListLen; i++) {
        DWCi_Acc_SetFlag5(&sFriendControl->friendList[i], FALSE);
    }
}

void DWCi_FriendProcess(void) {
    int count;
    int added;
    int i;

    if (sFriendControl == NULL || DWCi_IsError()) {
        return;
    }
    if (sFriendControl->friendList == NULL) {
        if (sFriendControl->pGpObj != NULL && *sFriendControl->pGpObj != NULL) {
            DWCi_GPProcess();
        }
        return;
    }
    if (DWCi_GetPersCallbackLevel() != 0 || IsStatsConnected()) {
        sInPersistThink = TRUE;
        sCloseStats = FALSE;
        PersistThink();
        sInPersistThink = FALSE;
        if (sCloseStats == TRUE) {
            sCloseStats = FALSE;
            CloseStatsConnection();
        }
    }
    if (sFriendControl->pGpObj != NULL && *sFriendControl->pGpObj != NULL) {
        if (DWCi_GPProcess() != 0) {
            return;
        }
        if (sFriendControl->state == 0) {
            return;
        }
        if (sFriendControl->friendList != NULL && sFriendControl->buddyUpdateState != 3 &&
            sFriendControl->gpProcessCount > 7) {
            if (sFriendControl->buddyUpdateState <= 1) {
                DWCi_UpdateFriendReq(sFriendControl->friendList, sFriendControl->friendListLen);
            }
            if (sFriendControl->buddyUpdateIdx >= sFriendControl->friendListLen) {
                sFriendControl->buddyUpdateState = 3;
                sFriendControl->updateSteps++;
            }
        }
    }
    if (sFriendControl->updateSteps >= 2) {
        if (sFriendControl->pGpObj != NULL && *sFriendControl->pGpObj != NULL) {
            count = 0;
            added = 0;
            for (i = 0; i < sFriendControl->friendListLen; i++) {
                if (DWC_GetFriendDataType(&sFriendControl->friendList[i]) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID) {
                    count++;
                }
            }
            if (count == 0) {
                sFriendControl->updateSteps = 0;
                DWCi_EndUpdateServers();
                return;
            }
            sFriendControl->profileIds = DWC_Alloc(4, count * sizeof(int));
            if (sFriendControl->profileIds == NULL) {
                DWCi_StopFriendProcess(DWC_ERROR_FATAL, -70001);
                return;
            }
            for (i = 0; i < sFriendControl->friendListLen; i++) {
                if (DWC_GetFriendDataType(&sFriendControl->friendList[i]) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID) {
                    sFriendControl->profileIds[added] =
                        DWC_GetGsProfileId(DWCi_GetUserData(), &sFriendControl->friendList[i]);
                    added++;
                }
            }
            if (DWCi_HandleFriendGPError(gpGetReverseBuddiesList(sFriendControl->pGpObj, sFriendControl->profileIds,
                                                                 count, 0, (GPCallback)DWCi_GetReverseBuddiesCallback,
                                                                 NULL)) != 0) {
                return;
            }
        }
        sFriendControl->updateSteps = 0;
    }
}

BOOL DWCi_IsServerUpdating(void) {
    BOOL result = FALSE;

    if (sFriendControl != NULL && sFriendControl->state == 1) {
        result = TRUE;
    }
    return result;
}

void DWCi_UpdateServersAsync(const char *authToken, const char *partnerChallenge,
                             DWCUpdateServersCallback updateCallback, void *updateParam,
                             DWCFriendStatusCallback statusCallback, void *statusParam,
                             DWCDeleteFriendListCallback deleteCallback, void *deleteParam) {
    sFriendControl->updateCallback = updateCallback;
    sFriendControl->updateParam = updateParam;
    sFriendControl->statusCallback = statusCallback;
    sFriendControl->statusParam = statusParam;
    sFriendControl->deleteCallback = deleteCallback;
    sFriendControl->deleteParam = deleteParam;
    sFriendControl->friendListChanged = 0;
    sFriendControl->buddyUpdateState = 0;
    sFriendControl->updateSteps = 0;
    sFriendControl->buddyUpdateIdx = 0;
    sFriendControl->state = 1;
    if (sFriendControl->friendList == NULL) {
        sFriendControl->updateSteps++;
    }
    sFriendControl->updateSteps++;
}

void DWCi_StopFriendProcess(DWCError error, int code) {
    if (sFriendControl != NULL && error != DWC_ERROR_NONE) {
        DWCi_SetError(error, code);
        if (DWCi_IsServerUpdating()) {
            if (sFriendControl->profileIds != NULL) {
                DWC_Free(4, sFriendControl->profileIds, 0);
                sFriendControl->profileIds = NULL;
            }
            sFriendControl->updateCallback(error, sFriendControl->friendListChanged, sFriendControl->updateParam);
        }
        DWCi_CloseFriendProcess();
    }
}

void DWCi_GPRecvBuddyRequestCallback(GPConnection *connection, GPRecvBuddyRequestArg *arg, void *param) {
    if (sFriendControl->friendList != NULL) {
        gpGetInfo(connection, arg->profile, 0, 0, (GPCallback)DWCi_GPGetInfoCallback_RecvBuddyRequest, NULL);
    }
}

BOOL DWCi_GPRecvBuddyAuthCallback(GPConnection *connection, GPRecvBuddyAuthArg *arg, void *param) {
    gpGetInfo(connection, arg->profile, 0, 0, (GPCallback)DWCi_GPGetInfoCallback_RecvAuthMessage, NULL);
    return TRUE;
}

void DWCi_GPRecvBuddyStatusCallback(GPConnection *connection, GPRecvBuddyStatusArg *arg, void *param) {
    GPBuddyStatus status;
    int index = DWCi_GetFriendListIndex(arg->profile);

    if (index == -1) {
        if (gpIsBuddy(connection, arg->profile)) {
            gpDeleteBuddy(connection, arg->profile);
        }
    } else if (sFriendControl->statusCallback != NULL) {
        gpGetBuddyStatus(connection, arg->index, &status);
        sFriendControl->statusCallback(index, status.status, status.locationString, sFriendControl->statusParam);
    }
}

int DWCi_GetProfileIDFromList(int index) {
    DWCFriendData *friendList = sFriendControl->friendList;
    int profileId;

    if (friendList == NULL) {
        return 0;
    }
    profileId = DWC_GetGsProfileId(DWCi_GetUserData(), &friendList[index]);
    if (profileId != 0 && profileId != -1) {
        return profileId;
    }
    return 0;
}

int DWCi_GetFriendListIndex(int profileId) {
    int i;

    if (sFriendControl == NULL || profileId == 0) {
        return -1;
    }
    for (i = 0; i < sFriendControl->friendListLen; i++) {
        if (profileId == DWCi_GetProfileIDFromList(i)) {
            return i;
        }
    }
    return -1;
}

void DWCi_InitGPProcessCount(void) {
    if (sFriendControl != NULL) {
        sFriendControl->gpProcessCount = 0;
        sFriendControl->lastGpProcess = OS_GetTick();
    }
}

int DWCi_SetGPStatus(int status, const char *statusString, const char *locationString) {
    if (sFriendControl == NULL || !DWCi_CheckLogin()) {
        return 0;
    }
    if (status == -1) {
        status = ((GPIConnection *)*sFriendControl->pGpObj)->lastStatusState;
    }
    if (statusString == NULL) {
        statusString = ((GPIConnection *)*sFriendControl->pGpObj)->lastStatusString;
    }
    if (locationString == NULL) {
        locationString = ((GPIConnection *)*sFriendControl->pGpObj)->lastLocationString;
    }
    return gpSetStatusA(sFriendControl->pGpObj, status, statusString, locationString);
}

// Tells the game that a friend became a buddy, and its status
static void DWCi_CallBuddyFriendCallback(int index) {
    GPBuddyStatus status;

    if (sFriendControl->buddyCallback != NULL && sFriendControl->state != 1) {
        sFriendControl->buddyCallback(index, sFriendControl->buddyParam);
    }
    if (sFriendControl->statusCallback != NULL) {
        u8 result = DWC_GetFriendStatus(&sFriendControl->friendList[index], status.locationString);

        sFriendControl->statusCallback(index, result, status.locationString, sFriendControl->statusParam);
    }
}

void DWCi_ShutdownFriend(void) {
    sFriendControl = NULL;
}

// Runs GP every 300 milliseconds
static int DWCi_GPProcess(void) {
    int result = 0;
    OSTick now = OS_GetTick();
    DWCFriendControl *control = sFriendControl;

    if (OS_TicksToMilliSeconds(now - control->lastGpProcess) >= 300) {
        control->gpProcessCount++;
        result = gpProcess(sFriendControl->pGpObj);
        sFriendControl->lastGpProcess = OS_GetTick();
    }
    return result;
}

static void DWCi_CloseFriendProcess(void) {
    if (sFriendControl != NULL) {
        sFriendControl->state = 0;
        sFriendControl->buddyUpdateState = 0;
        sFriendControl->updateSteps = 0;
    }
}

// Confirms the friends that are buddies, deletes the buddies that aren't friends, and asks the friends that aren't
// buddies to be, looking up the profile of a login ID first
static void DWCi_UpdateFriendReq(DWCFriendData *friendList, int friendListLen) {
    int count;
    int index;
    char loginName[0x18];
    GPBuddyStatus status;
    int i;

    if (sFriendControl->buddyUpdateState == 0) {
        DWCi_HandleFriendGPError(gpGetNumBuddies(sFriendControl->pGpObj, &count));
        for (index = 0; index < count; index++) {
            DWCi_HandleFriendGPError(gpGetBuddyStatus(sFriendControl->pGpObj, index, &status));
            for (i = 0; i < friendListLen; i++) {
                if (status.profile == DWCi_GetProfileIDFromList(i)) {
                    if (!DWCi_Acc_IsBuddyData(&friendList[i])) {
                        DWC_SetGsProfileId(&friendList[i], status.profile);
                        DWCi_SetBuddyFriendData(&friendList[i]);
                        if (DWC_IsBuddyFriendData(&sFriendControl->friendList[i])) {
                            DWCi_Acc_SetFlag4(&sFriendControl->friendList[i], TRUE);
                        }
                        sFriendControl->friendListChanged = 1;
                    }
                    break;
                }
            }
            if (i == friendListLen) {
                DWCi_HandleFriendGPError(gpDeleteBuddy(sFriendControl->pGpObj, status.profile));
                count--;
                index--;
            }
        }
        sFriendControl->buddyUpdateState = 1;
    }
    for (; sFriendControl->buddyUpdateIdx < friendListLen; sFriendControl->buddyUpdateIdx++) {
        if (DWCi_Acc_IsFlag5(&friendList[sFriendControl->buddyUpdateIdx])) {
            continue;
        }
        {
            int profileId = DWCi_GetProfileIDFromList(sFriendControl->buddyUpdateIdx);

            if (profileId != 0) {
                if (!DWCi_RefreshFriendListForth(friendList, sFriendControl->buddyUpdateIdx, profileId)) {
                    DWCi_HandleFriendGPError(gpGetBuddyIndex(sFriendControl->pGpObj, profileId, &index));
                    if (index == -1) {
                        DWCi_GPSendBuddyRequest(profileId);
                        DWCi_Acc_SetFlag5(&friendList[sFriendControl->buddyUpdateIdx], TRUE);
                    }
                }
            } else if (DWC_GetGsProfileId(DWCi_GetUserData(), &friendList[sFriendControl->buddyUpdateIdx]) == -1) {
                DWC_LoginIdToUserName(DWCi_GetUserData(), &friendList[sFriendControl->buddyUpdateIdx], loginName);
#ifdef PLATFORM_PC
                gpProfileSearchA(sFriendControl->pGpObj, NULL, NULL, NULL, NULL, loginName, 0, 0,
                                 (GPCallback)DWCi_GPProfileSearchCallback,
                                 (void *)(uptr)sFriendControl->buddyUpdateIdx);
#else
                gpProfileSearchA(sFriendControl->pGpObj, NULL, NULL, NULL, NULL, loginName, 0, 0,
                                 (GPCallback)DWCi_GPProfileSearchCallback, (void *)sFriendControl->buddyUpdateIdx);
#endif
                sFriendControl->buddyUpdateState = 2;
                return;
            }
        }
    }
}

static void DWCi_EndUpdateServers(void) {
    sFriendControl->updateCallback(DWC_ERROR_NONE, sFriendControl->friendListChanged, sFriendControl->updateParam);
    sFriendControl->state = 2;
}

// Deletes a friend that is a copy of the one at srcIndex
static void DWCi_DeleteFriendFromList(DWCFriendData *friendList, int index, int srcIndex) {
    if (sFriendControl != NULL) {
        MI_CpuFill8(&friendList[index], 0, sizeof(DWCFriendData));
        if (sFriendControl->deleteCallback != NULL) {
            sFriendControl->deleteCallback(index, srcIndex, sFriendControl->deleteParam);
        }
    }
}

// Deletes a friend, or the earlier friend, that has the same profile ID, keeping the confirmed one
static BOOL DWCi_RefreshFriendListForth(DWCFriendData *friendList, int index, int profileId) {
    DWCFriendData *friendData = &friendList[index];
    int i;

    if (DWC_GetFriendDataType(friendData) == DWC_ACC_FRIENDDATA_LOGIN_ID) {
        return FALSE;
    }
    for (i = 0; i < index; i++) {
        int otherId = DWCi_GetProfileIDFromList(i);

        if (otherId != 0 && otherId == profileId) {
            if (DWCi_Acc_IsBuddyData(friendData) && !DWCi_Acc_IsBuddyData(&friendList[i])) {
                DWCi_DeleteFriendFromList(friendList, i, index);
            } else {
                DWCi_DeleteFriendFromList(friendList, index, i);
            }
            sFriendControl->friendListChanged = 1;
            return TRUE;
        }
    }
    return FALSE;
}

// Merges the friends that have the same profile ID, returning the index of the one that has the given ID
static int DWCi_RefreshFriendListAll(DWCFriendData *friendList, int friendListLen, int profileId) {
    int found = -1;
    int i;

    for (i = 0; i < friendListLen; i++) {
        int id = DWCi_GetProfileIDFromList(i);
        int j;

        if (id == 0) {
            continue;
        }
        if (id == profileId) {
            found = i;
        }
        for (j = i + 1; j < friendListLen; j++) {
            if (id == DWCi_GetProfileIDFromList(j)) {
                if (DWC_GetFriendDataType(&friendList[i]) == DWC_ACC_FRIENDDATA_FRIEND_KEY &&
                    DWC_GetFriendDataType(&friendList[j]) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID) {
                    DWC_SetGsProfileId(&friendList[i], id);
                }
                if (DWCi_Acc_IsBuddyData(&friendList[j])) {
                    DWCi_SetBuddyFriendData(&friendList[i]);
                    if (DWC_IsBuddyFriendData(&sFriendControl->friendList[i])) {
                        DWCi_Acc_SetFlag4(&sFriendControl->friendList[i], TRUE);
                    }
                }
                DWCi_DeleteFriendFromList(friendList, j, i);
                sFriendControl->friendListChanged = 1;
            }
        }
    }
    return found;
}

static int DWCi_GPSendBuddyRequest(int profileId) {
    int result = gpSendBuddyRequestA(sFriendControl->pGpObj, profileId, "");

    DWCi_HandleFriendGPError(result);
    return result;
}

static BOOL DWCi_GetFriendBuddyStatus(const DWCFriendData *friendData, GPBuddyStatus *status) {
    BOOL result = FALSE;
    int index = 0;
    int profileId;

    if (sFriendControl == NULL || !DWCi_CheckLogin()) {
        return FALSE;
    }
    profileId = DWC_GetGsProfileId(DWCi_GetUserData(), friendData);
    if (profileId > 0) {
        if (gpGetBuddyIndex(sFriendControl->pGpObj, profileId, &index) != 0) {
            // style-ok: goto-return BW1's DWC decompilation (aletheia, dwc_friend.c) exits the same way
            goto end;
        }
    }
    if (!(profileId > 0 && index != -1)) {
        result = FALSE;
    } else {
        int error = gpGetBuddyStatus(sFriendControl->pGpObj, index, status);

        result = FALSE;
        if (error == 0) {
            result = TRUE;
        }
    }
end:
    return result;
}

// Ends the friend list's update for an error of GP, returning it
static int DWCi_HandleFriendGPError(int result) {
    int error = result;
    int code;

    if (result == 0) {
        return 0;
    }
    switch (result) {
    case 1:
        error = DWC_ERROR_FATAL;
        code = error - 10;
        break;
    case 2:
        error = DWC_ERROR_FATAL;
        code = error - 11;
        break;
    case 3:
        error = DWC_ERROR_NETWORK;
        code = error - 16;
        break;
    case 4:
        error = DWC_ERROR_NETWORK;
        code = error - 26;
        break;
    }
    DWCi_StopFriendProcess(error, code - 71000);
    return result;
}

// The profile search of a login ID: the friend becomes the profile, as a buddy if it is one already
static void DWCi_GPProfileSearchCallback(GPConnection *connection, GPProfileSearchResponseArg *arg, void *param) {
#ifdef PLATFORM_PC
    // The callback argument is the u8 buddy index supplied by gpProfileSearch.
    int index = (int)(uptr)param;
#else
    int index = (int)param;
#endif
    int i;
    int buddyIndex;

    if (arg->result == 0 && arg->numMatches != 0 &&
        DWC_GetFriendDataType(&sFriendControl->friendList[index]) != DWC_ACC_FRIENDDATA_NODATA) {
        if (sFriendControl->state != 1) {
            return;
        }
        for (i = 0; i < arg->numMatches; i++) {
            if (DWCi_RefreshFriendListForth(sFriendControl->friendList, index, arg->matches[i].profile)) {
                sFriendControl->buddyUpdateIdx++;
                sFriendControl->buddyUpdateState = 1;
                arg->more = GP_DONE;
                return;
            }
        }
        for (i = 0; i < arg->numMatches; i++) {
            DWCi_HandleFriendGPError(gpGetBuddyIndex(connection, arg->matches[i].profile, &buddyIndex));
            if (buddyIndex == -1) {
                DWCi_GPSendBuddyRequest(arg->matches[i].profile);
                DWCi_Acc_SetFlag5(&sFriendControl->friendList[index], TRUE);
            } else {
                DWC_SetGsProfileId(&sFriendControl->friendList[index], arg->matches[0].profile);
                DWCi_SetBuddyFriendData(&sFriendControl->friendList[index]);
                DWCi_Acc_SetBuddyFlag(&sFriendControl->friendList[index], TRUE);
                if (DWC_IsBuddyFriendData(&sFriendControl->friendList[index])) {
                    DWCi_Acc_SetFlag4(&sFriendControl->friendList[index], TRUE);
                }
                DWCi_CallBuddyFriendCallback(index);
                sFriendControl->buddyUpdateIdx++;
                sFriendControl->buddyUpdateState = 1;
                arg->more = GP_DONE;
                sFriendControl->friendListChanged = 1;
                return;
            }
        }
        if (arg->more != GP_MORE) {
            sFriendControl->buddyUpdateIdx++;
            sFriendControl->buddyUpdateState = 1;
        }
        return;
    }
    if (arg->result != 0) {
        if (DWCi_HandleFriendGPError(arg->result) != 0) {
            return;
        }
    } else if (sFriendControl->state == 1 ||
               DWC_GetFriendDataType(&sFriendControl->friendList[index]) == DWC_ACC_FRIENDDATA_NODATA) {
        sFriendControl->buddyUpdateIdx++;
        sFriendControl->buddyUpdateState = 1;
    }
}

// The info of a profile that asked to be a buddy: the request is accepted if the profile is a friend (by its login
// name in the last name, or by its profile ID and game code), and denied if not
static void DWCi_GPGetInfoCallback_RecvBuddyRequest(GPConnection *connection, GPGetInfoResponseArg *arg, void *param) {
    BOOL found = FALSE;
    BOOL wasBuddy = FALSE;
    char gameCode[5];
    char loginName[0x17];
    int i;

    if (arg->result != 0) {
        return;
    }
    for (i = 0; i < sFriendControl->friendListLen; i++) {
        if (DWC_GetFriendDataType(&sFriendControl->friendList[i]) == DWC_ACC_FRIENDDATA_LOGIN_ID) {
            DWC_LoginIdToUserName(DWCi_GetUserData(), &sFriendControl->friendList[i], loginName);
            if (strcmp(loginName, arg->lastname) == 0) {
                if (DWC_IsBuddyFriendData(&sFriendControl->friendList[i])) {
                    wasBuddy = TRUE;
                }
                gpAuthBuddyRequest(connection, arg->profile);
                DWC_SetGsProfileId(&sFriendControl->friendList[i], arg->profile);
                found = TRUE;
            }
        } else if (DWC_GetFriendDataType(&sFriendControl->friendList[i]) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID ||
                   DWC_GetFriendDataType(&sFriendControl->friendList[i]) == DWC_ACC_FRIENDDATA_FRIEND_KEY) {
            u32 code = DWCi_GetUserData()->gamecode;

            OS_SNPrintf(gameCode, 5, "%c%c%c%c", (u8)(code >> 24), (u8)(code >> 16), (u8)(code >> 8), (u8)code);
            if (arg->profile == DWC_GetGsProfileId(DWCi_GetUserData(), &sFriendControl->friendList[i]) &&
                memcmp(gameCode, &arg->lastname[9], 4) == 0) {
                if (DWC_IsBuddyFriendData(&sFriendControl->friendList[i])) {
                    wasBuddy = TRUE;
                }
                gpAuthBuddyRequest(connection, arg->profile);
                found = TRUE;
            }
        }
    }
    if (found) {
        int index;

        DWCi_GPSendBuddyRequest(arg->profile);
        index = DWCi_RefreshFriendListAll(sFriendControl->friendList, sFriendControl->friendListLen, arg->profile);
        DWCi_Acc_SetBuddyFlag(&sFriendControl->friendList[index], TRUE);
        if (DWC_IsBuddyFriendData(&sFriendControl->friendList[index])) {
            DWCi_Acc_SetFlag4(&sFriendControl->friendList[index], TRUE);
            if (!wasBuddy) {
                DWCi_CallBuddyFriendCallback(index);
                sFriendControl->friendListChanged = 1;
            }
        }
    } else {
        gpDenyBuddyRequest(connection, arg->profile);
    }
}

// The info of a profile that authorized our buddy request: the friend it is becomes a confirmed buddy
static void DWCi_GPGetInfoCallback_RecvAuthMessage(GPConnection *connection, GPGetInfoResponseArg *arg, void *param) {
    int i;
    char loginName[0x18];
    BOOL found = FALSE;
    BOOL wasAuthentic = FALSE;

    if (arg->result == 0) {
        for (i = 0; i < sFriendControl->friendListLen; i++) {
            if (DWC_GetFriendDataType(&sFriendControl->friendList[i]) == DWC_ACC_FRIENDDATA_LOGIN_ID) {
                DWC_LoginIdToUserName(DWCi_GetUserData(), &sFriendControl->friendList[i], loginName);
                if (strcmp(loginName, arg->lastname) == 0) {
                    DWC_SetGsProfileId(&sFriendControl->friendList[i], arg->profile);
                    DWCi_SetBuddyFriendData(&sFriendControl->friendList[i]);
                    found = TRUE;
                }
            } else if (DWC_GetFriendDataType(&sFriendControl->friendList[i]) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID ||
                       DWC_GetFriendDataType(&sFriendControl->friendList[i]) == DWC_ACC_FRIENDDATA_FRIEND_KEY) {
                if (arg->profile == DWC_GetGsProfileId(DWCi_GetUserData(), &sFriendControl->friendList[i])) {
                    if (DWCi_Acc_IsBuddyData(&sFriendControl->friendList[i]) == TRUE) {
                        wasAuthentic = TRUE;
                    } else {
                        DWC_SetGsProfileId(&sFriendControl->friendList[i], arg->profile);
                        DWCi_SetBuddyFriendData(&sFriendControl->friendList[i]);
                        found = TRUE;
                    }
                }
            }
        }
        if (found) {
            int index =
                DWCi_RefreshFriendListAll(sFriendControl->friendList, sFriendControl->friendListLen, arg->profile);

            if (!wasAuthentic) {
                DWCi_Acc_SetBuddyFlag(&sFriendControl->friendList[index], TRUE);
                if (DWC_IsBuddyFriendData(&sFriendControl->friendList[index])) {
                    DWCi_Acc_SetFlag4(&sFriendControl->friendList[index], TRUE);
                }
                DWCi_CallBuddyFriendCallback(index);
            }
            sFriendControl->friendListChanged = 1;
        }
    }
}

static int DWCi_GetPersCallbackLevel(void) {
    return sFriendControl->persLogin;
}

void DWCi_BeginCloseStatsConnection(void) {
    sInCloseStats = TRUE;
}

void DWCi_EndCloseStatsConnection(void) {
    sInCloseStats = FALSE;
}

// The reverse buddies of the profile IDs: a friend whose profile has the player as a buddy gets the buddy flag, and
// one that hasn't loses it
static void DWCi_GetReverseBuddiesCallback(GPConnection *connection, GPGetReverseBuddiesListResponseArg *arg,
                                           void *param) {
    int i;
    int j;

    if (DWCi_IsServerUpdating()) {
        if (sFriendControl->profileIds != NULL) {
            DWC_Free(4, sFriendControl->profileIds, 0);
            sFriendControl->profileIds = NULL;
        }
        if (arg->result != 0) {
            DWCi_EndUpdateServers();
            return;
        }
        for (i = 0; i < sFriendControl->friendListLen; i++) {
            if (DWC_GetFriendDataType(&sFriendControl->friendList[i]) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID) {
                for (j = 0; j < arg->numOfUniqueMatches; j++) {
                    if (arg->matches[j].profile ==
                        DWC_GetGsProfileId(DWCi_GetUserData(), &sFriendControl->friendList[i])) {
                        if (!DWCi_Acc_IsBuddyFlag(&sFriendControl->friendList[i])) {
                            sFriendControl->friendListChanged = 1;
                            DWCi_Acc_SetBuddyFlag(&sFriendControl->friendList[i], TRUE);
                            if (DWC_IsBuddyFriendData(&sFriendControl->friendList[i])) {
                                DWCi_Acc_SetFlag4(&sFriendControl->friendList[i], TRUE);
                            }
                        }
                        break;
                    }
                }
                if (j == arg->numOfUniqueMatches && DWCi_Acc_IsBuddyFlag(&sFriendControl->friendList[i]) == TRUE) {
                    sFriendControl->friendListChanged = 1;
                    DWCi_Acc_SetBuddyFlag(&sFriendControl->friendList[i], FALSE);
                }
                if (DWC_IsBuddyFriendData(&sFriendControl->friendList[i])) {
                    DWCi_Acc_SetFlag4(&sFriendControl->friendList[i], TRUE);
                }
            }
        }
        DWCi_EndUpdateServers();
    }
}
