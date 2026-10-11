// Game Sync's download of a file from the Dream World's server, through NitroDWC's download library, with a timeout.
// The name is the ROM's string, from GFL_HeapAllocate's calls, and the work's name, _pDLWork, from an assert.
// Function names are ours.

#include "types.h"
#include "app/gsync/gsync_download.h"
#include "dwc/nd.h"
#include "gfl/dwc_rap.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "nitro/os.h"

// How long a call may take, in frames
#define GSYNC_DOWNLOAD_TIMEOUT 3600
// The buffer's room beyond the file's size
#define GSYNC_DOWNLOAD_BUFFER_EXTRA 0x40

struct GSyncDownload {
    DWCNdFileInfo file;
    // Set by the download library's callback once a call ends, with its error
    BOOL done;
    u32 error;
    void *buffer;
    u32 bufferSize;
    u32 unkC0;
    BOOL cleanedUp;
    BOOL cancelled;
    // Counts the frames of the call since it started, while it is not 0
    int timer;
    BOOL timedOut;
};

static void GSyncDownload_OnNdEvent(u32 reason, u32 error);
static u32 GSyncDownload_OnDwcEvent(void *work, int a1, int event, int a3);
static BOOL GSyncDownload_UpdateTimer(GSyncDownload *dl);

static GSyncDownload *_pDLWork;

static void GSyncDownload_OnNdEvent(u32 reason, u32 error) {
    switch (error) {
    case DWC_ND_ERROR_NONE:
        break;
    case DWC_ND_ERROR_HTTP:
        break;
    case DWC_ND_ERROR_CANCELED:
        _pDLWork->cancelled = TRUE;
        break;
    case DWC_ND_ERROR_FATAL:
        func_02011d20();
        break;
    }
    _pDLWork->done = TRUE;
    _pDLWork->error = error;
}

GSyncDownload *GSyncDownload_Create(HeapID heapId, u32 size) {
    GSyncDownload *dl = GFL_HeapAllocate(heapId, sizeof(GSyncDownload), TRUE, "gsync_download.c", 136);

    GFL_ASSERT(!_pDLWork);
    _pDLWork = dl;
    dl->buffer = GFL_HeapAllocate(heapId, size + GSYNC_DOWNLOAD_BUFFER_EXTRA, FALSE, "gsync_download.c", 140);
    dl->bufferSize = size;
    return dl;
}

void GSyncDownload_Free(GSyncDownload *dl) {
    if (_pDLWork != NULL) {
        DWCRap_SetEventFunc(NULL, NULL);
        GFL_HeapFree(dl->buffer);
        GFL_HeapFree(dl);
        _pDLWork = NULL;
    }
}

void *GSyncDownload_GetBuffer(GSyncDownload *dl) {
    return dl->buffer;
}

static u32 GSyncDownload_OnDwcEvent(void *work, int a1, int event, int a3) {
    GSyncDownload *dl = work;

    if (!dl->cleanedUp && func_ov189_021a57dc()) {
        dl->cleanedUp = TRUE;
    }
    if (!dl->done) {
        return TRUE;
    }
    return FALSE;
}

BOOL GSyncDownload_Init(GSyncDownload *dl) {
    dl->done = FALSE;
    if (!func_ov189_021a5674(GSyncDownload_OnNdEvent, "IRAO", "WX9x7Zh6J3aBC4zQ")) {
        return FALSE;
    }
    DWCRap_SetEventFunc(GSyncDownload_OnDwcEvent, dl);
    return TRUE;
}

BOOL GSyncDownload_IsSucceeded(GSyncDownload *dl) {
    if (_pDLWork->done && dl->error == DWC_ND_ERROR_NONE) {
        return TRUE;
    }
    return FALSE;
}

BOOL GSyncDownload_IsFailed(GSyncDownload *dl) {
    if (dl->timedOut && dl->cancelled) {
        return TRUE;
    }
    if (_pDLWork->done && dl->error != DWC_ND_ERROR_NONE) {
        return TRUE;
    }
    return FALSE;
}

BOOL GSyncDownload_SetAttr(GSyncDownload *dl, const char *attr1, int attr2) {
    BOOL result = FALSE;
    char buf[20];

    dl->done = FALSE;
    OS_SNPrintf(buf, sizeof(buf), "%d", attr2);
    if (func_ov189_021a5830(attr1, buf, "")) {
        result = TRUE;
    }
    return result;
}

void GSyncDownload_ResetTimer(GSyncDownload *dl) {
    dl->timer = 0;
    dl->timedOut = FALSE;
}

// Counts a frame of the call, and cancels it once it takes too long. Returns whether it goes on
static BOOL GSyncDownload_UpdateTimer(GSyncDownload *dl) {
    if (dl->timedOut) {
        return FALSE;
    }
    if (dl->timer != 0) {
        dl->timer++;
        if (dl->timer > GSYNC_DOWNLOAD_TIMEOUT) {
            dl->timedOut = TRUE;
            if (!func_ov189_021a5938()) {
                dl->cancelled = TRUE;
            }
            return FALSE;
        }
    }
    return TRUE;
}

BOOL GSyncDownload_GetFileList(GSyncDownload *dl) {
    BOOL result = TRUE;

    dl->done = FALSE;
    dl->timer = 1;
    dl->timedOut = FALSE;
    if (!func_ov189_021a5850(&dl->file, 0, 1)) {
        result = FALSE;
    }
    return result;
}

BOOL GSyncDownload_GetFile(GSyncDownload *dl) {
    BOOL result = TRUE;

    dl->done = FALSE;
    dl->timer = 1;
    dl->timedOut = FALSE;
    if (!func_ov189_021a58c8(&dl->file, dl->buffer, dl->bufferSize + GSYNC_DOWNLOAD_BUFFER_EXTRA)) {
        result = FALSE;
    }
    return result;
}

BOOL GSyncDownload_Cleanup(GSyncDownload *dl) {
    if (!dl->cleanedUp) {
        dl->done = FALSE;
        if (!func_ov189_021a57dc()) {
            return FALSE;
        }
    }
    return TRUE;
}

void GSyncDownload_Main(GSyncDownload *dl) {
    if (GSyncDownload_UpdateTimer(dl)) {
        func_ov189_021a5768();
    }
}

u32 GSyncDownload_GetFileSize(GSyncDownload *dl) {
    return dl->file.size;
}
