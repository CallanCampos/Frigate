#include "dwc/ac.h"
#include "dwc/bm.h"
#include "dwc/ext.h"
#include "dwc/nasfunc.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include <errno.h>

// DWC's connection test (overlay 11, Thumb): it reaches the test server, logs in to the Nintendo server when the
// console has no Wi-Fi ID yet, and answers a hotspot's login page through the server ("HotSpotResponse" and "parse").
// The ROM embeds no name; the file is named after the test server it reaches ("http://conntest.nintendowifi.net/"),
// and its globals' names are ours. The thread's error exits are written as ctrl-cheeb-del/pokeheartgold writes
// NitroDWC's connection test

// The work of the connection test (0x11f4 bytes), whose first 0x1000 bytes are the stack of its thread
typedef struct NasPrWork {
    u8 stack[0x1000];
    int state;
    int result;
    u8 unk_1008[0x100];
    DWCNetcheckParam memFuncs;
    char *redirectUrl;
    char *hotspotUrl;
    OSThread thread;
    OSMutex lock;
} NasPrWork;

static void DWCi_Netcheck_StartThread(void);
static void DWCi_Netcheck_Thread(void *arg);
static void DWCi_Netcheck_SetError(int state);
static void DWCi_Netcheck_KeepUnusedData(void);

// The test server's URL, after a flag that is 1 and that only code the linker stripped reads. MWCC sorts the file's
// data by size, and as two definitions the URL would come first, so they are one object, which gives the ROM's layout
static struct {
    BOOL unk_00;
    char *url;
} sConnTest = { TRUE, "http://conntest.nintendowifi.net/" };
// The address that the test reached
CPSInAddr DWCi_ConnTestIp;
// The test's HTTP client
DWCHttp *DWCi_ConnTestHttp;
BOOL DWCi_SendSsid;
// The test's work
NasPrWork *DWCi_ConnTestWork;
// 4 bytes, and 0x18 bytes below, that only code the linker stripped reads
int DWCi_ConnTestUnk08;
// Whether a redirect (302) counts as reaching the network, which nothing sets (the SDK's DWC_Netcheck_SetAllow302)
BOOL DWCi_ConnTestAllow302;
u8 DWCi_ConnTestUnk18[0x18];
// The request and the login that the test makes
DWCHttpParam DWCi_ConnTestConfig;
DWCAuthParam DWCi_ConnTestInit;

DWCNetcheckError DWC_Netcheck_Create(DWCNetcheckParam *init) {
    DWCHttpAlloc alloc = init->alloc;

    if (DWCi_ConnTestWork != NULL) {
        return 4;
    }
    DWCi_ConnTestWork = alloc(0, sizeof(NasPrWork));
    if (DWCi_ConnTestWork == NULL) {
        return 4;
    }
    MI_CpuFill8(DWCi_ConnTestWork, 0, sizeof(NasPrWork));
    DWCi_ConnTestWork->result = -99999;
    DWCi_ConnTestWork->memFuncs = *init;
    if (DWCi_ConnTestHttp != NULL) {
        return 4;
    }
    DWCi_ConnTestHttp = alloc(0, sizeof(DWCHttp));
    if (DWCi_ConnTestHttp == NULL) {
        return 4;
    }
    DWCi_SendSsid = 0;
    OS_InitMutex(&DWCi_ConnTestWork->lock);
    DWCi_Netcheck_StartThread();
    return 0;
}

void DWC_Netcheck_Destroy(void) {
    DWCHttpFree free = DWCi_ConnTestWork->memFuncs.free;

    if (DWCi_ConnTestHttp != NULL) {
        DWC_Http_Destroy(DWCi_ConnTestHttp);
        free(0, DWCi_ConnTestHttp, 0);
        DWCi_ConnTestHttp = NULL;
    }
    DWC_Auth_Destroy();
    if (DWCi_ConnTestWork != NULL) {
        if (DWCi_ConnTestWork->redirectUrl != NULL) {
            free(0, DWCi_ConnTestWork->redirectUrl, 0);
            DWCi_ConnTestWork->redirectUrl = NULL;
        }
        if (DWCi_ConnTestWork->hotspotUrl != NULL) {
            free(0, DWCi_ConnTestWork->hotspotUrl, 0);
            DWCi_ConnTestWork->hotspotUrl = NULL;
        }
        free(0, DWCi_ConnTestWork, 0);
        DWCi_ConnTestWork = NULL;
    }
}

void DWC_Netcheck_Abort(void) {
    if (DWCi_ConnTestWork != NULL) {
        if (DWCi_ConnTestHttp != NULL) {
            DWC_Http_Abort(DWCi_ConnTestHttp);
        }
        DWC_Auth_Abort();
        if (DWCi_ConnTestWork->thread.id != 0 && !OS_IsThreadTerminated(&DWCi_ConnTestWork->thread)) {
            OS_JoinThread(&DWCi_ConnTestWork->thread);
        }
        DWCi_ConnTestWork->result = -7;
    }
}

DWCNetcheckError DWC_Netcheck_GetError(void) {
    int state;

    OS_LockMutex(&DWCi_ConnTestWork->lock);
    state = DWCi_ConnTestWork->state;
    OS_UnlockMutex(&DWCi_ConnTestWork->lock);
    return state;
}

int DWC_Netcheck_GetReturnCode(void) {
    return DWCi_ConnTestWork->result;
}

static void DWCi_Netcheck_StartThread(void) {
    if (DWCi_ConnTestWork->thread.id == 0 || OS_IsThreadTerminated(&DWCi_ConnTestWork->thread)) {
        OS_CreateThread(&DWCi_ConnTestWork->thread, DWCi_Netcheck_Thread, DWCi_ConnTestWork,
                        DWCi_ConnTestWork->stack + 0x1000, 0x1000, 16);
        OS_WakeupThreadDirect(&DWCi_ConnTestWork->thread);
    }
}

static void DWCi_Netcheck_Thread(void *arg) {
    DWCHttpAlloc alloc = DWCi_ConnTestWork->memFuncs.alloc;
    DWCHttpFree free = DWCi_ConnTestWork->memFuncs.free;
    char *url = NULL;
    char *data = NULL;
    char *waitText = NULL;
    int waitMs;
    int status;
    int length;
    int dataLength;
    int waitLength;
    int returnCode;
    char returnText[4];
    char *location;
    DWCBmWifiIdInfo userInfo;
    NasAuthInfo info;
    DWCAuthResult nasStatus;

    for (;;) {
        DWCi_ConnTestConfig.url = sConnTest.url;
        DWCi_ConnTestConfig.action = DWC_HTTP_GET;
        DWCi_ConnTestConfig.bufferSize = 0x1000;
        DWCi_ConnTestConfig.alloc = alloc;
        DWCi_ConnTestConfig.free = free;
        DWCi_ConnTestConfig.timeout = 20000;
        DWCi_ConnTestWork->result = -2;
        if (DWC_Http_Create(DWCi_ConnTestHttp, &DWCi_ConnTestConfig) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        if (DWC_Http_FinishHeader(DWCi_ConnTestHttp) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        DWC_Http_StartThread(DWCi_ConnTestHttp, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
        if (DWCi_ConnTestHttp->thread.id != 0 && !OS_IsThreadTerminated(&DWCi_ConnTestHttp->thread)) {
            OS_JoinThread(&DWCi_ConnTestHttp->thread);
        }
        if (DWCi_ConnTestHttp->result == 2) {
            DWCi_ConnTestWork->result = -1;
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (DWCi_ConnTestHttp->result != 8) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (DWC_Http_ParseResult(DWCi_ConnTestHttp, 0) != 1) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        status = atol(DWC_Http_GetResult(DWCi_ConnTestHttp, "httpresult"));
        if (errno == ERANGE) {
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        if (status == 200) {
            DWCi_ConnTestIp = DWCi_ConnTestHttp->ip;
        } else if (status == 302) {
            DWCi_SendSsid = 1;
            if (DWCi_ConnTestWork->hotspotUrl != NULL) {
                DWCi_ConnTestWork->result = -6;
                DWC_Http_Destroy(DWCi_ConnTestHttp);
                DWCi_ConnTestConfig.url = DWCi_AuthConfig.url;
                DWCi_ConnTestConfig.action = DWC_HTTP_POST;
                DWCi_ConnTestConfig.bufferSize = 0x200;
                DWCi_ConnTestConfig.alloc = alloc;
                DWCi_ConnTestConfig.free = free;
                DWCi_ConnTestConfig.timeout = 20000;
                if (strcmp(DWCi_AuthConfig.url, "https://nas.nintendowifi.net/ac") != 0) {
                    DWCi_ConnTestConfig.ignoreCa = 1;
                }
                if (DWC_Http_Create(DWCi_ConnTestHttp, &DWCi_ConnTestConfig) != 0) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(1);
                    goto done;
                }
                if (!DWCi_Auth_GetCurrentInfo(&info) || !DWCi_Auth_AddCommonItems(DWCi_ConnTestHttp, &info, 1)) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(8);
                    goto done;
                }
                if (DWC_Http_Add_PostBase64Item(DWCi_ConnTestHttp, "action", "message", 7) != 0) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(8);
                    goto done;
                }
                if (DWC_Http_Add_PostBase64Item(DWCi_ConnTestHttp, "HotSpotResponse", DWCi_ConnTestWork->hotspotUrl,
                                                strlen(DWCi_ConnTestWork->hotspotUrl)) != 0) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(8);
                    goto done;
                }
                free(0, DWCi_ConnTestWork->hotspotUrl, 0);
                DWCi_ConnTestWork->hotspotUrl = NULL;
                if (DWC_Http_FinishHeader(DWCi_ConnTestHttp) != 0) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(1);
                    goto done;
                }
                DWC_Http_StartThread(DWCi_ConnTestHttp, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
                if (DWCi_ConnTestHttp->thread.id != 0 && !OS_IsThreadTerminated(&DWCi_ConnTestHttp->thread)) {
                    OS_JoinThread(&DWCi_ConnTestHttp->thread);
                }
                if (DWCi_ConnTestHttp->result == 2) {
                    DWCi_ConnTestWork->result = -1;
                } else if (DWCi_ConnTestHttp->result == 8) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(7);
                    goto done;
                }
                DWC_Http_Destroy(DWCi_ConnTestHttp);
                DWCi_Netcheck_SetError(3);
                goto done;
            } else {
                location = DWCi_ConnTestHttp->response.start;
                if (location == NULL) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(2);
                    goto done;
                }
                DWCi_ConnTestWork->redirectUrl = alloc(0, strlen(location) + 1);
                if (DWCi_ConnTestWork->redirectUrl == NULL) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(4);
                    goto done;
                }
                strncpy(DWCi_ConnTestWork->redirectUrl, location, strlen(location) + 1);
            }
        } else {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(10);
            goto done;
        }
        DWC_Http_Destroy(DWCi_ConnTestHttp);
        DWCBm_GetWifiIdInfo(&userInfo);
        if (userInfo.id == 0) {
            DWCi_ConnTestWork->result = -3;
            DWCi_ConnTestInit.ingamesn[0] = 0;
            DWCi_ConnTestInit.gsbrcd[0] = 0;
            DWCi_ConnTestInit.alloc = DWCi_ConnTestWork->memFuncs.alloc;
            DWCi_ConnTestInit.free = DWCi_ConnTestWork->memFuncs.free;
            if (DWC_Auth_Create(&DWCi_ConnTestInit, DWCi_ConnTestHttp) != 0) {
                DWCi_Netcheck_SetError(5);
                goto done;
            }
            DWC_Auth_Join();
            if (DWC_Auth_GetError() != 0x15) {
                if (DWC_Auth_GetError() == 9) {
                    nasStatus.returnCode = -1;
                } else {
                    DWC_Auth_GetResult(&nasStatus);
                    if (DWCi_ConnTestAllow302 == 1 && (nasStatus.returnCode == -23302 || DWC_Auth_GetError() == 0xb)) {
                        DWCi_ConnTestWork->result = 0;
                        DWC_Auth_Destroy();
                        DWCi_Netcheck_SetError(0xb);
                        goto done;
                    }
                }
                DWCi_ConnTestWork->result = nasStatus.returnCode;
                DWC_Auth_Destroy();
                DWCi_Netcheck_SetError(6);
                goto done;
            }
            DWC_Auth_Destroy();
        }
        if (status == 200) {
            DWCi_ConnTestWork->result = 0;
            DWCi_Netcheck_SetError(0xb);
            goto done;
        }
        DWCi_ConnTestWork->result = -4;
        DWCi_ConnTestConfig.url = DWCi_AuthConfig.url;
        DWCi_ConnTestConfig.action = DWC_HTTP_POST;
        DWCi_ConnTestConfig.bufferSize = 0x1000;
        DWCi_ConnTestConfig.alloc = alloc;
        DWCi_ConnTestConfig.free = free;
        DWCi_ConnTestConfig.timeout = 40000;
        if (strcmp(DWCi_AuthConfig.url, "https://nas.nintendowifi.net/ac") != 0) {
            DWCi_ConnTestConfig.ignoreCa = 1;
        }
        if (DWC_Http_Create(DWCi_ConnTestHttp, &DWCi_ConnTestConfig) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        if (!DWCi_Auth_GetCurrentInfo(&info) || !DWCi_Auth_AddCommonItems(DWCi_ConnTestHttp, &info, 1) ||
            DWC_Http_Add_PostBase64Item(DWCi_ConnTestHttp, "action", "parse", 5) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(8);
            goto done;
        }
        if (DWC_Http_Add_PostBase64Item(DWCi_ConnTestHttp, "HTML", DWCi_ConnTestWork->redirectUrl,
                                        strlen(DWCi_ConnTestWork->redirectUrl)) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(8);
            goto done;
        }
        free(0, DWCi_ConnTestWork->redirectUrl, 0);
        DWCi_ConnTestWork->redirectUrl = NULL;
        if (DWC_Http_FinishHeader(DWCi_ConnTestHttp) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        DWC_Http_StartThread(DWCi_ConnTestHttp, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
        if (DWCi_ConnTestHttp->thread.id != 0 && !OS_IsThreadTerminated(&DWCi_ConnTestHttp->thread)) {
            OS_JoinThread(&DWCi_ConnTestHttp->thread);
        }
        status = DWCi_ConnTestHttp->result;
        if (status == 2) {
            DWCi_ConnTestWork->result = -1;
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (status == 3) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            if (DWCi_ConnTestAllow302 == 1) {
                DWCi_ConnTestWork->result = 0;
                DWCi_Netcheck_SetError(0xb);
            } else {
                DWCi_Netcheck_SetError(3);
            }
            goto done;
        }
        if (status != 8) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (DWC_Http_ParseResult(DWCi_ConnTestHttp, 0) != 1) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        status = atol(DWC_Http_GetResult(DWCi_ConnTestHttp, "httpresult"));
        if (errno == ERANGE) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        if (status != 200) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            if (DWCi_ConnTestAllow302 == 1 && status == 302) {
                DWCi_ConnTestWork->result = 0;
                DWCi_Netcheck_SetError(0xb);
            } else {
                DWCi_Netcheck_SetError(2);
            }
            goto done;
        }
        if (DWC_Http_GetBase64DecodedResult(DWCi_ConnTestHttp, "returncd", returnText, 4) <= 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        returnCode = atol(returnText);
        if (errno == ERANGE) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        if (DWCi_ConnTestAllow302 == 1 && returnCode == 0x72) {
            DWCi_ConnTestWork->result = 0;
            DWCi_Netcheck_SetError(0xb);
            goto done;
        }
        if (returnCode >= 100) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(6);
            goto done;
        }
        length = DWC_Http_GetBase64DecodedResult(DWCi_ConnTestHttp, "url", NULL, 0);
        if (length <= 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        dataLength = DWC_Http_GetBase64DecodedResult(DWCi_ConnTestHttp, "data", NULL, 0);
        if (dataLength <= 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        waitLength = DWC_Http_GetBase64DecodedResult(DWCi_ConnTestHttp, "wait", NULL, 0);
        url = alloc(0, length + 1);
        if (url == NULL) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(4);
            goto done;
        }
        data = alloc(0, dataLength + 1);
        if (data == NULL) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(4);
            goto done;
        }
        if (waitLength > 0) {
            waitText = alloc(0, waitLength + 1);
            if (waitText == NULL) {
                DWC_Http_Destroy(DWCi_ConnTestHttp);
                DWCi_Netcheck_SetError(4);
                goto done;
            }
        }
        length = DWC_Http_GetBase64DecodedResult(DWCi_ConnTestHttp, "url", url, length + 1);
        if (length < 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        url[length] = 0;
        length = DWC_Http_GetBase64DecodedResult(DWCi_ConnTestHttp, "data", data, dataLength + 1);
        if (length < 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        data[length] = 0;
        waitMs = 0;
        if (waitLength > 0) {
            length = DWC_Http_GetBase64DecodedResult(DWCi_ConnTestHttp, "wait", waitText, waitLength + 1);
            if (length < 0) {
                DWC_Http_Destroy(DWCi_ConnTestHttp);
                DWCi_Netcheck_SetError(9);
                goto done;
            }
            waitText[length] = 0;
            waitMs = atol(waitText);
            if (errno == ERANGE) {
                DWC_Http_Destroy(DWCi_ConnTestHttp);
                DWCi_Netcheck_SetError(9);
                goto done;
            }
            waitMs *= 1000;
            if (waitMs > 180000) {
                waitMs = 180000;
            }
        }
        DWC_Http_Destroy(DWCi_ConnTestHttp);
        DWCi_ConnTestWork->result = -5;
        DWCi_ConnTestConfig.url = url;
        DWCi_ConnTestConfig.action = DWC_HTTP_POST;
        DWCi_ConnTestConfig.bufferSize = 0x1000;
        DWCi_ConnTestConfig.alloc = alloc;
        DWCi_ConnTestConfig.free = free;
        DWCi_ConnTestConfig.timeout = 120000;
        if (DWC_Http_Create(DWCi_ConnTestHttp, &DWCi_ConnTestConfig) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        if (DWC_Http_Add_Body(DWCi_ConnTestHttp, data) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(8);
            goto done;
        }
        if (DWC_Http_FinishHeader(DWCi_ConnTestHttp) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        DWC_Http_StartThread(DWCi_ConnTestHttp, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
        if (DWCi_ConnTestHttp->thread.id != 0 && !OS_IsThreadTerminated(&DWCi_ConnTestHttp->thread)) {
            OS_JoinThread(&DWCi_ConnTestHttp->thread);
        }
        if (DWCi_ConnTestHttp->result == 2) {
            DWCi_ConnTestWork->result = -1;
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (DWCi_ConnTestHttp->result != 8) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (DWC_Http_ParseResult(DWCi_ConnTestHttp, 1) != 1) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        location = DWCi_ConnTestHttp->response.start;
        if (location == NULL) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        DWCi_ConnTestWork->hotspotUrl = alloc(0, strlen(location) + 1);
        if (DWCi_ConnTestWork->hotspotUrl == NULL) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(4);
            goto done;
        }
        strncpy(DWCi_ConnTestWork->hotspotUrl, location, strlen(location) + 1);
        DWC_Http_Destroy(DWCi_ConnTestHttp);
        OS_Sleep(waitMs);
    }
done:
    if (url != NULL) {
        free(0, url, 0);
    }
    if (data != NULL) {
        free(0, data, 0);
    }
    if (waitText != NULL) {
        free(0, waitText, 0);
    }
}

static void DWCi_Netcheck_SetError(int state) {
    OS_LockMutex(&DWCi_ConnTestWork->lock);
    DWCi_ConnTestWork->state = state;
    OS_UnlockMutex(&DWCi_ConnTestWork->lock);
}

// Stands for the code that the linker stripped: nothing calls it, so MWCC doesn't emit it, and reading
// DWCi_ConnTestUnk08 and DWCi_ConnTestUnk18 keeps them in the file's shared .bss where the ROM has them, as mbp.c does.
// Its body is a guess
static void DWCi_Netcheck_KeepUnusedData(void) {
    if (sConnTest.unk_00) {
        sConnTest.unk_00 = FALSE;
        MI_CpuFill8(DWCi_ConnTestUnk18, 0, sizeof(DWCi_ConnTestUnk18));
        DWCi_ConnTestUnk08 = 0;
    }
}
