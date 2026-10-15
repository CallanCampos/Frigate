#include "ghttp_internal.h"

// dwc_ghttp: DWC's use of the GameSpy HTTP client: the root certificate of its hosts, the starting and ending of the
// client, and a request with the callbacks of the game and the error that a result turns to

// The heap that DWC's requests are made in
#define DWC_GHTTP_HEAP 6

// What the game gives to be done with the result of a request
typedef void (*DWCGHTTPCompletedFunc)(char *buffer, int length, int result, void *param);
typedef void (*DWCGHTTPProgressFunc)(int state, const char *buffer, int length, int received, int total, void *param);

typedef struct DWCGHTTPParam {
    void *param;
    DWCGHTTPCompletedFunc completed;
    DWCGHTTPProgressFunc progress;
    int unk_0C;
    BOOL freeBuffer;
} DWCGHTTPParam;

// A request of DWC, in a list: what the game gave, the buffer that DWC made for it and the request's number
typedef struct DWCGHTTPEntry DWCGHTTPEntry;
struct DWCGHTTPEntry {
    DWCGHTTPParam param;
    char *buffer;
    int request;
    DWCGHTTPEntry *next;
};

extern u8 data_ov011_021866c0[];

int DWC_GetGHTTPDataEx(const char *url, int bufferSize, BOOL freeBuffer, GHIPost **post, DWCGHTTPProgressFunc progress,
                       DWCGHTTPCompletedFunc completed, void *param);
int DWC_GetGHTTPData(const char *url, int bufferSize, BOOL freeBuffer, DWCGHTTPProgressFunc progress,
                     DWCGHTTPCompletedFunc completed, void *param);
void DWC_CancelGHTTPRequest(int request);
int DWCi_HandleGHTTPError(int error);
static DWCGHTTPEntry *DWCi_NewGHTTPEntry(const DWCGHTTPParam *param);
static void DWCi_RemoveDWCGHTTPParamEntry(DWCGHTTPEntry *entry);
static DWCGHTTPEntry *DWCi_FindGHTTPEntry(int request);
static void DWCi_FreeAllGHTTPEntries(void);

// The root certificate that DWC's hosts are checked against
static void *sRootCerts[1] = {data_ov011_021866c0};
static GHICertList sDWCRootCAList = {sRootCerts, 1};

static DWCGHTTPEntry *sGHTTPEntries;
static int sGHTTPStartCount;

BOOL DWCi_SetGHTTPRootCA(const char *url) {
    if (ghttpSetRootCAList(url, &sDWCRootCAList) == 1) {
        return TRUE;
    }
    return FALSE;
}

BOOL DWCi_RemoveGHTTPRootCA(const char *url) {
    if (ghttpRemoveRootCAList(url) == 1) {
        return TRUE;
    }
    return FALSE;
}

BOOL DWC_InitGHTTP(int unused) {
    ghttpStartup();
    sGHTTPStartCount++;
    return TRUE;
}

BOOL DWC_ShutdownGHTTP(void) {
    if (sGHTTPStartCount <= 0) {
        return TRUE;
    }
    ghttpCleanup();
    sGHTTPStartCount--;
    if (sGHTTPStartCount == 0) {
        DWCi_FreeAllGHTTPEntries();
    }
    return TRUE;
}

BOOL DWC_ProcessGHTTP(void) {
    if (func_020584b0()) {
        return FALSE;
    }
    ghttpThink();
    return TRUE;
}

// Tells the game that its request has ended, and frees what DWC made for it. Returns whether the client may free the
// buffer that it received into
static BOOL DWCi_GHTTPCompleted(int request, int result, char *buffer, int length, void *param) {
    DWCGHTTPEntry *entry = param;
    DWCGHTTPCompletedFunc completed;
    BOOL freeBuffer;

    completed = entry->param.completed;
    freeBuffer = entry->param.freeBuffer;
    if (completed != NULL) {
        char *data;
        int dataLength;

        if (result == 0) {
            data = buffer;
            dataLength = length;
        } else {
            if (result == 0x12) {
            } else {
                DWCi_HandleGHTTPError(result);
            }
            data = NULL;
            dataLength = 0;
        }
        completed(data, dataLength, result, entry->param.param);
    }
    if (result != 0 || freeBuffer == TRUE) {
        if (entry != NULL && entry->buffer != NULL) {
            func_02058728(DWC_GHTTP_HEAP, entry->buffer, 0);
        } else {
            freeBuffer = TRUE;
        }
    }
    DWCi_RemoveDWCGHTTPParamEntry(entry);
    return freeBuffer != FALSE ? TRUE : FALSE;
}

static void DWCi_GHTTPProgress(int request, int state, const char *buffer, int length, int received, int total,
                               void *param) {
    DWCGHTTPEntry *entry = param;
    DWCGHTTPProgressFunc progress = entry->param.progress;

    if (progress != NULL) {
        progress(state, buffer, length, received, total, entry->param.param);
    }
}

int DWC_GetGHTTPData(const char *url, int bufferSize, BOOL freeBuffer, DWCGHTTPProgressFunc progress,
                     DWCGHTTPCompletedFunc completed, void *param) {
    return DWC_GetGHTTPDataEx(url, bufferSize, freeBuffer, NULL, progress, completed, param);
}

// Starts the request for a URL, with a buffer of this size (the game gets it in the callbacks). Returns the request, or
// an error that is below 0
int DWC_GetGHTTPDataEx(const char *url, int bufferSize, BOOL freeBuffer, GHIPost **post,
                       DWCGHTTPProgressFunc progress, DWCGHTTPCompletedFunc completed, void *param) {
    char *buffer = NULL;
    DWCGHTTPParam entryParam;
    DWCGHTTPEntry *entry;
    int request;

    if (func_020584b0()) {
        return -1;
    }
    entryParam.param = param;
    entryParam.completed = completed;
    entryParam.progress = progress;
    entryParam.freeBuffer = freeBuffer;
    entry = DWCi_NewGHTTPEntry(&entryParam);
    if (entry == NULL) {
        request = -5;
        DWCi_HandleGHTTPError(request);
        completed(NULL, 0, request, param);
        return request;
    }

    if (bufferSize > 0) {
        buffer = func_020586e4(DWC_GHTTP_HEAP, bufferSize);
        if (buffer == NULL) {
            request = -5;
            DWCi_HandleGHTTPError(request);
            completed(NULL, 0, request, param);
            DWCi_RemoveDWCGHTTPParamEntry(entry);
            return request;
        }
        entry->buffer = buffer;
    }

    request = ghttpGetExA(url, NULL, buffer, bufferSize, post == NULL ? NULL : *post, FALSE, FALSE,
                          progress == NULL ? NULL : DWCi_GHTTPProgress, DWCi_GHTTPCompleted, entry);
    if (request < 0) {
        DWCi_HandleGHTTPError(request);
        completed(NULL, 0, request, param);
        if (entry->buffer != NULL) {
            func_02058728(DWC_GHTTP_HEAP, entry->buffer, 0);
        }
        DWCi_RemoveDWCGHTTPParamEntry(entry);
    }
    entry->request = request;
    ghttpSetMaxRecvTime(request, 1);
    return request;
}

void DWC_CancelGHTTPRequest(int request) {
    DWCGHTTPEntry *entry;

    ghttpCancelRequest(request);
    entry = DWCi_FindGHTTPEntry(request);
    if (entry != NULL) {
        if (entry->buffer != NULL) {
            func_02058728(DWC_GHTTP_HEAP, entry->buffer, 0);
        }
        DWCi_RemoveDWCGHTTPParamEntry(entry);
    }
}

// Sets the error of the Wi-Fi library from the result of a request (or the error that making it gave)
int DWCi_HandleGHTTPError(int error) {
    int type = 7;
    int code = -98000;

    if (error == 0) {
        return 0;
    }
    switch (error) {
    case -7:
        code -= 800;
        break;
    case -6:
        code -= 810;
        break;
    case -5:
        code -= 840;
        break;
    case -4:
    case -3:
    case -2:
        code -= 820;
        break;
    case -1:
        code -= 830;
        break;
    case 1:
    case 20:
        type = 9;
        code -= 1;
        break;
    case 2:
        code -= 840;
        break;
    case 3:
        code -= 850;
        break;
    case 4:
        code -= 30;
        break;
    case 5:
        code -= 50;
        break;
    case 6:
        code -= 20;
        break;
    case 7:
        code -= 860;
        break;
    case 8:
    case 9:
    case 10:
        code -= 870;
        break;
    case 11:
    case 12:
        code -= 20;
        break;
    case 13:
    case 14:
        code -= 880;
        break;
    case 15:
        code -= 890;
        break;
    case 16:
        code -= 900;
        break;
    case 17:
        code -= 910;
        break;
    default:
        break;
    }
    func_020584cc(type, code);
    return error;
}

// Adds a request to the list
static DWCGHTTPEntry *DWCi_NewGHTTPEntry(const DWCGHTTPParam *param) {
    DWCGHTTPEntry *entry = func_020586e4(DWC_GHTTP_HEAP, sizeof(DWCGHTTPEntry));

    if (entry == NULL) {
        return NULL;
    }
    entry->param = *param;
    entry->next = NULL;
    entry->buffer = NULL;
    if (sGHTTPEntries == NULL) {
        sGHTTPEntries = entry;
        return entry;
    }
    entry->next = sGHTTPEntries;
    sGHTTPEntries = entry;
    return entry;
}

// Takes a request out of the list and frees it
static void DWCi_RemoveDWCGHTTPParamEntry(DWCGHTTPEntry *entry) {
    DWCGHTTPEntry *previous = sGHTTPEntries;
    DWCGHTTPEntry *current;

    if (previous == NULL) {
        return;
    }
    if (previous == entry) {
        DWCGHTTPEntry *next = previous->next;

        func_02058728(DWC_GHTTP_HEAP, previous, 0);
        sGHTTPEntries = next;
        return;
    }
    current = previous->next;
    if (current != NULL) {
        do {
            if (current != entry) {
                previous = current;
            } else {
                DWCGHTTPEntry *next = previous->next;

                previous->next = next->next;
                func_02058728(DWC_GHTTP_HEAP, next, 0);
                return;
            }
            current = current->next;
        } while (current != NULL);
    }
}

static DWCGHTTPEntry *DWCi_FindGHTTPEntry(int request) {
    DWCGHTTPEntry *entry = sGHTTPEntries;

    while (entry != NULL && entry->request != request) {
        entry = entry->next;
    }
    return entry;
}

static void DWCi_FreeAllGHTTPEntries(void) {
    DWCGHTTPEntry *entry = sGHTTPEntries;

    if (entry != NULL) {
        do {
            DWCGHTTPEntry *current = entry;

            entry = entry->next;
            if (current->buffer != NULL) {
                func_02058728(DWC_GHTTP_HEAP, current->buffer, 0);
            }
            func_02058728(DWC_GHTTP_HEAP, current, 0);
        } while (entry != NULL);
    }
    sGHTTPEntries = NULL;
}
