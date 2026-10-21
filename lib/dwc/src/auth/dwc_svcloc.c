#include "dwc/common.h"
#include "dwc/error.h"
#include "dwc/ext.h"
#include "dwc/friend.h"
#include "dwc/nasfunc.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include <errno.h>

// DWC's service locator (overlay 11, Thumb): it asks the server's "SVCLOC" request for a service's host and token. The
// ROM embeds no name; the file is named after that request's action

// The service locator's state (0x1c5c bytes)
typedef struct NasLocator {
    DWCHttpAlloc alloc;
    DWCHttpFree free;
    DWCHttp http;
    DWCSvlResult *result;
} NasLocator;

static BOOL DWCi_Svl_ParseResult(void);

static NasLocator *sLocator;

BOOL DWC_Svl_Init(DWCHttpAlloc alloc, DWCHttpFree free) {
    if (sLocator != NULL) {
        DWCi_SetError(9, -24100);
        return FALSE;
    }
    sLocator = alloc(0, sizeof(NasLocator));
    if (sLocator == NULL) {
        DWCi_SetError(9, -24100);
        return FALSE;
    }
    MI_CpuFill8(sLocator, 0, sizeof(NasLocator));
    sLocator->alloc = alloc;
    sLocator->free = free;
    return TRUE;
}

void DWC_Svl_Cleanup(void) {
    if (sLocator != NULL) {
        DWC_Http_Destroy(&sLocator->http);
        sLocator->free(0, sLocator, 0);
        sLocator = NULL;
    }
}

BOOL DWC_Svl_GetTokenAsync(const char *service, DWCSvlResult *result) {
    DWCHttpParam config;
    NasAuthInfo info;
    NasLocator *locator;

    sLocator->result = result;
    MI_CpuFill8(result, 0, sizeof(DWCSvlResult));
    if (sLocator->http.thread.id != 0 && !OS_IsThreadTerminated(&sLocator->http.thread)) {
        OS_Terminate();
    }
    locator = sLocator;
    config.url = DWCi_AuthConfig.url;
    config.get = 0;
    config.bufferSize = 0x1000;
    config.alloc = locator->alloc;
    config.free = locator->free;
    if (strcmp(DWCi_AuthConfig.url, "https://nas.nintendowifi.net/ac") != 0) {
        config.ignoreCa = 1;
    } else {
        config.ignoreCa = 0;
    }
    config.timeout = 20000;
    if (DWC_Http_Create(&locator->http, &config) != 0) {
        DWCi_SetError(9, -24100);
        return FALSE;
    }
    if (!DWCi_Auth_GetCurrentInfo(&info) || !DWCi_Auth_AddCommonItems(&sLocator->http, &info, 0)) {
        DWC_Http_Destroy(&sLocator->http);
        DWCi_SetError(9, -24100);
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(&sLocator->http, "action", "SVCLOC", 6) != 0 ||
        DWC_Http_Add_PostBase64Item(&sLocator->http, "svc", service, 4) != 0) {
        DWC_Http_Destroy(&sLocator->http);
        DWCi_SetError(9, -24100);
        return FALSE;
    }
    if (DWC_Http_FinishHeader(&sLocator->http) != 0) {
        DWC_Http_Destroy(&sLocator->http);
        DWCi_SetError(9, -24100);
        return FALSE;
    }
    DWC_Http_StartThread(&sLocator->http, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
    if (sLocator->http.thread.id == 0) {
        DWC_Http_Destroy(&sLocator->http);
        DWCi_SetError(9, -24100);
        return FALSE;
    }
    return TRUE;
}

int DWC_Svl_Process(void) {
    if (sLocator == NULL) {
        return 0;
    }
    if (sLocator->http.thread.id == 0) {
        return 1;
    }
    if (OS_IsThreadTerminated(&sLocator->http.thread) == 1) {
        switch (sLocator->http.result) {
        case 8:
            if (DWCi_Svl_ParseResult() == 0) {
                DWC_Http_Destroy(&sLocator->http);
                return 4;
            }
            DWC_Http_Destroy(&sLocator->http);
            return 3;
        case 7:
            DWC_Http_Destroy(&sLocator->http);
            return 5;
        default:
            DWC_Http_Destroy(&sLocator->http);
            DWCi_SetError(0x10, -24101);
            return 4;
        }
    }
    return 2;
}

static BOOL DWCi_Svl_ParseResult(void) {
    DWCHttp *http = &sLocator->http;
    char text[4];
    const char *date;
    const char *httpResult;
    int status;
    int returnCode;

    if (DWC_Http_ParseResult(http, 0) == 0) {
        DWCi_SetError(0xf, -25101);
        return FALSE;
    }
    date = DWC_Http_GetResult(http, "Date");
    if (date != NULL) {
        DWCi_SetNasTimeFromDateHeader(date);
    }
    httpResult = DWC_Http_GetResult(http, "httpresult");
    if (httpResult == NULL) {
        DWCi_SetError(0xf, -25101);
        return FALSE;
    }
    status = atol(httpResult);
    if (errno == ERANGE) {
        DWCi_SetError(0xf, -25101);
        return FALSE;
    }
    if (status != 200) {
        DWCi_SetError(0xf, -25000 - status);
        return FALSE;
    }
    if (DWC_Http_GetBase64DecodedResult(http, "returncd", text, 4) <= 0) {
        DWCi_SetError(0xf, -25101);
        return FALSE;
    }
    returnCode = atol(text);
    if (errno == ERANGE) {
        DWCi_SetError(0xf, -25101);
        return FALSE;
    }
    if (returnCode >= 100) {
        DWCi_SetError(0xf, -24000 - returnCode);
        return FALSE;
    }
    if (DWC_Http_GetBase64DecodedResult(http, "svchost", sLocator->result->svlhost, 0x40) < 0 ||
        DWC_Http_GetBase64DecodedResult(http, "servicetoken", sLocator->result->svltoken, 0x12c) < 0 ||
        DWC_Http_GetBase64DecodedResult(http, "statusdata", text, 1) < 0) {
        DWCi_SetError(0xf, -25101);
        return FALSE;
    }
    sLocator->result->status = text[0] == 'Y' ? 1 : 0;
    return TRUE;
}
