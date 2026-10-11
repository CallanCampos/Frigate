#include "types.h"
#include "app/net_common_download.h"
#include "dwc/nd.h"
#include "field/delivery_beacon.h"
#include "gfl/dwc_rap.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/std.h"
#include "system/game_data.h"

// The states of a download that wait for the download library
#define DOWNLOAD_WAIT 100
#define DOWNLOAD_WAIT_CLEANUP 200
// A wait that takes longer than this many frames times out
#define DOWNLOAD_TIMEOUT 7200

typedef BOOL (*NetCommonDownloadHandler)(NetCommonDownload *download, u32 *state);

// What the download library is doing
typedef struct {
    DWCNdFileInfo *files;
    // Where a wait goes on to
    u32 nextState;
    BOOL cancel;
    BOOL cancelled;
    int count;
    u32 percent;
    u32 received;
    u32 contentLength;
    u32 selected;
    u32 timeout;
    u32 unk28;
    BOOL initialized;
} NetCommonDownloadSession;

// The command that downloads one file by its name, which nothing starts
typedef struct {
    const char *name;
    u32 size;
} NetCommonDownloadFileArgs;

struct NetCommonDownload {
    NetCommonDownloadParam param;
    // Whether the download is started and its command runs
    BOOL started;
    u32 status;
    u32 update;
    u32 state;
    NetCommonDownloadSession session;
    void *beacon;
    BOOL stopBeacon;
    NetCommonDownloadHandler handler;
    union {
        NetCommonDownloadFileListArgs fileList;
        NetCommonDownloadFileArgs file;
        NetCommonDownloadBeaconArgs beacon;
        u8 raw[0x20];
    } args;
    u8 data[];
};

// What the download library's callback last gave
typedef struct {
    BOOL ready;
    BOOL done;
    u32 error;
} NetCommonDownloadNdState;

static const void *NetCommonDownload_GetArgs(NetCommonDownload *download);
static BOOL NetCommonDownload_CanRequest(NetCommonDownload *download, u32 command);
static BOOL NetCommonDownload_Idle(NetCommonDownload *download, u32 *state);
static BOOL NetCommonDownload_DownloadFileList(NetCommonDownload *download, u32 *state);
static BOOL NetCommonDownload_DownloadFile(NetCommonDownload *download, u32 *state);
static BOOL NetCommonDownload_ReceiveBeacons(NetCommonDownload *download, u32 *state);
static u32 NetCommonDownload_OnDwcEvent(void *work, int a1, int event, int a3);
static void NetCommonDownload_AllocFileList(NetCommonDownloadSession *session,
                                            const NetCommonDownloadFileListArgs *args, HeapID heapId);
static void NetCommonDownload_AllocFile(NetCommonDownloadSession *session, const NetCommonDownloadFileArgs *args,
                                        HeapID heapId);
static void NetCommonDownload_FreeSession(NetCommonDownloadSession *session);
static void NetCommonDownload_Wait(NetCommonDownloadSession *session, u32 *state, u32 nextState);
static void NetCommonDownload_Cleanup(NetCommonDownloadSession *session, u32 error, u32 *state, u32 nextState,
                                      u32 failState);
static void NetCommonDownload_OnNdEvent(u32 reason, u32 error, int serverError);
static BOOL NetCommonDownload_IsZero(const u32 *data, u32 size);

static const u8 sModeCommands[2][5] = {
    { 1, 1, 1, 0, 0 },
    { 0, 0, 0, 1, 1 },
};

static const u8 sKindCommands[4][5] = {
    { 1, 1, 0, 1, 0 },
    { 0, 0, 1, 0, 0 },
    { 0, 0, 1, 0, 0 },
    { 0, 0, 0, 0, 1 },
};

static NetCommonDownloadNdState sNdState;

NetCommonDownload *NetCommonDownload_Create(const NetCommonDownloadParam *param) {
    NetCommonDownload *download = GFL_HeapAllocate(param->heapId, sizeof(NetCommonDownload) + param->dataSize, TRUE,
                                                   "net_common_download.c", 147);

    download->param = *param;
    download->handler = NetCommonDownload_Idle;
    return download;
}

void NetCommonDownload_Free(NetCommonDownload *download) {
    GFL_HeapFree(download);
}

void NetCommonDownload_Start(NetCommonDownload *download) {
    if (download->update == 0) {
        download->started = TRUE;
    }
}

void NetCommonDownload_Step(NetCommonDownload *download) {
    if (download->started) {
        if (download->handler(download, &download->state)) {
            download->state = 0;
            download->handler = NetCommonDownload_Idle;
        }
    }
    download->started = FALSE;
}

BOOL NetCommonDownload_Request(NetCommonDownload *download, u32 command, const void *args) {
    BOOL allowed = NetCommonDownload_CanRequest(download, command);

    if (!download->started) {
        return FALSE;
    }
    if (allowed) {
        switch (command) {
        case NET_COMMON_DOWNLOAD_FILE_LIST:
            sys_memcpy(args, &download->args, sizeof(NetCommonDownloadFileListArgs));
            download->handler = NetCommonDownload_DownloadFileList;
            break;
        case NET_COMMON_DOWNLOAD_COMMAND_1:
            break;
        case NET_COMMON_DOWNLOAD_CANCEL:
            download->session.cancelled = TRUE;
            break;
        case NET_COMMON_DOWNLOAD_BEACON:
            sys_memcpy(args, &download->args, sizeof(NetCommonDownloadBeaconArgs));
            download->handler = NetCommonDownload_ReceiveBeacons;
            break;
        case NET_COMMON_DOWNLOAD_STOP_BEACON:
            download->stopBeacon = TRUE;
            break;
        }
        return TRUE;
    }
    return FALSE;
}

u32 NetCommonDownload_GetKind(NetCommonDownload *download) {
    if (download->handler == NetCommonDownload_Idle) {
        return NET_COMMON_DOWNLOAD_KIND_IDLE;
    }
    if (download->handler == NetCommonDownload_DownloadFileList) {
        return NET_COMMON_DOWNLOAD_KIND_FILE_LIST;
    }
    if (download->handler == NetCommonDownload_DownloadFile) {
        return NET_COMMON_DOWNLOAD_KIND_FILE;
    }
    if (download->handler == NetCommonDownload_ReceiveBeacons) {
        return NET_COMMON_DOWNLOAD_KIND_BEACON;
    }
    return NET_COMMON_DOWNLOAD_KIND_IDLE;
}

u32 NetCommonDownload_GetStatus(NetCommonDownload *download) {
    return download->status;
}

void *NetCommonDownload_GetData(NetCommonDownload *download) {
    return download->data;
}

u32 NetCommonDownload_Update(NetCommonDownload *download) {
    if (GameData_CheckEventsPaused(download->param.gameData)) {
        return NET_COMMON_DOWNLOAD_UPDATE_RUNNING;
    }
    switch (download->param.mode) {
    case 0:
        if (download->session.initialized) {
            return NET_COMMON_DOWNLOAD_UPDATE_RUNNING;
        }
        if (func_02042788()) {
            switch (DWCRapCommon_CheckError(TRUE, FALSE)) {
            case 0:
                download->update = NET_COMMON_DOWNLOAD_UPDATE_RUNNING;
                break;
            case 1:
                download->update = NET_COMMON_DOWNLOAD_UPDATE_ENDED;
                break;
            case 2:
                download->update = NET_COMMON_DOWNLOAD_UPDATE_ERROR;
                break;
            case 3:
            case 4:
                break;
            }
        }
        break;
    case 1:
        download->update = NET_COMMON_DOWNLOAD_UPDATE_RUNNING;
        break;
    }
    return download->update;
}

void NetCommonDownload_ResetUpdate(NetCommonDownload *download) {
    download->update = NET_COMMON_DOWNLOAD_UPDATE_RUNNING;
}

static BOOL NetCommonDownload_Idle(NetCommonDownload *download, u32 *state) {
    return FALSE;
}

// Downloads a file of the server: offers it the files of the list, picks one and receives it
static BOOL NetCommonDownload_DownloadFileList(NetCommonDownload *download, u32 *state) {
    NetCommonDownloadSession *session = &download->session;
    const NetCommonDownloadFileListArgs *args = NetCommonDownload_GetArgs(download);
    u32 i;

    switch (*state) {
    case 0:
        DWCRap_SetEventFunc(NetCommonDownload_OnDwcEvent, session);
        NetCommonDownload_AllocFileList(session, args, download->param.heapId);
        download->status = NET_COMMON_DOWNLOAD_STATUS_NONE;
        if (!DWC_NdInitAsync(NetCommonDownload_OnNdEvent, "IRAO", "WX9x7Zh6J3aBC4zQ")) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_ERROR;
            *state = 9;
            break;
        }
        session->initialized = TRUE;
        NetCommonDownload_Wait(session, state, 1);
        break;
    case 1:
        if (session->cancelled) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_CANCELLED;
            NetCommonDownload_Cleanup(session, 3, state, 7, 7);
            break;
        }
        if (!DWC_NdSetAttr(args->attr1, args->attr2, args->attr3)) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_ERROR;
            NetCommonDownload_Cleanup(session, 4, state, 9, 9);
            break;
        }
        *state = 3;
        break;
    case 3:
        if (!DWC_NdGetFileListAsync(session->files, 0, args->fileCount)) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_ERROR;
            NetCommonDownload_Cleanup(session, 4, state, 9, 9);
            break;
        }
        NetCommonDownload_Wait(session, state, 2);
        break;
    case 2:
        session->count = 0;
        for (i = 0; i < args->fileCount; i++) {
            if (!NetCommonDownload_IsZero((u32 *)&session->files[i], sizeof(DWCNdFileInfo))) {
                session->count++;
            }
        }
        *state = 4;
        break;
    case 4:
        if (session->count == 1) {
            for (i = 0; i < args->fileCount; i++) {
                if (!NetCommonDownload_IsZero((u32 *)&session->files[i], sizeof(DWCNdFileInfo))) {
                    session->selected = i;
                    break;
                }
            }
        } else if (session->count > 1) {
            session->selected = args->select(session->files, args->fileCount);
        } else {
            download->status = NET_COMMON_DOWNLOAD_STATUS_NO_FILE;
            NetCommonDownload_Cleanup(session, 1, state, 8, 8);
            break;
        }
        *state = 5;
        break;
    case 5:
        if (!DWC_NdGetFileAsync(&session->files[session->selected], download->data,
                                 session->files[session->selected].size)) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_ERROR;
            NetCommonDownload_Cleanup(session, 4, state, 9, 9);
            break;
        }
        session->percent = 0;
        sNdState.ready = FALSE;
        sNdState.error = 0;
        session->timeout = 0;
        *state = 6;
        break;
    case 6:
        NdReportResults();
        if (session->timeout++ > DOWNLOAD_TIMEOUT) {
            func_020424ac(0, 0, 0, 1013);
            download->status = NET_COMMON_DOWNLOAD_STATUS_TIMEOUT;
            NetCommonDownload_Cleanup(session, 3, state, 7, 7);
        }
        if (!sNdState.ready) {
            if (session->cancel) {
                download->status = NET_COMMON_DOWNLOAD_STATUS_CANCELLED;
                NetCommonDownload_Cleanup(session, 3, state, 7, 7);
                break;
            }
            if (DWC_NdGetProgress(&session->received, &session->contentLength) == TRUE) {
                u32 percent = session->received * 100 / session->contentLength;

                if (session->percent != percent) {
                    session->percent = percent;
                }
            }
        } else if (sNdState.error != 0) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_ERROR;
            NetCommonDownload_Cleanup(session, 4, state, 9, 9);
        } else if (!session->cancelled) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_DONE;
            NetCommonDownload_Cleanup(session, 1, state, 8, 8);
        } else {
            download->status = NET_COMMON_DOWNLOAD_STATUS_CANCELLED;
            NetCommonDownload_Cleanup(session, 3, state, 8, 8);
        }
        break;
    case 7:
        DWC_NdCancelAsync();
        *state = 8;
        break;
    case 8:
        *state = 9;
        break;
    case 9:
        NetCommonDownload_FreeSession(session);
        DWCRap_SetEventFunc(NULL, NULL);
        session->initialized = FALSE;
        return TRUE;
    case DOWNLOAD_WAIT:
        NdReportResults();
        if (session->timeout++ > DOWNLOAD_TIMEOUT) {
            func_020424ac(0, 0, 0, 1013);
            download->status = NET_COMMON_DOWNLOAD_STATUS_TIMEOUT;
            NetCommonDownload_Cleanup(session, 3, state, 9, 9);
        }
        if (sNdState.ready) {
            sNdState.ready = FALSE;
            if (sNdState.error != 0) {
                download->status = NET_COMMON_DOWNLOAD_STATUS_ERROR;
                NetCommonDownload_Cleanup(session, 3, state, 9, 9);
                break;
            }
            *state = session->nextState;
            break;
        }
        if (session->cancel) {
            session->cancelled = TRUE;
        }
        break;
    case DOWNLOAD_WAIT_CLEANUP:
        NdReportResults();
        if (session->timeout++ > DOWNLOAD_TIMEOUT) {
            func_020424ac(0, 0, 0, 1013);
            *state = session->nextState;
        }
        if (sNdState.done) {
            sNdState.done = FALSE;
            *state = session->nextState;
            break;
        }
        if (session->cancel) {
            session->cancelled = TRUE;
        }
        break;
    }
    return FALSE;
}

// Downloads the file of a name
static BOOL NetCommonDownload_DownloadFile(NetCommonDownload *download, u32 *state) {
    NetCommonDownloadSession *session = &download->session;
    const NetCommonDownloadFileArgs *args = NetCommonDownload_GetArgs(download);

    switch (*state) {
    case 0:
        DWCRap_SetEventFunc(NetCommonDownload_OnDwcEvent, session);
        NetCommonDownload_AllocFile(session, args, download->param.heapId);
        download->status = NET_COMMON_DOWNLOAD_STATUS_NONE;
        if (!DWC_NdInitAsync(NetCommonDownload_OnNdEvent, "IRAO", "WX9x7Zh6J3aBC4zQ")) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_ERROR;
            *state = 6;
            break;
        }
        session->initialized = TRUE;
        NetCommonDownload_Wait(session, state, 1);
        break;
    case 1:
        if (session->cancelled) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_CANCELLED;
            NetCommonDownload_Cleanup(session, 3, state, 4, 4);
            break;
        }
        if (!DWC_NdSetAttr("", "", "")) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_ERROR;
            NetCommonDownload_Cleanup(session, 4, state, 6, 6);
            break;
        }
        *state = 2;
        break;
    case 2:
        if (!DWC_NdGetFileAsync(session->files, download->data, session->files->size)) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_ERROR;
            NetCommonDownload_Cleanup(session, 4, state, 6, 6);
            break;
        }
        session->percent = 0;
        sNdState.ready = FALSE;
        sNdState.error = 0;
        session->timeout = 0;
        *state = 3;
        break;
    case 3:
        NdReportResults();
        if (session->timeout++ > DOWNLOAD_TIMEOUT) {
            func_020424ac(0, 0, 0, 1013);
            download->status = NET_COMMON_DOWNLOAD_STATUS_TIMEOUT;
            NetCommonDownload_Cleanup(session, 3, state, 4, 4);
        }
        if (!sNdState.ready) {
            if (session->cancel) {
                download->status = NET_COMMON_DOWNLOAD_STATUS_CANCELLED;
                NetCommonDownload_Cleanup(session, 3, state, 4, 4);
                break;
            }
            if (DWC_NdGetProgress(&session->received, &session->contentLength) == TRUE) {
                u32 percent = session->received * 100 / session->contentLength;

                if (session->percent != percent) {
                    session->percent = percent;
                }
            }
        } else if (sNdState.error != 0) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_ERROR;
            NetCommonDownload_Cleanup(session, 4, state, 6, 6);
        } else if (!session->cancelled) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_DONE;
            NetCommonDownload_Cleanup(session, 1, state, 5, 5);
        } else {
            download->status = NET_COMMON_DOWNLOAD_STATUS_CANCELLED;
            NetCommonDownload_Cleanup(session, 3, state, 5, 5);
        }
        break;
    case 4:
        DWC_NdCancelAsync();
        *state = 5;
        break;
    case 5:
        *state = 6;
        break;
    case 6:
        NetCommonDownload_FreeSession(session);
        DWCRap_SetEventFunc(NULL, NULL);
        session->initialized = FALSE;
        return TRUE;
    case DOWNLOAD_WAIT:
        NdReportResults();
        if (session->timeout++ > DOWNLOAD_TIMEOUT) {
            func_020424ac(0, 0, 0, 1013);
            download->status = NET_COMMON_DOWNLOAD_STATUS_TIMEOUT;
            NetCommonDownload_Cleanup(session, 3, state, 6, 6);
        }
        if (sNdState.ready) {
            sNdState.ready = FALSE;
            if (sNdState.error != 0) {
                download->status = NET_COMMON_DOWNLOAD_STATUS_ERROR;
                NetCommonDownload_Cleanup(session, 3, state, 6, 6);
                break;
            }
            *state = session->nextState;
            break;
        }
        if (session->cancel) {
            session->cancelled = TRUE;
        }
        break;
    case DOWNLOAD_WAIT_CLEANUP:
        NdReportResults();
        if (session->timeout++ > DOWNLOAD_TIMEOUT) {
            func_020424ac(0, 0, 0, 1013);
            *state = session->nextState;
        }
        if (sNdState.done) {
            sNdState.done = FALSE;
            *state = session->nextState;
            break;
        }
        if (session->cancel) {
            session->cancelled = TRUE;
        }
        break;
    }
    return FALSE;
}

// Receives data that a beacon sends
static BOOL NetCommonDownload_ReceiveBeacons(NetCommonDownload *download, u32 *state) {
    const NetCommonDownloadBeaconArgs *args = NetCommonDownload_GetArgs(download);
    DeliveryInit init;

    switch (*state) {
    case 0:
        sys_memset(&init, 0, sizeof(DeliveryInit));
        init.code = args->code;
        init.data[0].datasize = args->dataSize;
        init.data[0].pData = download->data;
        init.data[0].region = args->region;
        init.data[0].mask = args->mask;
        init.dataNum = 1;
        init.flag4 = 0;
        init.heapId = download->param.heapId;
        download->beacon = func_ov012_02152990(&init);
        func_ov012_02152b64(download->beacon);
        download->stopBeacon = FALSE;
        download->status = NET_COMMON_DOWNLOAD_STATUS_NONE;
        (*state)++;
        break;
    case 1:
        if (func_02042788()) {
            func_02042ba8(TRUE, download->param.heapId);
            (*state)++;
        }
        break;
    case 2:
        func_ov012_02152bec(download->beacon);
        if (func_ov012_02152bd4(download->beacon)) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_DONE;
            *state = 3;
        } else if (download->stopBeacon) {
            download->status = NET_COMMON_DOWNLOAD_STATUS_CANCELLED;
            *state = 3;
        }
        break;
    case 3:
        func_ov012_02152bfc(download->beacon);
        (*state)++;
        break;
    case 4:
        if (func_02042ab8()) {
            (*state)++;
        }
        break;
    case 5:
        return TRUE;
    }
    return FALSE;
}

static const void *NetCommonDownload_GetArgs(NetCommonDownload *download) {
    return &download->args;
}

static BOOL NetCommonDownload_CanRequest(NetCommonDownload *download, u32 command) {
    u32 kind = NetCommonDownload_GetKind(download);
    BOOL allowed = FALSE;

    if (sKindCommands[kind][command] != 0) {
        if (sModeCommands[download->param.mode][command] != 0) {
            allowed = TRUE;
        }
    }
    return allowed;
}

static u32 NetCommonDownload_OnDwcEvent(void *work, int a1, int event, int a3) {
    NetCommonDownloadSession *session = work;

    switch (event) {
    case 3:
    case 5:
    case 6:
        return session->initialized;
    }
    return 0;
}

static void NetCommonDownload_AllocFileList(NetCommonDownloadSession *session,
                                            const NetCommonDownloadFileListArgs *args, HeapID heapId) {
    sys_memset(session, 0, sizeof(NetCommonDownloadSession));
    session->files =
        GFL_HeapAllocate(heapId, sizeof(DWCNdFileInfo) * args->fileCount, TRUE, "net_common_download.c", 1106);
}

static void NetCommonDownload_AllocFile(NetCommonDownloadSession *session, const NetCommonDownloadFileArgs *args,
                                        HeapID heapId) {
    sys_memset(session, 0, sizeof(NetCommonDownloadSession));
    session->files = GFL_HeapAllocate(heapId, sizeof(DWCNdFileInfo), TRUE, "net_common_download.c", 1120);
    sys_memcpy(args->name, session->files, GFL_STD_StrLen(args->name));
    session->files->size = args->size;
}

static void NetCommonDownload_FreeSession(NetCommonDownloadSession *session) {
    if (session->files != NULL) {
        GFL_HeapFree(session->files);
    }
    sys_memset(session, 0, sizeof(NetCommonDownloadSession));
}

static void NetCommonDownload_Wait(NetCommonDownloadSession *session, u32 *state, u32 nextState) {
    sNdState.ready = FALSE;
    sNdState.error = 0;
    session->nextState = nextState;
    session->timeout = 0;
    *state = DOWNLOAD_WAIT;
}

// Ends the download library, then goes on to nextState, or to failState at once if it can't
static void NetCommonDownload_Cleanup(NetCommonDownloadSession *session, u32 error, u32 *state, u32 nextState,
                                      u32 failState) {
    sNdState.done = FALSE;
    sNdState.error = error;
    session->nextState = nextState;
    session->timeout = 0;
    *state = DOWNLOAD_WAIT_CLEANUP;
    if (!DWC_NdCleanupAsync()) {
        *state = failState;
    }
}

static void NetCommonDownload_OnNdEvent(u32 reason, u32 error, int serverError) {
    sNdState.ready = TRUE;
    sNdState.done = TRUE;
    sNdState.error = error;
}

static BOOL NetCommonDownload_IsZero(const u32 *data, u32 size) {
    u32 i;

    for (i = 0; i < size / 4; i++) {
        if (data[i] != 0) {
            return FALSE;
        }
    }
    return TRUE;
}
