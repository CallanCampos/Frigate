// DWC's calls of the Nintendo authentication server (TWL-DWC's dwc_nasfunc.c, overlay 11, Thumb): the server's date
// and time, which dwc_nastime.c keeps, the service locator's token, and the login to the server that the game runs
// by itself. Its .data holds the service locator's default service and the login's empty code, and its .bss the
// login's work.
// The ROM names none of DWC's files. Black 1's DWC, whose functions aletheia matched to TWL-DWC's library objects
// (layout/symbols_sdk.txt), has overlay 11's run of them in libdwcnonport's dwc_nonport.o, libdwcnas's dwc_nasfunc.o
// and dwc_nastime.o, and libdwcmatch's dwc_common.o, dwc_friend.o and dwc_login.o, in this order and with the same
// functions, and each file's .data and .bss follow in the same order.

#include "dwc/bm.h"
#include "dwc/dwc.h"
#include "dwc/ext.h"
#include "dwc/memfunc.h"
#include "dwc/nasfunc.h"
#include "dwc/nastime.h"
#include "nitro/mi.h"

// The login to the authentication server that the game runs by itself: the HTTP client, what it is started with and
// what the server answered
typedef struct NasLogin {
    DWCHttp http;
    DWCAuthParam param;
    DWCAuthResult result;
} NasLogin;

// The seconds of a century, which the clock's seconds wrap at
#define SECONDS_PER_CENTURY 0xbc191380

static NasLogin *sNasLogin;

BOOL DWC_GetDateTime(RTCDate *date, RTCTime *time) {
    s64 seconds;

    if (!DWCi_NasTimeValid) {
        return FALSE;
    }
    if (RTC_GetDateTime(date, time)) {
        return FALSE;
    }
    seconds = RTC_ConvertDateTimeToSecond(date, time);
    if (seconds == -1) {
        return FALSE;
    }
    if (seconds < DWCi_NasTimeLocalSeconds) {
        seconds += SECONDS_PER_CENTURY;
    }
    seconds -= DWCi_NasTimeOffset;
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
