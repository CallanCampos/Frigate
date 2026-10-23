// Overlay 11's friend list and login (friend.c, a guessed name: the ROM embeds none for DWC's files; Thumb). It keeps
// the server's date from the authentication server's answer, reads and writes the texts of the friends' statuses, keeps
// the friend list in step with GameSpy's buddy list and logs in to GameSpy's GP with the authentication server's
// token. The layouts are in dwc/friend.h. The functions carry the names they have in the SDK (NitroDWC's and the Wii
// DWC's dwc_nonport.c, dwc_nasfunc.c, dwc_common.c, dwc_friend.c and dwc_login.c); the static DWCi_HandleGPError of
// the last two is DWCi_HandleFriendGPError and DWCi_HandleLoginGPError here, and the functions that BW2's DWC added
// have names of ours after them.

#include "dwc/friend.h"
#include "dwc/bm.h"
#include "dwc/common.h"
#include "dwc/dwc.h"
#include "dwc/ext.h"
#include "dwc/gpi.h"
#include "dwc/memfunc.h"
#include "dwc/nasfunc.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include <errno.h>

// The login to the authentication server that the game runs by itself: the HTTP client, what it is started with and
// what the server answered
typedef struct NasLogin {
    DWCHttp http;
    DWCAuthParam param;
    DWCAuthResult result;
} NasLogin;

// The info keys of GP that the login sets: the first and last names and the AIM name hold the console's ID, the login
// name and the user ID, and the zip code the game's code
#define GP_INFO_FIRSTNAME 0x704
#define GP_INFO_LASTNAME 0x705
#define GP_INFO_ZIPCODE 0x708
#define GP_INFO_AIMNAME 0x717

// The seconds of a century, which the clock's seconds wrap at
#define SECONDS_PER_CENTURY 0xbc191380

static u64 DWCi_Np_GetConsoleId(void);
static const char *DWCi_Np_GetConsoleName(void);
static char *DWCi_Np_GetAppGameName(void);
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
static void DWCi_GPGetReverseBuddiesListCallback(GPConnection *connection, GPGetReverseBuddiesListResponseArg *arg,
                                                 void *param);
static void DWCi_CloseLogin(void);
static int DWCi_HandleLoginGPError(int result);
static void DWCi_GPConnectCallback(GPConnection *connection, GPConnectResponseArg *arg, void *param);
static void DWCi_GPConnect(GPCallback callback, int nextState);
static BOOL DWCi_RemoteLogin(void);
static void DWCi_RemoteLoginProcess(void);
static void DWCi_GPGetInfoCallback(GPConnection *connection, GPGetInfoResponseArg *arg, void *param);
static BOOL DWCi_CheckLogin(void);

static const char *sMonthNames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static DWCLoginControl *sLogin;
// The friend list's state: whether the stats server is to be closed once PersistThink returns, whether PersistThink
// runs, the friend list's control, and a flag that two calls set and clear
static struct {
    BOOL closeStats;
    BOOL inPersistThink;
    u32 unk_08;
    DWCFriendControl *control;
    BOOL unk_10;
} sFriend;
static MATHRandContext32 sRandom;
// How far the console's clock is from the server's, the console's time when it was read, and whether it was
static s64 sDateOffset;
static s64 sDateLocalSeconds;
static BOOL sDateValid;
static NasLogin *sNasLogin;

static u64 DWCi_Np_GetConsoleId(void) {
    return 0;
}

static const char *DWCi_Np_GetConsoleName(void) {
    return "NDS";
}

// The game's code from the ROM's header, as a text
static char *DWCi_Np_GetAppGameName(void) {
    static char gameCode[5];

    MI_CpuCopy8((void *)(HW_ROM_HEADER_BUF + 0xc), gameCode, 4);
    gameCode[4] = 0;
    return gameCode;
}

BOOL DWC_GetDateTime(RTCDate *date, RTCTime *time) {
    s64 seconds;

    if (!sDateValid) {
        return FALSE;
    }
    if (RTC_GetDateTime(date, time)) {
        return FALSE;
    }
    seconds = RTC_ConvertDateTimeToSecond(date, time);
    if (seconds == -1) {
        return FALSE;
    }
    if (seconds < sDateLocalSeconds) {
        seconds += SECONDS_PER_CENTURY;
    }
    seconds -= sDateOffset;
    if (seconds < 0 || seconds > SECONDS_PER_CENTURY - 1) {
        return FALSE;
    }
    RTC_ConvertSecondToDateTime(date, time, seconds);
    return TRUE;
}

BOOL DWC_SVLGetTokenAsync(char *svc, DWCSvlResult *buffer) {
    DWCAuthWiFiId id;
    BOOL started;

    DWC_Auth_GetId(&id);
    if (!id.flag) {
        return FALSE;
    }
    if (!DWC_Svl_Init((DWCAuthAlloc)DWC_Alloc, (DWCAuthFree)DWC_Free)) {
        return FALSE;
    }
    if (strlen(svc) == 0) {
        started = DWC_Svl_GetTokenAsync("0000", buffer);
    } else {
        started = DWC_Svl_GetTokenAsync(svc, buffer);
    }
    if (!started) {
        DWC_Svl_Cleanup();
        return FALSE;
    }
    return TRUE;
}

DWCSvlState DWC_SVLProcess(void) {
    DWCSvlState state = DWC_Svl_Process();

    if (state == DWC_SVL_STATE_ERROR || state == DWC_SVL_STATE_SUCCESS || state == DWC_SVL_STATE_CANCELED) {
        DWC_Svl_Cleanup();
    }
    return state;
}

BOOL DWC_NASLoginAsync(void) {
    if (sNasLogin != NULL) {
        OS_Terminate();
    }
    sNasLogin = DWC_Alloc(4, sizeof(NasLogin));
    if (sNasLogin == NULL) {
        DWCi_SetError(DWC_ERROR_FATAL, -20100);
        return FALSE;
    }
    MI_CpuFill8(sNasLogin, 0, sizeof(NasLogin));
    MI_CpuCopy8("", sNasLogin->param.gsbrcd, 9);
    sNasLogin->param.alloc = (DWCAuthAlloc)DWC_Alloc;
    sNasLogin->param.free = (DWCAuthFree)DWC_Free;
    DWC_Auth_Create(&sNasLogin->param, &sNasLogin->http);
    return TRUE;
}

DWCNasLoginState DWC_NASLoginProcess(void) {
    DWCAuthResult result;

    if (sNasLogin == NULL) {
        return 0;
    }
    switch (DWC_Auth_GetError()) {
    case 0:
        return 2;
    case 0x15:
        MI_CpuFill8(&result, 0, sizeof(result));
        DWC_Auth_GetResult(&result);
        DWC_Auth_Destroy();
        if (result.returncode == 20002) {
            DWC_Auth_Create(&sNasLogin->param, &sNasLogin->http);
            return 2;
        }
        DWC_Free(4, sNasLogin, sizeof(NasLogin));
        sNasLogin = NULL;
        return 3;
    case 0x14:
        DWC_Auth_Destroy();
        DWC_Free(4, sNasLogin, sizeof(NasLogin));
        sNasLogin = NULL;
        return 5;
    default:
        DWC_Auth_GetResult(&result);
        DWC_Auth_Destroy();
        DWC_Free(4, sNasLogin, sizeof(NasLogin));
        sNasLogin = NULL;
        DWCi_SetError(DWC_ERROR_AUTH_ANY, result.returncode);
        return 4;
    }
}

BOOL DWCi_SetNasTimeFromDateHeader(const char *dateHeader) {
    RTCDate date;
    RTCTime time;
    char text[0x20];
    s64 localSeconds;
    s64 serverSeconds;
    int i;

    if (strlen(dateHeader) == strlen("Fri, 03 Mar 2006 01:28:13 GMT") && dateHeader[7] == ' ' &&
        dateHeader[11] == ' ' && dateHeader[16] == ' ' && dateHeader[19] == ':' && dateHeader[22] == ':' &&
        dateHeader[25] == ' ') {
        if (RTC_GetDateTime(&date, &time) == 0 && (localSeconds = RTC_ConvertDateTimeToSecond(&date, &time)) != -1) {
            strcpy(text, dateHeader);
            text[7] = 0;
            text[11] = 0;
            text[16] = 0;
            text[19] = 0;
            text[22] = 0;
            text[25] = 0;
            errno = 0;
            date.year = atol(&text[12]);
            if (errno != ERANGE) {
                date.year -= 2000;
                date.month = 13;
                for (i = 0; i < 12; i++) {
                    if (strcmp(sMonthNames[i], &text[8]) == 0) {
                        date.month = i + 1;
                        break;
                    }
                }
                if (date.month <= 12) {
                    errno = 0;
                    date.day = atol(&text[5]);
                    if (errno != ERANGE) {
                        errno = 0;
                        time.hour = atol(&text[17]);
                        if (errno != ERANGE) {
                            errno = 0;
                            time.minute = atol(&text[20]);
                            if (errno != ERANGE) {
                                errno = 0;
                                time.second = atol(&text[23]);
                                if (errno != ERANGE &&
                                    (serverSeconds = RTC_ConvertDateTimeToSecond(&date, &time)) != -1) {
                                    sDateOffset = localSeconds - serverSeconds;
                                    sDateLocalSeconds = localSeconds;
                                    sDateValid = TRUE;
                                    return TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return FALSE;
}

int DWC_SetCommonKeyValueString(const char *key, const char *value, char *out, char sep) {
    OS_SNPrintf(out, 0x1000, "%c%s%c%s", sep, key, sep, value);
    return strlen(out);
}

int DWC_AddCommonKeyValueString(const char *key, const char *value, char *out, char sep) {
    DWC_SetCommonKeyValueString(key, value, strchr(out, 0), sep);
    return strlen(out);
}

int DWC_GetCommonValueString(const char *key, char *dest, const char *text, char sep) {
    char *pair = strchr(text, sep);

    if (pair == NULL) {
        return -1;
    }
    for (;;) {
        char *next;

        if (strncmp(pair + 1, key, strlen(key)) == 0 && sep == pair[strlen(key) + 1]) {
            break;
        }
        next = strchr(pair + 1, sep);
        if (next == NULL) {
            return -1;
        }
        pair = strchr(next + 1, sep);
        if (pair == NULL) {
            return -1;
        }
    }
    pair = strchr(pair + 1, sep);
    if (pair == NULL) {
        return -1;
    }
    {
        char *end = strchr(pair + 1, sep);
        int length;

        if (end != NULL) {
            length = end - (pair + 1);
        } else {
            length = strlen(pair + 1);
        }
        if (dest == NULL) {
            return length;
        }
        strncpy(dest, pair + 1, length);
        dest[length] = 0;
        return length;
    }
}

u32 DWCi_GetMathRand32(u32 max) {
    if (sRandom.x == 0 && sRandom.mul == 0 && sRandom.add == 0) {
        u64 seed;

        OS_GetMacAddress((u8 *)&seed);
        seed = (seed >> 24 & 0xffffff) | OS_GetTick() << 24;
        MATH_InitRand32(&sRandom, seed);
    }
    return MATH_Rand32(&sRandom, max);
}

u32 DWCi_WStrLen(const u16 *str) {
    u32 length = 0;

    while (str[length] != 0) {
        length++;
    }
    return length;
}

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

    if (sFriend.control == NULL || !DWCi_CheckLogin()) {
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
    if (sFriend.control == NULL) {
        return FALSE;
    }
    sFriend.control->buddyCallback = callback;
    sFriend.control->buddyParam = param;
    return TRUE;
}

void DWCi_FriendInit(DWCFriendControl *control, GPConnection *connection, const u16 *playerName,
                     DWCFriendData *friendList, int friendListLen) {
    int i = 0;

    sFriend.control = control;
    control->state = 0;
    sFriend.control->pGpObj = connection;
    sFriend.control->gpProcessCount = 0;
    sFriend.control->lastGpProcess = 0;
    sFriend.control->friendListLen = friendListLen;
    sFriend.control->friendList = friendList;
    sFriend.control->buddyUpdateIdx = 0;
    sFriend.control->friendListChanged = 0;
    sFriend.control->buddyUpdateState = 0;
    sFriend.control->updateSteps = 0;
    sFriend.control->persLogin = 0;
    sFriend.control->profileID = 0;
    sFriend.control->playerName = playerName;
    sFriend.control->updateCallback = NULL;
    sFriend.control->updateParam = NULL;
    sFriend.control->statusCallback = NULL;
    sFriend.control->statusParam = NULL;
    sFriend.control->deleteCallback = NULL;
    sFriend.control->deleteParam = NULL;
    sFriend.control->buddyCallback = NULL;
    sFriend.control->buddyParam = NULL;
    sFriend.control->persLoginCallback = NULL;
    sFriend.control->persLoginParam = NULL;
    sFriend.control->saveCallback = NULL;
    sFriend.control->loadCallback = NULL;
    sFriend.control->profileIds = NULL;
    for (; i < friendListLen; i++) {
        DWCi_Acc_SetFlag5(&sFriend.control->friendList[i], FALSE);
    }
}

void DWCi_FriendProcess(void) {
    int count;
    int added;
    int i;

    if (sFriend.control == NULL || DWCi_IsError()) {
        return;
    }
    if (sFriend.control->friendList == NULL) {
        if (sFriend.control->pGpObj != NULL && *sFriend.control->pGpObj != NULL) {
            DWCi_GPProcess();
        }
        return;
    }
    if (DWCi_GetPersCallbackLevel() != 0 || IsStatsConnected()) {
        sFriend.inPersistThink = TRUE;
        sFriend.closeStats = FALSE;
        PersistThink();
        sFriend.inPersistThink = FALSE;
        if (sFriend.closeStats == TRUE) {
            sFriend.closeStats = FALSE;
            CloseStatsConnection();
        }
    }
    if (sFriend.control->pGpObj != NULL && *sFriend.control->pGpObj != NULL) {
        if (DWCi_GPProcess() != 0) {
            return;
        }
        if (sFriend.control->state == 0) {
            return;
        }
        if (sFriend.control->friendList != NULL && sFriend.control->buddyUpdateState != 3 &&
            sFriend.control->gpProcessCount > 7) {
            if (sFriend.control->buddyUpdateState <= 1) {
                DWCi_UpdateFriendReq(sFriend.control->friendList, sFriend.control->friendListLen);
            }
            if (sFriend.control->buddyUpdateIdx >= sFriend.control->friendListLen) {
                sFriend.control->buddyUpdateState = 3;
                sFriend.control->updateSteps++;
            }
        }
    }
    if (sFriend.control->updateSteps >= 2) {
        if (sFriend.control->pGpObj != NULL && *sFriend.control->pGpObj != NULL) {
            count = 0;
            added = 0;
            for (i = 0; i < sFriend.control->friendListLen; i++) {
                if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID) {
                    count++;
                }
            }
            if (count == 0) {
                sFriend.control->updateSteps = 0;
                DWCi_EndUpdateServers();
                return;
            }
            sFriend.control->profileIds = DWC_Alloc(4, count * sizeof(int));
            if (sFriend.control->profileIds == NULL) {
                DWCi_StopFriendProcess(DWC_ERROR_FATAL, -70001);
                return;
            }
            for (i = 0; i < sFriend.control->friendListLen; i++) {
                if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID) {
                    sFriend.control->profileIds[added] =
                        DWC_GetGsProfileId(DWCi_GetUserData(), &sFriend.control->friendList[i]);
                    added++;
                }
            }
            if (DWCi_HandleFriendGPError(
                    gpGetReverseBuddiesList(sFriend.control->pGpObj, sFriend.control->profileIds, count, 0,
                                            (GPCallback)DWCi_GPGetReverseBuddiesListCallback, NULL)) != 0) {
                return;
            }
        }
        sFriend.control->updateSteps = 0;
    }
}

BOOL DWCi_IsUpdatingServers(void) {
    BOOL result = FALSE;

    if (sFriend.control != NULL && sFriend.control->state == 1) {
        result = TRUE;
    }
    return result;
}

void DWCi_UpdateServersAsync(const char *authToken, const char *partnerChallenge,
                             DWCUpdateServersCallback updateCallback, void *updateParam,
                             DWCFriendStatusCallback statusCallback, void *statusParam,
                             DWCDeleteFriendListCallback deleteCallback, void *deleteParam) {
    sFriend.control->updateCallback = updateCallback;
    sFriend.control->updateParam = updateParam;
    sFriend.control->statusCallback = statusCallback;
    sFriend.control->statusParam = statusParam;
    sFriend.control->deleteCallback = deleteCallback;
    sFriend.control->deleteParam = deleteParam;
    sFriend.control->friendListChanged = 0;
    sFriend.control->buddyUpdateState = 0;
    sFriend.control->updateSteps = 0;
    sFriend.control->buddyUpdateIdx = 0;
    sFriend.control->state = 1;
    if (sFriend.control->friendList == NULL) {
        sFriend.control->updateSteps++;
    }
    sFriend.control->updateSteps++;
}

void DWCi_StopFriendProcess(DWCError error, int code) {
    if (sFriend.control != NULL && error != DWC_ERROR_NONE) {
        DWCi_SetError(error, code);
        if (DWCi_IsUpdatingServers()) {
            if (sFriend.control->profileIds != NULL) {
                DWC_Free(4, sFriend.control->profileIds, 0);
                sFriend.control->profileIds = NULL;
            }
            sFriend.control->updateCallback(error, sFriend.control->friendListChanged, sFriend.control->updateParam);
        }
        DWCi_CloseFriendProcess();
    }
}

void DWCi_GPRecvBuddyRequestCallback(GPConnection *connection, GPRecvBuddyRequestArg *arg, void *param) {
    if (sFriend.control->friendList != NULL) {
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
    } else if (sFriend.control->statusCallback != NULL) {
        gpGetBuddyStatus(connection, arg->index, &status);
        sFriend.control->statusCallback(index, status.status, status.locationString, sFriend.control->statusParam);
    }
}

int DWCi_GetProfileIDFromList(int index) {
    DWCFriendData *friendList = sFriend.control->friendList;
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

    if (sFriend.control == NULL || profileId == 0) {
        return -1;
    }
    for (i = 0; i < sFriend.control->friendListLen; i++) {
        if (profileId == DWCi_GetProfileIDFromList(i)) {
            return i;
        }
    }
    return -1;
}

void DWCi_InitGPProcessCount(void) {
    if (sFriend.control != NULL) {
        sFriend.control->gpProcessCount = 0;
        sFriend.control->lastGpProcess = OS_GetTick();
    }
}

int DWCi_SetGPStatus(int status, const char *statusString, const char *locationString) {
    if (sFriend.control == NULL || !DWCi_CheckLogin()) {
        return 0;
    }
    if (status == -1) {
        status = ((GPIConnection *)*sFriend.control->pGpObj)->lastStatusState;
    }
    if (statusString == NULL) {
        statusString = ((GPIConnection *)*sFriend.control->pGpObj)->lastStatusString;
    }
    if (locationString == NULL) {
        locationString = ((GPIConnection *)*sFriend.control->pGpObj)->lastLocationString;
    }
    return gpSetStatusA(sFriend.control->pGpObj, status, statusString, locationString);
}

// Tells the game that a friend became a buddy, and its status
static void DWCi_CallBuddyFriendCallback(int index) {
    GPBuddyStatus status;

    if (sFriend.control->buddyCallback != NULL && sFriend.control->state != 1) {
        sFriend.control->buddyCallback(index, sFriend.control->buddyParam);
    }
    if (sFriend.control->statusCallback != NULL) {
        u8 result = DWC_GetFriendStatus(&sFriend.control->friendList[index], status.locationString);

        sFriend.control->statusCallback(index, result, status.locationString, sFriend.control->statusParam);
    }
}

void DWCi_ShutdownFriend(void) {
    sFriend.control = NULL;
}

// Runs GP every 300 milliseconds
static int DWCi_GPProcess(void) {
    int result = 0;
    OSTick now = OS_GetTick();
    DWCFriendControl *control = sFriend.control;

    if (OS_TicksToMilliSeconds(now - control->lastGpProcess) >= 300) {
        control->gpProcessCount++;
        result = gpProcess(sFriend.control->pGpObj);
        sFriend.control->lastGpProcess = OS_GetTick();
    }
    return result;
}

static void DWCi_CloseFriendProcess(void) {
    if (sFriend.control != NULL) {
        sFriend.control->state = 0;
        sFriend.control->buddyUpdateState = 0;
        sFriend.control->updateSteps = 0;
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

    if (sFriend.control->buddyUpdateState == 0) {
        DWCi_HandleFriendGPError(gpGetNumBuddies(sFriend.control->pGpObj, &count));
        for (index = 0; index < count; index++) {
            DWCi_HandleFriendGPError(gpGetBuddyStatus(sFriend.control->pGpObj, index, &status));
            for (i = 0; i < friendListLen; i++) {
                if (status.profile == DWCi_GetProfileIDFromList(i)) {
                    if (!DWCi_Acc_IsBuddyData(&friendList[i])) {
                        DWC_SetGsProfileId(&friendList[i], status.profile);
                        DWCi_SetBuddyFriendData(&friendList[i]);
                        if (DWC_IsBuddyFriendData(&sFriend.control->friendList[i])) {
                            DWCi_Acc_SetFlag4(&sFriend.control->friendList[i], TRUE);
                        }
                        sFriend.control->friendListChanged = 1;
                    }
                    break;
                }
            }
            if (i == friendListLen) {
                DWCi_HandleFriendGPError(gpDeleteBuddy(sFriend.control->pGpObj, status.profile));
                count--;
                index--;
            }
        }
        sFriend.control->buddyUpdateState = 1;
    }
    for (; sFriend.control->buddyUpdateIdx < friendListLen; sFriend.control->buddyUpdateIdx++) {
        if (DWCi_Acc_IsFlag5(&friendList[sFriend.control->buddyUpdateIdx])) {
            continue;
        }
        {
            int profileId = DWCi_GetProfileIDFromList(sFriend.control->buddyUpdateIdx);

            if (profileId != 0) {
                if (!DWCi_RefreshFriendListForth(friendList, sFriend.control->buddyUpdateIdx, profileId)) {
                    DWCi_HandleFriendGPError(gpGetBuddyIndex(sFriend.control->pGpObj, profileId, &index));
                    if (index == -1) {
                        DWCi_GPSendBuddyRequest(profileId);
                        DWCi_Acc_SetFlag5(&friendList[sFriend.control->buddyUpdateIdx], TRUE);
                    }
                }
            } else if (DWC_GetGsProfileId(DWCi_GetUserData(), &friendList[sFriend.control->buddyUpdateIdx]) == -1) {
                DWC_LoginIdToUserName(DWCi_GetUserData(), &friendList[sFriend.control->buddyUpdateIdx], loginName);
                gpProfileSearchA(sFriend.control->pGpObj, NULL, NULL, NULL, NULL, loginName, 0, 0,
                                 (GPCallback)DWCi_GPProfileSearchCallback, (void *)sFriend.control->buddyUpdateIdx);
                sFriend.control->buddyUpdateState = 2;
                return;
            }
        }
    }
}

static void DWCi_EndUpdateServers(void) {
    sFriend.control->updateCallback(DWC_ERROR_NONE, sFriend.control->friendListChanged, sFriend.control->updateParam);
    sFriend.control->state = 2;
}

// Deletes a friend that is a copy of the one at srcIndex
static void DWCi_DeleteFriendFromList(DWCFriendData *friendList, int index, int srcIndex) {
    if (sFriend.control != NULL) {
        MI_CpuFill8(&friendList[index], 0, sizeof(DWCFriendData));
        if (sFriend.control->deleteCallback != NULL) {
            sFriend.control->deleteCallback(index, srcIndex, sFriend.control->deleteParam);
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
            sFriend.control->friendListChanged = 1;
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
                    if (DWC_IsBuddyFriendData(&sFriend.control->friendList[i])) {
                        DWCi_Acc_SetFlag4(&sFriend.control->friendList[i], TRUE);
                    }
                }
                DWCi_DeleteFriendFromList(friendList, j, i);
                sFriend.control->friendListChanged = 1;
            }
        }
    }
    return found;
}

static int DWCi_GPSendBuddyRequest(int profileId) {
    int result = gpSendBuddyRequestA(sFriend.control->pGpObj, profileId, "");

    DWCi_HandleFriendGPError(result);
    return result;
}

static BOOL DWCi_GetFriendBuddyStatus(const DWCFriendData *friendData, GPBuddyStatus *status) {
    BOOL result = FALSE;
    int index = 0;
    int profileId;

    if (sFriend.control == NULL || !DWCi_CheckLogin()) {
        return FALSE;
    }
    profileId = DWC_GetGsProfileId(DWCi_GetUserData(), friendData);
    if (profileId > 0) {
        if (gpGetBuddyIndex(sFriend.control->pGpObj, profileId, &index) != 0) {
            // style-ok: goto-return BW1's DWC decompilation (aletheia, dwc_friend.c) exits the same way
            goto end;
        }
    }
    if (!(profileId > 0 && index != -1)) {
        result = FALSE;
    } else {
        int error = gpGetBuddyStatus(sFriend.control->pGpObj, index, status);

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
    int index = (int)param;
    int i;
    int buddyIndex;

    if (arg->result == 0 && arg->numMatches != 0 &&
        DWC_GetFriendDataType(&sFriend.control->friendList[index]) != DWC_ACC_FRIENDDATA_NODATA) {
        if (sFriend.control->state != 1) {
            return;
        }
        for (i = 0; i < arg->numMatches; i++) {
            if (DWCi_RefreshFriendListForth(sFriend.control->friendList, index, arg->matches[i].profile)) {
                sFriend.control->buddyUpdateIdx++;
                sFriend.control->buddyUpdateState = 1;
                arg->more = GP_DONE;
                return;
            }
        }
        for (i = 0; i < arg->numMatches; i++) {
            DWCi_HandleFriendGPError(gpGetBuddyIndex(connection, arg->matches[i].profile, &buddyIndex));
            if (buddyIndex == -1) {
                DWCi_GPSendBuddyRequest(arg->matches[i].profile);
                DWCi_Acc_SetFlag5(&sFriend.control->friendList[index], TRUE);
            } else {
                DWC_SetGsProfileId(&sFriend.control->friendList[index], arg->matches[0].profile);
                DWCi_SetBuddyFriendData(&sFriend.control->friendList[index]);
                DWCi_Acc_SetBuddyFlag(&sFriend.control->friendList[index], TRUE);
                if (DWC_IsBuddyFriendData(&sFriend.control->friendList[index])) {
                    DWCi_Acc_SetFlag4(&sFriend.control->friendList[index], TRUE);
                }
                DWCi_CallBuddyFriendCallback(index);
                sFriend.control->buddyUpdateIdx++;
                sFriend.control->buddyUpdateState = 1;
                arg->more = GP_DONE;
                sFriend.control->friendListChanged = 1;
                return;
            }
        }
        if (arg->more != GP_MORE) {
            sFriend.control->buddyUpdateIdx++;
            sFriend.control->buddyUpdateState = 1;
        }
        return;
    }
    if (arg->result != 0) {
        if (DWCi_HandleFriendGPError(arg->result) != 0) {
            return;
        }
    } else if (sFriend.control->state == 1 ||
               DWC_GetFriendDataType(&sFriend.control->friendList[index]) == DWC_ACC_FRIENDDATA_NODATA) {
        sFriend.control->buddyUpdateIdx++;
        sFriend.control->buddyUpdateState = 1;
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
    for (i = 0; i < sFriend.control->friendListLen; i++) {
        if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_ACC_FRIENDDATA_LOGIN_ID) {
            DWC_LoginIdToUserName(DWCi_GetUserData(), &sFriend.control->friendList[i], loginName);
            if (strcmp(loginName, arg->lastname) == 0) {
                if (DWC_IsBuddyFriendData(&sFriend.control->friendList[i])) {
                    wasBuddy = TRUE;
                }
                gpAuthBuddyRequest(connection, arg->profile);
                DWC_SetGsProfileId(&sFriend.control->friendList[i], arg->profile);
                found = TRUE;
            }
        } else if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID ||
                   DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_ACC_FRIENDDATA_FRIEND_KEY) {
            u32 code = DWCi_GetUserData()->gamecode;

            OS_SNPrintf(gameCode, 5, "%c%c%c%c", (u8)(code >> 24), (u8)(code >> 16), (u8)(code >> 8), (u8)code);
            if (arg->profile == DWC_GetGsProfileId(DWCi_GetUserData(), &sFriend.control->friendList[i]) &&
                memcmp(gameCode, &arg->lastname[9], 4) == 0) {
                if (DWC_IsBuddyFriendData(&sFriend.control->friendList[i])) {
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
        index = DWCi_RefreshFriendListAll(sFriend.control->friendList, sFriend.control->friendListLen, arg->profile);
        DWCi_Acc_SetBuddyFlag(&sFriend.control->friendList[index], TRUE);
        if (DWC_IsBuddyFriendData(&sFriend.control->friendList[index])) {
            DWCi_Acc_SetFlag4(&sFriend.control->friendList[index], TRUE);
            if (!wasBuddy) {
                DWCi_CallBuddyFriendCallback(index);
                sFriend.control->friendListChanged = 1;
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
        for (i = 0; i < sFriend.control->friendListLen; i++) {
            if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_ACC_FRIENDDATA_LOGIN_ID) {
                DWC_LoginIdToUserName(DWCi_GetUserData(), &sFriend.control->friendList[i], loginName);
                if (strcmp(loginName, arg->lastname) == 0) {
                    DWC_SetGsProfileId(&sFriend.control->friendList[i], arg->profile);
                    DWCi_SetBuddyFriendData(&sFriend.control->friendList[i]);
                    found = TRUE;
                }
            } else if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID ||
                       DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_ACC_FRIENDDATA_FRIEND_KEY) {
                if (arg->profile == DWC_GetGsProfileId(DWCi_GetUserData(), &sFriend.control->friendList[i])) {
                    if (DWCi_Acc_IsBuddyData(&sFriend.control->friendList[i]) == TRUE) {
                        wasAuthentic = TRUE;
                    } else {
                        DWC_SetGsProfileId(&sFriend.control->friendList[i], arg->profile);
                        DWCi_SetBuddyFriendData(&sFriend.control->friendList[i]);
                        found = TRUE;
                    }
                }
            }
        }
        if (found) {
            int index =
                DWCi_RefreshFriendListAll(sFriend.control->friendList, sFriend.control->friendListLen, arg->profile);

            if (!wasAuthentic) {
                DWCi_Acc_SetBuddyFlag(&sFriend.control->friendList[index], TRUE);
                if (DWC_IsBuddyFriendData(&sFriend.control->friendList[index])) {
                    DWCi_Acc_SetFlag4(&sFriend.control->friendList[index], TRUE);
                }
                DWCi_CallBuddyFriendCallback(index);
            }
            sFriend.control->friendListChanged = 1;
        }
    }
}

static int DWCi_GetPersCallbackLevel(void) {
    return sFriend.control->persLogin;
}

void DWCi_BeginCloseStatsConnection(void) {
    sFriend.unk_10 = TRUE;
}

void DWCi_EndCloseStatsConnection(void) {
    sFriend.unk_10 = FALSE;
}

// The reverse buddies of the profile IDs: a friend whose profile has the player as a buddy gets the buddy flag, and
// one that hasn't loses it
static void DWCi_GPGetReverseBuddiesListCallback(GPConnection *connection, GPGetReverseBuddiesListResponseArg *arg,
                                                 void *param) {
    int i;
    int j;

    if (DWCi_IsUpdatingServers()) {
        if (sFriend.control->profileIds != NULL) {
            DWC_Free(4, sFriend.control->profileIds, 0);
            sFriend.control->profileIds = NULL;
        }
        if (arg->result != 0) {
            DWCi_EndUpdateServers();
            return;
        }
        for (i = 0; i < sFriend.control->friendListLen; i++) {
            if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_ACC_FRIENDDATA_GS_PROFILE_ID) {
                for (j = 0; j < arg->numOfUniqueMatches; j++) {
                    if (arg->matches[j].profile ==
                        DWC_GetGsProfileId(DWCi_GetUserData(), &sFriend.control->friendList[i])) {
                        if (!DWCi_Acc_IsBuddyFlag(&sFriend.control->friendList[i])) {
                            sFriend.control->friendListChanged = 1;
                            DWCi_Acc_SetBuddyFlag(&sFriend.control->friendList[i], TRUE);
                            if (DWC_IsBuddyFriendData(&sFriend.control->friendList[i])) {
                                DWCi_Acc_SetFlag4(&sFriend.control->friendList[i], TRUE);
                            }
                        }
                        break;
                    }
                }
                if (j == arg->numOfUniqueMatches && DWCi_Acc_IsBuddyFlag(&sFriend.control->friendList[i]) == TRUE) {
                    sFriend.control->friendListChanged = 1;
                    DWCi_Acc_SetBuddyFlag(&sFriend.control->friendList[i], FALSE);
                }
                if (DWC_IsBuddyFriendData(&sFriend.control->friendList[i])) {
                    DWCi_Acc_SetFlag4(&sFriend.control->friendList[i], TRUE);
                }
            }
        }
        DWCi_EndUpdateServers();
    }
}

void DWCi_LoginInit(DWCLoginControl *control, DWCUserData *userData, GPConnection *connection, int productId,
                    u32 gameCode, const u16 *playerName, DWCLoginCallback callback, void *param) {
    sLogin = control;
    MI_CpuFill8(control, 0, sizeof(DWCLoginControl));
    sLogin->pGpObj = connection;
    sLogin->state = 0;
    sLogin->productID = productId;
    sLogin->gamecode = gameCode;
    sLogin->playerName = playerName;
    sLogin->callback = callback;
    sLogin->param = param;
    sLogin->userdata = userData;
    DWCi_Auth_HasConsoleUserId();
}

BOOL DWCi_LoginAsync(void) {
    if (DWCi_RemoteLogin()) {
        sLogin->state = 1;
        sLogin->connectFlag = FALSE;
        return TRUE;
    }
    return FALSE;
}

void DWCi_LoginProcess(void) {
    if (sLogin != NULL && !DWCi_IsError()) {
        switch (sLogin->state) {
        case 0:
        case 5:
            break;
        case 1:
            DWCi_RemoteLoginProcess();
            return;
        case 2:
        case 3:
        case 4:
            if (sLogin->pGpObj != NULL && *sLogin->pGpObj != NULL) {
                gpProcess(sLogin->pGpObj);
            }
            if (sLogin->connectFlag && OS_TicksToMilliSeconds(OS_GetTick() - sLogin->connectTick) > 60000) {
                DWCi_StopLogin(DWC_ERROR_NETWORK, -61070);
                sLogin->connectFlag = FALSE;
            }
            break;
        }
    }
}

DWCUserData *DWCi_GetUserData(void) {
    if (sLogin != NULL) {
        return sLogin->userdata;
    }
    return NULL;
}

void DWCi_StopLogin(DWCError error, int code) {
    if (sLogin != NULL && error != DWC_ERROR_NONE) {
        DWCi_SetError(error, code);
        if (sLogin->callback != NULL) {
            sLogin->callback(error, 0, sLogin->param);
        }
        DWCi_CloseLogin();
    }
}

void DWCi_ShutdownLogin(void) {
    if (!DWCi_Auth_IsFinished()) {
        DWCi_Auth_AbortAuthentication();
    }
    sLogin = NULL;
}

static void DWCi_CloseLogin(void) {
    if (sLogin != NULL) {
        DWCi_Auth_EndAuthentication();
        sLogin->state = 0;
        sLogin->connectFlag = FALSE;
    }
}

// Ends the login for an error of GP, returning it
static int DWCi_HandleLoginGPError(int result) {
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
    DWCi_StopLogin(error, code - 61000);
    return result;
}

// GP's login: it sets the profile's info and, logged in with the user data's profile, ends the login, or else looks
// up the info of the profile that it logged in to
static void DWCi_GPConnectCallback(GPConnection *connection, GPConnectResponseArg *arg, void *param) {
    char consoleId[0x21];
    char userId[0x33];

    sLogin->connectFlag = FALSE;
    OS_SNPrintf(consoleId, 0x1f, "%s:%016llu", DWCi_Np_GetConsoleName(), DWCi_Np_GetConsoleId());
    if (DWCi_HandleLoginGPError(gpSetInfosA(connection, GP_INFO_FIRSTNAME, consoleId)) != 0) {
        return;
    }
    DWCi_Acc_U64ToString32(DWCi_Auth_GetConsoleUserId(), 43, userId);
    if (DWCi_HandleLoginGPError(gpSetInfosA(connection, GP_INFO_AIMNAME, userId)) != 0) {
        return;
    }
    if (DWCi_HandleLoginGPError(gpSetInfosA(connection, GP_INFO_ZIPCODE, DWCi_Np_GetAppGameName())) != 0) {
        return;
    }
    if (arg->result == 0) {
        if (sLogin->state == 2) {
            if (sLogin->userdata->gs_profile_id == arg->profile) {
                sLogin->state = 5;
                if (DWCi_HandleLoginGPError(DWCi_GPSetServerStatus(1)) != 0) {
                    return;
                }
                sLogin->callback(DWC_ERROR_NONE, arg->profile, sLogin->param);
                if (DWCi_GT2Startup() != 0) {
                    return;
                }
                DWCi_SetMatchProfileID(arg->profile);
                DWCi_RegisterMatchQR2Keys();
            } else {
                DWCi_StopLogin(DWC_ERROR_NETWORK, -60000);
            }
        } else if (sLogin->state == 3) {
            if (DWCi_HandleLoginGPError(
                    gpGetInfo(connection, arg->profile, 0, 0, (GPCallback)DWCi_GPGetInfoCallback, NULL)) != 0) {
                return;
            }
        }
    } else {
        DWCi_HandleLoginGPError(arg->result);
    }
}

// Connects to GP with the authentication server's token, going to the given state
static void DWCi_GPConnect(GPCallback callback, int nextState) {
    OSTick now = OS_GetTick();

    sLogin->connectTick = now;
    sLogin->connectFlag = TRUE;
    if (DWCi_HandleLoginGPError(gpConnectPreAuthenticatedA(sLogin->pGpObj, sLogin->authToken, sLogin->partnerChallenge,
                                                           1, 0, callback, NULL)) == 0) {
        sLogin->state = nextState;
    }
}

// Starts the login to the authentication server with the user data's login ID, or a temporary one until it has one
static BOOL DWCi_RemoteLogin(void) {
    u64 userId;

    if (DWCi_Acc_IsAuthentic(sLogin->userdata)) {
        DWCi_Acc_LoginIdToUserName(&sLogin->userdata->authentic, sLogin->userdata->gamecode, sLogin->username);
        userId = DWCi_Acc_GetUserId(&sLogin->userdata->authentic);
    } else {
        if (!DWCi_Acc_IsValidLoginId(&sLogin->tempLoginId)) {
            if (DWCi_Acc_CheckConsoleUserId(&sLogin->userdata->pseudo)) {
                sLogin->tempLoginId = sLogin->userdata->pseudo;
            } else {
                DWCi_Acc_CreateTempLoginId(&sLogin->tempLoginId);
            }
        } else {
            DWCi_Acc_SetPlayerId(&sLogin->tempLoginId, (OS_GetTick() * 0x5d588b656c078965 + 0x269ec3) >> 32);
        }
        DWCi_Acc_LoginIdToUserName(&sLogin->tempLoginId, sLogin->gamecode, sLogin->username);
        userId = 0;
    }
    return DWCi_Auth_StartAuthentication(sLogin->playerName, &sLogin->username[9], userId, (DWCAuthAlloc)DWC_Alloc,
                                         (DWCAuthFree)DWC_Free);
}

// Waits for the authentication server, then connects to GP with the profile of the user data, or looks for it
static void DWCi_RemoteLoginProcess(void) {
    DWCi_Auth_ProcessAuthentication();
    if (DWCi_Auth_IsFinished()) {
        if (DWCi_Auth_IsSucceeded()) {
            DWCi_Auth_GetGameSpyToken(sLogin->authToken, sLogin->partnerChallenge);
            DWCi_Auth_EndAuthentication();
            if (DWCi_Acc_IsAuthentic(sLogin->userdata)) {
                DWCi_GPConnect((GPCallback)DWCi_GPConnectCallback, 2);
                return;
            }
            DWCi_Acc_SetUserId(&sLogin->tempLoginId, DWCi_Auth_GetConsoleUserId());
            DWCi_GPConnect((GPCallback)DWCi_GPConnectCallback, 3);
            return;
        }
        {
            int error = DWCi_Auth_GetErrorCode();

            if (error <= -29000) {
                DWCi_StopLogin(DWC_ERROR_FATAL, error);
                return;
            }
            DWCi_StopLogin(DWC_ERROR_AUTH_ANY, error);
        }
    }
}

// The info of the profile that GP logged in to without the user data's: a new profile gets the login name, and once
// the profile has it, the login ID is authenticated for the profile and GP logs in again with it
static void DWCi_GPGetInfoCallback(GPConnection *connection, GPGetInfoResponseArg *arg, void *param) {
    char loginName[0x16];
    char userName[0x15];
    char tempName[0x15];

    if (arg->result == 0) {
        if (sLogin->state == 3) {
            if (arg->lastname[0] == 0) {
                DWCi_Acc_LoginIdToUserName(&sLogin->userdata->pseudo, sLogin->gamecode, loginName);
                if (DWCi_HandleLoginGPError(gpSetInfosA(connection, GP_INFO_LASTNAME, loginName)) != 0) {
                    return;
                }
                sLogin->state = 4;
                if (DWCi_HandleLoginGPError(
                        gpGetInfo(connection, arg->profile, 0, 0, (GPCallback)DWCi_GPGetInfoCallback, NULL)) != 0) {
                    return;
                }
            } else {
                gpDisconnect(connection);
                DWCi_RemoteLogin();
                sLogin->state = 1;
            }
        } else if (sLogin->state == 4) {
            DWCi_Acc_LoginIdToUserName(&sLogin->userdata->pseudo, sLogin->gamecode, userName);
            if (strcmp(arg->lastname, userName) == 0) {
                DWCi_Acc_LoginIdToUserName(&sLogin->tempLoginId, sLogin->gamecode, tempName);
                DWCi_Acc_SetLoginIdToUserData(sLogin->userdata, &sLogin->tempLoginId, arg->profile);
                gpDisconnect(connection);
                DWCi_GPConnect((GPCallback)DWCi_GPConnectCallback, 2);
                return;
            }
            if (DWCi_HandleLoginGPError(
                    gpGetInfo(connection, arg->profile, 0, 0, (GPCallback)DWCi_GPGetInfoCallback, NULL)) != 0) {
                return;
            }
        }
    }
}

// Whether GP is logged in
static BOOL DWCi_CheckLogin(void) {
    if (sLogin != NULL && sLogin->state == 5) {
        return TRUE;
    }
    return FALSE;
}

BOOL DWCi_GetAuthInfo(char **authToken, char **challenge) {
    if (!DWCi_CheckLogin()) {
        return FALSE;
    }
    *authToken = sLogin->authToken;
    *challenge = sLogin->partnerChallenge;
    return TRUE;
}

BOOL DWCi_GetLoginTicket(SakeLoginTicket *ticket) {
    if (!DWCi_CheckLogin()) {
        return FALSE;
    }
    if (gpGetLoginTicket(sLogin->pGpObj, ticket->data) == 0) {
        return TRUE;
    }
    return FALSE;
}
