#include "dwc/ac.h"
#include "dwc/bm.h"
#include "dwc/common.h"
#include "dwc/dwc.h"
#include "dwc/error.h"
#include "dwc/ext.h"
#include "dwc/friend.h"
#include "dwc/memfunc.h"
#include "dwc/nasfunc.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "nitrowifi/cps.h"
#include "nitrowifi/wcm.h"
#include <errno.h>
#include <wchar.h>

// DWC's login to the Nintendo authentication server (overlay 11, Thumb): the "ac" requests and the worker thread that
// runs them, the HTTP client with its root certificates, the connection test, the profanity check and the service
// locator. The ROM embeds no name for the file, so the name is a guess from what the code does, and the range may be
// several original files. See dwc/nasfunc.h for the layouts that the friends' match shares.

// The game's code, the letter of its region (the code's last) and the maker's code, in the ROM header that the card
// leaves in main memory
#define ROM_GAME_CODE ((const u8 *)(HW_ROM_HEADER_BUF + 0xc))
#define ROM_REGION (*(const char *)(HW_ROM_HEADER_BUF + 0xf))
#define ROM_MAKER_CODE ((const u8 *)(HW_ROM_HEADER_BUF + 0x10))

// The room for the fields of an answer and how many are used
typedef struct NasFieldTable {
    DWCHttpLabelValue *fields;
    int capacity;
    int count;
} NasFieldTable;

// What the requests to the server tell about the player and the console, as the form fields are made from them
typedef struct NasAuthInfo {
    char userId[0xe];
    char password[7];
    char gameCode[5];
    char makerCode[3];
    char unitCode[2];
    char macAddress[0xd];
    char language[3];
    char birthday[5];
    char deviceTime[0xd];
    char bssid[0xd];
    char ssid[0x20];
    char unk_6e;
    char apInfo[0xe];
    char unk_7d;
    u16 deviceName[10];
    u8 unk_92[2];
} NasAuthInfo;

// The login's work (0x13f4 bytes), whose first 0x1000 bytes are the stack of its thread
typedef struct NasWork {
    u8 stack[0x1000];
    int unk_1000;
    int step;
    DWCAuthResult result;
    DWCAuthParam param;
    u8 unk_1214[0x100];
    DWCHttp *http;
    OSThread thread;
    OSMutex lock;
    BOOL cancel;
} NasWork;

// The work of the connection test (0x11f4 bytes), whose first 0x1000 bytes are the stack of its thread
typedef struct NasPrWork {
    u8 stack[0x1000];
    int state;
    int result;
    u8 unk_1008[0x100];
    DWCAcMemFuncs memFuncs;
    char *redirectUrl;
    char *hotspotUrl;
    OSThread thread;
    OSMutex lock;
} NasPrWork;

// The profanity check's state (0x1d64 bytes): its HTTP client, the words it asked about and where the answers go
typedef struct NasWordWork {
    DWCHttp http;
    BOOL active;
    char url[0x104];
    char *flags;
    int *profaneCount;
    int count;
} NasWordWork;

// The service locator's state (0x1c5c bytes)
typedef struct NasLocator {
    DWCHttpAlloc alloc;
    DWCHttpFree free;
    DWCHttp http;
    DWCSvlResult *result;
} NasLocator;

// A root certificate that the HTTPS connection accepts: its name, its public key and the exponent of the key
typedef struct NasRootCert {
    char *name;
    int keyLength;
    u8 *key;
    int exponentLength;
    u8 *exponent;
} NasRootCert;

static void DWCi_Auth_StartThread(void);
static void DWC_Auth_Abort(void);
static void DWC_Auth_Join(void);
static int DWCi_Auth_SendRequest(int mode);
static void DWCi_Auth_Thread(void *arg);
static int DWCi_Auth_HandleResponse(void);
static int DWCi_Auth_ParseResult(void);
static int DWCi_Auth_AddPostItems(DWCHttp *http, const char *gsbrcd, const u16 *ingamesn);
static void DWCi_Auth_SetError(int step);
static void DWCi_Auth_ClearResult(void);
static BOOL DWCi_Auth_GetCurrentInfo(NasAuthInfo *info);
static BOOL DWCi_Auth_GetInfo(NasAuthInfo *info, const DWCBmWifiIdInfo *user);
static BOOL DWCi_Auth_AddCommonItems(DWCHttp *http, const NasAuthInfo *info, BOOL sendSsid);
static int DWC_Http_Create(DWCHttp *http, const DWCHttpParam *param);
static BOOL DWC_Http_FinishHeader(DWCHttp *http);
static void DWC_Http_StartThread(DWCHttp *http, u32 priority);
static void DWC_Http_Abort(DWCHttp *http);
static BOOL DWCi_Http_SleepUnlessAborted(DWCHttp *http);
static void DWCi_Http_RegisterSocket(DWCHttp *http);
static CPSInAddr DWCi_Http_ResolveHost(DWCHttp *http);
static BOOL DWCi_Http_ParseContentLength(DWCHttp *http);
static void DWCi_Http_Thread(void *arg);
static void DWC_Http_Destroy(DWCHttp *http);
static int DWCi_Http_MakeRequestLine(DWCHttp *http);
static int DWC_Http_Add_HeaderItem(DWCHttp *http, const char *name, const char *value);
static int DWC_Http_Add_PostBase64Item(DWCHttp *http, const char *name, const void *value, int size);
static int DWC_Http_Add_Body(DWCHttp *http, const char *text);
static int DWCi_Http_VerifyCertificate(int result, DWCHttpSslConnection *con, int index);
static int DWCi_Http_AllocBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int size);
static void DWCi_Http_FreeBuffer(DWCHttp *http, DWCHttpBuffer *buffer);
static int DWCi_Http_GrowBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int extra);
static int DWCi_Http_ParseUrl(DWCHttp *http, const char *url);
static int DWCi_Http_AddLabelValue(DWCHttp *http, NasFieldTable *table, const char *label, const char *value);
static int DWC_Http_ParseResult(DWCHttp *http, BOOL headersOnly);
static const char *DWC_Http_GetResult(DWCHttp *http, const char *label);
static int DWC_Http_GetBase64DecodedResult(DWCHttp *http, const char *label, char *dest, int size);
static int DWC_Http_GetRawResult(DWCHttp *http, const char *label, char *dest, int size);
static void DWCi_Http_FreeLabelValues(DWCHttp *http, DWCHttpLabelValue *fields, int count);
static void DWCi_Netcheck_StartThread(void);
static void DWCi_Netcheck_Thread(void *arg);
static void DWCi_Netcheck_SetError(int state);
static BOOL DWCi_Prof_StartRequest(const u16 **words, int count, const char *type, int timeout, char *flags,
                                   int *profaneCount, const char *region);
static BOOL DWCi_Prof_ParseResult(void);
static void *DWCi_Prof_Alloc(u32 name, u32 size);
static void DWCi_Prof_Free(u32 name, void *ptr, u32 size);
static BOOL DWCi_Svl_ParseResult(void);

// The root certificates (defined at the end of the file)
static u8 sNasCert0Key[256];
static u8 sNasCert1Key[256];
static u8 sNasCert2Key[128];
static u8 sNasCert3Key[128];
static u8 sNasCert4Key[256];
static u8 sNasCert5Key[128];
static u8 sNasCert6Key[128];
static u8 sNasCert7Key[128];
static u8 sNasCert8Key[128];
static u8 sNasCert9Key[128];
static u8 sNasCert10Key[256];
static u8 sNasCert11Key[128];
static u8 sNasCert12Key[125];
static char sNasCert0Name[53];
static char sNasCert1Name[50];
static char sNasCert2Name[80];
static char sNasCert3Name[41];
static char sNasCert4Name[48];
static char sNasCert5Name[79];
static char sNasCert6Name[88];
static char sNasCert7Name[136];
static char sNasCert8Name[126];
static char sNasCert9Name[147];
static char sNasCert10Name[156];
static char sNasCert11Name[67];
static char sNasCert12Name[67];
static NasRootCert sNasCert0;
static NasRootCert sNasCert1;
static NasRootCert sNasCert2;
static NasRootCert sNasCert3;
static NasRootCert sNasCert4;
static NasRootCert sNasCert5;
static NasRootCert sNasCert6;
static NasRootCert sNasCert7;
static NasRootCert sNasCert8;
static NasRootCert sNasCert9;
static NasRootCert sNasCert10;
static NasRootCert sNasCert11;
static NasRootCert sNasCert12;

// The statics are declared from the highest address down: the compiler lays out .bss in reverse order of declaration
static NasLocator *sLocator;
static NasWordWork *sWords;
static DWCAuthParam sPrInit;
static DWCHttpParam sPrConfig;
// 0x18 bytes that no code of the file reads
static u8 sPrUnk18[0x18];
// Whether the requests to the server send the access point's SSID
static BOOL sSendSsid;
// The connection test's state (0x021905fc): its HTTP client, its work and the address it last reached
static struct {
    DWCHttp *http;
    NasPrWork *work;
    int unk_08;
    CPSInAddr ip;
    // Whether a redirect (302) counts as reaching the network, which nothing sets (the SDK's
    // DWC_Netcheck_SetAllow302)
    int allow302;
} sPr;
static int sTestServer;
static DWCHttp sHttp;
static DWCAuthParam sInit;
// The outcome that the game reads: whether the login ended, its error and whether it worked
static struct {
    int done;
    int code;
    int succeeded;
    u64 userId;
    u64 startTime;
} sResult;
// The login's one state (0x0218e924): whether it asks for the test flag, its work and whether the last answer was
// good
static struct {
    int testMode;
    NasWork *work;
    int succeeded;
    DWCBmWifiIdInfo userInfo;
} sNas;

static DWCHttpParam sConfig = { "", 0, 0x1000, NULL, NULL, 0, 20000 };
static char sDefaultUrl[] = "https://nas.nintendowifi.net/ac";

void DWC_Auth_SetCustomNas(const char *url) {
    sConfig.url = url;
}

const char *DWCi_Auth_GetCustomNas(void) {
    return sConfig.url;
}

int DWC_Auth_Create(const DWCAuthParam *init, DWCHttp *http) {
    DWCHttpAlloc alloc = init->alloc;

    if (sNas.work != NULL) {
        return 2;
    }
    sNas.work = alloc(0, sizeof(NasWork));
    if (sNas.work == NULL) {
        return 2;
    }
    MI_CpuFill8(sNas.work, 0, sizeof(NasWork));
    sNas.work->http = http;
    sNas.succeeded = 0;
    MI_CpuFill8(&sNas.work->result, 0, sizeof(DWCAuthResult));
    sNas.work->result.returnCode = 20100;
    sNas.work->param = *init;
    sNas.work->param.ingamesn[0x19] = 0;
    sNas.work->param.gsbrcd[0xb] = 0;
    sNas.work->step = DWCi_Auth_SendRequest(1);
    if (sNas.work->step == 0) {
        sNas.testMode = 0;
        DWCi_Auth_StartThread();
        return 0;
    }
    return sNas.work->step;
}

static void DWCi_Auth_StartThread(void) {
    OS_InitMutex(&sNas.work->lock);
    sNas.work->cancel = 0;
    if (sNas.work->thread.id == 0 || OS_IsThreadTerminated(&sNas.work->thread)) {
        OS_CreateThread(&sNas.work->thread, DWCi_Auth_Thread, &sNas.work, sNas.work->stack + 0x1000, 0x1000, 16);
        OS_WakeupThreadDirect(&sNas.work->thread);
    }
}

static void DWC_Auth_Abort(void) {
    if (sNas.work != NULL) {
        OS_LockMutex(&sNas.work->lock);
        sNas.work->cancel = 1;
        OS_UnlockMutex(&sNas.work->lock);
        if (sNas.work->http != NULL) {
            DWC_Http_Abort(sNas.work->http);
        }
        if (sNas.work->thread.id != 0 && !OS_IsThreadTerminated(&sNas.work->thread)) {
            OS_JoinThread(&sNas.work->thread);
        }
    }
}

void DWC_Auth_Destroy(void) {
    NasWork *work = sNas.work;

    if (work != NULL) {
        DWCHttpFree free = work->param.free;

        if (work->http != NULL) {
            DWC_Http_Destroy(work->http);
        }
        free(0, sNas.work, 0);
        sNas.work = NULL;
    }
}

static void DWC_Auth_Join(void) {
    if (sNas.work->thread.id != 0 && !OS_IsThreadTerminated(&sNas.work->thread)) {
        OS_JoinThread(&sNas.work->thread);
    }
}

int DWC_Auth_GetError(void) {
    int step;

    if (sNas.work == NULL) {
        return 0x16;
    }
    OS_LockMutex(&sNas.work->lock);
    step = sNas.work->step;
    OS_UnlockMutex(&sNas.work->lock);
    return step;
}

void DWC_Auth_GetResult(DWCAuthResult *out) {
    if (sNas.work == NULL) {
        MI_CpuFill8(out, 0, sizeof(DWCAuthResult));
        // BUG: without a login, the answer is then copied from where its work would be, near address 0
#ifdef BUGFIX
        return;
#endif
    }
    MI_CpuCopy8(&sNas.work->result, out, sizeof(DWCAuthResult));
    if (out->returnCode < 20000 || out->returnCode >= 30000) {
        out->returnCode = 20998;
    }
    if (out->returnCode >= 20100) {
        out->returnCode = -out->returnCode;
    }
}

static int DWCi_Auth_SendRequest(int mode) {
    if (strcmp(sConfig.url, sDefaultUrl) != 0) {
        sConfig.ignoreCa = 1;
    }
    sConfig.alloc = sNas.work->param.alloc;
    sConfig.free = sNas.work->param.free;
    if (DWC_Http_Create(sNas.work->http, &sConfig) != 0) {
        return 4;
    }
    if (mode == 1) {
        DWCBm_GetWifiIdInfo(&sNas.userInfo);
    }
    sNas.work->step = DWCi_Auth_AddPostItems(sNas.work->http, sNas.work->param.gsbrcd, sNas.work->param.ingamesn);
    if (sNas.work->step != 0) {
        return 4;
    }
    if (DWC_Http_FinishHeader(sNas.work->http) != 0) {
        return 4;
    }
    DWC_Http_StartThread(sNas.work->http, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
    return 0;
}

static void DWCi_Auth_Thread(void *arg) {
    int retries = 0;
    int mode;
    u64 start;
    int result;

    for (;;) {
        if (sNas.work->http->thread.id != 0 && !OS_IsThreadTerminated(&sNas.work->http->thread)) {
            OS_JoinThread(&sNas.work->http->thread);
        }
        if (sNas.work->http->result != 8) {
            sNas.work->result.returnCode = 20100;
            if (sNas.work->http->result == 7) {
                DWCi_Auth_SetError(0x14);
                return;
            }
            if (retries > 2) {
                if (sNas.work->http->result == 2) {
                    DWCi_Auth_SetError(9);
                } else if (sNas.work->http->result == 3) {
                    DWCi_Auth_SetError(0xb);
                } else {
                    DWCi_Auth_SetError(0xd);
                }
                return;
            }
            retries++;
            mode = 1;
        } else {
            result = DWCi_Auth_HandleResponse();
            switch (result) {
            case 0x15:
                DWCi_Auth_SetError(0x15);
                return;
            case 0x10:
                if (retries > 2) {
                    DWCi_Auth_SetError(0x10);
                    return;
                }
                retries++;
                mode = 0;
                break;
            case 0x11:
                DWCi_Auth_SetError(result);
                return;
            default:
                if (retries >= 2) {
                    DWCi_Auth_SetError(result);
                    return;
                }
                retries++;
                mode = 1;
                break;
            }
        }
        start = OS_GetTick();
        while (OS_TicksToMilliSeconds(OS_GetTick() - start) < 5000) {
            OS_LockMutex(&sNas.work->lock);
            if (sNas.work->cancel == 1) {
                sNas.work->result.returnCode = 20100;
                OS_UnlockMutex(&sNas.work->lock);
                DWCi_Auth_SetError(0x14);
                return;
            }
            OS_UnlockMutex(&sNas.work->lock);
            OS_Sleep(5000);
        }
        DWC_Http_Destroy(sNas.work->http);
        OS_LockMutex(&sNas.work->lock);
        sNas.work->step = DWCi_Auth_SendRequest(mode);
        if (sNas.work->step != 0) {
            sNas.work->result.returnCode = 20100;
            OS_UnlockMutex(&sNas.work->lock);
            return;
        }
        OS_UnlockMutex(&sNas.work->lock);
    }
}

static int DWCi_Auth_HandleResponse(void) {
    DWCHttpAlloc alloc = sNas.work->param.alloc;
    DWCHttpFree free = sNas.work->param.free;
    void *buffer;

    if (DWC_Http_ParseResult(sNas.work->http, 0) != 1) {
        sNas.work->result.returnCode = 20100;
        return 0xe;
    }
    if (DWCi_Auth_ParseResult() != 0) {
        return 0xe;
    }
    if (sNas.work->result.returnCode < 20100) {
        if (sNas.work->result.returnCode == 20002) {
            buffer = alloc(0, 0x101f);
            if (buffer == NULL) {
                sNas.work->result.returnCode = 20100;
                return 2;
            }
            if (DWCBm_ConfirmWifiId(&sNas.userInfo, (void *)(((u32)buffer + 0x1f) & ~0x1f)) != 1) {
                free(0, buffer, 0);
                sNas.work->result.returnCode = 20100;
                return 0xf;
            }
            free(0, buffer, 0);
        }
        return 0x15;
    }
    switch (sNas.work->result.returnCode) {
    case 20104:
        DWCBm_RenewPendingWifiId(&sNas.userInfo);
        sNas.work->result.returnCode = 20104;
        return 0x10;
    case 20108:
        buffer = alloc(0, 0x1000);
        if (buffer == NULL) {
            sNas.work->result.returnCode = 20108;
            return 0x11;
        }
        DWCBm_MakeAndSaveWifiId((void *)(((u32)buffer + 0x1f) & ~0x1f));
        free(0, buffer, 0);
        sNas.work->result.returnCode = 20108;
        return 0x11;
    }
    return 0x12;
}

static int DWCi_Auth_ParseResult(void) {
    char *end = NULL;
    int httpResult;
    int returnCode;

    end = (char *)DWC_Http_GetResult(sNas.work->http, "Date");
    if (end != NULL) {
        DWCi_SetNasTimeFromDateHeader(end);
    }
    end = NULL;
    errno = 0;
    httpResult = atol(DWC_Http_GetResult(sNas.work->http, "httpresult"));
    if (errno == ERANGE) {
        sNas.work->result.returnCode = 20101;
        return 0xc;
    }
    if (httpResult != 200) {
        sNas.work->result.returnCode = httpResult + 23000;
        return 0x12;
    }
    if (DWC_Http_GetBase64DecodedResult(sNas.work->http, "returncd", sNas.work->result.retcode, 4) <= 0) {
        sNas.work->result.returnCode = 20101;
        return 0xe;
    }
    returnCode = strtol(sNas.work->result.retcode, &end, 10);
    if (end != sNas.work->result.retcode + strlen(sNas.work->result.retcode)) {
        sNas.work->result.returnCode = 20101;
        return 0xc;
    }
    sNas.work->result.returnCode = returnCode + 20000;
    if (returnCode < 100) {
        sNas.work->result.token[0] = 0;
        sNas.work->result.locator[0] = 0;
        sNas.work->result.challenge[0] = 0;
        sNas.work->result.datetime[0] = 0;
        sNas.work->result.cookie[0] = 0;
        DWC_Http_GetBase64DecodedResult(sNas.work->http, "token", sNas.work->result.token, 0x12d);
        DWC_Http_GetBase64DecodedResult(sNas.work->http, "locator", sNas.work->result.locator, 0x33);
        DWC_Http_GetBase64DecodedResult(sNas.work->http, "challenge", sNas.work->result.challenge, 9);
        DWC_Http_GetBase64DecodedResult(sNas.work->http, "datetime", sNas.work->result.datetime, 0xf);
        DWC_Http_GetRawResult(sNas.work->http, "Set-Cookie", sNas.work->result.cookie, 0x41);
        sNas.work->result.cookie[0x2b] = 0;
        if (returnCode != 40) {
            sNas.succeeded = 1;
        } else {
            sNas.succeeded = 2;
        }
    }
    return 0;
}

static int DWCi_Auth_AddPostItems(DWCHttp *http, const char *gameCode, const u16 *name) {
    NasAuthInfo info;

    if (sNas.userInfo.id == 0) {
        if (DWC_Http_Add_PostBase64Item(http, "action", "acctcreate", strlen("acctcreate")) != 0) {
            return 8;
        }
    } else {
        if (DWC_Http_Add_PostBase64Item(http, "action", "login", strlen("login")) != 0) {
            return 8;
        }
        if (DWC_Http_Add_PostBase64Item(http, "gsbrcd", gameCode, strlen(gameCode)) != 0) {
            return 8;
        }
    }
    if (!DWCi_Auth_GetInfo(&info, &sNas.userInfo)) {
        return 5;
    }
    if (!DWCi_Auth_AddCommonItems(http, &info, sSendSsid)) {
        return 8;
    }
    if (sNas.testMode == 1) {
        if (DWC_Http_Add_PostBase64Item(http, "iswfc", "Y", strlen("Y")) != 0) {
            return 8;
        }
    }
    if (wcslen(name) != 0) {
        if (DWC_Http_Add_PostBase64Item(http, "ingamesn", name, wcslen(name) * 2) != 0) {
            return 8;
        }
    }
    return 0;
}

static void DWCi_Auth_SetError(int step) {
    OS_LockMutex(&sNas.work->lock);
    sNas.work->step = step;
    OS_UnlockMutex(&sNas.work->lock);
}

BOOL DWCi_Auth_InitInterface(int server) {
    switch (server) {
    case 0:
        DWC_Auth_SetCustomNas("https://nas.test.nintendowifi.net/ac");
        break;
    case 2:
        DWC_Auth_SetCustomNas("https://nas.dev.nintendowifi.net/ac");
        break;
    case 1:
        DWC_Auth_SetCustomNas("https://nas.nintendowifi.net/ac");
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

static void DWCi_Auth_ClearResult(void) {
    sResult.done = 0;
    sResult.succeeded = 0;
    sResult.code = 0;
    MI_CpuFill8(&sHttp, 0, sizeof(DWCHttp));
}

BOOL DWCi_Auth_StartAuthentication(const u16 *ingamesn, const char *gsbrcd, u64 userId, DWCHttpAlloc alloc,
                                   DWCHttpFree free) {
    DWCi_Auth_ClearResult();
    MI_CpuCopy8(ingamesn, sInit.ingamesn, sizeof(sInit.ingamesn));
    MI_CpuCopy8(gsbrcd, sInit.gsbrcd, sizeof(sInit.gsbrcd));
    sInit.alloc = alloc;
    sInit.free = free;
    sResult.startTime = OS_GetTick();
    sResult.userId = userId;
    return DWC_Auth_Create(&sInit, &sHttp) == 0 ? TRUE : FALSE;
}

void DWCi_Auth_AbortAuthentication(void) {
    DWC_Auth_Abort();
    DWC_Auth_Destroy();
}

void DWCi_Auth_ProcessAuthentication(void) {
    DWCAuthResult done;
    DWCAuthResult failed;
    int step = DWC_Auth_GetError();
    int code;

    if (step == 0x15) {
        DWC_Auth_GetResult(&done);
        if (done.returnCode == 20002) {
            DWC_Auth_Destroy();
            DWCi_Auth_ClearResult();
            if (DWC_Auth_Create(&sInit, &sHttp) == 0) {
                return;
            }
            code = -20100;
        } else {
            sResult.done = 1;
            sResult.succeeded = 1;
            return;
        }
    } else if (step != 0) {
        DWC_Auth_GetResult(&failed);
        DWC_Auth_Destroy();
        code = failed.returnCode;
    } else {
        return;
    }
    sResult.done = 1;
    sResult.succeeded = 0;
    sResult.code = code;
}

BOOL DWCi_Auth_IsFinished(void) {
    return sResult.done;
}

BOOL DWCi_Auth_IsSucceeded(void) {
    return sResult.succeeded;
}

int DWCi_Auth_GetErrorCode(void) {
    return sResult.code;
}

void DWCi_Auth_GetGameSpyToken(char *token, char *challenge) {
    DWCAuthResult status;

    DWC_Auth_GetResult(&status);
    strncpy(token, status.token, 0x12d);
    token[0x12c] = 0;
    strncpy(challenge, status.challenge, 9);
    challenge[8] = 0;
}

void DWCi_Auth_EndAuthentication(void) {
    DWC_Auth_Destroy();
}

u64 DWCi_Auth_GetConsoleUserId(void) {
    DWCBmWifiId ids;

    DWCBm_GetWifiId(&ids);
    if (ids.isSet != 0) {
        return ids.id;
    }
    return ids.pendingId;
}

BOOL DWCi_Auth_HasConsoleUserId(void) {
    DWCBmWifiId ids;

    DWCBm_GetWifiId(&ids);
    return ids.isSet;
}

static BOOL DWCi_Auth_GetCurrentInfo(NasAuthInfo *info) {
    DWCBmWifiIdInfo user;

    DWCBm_GetWifiIdInfo(&user);
    return DWCi_Auth_GetInfo(info, &user);
}

static BOOL DWCi_Auth_GetInfo(NasAuthInfo *info, const DWCBmWifiIdInfo *user) {
    u16 channel;
    u8 mac[6];
    RTCDate date;
    RTCTime time;
    OSOwnerInfo owner;
    u32 irq;
    u64 id;
    const u8 *apMac;
    u8 *ssid;
    int i;
    u32 level;

    MI_CpuFill8(info, 0, sizeof(NasAuthInfo));
    id = user->id;
    if (id == 0) {
        id = user->pendingId;
    }
    OS_SNPrintf(info->userId, 0xe, "%013llu", id);
    OS_SNPrintf(info->password, 7, "%03u", user->number);
    if (*ROM_GAME_CODE == 0) {
        OS_Terminate();
    }
    MI_CpuCopy8(ROM_GAME_CODE, info->gameCode, 4);
    if (*ROM_MAKER_CODE == 0) {
        OS_Terminate();
    }
    MI_CpuCopy8(ROM_MAKER_CODE, info->makerCode, 2);
    info->unitCode[0] = '0';
    OS_GetMacAddress(mac);
    for (i = 0; i < 6; i++) {
        OS_SPrintf(&info->macAddress[i * 2], "%02x", mac[i]);
    }
    OS_GetOwnerInfo(&owner);
    if (owner.language >= 8) {
        owner.language = 1;
    }
    OS_SNPrintf(info->language, 3, "%02x", owner.language);
    MI_CpuCopy8(owner.nickName, info->deviceName, 0x14);
    OS_SNPrintf(info->birthday, 5, "%02x%02x", owner.birthday.month, owner.birthday.day);
    if (RTC_GetDateTime(&date, &time) != 0) {
        return FALSE;
    }
    OS_SNPrintf(info->deviceTime, 0xd, "%02d%02d%02d%02d%02d%02d", date.year, date.month, date.day, time.hour,
                time.minute, time.second);
    irq = OS_DisableInterrupts();
    apMac = WCM_GetApMacAddress();
    DC_InvalidateRange((void *)apMac, 6);
    if (apMac == NULL) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    for (i = 0; i < 6; i++) {
        OS_SPrintf(&info->bssid[i * 2], "%02x", apMac[i]);
    }
    level = DWCAc_GetApType();
    if (level == 0xff || level > 99) {
        level = 99;
    }
    OS_SNPrintf(info->apInfo, 0xe, "%02d:0000000-00", level);
    ssid = (u8 *)WCM_GetConnectedApSsid(&channel);
    DC_InvalidateRange(ssid, 0x20);
    if (ssid == NULL) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    MI_CpuCopy8(ssid, info->ssid, 0x20);
    DWCAc_GetApSpotInfo(&info->apInfo[3]);
    OS_RestoreInterrupts(irq);
    return TRUE;
}

static BOOL DWCi_Auth_AddCommonItems(DWCHttp *http, const NasAuthInfo *info, int sendSsid) {
    char text[0x21];

    OS_SNPrintf(text, 0x21, "%03d%03d", 5, 3);
    if (DWC_Http_Add_PostBase64Item(http, "sdkver", text, strlen(text)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "userid", info->userId, strlen(info->userId)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "passwd", info->password, strlen(info->password)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "bssid", info->bssid, strlen(info->bssid)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "apinfo", info->apInfo, strlen(info->apInfo)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "gamecd", info->gameCode, strlen(info->gameCode)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "makercd", info->makerCode, strlen(info->makerCode)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "unitcd", info->unitCode, strlen(info->unitCode)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "macadr", info->macAddress, strlen(info->macAddress)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "lang", info->language, strlen(info->language)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "birth", info->birthday, strlen(info->birthday)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "devtime", info->deviceTime, strlen(info->deviceTime)) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_PostBase64Item(http, "devname", info->deviceName, wcslen(info->deviceName) * 2) != 0) {
        return FALSE;
    }
    if (sendSsid == 1) {
        if (DWC_Http_Add_PostBase64Item(http, "ssid", info->ssid, strlen(info->ssid)) != 0) {
            return FALSE;
        }
    }
    OS_SNPrintf(text, 0x21, "Nitro WiFi SDK/%d.%d", 5, 3);
    if (DWC_Http_Add_HeaderItem(http, "User-Agent", text) != 0) {
        return FALSE;
    }
    if (DWC_Http_Add_HeaderItem(http, "HTTP_X_GAMECD", info->gameCode) != 0) {
        return FALSE;
    }
    return TRUE;
}

static int DWC_Http_Create(DWCHttp *http, const DWCHttpParam *config) {
    DWCHttpAlloc alloc = config->alloc;

    MI_CpuFill8(http, 0, sizeof(DWCHttp));
    http->contentLength = -1;
    http->bodyLength = -1;
    http->param = *config;
    http->rcvBuffer = alloc(0, 0xb68);
    if (http->rcvBuffer == NULL) {
        http->result = 1;
        return 1;
    }
    http->sndBuffer = alloc(0, 0x5ea);
    if (http->sndBuffer == NULL) {
        http->result = 1;
        return 1;
    }
    if (DWCi_Http_AllocBuffer(http, &http->response, http->param.bufferSize) == 0) {
        http->result = 1;
        return 1;
    }
    if (DWCi_Http_ParseUrl(http, config->url) == 0) {
        http->result = 1;
        return 1;
    }
    http->result = DWCi_Http_MakeRequestLine(http);
    if (http->result == 0) {
        http->started = 0xff;
    }
    return http->result;
}

static BOOL DWC_Http_FinishHeader(DWCHttp *http) {
    char length[8];
    int size;

    if (DWC_Http_Add_HeaderItem(http, "Connection", "close") != 0) {
        return TRUE;
    }
    size = strlen(strstr(http->request.start, "\r\n\r\n") + 4);
    if (size != 0) {
        OS_SNPrintf(length, 7, "%d", size);
        if (DWC_Http_Add_HeaderItem(http, "Content-Length", length) != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

static void DWC_Http_StartThread(DWCHttp *http, u32 priority) {
    http->cancelled = 0;
    OS_InitMutex(&http->cancelLock);
    OS_InitMutex(&http->lock);
    if (http->param.ignoreCa == 1) {
        sTestServer = 1;
    } else {
        sTestServer = 0;
    }
    if (http->thread.id == 0 || OS_IsThreadTerminated(&http->thread)) {
        OS_CreateThread(&http->thread, DWCi_Http_Thread, http, http->stack + 0x1000, 0x1000, priority);
        OS_WakeupThreadDirect(&http->thread);
    }
}

static void DWC_Http_Abort(DWCHttp *http) {
    if (http->started == 0xff) {
        OS_LockMutex(&http->cancelLock);
        http->cancelled = 1;
        OS_UnlockMutex(&http->cancelLock);
    }
}

static BOOL DWCi_Http_SleepUnlessAborted(DWCHttp *http) {
    if (http->secure == 1) {
        OS_GetLowEntropyData(http->entropy);
        CPS_SslAddRandomSeed(http->entropy, 0x20);
    }
    OS_LockMutex(&http->cancelLock);
    if (http->cancelled == 1) {
        OS_UnlockMutex(&http->cancelLock);
        return FALSE;
    }
    OS_UnlockMutex(&http->cancelLock);
    OS_Sleep(10);
    return TRUE;
}

static void DWCi_Http_RegisterSocket(DWCHttp *http) {
    MI_CpuFill8(&http->soc, 0, sizeof(CPSSoc));
    http->soc.rcvbuf_size = 0xb68;
    http->soc.rcvbuf = http->rcvBuffer;
    http->soc.sndbuf_size = 0x5ea;
    http->soc.sndbuf = http->sndBuffer;
    CPS_SocRegister(&http->soc);
}

static CPSInAddr DWCi_Http_ResolveHost(DWCHttp *http) {
    return CPS_Resolve(http->host);
}

static BOOL DWCi_Http_ParseContentLength(DWCHttp *http) {
    DWCHttpBuffer *response = &http->response;
    char *text = response->start;
    char *body;
    char *lineEnd;
    char *lengthText;
    char saved;
    int labelLength;

    if (strstr(text, "\r\n\r\n") == NULL) {
        return FALSE;
    }
    body = strstr(text, "\r\n\r\n") + 4;
    OS_LockMutex(&http->lock);
    http->bodyLength = response->cur - body;
    OS_UnlockMutex(&http->lock);
    lengthText = strstr(response->start, "Content-Length: ");
    if (lengthText == NULL) {
        return TRUE;
    }
    labelLength = strlen("Content-Length: ");
    lineEnd = strstr(lengthText + labelLength, "\r\n");
    saved = *lineEnd;
    *lineEnd = 0;
    OS_LockMutex(&http->lock);
    http->contentLength = atol(lengthText + labelLength);
    OS_UnlockMutex(&http->lock);
    *lineEnd = saved;
    return TRUE;
}

static void DWCi_Http_Thread(void *arg) {
    static void *sNasRootCerts[12] = { &sNasCert10, &sNasCert9, &sNasCert11, &sNasCert12, &sNasCert1, &sNasCert2,
                                       &sNasCert3,  &sNasCert0, &sNasCert8,  &sNasCert7,  &sNasCert4, &sNasCert5 };
    DWCHttp *http = arg;
    DWCHttpBuffer *response = &http->response;
    CPSSoc *soc = &http->soc;
    int headersDone = 0;
    int timeout = http->param.timeout;
    CPSInAddr ip;
    char *request;
    int sent;
    u32 length;
    int room;
    int size;
    u64 lastData;
    u8 *data;
    char *old;

    if (timeout <= 0) {
        timeout = 60000;
    }
    DWCi_Http_RegisterSocket(http);
    ip = DWCi_Http_ResolveHost(http);
    if (ip == 0) {
        http->result = 2;
        return;
    }
    http->ip = ip;
    CPS_SocUse();
    if (http->secure == 1) {
        MI_CpuFill8(&http->con, 0, sizeof(DWCHttpSslConnection));
        http->con.validateCertificate = DWCi_Http_VerifyCertificate;
        http->con.hostname = http->host;
        soc->con = &http->con;
        CPS_SslSetNamedParams(sNasRootCerts, 12);
        CPS_SetSsl(TRUE);
    }
    CPS_SocBind(0, http->port, ip);
    if (CPS_TcpConnect() != 0) {
        http->result = 3;
        CPS_SocRelease();
        CPS_SocUnRegister();
        return;
    }
    request = http->request.start;
    sent = CPS_SocWrite((u8 *)request, strlen(request));
    if (sent <= 0) {
        http->result = 5;
        goto cleanup;
    }
    CPS_SocFlush();
    if (!DWCi_Http_SleepUnlessAborted(http)) {
        http->result = 7;
        goto cleanup;
    }
    response->cur = response->start;
    response->end = response->start + response->capacity;
    lastData = OS_GetTick();
    for (;;) {
        if (CPSMyIp == 0) {
            http->result = 5;
            goto cleanup;
        }
        size = CPS_SocGetLength();
        length = size;
        if (size < 0) {
            break;
        }
        if (size > 0) {
            lastData = OS_GetTick();
            data = CPS_SocRead(&length);
            if (data == NULL) {
                break;
            }
            room = response->end - 1 - response->cur;
            size = length;
            if (size >= room) {
                size = room;
            }
            MI_CpuCopy8(data, response->cur, size);
            old = response->cur;
            response->cur = old + size;
            old[size] = 0;
            if (headersDone == 1) {
                OS_LockMutex(&http->lock);
                http->bodyLength += size;
                OS_UnlockMutex(&http->lock);
            } else {
                headersDone = DWCi_Http_ParseContentLength(http);
            }
            if (length > size) {
                CPS_SocConsume(length);
                break;
            }
            CPS_SocConsume(size);
        }
        if (http->contentLength >= 0 && http->contentLength <= http->bodyLength) {
            break;
        }
        if (OS_TicksToMilliSeconds(OS_GetTick() - lastData) > timeout) {
            http->result = 6;
            goto cleanup;
        }
        if (!DWCi_Http_SleepUnlessAborted(http)) {
            http->result = 7;
            goto cleanup;
        }
    }
    CPS_TcpShutdown();
    CPS_TcpClose();
    CPS_SocRelease();
    CPS_SocUnRegister();
    http->result = 8;
    return;
cleanup:
    CPS_TcpShutdown();
    CPS_TcpClose();
    CPS_SocRelease();
    CPS_SocUnRegister();
}

static void DWC_Http_Destroy(DWCHttp *http) {
    DWCHttpFree free = http->param.free;

    if (http != NULL) {
        DWCi_Http_FreeLabelValues(http, http->fields, 0x20);
        DWCi_Http_FreeBuffer(http, &http->response);
        DWCi_Http_FreeBuffer(http, &http->request);
        if (http->rcvBuffer != NULL) {
            free(0, http->rcvBuffer, 0);
            http->rcvBuffer = NULL;
        }
        if (http->sndBuffer != NULL) {
            free(0, http->sndBuffer, 0);
            http->sndBuffer = NULL;
        }
        MI_CpuFill8(http, 0, sizeof(DWCHttp));
    }
}

static int DWCi_Http_MakeRequestLine(DWCHttp *http) {
    DWCHttpBuffer *request = &http->request;
    const char *format = "POST /%s HTTP/1.0\r\nContent-type: application/x-www-form-urlencoded\r\nHost: %s\r\n\r\n";
    int length;

    if (http->param.get != 0) {
        format = "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n";
    }
    length = strlen(http->host) + (strlen(format) - 4 + strlen(http->path));
    if (DWCi_Http_AllocBuffer(http, &http->request, length + 0x400) != 1) {
        return 1;
    }
    request->cur += OS_SNPrintf(request->cur, request->capacity, format, http->path, http->host);
    return 0;
}

static int DWC_Http_Add_HeaderItem(DWCHttp *http, const char *name, const char *value) {
    int length;
    DWCHttpBuffer *request = &http->request;
    char *line;
    char saved;

    length = strlen(value) + (strlen("%s: %s\r\n") - 4 + strlen(name));
    if (length + 1 > request->end - request->cur) {
        if (DWCi_Http_GrowBuffer(http, request, length - (request->end - request->cur) + 1) == 0) {
            return 1;
        }
    }
    line = strstr(request->start, "\r\n\r\n") + 2;
    saved = *line;
    memmove(line + length, line, strlen(line) + 1);
    line[OS_SNPrintf(line, length + 1, "%s: %s\r\n", name, value)] = saved;
    request->cur += length;
    return 0;
}

static int DWC_Http_Add_PostBase64Item(DWCHttp *http, const char *name, const void *value, int size) {
    DWCHttpBuffer *request = &http->request;
    const char *format = "%s=";
    int encoded;
    int length;
    int room;

    if (http->fieldCount != 0) {
        format = "&%s=";
    }
    http->fieldCount++;
    encoded = DWC_Base64Encode(value, size, NULL, 0);
    length = encoded + (strlen(format) - 2 + strlen(name));
    room = request->end - request->cur;
    if (length > room) {
        if (DWCi_Http_GrowBuffer(http, request, length - room + 1) == 0) {
            return 1;
        }
        room = request->end - request->cur;
    }
    request->cur += OS_SNPrintf(request->cur, room, format, name);
    if (DWC_Base64Encode(value, size, request->cur, request->end - request->cur - 1) < 0) {
        return 1;
    }
    request->cur += encoded;
    *request->cur = 0;
    return 0;
}

static int DWC_Http_Add_Body(DWCHttp *http, const char *text) {
    int length;
    DWCHttpBuffer *request = &http->request;
    int room;
    int written;

    length = strlen(text);
    room = request->end - request->cur;
    if (length > room) {
        if (DWCi_Http_GrowBuffer(http, request, length - room + 1) == 0) {
            return 1;
        }
        room = request->end - request->cur;
    }
    written = OS_SNPrintf(request->cur, room, "%s", text);
    if (written != length) {
        return 1;
    }
    request->cur += written;
    return 0;
}

static int DWCi_Http_VerifyCertificate(int result, DWCHttpSslConnection *con, int index) {
    if (result & 0x8000) {
        result &= ~0x8000;
    }
    return result;
}

static int DWCi_Http_AllocBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int size) {
    DWCHttpAlloc alloc = http->param.alloc;

    if (size == 0) {
        return 0;
    }
    buffer->start = alloc(0, size);
    if (buffer->start == NULL) {
        return 0;
    }
    buffer->cur = buffer->start;
    buffer->end = buffer->start + size;
    buffer->capacity = size;
    return 1;
}

static void DWCi_Http_FreeBuffer(DWCHttp *http, DWCHttpBuffer *buffer) {
    DWCHttpFree free = http->param.free;

    if (buffer->start != NULL) {
        free(0, buffer->start, 0);
    }
    MI_CpuFill8(buffer, 0, sizeof(DWCHttpBuffer));
}

static int DWCi_Http_GrowBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int extra) {
    DWCHttpAlloc alloc = http->param.alloc;
    DWCHttpFree free = http->param.free;
    char *start;

    if (extra <= 0) {
        return 0;
    }
    start = alloc(0, buffer->capacity + extra);
    if (start == NULL) {
        return 0;
    }
    MI_CpuCopy8(buffer->start, start, buffer->capacity);
    free(0, buffer->start, 0);
    if (start == NULL) {
        return 0;
    }
    buffer->cur += start - buffer->start;
    buffer->start = start;
    buffer->capacity += extra;
    buffer->end = start + buffer->capacity;
    return 1;
}

static int DWCi_Http_ParseUrl(DWCHttp *http, const char *url) {
    char *port = NULL;
    char *found;

    if (strlen(url) >= 0x100) {
        return 0;
    }
    strncpy(http->url, url, 0x100);
    if (strlen(url) != strlen(http->url)) {
        return 0;
    }
    if (strstr(http->url, "http://") != NULL) {
        http->host = http->url + 7;
        http->secure = 0;
        http->port = 80;
    } else {
        found = strstr(http->url, "https://");
        if (found == NULL) {
            return 0;
        }
        http->host = found + 8;
        http->secure = 1;
        http->port = 443;
    }
    found = strstr(http->host, ":");
    if (found != NULL) {
        port = found + 1;
        *found = 0;
    }
    found = strstr(http->host, "/");
    if (found == NULL) {
        http->path = NULL;
    } else {
        *found = 0;
        http->path = found + 1;
    }
    if (port != NULL) {
        http->port = atol(port);
    }
    return 1;
}

static int DWCi_Http_AddLabelValue(DWCHttp *http, NasFieldTable *table, const char *name, const char *value) {
    DWCHttpAlloc alloc = http->param.alloc;
    DWCHttpFree free = http->param.free;

    if (table->count > table->capacity) {
        return 0;
    }
    table->fields[table->count].label = alloc(0, strlen(name) + 1);
    if (table->fields[table->count].label != NULL) {
        table->fields[table->count].value = alloc(0, strlen(value) + 1);
        if (table->fields[table->count].value != NULL) {
            strcpy(table->fields[table->count].label, name);
            strcpy(table->fields[table->count].value, value);
            table->count++;
            return 1;
        }
    }
    if (table->fields[table->count].label != NULL) {
        free(0, table->fields[table->count].label, 0);
        table->fields[table->count].label = NULL;
    }
    if (table->fields[table->count].value != NULL) {
        free(0, table->fields[table->count].value, 0);
        table->fields[table->count].value = NULL;
    }
    return 0;
}

static int DWC_Http_ParseResult(DWCHttp *http, int headersOnly) {
    NasFieldTable table;
    char savedColon;
    char savedEnd;
    char *response;
    char *bodyStart;
    char *bodyEnd;
    char *space;
    char *status;
    char *line;
    char *name;
    char saved;
    char savedEquals;

    table.fields = http->fields;
    table.capacity = 32;
    table.count = 0;
    MI_CpuFill8(http->fields, 0, sizeof(http->fields));
    response = http->response.start;
    bodyStart = strstr(response, "\r\n\r\n");
    if (bodyStart == NULL) {
        return 0;
    }
    bodyEnd = bodyStart + 4 + strlen(bodyStart + 4);
    space = strstr(response, " ");
    if (space == NULL) {
        return 0;
    }
    status = space + 1;
    saved = status[3];
    status[3] = 0;
    if (DWCi_Http_AddLabelValue(http, &table, "httpresult", status) != 1) {
        status[3] = saved;
        return 0;
    }
    status[3] = saved;
    if (headersOnly == 1 || strncmp(status, "200", 3) != 0) {
        return 1;
    }
    line = strstr(status + 4, "\r\n");
    if (line == NULL) {
        return 0;
    }
    line += 2;
    while (line[0] != '\r' && line[1] != '\n') {
        char *colon;
        char *eol;
        char *value;

        colon = strstr(line, ": ");
        if (colon == NULL) {
            break;
        }
        savedColon = *colon;
        *colon = 0;
        value = colon + 2;
        eol = strstr(value, "\r\n");
        if (eol == NULL) {
            *colon = savedColon;
            break;
        }
        savedEnd = *eol;
        *eol = 0;
        if (DWCi_Http_AddLabelValue(http, &table, line, value) != 1) {
            *colon = savedColon;
            *eol = savedEnd;
            return 0;
        }
        line = value + strlen(value) + 2;
        *colon = savedColon;
        *eol = savedEnd;
    }
    name = bodyStart + 4;
    while (name < bodyEnd) {
        char *equals;
        char *amp;
        char *value;

        equals = strstr(name, "=");
        if (equals == NULL) {
            break;
        }
        savedEquals = *equals;
        *equals = 0;
        value = equals + 1;
        amp = strstr(value, "&");
        if (amp == NULL) {
            amp = strstr(value, "\r\n");
        }
        if (amp != NULL) {
            savedEnd = *amp;
            *amp = 0;
        }
        if (DWCi_Http_AddLabelValue(http, &table, name, value) != 1) {
            *equals = savedEquals;
            if (amp != NULL) {
                *amp = savedEnd;
            }
            return 0;
        }
        name = value + strlen(value) + 1;
        *equals = savedEquals;
        if (amp != NULL) {
            *amp = savedEnd;
        }
    }
    return 1;
}

static const char *DWC_Http_GetResult(DWCHttp *http, const char *name) {
    int i;

    for (i = 0; i < 32; i++) {
        if (http->fields[i].label == NULL) {
            break;
        }
        if (strcmp(name, http->fields[i].label) == 0) {
            return http->fields[i].value;
        }
    }
    return NULL;
}

static int DWC_Http_GetBase64DecodedResult(DWCHttp *http, const char *name, char *dest, int size) {
    const char *value = DWC_Http_GetResult(http, name);
    u32 length;

    if (value == NULL) {
        return 0;
    }
    length = DWC_Base64Decode(value, strlen(value), dest, size);
    if (length != -1 && length < size) {
        dest[length] = 0;
    }
    return length;
}

static int DWC_Http_GetRawResult(DWCHttp *http, const char *name, char *dest, int size) {
    const char *value = DWC_Http_GetResult(http, name);
    int length;

    if (value == NULL) {
        return 0;
    }
    length = strlen(value);
    if (length >= size) {
        return 0;
    }
    strcpy(dest, value);
    return 1;
}

static void DWCi_Http_FreeLabelValues(DWCHttp *http, DWCHttpLabelValue *fields, int count) {
    DWCHttpFree free = http->param.free;
    int i;

    for (i = 0; i < count; i++) {
        if (fields[i].label != NULL) {
            free(0, fields[i].label, 0);
            fields[i].label = NULL;
        }
        if (fields[i].value != NULL) {
            free(0, fields[i].value, 0);
            fields[i].value = NULL;
        }
    }
}

int DWC_Netcheck_Create(const DWCAcMemFuncs *init) {
    DWCHttpAlloc alloc = init->alloc;

    if (sPr.work != NULL) {
        return 4;
    }
    sPr.work = alloc(0, sizeof(NasPrWork));
    if (sPr.work == NULL) {
        return 4;
    }
    MI_CpuFill8(sPr.work, 0, sizeof(NasPrWork));
    sPr.work->result = -99999;
    sPr.work->memFuncs = *init;
    if (sPr.http != NULL) {
        return 4;
    }
    sPr.http = alloc(0, sizeof(DWCHttp));
    if (sPr.http == NULL) {
        return 4;
    }
    sSendSsid = 0;
    OS_InitMutex(&sPr.work->lock);
    DWCi_Netcheck_StartThread();
    return 0;
}

void DWC_Netcheck_Destroy(void) {
    DWCHttpFree free = sPr.work->memFuncs.free;

    if (sPr.http != NULL) {
        DWC_Http_Destroy(sPr.http);
        free(0, sPr.http, 0);
        sPr.http = NULL;
    }
    DWC_Auth_Destroy();
    if (sPr.work != NULL) {
        if (sPr.work->redirectUrl != NULL) {
            free(0, sPr.work->redirectUrl, 0);
            sPr.work->redirectUrl = NULL;
        }
        if (sPr.work->hotspotUrl != NULL) {
            free(0, sPr.work->hotspotUrl, 0);
            sPr.work->hotspotUrl = NULL;
        }
        free(0, sPr.work, 0);
        sPr.work = NULL;
    }
}

void DWC_Netcheck_Abort(void) {
    if (sPr.work != NULL) {
        if (sPr.http != NULL) {
            DWC_Http_Abort(sPr.http);
        }
        DWC_Auth_Abort();
        if (sPr.work->thread.id != 0 && !OS_IsThreadTerminated(&sPr.work->thread)) {
            OS_JoinThread(&sPr.work->thread);
        }
        sPr.work->result = -7;
    }
}

u32 DWC_Netcheck_GetError(void) {
    int state;

    OS_LockMutex(&sPr.work->lock);
    state = sPr.work->state;
    OS_UnlockMutex(&sPr.work->lock);
    return state;
}

u32 DWC_Netcheck_GetReturnCode(void) {
    return sPr.work->result;
}

static void DWCi_Netcheck_StartThread(void) {
    if (sPr.work->thread.id == 0 || OS_IsThreadTerminated(&sPr.work->thread)) {
        OS_CreateThread(&sPr.work->thread, DWCi_Netcheck_Thread, sPr.work, sPr.work->stack + 0x1000, 0x1000, 16);
        OS_WakeupThreadDirect(&sPr.work->thread);
    }
}

// The address of the connection test: a number, then the URL
static struct {
    int unk_00;
    const char *url;
} sConnTest = { 1, "http://conntest.nintendowifi.net/" };

static void DWCi_Netcheck_Thread(void *arg) {
    DWCHttpAlloc alloc = sPr.work->memFuncs.alloc;
    DWCHttpFree free = sPr.work->memFuncs.free;
    char *url = NULL;
    char *data = NULL;
    char *waitText = NULL;
    int waitMs;
    int code;
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
        sPrConfig.url = sConnTest.url;
        sPrConfig.get = 1;
        sPrConfig.bufferSize = 0x1000;
        sPrConfig.alloc = alloc;
        sPrConfig.free = free;
        sPrConfig.timeout = 20000;
        sPr.work->result = -2;
        if (DWC_Http_Create(sPr.http, &sPrConfig) != 0 || DWC_Http_FinishHeader(sPr.http) != 0) {
            break;
        }
        DWC_Http_StartThread(sPr.http, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
        if (sPr.http->thread.id != 0 && !OS_IsThreadTerminated(&sPr.http->thread)) {
            OS_JoinThread(&sPr.http->thread);
        }
        if (sPr.http->result == 2) {
            sPr.work->result = -1;
            goto closed;
        }
        if (sPr.http->result != 8) {
            goto closed;
        }
        if (DWC_Http_ParseResult(sPr.http, 0) != 1) {
            goto badResponse;
        }
        status = atol(DWC_Http_GetResult(sPr.http, "httpresult"));
        if (errno == ERANGE) {
            goto reportBadResponse;
        }
        if (status == 200) {
            sPr.ip = sPr.http->ip;
        } else if (status != 302) {
            DWC_Http_Destroy(sPr.http);
            code = 10;
            goto done;
        } else {
            sSendSsid = 1;
            if (sPr.work->hotspotUrl == NULL) {
                location = sPr.http->response.start;
                if (location == NULL) {
                    goto badResponse;
                }
                sPr.work->redirectUrl = alloc(0, strlen(location) + 1);
                if (sPr.work->redirectUrl == NULL) {
                    goto noMemory;
                }
                strncpy(sPr.work->redirectUrl, location, strlen(location) + 1);
            } else {
                sPr.work->result = -6;
                DWC_Http_Destroy(sPr.http);
                sPrConfig.url = sConfig.url;
                sPrConfig.get = 0;
                sPrConfig.bufferSize = 0x200;
                sPrConfig.alloc = alloc;
                sPrConfig.free = free;
                sPrConfig.timeout = 20000;
                if (strcmp(sConfig.url, "https://nas.nintendowifi.net/ac") != 0) {
                    sPrConfig.ignoreCa = 1;
                }
                if (DWC_Http_Create(sPr.http, &sPrConfig) != 0) {
                    goto noRequest;
                }
                if (!DWCi_Auth_GetCurrentInfo(&info) || !DWCi_Auth_AddCommonItems(sPr.http, &info, 1)) {
                    DWC_Http_Destroy(sPr.http);
                    code = 8;
                    goto done;
                }
                if (DWC_Http_Add_PostBase64Item(sPr.http, "action", "message", 7) != 0) {
                    goto badRequest;
                }
                if (DWC_Http_Add_PostBase64Item(sPr.http, "HotSpotResponse", sPr.work->hotspotUrl,
                                                strlen(sPr.work->hotspotUrl)) != 0) {
                    goto badRequest;
                }
                free(0, sPr.work->hotspotUrl, 0);
                sPr.work->hotspotUrl = NULL;
                if (DWC_Http_FinishHeader(sPr.http) != 0) {
                    goto noRequest;
                }
                DWC_Http_StartThread(sPr.http, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
                if (sPr.http->thread.id != 0 && !OS_IsThreadTerminated(&sPr.http->thread)) {
                    OS_JoinThread(&sPr.http->thread);
                }
                if (sPr.http->result == 2) {
                    sPr.work->result = -1;
                } else if (sPr.http->result == 8) {
                    DWC_Http_Destroy(sPr.http);
                    code = 7;
                    goto done;
                }
                DWC_Http_Destroy(sPr.http);
                goto closedCode;
            }
        }
        DWC_Http_Destroy(sPr.http);
        DWCBm_GetWifiIdInfo(&userInfo);
        if (userInfo.id == 0) {
            sPr.work->result = -3;
            sPrInit.ingamesn[0] = 0;
            sPrInit.gsbrcd[0] = 0;
            sPrInit.alloc = sPr.work->memFuncs.alloc;
            sPrInit.free = sPr.work->memFuncs.free;
            if (DWC_Auth_Create(&sPrInit, sPr.http) != 0) {
                code = 5;
                goto done;
            }
            DWC_Auth_Join();
            if (DWC_Auth_GetError() != 0x15) {
                if (DWC_Auth_GetError() == 9) {
                    nasStatus.returnCode = -1;
                } else {
                    DWC_Auth_GetResult(&nasStatus);
                    if (sPr.allow302 == 1 && (nasStatus.returnCode == -23302 || DWC_Auth_GetError() == 0xb)) {
                        sPr.work->result = 0;
                        DWC_Auth_Destroy();
                        code = 0xb;
                        goto done;
                    }
                }
                sPr.work->result = nasStatus.returnCode;
                DWC_Auth_Destroy();
                code = 6;
                goto done;
            }
            DWC_Auth_Destroy();
        }
        if (status == 200) {
            goto finished;
        }
        sPr.work->result = -4;
        sPrConfig.url = sConfig.url;
        sPrConfig.get = 0;
        sPrConfig.bufferSize = 0x1000;
        sPrConfig.alloc = alloc;
        sPrConfig.free = free;
        sPrConfig.timeout = 40000;
        if (strcmp(sConfig.url, "https://nas.nintendowifi.net/ac") != 0) {
            sPrConfig.ignoreCa = 1;
        }
        if (DWC_Http_Create(sPr.http, &sPrConfig) != 0) {
            break;
        }
        if (!DWCi_Auth_GetCurrentInfo(&info) || !DWCi_Auth_AddCommonItems(sPr.http, &info, 1) ||
            DWC_Http_Add_PostBase64Item(sPr.http, "action", "parse", 5) != 0) {
            goto badRequest;
        }
        if (DWC_Http_Add_PostBase64Item(sPr.http, "HTML", sPr.work->redirectUrl, strlen(sPr.work->redirectUrl)) != 0) {
            goto badRequest;
        }
        free(0, sPr.work->redirectUrl, 0);
        sPr.work->redirectUrl = NULL;
        if (DWC_Http_FinishHeader(sPr.http) != 0) {
            break;
        }
        DWC_Http_StartThread(sPr.http, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
        if (sPr.http->thread.id != 0 && !OS_IsThreadTerminated(&sPr.http->thread)) {
            OS_JoinThread(&sPr.http->thread);
        }
        status = sPr.http->result;
        if (status == 2) {
            sPr.work->result = -1;
            goto closed;
        }
        if (status == 3) {
            DWC_Http_Destroy(sPr.http);
            if (sPr.allow302 != 1) {
                goto closedCode;
            }
            goto finished;
        }
        if (status != 8) {
            goto closed;
        }
        if (DWC_Http_ParseResult(sPr.http, 0) != 1) {
            goto badResponse;
        }
        status = atol(DWC_Http_GetResult(sPr.http, "httpresult"));
        if (errno == ERANGE) {
            goto badResponse;
        }
        if (status != 200) {
            DWC_Http_Destroy(sPr.http);
            if (sPr.allow302 != 1 || status != 302) {
                goto reportBadResponse;
            }
            goto finished;
        }
        if (DWC_Http_GetBase64DecodedResult(sPr.http, "returncd", returnText, 4) <= 0) {
            goto badAnswer;
        }
        returnCode = atol(returnText);
        if (errno == ERANGE) {
            goto badAnswer;
        }
        if (sPr.allow302 == 1 && returnCode == 0x72) {
            goto finished;
        }
        if (returnCode >= 100) {
            DWC_Http_Destroy(sPr.http);
            code = 6;
            goto done;
        }
        length = DWC_Http_GetBase64DecodedResult(sPr.http, "url", NULL, 0);
        if (length <= 0) {
            goto badAnswer;
        }
        dataLength = DWC_Http_GetBase64DecodedResult(sPr.http, "data", NULL, 0);
        if (dataLength <= 0) {
            goto badAnswer;
        }
        waitLength = DWC_Http_GetBase64DecodedResult(sPr.http, "wait", NULL, 0);
        url = alloc(0, length + 1);
        if (url == NULL) {
            goto noMemory;
        }
        data = alloc(0, dataLength + 1);
        if (data == NULL) {
            goto noMemory;
        }
        if (waitLength > 0) {
            waitText = alloc(0, waitLength + 1);
            if (waitText == NULL) {
                goto noMemory;
            }
        }
        length = DWC_Http_GetBase64DecodedResult(sPr.http, "url", url, length + 1);
        if (length < 0) {
            goto badAnswer;
        }
        url[length] = 0;
        length = DWC_Http_GetBase64DecodedResult(sPr.http, "data", data, dataLength + 1);
        if (length < 0) {
            goto badAnswer;
        }
        data[length] = 0;
        waitMs = 0;
        if (waitLength > 0) {
            length = DWC_Http_GetBase64DecodedResult(sPr.http, "wait", waitText, waitLength + 1);
            if (length < 0) {
                goto badAnswer;
            }
            waitText[length] = 0;
            waitMs = atol(waitText);
            if (errno == ERANGE) {
                goto badAnswer;
            }
            waitMs *= 1000;
            if (waitMs > 180000) {
                waitMs = 180000;
            }
        }
        DWC_Http_Destroy(sPr.http);
        sPr.work->result = -5;
        sPrConfig.url = url;
        sPrConfig.get = 0;
        sPrConfig.bufferSize = 0x1000;
        sPrConfig.alloc = alloc;
        sPrConfig.free = free;
        sPrConfig.timeout = 120000;
        if (DWC_Http_Create(sPr.http, &sPrConfig) != 0) {
            break;
        }
        if (DWC_Http_Add_Body(sPr.http, data) != 0) {
            goto badRequest;
        }
        if (DWC_Http_FinishHeader(sPr.http) != 0) {
            break;
        }
        DWC_Http_StartThread(sPr.http, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
        if (sPr.http->thread.id != 0 && !OS_IsThreadTerminated(&sPr.http->thread)) {
            OS_JoinThread(&sPr.http->thread);
        }
        if (sPr.http->result == 2) {
            sPr.work->result = -1;
            goto closed;
        }
        if (sPr.http->result != 8) {
            goto closed;
        }
        if (DWC_Http_ParseResult(sPr.http, 1) != 1) {
            goto badResponse;
        }
        location = sPr.http->response.start;
        if (location == NULL) {
            goto badResponse;
        }
        sPr.work->hotspotUrl = alloc(0, strlen(location) + 1);
        if (sPr.work->hotspotUrl == NULL) {
            goto noMemory;
        }
        strncpy(sPr.work->hotspotUrl, location, strlen(location) + 1);
        DWC_Http_Destroy(sPr.http);
        OS_Sleep(waitMs);
    }
noRequest:
    DWC_Http_Destroy(sPr.http);
    code = 1;
    goto done;
closed:
    DWC_Http_Destroy(sPr.http);
closedCode:
    code = 3;
    goto done;
badResponse:
    DWC_Http_Destroy(sPr.http);
reportBadResponse:
    code = 2;
    goto done;
noMemory:
    DWC_Http_Destroy(sPr.http);
    code = 4;
    goto done;
badRequest:
    DWC_Http_Destroy(sPr.http);
    code = 8;
    goto done;
badAnswer:
    DWC_Http_Destroy(sPr.http);
    code = 9;
    goto done;
finished:
    sPr.work->result = 0;
    code = 0xb;
done:
    DWCi_Netcheck_SetError(code);
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
    OS_LockMutex(&sPr.work->lock);
    sPr.work->state = state;
    OS_UnlockMutex(&sPr.work->lock);
}

static BOOL DWCi_Prof_StartRequest(const u16 **words, int count, const char *type, int timeout, char *flags,
                                   int *profaneCount, const char *region) {
    u16 *joined = NULL;
    int total = 0;
    int limit;
    DWCHttpParam config;
    NasAuthInfo info;
    int i;
    int position;
    int next;
    char *found;

    if (words == NULL || count <= 0 || count > 50 || flags == NULL || profaneCount == NULL) {
        return FALSE;
    }
    for (i = 0; i < count; i++) {
        total += wcslen(words[i]) + 1;
    }
    if (total - 1 <= 0 || total - 1 > 500) {
        return FALSE;
    }
    if (sWords != NULL) {
        return FALSE;
    }
    MI_CpuFill8(flags, 0, count);
    sWords = DWC_Alloc(0, sizeof(NasWordWork));
    if (sWords != NULL) {
        MI_CpuFill8(sWords, 0, sizeof(NasWordWork));
        sWords->active = 0;
        MI_CpuFill8(&info, 0, sizeof(NasAuthInfo));
        sWords->flags = flags;
        sWords->profaneCount = profaneCount;
        sWords->count = count;
        strncpy(sWords->url, sConfig.url, 0x100);
        sWords->url[0x100] = 0;
        found = strstr(sWords->url, "/ac");
        if (found != NULL) {
            MI_CpuCopy8("/pr", found, 4);
            config.url = sWords->url;
            config.get = 0;
            config.bufferSize = 0x400;
            config.alloc = DWCi_Prof_Alloc;
            config.free = DWCi_Prof_Free;
            config.ignoreCa = 0;
            config.timeout = timeout > 0 ? timeout : 10000;
            if (DWC_Http_Create(&sWords->http, &config) == 0) {
                sWords->active = 1;
                if (DWCi_Auth_GetCurrentInfo(&info) && DWCi_Auth_AddCommonItems(&sWords->http, &info, sSendSsid) &&
                    DWC_Http_Add_PostBase64Item(&sWords->http, "wregion", region, 1) == 0 &&
                    DWC_Http_Add_PostBase64Item(&sWords->http, "wenc", "UTF-16LE", 8) == 0 &&
                    (type == NULL || DWC_Http_Add_PostBase64Item(&sWords->http, "wtype", type, 4) == 0) &&
                    DWC_Http_Add_Body(&sWords->http, "&wtype=") == 0) {
                    joined = DWC_Alloc(0, total * 2);
                    if (joined != NULL) {
                        position = 0;
                        for (i = 0; i < count; i++) {
                            wcscpy(joined + position, words[i]);
                            next = position + wcslen(words[i]);
                            if (next >= total - 1) {
                                break;
                            }
                            joined[next] = 9;
                            position = next + 1;
                        }
                        if (DWC_Http_Add_PostBase64Item(&sWords->http, "words", joined, (total - 1) * 2) == 0) {
                            DWC_Free(0, joined, 0);
                            joined = NULL;
                            if (DWC_Http_FinishHeader(&sWords->http) == 0) {
                                DWC_Http_StartThread(&sWords->http, OS_GetThreadPriority(OSi_ThreadInfo.current) - 1);
                                if (sWords->http.thread.id != 0) {
                                    return TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (joined != NULL) {
        DWC_Free(0, joined, 0);
    }
    if (sWords != NULL) {
        if (sWords->active == 1) {
            DWC_Http_Destroy(&sWords->http);
        }
        DWC_Free(0, sWords, 0);
        sWords = NULL;
    }
    return FALSE;
}

BOOL DWC_CheckProfanityExAsync(const u16 **words, int count, const char *type, int timeout, char *flags,
                               int *profaneCount, int mode) {
    char region;

    if (mode == 0x80) {
        region = 'A';
    } else {
        region = ROM_REGION;
    }
    return DWCi_Prof_StartRequest(words, count, type, timeout, flags, profaneCount, &region);
}

int DWC_CheckProfanityProcess(void) {
    int result;
    u32 code;

    if (sWords == NULL || sWords->http.thread.id == 0) {
        return 0;
    }
    if (OS_IsThreadTerminated(&sWords->http.thread) == TRUE) {
        code = DWCAc_GetApType();
        if (code == 0xff || code > 99) {
            code = 99;
        }
        switch (sWords->http.result) {
        case DWC_HTTP_DONE:
            if (!DWCi_Prof_ParseResult()) {
                result = 3;
            } else {
                result = 2;
            }
            break;
        case DWC_HTTP_DNS_ERROR:
            DWCi_SetError(8, -52100 - code);
            result = 3;
            break;
        case DWC_HTTP_CONNECT_ERROR:
        case 4:
        case DWC_HTTP_TIMEOUT:
            DWCi_SetError(0x12, -33010);
            result = 3;
            break;
        default:
            DWCi_SetError(0x12, -33030);
            result = 3;
            break;
        }
    } else {
        return 1;
    }
    DWC_Http_Destroy(&sWords->http);
    DWC_Free(0, sWords, 0);
    sWords = NULL;
    return result;
}

static BOOL DWCi_Prof_ParseResult(void) {
    const char *names[8] = { "prwordsJ", "prwordsE", "prwordsP", "prwordsK", "prwordsC" };
    int gotAny = 0;
    u32 i;
    DWCHttp *http = &sWords->http;
    const char *text;
    char returnText[4];
    char flagText[0x33];
    int length;
    int returnCode;
    int status;
    int j;

    if (DWC_Http_ParseResult(http, 0) != 0) {
        text = DWC_Http_GetResult(http, "Date");
        if (text != NULL) {
            DWCi_SetNasTimeFromDateHeader(text);
        }
        text = DWC_Http_GetResult(http, "httpresult");
        if (text != NULL) {
            status = atol(text);
            if (errno == ERANGE || status != 200) {
                goto fail;
            }
            length = DWC_Http_GetBase64DecodedResult(http, "returncd", returnText, 4);
            if (length > 0) {
                returnCode = atol(returnText);
                if (errno != 0x22) {
                    if (returnCode == 1 || (returnCode >= 100 && returnCode != 100)) {
                        DWCi_SetError(0x11, -33000 - returnCode);
                        return FALSE;
                    }
                    for (i = 0; i < 7; i++) {
                        if (names[i] != NULL) {
                            length = DWC_Http_GetBase64DecodedResult(http, names[i], flagText, 0x33);
                            if (length < 0) {
                                goto fail;
                            }
                            if (length != 0) {
                                gotAny = 1;
                                if (length < sWords->count) {
                                    goto fail;
                                }
                                for (j = 0; j < sWords->count; j++) {
                                    if (flagText[j] == '1') {
                                        sWords->flags[j] |= (char)(1 << i);
                                    }
                                }
                            }
                        }
                    }
                    length = DWC_Http_GetBase64DecodedResult(http, "prwords", flagText, 0x33);
                    if (length < 0 || length == 0 || length < sWords->count) {
                        goto fail;
                    }
                    *sWords->profaneCount = 0;
                    for (j = 0; j < sWords->count; j++) {
                        if (flagText[j] == '1') {
                            (*sWords->profaneCount)++;
                            if (gotAny != 0) {
                                sWords->flags[j] |= 0x80;
                            } else {
                                sWords->flags[j] = 1;
                            }
                        }
                    }
                    return TRUE;
                }
            }
        }
    }
fail:
    DWCi_SetError(0x11, -33020);
    return FALSE;
}

static void *DWCi_Prof_Alloc(u32 name, u32 size) {
    return DWC_Alloc(0, size);
}

static void DWCi_Prof_Free(u32 name, void *ptr, u32 size) {
    DWC_Free(0, ptr, size);
}

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
    config.url = sConfig.url;
    config.get = 0;
    config.bufferSize = 0x1000;
    config.alloc = locator->alloc;
    config.free = locator->free;
    if (strcmp(sConfig.url, "https://nas.nintendowifi.net/ac") != 0) {
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

static u8 sNasCert0Exponent[3] = { 1, 0, 1 };
static char sNasCert0Name[53] = "IE, Baltimore, CyberTrust, Baltimore CyberTrust Root";
static NasRootCert sNasCert0 = { sNasCert0Name, 256, sNasCert0Key, 3, sNasCert0Exponent };
static u8 sNasCert0Key[256] = {
    0xa3, 0x04, 0xbb, 0x22, 0xab, 0x98, 0x3d, 0x57, 0xe8, 0x26, 0x72, 0x9a, 0xb5, 0x79, 0xd4, 0x29, 0xe2, 0xe1, 0xe8,
    0x95, 0x80, 0xb1, 0xb0, 0xe3, 0x5b, 0x8e, 0x2b, 0x29, 0x9a, 0x64, 0xdf, 0xa1, 0x5d, 0xed, 0xb0, 0x09, 0x05, 0x6d,
    0xdb, 0x28, 0x2e, 0xce, 0x62, 0xa2, 0x62, 0xfe, 0xb4, 0x88, 0xda, 0x12, 0xeb, 0x38, 0xeb, 0x21, 0x9d, 0xc0, 0x41,
    0x2b, 0x01, 0x52, 0x7b, 0x88, 0x77, 0xd3, 0x1c, 0x8f, 0xc7, 0xba, 0xb9, 0x88, 0xb5, 0x6a, 0x09, 0xe7, 0x73, 0xe8,
    0x11, 0x40, 0xa7, 0xd1, 0xcc, 0xca, 0x62, 0x8d, 0x2d, 0xe5, 0x8f, 0x0b, 0xa6, 0x50, 0xd2, 0xa8, 0x50, 0xc3, 0x28,
    0xea, 0xf5, 0xab, 0x25, 0x87, 0x8a, 0x9a, 0x96, 0x1c, 0xa9, 0x67, 0xb8, 0x3f, 0x0c, 0xd5, 0xf7, 0xf9, 0x52, 0x13,
    0x2f, 0xc2, 0x1b, 0xd5, 0x70, 0x70, 0xf0, 0x8f, 0xc0, 0x12, 0xca, 0x06, 0xcb, 0x9a, 0xe1, 0xd9, 0xca, 0x33, 0x7a,
    0x77, 0xd6, 0xf8, 0xec, 0xb9, 0xf1, 0x68, 0x44, 0x42, 0x48, 0x13, 0xd2, 0xc0, 0xc2, 0xa4, 0xae, 0x5e, 0x60, 0xfe,
    0xb6, 0xa6, 0x05, 0xfc, 0xb4, 0xdd, 0x07, 0x59, 0x02, 0xd4, 0x59, 0x18, 0x98, 0x63, 0xf5, 0xa5, 0x63, 0xe0, 0x90,
    0x0c, 0x7d, 0x5d, 0xb2, 0x06, 0x7a, 0xf3, 0x85, 0xea, 0xeb, 0xd4, 0x03, 0xae, 0x5e, 0x84, 0x3e, 0x5f, 0xff, 0x15,
    0xed, 0x69, 0xbc, 0xf9, 0x39, 0x36, 0x72, 0x75, 0xcf, 0x77, 0x52, 0x4d, 0xf3, 0xc9, 0x90, 0x2c, 0xb9, 0x3d, 0xe5,
    0xc9, 0x23, 0x53, 0x3f, 0x1f, 0x24, 0x98, 0x21, 0x5c, 0x07, 0x99, 0x29, 0xbd, 0xc6, 0x3a, 0xec, 0xe7, 0x6e, 0x86,
    0x3a, 0x6b, 0x97, 0x74, 0x63, 0x33, 0xbd, 0x68, 0x18, 0x31, 0xf0, 0x78, 0x8d, 0x76, 0xbf, 0xfc, 0x9e, 0x8e, 0x5d,
    0x2a, 0x86, 0xa7, 0x4d, 0x90, 0xdc, 0x27, 0x1a, 0x39,
};
static u8 sNasCert1Exponent[3] = { 1, 0, 1 };
static char sNasCert1Name[50] = "BE, GlobalSign nv-sa, Root CA, GlobalSign Root CA";
static NasRootCert sNasCert1 = { sNasCert1Name, 256, sNasCert1Key, 3, sNasCert1Exponent };
static u8 sNasCert1Key[256] = {
    0xda, 0x0e, 0xe6, 0x99, 0x8d, 0xce, 0xa3, 0xe3, 0x4f, 0x8a, 0x7e, 0xfb, 0xf1, 0x8b, 0x83, 0x25, 0x6b, 0xea, 0x48,
    0x1f, 0xf1, 0x2a, 0xb0, 0xb9, 0x95, 0x11, 0x04, 0xbd, 0xf0, 0x63, 0xd1, 0xe2, 0x67, 0x66, 0xcf, 0x1c, 0xdd, 0xcf,
    0x1b, 0x48, 0x2b, 0xee, 0x8d, 0x89, 0x8e, 0x9a, 0xaf, 0x29, 0x80, 0x65, 0xab, 0xe9, 0xc7, 0x2d, 0x12, 0xcb, 0xab,
    0x1c, 0x4c, 0x70, 0x07, 0xa1, 0x3d, 0x0a, 0x30, 0xcd, 0x15, 0x8d, 0x4f, 0xf8, 0xdd, 0xd4, 0x8c, 0x50, 0x15, 0x1c,
    0xef, 0x50, 0xee, 0xc4, 0x2e, 0xf7, 0xfc, 0xe9, 0x52, 0xf2, 0x91, 0x7d, 0xe0, 0x6d, 0xd5, 0x35, 0x30, 0x8e, 0x5e,
    0x43, 0x73, 0xf2, 0x41, 0xe9, 0xd5, 0x6a, 0xe3, 0xb2, 0x89, 0x3a, 0x56, 0x39, 0x38, 0x6f, 0x06, 0x3c, 0x88, 0x69,
    0x5b, 0x2a, 0x4d, 0xc5, 0xa7, 0x54, 0xb8, 0x6c, 0x89, 0xcc, 0x9b, 0xf9, 0x3c, 0xca, 0xe5, 0xfd, 0x89, 0xf5, 0x12,
    0x3c, 0x92, 0x78, 0x96, 0xd6, 0xdc, 0x74, 0x6e, 0x93, 0x44, 0x61, 0xd1, 0x8d, 0xc7, 0x46, 0xb2, 0x75, 0x0e, 0x86,
    0xe8, 0x19, 0x8a, 0xd5, 0x6d, 0x6c, 0xd5, 0x78, 0x16, 0x95, 0xa2, 0xe9, 0xc8, 0x0a, 0x38, 0xeb, 0xf2, 0x24, 0x13,
    0x4f, 0x73, 0x54, 0x93, 0x13, 0x85, 0x3a, 0x1b, 0xbc, 0x1e, 0x34, 0xb5, 0x8b, 0x05, 0x8c, 0xb9, 0x77, 0x8b, 0xb1,
    0xdb, 0x1f, 0x20, 0x91, 0xab, 0x09, 0x53, 0x6e, 0x90, 0xce, 0x7b, 0x37, 0x74, 0xb9, 0x70, 0x47, 0x91, 0x22, 0x51,
    0x63, 0x16, 0x79, 0xae, 0xb1, 0xae, 0x41, 0x26, 0x08, 0xc8, 0x19, 0x2b, 0xd1, 0x46, 0xaa, 0x48, 0xd6, 0x64, 0x2a,
    0xd7, 0x83, 0x34, 0xff, 0x2c, 0x2a, 0xc1, 0x6c, 0x19, 0x43, 0x4a, 0x07, 0x85, 0xe7, 0xd3, 0x7c, 0xf6, 0x21, 0x68,
    0xef, 0xea, 0xf2, 0x52, 0x9f, 0x7f, 0x93, 0x90, 0xcf,
};
static u8 sNasCert2Exponent[3] = { 1, 0, 1 };
static NasRootCert sNasCert2 = { sNasCert2Name, 128, sNasCert2Key, 3, sNasCert2Exponent };
static char sNasCert2Name[80] = "US, GTE Corporation, GTE CyberTrust Solutions, Inc., GTE CyberTrust Global Root";
static u8 sNasCert2Key[128] = {
    0x95, 0x0f, 0xa0, 0xb6, 0xf0, 0x50, 0x9c, 0xe8, 0x7a, 0xc7, 0x88, 0xcd, 0xdd, 0x17, 0x0e, 0x2e, 0xb0, 0x94, 0xd0,
    0x1b, 0x3d, 0x0e, 0xf6, 0x94, 0xc0, 0x8a, 0x94, 0xc7, 0x06, 0xc8, 0x90, 0x97, 0xc8, 0xb8, 0x64, 0x1a, 0x7a, 0x7e,
    0x6c, 0x3c, 0x53, 0xe1, 0x37, 0x28, 0x73, 0x60, 0x7f, 0xb2, 0x97, 0x53, 0x07, 0x9f, 0x53, 0xf9, 0x6d, 0x58, 0x94,
    0xd2, 0xaf, 0x8d, 0x6d, 0x88, 0x67, 0x80, 0xe6, 0xed, 0xb2, 0x95, 0xcf, 0x72, 0x31, 0xca, 0xa5, 0x1c, 0x72, 0xba,
    0x5c, 0x02, 0xe7, 0x64, 0x42, 0xe7, 0xf9, 0xa9, 0x2c, 0xd6, 0x3a, 0x0d, 0xac, 0x8d, 0x42, 0xaa, 0x24, 0x01, 0x39,
    0xe6, 0x9c, 0x3f, 0x01, 0x85, 0x57, 0x0d, 0x58, 0x87, 0x45, 0xf8, 0xd3, 0x85, 0xaa, 0x93, 0x69, 0x26, 0x85, 0x70,
    0x48, 0x80, 0x3f, 0x12, 0x15, 0xc7, 0x79, 0xb4, 0x1f, 0x05, 0x2f, 0x3b, 0x62, 0x99,
};
static u8 sNasCert3Exponent[3] = { 1, 0, 1 };
static char sNasCert3Name[41] = "US, GTE Corporation, GTE CyberTrust Root";
static NasRootCert sNasCert3 = { sNasCert3Name, 128, sNasCert3Key, 3, sNasCert3Exponent };
static u8 sNasCert3Key[128] = {
    0xb8, 0xe6, 0x4f, 0xba, 0xdb, 0x98, 0x7c, 0x71, 0x7c, 0xaf, 0x44, 0xb7, 0xd3, 0x0f, 0x46, 0xd9, 0x64, 0xe5, 0x93,
    0xc1, 0x42, 0x8e, 0xc7, 0xba, 0x49, 0x8d, 0x35, 0x2d, 0x7a, 0xe7, 0x8b, 0xbd, 0xe5, 0x05, 0x31, 0x59, 0xc6, 0xb1,
    0x2f, 0x0a, 0x0c, 0xfb, 0x9f, 0xa7, 0x3f, 0xa2, 0x09, 0x66, 0x84, 0x56, 0x1e, 0x37, 0x29, 0x1b, 0x87, 0xe9, 0x7e,
    0x0c, 0xca, 0x9a, 0x9f, 0xa5, 0x7f, 0xf5, 0x15, 0x94, 0xa3, 0xd5, 0xa2, 0x46, 0x82, 0xd8, 0x68, 0x4c, 0xd1, 0x37,
    0x15, 0x06, 0x68, 0xaf, 0xbd, 0xf8, 0xb0, 0xb3, 0xf0, 0x29, 0xf5, 0x95, 0x5a, 0x09, 0x16, 0x61, 0x77, 0x0a, 0x22,
    0x25, 0xd4, 0x4f, 0x45, 0xaa, 0xc7, 0xbd, 0xe5, 0x96, 0xdf, 0xf9, 0xd4, 0xa8, 0x8e, 0x42, 0xcc, 0x24, 0xc0, 0x1e,
    0x91, 0x27, 0x4a, 0xb5, 0x6d, 0x06, 0x80, 0x63, 0x39, 0xc4, 0xa2, 0x5e, 0x38, 0x03,
};
static u8 sNasCert4Exponent[3] = { 1, 0, 1 };
static char sNasCert4Name[48] = "GlobalSign Root CA - R2, GlobalSign, GlobalSign";
static NasRootCert sNasCert4 = { sNasCert4Name, 256, sNasCert4Key, 3, sNasCert4Exponent };
static u8 sNasCert4Key[256] = {
    0xa6, 0xcf, 0x24, 0x0e, 0xbe, 0x2e, 0x6f, 0x28, 0x99, 0x45, 0x42, 0xc4, 0xab, 0x3e, 0x21, 0x54, 0x9b, 0x0b, 0xd3,
    0x7f, 0x84, 0x70, 0xfa, 0x12, 0xb3, 0xcb, 0xbf, 0x87, 0x5f, 0xc6, 0x7f, 0x86, 0xd3, 0xb2, 0x30, 0x5c, 0xd6, 0xfd,
    0xad, 0xf1, 0x7b, 0xdc, 0xe5, 0xf8, 0x60, 0x96, 0x09, 0x92, 0x10, 0xf5, 0xd0, 0x53, 0xde, 0xfb, 0x7b, 0x7e, 0x73,
    0x88, 0xac, 0x52, 0x88, 0x7b, 0x4a, 0xa6, 0xca, 0x49, 0xa6, 0x5e, 0xa8, 0xa7, 0x8c, 0x5a, 0x11, 0xbc, 0x7a, 0x82,
    0xeb, 0xbe, 0x8c, 0xe9, 0xb3, 0xac, 0x96, 0x25, 0x07, 0x97, 0x4a, 0x99, 0x2a, 0x07, 0x2f, 0xb4, 0x1e, 0x77, 0xbf,
    0x8a, 0x0f, 0xb5, 0x02, 0x7c, 0x1b, 0x96, 0xb8, 0xc5, 0xb9, 0x3a, 0x2c, 0xbc, 0xd6, 0x12, 0xb9, 0xeb, 0x59, 0x7d,
    0xe2, 0xd0, 0x06, 0x86, 0x5f, 0x5e, 0x49, 0x6a, 0xb5, 0x39, 0x5e, 0x88, 0x34, 0xec, 0xbc, 0x78, 0x0c, 0x08, 0x98,
    0x84, 0x6c, 0xa8, 0xcd, 0x4b, 0xb4, 0xa0, 0x7d, 0x0c, 0x79, 0x4d, 0xf0, 0xb8, 0x2d, 0xcb, 0x21, 0xca, 0xd5, 0x6c,
    0x5b, 0x7d, 0xe1, 0xa0, 0x29, 0x84, 0xa1, 0xf9, 0xd3, 0x94, 0x49, 0xcb, 0x24, 0x62, 0x91, 0x20, 0xbc, 0xdd, 0x0b,
    0xd5, 0xd9, 0xcc, 0xf9, 0xea, 0x27, 0x0a, 0x2b, 0x73, 0x91, 0xc6, 0x9d, 0x1b, 0xac, 0xc8, 0xcb, 0xe8, 0xe0, 0xa0,
    0xf4, 0x2f, 0x90, 0x8b, 0x4d, 0xfb, 0xb0, 0x36, 0x1b, 0xf6, 0x19, 0x7a, 0x85, 0xe0, 0x6d, 0xf2, 0x61, 0x13, 0x88,
    0x5c, 0x9f, 0xe0, 0x93, 0x0a, 0x51, 0x97, 0x8a, 0x5a, 0xce, 0xaf, 0xab, 0xd5, 0xf7, 0xaa, 0x09, 0xaa, 0x60, 0xbd,
    0xdc, 0xd9, 0x5f, 0xdf, 0x72, 0xa9, 0x60, 0x13, 0x5e, 0x00, 0x01, 0xc9, 0x4a, 0xfa, 0x3f, 0xa4, 0xea, 0x07, 0x03,
    0x21, 0x02, 0x8e, 0x82, 0xca, 0x03, 0xc2, 0x9b, 0x8f,
};
static u8 sNasCert5Exponent[3] = { 1, 0, 1 };
static NasRootCert sNasCert5 = { sNasCert5Name, 128, sNasCert5Key, 3, sNasCert5Exponent };
static char sNasCert5Name[79] = "US, Washington, Nintendo of America Inc, NOA, Nintendo CA, ca@noa.nintendo.com";
static u8 sNasCert5Key[128] = {
    0xb3, 0xcd, 0x79, 0x97, 0x77, 0x5d, 0x8a, 0xaf, 0x86, 0xa8, 0xe8, 0xd7, 0x73, 0x1c, 0x77, 0xdf, 0x10, 0x90, 0x1f,
    0x81, 0xf8, 0x41, 0x9e, 0x21, 0x55, 0xdf, 0xbc, 0xfc, 0x63, 0xfb, 0x19, 0x43, 0xf1, 0xf6, 0xc4, 0x72, 0x42, 0x49,
    0xbd, 0xad, 0x44, 0x68, 0x4e, 0xf3, 0xda, 0x1d, 0xe6, 0x4d, 0xd8, 0xf9, 0x59, 0x88, 0xdc, 0xae, 0x3e, 0x9b, 0x38,
    0x09, 0xca, 0x7f, 0xff, 0xdc, 0x24, 0xa2, 0x44, 0x78, 0x78, 0x49, 0x93, 0xd4, 0x84, 0x40, 0x10, 0xb8, 0xec, 0x3e,
    0xdb, 0x2d, 0x93, 0xc8, 0x11, 0xc8, 0xfd, 0x78, 0x2d, 0x61, 0xad, 0x31, 0xae, 0x86, 0x26, 0xb0, 0xfd, 0x5a, 0x3f,
    0xa1, 0x3d, 0xbf, 0xe2, 0x4b, 0x49, 0xec, 0xce, 0x66, 0x98, 0x58, 0x26, 0x12, 0xc0, 0xfb, 0xf4, 0x77, 0x65, 0x1b,
    0xea, 0xfb, 0xcb, 0x7f, 0xe0, 0x8c, 0xcb, 0x02, 0xa3, 0x4e, 0x5e, 0x8c, 0xea, 0x9b,
};
static u8 sNasCert6Exponent[3] = { 1, 0, 1 };
static NasRootCert sNasCert6 = { sNasCert6Name, 128, sNasCert6Key, 3, sNasCert6Exponent };
static char sNasCert6Name[88] =
    "US, Washington, Nintendo of America Inc., NOA, Nintendo Class 2 CA, ca@noa.nintendo.com";
static u8 sNasCert6Key[128] = {
    0xb4, 0xa8, 0x0f, 0xe3, 0x80, 0x02, 0xa7, 0xad, 0xfb, 0x59, 0x9d, 0xe1, 0x92, 0x9d, 0x19, 0x7b, 0x62, 0x00, 0x96,
    0xce, 0x13, 0x2e, 0x5b, 0x83, 0xc0, 0x33, 0x26, 0xa6, 0x39, 0x34, 0xac, 0x8e, 0xe7, 0x7f, 0x69, 0x7e, 0xd4, 0xc0,
    0x7c, 0xb7, 0x60, 0x86, 0xaf, 0xb6, 0x45, 0x06, 0x36, 0xe4, 0x5d, 0xea, 0xed, 0x15, 0x54, 0x36, 0x1d, 0x57, 0xb6,
    0x9e, 0x0e, 0x07, 0x43, 0x62, 0x10, 0xee, 0x35, 0xc2, 0x41, 0x34, 0x38, 0xd0, 0xdd, 0x52, 0xae, 0x86, 0xfd, 0x0c,
    0x9d, 0x29, 0xfa, 0x3e, 0xb6, 0xbb, 0xd9, 0x51, 0x38, 0x26, 0x24, 0x87, 0xa2, 0x20, 0x37, 0x4e, 0x8e, 0xbc, 0xd1,
    0x04, 0x5b, 0x98, 0x18, 0x7e, 0xa8, 0x54, 0xfd, 0xd0, 0xbf, 0xa6, 0x1b, 0x48, 0x0e, 0xe6, 0x19, 0xb4, 0xdf, 0xc4,
    0x3f, 0x8a, 0x19, 0xce, 0xa0, 0x14, 0x23, 0xb1, 0x47, 0x12, 0x2c, 0x1e, 0x8c, 0xa7,
};
static u8 sNasCert7Exponent[3] = { 1, 0, 1 };
static NasRootCert sNasCert7 = { sNasCert7Name, 128, sNasCert7Key, 3, sNasCert7Exponent };
static u8 sNasCert7Key[128] = {
    0xd2, 0x36, 0x36, 0x6a, 0x8b, 0xd7, 0xc2, 0x5b, 0x9e, 0xda, 0x81, 0x41, 0x62, 0x8f, 0x38, 0xee, 0x49, 0x04, 0x55,
    0xd6, 0xd0, 0xef, 0x1c, 0x1b, 0x95, 0x16, 0x47, 0xef, 0x18, 0x48, 0x35, 0x3a, 0x52, 0xf4, 0x2b, 0x6a, 0x06, 0x8f,
    0x3b, 0x2f, 0xea, 0x56, 0xe3, 0xaf, 0x86, 0x8d, 0x9e, 0x17, 0xf7, 0x9e, 0xb4, 0x65, 0x75, 0x02, 0x4d, 0xef, 0xcb,
    0x09, 0xa2, 0x21, 0x51, 0xd8, 0x9b, 0xd0, 0x67, 0xd0, 0xba, 0x0d, 0x92, 0x06, 0x14, 0x73, 0xd4, 0x93, 0xcb, 0x97,
    0x2a, 0x00, 0x9c, 0x5c, 0x4e, 0x0c, 0xbc, 0xfa, 0x15, 0x52, 0xfc, 0xf2, 0x44, 0x6e, 0xda, 0x11, 0x4a, 0x6e, 0x08,
    0x9f, 0x2f, 0x2d, 0xe3, 0xf9, 0xaa, 0x3a, 0x86, 0x73, 0xb6, 0x46, 0x53, 0x58, 0xc8, 0x89, 0x05, 0xbd, 0x83, 0x11,
    0xb8, 0x73, 0x3f, 0xaa, 0x07, 0x8d, 0xf4, 0x42, 0x4d, 0xe7, 0x40, 0x9d, 0x1c, 0x37,
};
static char sNasCert7Name[136] = "ZA, Western Cape, Cape Town, Thawte Consulting cc, Certification Services Division, "
                                 "Thawte Premium Server CA, premium-server@thawte.com";
static u8 sNasCert8Exponent[3] = { 1, 0, 1 };
static NasRootCert sNasCert8 = { sNasCert8Name, 128, sNasCert8Key, 3, sNasCert8Exponent };
static char sNasCert8Name[126] = "ZA, Western Cape, Cape Town, Thawte Consulting cc, Certification Services Division, "
                                 "Thawte Server CA, server-certs@thawte.com";
static u8 sNasCert8Key[128] = {
    0xd3, 0xa4, 0x50, 0x6e, 0xc8, 0xff, 0x56, 0x6b, 0xe6, 0xcf, 0x5d, 0xb6, 0xea, 0x0c, 0x68, 0x75, 0x47, 0xa2, 0xaa,
    0xc2, 0xda, 0x84, 0x25, 0xfc, 0xa8, 0xf4, 0x47, 0x51, 0xda, 0x85, 0xb5, 0x20, 0x74, 0x94, 0x86, 0x1e, 0x0f, 0x75,
    0xc9, 0xe9, 0x08, 0x61, 0xf5, 0x06, 0x6d, 0x30, 0x6e, 0x15, 0x19, 0x02, 0xe9, 0x52, 0xc0, 0x62, 0xdb, 0x4d, 0x99,
    0x9e, 0xe2, 0x6a, 0x0c, 0x44, 0x38, 0xcd, 0xfe, 0xbe, 0xe3, 0x64, 0x09, 0x70, 0xc5, 0xfe, 0xb1, 0x6b, 0x29, 0xb6,
    0x2f, 0x49, 0xc8, 0x3b, 0xd4, 0x27, 0x04, 0x25, 0x10, 0x97, 0x2f, 0xe7, 0x90, 0x6d, 0xc0, 0x28, 0x42, 0x99, 0xd7,
    0x4c, 0x43, 0xde, 0xc3, 0xf5, 0x21, 0x6d, 0x54, 0x9f, 0x5d, 0xc3, 0x58, 0xe1, 0xc0, 0xe4, 0xd9, 0x5b, 0xb0, 0xb8,
    0xdc, 0xb4, 0x7b, 0xdf, 0x36, 0x3a, 0xc2, 0xb5, 0x66, 0x22, 0x12, 0xd6, 0x87, 0x0d,
};
static u8 sNasCert9Exponent[3] = { 1, 0, 1 };
static NasRootCert sNasCert9 = { sNasCert9Name, 128, sNasCert9Key, 3, sNasCert9Exponent };
static u8 sNasCert9Key[128] = {
    0xcc, 0x5e, 0xd1, 0x11, 0x5d, 0x5c, 0x69, 0xd0, 0xab, 0xd3, 0xb9, 0x6a, 0x4c, 0x99, 0x1f, 0x59, 0x98, 0x30, 0x8e,
    0x16, 0x85, 0x20, 0x46, 0x6d, 0x47, 0x3f, 0xd4, 0x85, 0x20, 0x84, 0xe1, 0x6d, 0xb3, 0xf8, 0xa4, 0xed, 0x0c, 0xf1,
    0x17, 0x0f, 0x3b, 0xf9, 0xa7, 0xf9, 0x25, 0xd7, 0xc1, 0xcf, 0x84, 0x63, 0xf2, 0x7c, 0x63, 0xcf, 0xa2, 0x47, 0xf2,
    0xc6, 0x5b, 0x33, 0x8e, 0x64, 0x40, 0x04, 0x68, 0xc1, 0x80, 0xb9, 0x64, 0x1c, 0x45, 0x77, 0xc7, 0xd8, 0x6e, 0xf5,
    0x95, 0x29, 0x3c, 0x50, 0xe8, 0x34, 0xd7, 0x78, 0x1f, 0xa8, 0xba, 0x6d, 0x43, 0x91, 0x95, 0x8f, 0x45, 0x57, 0x5e,
    0x7e, 0xc5, 0xfb, 0xca, 0xa4, 0x04, 0xeb, 0xea, 0x97, 0x37, 0x54, 0x30, 0x6f, 0xbb, 0x01, 0x47, 0x32, 0x33, 0xcd,
    0xdc, 0x57, 0x9b, 0x64, 0x69, 0x61, 0xf8, 0x9b, 0x1d, 0x1c, 0x89, 0x4f, 0x5c, 0x67,
};
static char sNasCert9Name[147] = "US, VeriSign, Inc., Class 3 Public Primary Certification Authority - G2, (c) 1998 "
                                 "VeriSign, Inc. - For authorized use only, VeriSign Trust Network";
static u8 sNasCert10Exponent[3] = { 1, 0, 1 };
static NasRootCert sNasCert10 = { sNasCert10Name, 256, sNasCert10Key, 3, sNasCert10Exponent };
static char sNasCert10Name[156] = "US, VeriSign, Inc., VeriSign Trust Network, (c) 1999 VeriSign, Inc. - For "
                                  "authorized use only, VeriSign Class 3 Public Primary Certification Authority - G3";
static u8 sNasCert10Key[256] = {
    0xcb, 0xba, 0x9c, 0x52, 0xfc, 0x78, 0x1f, 0x1a, 0x1e, 0x6f, 0x1b, 0x37, 0x73, 0xbd, 0xf8, 0xc9, 0x6b, 0x94, 0x12,
    0x30, 0x4f, 0xf0, 0x36, 0x47, 0xf5, 0xd0, 0x91, 0x0a, 0xf5, 0x17, 0xc8, 0xa5, 0x61, 0xc1, 0x16, 0x40, 0x4d, 0xfb,
    0x8a, 0x61, 0x90, 0xe5, 0x76, 0x20, 0xc1, 0x11, 0x06, 0x7d, 0xab, 0x2c, 0x6e, 0xa6, 0xf5, 0x11, 0x41, 0x8e, 0xfa,
    0x2d, 0xad, 0x2a, 0x61, 0x59, 0xa4, 0x67, 0x26, 0x4c, 0xd0, 0xe8, 0xbc, 0x52, 0x5b, 0x70, 0x20, 0x04, 0x58, 0xd1,
    0x7a, 0xc9, 0xa4, 0x69, 0xbc, 0x83, 0x17, 0x64, 0xad, 0x05, 0x8b, 0xbc, 0xd0, 0x58, 0xce, 0x8d, 0x8c, 0xf5, 0xeb,
    0xf0, 0x42, 0x49, 0x0b, 0x9d, 0x97, 0x27, 0x67, 0x32, 0x6e, 0xe1, 0xae, 0x93, 0x15, 0x1c, 0x70, 0xbc, 0x20, 0x4d,
    0x2f, 0x18, 0xde, 0x92, 0x88, 0xe8, 0x6c, 0x85, 0x57, 0x11, 0x1a, 0xe9, 0x7e, 0xe3, 0x26, 0x11, 0x54, 0xa2, 0x45,
    0x96, 0x55, 0x83, 0xca, 0x30, 0x89, 0xe8, 0xdc, 0xd8, 0xa3, 0xed, 0x2a, 0x80, 0x3f, 0x7f, 0x79, 0x65, 0x57, 0x3e,
    0x15, 0x20, 0x66, 0x08, 0x2f, 0x95, 0x93, 0xbf, 0xaa, 0x47, 0x2f, 0xa8, 0x46, 0x97, 0xf0, 0x12, 0xe2, 0xfe, 0xc2,
    0x0a, 0x2b, 0x51, 0xe6, 0x76, 0xe6, 0xb7, 0x46, 0xb7, 0xe2, 0x0d, 0xa6, 0xcc, 0xa8, 0xc3, 0x4c, 0x59, 0x55, 0x89,
    0xe6, 0xe8, 0x53, 0x5c, 0x1c, 0xea, 0x9d, 0xf0, 0x62, 0x16, 0x0b, 0xa7, 0xc9, 0x5f, 0x0c, 0xf0, 0xde, 0xc2, 0x76,
    0xce, 0xaf, 0xf7, 0x6a, 0xf2, 0xfa, 0x41, 0xa6, 0xa2, 0x33, 0x14, 0xc9, 0xe5, 0x7a, 0x63, 0xd3, 0x9e, 0x62, 0x37,
    0xd5, 0x85, 0x65, 0x9e, 0x0e, 0xe6, 0x53, 0x24, 0x74, 0x1b, 0x5e, 0x1d, 0x12, 0x53, 0x5b, 0xc7, 0x2c, 0xe7, 0x83,
    0x49, 0x3b, 0x15, 0xae, 0x8a, 0x68, 0xb9, 0x57, 0x97,
};
static u8 sNasCert11Exponent[3] = { 1, 0, 1 };
static NasRootCert sNasCert11 = { sNasCert11Name, 128, sNasCert11Key, 3, sNasCert11Exponent };
static char sNasCert11Name[67] = "US, VeriSign, Inc., Class 3 Public Primary Certification Authority";
static u8 sNasCert11Key[128] = {
    0xc9, 0x5c, 0x59, 0x9e, 0xf2, 0x1b, 0x8a, 0x01, 0x14, 0xb4, 0x10, 0xdf, 0x04, 0x40, 0xdb, 0xe3, 0x57, 0xaf, 0x6a,
    0x45, 0x40, 0x8f, 0x84, 0x0c, 0x0b, 0xd1, 0x33, 0xd9, 0xd9, 0x11, 0xcf, 0xee, 0x02, 0x58, 0x1f, 0x25, 0xf7, 0x2a,
    0xa8, 0x44, 0x05, 0xaa, 0xec, 0x03, 0x1f, 0x78, 0x7f, 0x9e, 0x93, 0xb9, 0x9a, 0x00, 0xaa, 0x23, 0x7d, 0xd6, 0xac,
    0x85, 0xa2, 0x63, 0x45, 0xc7, 0x72, 0x27, 0xcc, 0xf4, 0x4c, 0xc6, 0x75, 0x71, 0xd2, 0x39, 0xef, 0x4f, 0x42, 0xf0,
    0x75, 0xdf, 0x0a, 0x90, 0xc6, 0x8e, 0x20, 0x6f, 0x98, 0x0f, 0xf8, 0xac, 0x23, 0x5f, 0x70, 0x29, 0x36, 0xa4, 0xc9,
    0x86, 0xe7, 0xb1, 0x9a, 0x20, 0xcb, 0x53, 0xa5, 0x85, 0xe7, 0x3d, 0xbe, 0x7d, 0x9a, 0xfe, 0x24, 0x45, 0x33, 0xdc,
    0x76, 0x15, 0xed, 0x0f, 0xa2, 0x71, 0x64, 0x4c, 0x65, 0x2e, 0x81, 0x68, 0x45, 0xa7,
};
static u8 sNasCert12Exponent[3] = { 1, 0, 1 };
static NasRootCert sNasCert12 = { sNasCert12Name, 125, sNasCert12Key, 3, sNasCert12Exponent };
static char sNasCert12Name[67] = "US, RSA Data Security, Inc., Secure Server Certification Authority";
static u8 sNasCert12Key[125] = {
    0x92, 0xce, 0x7a, 0xc1, 0xae, 0x83, 0x3e, 0x5a, 0xaa, 0x89, 0x83, 0x57, 0xac, 0x25, 0x01, 0x76, 0x0c, 0xad,
    0xae, 0x8e, 0x2c, 0x37, 0xce, 0xeb, 0x35, 0x78, 0x64, 0x54, 0x03, 0xe5, 0x84, 0x40, 0x51, 0xc9, 0xbf, 0x8f,
    0x08, 0xe2, 0x8a, 0x82, 0x08, 0xd2, 0x16, 0x86, 0x37, 0x55, 0xe9, 0xb1, 0x21, 0x02, 0xad, 0x76, 0x68, 0x81,
    0x9a, 0x05, 0xa2, 0x4b, 0xc9, 0x4b, 0x25, 0x66, 0x22, 0x56, 0x6c, 0x88, 0x07, 0x8f, 0xf7, 0x81, 0x59, 0x6d,
    0x84, 0x07, 0x65, 0x70, 0x13, 0x71, 0x76, 0x3e, 0x9b, 0x77, 0x4c, 0xe3, 0x50, 0x89, 0x56, 0x98, 0x48, 0xb9,
    0x1d, 0xa7, 0x29, 0x1a, 0x13, 0x2e, 0x4a, 0x11, 0x59, 0x9c, 0x1e, 0x15, 0xd5, 0x49, 0x54, 0x2c, 0x73, 0x3a,
    0x69, 0x82, 0xb1, 0x97, 0x39, 0x9c, 0x6d, 0x70, 0x67, 0x48, 0xe5, 0xdd, 0x2d, 0xd6, 0xc8, 0x1e, 0x7b,
};
