// DWC's login to GameSpy's GP (TWL-DWC's dwc_login.c, overlay 11, Thumb), with the token that the authentication
// server gives for the console's login ID. Its .data holds the format of the console's ID and its .bss the login's
// control. The layouts are in dwc/login.h.
// The ROM names none of DWC's files. Black 1's DWC, whose functions aletheia matched to TWL-DWC's library objects
// (layout/symbols_sdk.txt), has overlay 11's run of them in libdwcnonport's dwc_nonport.o, libdwcnas's dwc_nasfunc.o
// and dwc_nastime.o, and libdwcmatch's dwc_common.o, dwc_friend.o and dwc_login.o, in this order and with the same
// functions, and each file's .data and .bss follow in the same order.

#include "dwc/dwc.h"
#include "dwc/dwc_nonport.h"
#include "dwc/ext.h"
#include "dwc/gpi.h"
#include "dwc/login.h"
#include "dwc/memfunc.h"
#include "dwc/nasfunc.h"
#include "nitro/mi.h"

// The info keys of GP that the login sets: the first and last names and the AIM name hold the console's ID, the login
// name and the user ID, and the zip code the game's code
#define GP_INFO_FIRSTNAME 0x704
#define GP_INFO_LASTNAME 0x705
#define GP_INFO_ZIPCODE 0x708
#define GP_INFO_AIMNAME 0x717

static void DWCi_CloseLogin(void);
static int DWCi_HandleLoginGPError(int result);
static void DWCi_GPConnectCallback(GPConnection *connection, GPConnectResponseArg *arg, void *param);
static void DWCi_GPConnect(GPCallback callback, int nextState);
static BOOL DWCi_RemoteLogin(void);
static void DWCi_RemoteLoginProcess(void);
static void DWCi_GPGetInfoCallback(GPConnection *connection, GPGetInfoResponseArg *arg, void *param);

static DWCLoginControl *sLogin;

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
BOOL DWCi_CheckLogin(void) {
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
