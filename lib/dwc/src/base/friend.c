// Overlay 11's friend list and login (friend.c, a guessed name: the ROM embeds none for DWC's files; Thumb). It keeps
// the server's date from the authentication server's answer, reads and writes the texts of the friends' statuses, keeps
// the friend list in step with GameSpy's buddy list and logs in to GameSpy's GP with the authentication server's
// token. The layouts are in dwc/friend.h.

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

static u64 func_ov011_0215dd74(void);
static const char *func_ov011_0215dd7c(void);
static char *func_ov011_0215dd84(void);
static u8 func_ov011_0215e344(const DWCFriendData *friendData, char *location);
static u8 func_ov011_0215e354(const DWCFriendData *friendData, u8 *scm, u8 *scn, char *location);
static u8 func_ov011_0215e420(const DWCFriendData *friendData, u8 *scm, u8 *scn, char *statusData, int *size);
static void func_ov011_0215e980(int index);
static int func_ov011_0215e9dc(void);
static void func_ov011_0215ea34(void);
static void func_ov011_0215ea50(DWCFriendData *friendList, int friendListLen);
static void func_ov011_0215ebf8(void);
static void func_ov011_0215ec14(DWCFriendData *friendList, int index, int srcIndex);
static BOOL func_ov011_0215ec48(DWCFriendData *friendList, int index, int profileId);
static int func_ov011_0215ecbc(DWCFriendData *friendList, int friendListLen, int profileId);
static int func_ov011_0215ed88(int profileId);
static BOOL func_ov011_0215edac(const DWCFriendData *friendData, GPBuddyStatus *status);
static int func_ov011_0215ee18(int result);
static void func_ov011_0215ee6c(GPConnection *connection, GPProfileSearchResponseArg *arg, void *param);
static void func_ov011_0215f004(GPConnection *connection, GPGetInfoResponseArg *arg, void *param);
static void func_ov011_0215f198(GPConnection *connection, GPGetInfoResponseArg *arg, void *param);
static int func_ov011_0215f2b4(void);
static void func_ov011_0215f2d8(GPConnection *connection, GPGetReverseBuddiesListResponseArg *arg, void *param);
static void func_ov011_0215f57c(void);
static int func_ov011_0215f59c(int result);
static void func_ov011_0215f5f0(GPConnection *connection, GPConnectResponseArg *arg, void *param);
static void func_ov011_0215f700(GPCallback callback, int nextState);
static BOOL func_ov011_0215f748(void);
static void func_ov011_0215f820(void);
static void func_ov011_0215f8a8(GPConnection *connection, GPGetInfoResponseArg *arg, void *param);
static BOOL func_ov011_0215f998(void);

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

static u64 func_ov011_0215dd74(void) {
    return 0;
}

static const char *func_ov011_0215dd7c(void) {
    return "NDS";
}

// The game's code from the ROM's header, as a text
static char *func_ov011_0215dd84(void) {
    static char gameCode[5];

    MI_CpuCopy8((void *)(HW_ROM_HEADER_BUF + 0xc), gameCode, 4);
    gameCode[4] = 0;
    return gameCode;
}

BOOL func_ov011_0215dda8(RTCDate *date, RTCTime *time) {
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

BOOL func_ov011_0215de44(const char *apName, void *buffer) {
    DWCBmWifiId id;
    BOOL started;

    DWCBm_GetWifiId(&id);
    if (!id.isSet) {
        return FALSE;
    }
    if (!func_ov011_0216c154(DWC_Alloc, DWC_Free)) {
        return FALSE;
    }
    if (strlen(apName) == 0) {
        started = func_ov011_0216c1cc("0000", buffer);
    } else {
        started = func_ov011_0216c1cc(apName, buffer);
    }
    if (!started) {
        func_ov011_0216c1a8();
        return FALSE;
    }
    return TRUE;
}

int func_ov011_0215dea8(void) {
    int state = func_ov011_0216c344();

    if ((u32)(state - 3) <= 2) {
        func_ov011_0216c1a8();
    }
    return state;
}

BOOL func_ov011_0215dec0(void) {
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
    sNasLogin->param.alloc = DWC_Alloc;
    sNasLogin->param.free = DWC_Free;
    func_ov011_021694ec(&sNasLogin->param, &sNasLogin->http);
    return TRUE;
}

int func_ov011_0215df40(void) {
    DWCAuthResult result;

    if (sNasLogin == NULL) {
        return 0;
    }
    switch (func_ov011_021696d0()) {
    case 0:
        return 2;
    case 0x15:
        MI_CpuFill8(&result, 0, sizeof(result));
        func_ov011_02169704(&result);
        func_ov011_02169668();
        if (result.returnCode == 20002) {
            func_ov011_021694ec(&sNasLogin->param, &sNasLogin->http);
            return 2;
        }
        DWC_Free(4, sNasLogin, sizeof(NasLogin));
        sNasLogin = NULL;
        return 3;
    case 0x14:
        func_ov011_02169668();
        DWC_Free(4, sNasLogin, sizeof(NasLogin));
        sNasLogin = NULL;
        return 5;
    default:
        func_ov011_02169704(&result);
        func_ov011_02169668();
        DWC_Free(4, sNasLogin, sizeof(NasLogin));
        sNasLogin = NULL;
        DWCi_SetError(DWC_ERROR_AUTH_ANY, result.returnCode);
        return 4;
    }
}

BOOL func_ov011_0215dffc(const char *dateHeader) {
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

int func_ov011_0215e160(const char *key, const char *value, char *out, char sep) {
    OS_SNPrintf(out, 0x1000, "%c%s%c%s", sep, key, sep, value);
    return strlen(out);
}

int func_ov011_0215e188(const char *key, const char *value, char *out, char sep) {
    func_ov011_0215e160(key, value, strchr(out, 0), sep);
    return strlen(out);
}

int func_ov011_0215e1b0(const char *key, char *dest, const char *text, char sep) {
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

u32 func_ov011_0215e25c(u32 max) {
    if (sRandom.x == 0 && sRandom.mul == 0 && sRandom.add == 0) {
        u64 seed;

        OS_GetMacAddress((u8 *)&seed);
        seed = (seed >> 24 & 0xffffff) | OS_GetTick() << 24;
        MATH_InitRand32(&sRandom, seed);
    }
    return MATH_Rand32(&sRandom, max);
}

u32 func_ov011_0215e32c(const u16 *str) {
    u32 length = 0;

    while (str[length] != 0) {
        length++;
    }
    return length;
}

static u8 func_ov011_0215e344(const DWCFriendData *friendData, char *location) {
    return func_ov011_0215e354(friendData, NULL, NULL, location);
}

// A friend's status, and from a status text of the matching (status 6) its SCM and SCN values
static u8 func_ov011_0215e354(const DWCFriendData *friendData, u8 *scm, u8 *scn, char *location) {
    char number[4];
    GPBuddyStatus status;
    u8 result;

    if (func_ov011_0215edac(friendData, &status)) {
        if (status.status == 6) {
            if (scm != NULL) {
                if (func_ov011_0215e1b0("SCM", number, status.statusString, '/') > 0) {
                    *scm = strtoul(number, NULL, 10);
                } else {
                    *scm = 0;
                }
            }
            if (scn != NULL) {
                if (func_ov011_0215e1b0("SCN", number, status.statusString, '/') > 0) {
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
        if (func_ov011_021686c0(&status)) {
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

u8 func_ov011_0215e410(const DWCFriendData *friendData, char *statusData, int *size) {
    return func_ov011_0215e420(friendData, NULL, NULL, statusData, size);
}

static u8 func_ov011_0215e420(const DWCFriendData *friendData, u8 *scm, u8 *scn, char *statusData, int *size) {
    char encoded[0x100];
    u8 status = func_ov011_0215e354(friendData, scm, scn, encoded);

    if (status == 0) {
        *size = -1;
        return status;
    }
    *size = DWC_Base64Decode(encoded, strlen(encoded), NULL, 0);
    if (statusData == NULL || *size == -1) {
        return status;
    }
    DWC_Base64Decode(encoded, strlen(encoded), (u8 *)statusData, *size);
    return status;
}

BOOL func_ov011_0215e47c(const char *statusData, int size) {
    char encoded[0x100];
    int length;
    BOOL result;

    if (sFriend.control == NULL || !func_ov011_0215f998()) {
        return FALSE;
    }
    length = DWC_Base64Encode((const u8 *)statusData, size, encoded, 0xff);
    if (length == -1) {
        return FALSE;
    }
    result = FALSE;
    encoded[length] = 0;
    if (func_ov011_0215e914(-1, 0, encoded) == 0) {
        result = TRUE;
    }
    return result;
}

BOOL func_ov011_0215e4d4(void (*callback)(int index, void *param), void *param) {
    if (sFriend.control == NULL) {
        return FALSE;
    }
    sFriend.control->buddyCallback = callback;
    sFriend.control->buddyParam = param;
    return TRUE;
}

void func_ov011_0215e4f0(DWCFriendControl *control, GPConnection *connection, u32 unk_28, DWCFriendData *friendList,
                         int friendListLen) {
    int i = 0;

    sFriend.control = control;
    control->state = 0;
    sFriend.control->connection = connection;
    sFriend.control->gpProcessCount = 0;
    sFriend.control->lastGpProcess = 0;
    sFriend.control->friendListLen = friendListLen;
    sFriend.control->friendList = friendList;
    sFriend.control->buddyUpdateIdx = 0;
    sFriend.control->friendListChanged = 0;
    sFriend.control->buddyUpdateState = 0;
    sFriend.control->updateSteps = 0;
    sFriend.control->persLogin = 0;
    sFriend.control->unk_24 = 0;
    sFriend.control->unk_28 = unk_28;
    sFriend.control->updateCallback = NULL;
    sFriend.control->updateParam = NULL;
    sFriend.control->statusCallback = NULL;
    sFriend.control->statusParam = NULL;
    sFriend.control->deleteCallback = NULL;
    sFriend.control->deleteParam = NULL;
    sFriend.control->buddyCallback = NULL;
    sFriend.control->buddyParam = NULL;
    sFriend.control->unk_4c = 0;
    sFriend.control->unk_50 = 0;
    sFriend.control->unk_54 = 0;
    sFriend.control->unk_58 = 0;
    sFriend.control->profileIds = NULL;
    for (; i < friendListLen; i++) {
        DWCi_Acc_SetFlag5(&sFriend.control->friendList[i], FALSE);
    }
}

void func_ov011_0215e584(void) {
    int count;
    int added;
    int i;

    if (sFriend.control == NULL || DWCi_IsError()) {
        return;
    }
    if (sFriend.control->friendList == NULL) {
        if (sFriend.control->connection != NULL && *sFriend.control->connection != NULL) {
            func_ov011_0215e9dc();
        }
        return;
    }
    if (func_ov011_0215f2b4() != 0 || IsStatsConnected()) {
        sFriend.inPersistThink = TRUE;
        sFriend.closeStats = FALSE;
        PersistThink();
        sFriend.inPersistThink = FALSE;
        if (sFriend.closeStats == TRUE) {
            sFriend.closeStats = FALSE;
            CloseStatsConnection();
        }
    }
    if (sFriend.control->connection != NULL && *sFriend.control->connection != NULL) {
        if (func_ov011_0215e9dc() != 0) {
            return;
        }
        if (sFriend.control->state == 0) {
            return;
        }
        if (sFriend.control->friendList != NULL && sFriend.control->buddyUpdateState != 3 &&
            sFriend.control->gpProcessCount > 7) {
            if (sFriend.control->buddyUpdateState <= 1) {
                func_ov011_0215ea50(sFriend.control->friendList, sFriend.control->friendListLen);
            }
            if (sFriend.control->buddyUpdateIdx >= sFriend.control->friendListLen) {
                sFriend.control->buddyUpdateState = 3;
                sFriend.control->updateSteps++;
            }
        }
    }
    if (sFriend.control->updateSteps >= 2) {
        if (sFriend.control->connection != NULL && *sFriend.control->connection != NULL) {
            count = 0;
            added = 0;
            for (i = 0; i < sFriend.control->friendListLen; i++) {
                if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_FRIEND_TYPE_PROFILE_ID) {
                    count++;
                }
            }
            if (count == 0) {
                sFriend.control->updateSteps = 0;
                func_ov011_0215ebf8();
                return;
            }
            sFriend.control->profileIds = DWC_Alloc(4, count * sizeof(int));
            if (sFriend.control->profileIds == NULL) {
                func_ov011_0215e784(DWC_ERROR_FATAL, -70001);
                return;
            }
            for (i = 0; i < sFriend.control->friendListLen; i++) {
                if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_FRIEND_TYPE_PROFILE_ID) {
                    sFriend.control->profileIds[added] =
                        DWC_GetGsProfileId(func_ov011_0215f51c(), &sFriend.control->friendList[i]);
                    added++;
                }
            }
            if (func_ov011_0215ee18(gpGetReverseBuddiesList(sFriend.control->connection, sFriend.control->profileIds,
                                                            count, 0, (GPCallback)func_ov011_0215f2d8, NULL)) != 0) {
                return;
            }
        }
        sFriend.control->updateSteps = 0;
    }
}

BOOL func_ov011_0215e714(void) {
    BOOL result = FALSE;

    if (sFriend.control != NULL && sFriend.control->state == 1) {
        result = TRUE;
    }
    return result;
}

void func_ov011_0215e72c(u32 unused1, u32 unused2, DWCUpdateServersCallback updateCallback, void *updateParam,
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

void func_ov011_0215e784(int error, int code) {
    if (sFriend.control != NULL && error != DWC_ERROR_NONE) {
        DWCi_SetError(error, code);
        if (func_ov011_0215e714()) {
            if (sFriend.control->profileIds != NULL) {
                DWC_Free(4, sFriend.control->profileIds, 0);
                sFriend.control->profileIds = NULL;
            }
            sFriend.control->updateCallback(error, sFriend.control->friendListChanged, sFriend.control->updateParam);
        }
        func_ov011_0215ea34();
    }
}

void func_ov011_0215e7d0(GPConnection *connection, GPRecvBuddyRequestArg *arg, void *param) {
    if (sFriend.control->friendList != NULL) {
        gpGetInfo(connection, arg->profile, 0, 0, (GPCallback)func_ov011_0215f004, NULL);
    }
}

BOOL func_ov011_0215e7fc(GPConnection *connection, GPRecvBuddyAuthArg *arg, void *param) {
    gpGetInfo(connection, arg->profile, 0, 0, (GPCallback)func_ov011_0215f198, NULL);
    return TRUE;
}

void func_ov011_0215e81c(GPConnection *connection, GPRecvBuddyStatusArg *arg, void *param) {
    GPBuddyStatus status;
    int index = func_ov011_0215e8b4(arg->profile);

    if (index == -1) {
        if (gpIsBuddy(connection, arg->profile)) {
            gpDeleteBuddy(connection, arg->profile);
        }
    } else if (sFriend.control->statusCallback != NULL) {
        gpGetBuddyStatus(connection, arg->index, &status);
        sFriend.control->statusCallback(index, status.status, status.locationString, sFriend.control->statusParam);
    }
}

int func_ov011_0215e880(int index) {
    DWCFriendData *friendList = sFriend.control->friendList;
    int profileId;

    if (friendList == NULL) {
        return 0;
    }
    profileId = DWC_GetGsProfileId(func_ov011_0215f51c(), &friendList[index]);
    if (profileId != 0 && profileId != -1) {
        return profileId;
    }
    return 0;
}

int func_ov011_0215e8b4(int profileId) {
    int i;

    if (sFriend.control == NULL || profileId == 0) {
        return -1;
    }
    for (i = 0; i < sFriend.control->friendListLen; i++) {
        if (profileId == func_ov011_0215e880(i)) {
            return i;
        }
    }
    return -1;
}

void func_ov011_0215e8f4(void) {
    if (sFriend.control != NULL) {
        sFriend.control->gpProcessCount = 0;
        sFriend.control->lastGpProcess = OS_GetTick();
    }
}

int func_ov011_0215e914(int status, const char *statusString, const char *locationString) {
    if (sFriend.control == NULL || !func_ov011_0215f998()) {
        return 0;
    }
    if (status == -1) {
        status = ((GPIConnection *)*sFriend.control->connection)->lastStatusState;
    }
    if (statusString == NULL) {
        statusString = ((GPIConnection *)*sFriend.control->connection)->lastStatusString;
    }
    if (locationString == NULL) {
        locationString = ((GPIConnection *)*sFriend.control->connection)->lastLocationString;
    }
    return gpSetStatus(sFriend.control->connection, status, statusString, locationString);
}

// Tells the game that a friend became a buddy, and its status
static void func_ov011_0215e980(int index) {
    GPBuddyStatus status;

    if (sFriend.control->buddyCallback != NULL && sFriend.control->state != 1) {
        sFriend.control->buddyCallback(index, sFriend.control->buddyParam);
    }
    if (sFriend.control->statusCallback != NULL) {
        u8 result = func_ov011_0215e344(&sFriend.control->friendList[index], status.locationString);

        sFriend.control->statusCallback(index, result, status.locationString, sFriend.control->statusParam);
    }
}

void func_ov011_0215e9d0(void) {
    sFriend.control = NULL;
}

// Runs GP every 300 milliseconds
static int func_ov011_0215e9dc(void) {
    int result = 0;
    OSTick now = OS_GetTick();
    DWCFriendControl *control = sFriend.control;

    if (OS_TicksToMilliSeconds(now - control->lastGpProcess) >= 300) {
        control->gpProcessCount++;
        result = gpProcess(sFriend.control->connection);
        sFriend.control->lastGpProcess = OS_GetTick();
    }
    return result;
}

static void func_ov011_0215ea34(void) {
    if (sFriend.control != NULL) {
        sFriend.control->state = 0;
        sFriend.control->buddyUpdateState = 0;
        sFriend.control->updateSteps = 0;
    }
}

// Confirms the friends that are buddies, deletes the buddies that aren't friends, and asks the friends that aren't
// buddies to be, looking up the profile of a login ID first
static void func_ov011_0215ea50(DWCFriendData *friendList, int friendListLen) {
    int count;
    int index;
    char loginName[0x18];
    GPBuddyStatus status;
    int i;

    if (sFriend.control->buddyUpdateState == 0) {
        func_ov011_0215ee18(gpGetNumBuddies(sFriend.control->connection, &count));
        for (index = 0; index < count; index++) {
            func_ov011_0215ee18(gpGetBuddyStatus(sFriend.control->connection, index, &status));
            for (i = 0; i < friendListLen; i++) {
                if (status.profile == func_ov011_0215e880(i)) {
                    if (!DWCi_Acc_IsAuthentic(&friendList[i])) {
                        DWCi_Acc_CreateProfileIdToken(&friendList[i], status.profile);
                        DWCi_Acc_SetAuthentic(&friendList[i]);
                        if (DWC_IsBuddyFriendData(&sFriend.control->friendList[i])) {
                            DWCi_Acc_SetFlag4(&sFriend.control->friendList[i], TRUE);
                        }
                        sFriend.control->friendListChanged = 1;
                    }
                    break;
                }
            }
            if (i == friendListLen) {
                func_ov011_0215ee18(gpDeleteBuddy(sFriend.control->connection, status.profile));
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
            int profileId = func_ov011_0215e880(sFriend.control->buddyUpdateIdx);

            if (profileId != 0) {
                if (!func_ov011_0215ec48(friendList, sFriend.control->buddyUpdateIdx, profileId)) {
                    func_ov011_0215ee18(gpGetBuddyIndex(sFriend.control->connection, profileId, &index));
                    if (index == -1) {
                        func_ov011_0215ed88(profileId);
                        DWCi_Acc_SetFlag5(&friendList[sFriend.control->buddyUpdateIdx], TRUE);
                    }
                }
            } else if (DWC_GetGsProfileId(func_ov011_0215f51c(), &friendList[sFriend.control->buddyUpdateIdx]) == -1) {
                DWCi_Acc_GetLoginName(func_ov011_0215f51c(), &friendList[sFriend.control->buddyUpdateIdx], loginName);
                gpProfileSearch(sFriend.control->connection, NULL, NULL, NULL, NULL, loginName, 0, 0,
                                (GPCallback)func_ov011_0215ee6c, (void *)sFriend.control->buddyUpdateIdx);
                sFriend.control->buddyUpdateState = 2;
                return;
            }
        }
    }
}

static void func_ov011_0215ebf8(void) {
    sFriend.control->updateCallback(DWC_ERROR_NONE, sFriend.control->friendListChanged, sFriend.control->updateParam);
    sFriend.control->state = 2;
}

// Deletes a friend that is a copy of the one at srcIndex
static void func_ov011_0215ec14(DWCFriendData *friendList, int index, int srcIndex) {
    if (sFriend.control != NULL) {
        MI_CpuFill8(&friendList[index], 0, sizeof(DWCFriendData));
        if (sFriend.control->deleteCallback != NULL) {
            sFriend.control->deleteCallback(index, srcIndex, sFriend.control->deleteParam);
        }
    }
}

// Deletes a friend, or the earlier friend, that has the same profile ID, keeping the confirmed one
static BOOL func_ov011_0215ec48(DWCFriendData *friendList, int index, int profileId) {
    DWCFriendData *friendData = &friendList[index];
    int i;

    if (DWC_GetFriendDataType(friendData) == DWC_FRIEND_TYPE_LOGIN_ID) {
        return FALSE;
    }
    for (i = 0; i < index; i++) {
        int otherId = func_ov011_0215e880(i);

        if (otherId != 0 && otherId == profileId) {
            if (DWCi_Acc_IsAuthentic(friendData) && !DWCi_Acc_IsAuthentic(&friendList[i])) {
                func_ov011_0215ec14(friendList, i, index);
            } else {
                func_ov011_0215ec14(friendList, index, i);
            }
            sFriend.control->friendListChanged = 1;
            return TRUE;
        }
    }
    return FALSE;
}

// Merges the friends that have the same profile ID, returning the index of the one that has the given ID
static int func_ov011_0215ecbc(DWCFriendData *friendList, int friendListLen, int profileId) {
    int found = -1;
    int i;

    for (i = 0; i < friendListLen; i++) {
        int id = func_ov011_0215e880(i);
        int j;

        if (id == 0) {
            continue;
        }
        if (id == profileId) {
            found = i;
        }
        for (j = i + 1; j < friendListLen; j++) {
            if (id == func_ov011_0215e880(j)) {
                if (DWC_GetFriendDataType(&friendList[i]) == DWC_FRIEND_TYPE_FRIEND_KEY &&
                    DWC_GetFriendDataType(&friendList[j]) == DWC_FRIEND_TYPE_PROFILE_ID) {
                    DWCi_Acc_CreateProfileIdToken(&friendList[i], id);
                }
                if (DWCi_Acc_IsAuthentic(&friendList[j])) {
                    DWCi_Acc_SetAuthentic(&friendList[i]);
                    if (DWC_IsBuddyFriendData(&sFriend.control->friendList[i])) {
                        DWCi_Acc_SetFlag4(&sFriend.control->friendList[i], TRUE);
                    }
                }
                func_ov011_0215ec14(friendList, j, i);
                sFriend.control->friendListChanged = 1;
            }
        }
    }
    return found;
}

static int func_ov011_0215ed88(int profileId) {
    int result = gpSendBuddyRequest(sFriend.control->connection, profileId, "");

    func_ov011_0215ee18(result);
    return result;
}

static BOOL func_ov011_0215edac(const DWCFriendData *friendData, GPBuddyStatus *status) {
    int index = 0;
    BOOL result = FALSE;
    int profileId;

    if (sFriend.control == NULL || !func_ov011_0215f998()) {
        return FALSE;
    }
    profileId = DWC_GetGsProfileId(func_ov011_0215f51c(), friendData);
    if (profileId > 0 && gpGetBuddyIndex(sFriend.control->connection, profileId, &index)) {
        return result;
    }
    if (profileId <= 0 || index == -1) {
        return FALSE;
    }
    if (gpGetBuddyStatus(sFriend.control->connection, index, status) == 0) {
        result = TRUE;
    }
    return result;
}

// Ends the friend list's update for an error of GP, returning it
static int func_ov011_0215ee18(int result) {
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
    func_ov011_0215e784(error, code - 71000);
    return result;
}

// The profile search of a login ID: the friend becomes the profile, as a buddy if it is one already
static void func_ov011_0215ee6c(GPConnection *connection, GPProfileSearchResponseArg *arg, void *param) {
    int index = (int)param;
    int i;
    int buddyIndex;

    if (arg->result == 0 && arg->numMatches != 0 &&
        DWC_GetFriendDataType(&sFriend.control->friendList[index]) != DWC_FRIEND_TYPE_NONE) {
        if (sFriend.control->state != 1) {
            return;
        }
        for (i = 0; i < arg->numMatches; i++) {
            if (func_ov011_0215ec48(sFriend.control->friendList, index, arg->matches[i].profile)) {
                sFriend.control->buddyUpdateIdx++;
                sFriend.control->buddyUpdateState = 1;
                arg->more = GP_DONE;
                return;
            }
        }
        for (i = 0; i < arg->numMatches; i++) {
            func_ov011_0215ee18(gpGetBuddyIndex(connection, arg->matches[i].profile, &buddyIndex));
            if (buddyIndex == -1) {
                func_ov011_0215ed88(arg->matches[i].profile);
                DWCi_Acc_SetFlag5(&sFriend.control->friendList[index], TRUE);
            } else {
                DWCi_Acc_CreateProfileIdToken(&sFriend.control->friendList[index], arg->matches[0].profile);
                DWCi_Acc_SetAuthentic(&sFriend.control->friendList[index]);
                DWCi_Acc_SetBuddyFlag(&sFriend.control->friendList[index], TRUE);
                if (DWC_IsBuddyFriendData(&sFriend.control->friendList[index])) {
                    DWCi_Acc_SetFlag4(&sFriend.control->friendList[index], TRUE);
                }
                func_ov011_0215e980(index);
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
        if (func_ov011_0215ee18(arg->result) != 0) {
            return;
        }
    } else if (sFriend.control->state == 1 ||
               DWC_GetFriendDataType(&sFriend.control->friendList[index]) == DWC_FRIEND_TYPE_NONE) {
        sFriend.control->buddyUpdateIdx++;
        sFriend.control->buddyUpdateState = 1;
    }
}

// The info of a profile that asked to be a buddy: the request is accepted if the profile is a friend (by its login
// name in the last name, or by its profile ID and game code), and denied if not
static void func_ov011_0215f004(GPConnection *connection, GPGetInfoResponseArg *arg, void *param) {
    BOOL found = FALSE;
    BOOL wasBuddy = FALSE;
    char gameCode[5];
    char loginName[0x17];
    int i;

    if (arg->result != 0) {
        return;
    }
    for (i = 0; i < sFriend.control->friendListLen; i++) {
        if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_FRIEND_TYPE_LOGIN_ID) {
            DWCi_Acc_GetLoginName(func_ov011_0215f51c(), &sFriend.control->friendList[i], loginName);
            if (strcmp(loginName, arg->lastname) == 0) {
                if (DWC_IsBuddyFriendData(&sFriend.control->friendList[i])) {
                    wasBuddy = TRUE;
                }
                gpAuthBuddyRequest(connection, arg->profile);
                DWCi_Acc_CreateProfileIdToken(&sFriend.control->friendList[i], arg->profile);
                found = TRUE;
            }
        } else if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_FRIEND_TYPE_PROFILE_ID ||
                   DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_FRIEND_TYPE_FRIEND_KEY) {
            u32 code = func_ov011_0215f51c()->gameCode;

            OS_SNPrintf(gameCode, 5, "%c%c%c%c", (u8)(code >> 24), (u8)(code >> 16), (u8)(code >> 8), (u8)code);
            if (arg->profile == DWC_GetGsProfileId(func_ov011_0215f51c(), &sFriend.control->friendList[i]) &&
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

        func_ov011_0215ed88(arg->profile);
        index = func_ov011_0215ecbc(sFriend.control->friendList, sFriend.control->friendListLen, arg->profile);
        DWCi_Acc_SetBuddyFlag(&sFriend.control->friendList[index], TRUE);
        if (DWC_IsBuddyFriendData(&sFriend.control->friendList[index])) {
            DWCi_Acc_SetFlag4(&sFriend.control->friendList[index], TRUE);
            if (!wasBuddy) {
                func_ov011_0215e980(index);
                sFriend.control->friendListChanged = 1;
            }
        }
    } else {
        gpDenyBuddyRequest(connection, arg->profile);
    }
}

// The info of a profile that authorized our buddy request: the friend it is becomes a confirmed buddy
static void func_ov011_0215f198(GPConnection *connection, GPGetInfoResponseArg *arg, void *param) {
    int i;
    char loginName[0x18];
    BOOL found = FALSE;
    BOOL wasAuthentic = FALSE;

    if (arg->result == 0) {
        for (i = 0; i < sFriend.control->friendListLen; i++) {
            if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_FRIEND_TYPE_LOGIN_ID) {
                DWCi_Acc_GetLoginName(func_ov011_0215f51c(), &sFriend.control->friendList[i], loginName);
                if (strcmp(loginName, arg->lastname) == 0) {
                    DWCi_Acc_CreateProfileIdToken(&sFriend.control->friendList[i], arg->profile);
                    DWCi_Acc_SetAuthentic(&sFriend.control->friendList[i]);
                    found = TRUE;
                }
            } else if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_FRIEND_TYPE_PROFILE_ID ||
                       DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_FRIEND_TYPE_FRIEND_KEY) {
                if (arg->profile == DWC_GetGsProfileId(func_ov011_0215f51c(), &sFriend.control->friendList[i])) {
                    if (DWCi_Acc_IsAuthentic(&sFriend.control->friendList[i]) == TRUE) {
                        wasAuthentic = TRUE;
                    } else {
                        DWCi_Acc_CreateProfileIdToken(&sFriend.control->friendList[i], arg->profile);
                        DWCi_Acc_SetAuthentic(&sFriend.control->friendList[i]);
                        found = TRUE;
                    }
                }
            }
        }
        if (found) {
            int index = func_ov011_0215ecbc(sFriend.control->friendList, sFriend.control->friendListLen, arg->profile);

            if (!wasAuthentic) {
                DWCi_Acc_SetBuddyFlag(&sFriend.control->friendList[index], TRUE);
                if (DWC_IsBuddyFriendData(&sFriend.control->friendList[index])) {
                    DWCi_Acc_SetFlag4(&sFriend.control->friendList[index], TRUE);
                }
                func_ov011_0215e980(index);
            }
            sFriend.control->friendListChanged = 1;
        }
    }
}

static int func_ov011_0215f2b4(void) {
    return sFriend.control->persLogin;
}

void func_ov011_0215f2c0(void) {
    sFriend.unk_10 = TRUE;
}

void func_ov011_0215f2cc(void) {
    sFriend.unk_10 = FALSE;
}

// The reverse buddies of the profile IDs: a friend whose profile has the player as a buddy gets the buddy flag, and
// one that hasn't loses it
static void func_ov011_0215f2d8(GPConnection *connection, GPGetReverseBuddiesListResponseArg *arg, void *param) {
    int i;
    int j;

    if (func_ov011_0215e714()) {
        if (sFriend.control->profileIds != NULL) {
            DWC_Free(4, sFriend.control->profileIds, 0);
            sFriend.control->profileIds = NULL;
        }
        if (arg->result != 0) {
            func_ov011_0215ebf8();
            return;
        }
        for (i = 0; i < sFriend.control->friendListLen; i++) {
            if (DWC_GetFriendDataType(&sFriend.control->friendList[i]) == DWC_FRIEND_TYPE_PROFILE_ID) {
                for (j = 0; j < arg->numOfUniqueMatchs; j++) {
                    if (arg->matches[j].profile ==
                        DWC_GetGsProfileId(func_ov011_0215f51c(), &sFriend.control->friendList[i])) {
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
                if (j == arg->numOfUniqueMatchs && DWCi_Acc_IsBuddyFlag(&sFriend.control->friendList[i]) == TRUE) {
                    sFriend.control->friendListChanged = 1;
                    DWCi_Acc_SetBuddyFlag(&sFriend.control->friendList[i], FALSE);
                }
                if (DWC_IsBuddyFriendData(&sFriend.control->friendList[i])) {
                    DWCi_Acc_SetFlag4(&sFriend.control->friendList[i], TRUE);
                }
            }
        }
        func_ov011_0215ebf8();
    }
}

void func_ov011_0215f410(DWCLoginControl *control, DWCUserData *userData, GPConnection *connection, int productId,
                         u32 gameCode, const u16 *playerName, DWCLoginCallback callback, void *param) {
    sLogin = control;
    MI_CpuFill8(control, 0, sizeof(DWCLoginControl));
    sLogin->connection = connection;
    sLogin->state = 0;
    sLogin->productId = productId;
    sLogin->gameCode = gameCode;
    sLogin->playerName = playerName;
    sLogin->callback = callback;
    sLogin->param = param;
    sLogin->userData = userData;
    func_ov011_02169fb8();
}

BOOL func_ov011_0215f45c(void) {
    if (func_ov011_0215f748()) {
        sLogin->state = 1;
        sLogin->connecting = FALSE;
        return TRUE;
    }
    return FALSE;
}

void func_ov011_0215f480(void) {
    if (sLogin != NULL && !DWCi_IsError()) {
        switch (sLogin->state) {
        case 0:
        case 5:
            break;
        case 1:
            func_ov011_0215f820();
            return;
        case 2:
        case 3:
        case 4:
            if (sLogin->connection != NULL && *sLogin->connection != NULL) {
                gpProcess(sLogin->connection);
            }
            if (sLogin->connecting && OS_TicksToMilliSeconds(OS_GetTick() - sLogin->connectTick) > 60000) {
                func_ov011_0215f530(DWC_ERROR_NETWORK, -61070);
                sLogin->connecting = FALSE;
            }
            break;
        }
    }
}

DWCUserData *func_ov011_0215f51c(void) {
    if (sLogin != NULL) {
        return sLogin->userData;
    }
    return NULL;
}

void func_ov011_0215f530(int error, int code) {
    if (sLogin != NULL && error != DWC_ERROR_NONE) {
        DWCi_SetError(error, code);
        if (sLogin->callback != NULL) {
            sLogin->callback(error, 0, sLogin->param);
        }
        func_ov011_0215f57c();
    }
}

void func_ov011_0215f560(void) {
    if (!func_ov011_02169f34()) {
        func_ov011_02169eb0();
    }
    sLogin = NULL;
}

static void func_ov011_0215f57c(void) {
    if (sLogin != NULL) {
        func_ov011_02169f90();
        sLogin->state = 0;
        sLogin->connecting = FALSE;
    }
}

// Ends the login for an error of GP, returning it
static int func_ov011_0215f59c(int result) {
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
    func_ov011_0215f530(error, code - 61000);
    return result;
}

// GP's login: it sets the profile's info and, logged in with the user data's profile, ends the login, or else looks
// up the info of the profile that it logged in to
static void func_ov011_0215f5f0(GPConnection *connection, GPConnectResponseArg *arg, void *param) {
    char consoleId[0x21];
    char userId[0x33];

    sLogin->connecting = FALSE;
    OS_SNPrintf(consoleId, 0x1f, "%s:%016llu", func_ov011_0215dd7c(), func_ov011_0215dd74());
    if (func_ov011_0215f59c(gpSetInfos(connection, GP_INFO_FIRSTNAME, consoleId)) != 0) {
        return;
    }
    DWCi_Acc_EncodeBase32(func_ov011_02169f98(), 43, userId);
    if (func_ov011_0215f59c(gpSetInfos(connection, GP_INFO_AIMNAME, userId)) != 0) {
        return;
    }
    if (func_ov011_0215f59c(gpSetInfos(connection, GP_INFO_ZIPCODE, func_ov011_0215dd84())) != 0) {
        return;
    }
    if (arg->result == 0) {
        if (sLogin->state == 2) {
            if (sLogin->userData->gsProfileId == arg->profile) {
                sLogin->state = 5;
                if (func_ov011_0215f59c(func_ov011_02162d80(1)) != 0) {
                    return;
                }
                sLogin->callback(DWC_ERROR_NONE, arg->profile, sLogin->param);
                if (func_ov011_0216043c() != 0) {
                    return;
                }
                func_ov011_02168780(arg->profile);
                func_ov011_02168794();
            } else {
                func_ov011_0215f530(DWC_ERROR_NETWORK, -60000);
            }
        } else if (sLogin->state == 3) {
            if (func_ov011_0215f59c(gpGetInfo(connection, arg->profile, 0, 0, (GPCallback)func_ov011_0215f8a8, NULL)) !=
                0) {
                return;
            }
        }
    } else {
        func_ov011_0215f59c(arg->result);
    }
}

// Connects to GP with the authentication server's token, going to the given state
static void func_ov011_0215f700(GPCallback callback, int nextState) {
    OSTick now = OS_GetTick();

    sLogin->connectTick = now;
    sLogin->connecting = TRUE;
    if (func_ov011_0215f59c(gpConnectPreAuthenticated(sLogin->connection, sLogin->authToken, sLogin->partnerChallenge,
                                                      1, 0, callback, NULL)) == 0) {
        sLogin->state = nextState;
    }
}

// Starts the login to the authentication server with the user data's login ID, or a temporary one until it has one
static BOOL func_ov011_0215f748(void) {
    u64 userId;

    if (DWCi_Acc_HasAuthenticLoginId(sLogin->userData)) {
        DWCi_Acc_CreateLoginName(&sLogin->userData->authentic, sLogin->userData->gameCode, sLogin->loginName);
        userId = DWCi_Acc_GetUserId(&sLogin->userData->authentic);
    } else {
        if (!DWCi_Acc_IsLoginId(&sLogin->tempLoginId)) {
            if (DWCi_Acc_IsConsoleLoginId(&sLogin->userData->pseudo)) {
                sLogin->tempLoginId = sLogin->userData->pseudo;
            } else {
                DWCi_Acc_CreateLoginId(&sLogin->tempLoginId);
            }
        } else {
            DWCi_Acc_SetPlayerId(&sLogin->tempLoginId, (OS_GetTick() * 0x5d588b656c078965 + 0x269ec3) >> 32);
        }
        DWCi_Acc_CreateLoginName(&sLogin->tempLoginId, sLogin->gameCode, sLogin->loginName);
        userId = 0;
    }
    return func_ov011_02169e50(sLogin->playerName, &sLogin->loginName[9], userId, DWC_Alloc, DWC_Free);
}

// Waits for the authentication server, then connects to GP with the profile of the user data, or looks for it
static void func_ov011_0215f820(void) {
    func_ov011_02169ebc();
    if (func_ov011_02169f34()) {
        if (func_ov011_02169f40()) {
            func_ov011_02169f58(sLogin->authToken, sLogin->partnerChallenge);
            func_ov011_02169f90();
            if (DWCi_Acc_HasAuthenticLoginId(sLogin->userData)) {
                func_ov011_0215f700((GPCallback)func_ov011_0215f5f0, 2);
                return;
            }
            DWCi_Acc_SetUserId(&sLogin->tempLoginId, func_ov011_02169f98());
            func_ov011_0215f700((GPCallback)func_ov011_0215f5f0, 3);
            return;
        }
        {
            int error = func_ov011_02169f4c();

            if (error <= -29000) {
                func_ov011_0215f530(DWC_ERROR_FATAL, error);
                return;
            }
            func_ov011_0215f530(DWC_ERROR_AUTH_ANY, error);
        }
    }
}

// The info of the profile that GP logged in to without the user data's: a new profile gets the login name, and once
// the profile has it, the login ID is authenticated for the profile and GP logs in again with it
static void func_ov011_0215f8a8(GPConnection *connection, GPGetInfoResponseArg *arg, void *param) {
    char loginName[0x16];
    char userName[0x15];
    char tempName[0x15];

    if (arg->result == 0) {
        if (sLogin->state == 3) {
            if (arg->lastname[0] == 0) {
                DWCi_Acc_CreateLoginName(&sLogin->userData->pseudo, sLogin->gameCode, loginName);
                if (func_ov011_0215f59c(gpSetInfos(connection, GP_INFO_LASTNAME, loginName)) != 0) {
                    return;
                }
                sLogin->state = 4;
                if (func_ov011_0215f59c(
                        gpGetInfo(connection, arg->profile, 0, 0, (GPCallback)func_ov011_0215f8a8, NULL)) != 0) {
                    return;
                }
            } else {
                gpDisconnect(connection);
                func_ov011_0215f748();
                sLogin->state = 1;
            }
        } else if (sLogin->state == 4) {
            DWCi_Acc_CreateLoginName(&sLogin->userData->pseudo, sLogin->gameCode, userName);
            if (strcmp(arg->lastname, userName) == 0) {
                DWCi_Acc_CreateLoginName(&sLogin->tempLoginId, sLogin->gameCode, tempName);
                DWCi_Acc_SetAuthenticLoginId(sLogin->userData, &sLogin->tempLoginId, arg->profile);
                gpDisconnect(connection);
                func_ov011_0215f700((GPCallback)func_ov011_0215f5f0, 2);
                return;
            }
            if (func_ov011_0215f59c(gpGetInfo(connection, arg->profile, 0, 0, (GPCallback)func_ov011_0215f8a8, NULL)) !=
                0) {
                return;
            }
        }
    }
}

// Whether GP is logged in
static BOOL func_ov011_0215f998(void) {
    if (sLogin != NULL && sLogin->state == 5) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov011_0215f9b4(const char **authToken, const char **challenge) {
    if (!func_ov011_0215f998()) {
        return FALSE;
    }
    *authToken = sLogin->authToken;
    *challenge = sLogin->partnerChallenge;
    return TRUE;
}

BOOL func_ov011_0215f9e0(SakeLoginTicket *ticket) {
    if (!func_ov011_0215f998()) {
        return FALSE;
    }
    if (gpGetLoginTicket(sLogin->connection, (GPLoginTicket *)ticket) == 0) {
        return TRUE;
    }
    return FALSE;
}
