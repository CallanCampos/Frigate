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
// runs them, and the form fields that the requests share. The ROM embeds no name for the file, so the name is a guess
// from what the code does. The HTTP client, the connection test, the profanity check, the service locator and the root
// certificates follow it in files of their own (dwc_http_client.c, dwc_conntest.c, dwc_profanity.c, dwc_svcloc.c and
// dwc_rootcert_*.c). See dwc/nasfunc.h for the layouts that the friends' match shares.

// The game's code and the maker's code, in the ROM header that the card leaves in main memory
#define ROM_GAME_CODE ((const u8 *)(HW_ROM_HEADER_BUF + 0xc))
#define ROM_MAKER_CODE ((const u8 *)(HW_ROM_HEADER_BUF + 0x10))

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

static void DWCi_Auth_StartThread(void);
static int DWCi_Auth_SendRequest(int mode);
static void DWCi_Auth_Thread(void *arg);
static int DWCi_Auth_HandleResponse(void);
static int DWCi_Auth_ParseResult(void);
static int DWCi_Auth_AddPostItems(DWCHttp *http, const char *gsbrcd, const u16 *ingamesn);
static void DWCi_Auth_SetError(int step);
static void DWCi_Auth_ClearResult(void);
static BOOL DWCi_Auth_GetInfo(NasAuthInfo *info, const DWCBmWifiIdInfo *user);

// The statics are declared from the highest address down: the compiler lays out .bss in reverse order of declaration
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

DWCHttpParam DWCi_AuthConfig = { "", 0, 0x1000, NULL, NULL, 0, 20000 };
static char sDefaultUrl[] = "https://nas.nintendowifi.net/ac";

void DWC_Auth_SetCustomNas(const char *url) {
    DWCi_AuthConfig.url = url;
}

const char *DWCi_Auth_GetCustomNas(void) {
    return DWCi_AuthConfig.url;
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

void DWC_Auth_Abort(void) {
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

void DWC_Auth_Join(void) {
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
    if (strcmp(DWCi_AuthConfig.url, sDefaultUrl) != 0) {
        DWCi_AuthConfig.ignoreCa = 1;
    }
    DWCi_AuthConfig.alloc = sNas.work->param.alloc;
    DWCi_AuthConfig.free = sNas.work->param.free;
    if (DWC_Http_Create(sNas.work->http, &DWCi_AuthConfig) != 0) {
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
    if (!DWCi_Auth_AddCommonItems(http, &info, DWCi_SendSsid)) {
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

BOOL DWCi_Auth_GetCurrentInfo(NasAuthInfo *info) {
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
    const u8 *apMac;
    u8 *ssid;
    int i;
    u32 level;

    MI_CpuFill8(info, 0, sizeof(NasAuthInfo));
    if (user->id != 0) {
        OS_SNPrintf(info->userId, 0xe, "%013llu", user->id);
    } else {
        OS_SNPrintf(info->userId, 0xe, "%013llu", user->pendingId);
    }
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

BOOL DWCi_Auth_AddCommonItems(DWCHttp *http, const NasAuthInfo *info, int sendSsid) {
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
