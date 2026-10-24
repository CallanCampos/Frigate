#include "dwc/nd.h"
#include "dwc/dwc.h"
#include "dwc/nhttp.h"
#include "nd_internal.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// NitroDWC's download library (DWC_Nd), which the games use to fetch files from the Nintendo Wi-Fi Connection's
// download server (Mystery Gift's gifts). The ROM names none of it: see nd.h for where the names come from.
//
// A call starts a request to the server with HTTP's POST; the parameters travel in base64 (a variant that pads with
// '*', and puts '.' and '-' for 62 and 63) and the answer comes back as a status and a body. A thread waits until the
// Wi-Fi connection is up and sets the library up; NdReportResults then reports each result to the game's callback.

// Where the library is: 0 idle, 1 ready, 2 busy with a call, 3 a call ended well, 4 one failed
enum {
    ND_PHASE_IDLE,
    ND_PHASE_READY,
    ND_PHASE_BUSY,
    ND_PHASE_DONE,
    ND_PHASE_FAILED,
};

// The events of the HTTP client's callback
enum {
    NHTTP_EVENT_BUFFER_FULL = 2,
    NHTTP_EVENT_AFTER_BUFFER_FULL,
    NHTTP_EVENT_ANSWER,
};

// The answer's buffer was too small; the callback sets whether the client may grow it
typedef struct NdBufferFullEvent {
    u8 unk0[8];
    BOOL grow;
} NdBufferFullEvent;

// The thread's memory: what DWC_NdInitAsync was given, the connection's information (the service locator's host and
// token), the thread's stack and the thread
typedef struct DWCNdCnt {
    char gamecd[0x20];
    char passwd[0x20];
    // Whether the thread set the library up
    BOOL ndinitialized;
    DWCSvlResult svlresult;
    u8 svlthreadstack[0x800];
    OSThread svlthread;
} DWCNdCnt;

// The call in progress and the result that NdReportResults gives the game
typedef struct NdState {
    DWCNdCallback callback;
    BOOL notified;
    int code;
    int error;
    int reason;
    int phase;
    DWCNdCnt *work;
} NdState;

// The library's settings, and the request that is running
typedef struct NdContext {
    // Records a result, and is given to the requests
    DWCNdCallback onResult;
    NHTTPAlloc alloc;
    BOOL running;
    BOOL hasApInfo;
    int request;
    NHTTPFree free;
    char *token;
    char *url;
    // Whether a request is in flight
    BOOL requestActive;
    void (*onCleanedUp)(void);
    // The game's code and the cartridge's, in base64
    char rhGameCodeParam[9];
    char gameCodeParam[9];
    // The attributes of DWC_NdSetAttr, in base64
    char *attrs[3];
} NdContext;

// A request in flight, which the HTTP client's callback gets back
typedef struct NdTask {
    int kind;
    DWCNdCallback callback;
    // A queue to wake when the request has ended
    OSMessageQueue *waiter;
    // What the answer fills: the list of files, the number, or the name of the file
    union {
        DWCNdFileInfo *files;
        int *number;
        char *fileName;
    } target;
    int count;
    char *buffer;
    char *offsetParam;
    char *countParam;
} NdTask;

// The certificate that the server's identity is checked against
typedef struct NdCertificate {
    const char *name;
    int modulusSize;
    const u8 *modulus;
    int exponentSize;
    const u8 *exponent;
} NdCertificate;

static void *DWCi_NdAlloc(u32 size, int align);
static void DWCi_NdFree(void *ptr);
static void DWCi_NdCleanupCallback(void);
static void DWCi_SVLTokenWaitThread(void *arg);
static void DWCi_NdCallback(DWCNdCallbackReason reason, DWCNdError error, int code);
static void DWCi_Ndi_freeAttr(void);
static BOOL setAttr(int index, const char *text);
static BOOL DWCi_NdSetAttr(const char *attr1, const char *attr2, const char *attr3);
static int getNchar(u8 *dest, const char *src, int size);
static int decodeChar(u32 c);
static u32 DWCi_Ndi_getDecodeLenBASE64(int size);
static int DWCi_Ndi_getEncodeLenBASE64(int size);
static int DWCi_Ndi_encodeBASE64(char *dest, const char *src, int srcSize);
static int DWCi_Ndi_decodeBASE64(char *dest, const char *src, int srcSize);
static char *strAlloc(const char *text);
static BOOL DWCi_NdStartup(NHTTPAlloc alloc, NHTTPFree free, const char *url, const char *token, const char *gameCode,
                           const char *password, DWCNdCallback onResult);
static void nhttpCleanupCallback(void);
static void DWCi_NdCleanupAsync(void (*onCleanedUp)(void));
static BOOL ReleaseRequest(int request);
static void SetWifiError(int error, int serverError);
static BOOL DWCi_NdCancelAsync(void);
static BOOL DWCi_Ndi_getApInfo(char *dest);
static BOOL DWCi_Ndi_getDSdesc(char *gameCode, char *userId);
static void DWCi_Ndi_getUserAgent(char *dest);
static int req_callback(int request, int event, void *arg);
static void FreeTask(NdTask *task);
static void freeHttp(NdTask *task, int request);
static BOOL setupHttp(NdTask **taskOut, int *requestOut, const char *action, char *buffer, int bufferSize,
                      const char *logData);
static BOOL runHttp(NdTask *task, int request);
static BOOL DWCi_NdGetFileListAsync(DWCNdFileInfo *files, u32 offset, u32 count);
static BOOL DWCi_NdGetFileAsync(DWCNdFileInfo *file, void *buffer, u32 size, const char *logData);
static BOOL DWCi_NdGetProgress(u32 *received, u32 *total);
static BOOL DWCi_Ndi_response_count(int request, NdTask *task);
static BOOL DWCi_Ndi_response_contents(int request, NdTask *task);
static BOOL DWCi_Ndi_response_list(int request, NdTask *task);
static BOOL DWCi_Ndi_initLockGlobal(void);
static void DWCi_Ndi_exitLockGlobal(void);
static void DWCi_Ndi_lockGlobal(void);
static void DWCi_Ndi_unlockGlobal(void);
static BOOL DWCi_Ndi_initHttpCallback(void);
static void DWCi_Ndi_exitHttpCallback(void);
static BOOL DWCi_Ndi_prepareHttpCallback(void);
static void DWCi_Ndi_unprepareHttpCallback(void);
static void DWCi_Ndi_kickThread(OSMessageQueue *queue);
static void DWCi_Ndi_getMacAddress(char *dest);
static int DWCi_Ndi_strlen(const char *text);
static int DWCi_Ndi_strncmp(const char *a, const char *b, int size);
static void DWCi_Ndi_memclr(void *dest, int size);
static void DWCi_Ndi_memcpy(void *dest, const void *src, int size);
static int DWCi_Ndi_strToInt(const char *text, int size);
static int DWCi_Ndi_intToStr(char *dest, u32 value);
static int DWCi_Ndi_intToStrFigure(char *dest, u64 value, int minDigits);
static int DWCi_Ndi_hexToStrFigure(char *dest, u32 value, int digits);

static char sNdLogDataParam[0x59];
static DWCApInfo sNdApInfo;
static char sNdUserAgent[0x21];
static char sNdPasswordParam[25];
static OSMutex sNdAttrMutex;
static OSMutex sNdRequestMutex;
static char sNdUserIdParam[21];
static char sNdApInfoParam[21];
static char sNdMacParam[17];
static NdContext sNdContext;
static OSMutex sNdMutex;
static NdState sDwcNdState;

// The padding character of the base64, and the parameter "action" of the requests, "count", "list" and "contents" in
// base64; the first two are not used
static const char pad[1] = { '*' };
static const char sActionCount[] = "Y291bnQ*";
static const char sActionList[] = "bGlzdA**";
static const char sActionContents[] = "Y29udGVudHM*";

static void *DWCi_NdAlloc(u32 size, int align) {
    return DWC_AllocEx(10, size, align);
}

static void DWCi_NdFree(void *ptr) {
    DWC_Free(10, ptr, 0);
}

// Ends the thread and gives its memory back, and tells NdReportResults that the library has ended
static void DWCi_NdCleanupCallback(void) {
    OS_JoinThread(&sDwcNdState.work->svlthread);
    DWCi_NdFree(sDwcNdState.work);
    sDwcNdState.work = NULL;
    OS_LockMutex(&sNdMutex);
    sDwcNdState.phase = ND_PHASE_DONE;
    sDwcNdState.reason = ND_CBREASON_CLEANED_UP;
    OS_UnlockMutex(&sNdMutex);
}

// Waits for the Wi-Fi connection, then sets the library up with the connection's host
static void DWCi_SVLTokenWaitThread(void *arg) {
    char url[0x100];
    char token[0x190];

    for (;;) {
        switch (DWC_SVLProcess()) {
        case 3: {
            DWCNdCnt *work = sDwcNdState.work;

            if (strlen(work->svlresult.svlhost) + strlen("https:///download") > 0xff) {
                DWCi_NdCallback(DWC_ND_CBREASON_INITIALIZE, DWC_ND_ERROR_FATAL, -1);
                return;
            }
            OS_SNPrintf(url, 0x100, "https://%s/download", work->svlresult.svlhost);
            work = sDwcNdState.work;
            if (DWC_Base64Encode(work->svlresult.svltoken, strlen(work->svlresult.svltoken), token, 0x190) < 0) {
                DWCi_NdCallback(DWC_ND_CBREASON_INITIALIZE, DWC_ND_ERROR_FATAL, -1);
                return;
            }
            if (!DWCi_NdStartup(DWCi_NdAlloc, DWCi_NdFree, url, token, sDwcNdState.work->gamecd,
                                sDwcNdState.work->passwd, DWCi_NdCallback)) {
                DWCi_NdCallback(DWC_ND_CBREASON_INITIALIZE, DWC_ND_ERROR_FATAL, -1);
                return;
            }
            sDwcNdState.work->ndinitialized = TRUE;
            DWCi_NdCallback(DWC_ND_CBREASON_INITIALIZE, DWC_ND_ERROR_NONE, -1);
            return;
        }
        case 4: {
            int code;
            DWC_GetLastError(&code);
            DWCi_SetError(13, code);
            DWCi_NdCallback(DWC_ND_CBREASON_INITIALIZE, DWC_ND_ERROR_HTTP, -1);
            return;
        }
        case 5:
            DWCi_NdCallback(DWC_ND_CBREASON_INITIALIZE, DWC_ND_ERROR_CANCELED, -1);
            return;
        default:
            OS_Sleep(100);
            break;
        }
    }
}

// Records the result of a call, for NdReportResults to give the game
static void DWCi_NdCallback(DWCNdCallbackReason reason, DWCNdError error, int code) {
    if (error != 0) {
        SetWifiError(error, code);
    }
    OS_LockMutex(&sNdMutex);
    sDwcNdState.phase = ND_PHASE_DONE;
    sDwcNdState.reason = reason;
    sDwcNdState.error = error;
    sDwcNdState.code = code;
    if (error != 0) {
        sDwcNdState.phase = ND_PHASE_FAILED;
    }
    OS_UnlockMutex(&sNdMutex);
}

BOOL DWC_NdInitAsync(DWCNdCallback callback, char *gameCode, char *password) {
    if (DWCi_IsError()) {
        return FALSE;
    }
    OS_InitMutex(&sNdMutex);
    OS_LockMutex(&sNdMutex);
    if (sDwcNdState.phase != ND_PHASE_IDLE) {
        SetWifiError(DWC_ND_ERROR_BUSY, 0);
        OS_UnlockMutex(&sNdMutex);
        return FALSE;
    }
    sDwcNdState.phase = ND_PHASE_BUSY;
    OS_UnlockMutex(&sNdMutex);
    sDwcNdState.reason = 0;
    sDwcNdState.error = 0;
    sDwcNdState.code = 0;
    sDwcNdState.callback = callback;
    sDwcNdState.notified = FALSE;
    sDwcNdState.work = DWCi_NdAlloc(sizeof(DWCNdCnt), 32);
    if (sDwcNdState.work == NULL) {
        SetWifiError(DWC_ND_ERROR_ALLOC, 0);
        return FALSE;
    }
    MI_CpuFill8(sDwcNdState.work, 0, sizeof(DWCNdCnt));
    sDwcNdState.work->ndinitialized = FALSE;
    strncpy(sDwcNdState.work->gamecd, gameCode, 0x1f);
    strncpy(sDwcNdState.work->passwd, password, 0x1f);
    if (!DWC_SVLGetTokenAsync("9000", &sDwcNdState.work->svlresult)) {
        DWCi_NdFree(sDwcNdState.work);
        sDwcNdState.work = NULL;
        return FALSE;
    }
    OS_CreateThread(&sDwcNdState.work->svlthread, DWCi_SVLTokenWaitThread, NULL,
                    sDwcNdState.work->svlthreadstack + sizeof(sDwcNdState.work->svlthreadstack),
                    sizeof(sDwcNdState.work->svlthreadstack), 16);
    OS_WakeupThreadDirect(&sDwcNdState.work->svlthread);
    return TRUE;
}

void NdReportResults(void) {
    BOOL notify = FALSE;
    BOOL ended = FALSE;
    int reason;

    OS_LockMutex(&sNdMutex);
    reason = sDwcNdState.reason;
    if (reason == 0 && sDwcNdState.phase == ND_PHASE_FAILED) {
        if (!sDwcNdState.notified) {
            notify = TRUE;
            sDwcNdState.notified = TRUE;
        }
    } else if ((u32)(sDwcNdState.phase - ND_PHASE_DONE) <= 1) {
        notify = TRUE;
        sDwcNdState.phase = ND_PHASE_READY;
    }
    if (reason == ND_CBREASON_CLEANED_UP) {
        sDwcNdState.phase = ND_PHASE_IDLE;
        sDwcNdState.reason = 0;
        ended = TRUE;
    }
    OS_UnlockMutex(&sNdMutex);
    if (sDwcNdState.callback != NULL) {
        if (ended) {
            sDwcNdState.callback(ND_CBREASON_CLEANED_UP, DWC_ND_ERROR_NONE, -1);
        } else if (notify) {
            sDwcNdState.callback(sDwcNdState.reason, sDwcNdState.error, sDwcNdState.code);
        }
    }
}

BOOL DWC_NdCleanupAsync(void) {
    if (sDwcNdState.phase == ND_PHASE_IDLE) {
        return FALSE;
    }
    OS_LockMutex(&sNdMutex);
    if (sDwcNdState.phase == ND_PHASE_IDLE) {
        OS_UnlockMutex(&sNdMutex);
        return FALSE;
    }
    sDwcNdState.phase = ND_PHASE_BUSY;
    OS_UnlockMutex(&sNdMutex);
    if (!sDwcNdState.work->ndinitialized) {
        DWCi_NdCleanupCallback();
    } else {
        DWCi_NdCleanupAsync(DWCi_NdCleanupCallback);
    }
    return TRUE;
}

BOOL DWC_NdSetAttr(const char *attr1, const char *attr2, const char *attr3) {
    if (DWCi_IsError()) {
        return FALSE;
    }
    return DWCi_NdSetAttr(attr1, attr2, attr3);
}

BOOL DWC_NdGetFileListAsync(DWCNdFileInfo *files, unsigned int offset, unsigned int count) {
    if (DWCi_IsError()) {
        return FALSE;
    }
    OS_LockMutex(&sNdMutex);
    if (sDwcNdState.phase != ND_PHASE_READY) {
        SetWifiError(DWC_ND_ERROR_BUSY, 0);
        OS_UnlockMutex(&sNdMutex);
        return FALSE;
    }
    sDwcNdState.phase = ND_PHASE_BUSY;
    OS_UnlockMutex(&sNdMutex);
    MI_CpuFill8(files, 0, sizeof(DWCNdFileInfo) * count);
    if (DWCi_NdGetFileListAsync(files, offset, count)) {
        return TRUE;
    }
    OS_LockMutex(&sNdMutex);
    sDwcNdState.phase = ND_PHASE_READY;
    OS_UnlockMutex(&sNdMutex);
    return FALSE;
}

BOOL DWC_NdGetFileAsync(DWCNdFileInfo *file, char *buffer, unsigned int size) {
    BOOL result;

    if (DWCi_IsError()) {
        return FALSE;
    }
    OS_LockMutex(&sNdMutex);
    if (sDwcNdState.phase != ND_PHASE_READY) {
        SetWifiError(DWC_ND_ERROR_BUSY, 0);
        OS_UnlockMutex(&sNdMutex);
        return FALSE;
    }
    sDwcNdState.phase = ND_PHASE_BUSY;
    OS_UnlockMutex(&sNdMutex);
    result = FALSE;
    if (DWCi_NdGetFileAsync(file, buffer, size, NULL)) {
        return TRUE;
    }
    OS_LockMutex(&sNdMutex);
    sDwcNdState.phase = ND_PHASE_READY;
    OS_UnlockMutex(&sNdMutex);
    return result;
}

BOOL DWC_NdCancelAsync(void) {
    if (DWCi_IsError()) {
        return FALSE;
    }
    OS_LockMutex(&sNdMutex);
    if (sDwcNdState.phase != ND_PHASE_BUSY) {
        OS_UnlockMutex(&sNdMutex);
        return FALSE;
    }
    OS_UnlockMutex(&sNdMutex);
    if (DWCi_NdCancelAsync()) {
        return TRUE;
    }
    return FALSE;
}

BOOL DWC_NdGetProgress(u32 *received, u32 *contentLength) {
    if (DWCi_IsError()) {
        return FALSE;
    }
    OS_LockMutex(&sNdMutex);
    if (sDwcNdState.phase != ND_PHASE_BUSY) {
        OS_UnlockMutex(&sNdMutex);
        return FALSE;
    }
    OS_UnlockMutex(&sNdMutex);
    return DWCi_NdGetProgress(received, contentLength);
}

// Gives the three attributes' memory back
static void DWCi_Ndi_freeAttr(void) {
    char **attrs = sNdContext.attrs;
    int i;

    for (i = 0; i < 3; i++) {
        if (attrs[i] != NULL) {
            sNdContext.free(attrs[i]);
            attrs[i] = NULL;
        }
    }
}

// Keeps an attribute, in base64, for the requests: up to 10 characters
static BOOL setAttr(int index, const char *text) {
    int size;
    int length;

    if (*text != '\0') {
        if (DWCi_Ndi_strlen(text) > 10) {
            SetWifiError(DWC_ND_ERROR_PARAM, 0);
            return FALSE;
        }
        size = DWCi_Ndi_strlen(text);
        if (size > 10) {
            SetWifiError(DWC_ND_ERROR_PARAM, 0);
            return FALSE;
        }
        length = DWCi_Ndi_getEncodeLenBASE64(size);
        sNdContext.attrs[index] = sNdContext.alloc(length + 1, 4);
        if (sNdContext.attrs[index] == NULL) {
            SetWifiError(DWC_ND_ERROR_ALLOC, 0);
            return FALSE;
        }
        length = DWCi_Ndi_encodeBASE64(sNdContext.attrs[index], text, size);
        sNdContext.attrs[index][length] = '\0';
    }
    return TRUE;
}

static BOOL DWCi_NdSetAttr(const char *attr1, const char *attr2, const char *attr3) {
    DWCi_Ndi_lockGlobal();
    DWCi_Ndi_freeAttr();
    if (setAttr(0, attr1) && setAttr(1, attr2) && setAttr(2, attr3)) {
        DWCi_Ndi_unlockGlobal();
        return TRUE;
    }
    DWCi_Ndi_freeAttr();
    DWCi_Ndi_unlockGlobal();
    return FALSE;
}

// Copies up to size characters, stopping to give zeros at a zero, and returns how many were not zero
static int getNchar(u8 *dest, const char *src, int size) {
    int i;
    int count = 0;

    for (i = 0; i < size; i++) {
        char c = *src;

        if (c != '\0') {
            count++;
            src++;
            *dest = c;
        } else {
            *dest = '\0';
        }
        dest++;
    }
    return count;
}

// The value of a character of the base64, or -1
static int decodeChar(u32 c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '.') {
        return 62;
    }
    if (c == '-') {
        return 63;
    }
    return -1;
}

static u32 DWCi_Ndi_getDecodeLenBASE64(int size) {
    return (size >> 2) * 3;
}

static int DWCi_Ndi_getEncodeLenBASE64(int size) {
    return (size + 2) / 3 * 4;
}

// Encodes the characters up to the first zero, and returns the length
static int DWCi_Ndi_encodeBASE64(char *dest, const char *src, int srcSize) {
    static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-";
    u8 chunk[3];
    int length = 0;
    int n;

    n = getNchar(chunk, src, srcSize > 3 ? 3 : srcSize);
    while (n > 0) {
        u8 index0;
        u8 index1;
        u8 index2;
        u8 index3;

        if (srcSize < 3) {
            chunk[2] = 0;
        }
        if (srcSize < 2) {
            chunk[1] = 0;
        }
        index0 = chunk[0] >> 2;
        index1 = ((chunk[0] << 4) | (chunk[1] >> 4)) & 0x3f;
        index2 = ((chunk[1] << 2) | (chunk[2] >> 6)) & 0x3f;
        index3 = chunk[2] & 0x3f;
        *dest++ = chars[index0];
        *dest++ = chars[index1];
        *dest++ = n > 1 ? chars[index2] : '*';
        *dest++ = n > 2 ? chars[index3] : '*';
        length += 4;
        src += n;
        srcSize -= n;
        if (srcSize == 0) {
            break;
        }
        n = getNchar(chunk, src, srcSize > 3 ? 3 : srcSize);
    }
    *dest = '\0';
    return length;
}

// Decodes at most srcSize characters, and returns the length of the bytes
static int DWCi_Ndi_decodeBASE64(char *dest, const char *src, int srcSize) {
    u8 in[4];
    char bytes[3];
    int length = 0;
    int n;
    int value;

    n = getNchar(in, src, 4);
    value = 0;
    while (n > 0 && value >= 0) {
        int i;

        bytes[0] = 0;
        bytes[1] = 0;
        bytes[2] = 0;
        value = decodeChar(in[0]);
        if (value >= 0) {
            bytes[0] = value << 2;
            value = decodeChar(in[1]);
            if (value >= 0) {
                bytes[0] |= value >> 4;
                bytes[1] = value << 4;
                value = decodeChar(in[2]);
                if (value >= 0) {
                    bytes[1] |= value >> 2;
                    bytes[2] = value << 6;
                    value = decodeChar(in[3]);
                    if (value >= 0) {
                        bytes[2] |= value;
                    }
                }
            }
        }
        for (i = 0; i < n - 1; i++) {
            *dest = bytes[i];
            dest++;
        }
        src += n;
        length += n - 1;
        srcSize -= 4;
        if (srcSize <= 0) {
            break;
        }
        n = getNchar(in, src, 4);
    }
    *dest = '\0';
    return length;
}

static char *strAlloc(const char *text) {
    int length = DWCi_Ndi_strlen(text);
    char *copy = sNdContext.alloc(length + 1, 4);

    if (copy != NULL) {
        DWCi_Ndi_memcpy(copy, text, length);
        copy[length] = '\0';
    }
    return copy;
}

// Sets the library up: the memory, the parameters that every request carries, and the HTTP client
static BOOL DWCi_NdStartup(NHTTPAlloc alloc, NHTTPFree free, const char *url, const char *token, const char *gameCode,
                           const char *password, DWCNdCallback onResult) {
    sNdContext.alloc = alloc;
    sNdContext.free = free;
    sNdContext.onResult = onResult;
    sNdContext.attrs[0] = NULL;
    sNdContext.attrs[1] = NULL;
    sNdContext.attrs[2] = NULL;
    sNdContext.hasApInfo = FALSE;
    sNdContext.request = 0;
    sNdContext.token = NULL;
    sNdContext.url = NULL;
    DWCi_Ndi_getMacAddress(sNdMacParam);
    DWCi_Ndi_getUserAgent(sNdUserAgent);
    sNdContext.url = strAlloc(url);
    if (sNdContext.url != NULL) {
        sNdContext.token = strAlloc(token);
        if (sNdContext.token != NULL) {
            sNdContext.gameCodeParam[DWCi_Ndi_encodeBASE64(sNdContext.gameCodeParam, gameCode, 4)] = '\0';
            sNdPasswordParam[DWCi_Ndi_encodeBASE64(sNdPasswordParam, password, 16)] = '\0';
            if (DWCi_Ndi_initHttpCallback() && DWCi_Ndi_initLockGlobal()) {
                if (NHTTPStartup(alloc, free, 17) != -1) {
                    sNdContext.hasApInfo = DWCi_Ndi_getApInfo(sNdApInfoParam);
                    sNdContext.running = TRUE;
                    return TRUE;
                }
                DWCi_Ndi_exitLockGlobal();
            }
            sNdContext.free(sNdContext.token);
            sNdContext.free(sNdContext.url);
            SetWifiError(DWC_ND_ERROR_FATAL, 0);
            return FALSE;
        }
    }
    if (sNdContext.token != NULL) {
        sNdContext.free(sNdContext.token);
    }
    if (sNdContext.url != NULL) {
        sNdContext.free(sNdContext.url);
    }
    SetWifiError(DWC_ND_ERROR_ALLOC, 0);
    return FALSE;
}

static void nhttpCleanupCallback(void) {
    DWCi_Ndi_freeAttr();
    DWCi_Ndi_exitLockGlobal();
    DWCi_Ndi_exitHttpCallback();
    sNdContext.free(sNdContext.token);
    sNdContext.free(sNdContext.url);
    sNdContext.onCleanedUp();
}

// Ends the library's request and the HTTP client, and then calls the function
static void DWCi_NdCleanupAsync(void (*onCleanedUp)(void)) {
    sNdContext.running = FALSE;
    sNdContext.onCleanedUp = onCleanedUp;
    ReleaseRequest(sNdContext.request);
    NHTTPCleanupAsync(nhttpCleanupCallback);
}

// Destroys a request; the answer is FALSE when it could not
static BOOL ReleaseRequest(int request) {
    BOOL result = TRUE;

    DWCi_Ndi_lockGlobal();
    if (request != 0) {
        NHTTPGetConnectionState(request);
        if (NHTTPDestroyConnection(request) != 0) {
            result = FALSE;
        } else {
            request = 0;
        }
    }
    DWCi_Ndi_unlockGlobal();
    return result;
}

// Gives the Wi-Fi library the error that a call of the download library ended with
static void SetWifiError(int error, int serverError) {
    switch (error) {
    case DWC_ND_ERROR_NONE:
    case DWC_ND_ERROR_CANCELED:
        break;
    case DWC_ND_ERROR_ALLOC:
        DWCi_SetError(DWC_ERROR_FATAL, -31001);
        break;
    case DWC_ND_ERROR_HTTP:
        DWCi_SetError(14, -31020);
        break;
    case DWC_ND_ERROR_BUFFULL:
        DWCi_SetError(13, -31040);
        break;
    case DWC_ND_ERROR_DLSERVER:
        DWCi_SetError(13, -31000 - serverError);
        break;
    case DWC_ND_ERROR_PARAM:
        DWCi_SetError(13, -31040);
        break;
    case DWC_ND_ERROR_FATAL:
        DWCi_SetError(DWC_ERROR_FATAL, -31009);
        break;
    case DWC_ND_ERROR_BUSY:
        DWCi_SetError(13, -31010);
        break;
    default:
        DWCi_SetError(DWC_ERROR_FATAL, -31009);
        break;
    }
}

// Cancels the request when one is running, and says whether it did
static BOOL DWCi_NdCancelAsync(void) {
    BOOL canceled;
    u32 received;
    u32 total;
    int request;

    if (!sNdContext.running) {
        return FALSE;
    }
    canceled = FALSE;
    received = 0;
    total = 0;
    DWC_NdGetProgress(&received, &total);
    if (received == total) {
        return canceled;
    }
    if (!DWCi_Ndi_prepareHttpCallback()) {
        DWCi_Ndi_lockGlobal();
        request = sNdContext.request;
        DWCi_Ndi_unlockGlobal();
        if (NHTTPCancelConnection(request) == 0) {
            canceled = TRUE;
        }
        return canceled;
    }
    DWCi_Ndi_unprepareHttpCallback();
    return canceled;
}

// Writes the information of the access point that the requests carry, in base64: a "00:0000000-00" with the type and
// the spot's digits in place
static BOOL DWCi_Ndi_getApInfo(char *dest) {
    char info[13] = "00:0000000-00";

    if (DWC_GetApInfo(&sNdApInfo)) {
        if (sNdApInfo.aptype >= 0 && sNdApInfo.aptype < 10) {
            info[1] = '0' + sNdApInfo.aptype;
        } else if (sNdApInfo.aptype >= 10 && sNdApInfo.aptype < 100) {
            DWCi_Ndi_intToStr(info, sNdApInfo.aptype);
        }
        if (sNdApInfo.aptype == 4 || sNdApInfo.aptype == 8) {
            if (sNdApInfo.area >= 0 && sNdApInfo.area < 10) {
                info[3] = '0' + sNdApInfo.area;
            }
            DWCi_Ndi_memcpy(info + 4, sNdApInfo.spotInfo, 9);
        }
        DWCi_Ndi_encodeBASE64(dest, info, 13);
        return TRUE;
    }
    return FALSE;
}

// Writes the console's identifier and the game's code in base64, for the requests
static BOOL DWCi_Ndi_getDSdesc(char *gameCode, char *userId) {
    DWCWiFiInfo ids;
    char digits[13];

    DWCi_Ndi_memclr(&ids, sizeof(ids));
    DWCi_BM_GetWiFiInfo(&ids);
    if (ids.attestedUserId == 0) {
        return FALSE;
    }
    if (DWCi_Ndi_intToStrFigure(digits, ids.attestedUserId, 13) < 0) {
        return FALSE;
    }
    DWCi_Ndi_encodeBASE64(userId, digits, 13);
    DWCi_Ndi_encodeBASE64(gameCode, (const char *)0x2fffe0c, 4);
    return TRUE;
}

// The certificate that the server's identity is checked against: its key is 1024 bits, with the exponent 65537
static u8 sNdCertExponent[3] = { 1, 0, 1 };
static NdCertificate sNdCertificate;
static NdCertificate *sNdCertificates[1] = { &sNdCertificate };
static char sNdCertName[80];
static u8 sNdCertModulus[128];
// clang-format off
static NdCertificate sNdCertificate = {
    sNdCertName,
    128,
    sNdCertModulus,
    3,
    sNdCertExponent,
};

// clang-format on
static char sNdCertName[80] = "US, Washington, Nintendo of America Inc, NOA, Nintendo CA, ca@noa.nintendo.com";
// clang-format off
static u8 sNdCertModulus[128] = {
    0xb3, 0xcd, 0x79, 0x97, 0x77, 0x5d, 0x8a, 0xaf, 0x86, 0xa8, 0xe8, 0xd7, 0x73, 0x1c, 0x77, 0xdf,
    0x10, 0x90, 0x1f, 0x81, 0xf8, 0x41, 0x9e, 0x21, 0x55, 0xdf, 0xbc, 0xfc, 0x63, 0xfb, 0x19, 0x43,
    0xf1, 0xf6, 0xc4, 0x72, 0x42, 0x49, 0xbd, 0xad, 0x44, 0x68, 0x4e, 0xf3, 0xda, 0x1d, 0xe6, 0x4d,
    0xd8, 0xf9, 0x59, 0x88, 0xdc, 0xae, 0x3e, 0x9b, 0x38, 0x09, 0xca, 0x7f, 0xff, 0xdc, 0x24, 0xa2,
    0x44, 0x78, 0x78, 0x49, 0x93, 0xd4, 0x84, 0x40, 0x10, 0xb8, 0xec, 0x3e, 0xdb, 0x2d, 0x93, 0xc8,
    0x11, 0xc8, 0xfd, 0x78, 0x2d, 0x61, 0xad, 0x31, 0xae, 0x86, 0x26, 0xb0, 0xfd, 0x5a, 0x3f, 0xa1,
    0x3d, 0xbf, 0xe2, 0x4b, 0x49, 0xec, 0xce, 0x66, 0x98, 0x58, 0x26, 0x12, 0xc0, 0xfb, 0xf4, 0x77,
    0x65, 0x1b, 0xea, 0xfb, 0xcb, 0x7f, 0xe0, 0x8c, 0xcb, 0x02, 0xa3, 0x4e, 0x5e, 0x8c, 0xea, 0x9b,
};

// clang-format on

// Writes "Nitro WiFi SDK/5.3"
static void DWCi_Ndi_getUserAgent(char *dest) {
    OS_SNPrintf(dest, 0x21, "Nitro WiFi SDK/%d.%d", 5, 3);
}

// The HTTP client's callback of every request: the answer of the server decides what the call ends with. The answer is
// a status, and with 302 the body carries "returncd=" and the server's code in base64
static int req_callback(int request, int event, void *arg) {
    NdTask *task;
    int kind;
    DWCNdCallback callback;
    OSMessageQueue *waiter;
    int status;
    int serverError;
    int error;
    int bodySize;
    char *statusText;
    int length;
    char *body;
    int start;
    int size;
    int pos;
    char code[4];
    BOOL isCode;

    serverError = -1;
    task = NHTTPGetUserParam(request);
    kind = task->kind;
    callback = task->callback;
    waiter = task->waiter;
    switch (event) {
    case NHTTP_EVENT_ANSWER:
        if (NHTTPGetConnectionError(request) == 8) {
            error = DWC_ND_ERROR_CANCELED;
        } else if (NHTTPGetHeaderField(request, "HTTPSTATUSCODE", &statusText) != 3) {
            error = DWC_ND_ERROR_HTTP;
        } else {
            status = DWCi_Ndi_strToInt(statusText, 3);
            if (status == 302) {
                isCode = FALSE;
                length = NHTTPGetBodyBuffer(request, &body, &bodySize);
                if (length < 0) {
                    error = DWC_ND_ERROR_HTTP;
                } else {
                    pos = 0;
                    while (pos < length) {
                        start = pos;
                        while (body[pos] != '=' && pos < length) {
                            pos++;
                        }
                        size = pos - start;
                        pos++;
                        if (size == 8) {
                            isCode = DWCi_Ndi_strncmp(body + start, "returncd", 8) == 0;
                        }
                        start = pos;
                        while (body[pos] != '&' && body[pos] != '\r' && body[pos] != '\0' && pos < length) {
                            pos++;
                        }
                        size = pos - start;
                        pos++;
                        if (isCode) {
                            if (size == 4) {
                                size = DWCi_Ndi_decodeBASE64(code, body + start, size);
                                code[size] = '\0';
                                serverError = DWCi_Ndi_strToInt(code, size);
                            }
                            break;
                        }
                    }
                    if (serverError < 0) {
                        error = DWC_ND_ERROR_HTTP;
                    } else {
                        error = DWC_ND_ERROR_DLSERVER;
                    }
                }
            } else if (status <= 0 || status != 200) {
                error = DWC_ND_ERROR_HTTP;
            } else {
                switch (kind) {
                case DWC_ND_CBREASON_GETFILELISTNUM:
                    error = DWCi_Ndi_response_count(request, task) ? DWC_ND_ERROR_NONE : DWC_ND_ERROR_HTTP;
                    break;
                case DWC_ND_CBREASON_GETFILELIST:
                    error = DWCi_Ndi_response_list(request, task) ? DWC_ND_ERROR_NONE : DWC_ND_ERROR_HTTP;
                    break;
                case DWC_ND_CBREASON_GETFILE:
                    error = DWCi_Ndi_response_contents(request, task) ? DWC_ND_ERROR_NONE : DWC_ND_ERROR_HTTP;
                    break;
                }
            }
        }
        FreeTask(task);
        DWCi_Ndi_unprepareHttpCallback();
        if (callback != NULL) {
            callback(kind, error, serverError);
        }
        if (waiter != NULL) {
            DWCi_Ndi_kickThread(waiter);
        }
        break;
    case NHTTP_EVENT_BUFFER_FULL:
        error = DWC_ND_ERROR_BUFFULL;
        ((NdBufferFullEvent *)arg)->grow = FALSE;
        SetWifiError(error, serverError);
        if (callback != NULL) {
            callback(kind, error, serverError);
        }
        break;
    case NHTTP_EVENT_AFTER_BUFFER_FULL:
        break;
    }
    return 0;
}

static void FreeTask(NdTask *task) {
    if (task != NULL) {
        switch (task->kind) {
        case DWC_ND_CBREASON_GETFILELISTNUM:
            break;
        case DWC_ND_CBREASON_GETFILELIST:
            if (task->countParam != NULL) {
                sNdContext.free(task->countParam);
                task->countParam = NULL;
            }
            if (task->offsetParam != NULL) {
                sNdContext.free(task->offsetParam);
                task->offsetParam = NULL;
            }
            sNdContext.free(task->buffer);
            task->buffer = NULL;
            break;
        case DWC_ND_CBREASON_GETFILE:
            sNdContext.free(task->target.fileName);
            task->target.fileName = NULL;
            break;
        }
        sNdContext.free(task);
    }
}

// Ends a request that did not get sent, and its task
static void freeHttp(NdTask *task, int request) {
    ReleaseRequest(request);
    DWCi_Ndi_unprepareHttpCallback();
    FreeTask(task);
}

// Makes a request with the parameters that every call carries, and the task that follows it
static BOOL setupHttp(NdTask **taskOut, int *requestOut, const char *action, char *buffer, int bufferSize,
                      const char *logData) {
    NdTask *task;
    int request;
    char decoded[0x100];

    if (!DWCi_Ndi_getDSdesc(sNdContext.rhGameCodeParam, sNdUserIdParam)) {
        SetWifiError(DWC_ND_ERROR_FATAL, 0);
        return FALSE;
    }
    task = sNdContext.alloc(sizeof(NdTask), 4);
    if (task == NULL) {
        SetWifiError(DWC_ND_ERROR_ALLOC, 0);
        return FALSE;
    }
    task->callback = sNdContext.onResult;
    task->waiter = NULL;
    if (!DWCi_Ndi_prepareHttpCallback()) {
        FreeTask(task);
        SetWifiError(DWC_ND_ERROR_BUSY, 0);
        return FALSE;
    }
    if (!ReleaseRequest(sNdContext.request)) {
        return FALSE;
    }
    request = NHTTPCreateConnection(sNdContext.url, 1, buffer, bufferSize, req_callback, task);
    if (request == 0) {
        DWCi_Ndi_unprepareHttpCallback();
        FreeTask(task);
        SetWifiError(DWC_ND_ERROR_HTTP, 0);
        return FALSE;
    }
    *taskOut = task;
    *requestOut = request;
    if (NHTTPSetRootCA(request, sNdCertificates, 1) == -1) {
        goto fail;
    }
    if (NHTTPSetVerifyOption(request, 0) == -1) {
        goto fail;
    }
    if (NHTTPAddHeaderField(request, "User-Agent", sNdUserAgent) == -1) {
        goto fail;
    }
    if (NHTTPAddPostDataAscii(request, "gamecd", sNdContext.gameCodeParam) == -1) {
        goto fail;
    }
    if (NHTTPAddPostDataAscii(request, "rhgamecd", sNdContext.rhGameCodeParam) == -1) {
        goto fail;
    }
    if (NHTTPAddPostDataAscii(request, "passwd", sNdPasswordParam) == -1) {
        goto fail;
    }
    if (NHTTPAddPostDataAscii(request, "token", sNdContext.token) == -1) {
        goto fail;
    }
    if (NHTTPAddPostDataAscii(request, "userid", sNdUserIdParam) == -1) {
        goto fail;
    }
    if (NHTTPAddPostDataAscii(request, "macadr", sNdMacParam) == -1) {
        goto fail;
    }
    if (NHTTPAddPostDataAscii(request, "action", action) == -1) {
        goto fail;
    }
    if (sNdContext.attrs[0] != NULL && NHTTPAddPostDataAscii(request, "attr1", sNdContext.attrs[0]) == -1) {
        goto fail;
    }
    if (sNdContext.attrs[1] != NULL && NHTTPAddPostDataAscii(request, "attr2", sNdContext.attrs[1]) == -1) {
        goto fail;
    }
    if (sNdContext.attrs[2] != NULL && NHTTPAddPostDataAscii(request, "attr3", sNdContext.attrs[2]) == -1) {
        goto fail;
    }
    if (sNdContext.hasApInfo && NHTTPAddPostDataAscii(request, "apinfo", sNdApInfoParam) == -1) {
        goto fail;
    }
    if (logData != NULL) {
        int length = DWCi_Ndi_strlen(logData);

        if (length > 0x40) {
            SetWifiError(DWC_ND_ERROR_PARAM, 0);
            freeHttp(task, request);
            return FALSE;
        }
        DWCi_Ndi_memclr(sNdLogDataParam, 0x59);
        DWCi_Ndi_encodeBASE64(sNdLogDataParam, logData, length);
        if (NHTTPAddPostDataAscii(request, "optionallogdata", sNdLogDataParam) == -1) {
            goto fail;
        }
    }
    // The parameters decoded again, probably for a log that this build leaves out
    DWCi_Ndi_decodeBASE64(decoded, sNdContext.gameCodeParam, 0x80);
    DWCi_Ndi_decodeBASE64(decoded, sNdContext.rhGameCodeParam, 0x80);
    DWCi_Ndi_decodeBASE64(decoded, sNdPasswordParam, 0x80);
    DWCi_Ndi_decodeBASE64(decoded, sNdContext.token, 0x80);
    DWCi_Ndi_decodeBASE64(decoded, sNdUserIdParam, 0x80);
    DWCi_Ndi_decodeBASE64(decoded, sNdMacParam, 0x80);
    DWCi_Ndi_decodeBASE64(decoded, action, 0x80);
    DWCi_Ndi_decodeBASE64(decoded, sNdApInfoParam, 0x80);
    if (sNdContext.attrs[0] != NULL) {
        DWCi_Ndi_decodeBASE64(decoded, sNdContext.attrs[0], 0x80);
    }
    if (sNdContext.attrs[1] != NULL) {
        DWCi_Ndi_decodeBASE64(decoded, sNdContext.attrs[1], 0x80);
    }
    if (sNdContext.attrs[2] != NULL) {
        DWCi_Ndi_decodeBASE64(decoded, sNdContext.attrs[2], 0x80);
    }
    if (logData != NULL) {
        DWCi_Ndi_decodeBASE64(decoded, sNdLogDataParam, 0x80);
    }
    return TRUE;

fail:
    freeHttp(task, request);
    SetWifiError(DWC_ND_ERROR_ALLOC, 0);
    return FALSE;
}

// Sends the request, and keeps it as the one that is running
static BOOL runHttp(NdTask *task, int request) {
    DWCi_Ndi_lockGlobal();
    if (NHTTPStartConnection(request) != 0) {
        SetWifiError(DWC_ND_ERROR_HTTP, 0);
        DWCi_Ndi_unlockGlobal();
        return FALSE;
    }
    sNdContext.request = request;
    DWCi_Ndi_unlockGlobal();
    return TRUE;
}

// Asks the server for count files from offset
static BOOL DWCi_NdGetFileListAsync(DWCNdFileInfo *files, u32 offset, u32 count) {
    NdTask *task;
    int request;
    int bufferSize = count << 8;
    char *buffer;
    char *offsetParam;
    char *countParam;
    char number[12];
    int length;
    int countLength;

    if (bufferSize < 0x80) {
        bufferSize = 0x80;
    }
    buffer = sNdContext.alloc(bufferSize, 4);
    if (buffer == NULL) {
        SetWifiError(DWC_ND_ERROR_ALLOC, 0);
        return FALSE;
    }
    length = DWCi_Ndi_intToStr(number, offset);
    offsetParam = sNdContext.alloc(DWCi_Ndi_getEncodeLenBASE64(length) + 1, 4);
    if (offsetParam == NULL) {
        sNdContext.free(buffer);
        SetWifiError(DWC_ND_ERROR_ALLOC, 0);
        return FALSE;
    }
    offsetParam[DWCi_Ndi_encodeBASE64(offsetParam, number, length)] = '\0';
    countLength = DWCi_Ndi_intToStr(number, count);
    countParam = sNdContext.alloc(DWCi_Ndi_getEncodeLenBASE64(countLength) + 1, 4);
    if (countParam == NULL) {
        sNdContext.free(offsetParam);
        sNdContext.free(buffer);
        SetWifiError(DWC_ND_ERROR_ALLOC, 0);
        return FALSE;
    }
    countParam[DWCi_Ndi_encodeBASE64(countParam, number, countLength)] = '\0';
    if (!setupHttp(&task, &request, sActionList, buffer, bufferSize, NULL)) {
        sNdContext.free(countParam);
        sNdContext.free(offsetParam);
        sNdContext.free(buffer);
        return FALSE;
    }
    if (NHTTPAddPostDataAscii(request, "offset", offsetParam) != -1 &&
        NHTTPAddPostDataAscii(request, "num", countParam) != -1) {
        task->kind = DWC_ND_CBREASON_GETFILELIST;
        task->target.files = files;
        task->offsetParam = offsetParam;
        task->countParam = countParam;
        task->count = count;
        task->buffer = buffer;
        if (!runHttp(task, request)) {
            sNdContext.free(countParam);
            sNdContext.free(offsetParam);
            sNdContext.free(buffer);
            return FALSE;
        }
        return TRUE;
    }
    sNdContext.free(countParam);
    sNdContext.free(offsetParam);
    sNdContext.free(buffer);
    freeHttp(task, request);
    SetWifiError(DWC_ND_ERROR_ALLOC, 0);
    return FALSE;
}

// Asks the server for a file, whose name it takes from the list's entry
static BOOL DWCi_NdGetFileAsync(DWCNdFileInfo *file, void *buffer, u32 size, const char *logData) {
    NdTask *task;
    int request;
    int nameLength;
    int paramSize;
    char *nameParam;

    if (size < file->size) {
        SetWifiError(DWC_ND_ERROR_PARAM, 0);
        return FALSE;
    }
    nameLength = DWCi_Ndi_strlen(file->name);
    if (nameLength > 0x20) {
        SetWifiError(DWC_ND_ERROR_PARAM, 0);
        return FALSE;
    }
    if (nameLength > 0) {
        paramSize = DWCi_Ndi_getEncodeLenBASE64(nameLength);
    } else {
        paramSize = 0;
    }
    nameParam = sNdContext.alloc(paramSize + 1, 4);
    if (nameParam == NULL) {
        SetWifiError(DWC_ND_ERROR_ALLOC, 0);
        return FALSE;
    }
    if (!setupHttp(&task, &request, sActionContents, buffer, file->size, logData)) {
        sNdContext.free(nameParam);
        return FALSE;
    }
    if (paramSize > 0) {
        nameParam[DWCi_Ndi_encodeBASE64(nameParam, file->name, nameLength)] = '\0';
        if (NHTTPAddPostDataAscii(request, "contents", nameParam) == -1) {
            goto fail;
        }
    }
    task->kind = DWC_ND_CBREASON_GETFILE;
    task->target.fileName = nameParam;
    task->count = file->size;
    if (!runHttp(task, request)) {
        sNdContext.free(nameParam);
        return FALSE;
    }
    return TRUE;

fail:
    freeHttp(task, request);
    sNdContext.free(nameParam);
    SetWifiError(DWC_ND_ERROR_ALLOC, 0);
    return FALSE;
}

static BOOL DWCi_NdGetProgress(u32 *received, u32 *total) {
    if (!sNdContext.running) {
        return FALSE;
    }
    if (NHTTPGetProgress(sNdContext.request, (int *)received, (int *)total) == 0) {
        return TRUE;
    }
    return FALSE;
}

// The answer is a number, which goes where the task says
static BOOL DWCi_Ndi_response_count(int request, NdTask *task) {
    char *body;
    int size;
    int length = NHTTPGetBodyBuffer(request, &body, &size);

    if (length >= 0) {
        *task->target.number = DWCi_Ndi_strToInt(body, length);
        if (*task->target.number >= 0) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

// The answer of a file is as long as the list said
static BOOL DWCi_Ndi_response_contents(int request, NdTask *task) {
    char *body;
    int size;
    int length = NHTTPGetBodyBuffer(request, &body, &size);

    if (length == task->count) {
        return TRUE;
    }
    return FALSE;
}

// The answer is a line for each file, whose fields a tab ends, and a carriage return and a line feed end: the name, its
// text in base64, three parameters and the size
static BOOL DWCi_Ndi_response_list(int request, NdTask *task) {
    BOOL result = FALSE;
    char *body;
    int bodySize;
    char *header;
    int length;
    int fileIndex;
    int pos;
    int start;
    int field;
    BOOL sawReturn;
    DWCNdFileInfo *files;

    DWCi_Ndi_memclr(task->target.files, sizeof(DWCNdFileInfo) * task->count);
    if (NHTTPGetHeaderField(request, "Content-Length", &header) > 0 && header[0] == '0' && header[1] == '\0') {
        return TRUE;
    }
    length = NHTTPGetBodyBuffer(request, &body, &bodySize);
    if (length <= 0) {
        goto done;
    }
    fileIndex = 0;
    pos = 0;
    if (task->count <= 0) {
        goto done;
    }
    do {
        DWCNdFileInfo *file;

        if (pos >= length) {
            result = TRUE;
            break;
        }
        files = task->target.files;
        file = &files[fileIndex];
        result = FALSE;
        field = 0;
        start = pos;
        sawReturn = FALSE;
        for (; pos < length; pos++) {
            char c = body[pos];

            if (c == '\r') {
                sawReturn = TRUE;
                continue;
            }
            if (sawReturn) {
                if (c == '\n') {
                    if (field == 5) {
                        int size = pos - 1 - start;
                        int value;

                        if (size > 8) {
                            goto done;
                        }
                        value = DWCi_Ndi_strToInt(body + start, size);
                        if (value < 0) {
                            goto done;
                        }
                        file->size = value;
                    }
                    pos++;
                    result = field >= 5;
                    break;
                }
            } else if (c == '\t') {
                switch (field) {
                case 0: {
                    u32 size = pos - start;

                    if (size > 0x20) {
                        goto done;
                    }
                    DWCi_Ndi_memcpy(file->name, body + start, size);
                    break;
                }
                case 1: {
                    char decoded[0x66 + 1];
                    u32 size = pos - start;
                    u32 decodedSize;

                    if (DWCi_Ndi_getDecodeLenBASE64(size) > 0x66) {
                        goto done;
                    }
                    decodedSize = DWCi_Ndi_decodeBASE64(decoded, body + start, size);
                    if (decodedSize > 0x66) {
                        decodedSize = 0x66;
                    }
                    DWCi_Ndi_memcpy(file->explain, decoded, decodedSize);
                    file->explain[0x33 - 1] = 0;
                    break;
                }
                case 2: {
                    u32 size = pos - start;

                    if (size > 10) {
                        goto done;
                    }
                    DWCi_Ndi_memcpy(file->param1, body + start, size);
                    break;
                }
                case 3: {
                    u32 size = pos - start;

                    if (size > 10) {
                        goto done;
                    }
                    DWCi_Ndi_memcpy(file->param2, body + start, size);
                    break;
                }
                case 4: {
                    u32 size = pos - start;

                    if (size > 10) {
                        goto done;
                    }
                    DWCi_Ndi_memcpy(file->param3, body + start, size);
                    break;
                }
                case 5: {
                    int size = pos - start;
                    int value;

                    if (size > 8) {
                        goto done;
                    }
                    value = DWCi_Ndi_strToInt(body + start, size);
                    if (value < 0) {
                        goto done;
                    }
                    file->size = value;
                    break;
                }
                }
                field++;
                start = pos + 1;
            }
            sawReturn = FALSE;
        }
        fileIndex++;
    } while (fileIndex < task->count);
done:
    return result;
}

static BOOL DWCi_Ndi_initLockGlobal(void) {
    OS_InitMutex(&sNdAttrMutex);
    return TRUE;
}

static void DWCi_Ndi_exitLockGlobal(void) {
}

static void DWCi_Ndi_lockGlobal(void) {
    OS_LockMutex(&sNdAttrMutex);
}

static void DWCi_Ndi_unlockGlobal(void) {
    OS_UnlockMutex(&sNdAttrMutex);
}

static BOOL DWCi_Ndi_initHttpCallback(void) {
    OS_InitMutex(&sNdRequestMutex);
    sNdContext.requestActive = FALSE;
    return TRUE;
}

static void DWCi_Ndi_exitHttpCallback(void) {
}

// Says that a request is in flight, unless one already is
static BOOL DWCi_Ndi_prepareHttpCallback(void) {
    BOOL started;

    OS_LockMutex(&sNdRequestMutex);
    started = TRUE;
    if (sNdContext.requestActive) {
        started = FALSE;
    }
    if (started) {
        sNdContext.requestActive = TRUE;
    }
    OS_UnlockMutex(&sNdRequestMutex);
    return started;
}

static void DWCi_Ndi_unprepareHttpCallback(void) {
    OS_LockMutex(&sNdRequestMutex);
    sNdContext.requestActive = FALSE;
    OS_UnlockMutex(&sNdRequestMutex);
}

static void DWCi_Ndi_kickThread(OSMessageQueue *queue) {
    OS_SendMessage(queue, NULL, 0);
}

// Writes the console's address in base64, as the text of its hexadecimal digits
static void DWCi_Ndi_getMacAddress(char *dest) {
    u8 mac[6];
    char digits[12];
    int i;

    OS_GetMacAddress(mac);
    for (i = 0; i < 6; i++) {
        DWCi_Ndi_hexToStrFigure(digits + i * 2, mac[i], 2);
    }
    DWCi_Ndi_encodeBASE64(dest, digits, 12);
}

static int DWCi_Ndi_strlen(const char *text) {
    return strlen(text);
}

static int DWCi_Ndi_strncmp(const char *a, const char *b, int size) {
    return strncmp(a, b, size);
}

static void DWCi_Ndi_memclr(void *dest, int size) {
    MI_CpuFill8(dest, 0, size);
}

static void DWCi_Ndi_memcpy(void *dest, const void *src, int size) {
    MI_CpuCopy8(src, dest, size);
}

static int DWCi_Ndi_strToInt(const char *text, int size) {
    return NHTTPi_strToInt(text, size);
}

static int DWCi_Ndi_intToStr(char *dest, u32 value) {
    return NHTTPi_intToStr(dest, value);
}

// Writes a number of up to 17 digits, with zeros in front to make minDigits of them
static int DWCi_Ndi_intToStrFigure(char *dest, u64 value, int minDigits) {
    // The powers of ten from 10^16 down to 1
    static const u64 powersOfTen[] = {
        10000000000000000ULL,
        1000000000000000ULL,
        100000000000000ULL,
        10000000000000ULL,
        1000000000000ULL,
        100000000000ULL,
        10000000000ULL,
        1000000000ULL,
        100000000ULL,
        10000000ULL,
        1000000ULL,
        100000ULL,
        10000ULL,
        1000ULL,
        100ULL,
        10ULL,
    };
    int length = 0;
    BOOL started;
    int i;

    if (value >= 100000000000000000ULL) {
        return -1;
    }
    for (i = 0, started = FALSE; i < 16; i++) {
        u64 power = powersOfTen[i];
        u32 digit;

        if (value >= power) {
            digit = value / power;
            value -= digit * power;
            digit += '0';
            started = TRUE;
            dest[length] = digit;
            length++;
        } else if (started || 17 - i <= minDigits) {
            dest[length] = '0';
            length++;
        }
    }
    dest[length] = value + '0';
    return length + 1;
}

// Writes a value as digits lowercase hexadecimal digits
static int DWCi_Ndi_hexToStrFigure(char *dest, u32 value, int digits) {
    int i;
    int length = 0;

    if (digits > 8) {
        return -1;
    }
    for (i = 1; i <= digits; i++) {
        u32 nibble = (value >> ((digits - i) * 4)) & 0xf;

        if (nibble < 10) {
            dest[length] = nibble + '0';
        } else {
            dest[length] = nibble + 'a' - 10;
        }
        length++;
    }
    return length;
}
