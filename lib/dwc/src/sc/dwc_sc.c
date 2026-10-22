#include "dwc/ext.h"
#include "dwc/sc.h"
#include <stdarg.h>

// Overlay 260's Sc, the Wi-Fi library's client of the competition service: see dwc/sc.h.

// What the Sc keeps: whether the login has ended, the competition client, the functions to call when its requests end,
// and what the login gave
typedef struct ScState {
    int loginDone;
    SCInterface *client;
    ScResultFunc createSessionFunc;
    ScResultFunc reportIntentionFunc;
    ScResultFunc submitReportFunc;
    GSLoginPrivateData keys;
    GSLoginCertificate certificate;
} ScState;

static void Sc_Printf(int line, const char *format, ...);
static void Sc_OnLogin(GHTTPResult status, WSLoginResponse *result, void *userData);
static int Sc_GetErrorType(int error);
static int Sc_GetErrorCode(int step, int error);
static int Sc_SetAuthError(int error);
static void Sc_ReleaseUrls(void);
static void Sc_OnCreateSession(SCInterface *client, GHTTPResult status, SCResult error, void *userData);
static void Sc_OnReportIntention(SCInterface *client, GHTTPResult status, SCResult error, void *userData);
static void Sc_OnSubmitReport(SCInterface *client, GHTTPResult status, SCResult error, void *userData);

static ScState sScState;

int Sc_Startup(int gameId, int server) {
    char *authToken;
    char *challenge;
    int error;

    gsCoreInitialize();
    ScClient_SetServers(server);
    DWCi_GetAuthInfo(&authToken, &challenge);
    sScState.loginDone = 0;
    error = wsLoginRemoteAuth(0, gameId, authToken, challenge, Sc_OnLogin, NULL);
    if (error != 0) {
        Sc_Printf(142, "Faild on wsLoginRemoteAuth[%d]\n", error);
        DWCi_SetError(0x14, Sc_GetErrorCode(1, 0));
        return 7;
    }
    while (sScState.loginDone == 0) {
        OS_Sleep(10);
        gsCoreThink(0);
    }
    if (DWC_GetLastError(0) != 0) {
        return 7;
    }
    error = scInitialize(gameId, &sScState.client);
    if (error != 0) {
        DWCi_SetError(Sc_GetErrorType(error), Sc_GetErrorCode(2, error));
    }
    return error;
}

int Sc_Shutdown(void) {
    int error = scShutdown(sScState.client);

    if (error != 0) {
        DWCi_SetError(Sc_GetErrorType(error), Sc_GetErrorCode(3, error));
    }
    Sc_ReleaseUrls();
    gsCoreShutdown();
    gsCoreThink(0);
    if (gsCoreIsShutdown() != GSCore_SHUTDOWN_COMPLETE) {
        gsCoreThink(0);
    }
    return error;
}

static void Sc_ReleaseUrls(void) {
    DWCi_RemoveGHTTPRootCA(Auth_GetUrl());
    DWCi_RemoveGHTTPRootCA(ScClient_GetUrl());
}

int Sc_Update(void) {
    int error = scThink(sScState.client);

    if (error != 0) {
        DWCi_SetError(8, Sc_GetErrorCode(4, error));
    }
    return error;
}

int Sc_CreateSession(ScResultFunc func, int timeout, void *userData) {
    int error;

    sScState.createSessionFunc = func;
    error =
        scCreateSession(sScState.client, &sScState.certificate, &sScState.keys, Sc_OnCreateSession, timeout, userData);
    if (error != 0) {
        DWCi_SetError(8, Sc_GetErrorCode(5, error));
    }
    return error;
}

static void Sc_OnCreateSession(SCInterface *client, GHTTPResult status, SCResult error, void *userData) {
    sScState.createSessionFunc(error, userData);
}

int Sc_SetReportIntention(int authoritative, ScResultFunc func, int timeout, void *userData) {
    int error;

    sScState.reportIntentionFunc = func;
    error = scSetReportIntention(sScState.client, NULL, authoritative, &sScState.certificate, &sScState.keys,
                                 Sc_OnReportIntention, timeout, userData);
    if (error != 0) {
        DWCi_SetError(8, Sc_GetErrorCode(7, error));
    }
    return error;
}

static void Sc_OnReportIntention(SCInterface *client, GHTTPResult status, SCResult error, void *userData) {
    sScState.reportIntentionFunc(error, userData);
}

int Sc_SubmitReport(SCIReport *report, int authoritative, ScResultFunc func, int timeout, void *userData) {
    int error;

    sScState.submitReportFunc = func;
    error = scSubmitReport(sScState.client, report, authoritative, &sScState.certificate, &sScState.keys,
                           Sc_OnSubmitReport, timeout, userData);
    if (error != 0) {
        DWCi_SetError(8, Sc_GetErrorCode(11, error));
    }
    return error;
}

static void Sc_OnSubmitReport(SCInterface *client, GHTTPResult status, SCResult error, void *userData) {
    sScState.submitReportFunc(error, userData);
}

int Sc_SetSessionId(const char *csid) {
    int error = scSetSessionId(sScState.client, csid);

    if (error != 0) {
        DWCi_SetError(8, Sc_GetErrorCode(8, error));
    }
    return error;
}

const char *Sc_GetSessionId(void) {
    return scGetSessionId(sScState.client);
}

const char *Sc_GetConnectionId(void) {
    return scGetConnectionId(sScState.client);
}

int Sc_CreateReport(int version, int playerNum, int teamNum, SCIReport **report) {
    int error = scCreateReport(sScState.client, version, playerNum, teamNum, report);

    if (error != 0) {
        DWCi_SetError(8, Sc_GetErrorCode(9, error));
    }
    return error;
}

int Sc_BeginGame(SCIReport *report) {
    int error = scReportBeginGlobalData(report);

    if (error != 0) {
        DWCi_SetError(8, Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_BeginPlayers(SCIReport *report) {
    int error = scReportBeginPlayerData(report);

    if (error != 0) {
        DWCi_SetError(8, Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_BeginTeams(SCIReport *report) {
    int error = scReportBeginTeamData(report);

    if (error != 0) {
        DWCi_SetError(8, Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_BeginGroup(SCIReport *report) {
    int error = scReportBeginNewPlayer(report);

    if (error != 0) {
        DWCi_SetError(8, Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_SetPlayer(SCIReport *report, int index, const char *id, int key, int result,
                 const GSLoginCertificate *certificate, void *unused) {
    int error = scReportSetPlayerData(report, index, id, key, result, certificate->mProfileId, certificate, unused);

    if (error != 0) {
        DWCi_SetError(8, Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_FinishReport(SCIReport *report, int flag, int value) {
    int error = scReportEnd(report, flag, value);

    if (error != 0) {
        DWCi_SetError(Sc_GetErrorType(error), Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_AddInt(SCIReport *report, u16 key, s32 value) {
    int error = scReportAddIntValue(report, key, value);

    if (error != 0) {
        DWCi_SetError(Sc_GetErrorType(error), Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_AddByte(SCIReport *report, u16 key, s8 value) {
    int error = scReportAddByteValue(report, key, value);

    if (error != 0) {
        DWCi_SetError(Sc_GetErrorType(error), Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_FreeReport(SCIReport *report) {
    int error = scDestroyReport(report);

    if (error != 0) {
        DWCi_SetError(Sc_GetErrorType(error), Sc_GetErrorCode(13, error));
    }
    return error;
}

int Sc_GetCertificate(const GSLoginCertificate **certificate) {
    *certificate = &sScState.certificate;
    return 0;
}

static void Sc_Printf(int line, const char *format, ...) {
    char buffer[0x100];
    int length;
    va_list args;

    length = OS_SNPrintf(buffer, sizeof(buffer), "Sc[%d]> ", line);
    va_start(args, format);
    OS_VSNPrintf(buffer + length, sizeof(buffer) - length, format, args);
    va_end(args);
}

static void Sc_OnLogin(GHTTPResult status, WSLoginResponse *result, void *userData) {
    if (status != 0) {
        Sc_Printf(789, "Failed on player login, GHTTP error: %d\n", status);
        if (status == 1) {
            DWCi_SetError(9, Sc_GetErrorCode(1, 5));
        } else {
            DWCi_SetError(0x14, Sc_GetErrorCode(1, 7));
        }
        DWCi_HandleGHTTPError(status);
    } else if (result->mLoginResult != 0) {
        Sc_Printf(803, "Failed on player login, Login result: %d\n", result->mLoginResult);
        Sc_SetAuthError(result->mLoginResult);
    } else {
        memcpy(&sScState.certificate, &result->mCertificate, sizeof(GSLoginCertificate));
        memcpy(&sScState.keys, &result->mPrivateData, sizeof(GSLoginPrivateData));
        Sc_Printf(811, "Player '%s' logged in.\r\n", result->mCertificate.mUniqueNick);
    }
    sScState.loginDone = 1;
}

static int Sc_GetErrorType(int error) {
    switch (error) {
    case 0:
        return 0;
    case 5:
        return 9;
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    default:
        return 0x14;
    }
}

static int Sc_GetErrorCode(int step, int error) {
    return -43000 - (error + step * 100);
}

static int Sc_SetAuthError(int error) {
    int code = -43000;
    int type = 0x14;

    if (error == 0) {
        return 0;
    }
    switch (error) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x69:
    case 0x6a:
    case 0x6b:
    case 0x6c:
        code -= 8;
        break;
    case 0x68:
        type = 9;
        code -= 6;
        break;
    default:
        code -= 15;
        break;
    }
    DWCi_SetError(type, code);
    return error;
}
