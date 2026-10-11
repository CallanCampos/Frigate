#include "dwc/sc.h"

#include <stdarg.h>
#include "dwc/ext.h"

// Overlay 260's Sc, the Wi-Fi library's client of the competition service: see dwc/sc.h.

// What the Sc keeps: whether the login has ended, the competition client, the functions to call when its requests end,
// and what the login gave
typedef struct ScState {
    int loginDone;
    ScClient *client;
    ScResultFunc createSessionFunc;
    ScResultFunc reportIntentionFunc;
    ScResultFunc submitReportFunc;
    AuthKeys keys;
    AuthCertificate certificate;
} ScState;

static void Sc_Printf(int line, const char *format, ...);
static void Sc_OnLogin(int status, AuthResult *result, void *userData);
static int Sc_GetErrorType(int error);
static int Sc_GetErrorCode(int step, int error);
static int Sc_SetAuthError(int error);
static void Sc_ReleaseUrls(void);
static void Sc_OnCreateSession(ScClient *client, int status, int error, void *userData);
static void Sc_OnReportIntention(ScClient *client, int status, int error, void *userData);
static void Sc_OnSubmitReport(ScClient *client, int status, int error, void *userData);

static ScState sScState;

int Sc_Startup(int gameId, int server) {
    const char *authToken;
    const char *challenge;
    int error;

    func_ov011_0216c77c();
    ScClient_SetServers(server);
    func_ov011_0215f9b4(&authToken, &challenge);
    sScState.loginDone = 0;
    error = wsLoginRemoteAuth(0, gameId, authToken, challenge, Sc_OnLogin, NULL);
    if (error != 0) {
        Sc_Printf(142, "Faild on wsLoginRemoteAuth[%d]\n", error);
        func_020584cc(0x14, Sc_GetErrorCode(1, 0));
        return 7;
    }
    while (sScState.loginDone == 0) {
        func_0207aa04(10);
        func_ov011_0216c910(0);
    }
    if (func_02058390(0) != 0) {
        return 7;
    }
    error = ScClient_Create(gameId, &sScState.client);
    if (error != 0) {
        func_020584cc(Sc_GetErrorType(error), Sc_GetErrorCode(2, error));
    }
    return error;
}

int Sc_Shutdown(void) {
    int error = ScClient_Destroy(sScState.client);

    if (error != 0) {
        func_020584cc(Sc_GetErrorType(error), Sc_GetErrorCode(3, error));
    }
    Sc_ReleaseUrls();
    func_ov011_0216c998();
    func_ov011_0216c910(0);
    if (func_ov011_0216c9f4() != 2) {
        func_ov011_0216c910(0);
    }
    return error;
}

static void Sc_ReleaseUrls(void) {
    DWCi_RemoveGHTTPRootCA(Auth_GetUrl());
    DWCi_RemoveGHTTPRootCA(ScClient_GetUrl());
}

int Sc_Update(void) {
    int error = ScClient_Update(sScState.client);

    if (error != 0) {
        func_020584cc(8, Sc_GetErrorCode(4, error));
    }
    return error;
}

int Sc_CreateSession(ScResultFunc func, int timeout, void *userData) {
    int error;

    sScState.createSessionFunc = func;
    error = ScClient_CreateSession(sScState.client, &sScState.certificate, &sScState.keys, Sc_OnCreateSession, timeout,
                                   userData);
    if (error != 0) {
        func_020584cc(8, Sc_GetErrorCode(5, error));
    }
    return error;
}

static void Sc_OnCreateSession(ScClient *client, int status, int error, void *userData) {
    sScState.createSessionFunc(error, userData);
}

int Sc_SetReportIntention(int authoritative, ScResultFunc func, int timeout, void *userData) {
    int error;

    sScState.reportIntentionFunc = func;
    error = ScClient_SetReportIntention(sScState.client, NULL, authoritative, &sScState.certificate, &sScState.keys,
                                        Sc_OnReportIntention, timeout, userData);
    if (error != 0) {
        func_020584cc(8, Sc_GetErrorCode(7, error));
    }
    return error;
}

static void Sc_OnReportIntention(ScClient *client, int status, int error, void *userData) {
    sScState.reportIntentionFunc(error, userData);
}

int Sc_SubmitReport(ScReport *report, int authoritative, ScResultFunc func, int timeout, void *userData) {
    int error;

    sScState.submitReportFunc = func;
    error = ScClient_SubmitReport(sScState.client, report, authoritative, &sScState.certificate, &sScState.keys,
                                  Sc_OnSubmitReport, timeout, userData);
    if (error != 0) {
        func_020584cc(8, Sc_GetErrorCode(11, error));
    }
    return error;
}

static void Sc_OnSubmitReport(ScClient *client, int status, int error, void *userData) {
    sScState.submitReportFunc(error, userData);
}

int Sc_SetSessionId(const char *csid) {
    int error = ScClient_SetCsid(sScState.client, csid);

    if (error != 0) {
        func_020584cc(8, Sc_GetErrorCode(8, error));
    }
    return error;
}

char *Sc_GetSessionId(void) {
    return ScClient_GetCsid(sScState.client);
}

char *Sc_GetConnectionId(void) {
    return ScClient_GetCcid(sScState.client);
}

int Sc_CreateReport(int version, int playerNum, int teamNum, ScReport **report) {
    int error = ScClient_CreateReport(sScState.client, version, playerNum, teamNum, report);

    if (error != 0) {
        func_020584cc(8, Sc_GetErrorCode(9, error));
    }
    return error;
}

int Sc_BeginGame(ScReport *report) {
    int error = ScReport_BeginGame(report);

    if (error != 0) {
        func_020584cc(8, Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_BeginPlayers(ScReport *report) {
    int error = ScReport_BeginPlayers(report);

    if (error != 0) {
        func_020584cc(8, Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_BeginTeams(ScReport *report) {
    int error = ScReport_BeginTeams(report);

    if (error != 0) {
        func_020584cc(8, Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_BeginGroup(ScReport *report) {
    int error = ScReport_BeginGroup(report);

    if (error != 0) {
        func_020584cc(8, Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_SetPlayer(ScReport *report, int index, const char *id, int key, int result, const AuthCertificate *certificate,
                 void *unused) {
    int error = ScReport_SetPlayer(report, index, id, key, result, certificate->profileId, certificate, unused);

    if (error != 0) {
        func_020584cc(8, Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_FinishReport(ScReport *report, int flag, int value) {
    int error = ScReport_Finish(report, flag, value);

    if (error != 0) {
        func_020584cc(Sc_GetErrorType(error), Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_AddInt(ScReport *report, int key, int value) {
    int error = ScReport_AddInt(report, key, value);

    if (error != 0) {
        func_020584cc(Sc_GetErrorType(error), Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_AddByte(ScReport *report, int key, int value) {
    int error = ScReport_AddByte(report, key, value);

    if (error != 0) {
        func_020584cc(Sc_GetErrorType(error), Sc_GetErrorCode(10, error));
    }
    return error;
}

int Sc_FreeReport(ScReport *report) {
    int error = ScReport_Free(report);

    if (error != 0) {
        func_020584cc(Sc_GetErrorType(error), Sc_GetErrorCode(13, error));
    }
    return error;
}

int Sc_GetCertificate(const AuthCertificate **certificate) {
    *certificate = &sScState.certificate;
    return 0;
}

static void Sc_Printf(int line, const char *format, ...) {
    char buffer[0x100];
    int length;
    va_list args;

    length = func_0207a2c0(buffer, sizeof(buffer), "Sc[%d]> ", line);
    va_start(args, format);
    func_0207a2dc(buffer + length, sizeof(buffer) - length, format, args);
    va_end(args);
}

static void Sc_OnLogin(int status, AuthResult *result, void *userData) {
    if (status != 0) {
        Sc_Printf(789, "Failed on player login, GHTTP error: %d\n", status);
        if (status == 1) {
            func_020584cc(9, Sc_GetErrorCode(1, 5));
        } else {
            func_020584cc(0x14, Sc_GetErrorCode(1, 7));
        }
        DWCi_HandleGHTTPError(status);
    } else if (result->error != 0) {
        Sc_Printf(803, "Failed on player login, Login result: %d\n", result->error);
        Sc_SetAuthError(result->error);
    } else {
        memcpy(&sScState.certificate, &result->certificate, sizeof(AuthCertificate));
        memcpy(&sScState.keys, &result->keys, sizeof(AuthKeys));
        Sc_Printf(811, "Player '%s' logged in.\r\n", result->certificate.uniqueNick);
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
    func_020584cc(type, code);
    return error;
}
