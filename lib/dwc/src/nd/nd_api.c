#include "dwc/nd.h"

// Public download wrapper nd_api.c (descriptive filename). The split before DWCi_Ndi_freeAttr and ownership of the
// engine context and action constants are inferred from the code, data and BSS layout; the ROM does not name the engine
// source file
//
// NitroDWC's download library (DWC_Nd), which the games use to fetch files from the Nintendo Wi-Fi Connection's
// download server (Mystery Gift's gifts). The ROM names none of it: see nd.h for where the names come from.
//
// A call starts a request to the server with HTTP's POST; the parameters travel in base64 (a variant that pads with
// '*', and puts '.' and '-' for 62 and 63) and the answer comes back as a status and a body. A thread waits until the
// Wi-Fi connection is up and sets the library up; NdReportResults then reports each result to the game's callback.

#include "dwc/dwc.h"
#include "dwc/nhttp.h"
#include "nd_internal.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// Where the library is: 0 idle, 1 ready, 2 busy with a call, 3 a call ended well, 4 one failed
enum {
    ND_PHASE_IDLE,
    ND_PHASE_READY,
    ND_PHASE_BUSY,
    ND_PHASE_DONE,
    ND_PHASE_FAILED,
};

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

static void *DWCi_NdAlloc(u32 size, int align);
static void DWCi_NdFree(void *ptr);
static void DWCi_NdCleanupCallback(void);
static void DWCi_SVLTokenWaitThread(void *arg);
static void DWCi_NdCallback(DWCNdCallbackReason reason, DWCNdError error, int code);
NdContext sNdContext;
static OSMutex sNdMutex;
static NdState sDwcNdState;
// Base64 padding and encoded actions; padding and count are retained in the original but unused
static const char pad[1] = { '*' };
static const char sActionCount[] = "Y291bnQ*";
const char sActionList[] = "bGlzdA**";
const char sActionContents[] = "Y29udGVudHM*";
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
