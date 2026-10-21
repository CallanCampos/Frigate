#include "dwc/ac.h"
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
#include <errno.h>
#include <wchar.h>

// DWC's profanity check (overlay 11, Thumb): it sends up to 50 words to the server's "pr" request, after the login's
// form fields, and reads which of them it finds profane. The ROM embeds no name; the file is named for what it does,
// after DWC_CheckProfanityExAsync, and keeps its own copies of the strings that the other requests' files share
// ("Date", "httpresult", "returncd")

// The letter of the game's region, the last of its code in the ROM header that the card leaves in main memory
#define ROM_REGION (*(const char *)(HW_ROM_HEADER_BUF + 0xf))

// The profanity check's state (0x1d64 bytes): its HTTP client, the words it asked about and where the answers go
typedef struct NasWordWork {
    DWCHttp http;
    BOOL active;
    char url[0x104];
    char *flags;
    int *profaneCount;
    int count;
} NasWordWork;

static BOOL DWCi_Prof_StartRequest(const u16 **words, int count, const char *type, int timeout, char *flags,
                                   int *profaneCount, const char *region);
static BOOL DWCi_Prof_ParseResult(void);
static void *DWCi_Prof_Alloc(u32 name, u32 size);
static void DWCi_Prof_Free(u32 name, void *ptr, u32 size);

static NasWordWork *sWords = NULL;

static BOOL DWCi_Prof_StartRequest(const u16 **words, int count, const char *type, int timeout, char *flags,
                                   int *profaneCount, const char *region) {
    int total;
    u16 *joined = NULL;
    DWCHttpParam config;
    NasAuthInfo info;
    int i;
    int position;
    int next;
    char *found;

    if (words == NULL || count <= 0 || count > 50 || flags == NULL || profaneCount == NULL) {
        return FALSE;
    }
    total = 0;
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
        strncpy(sWords->url, DWCi_AuthConfig.url, 0x100);
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
            if (timeout <= 0) {
                config.timeout = 10000;
            } else {
                config.timeout = timeout;
            }
            if (DWC_Http_Create(&sWords->http, &config) == 0) {
                sWords->active = 1;
                if (DWCi_Auth_GetCurrentInfo(&info) && DWCi_Auth_AddCommonItems(&sWords->http, &info, DWCi_SendSsid) &&
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
