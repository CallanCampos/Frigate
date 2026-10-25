#include <string.h>
// Before dwc/ext.h: MWCC treats strlen as writing no memory only when <string.h> declares it first, and only then
// does the thread hoist the work's field offsets into stack slots and keep a pointer across strlen, as the ROM does
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
// and its globals' names are NitroDWC's own, as its libraries' symbols and ntrtwl's reconstruction of NitroDWC have
// them where the code shows them, and ours elsewhere. The thread's error exits are written as
// ctrl-cheeb-del/pokeheartgold writes NitroDWC's connection test

// The work of the connection test (0x11f4 bytes), whose first 0x1000 bytes are the stack of its thread
typedef struct DWCNetcheck {
    u8 stack[0x1000];
    int error;
    int returncode;
    u8 http_labelvalue[0x100];
    DWCNetcheckParam param;
    char *body_302;
    char *body_wayport;
    OSThread thread;
    OSMutex mutex;
} DWCNetcheck;

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
BOOL DWCishotspot;
// The test's work
DWCNetcheck *DWCi_ConnTestWork;
// 4 bytes, and 0x18 bytes below, that only code the linker stripped reads
int DWCi_ConnTestUnk08;
// Whether a redirect (302) counts as reaching the network, which nothing sets (the SDK's DWC_Netcheck_SetAllow302)
BOOL DWCi_ConnTestAllow302;
u8 DWCi_ConnTestUnk18[0x18];
// The request and the login that the test makes
DWCHttpParam DWCi_ConnTestConfig;
DWCAuthParam DWCi_ConnTestInit;

DWCNetcheckError DWC_Netcheck_Create(DWCNetcheckParam *init) {
    DWCAuthAlloc alloc = init->alloc;

    if (DWCi_ConnTestWork != NULL) {
        return 4;
    }
    DWCi_ConnTestWork = alloc(0, sizeof(DWCNetcheck));
    if (DWCi_ConnTestWork == NULL) {
        return 4;
    }
    MI_CpuFill8(DWCi_ConnTestWork, 0, sizeof(DWCNetcheck));
    DWCi_ConnTestWork->returncode = -99999;
    DWCi_ConnTestWork->param = *init;
    if (DWCi_ConnTestHttp != NULL) {
        return 4;
    }
    DWCi_ConnTestHttp = alloc(0, sizeof(DWCHttp));
    if (DWCi_ConnTestHttp == NULL) {
        return 4;
    }
    DWCishotspot = 0;
    OS_InitMutex(&DWCi_ConnTestWork->mutex);
    DWCi_Netcheck_StartThread();
    return 0;
}

void DWC_Netcheck_Destroy(void) {
    DWCAuthFree free = DWCi_ConnTestWork->param.free;

    if (DWCi_ConnTestHttp != NULL) {
        DWC_Http_Destroy(DWCi_ConnTestHttp);
        free(0, DWCi_ConnTestHttp, 0);
        DWCi_ConnTestHttp = NULL;
    }
    DWC_Auth_Destroy();
    if (DWCi_ConnTestWork != NULL) {
        if (DWCi_ConnTestWork->body_302 != NULL) {
            free(0, DWCi_ConnTestWork->body_302, 0);
            DWCi_ConnTestWork->body_302 = NULL;
        }
        if (DWCi_ConnTestWork->body_wayport != NULL) {
            free(0, DWCi_ConnTestWork->body_wayport, 0);
            DWCi_ConnTestWork->body_wayport = NULL;
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
        DWCi_ConnTestWork->returncode = -7;
    }
}

DWCNetcheckError DWC_Netcheck_GetError(void) {
    int state;

    OS_LockMutex(&DWCi_ConnTestWork->mutex);
    state = DWCi_ConnTestWork->error;
    OS_UnlockMutex(&DWCi_ConnTestWork->mutex);
    return state;
}

int DWC_Netcheck_GetReturnCode(void) {
    return DWCi_ConnTestWork->returncode;
}

static void DWCi_Netcheck_StartThread(void) {
    if (DWCi_ConnTestWork->thread.id == 0 || OS_IsThreadTerminated(&DWCi_ConnTestWork->thread)) {
        OS_CreateThread(&DWCi_ConnTestWork->thread, DWCi_Netcheck_Thread, DWCi_ConnTestWork,
                        DWCi_ConnTestWork->stack + 0x1000, 0x1000, 16);
        OS_WakeupThreadDirect(&DWCi_ConnTestWork->thread);
    }
}

static void DWCi_Netcheck_Thread(void *arg) {
    int status;
    DWCWiFiInfo userInfo;
    DWCAuthResult nasStatus;
    DWCAuthAlloc alloc = DWCi_ConnTestWork->param.alloc;
    DWCAuthFree free = DWCi_ConnTestWork->param.free;
    char *location;
    char returnText[4];
    int returnCode;
    char *url = NULL;
    int length;
    char *data = NULL;
    int dataLength;
    char *waitText = NULL;
    int waitLength;
    int waitMs;
    DWCCalInfo info;

    for (;;) {
        DWCi_ConnTestConfig.url = sConnTest.url;
        DWCi_ConnTestConfig.action = DWCHTTP_GET;
        DWCi_ConnTestConfig.len_recvbuf = 0x1000;
        DWCi_ConnTestConfig.alloc = alloc;
        DWCi_ConnTestConfig.free = free;
        DWCi_ConnTestConfig.timeout = 20000;
        DWCi_ConnTestWork->returncode = -2;
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
        switch (DWCi_ConnTestHttp->error) {
        case 8:
            break;
        case 2:
            DWCi_ConnTestWork->returncode = -1;
        default:
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
        switch (status) {
        case 200:
            DWCi_ConnTestIp = DWCi_ConnTestHttp->hostip;
            break;
        case 302:
            DWCishotspot = 1;
            if (DWCi_ConnTestWork->body_wayport != NULL) {
                DWCi_ConnTestWork->returncode = -6;
                DWC_Http_Destroy(DWCi_ConnTestHttp);
                DWCi_ConnTestConfig.url = DWCauthhttpparam.url;
                DWCi_ConnTestConfig.action = DWCHTTP_POST;
                DWCi_ConnTestConfig.len_recvbuf = 0x200;
                DWCi_ConnTestConfig.alloc = alloc;
                DWCi_ConnTestConfig.free = free;
                DWCi_ConnTestConfig.timeout = 20000;
                if (strcmp(DWCauthhttpparam.url, "https://nas.nintendowifi.net/ac") != 0) {
                    DWCi_ConnTestConfig.ignoreca = 1;
                }
                if (DWC_Http_Create(DWCi_ConnTestHttp, &DWCi_ConnTestConfig) != 0) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(1);
                    goto done;
                }
                if (!DWC_Auth_GetCalInfoFromNVRAM(&info) || !DWC_Auth_SetCalInfoToHttp(DWCi_ConnTestHttp, &info, 1)) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(8);
                    goto done;
                }
                if (DWC_Http_Add_PostBase64Item(DWCi_ConnTestHttp, "action", "message", 7) != 0 ||
                    DWC_Http_Add_PostBase64Item(DWCi_ConnTestHttp, "HotSpotResponse", DWCi_ConnTestWork->body_wayport,
                                                strlen(DWCi_ConnTestWork->body_wayport)) != 0) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(8);
                    goto done;
                }
                free(0, DWCi_ConnTestWork->body_wayport, 0);
                DWCi_ConnTestWork->body_wayport = NULL;
                if (DWC_Http_FinishHeader(DWCi_ConnTestHttp) != 0) {
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(1);
                    goto done;
                }
                DWC_Http_StartThread(DWCi_ConnTestHttp, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
                if (DWCi_ConnTestHttp->thread.id != 0 && !OS_IsThreadTerminated(&DWCi_ConnTestHttp->thread)) {
                    OS_JoinThread(&DWCi_ConnTestHttp->thread);
                }
                switch (DWCi_ConnTestHttp->error) {
                case 2:
                    DWCi_ConnTestWork->returncode = -1;
                default:
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(3);
                    goto done;
                case 8:
                    DWC_Http_Destroy(DWCi_ConnTestHttp);
                    DWCi_Netcheck_SetError(7);
                    goto done;
                }
            }
            location = DWCi_ConnTestHttp->rep.buffer;
            if (location == NULL) {
                DWC_Http_Destroy(DWCi_ConnTestHttp);
                DWCi_Netcheck_SetError(2);
                goto done;
            }
            DWCi_ConnTestWork->body_302 = alloc(0, strlen(location) + 1);
            if (DWCi_ConnTestWork->body_302 == NULL) {
                DWC_Http_Destroy(DWCi_ConnTestHttp);
                DWCi_Netcheck_SetError(4);
                goto done;
            }
            strncpy(DWCi_ConnTestWork->body_302, location, strlen(location) + 1);
            break;
        default:
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(10);
            goto done;
        }
        DWC_Http_Destroy(DWCi_ConnTestHttp);
        DWCi_BM_GetWiFiInfo(&userInfo);
        if (userInfo.attestedUserId == 0) {
            DWCi_ConnTestWork->returncode = -3;
            DWCi_ConnTestInit.ingamesn[0] = 0;
            DWCi_ConnTestInit.gsbrcd[0] = 0;
            DWCi_ConnTestInit.alloc = DWCi_ConnTestWork->param.alloc;
            DWCi_ConnTestInit.free = DWCi_ConnTestWork->param.free;
            if (DWC_Auth_Create(&DWCi_ConnTestInit, DWCi_ConnTestHttp) != 0) {
                DWCi_Netcheck_SetError(5);
                goto done;
            }
            DWC_Auth_Join();
            if (DWC_Auth_GetError() != 0x15) {
                if (DWC_Auth_GetError() == 9) {
                    DWCi_ConnTestWork->returncode = -1;
                } else {
                    DWC_Auth_GetResult(&nasStatus);
                    if (DWCi_ConnTestAllow302 == 1 && (nasStatus.returncode == -23302 || DWC_Auth_GetError() == 0xb)) {
                        DWCi_ConnTestWork->returncode = 0;
                        DWC_Auth_Destroy();
                        DWCi_Netcheck_SetError(0xb);
                        goto done;
                    }
                    DWCi_ConnTestWork->returncode = nasStatus.returncode;
                }
                DWC_Auth_Destroy();
                DWCi_Netcheck_SetError(6);
                goto done;
            }
            DWC_Auth_Destroy();
        }
        if (status == 200) {
            DWCi_ConnTestWork->returncode = 0;
            DWCi_Netcheck_SetError(0xb);
            goto done;
        }
        DWCi_ConnTestWork->returncode = -4;
        DWCi_ConnTestConfig.url = DWCauthhttpparam.url;
        DWCi_ConnTestConfig.action = DWCHTTP_POST;
        DWCi_ConnTestConfig.len_recvbuf = 0x1000;
        DWCi_ConnTestConfig.alloc = alloc;
        DWCi_ConnTestConfig.free = free;
        DWCi_ConnTestConfig.timeout = 40000;
        if (strcmp(DWCauthhttpparam.url, "https://nas.nintendowifi.net/ac") != 0) {
            DWCi_ConnTestConfig.ignoreca = 1;
        }
        if (DWC_Http_Create(DWCi_ConnTestHttp, &DWCi_ConnTestConfig) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        if (!DWC_Auth_GetCalInfoFromNVRAM(&info) || !DWC_Auth_SetCalInfoToHttp(DWCi_ConnTestHttp, &info, 1)) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(8);
            goto done;
        }
        if (DWC_Http_Add_PostBase64Item(DWCi_ConnTestHttp, "action", "parse", 5) != 0 ||
            DWC_Http_Add_PostBase64Item(DWCi_ConnTestHttp, "HTML", DWCi_ConnTestWork->body_302,
                                        strlen(DWCi_ConnTestWork->body_302)) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(8);
            goto done;
        }
        free(0, DWCi_ConnTestWork->body_302, 0);
        DWCi_ConnTestWork->body_302 = NULL;
        if (DWC_Http_FinishHeader(DWCi_ConnTestHttp) != 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(1);
            goto done;
        }
        DWC_Http_StartThread(DWCi_ConnTestHttp, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
        if (DWCi_ConnTestHttp->thread.id != 0 && !OS_IsThreadTerminated(&DWCi_ConnTestHttp->thread)) {
            OS_JoinThread(&DWCi_ConnTestHttp->thread);
        }
        switch (DWCi_ConnTestHttp->error) {
        case 8:
            break;
        case 3:
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            if (DWCi_ConnTestAllow302 == 1) {
                DWCi_ConnTestWork->returncode = 0;
                DWCi_Netcheck_SetError(0xb);
                goto done;
            }
            DWCi_Netcheck_SetError(3);
            goto done;
        case 2:
            DWCi_ConnTestWork->returncode = -1;
        default:
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
                DWCi_ConnTestWork->returncode = 0;
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
            DWCi_ConnTestWork->returncode = 0;
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
        dataLength = DWC_Http_GetBase64DecodedResult(DWCi_ConnTestHttp, "data", data, dataLength + 1);
        if (dataLength < 0) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(9);
            goto done;
        }
        data[dataLength] = 0;
        waitMs = 0;
        if (waitLength > 0) {
            waitLength = DWC_Http_GetBase64DecodedResult(DWCi_ConnTestHttp, "wait", waitText, waitLength + 1);
            if (waitLength < 0) {
                DWC_Http_Destroy(DWCi_ConnTestHttp);
                DWCi_Netcheck_SetError(9);
                goto done;
            }
            waitText[waitLength] = 0;
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
        DWCi_ConnTestWork->returncode = -5;
        DWCi_ConnTestConfig.url = url;
        DWCi_ConnTestConfig.action = DWCHTTP_POST;
        DWCi_ConnTestConfig.len_recvbuf = 0x1000;
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
        switch (DWCi_ConnTestHttp->error) {
        case 8:
            break;
        case 2:
            DWCi_ConnTestWork->returncode = -1;
        default:
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(3);
            goto done;
        }
        if (DWC_Http_ParseResult(DWCi_ConnTestHttp, 1) != 1) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        location = DWCi_ConnTestHttp->rep.buffer;
        if (location == NULL) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(2);
            goto done;
        }
        DWCi_ConnTestWork->body_wayport = alloc(0, strlen(location) + 1);
        if (DWCi_ConnTestWork->body_wayport == NULL) {
            DWC_Http_Destroy(DWCi_ConnTestHttp);
            DWCi_Netcheck_SetError(4);
            goto done;
        }
        strncpy(DWCi_ConnTestWork->body_wayport, location, strlen(location) + 1);
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
    OS_LockMutex(&DWCi_ConnTestWork->mutex);
    DWCi_ConnTestWork->error = state;
    OS_UnlockMutex(&DWCi_ConnTestWork->mutex);
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
