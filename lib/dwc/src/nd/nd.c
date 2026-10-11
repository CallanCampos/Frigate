#include "dwc/nd.h"

#include "dwc/dwc.h"
#include "dwc/nhttp.h"
#include "nd_internal.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// NitroDWC's download library (DWC_Nd), which the games use to fetch files from the Nintendo Wi-Fi Connection's download
// server (Mystery Gift's gifts). The ROM names none of it: see nd.h for where the names come from.
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

// The thread's memory: what DWC_NdInitAsync was given, the connection's information, the thread's stack and the thread
typedef struct NdWork {
    char gameCode[0x20];
    char password[0x20];
    // Whether the thread set the library up
    BOOL started;
    u8 unk44[4];
    char host[0x41];
    char authToken[0x12f];
    u8 stack[0x800];
    OSThread thread;
} NdWork;

// The call in progress and the result that NdReportResults gives the game
typedef struct NdState {
    DWCNdCallback callback;
    BOOL notified;
    int code;
    int error;
    int reason;
    int phase;
    NdWork *work;
} NdState;

// The library's settings, and the request that is running
typedef struct NdContext {
    // Records a result, and is given to the requests
    DWCNdCallback onResult;
    NHttpAllocFunc alloc;
    BOOL running;
    BOOL hasApInfo;
    int request;
    NHttpFreeFunc free;
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

static void *NdAlloc(u32 size, int align);
static void NdFree(void *ptr);
static void NdFreeWork(void);
static void ThreadMain(void *arg);
static void NdSetResult(u32 reason, u32 error, int code);
static void FreeAttrs(void);
static BOOL SetAttr(int index, const char *text);
static BOOL SetAttrs(const char *attr1, const char *attr2, const char *attr3);
static int CopyNonZero(u8 *dest, const char *src, int size);
static int DecodeChar(u32 c);
static u32 DecodedSize(int size);
static int NdEncodedSize(int size);
static int Base64Encode(char *dest, const char *src, int srcSize);
static int Base64Decode(char *dest, const char *src, int srcSize);
static char *StrDup(const char *text);
static BOOL Setup(NHttpAllocFunc alloc, NHttpFreeFunc free, const char *url, const char *token, const char *gameCode,
                  const char *password, DWCNdCallback onResult);
static void OnCleanedUp(void);
static void BeginCleanup(void (*onCleanedUp)(void));
static BOOL ReleaseRequest(int request);
static void SetWifiError(int error, int serverError);
static BOOL CancelRequest(void);
static BOOL MakeApInfoParam(char *dest);
static BOOL MakeUserIdParams(char *gameCode, char *userId);
static void MakeUserAgent(char *dest);
static int OnHttpEvent(int request, int event, void *arg);
static void FreeTask(NdTask *task);
static void AbortTask(NdTask *task, int request);
static BOOL CreateRequest(NdTask **taskOut, int *requestOut, const char *action, char *buffer, int bufferSize,
                          const char *logData);
static BOOL NdSendRequest(NdTask *task, int request);
static BOOL RequestFileList(DWCNdFileInfo *files, u32 offset, u32 count);
static BOOL RequestFile(DWCNdFileInfo *file, void *buffer, u32 size, const char *logData);
static BOOL GetRequestProgress(u32 *received, u32 *total);
static BOOL ParseNumberAnswer(int request, NdTask *task);
static BOOL CheckFileAnswer(int request, NdTask *task);
static BOOL ParseFileListAnswer(int request, NdTask *task);
static BOOL InitAttrLock(void);
static void EndAttrLock(void);
static void LockAttr(void);
static void UnlockAttr(void);
static BOOL InitRequestLock(void);
static void EndRequestLock(void);
static BOOL BeginRequest(void);
static void EndRequest(void);
static void WakeWaiter(OSMessageQueue *queue);
static void MakeMacParam(char *dest);
static int StrLen(const char *text);
static int StrNCmp(const char *a, const char *b, int size);
static void MemZero(void *dest, int size);
static void MemCopy(void *dest, const void *src, int size);
static int ParseInt(const char *text, int size);
static int FormatInt(char *dest, u32 value);
static int FormatDecimal64(char *dest, u64 value, int minDigits);
static int FormatHex(char *dest, u32 value, int digits);

static char sNdLogDataParam[0x59];
static NdApInfo sNdApInfo;
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
static const char sBase64Pad[1] = {'*'};
static const char sActionCount[] = "Y291bnQ*";
static const char sActionList[] = "bGlzdA**";
static const char sActionContents[] = "Y29udGVudHM*";

static void *NdAlloc(u32 size, int align) {
    return func_020586f4(10, size, align);
}

static void NdFree(void *ptr) {
    func_02058728(10, ptr, 0);
}

// Ends the thread and gives its memory back, and tells NdReportResults that the library has ended
static void NdFreeWork(void) {
    func_0207a800(&sDwcNdState.work->thread);
    NdFree(sDwcNdState.work);
    sDwcNdState.work = NULL;
    OS_LockMutex(&sNdMutex);
    sDwcNdState.phase = ND_PHASE_DONE;
    sDwcNdState.reason = ND_CBREASON_CLEANED_UP;
    OS_UnlockMutex(&sNdMutex);
}

// Waits for the Wi-Fi connection, then sets the library up with the connection's host
static void ThreadMain(void *arg) {
    char url[0x100];
    char token[0x190];

    for (;;) {
        switch (func_ov011_0215dea8()) {
        case 3: {
            NdWork *work = sDwcNdState.work;

            if (strlen(work->host) + strlen("https:///download") > 0xff) {
                NdSetResult(DWC_ND_CBREASON_INITIALIZE, ND_ERROR_FATAL, -1);
                return;
            }
            func_0207a2c0(url, 0x100, "https://%s/download", work->host);
            work = sDwcNdState.work;
            if (func_020580b8(work->authToken, strlen(work->authToken), token, 0x190) < 0) {
                NdSetResult(DWC_ND_CBREASON_INITIALIZE, ND_ERROR_FATAL, -1);
                return;
            }
            if (!Setup(NdAlloc, NdFree, url, token, sDwcNdState.work->gameCode, sDwcNdState.work->password, NdSetResult)) {
                NdSetResult(DWC_ND_CBREASON_INITIALIZE, ND_ERROR_FATAL, -1);
                return;
            }
            sDwcNdState.work->started = TRUE;
            NdSetResult(DWC_ND_CBREASON_INITIALIZE, DWC_ND_ERROR_NONE, -1);
            return;
        }
        case 4: {
            int code;
            func_02058390(&code);
            func_020584cc(13, code);
            NdSetResult(DWC_ND_CBREASON_INITIALIZE, DWC_ND_ERROR_HTTP, -1);
            return;
        }
        case 5:
            NdSetResult(DWC_ND_CBREASON_INITIALIZE, DWC_ND_ERROR_CANCELED, -1);
            return;
        default:
            func_0207aa04(100);
            break;
        }
    }
}

// Records the result of a call, for NdReportResults to give the game
static void NdSetResult(u32 reason, u32 error, int code) {
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

BOOL DWC_NdInitAsync(DWCNdCallback callback, const char *gameCode, const char *password) {
    if (func_020584b0()) {
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
    sDwcNdState.work = NdAlloc(sizeof(NdWork), 32);
    if (sDwcNdState.work == NULL) {
        SetWifiError(DWC_ND_ERROR_ALLOC, 0);
        return FALSE;
    }
    sys_memset(sDwcNdState.work, 0, sizeof(NdWork));
    sDwcNdState.work->started = FALSE;
    strncpy(sDwcNdState.work->gameCode, gameCode, 0x1f);
    strncpy(sDwcNdState.work->password, password, 0x1f);
    if (!func_ov011_0215de44("9000", sDwcNdState.work->unk44)) {
        NdFree(sDwcNdState.work);
        sDwcNdState.work = NULL;
        return FALSE;
    }
    scheduler_init_thread(&sDwcNdState.work->thread, ThreadMain, NULL, sDwcNdState.work->stack + sizeof(sDwcNdState.work->stack),
                          sizeof(sDwcNdState.work->stack), 16);
    scheduler_start_thread(&sDwcNdState.work->thread);
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
    if (!sDwcNdState.work->started) {
        NdFreeWork();
    } else {
        BeginCleanup(NdFreeWork);
    }
    return TRUE;
}

BOOL DWC_NdSetAttr(const char *attr1, const char *attr2, const char *attr3) {
    if (func_020584b0()) {
        return FALSE;
    }
    return SetAttrs(attr1, attr2, attr3);
}

BOOL DWC_NdGetFileListAsync(DWCNdFileInfo *files, u32 offset, u32 count) {
    if (func_020584b0()) {
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
    sys_memset(files, 0, sizeof(DWCNdFileInfo) * count);
    if (RequestFileList(files, offset, count)) {
        return TRUE;
    }
    OS_LockMutex(&sNdMutex);
    sDwcNdState.phase = ND_PHASE_READY;
    OS_UnlockMutex(&sNdMutex);
    return FALSE;
}

BOOL DWC_NdGetFileAsync(DWCNdFileInfo *file, void *buffer, u32 size) {
    BOOL result;

    if (func_020584b0()) {
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
    if (RequestFile(file, buffer, size, NULL)) {
        return TRUE;
    }
    OS_LockMutex(&sNdMutex);
    sDwcNdState.phase = ND_PHASE_READY;
    OS_UnlockMutex(&sNdMutex);
    return result;
}

BOOL DWC_NdCancelAsync(void) {
    if (func_020584b0()) {
        return FALSE;
    }
    OS_LockMutex(&sNdMutex);
    if (sDwcNdState.phase != ND_PHASE_BUSY) {
        OS_UnlockMutex(&sNdMutex);
        return FALSE;
    }
    OS_UnlockMutex(&sNdMutex);
    if (CancelRequest()) {
        return TRUE;
    }
    return FALSE;
}

BOOL DWC_NdGetProgress(u32 *received, u32 *contentLength) {
    if (func_020584b0()) {
        return FALSE;
    }
    OS_LockMutex(&sNdMutex);
    if (sDwcNdState.phase != ND_PHASE_BUSY) {
        OS_UnlockMutex(&sNdMutex);
        return FALSE;
    }
    OS_UnlockMutex(&sNdMutex);
    return GetRequestProgress(received, contentLength);
}

// Gives the three attributes' memory back
static void FreeAttrs(void) {
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
static BOOL SetAttr(int index, const char *text) {
    int size;
    int length;
    char *attr;

    if (*text != '\0') {
        if (StrLen(text) > 10) {
            SetWifiError(ND_ERROR_BAD_PARAMETER, 0);
            return FALSE;
        }
        size = StrLen(text);
        if (size > 10) {
            SetWifiError(ND_ERROR_BAD_PARAMETER, 0);
            return FALSE;
        }
        attr = sNdContext.alloc(NdEncodedSize(size) + 1, 4);
        sNdContext.attrs[index] = attr;
        if (attr == NULL) {
            SetWifiError(DWC_ND_ERROR_ALLOC, 0);
            return FALSE;
        }
        length = Base64Encode(attr, text, size);
        sNdContext.attrs[index][length] = '\0';
    }
    return TRUE;
}

static BOOL SetAttrs(const char *attr1, const char *attr2, const char *attr3) {
    LockAttr();
    FreeAttrs();
    if (SetAttr(0, attr1) && SetAttr(1, attr2) && SetAttr(2, attr3)) {
        UnlockAttr();
        return TRUE;
    }
    FreeAttrs();
    UnlockAttr();
    return FALSE;
}

// Copies up to size characters, stopping to give zeros at a zero, and returns how many were not zero
static int CopyNonZero(u8 *dest, const char *src, int size) {
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
static int DecodeChar(u32 c) {
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

static u32 DecodedSize(int size) {
    return (size >> 2) * 3;
}

static int NdEncodedSize(int size) {
    return (size + 2) / 3 * 4;
}

// Encodes the characters up to the first zero, and returns the length
static int Base64Encode(char *dest, const char *src, int srcSize) {
    static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-";
    u8 chunk[3];
    int length = 0;
    int n;

    n = CopyNonZero(chunk, src, srcSize > 3 ? 3 : srcSize);
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
        n = CopyNonZero(chunk, src, srcSize > 3 ? 3 : srcSize);
    }
    *dest = '\0';
    return length;
}

// Decodes at most srcSize characters, and returns the length of the bytes
static int Base64Decode(char *dest, const char *src, int srcSize) {
    u8 in[4];
    char bytes[3];
    int length = 0;
    int n;
    int value = 0;

    n = CopyNonZero(in, src, 4);
    while (n > 0 && value >= 0) {
        int i;

        bytes[0] = 0;
        bytes[1] = 0;
        bytes[2] = 0;
        value = DecodeChar(in[0]);
        if (value >= 0) {
            bytes[0] = value << 2;
            value = DecodeChar(in[1]);
            if (value >= 0) {
                bytes[0] |= value >> 4;
                bytes[1] = value << 4;
                value = DecodeChar(in[2]);
                if (value >= 0) {
                    bytes[1] |= value >> 2;
                    bytes[2] = value << 6;
                    value = DecodeChar(in[3]);
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
        n = CopyNonZero(in, src, 4);
    }
    *dest = '\0';
    return length;
}

static char *StrDup(const char *text) {
    int length = StrLen(text);
    char *copy = sNdContext.alloc(length + 1, 4);

    if (copy != NULL) {
        MemCopy(copy, text, length);
        copy[length] = '\0';
    }
    return copy;
}

// Sets the library up: the memory, the parameters that every request carries, and the HTTP client
static BOOL Setup(NHttpAllocFunc alloc, NHttpFreeFunc free, const char *url, const char *token, const char *gameCode,
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
    MakeMacParam(sNdMacParam);
    MakeUserAgent(sNdUserAgent);
    sNdContext.url = StrDup(url);
    if (sNdContext.url != NULL) {
        sNdContext.token = StrDup(token);
        if (sNdContext.token != NULL) {
            sNdContext.gameCodeParam[Base64Encode(sNdContext.gameCodeParam, gameCode, 4)] = '\0';
            sNdPasswordParam[Base64Encode(sNdPasswordParam, password, 16)] = '\0';
            if (InitRequestLock() && InitAttrLock()) {
                if (NHTTPStartup(alloc, free, 17) != -1) {
                    sNdContext.hasApInfo = MakeApInfoParam(sNdApInfoParam);
                    sNdContext.running = TRUE;
                    return TRUE;
                }
                EndAttrLock();
            }
            sNdContext.free(sNdContext.token);
            sNdContext.free(sNdContext.url);
            SetWifiError(ND_ERROR_FATAL, 0);
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

static void OnCleanedUp(void) {
    FreeAttrs();
    EndAttrLock();
    EndRequestLock();
    sNdContext.free(sNdContext.token);
    sNdContext.free(sNdContext.url);
    sNdContext.onCleanedUp();
}

// Ends the library's request and the HTTP client, and then calls the function
static void BeginCleanup(void (*onCleanedUp)(void)) {
    sNdContext.running = FALSE;
    sNdContext.onCleanedUp = onCleanedUp;
    ReleaseRequest(sNdContext.request);
    NHTTPCleanupAsync(OnCleanedUp);
}

// Destroys a request; the answer is FALSE when it could not
static BOOL ReleaseRequest(int request) {
    BOOL result = TRUE;

    LockAttr();
    if (request != 0) {
        NHTTPGetConnectionState(request);
        if (NHTTPDestroyConnection(request) != 0) {
            result = FALSE;
        } else {
            request = 0;
        }
    }
    UnlockAttr();
    return result;
}

// Gives the Wi-Fi library the error that a call of the download library ended with
static void SetWifiError(int error, int serverError) {
    switch (error) {
    case DWC_ND_ERROR_NONE:
    case DWC_ND_ERROR_CANCELED:
        break;
    case DWC_ND_ERROR_ALLOC:
        func_020584cc(9, -31001);
        break;
    case DWC_ND_ERROR_HTTP:
        func_020584cc(14, -31020);
        break;
    case DWC_ND_ERROR_BUFFULL:
        func_020584cc(13, -31040);
        break;
    case DWC_ND_ERROR_DLSERVER:
        func_020584cc(13, -31000 - serverError);
        break;
    case ND_ERROR_BAD_PARAMETER:
        func_020584cc(13, -31040);
        break;
    case ND_ERROR_FATAL:
        func_020584cc(9, -31009);
        break;
    case DWC_ND_ERROR_BUSY:
        func_020584cc(13, -31010);
        break;
    default:
        func_020584cc(9, -31009);
        break;
    }
}

// Cancels the request when one is running, and says whether it did
static BOOL CancelRequest(void) {
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
    if (!BeginRequest()) {
        LockAttr();
        request = sNdContext.request;
        UnlockAttr();
        if (NHTTPCancelConnection(request) == 0) {
            canceled = TRUE;
        }
        return canceled;
    }
    EndRequest();
    return canceled;
}

// Writes the information of the access point that the requests carry, in base64: a "00:0000000-00" with the type and the
// spot's digits in place
static BOOL MakeApInfoParam(char *dest) {
    char info[13] = "00:0000000-00";

    if (func_0205b29c(&sNdApInfo)) {
        if (sNdApInfo.type >= 0 && sNdApInfo.type < 10) {
            info[1] = '0' + sNdApInfo.type;
        } else if (sNdApInfo.type >= 10 && sNdApInfo.type < 100) {
            FormatInt(info, sNdApInfo.type);
        }
        if (sNdApInfo.type == 4 || sNdApInfo.type == 8) {
            if (sNdApInfo.unk4 >= 0 && sNdApInfo.unk4 < 10) {
                info[3] = '0' + sNdApInfo.unk4;
            }
            MemCopy(info + 4, sNdApInfo.spot, 9);
        }
        Base64Encode(dest, info, 13);
        return TRUE;
    }
    return FALSE;
}

// Writes the console's identifier and the game's code in base64, for the requests
static BOOL MakeUserIdParams(char *gameCode, char *userId) {
    NdConsoleIds ids;
    char digits[13];

    MemZero(&ids, sizeof(ids));
    func_02056a90(&ids);
    if (ids.id == 0) {
        return FALSE;
    }
    if (FormatDecimal64(digits, ids.id, 13) < 0) {
        return FALSE;
    }
    Base64Encode(userId, digits, 13);
    Base64Encode(gameCode, (const char *)0x2fffe0c, 4);
    return TRUE;
}

// The certificate that the server's identity is checked against: its key is 1024 bits, with the exponent 65537
static u8 sNdCertExponent[3] = {1, 0, 1};
static NdCertificate sNdCertificate;
static NdCertificate *sNdCertificates[1] = {&sNdCertificate};
static char sNdCertName[80];
static u8 sNdCertModulus[128];
static NdCertificate sNdCertificate = {
    sNdCertName,
    128,
    sNdCertModulus,
    3,
    sNdCertExponent,
};
static char sNdCertName[80] = "US, Washington, Nintendo of America Inc, NOA, Nintendo CA, ca@noa.nintendo.com";
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

// Writes "Nitro WiFi SDK/5.3"
static void MakeUserAgent(char *dest) {
    func_0207a2c0(dest, 0x21, "Nitro WiFi SDK/%d.%d", 5, 3);
}

// The HTTP client's callback of every request: the answer of the server decides what the call ends with. The answer is
// a status, and with 302 the body carries "returncd=" and the server's code in base64
static int OnHttpEvent(int request, int event, void *arg) {
    int bodySize;
    char *statusText;
    char *body;
    char code[4];
    NdTask *task;
    int kind;
    OSMessageQueue *waiter;
    int serverError;
    DWCNdCallback callback;
    int error;

    task = NHTTPGetUserParam(request);
    kind = task->kind;
    callback = task->callback;
    waiter = task->waiter;
    serverError = -1;

    switch (event) {
    case NHTTP_EVENT_ANSWER:
        if (NHTTPGetConnectionError(request) == 8) {
            error = DWC_ND_ERROR_CANCELED;
        } else if (NHTTPGetHeaderField(request, "HTTPSTATUSCODE", &statusText) != 3) {
            error = DWC_ND_ERROR_HTTP;
        } else {
            int result = DWC_ND_ERROR_HTTP;
            int status = ParseInt(statusText, 3);

            if (status == 302) {
                int length;
                int pos;
                int start;
                int nameLength;
                int valueLength;
                BOOL isCode = FALSE;

                length = NHTTPGetBodyBuffer(request, &body, &bodySize);
                if (length < 0) {
                    error = result;
                } else {
                    pos = 0;
                    while (pos < length) {
                        start = pos;
                        while (body[pos] != '=' && pos < length) {
                            pos++;
                        }
                        nameLength = pos - start;
                        pos++;
                        if (nameLength == 8) {
                            isCode = StrNCmp(body + start, "returncd", 8) == 0;
                        }
                        start = pos;
                        while (body[pos] != '&' && body[pos] != '\r' && body[pos] != '\0' && pos < length) {
                            pos++;
                        }
                        valueLength = pos - start;
                        pos++;
                        if (isCode) {
                            if (valueLength == 4) {
                                int size = Base64Decode(code, body + start, 4);

                                code[size] = '\0';
                                serverError = ParseInt(code, size);
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
            } else if (status > 0 && status == 200) {
                switch (kind) {
                case DWC_ND_CBREASON_GETFILELISTNUM:
                    if (ParseNumberAnswer(request, task)) {
                        result = DWC_ND_ERROR_NONE;
                    }
                    break;
                case DWC_ND_CBREASON_GETFILELIST:
                    if (ParseFileListAnswer(request, task)) {
                        result = DWC_ND_ERROR_NONE;
                    }
                    break;
                case DWC_ND_CBREASON_GETFILE:
                    if (CheckFileAnswer(request, task)) {
                        result = DWC_ND_ERROR_NONE;
                    }
                    break;
                }
                error = result;
            } else {
                error = DWC_ND_ERROR_HTTP;
            }
        }
        FreeTask(task);
        EndRequest();
        if (callback != NULL) {
            callback(kind, error, serverError);
        }
        if (waiter != NULL) {
            WakeWaiter(waiter);
        }
        break;
    case NHTTP_EVENT_AFTER_BUFFER_FULL:
        break;
    case NHTTP_EVENT_BUFFER_FULL:
        ((NdBufferFullEvent *)arg)->grow = FALSE;
        error = DWC_ND_ERROR_BUFFULL;
        SetWifiError(error, serverError);
        if (callback != NULL) {
            callback(kind, error, serverError);
        }
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
static void AbortTask(NdTask *task, int request) {
    ReleaseRequest(request);
    EndRequest();
    FreeTask(task);
}

// Makes a request with the parameters that every call carries, and the task that follows it
static BOOL CreateRequest(NdTask **taskOut, int *requestOut, const char *action, char *buffer, int bufferSize,
                          const char *logData) {
    NdTask *task;
    int request;
    char decoded[0x100];

    if (!MakeUserIdParams(sNdContext.rhGameCodeParam, sNdUserIdParam)) {
        SetWifiError(ND_ERROR_FATAL, 0);
        return FALSE;
    }
    task = sNdContext.alloc(sizeof(NdTask), 4);
    if (task == NULL) {
        SetWifiError(DWC_ND_ERROR_ALLOC, 0);
        return FALSE;
    }
    task->callback = sNdContext.onResult;
    task->waiter = NULL;
    if (!BeginRequest()) {
        FreeTask(task);
        SetWifiError(DWC_ND_ERROR_BUSY, 0);
        return FALSE;
    }
    if (!ReleaseRequest(sNdContext.request)) {
        return FALSE;
    }
    request = NHTTPCreateConnection(sNdContext.url, 1, buffer, bufferSize, OnHttpEvent, task);
    if (request == 0) {
        EndRequest();
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
        int length = StrLen(logData);

        if (length > 0x40) {
            SetWifiError(ND_ERROR_BAD_PARAMETER, 0);
            AbortTask(task, request);
            return FALSE;
        }
        MemZero(sNdLogDataParam, 0x59);
        Base64Encode(sNdLogDataParam, logData, length);
        if (NHTTPAddPostDataAscii(request, "optionallogdata", sNdLogDataParam) == -1) {
            goto fail;
        }
    }
    // The parameters decoded again, probably for a log that this build leaves out
    Base64Decode(decoded, sNdContext.gameCodeParam, 0x80);
    Base64Decode(decoded, sNdContext.rhGameCodeParam, 0x80);
    Base64Decode(decoded, sNdPasswordParam, 0x80);
    Base64Decode(decoded, sNdContext.token, 0x80);
    Base64Decode(decoded, sNdUserIdParam, 0x80);
    Base64Decode(decoded, sNdMacParam, 0x80);
    Base64Decode(decoded, action, 0x80);
    Base64Decode(decoded, sNdApInfoParam, 0x80);
    if (sNdContext.attrs[0] != NULL) {
        Base64Decode(decoded, sNdContext.attrs[0], 0x80);
    }
    if (sNdContext.attrs[1] != NULL) {
        Base64Decode(decoded, sNdContext.attrs[1], 0x80);
    }
    if (sNdContext.attrs[2] != NULL) {
        Base64Decode(decoded, sNdContext.attrs[2], 0x80);
    }
    if (logData != NULL) {
        Base64Decode(decoded, sNdLogDataParam, 0x80);
    }
    return TRUE;

fail:
    AbortTask(task, request);
    SetWifiError(DWC_ND_ERROR_ALLOC, 0);
    return FALSE;
}

// Sends the request, and keeps it as the one that is running
static BOOL NdSendRequest(NdTask *task, int request) {
    LockAttr();
    if (NHTTPStartConnection(request) != 0) {
        SetWifiError(DWC_ND_ERROR_HTTP, 0);
        UnlockAttr();
        return FALSE;
    }
    sNdContext.request = request;
    UnlockAttr();
    return TRUE;
}

// Asks the server for count files from offset
static BOOL RequestFileList(DWCNdFileInfo *files, u32 offset, u32 count) {
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
    length = FormatInt(number, offset);
    offsetParam = sNdContext.alloc(NdEncodedSize(length) + 1, 4);
    if (offsetParam == NULL) {
        sNdContext.free(buffer);
        SetWifiError(DWC_ND_ERROR_ALLOC, 0);
        return FALSE;
    }
    offsetParam[Base64Encode(offsetParam, number, length)] = '\0';
    countLength = FormatInt(number, count);
    countParam = sNdContext.alloc(NdEncodedSize(countLength) + 1, 4);
    if (countParam == NULL) {
        sNdContext.free(offsetParam);
        sNdContext.free(buffer);
        SetWifiError(DWC_ND_ERROR_ALLOC, 0);
        return FALSE;
    }
    countParam[Base64Encode(countParam, number, countLength)] = '\0';
    if (!CreateRequest(&task, &request, sActionList, buffer, bufferSize, NULL)) {
        sNdContext.free(countParam);
        sNdContext.free(offsetParam);
        sNdContext.free(buffer);
        return FALSE;
    }
    if (NHTTPAddPostDataAscii(request, "offset", offsetParam) != -1
        && NHTTPAddPostDataAscii(request, "num", countParam) != -1) {
        task->kind = DWC_ND_CBREASON_GETFILELIST;
        task->target.files = files;
        task->offsetParam = offsetParam;
        task->countParam = countParam;
        task->count = count;
        task->buffer = buffer;
        if (!NdSendRequest(task, request)) {
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
    AbortTask(task, request);
    SetWifiError(DWC_ND_ERROR_ALLOC, 0);
    return FALSE;
}

// Asks the server for a file, whose name it takes from the list's entry
static BOOL RequestFile(DWCNdFileInfo *file, void *buffer, u32 size, const char *logData) {
    NdTask *task;
    int request;
    int nameLength;
    int paramSize;
    char *nameParam;

    if (size < file->size) {
        SetWifiError(ND_ERROR_BAD_PARAMETER, 0);
        return FALSE;
    }
    nameLength = StrLen(file->fileName);
    if (nameLength > 0x20) {
        SetWifiError(ND_ERROR_BAD_PARAMETER, 0);
        return FALSE;
    }
    if (nameLength > 0) {
        paramSize = NdEncodedSize(nameLength);
    } else {
        paramSize = 0;
    }
    nameParam = sNdContext.alloc(paramSize + 1, 4);
    if (nameParam == NULL) {
        SetWifiError(DWC_ND_ERROR_ALLOC, 0);
        return FALSE;
    }
    if (!CreateRequest(&task, &request, sActionContents, buffer, file->size, logData)) {
        sNdContext.free(nameParam);
        return FALSE;
    }
    if (paramSize > 0) {
        nameParam[Base64Encode(nameParam, file->fileName, nameLength)] = '\0';
        if (NHTTPAddPostDataAscii(request, "contents", nameParam) == -1) {
            goto fail;
        }
    }
    task->kind = DWC_ND_CBREASON_GETFILE;
    task->target.fileName = nameParam;
    task->count = file->size;
    if (!NdSendRequest(task, request)) {
        sNdContext.free(nameParam);
        return FALSE;
    }
    return TRUE;

fail:
    AbortTask(task, request);
    sNdContext.free(nameParam);
    SetWifiError(DWC_ND_ERROR_ALLOC, 0);
    return FALSE;
}

static BOOL GetRequestProgress(u32 *received, u32 *total) {
    if (!sNdContext.running) {
        return FALSE;
    }
    if (NHTTPGetProgress(sNdContext.request, (int *)received, (int *)total) == 0) {
        return TRUE;
    }
    return FALSE;
}

// The answer is a number, which goes where the task says
static BOOL ParseNumberAnswer(int request, NdTask *task) {
    char *body;
    int size;
    int length = NHTTPGetBodyBuffer(request, &body, &size);

    if (length >= 0) {
        *task->target.number = ParseInt(body, length);
        if (*task->target.number >= 0) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

// The answer of a file is as long as the list said
static BOOL CheckFileAnswer(int request, NdTask *task) {
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
static BOOL ParseFileListAnswer(int request, NdTask *task) {
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

    MemZero(task->target.files, sizeof(DWCNdFileInfo) * task->count);
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
                        value = ParseInt(body + start, size);
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
                    MemCopy(file->fileName, body + start, size);
                    break;
                }
                case 1: {
                    char decoded[0x66 + 1];
                    u32 size = pos - start;
                    u32 decodedSize;

                    if (DecodedSize(size) > 0x66) {
                        goto done;
                    }
                    decodedSize = Base64Decode(decoded, body + start, size);
                    if (decodedSize > 0x66) {
                        decodedSize = 0x66;
                    }
                    MemCopy(file->description, decoded, decodedSize);
                    file->description[0x33 - 1] = 0;
                    break;
                }
                case 2: {
                    u32 size = pos - start;

                    if (size > 10) {
                        goto done;
                    }
                    MemCopy(file->param1, body + start, size);
                    break;
                }
                case 3: {
                    u32 size = pos - start;

                    if (size > 10) {
                        goto done;
                    }
                    MemCopy(file->param2, body + start, size);
                    break;
                }
                case 4: {
                    u32 size = pos - start;

                    if (size > 10) {
                        goto done;
                    }
                    MemCopy(file->param3, body + start, size);
                    break;
                }
                case 5: {
                    int size = pos - start;
                    int value;

                    if (size > 8) {
                        goto done;
                    }
                    value = ParseInt(body + start, size);
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

static BOOL InitAttrLock(void) {
    OS_InitMutex(&sNdAttrMutex);
    return TRUE;
}

static void EndAttrLock(void) {
}

static void LockAttr(void) {
    OS_LockMutex(&sNdAttrMutex);
}

static void UnlockAttr(void) {
    OS_UnlockMutex(&sNdAttrMutex);
}

static BOOL InitRequestLock(void) {
    OS_InitMutex(&sNdRequestMutex);
    sNdContext.requestActive = FALSE;
    return TRUE;
}

static void EndRequestLock(void) {
}

// Says that a request is in flight, unless one already is
static BOOL BeginRequest(void) {
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

static void EndRequest(void) {
    OS_LockMutex(&sNdRequestMutex);
    sNdContext.requestActive = FALSE;
    OS_UnlockMutex(&sNdRequestMutex);
}

static void WakeWaiter(OSMessageQueue *queue) {
    OS_SendMessage(queue, NULL, 0);
}

// Writes the console's address in base64, as the text of its hexadecimal digits
static void MakeMacParam(char *dest) {
    u8 mac[6];
    char digits[12];
    int i;

    func_0207c33c(mac);
    for (i = 0; i < 6; i++) {
        FormatHex(digits + i * 2, mac[i], 2);
    }
    Base64Encode(dest, digits, 12);
}

static int StrLen(const char *text) {
    return strlen(text);
}

static int StrNCmp(const char *a, const char *b, int size) {
    return strncmp(a, b, size);
}

static void MemZero(void *dest, int size) {
    sys_memset(dest, 0, size);
}

static void MemCopy(void *dest, const void *src, int size) {
    sys_memcpy(src, dest, size);
}

static int ParseInt(const char *text, int size) {
    return NHTTPi_strToInt(text, size);
}

static int FormatInt(char *dest, u32 value) {
    return NHTTPi_intToStr(dest, value);
}

// Writes a number of up to 17 digits, with zeros in front to make minDigits of them
static int FormatDecimal64(char *dest, u64 value, int minDigits) {
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
    BOOL started = FALSE;
    int i;

    if (value >= 100000000000000000ULL) {
        return -1;
    }
    for (i = 0; i < 16; i++) {
        u64 power = powersOfTen[i];
        int digit;

        if (value >= power) {
            digit = value / power;
            value -= digit * power;
            digit += '0';
            started = TRUE;
        } else if (started || 17 - i <= minDigits) {
            digit = '0';
        } else {
            continue;
        }
        dest[length] = digit;
        length++;
    }
    dest[length] = value + '0';
    return length + 1;
}

// Writes a value as digits lowercase hexadecimal digits
static int FormatHex(char *dest, u32 value, int digits) {
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
