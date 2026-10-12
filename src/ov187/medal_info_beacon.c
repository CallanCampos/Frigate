#include "types.h"
#include "app/medal_info.h"
#include "app/medal_info_viewer.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "system/beacon_status.h"
#include "system/game_beacon.h"
#include "system/game_data.h"

// The processes of overlay 187's medal screens, named after the ROM's "medal_info_beacon.c" (our guess at the name's
// meaning: the file also serves the Join Avenue and the Trainer Card). The names are ours
//
// The first process shows the medals of one visitor, from a MedalInfoParam, and the second those of the senders of the
// beacon log, from a MedalInfoBeaconParam. The log's entries are what the viewer steps through

// The work of the first process
typedef struct {
    // Never set by the second process, so the heap it allocates from is 0 (see MedalInfoBeacon_LoadBeacons)
    HeapID heapId;
    MedalInfoParam *param;
    MedalInfoViewerParam viewerParam;
    MedalInfoViewer *viewer;
    MedalInfoEntry entry;
} MedalInfoWork;

// The work of the second process
typedef struct {
    HeapID heapId;
    MedalInfoBeaconParam *param;
    MedalInfoViewerParam viewerParam;
    MedalInfoViewer *viewer;
    u32 unk24;
} MedalInfoBeaconWork;

static BOOL MedalInfoBeacon_MedalMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MedalInfoBeacon_MedalExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MedalInfoBeacon_LogInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MedalInfoBeacon_LogExit(GameProc *proc, u32 *state, void *param, void *work);
static void MedalInfoBeacon_LoadBeacons(MedalInfoBeaconWork *work);

GameProcFunctions MedalInfo_ProcFunctions = { MedalInfo_Init, MedalInfoBeacon_MedalMain, MedalInfoBeacon_MedalExit };
GameProcFunctions MedalInfoBeacon_ProcFunctions = { MedalInfoBeacon_LogInit, MedalInfoBeacon_LogMain, MedalInfoBeacon_LogExit };

BOOL MedalInfo_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MedalInfoParam *medalParam = param;
    MedalInfoWork *medalWork;

    GFL_HeapCreateChild(medalParam->parentHeapId, HEAPID_MEDAL_INFO, 0x21000);
    medalWork = GFL_ProcInitSubsystem(proc, sizeof(MedalInfoWork), HEAPID_MEDAL_INFO);
    sys_memset(medalWork, 0, sizeof(MedalInfoWork));
    medalWork->entry.name = medalParam->name;
    medalWork->entry.medal = medalParam->medal;
    medalWork->entry.count = medalParam->medalCount;
    medalWork->entry.rank = medalParam->rank;
    medalWork->entry.year = medalParam->year;
    medalWork->entry.month = medalParam->month;
    medalWork->entry.day = medalParam->day;
    medalWork->param = medalParam;
    medalWork->viewerParam.heapId = HEAPID_MEDAL_INFO;
    medalWork->viewerParam.count = 1;
    medalWork->viewerParam.entries = &medalWork->entry;
    medalWork->viewerParam.startIndex = 0;
    if (medalParam->mode == 0) {
        medalWork->viewerParam.mode = MEDAL_INFO_VIEWER_MODE_BACK;
    } else {
        medalWork->viewerParam.mode = MEDAL_INFO_VIEWER_MODE_BACK_OR_FIELD;
    }
    medalWork->viewer = MedalInfoViewer_Create(&medalWork->viewerParam);
    return TRUE;
}

static BOOL MedalInfoBeacon_MedalMain(GameProc *proc, u32 *state, void *param, void *work) {
    MedalInfoWork *medalWork = work;

    return MedalInfoViewer_Main(medalWork->viewer);
}

static BOOL MedalInfoBeacon_MedalExit(GameProc *proc, u32 *state, void *param, void *work) {
    MedalInfoWork *medalWork = work;
    u32 result = MedalInfoViewer_Delete(medalWork->viewer);

    if (result == 0) {
        medalWork->param->result = 0;
    } else if (result == 1) {
        medalWork->param->result = 1;
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_MEDAL_INFO);
    return TRUE;
}

static BOOL MedalInfoBeacon_LogInit(GameProc *proc, u32 *state, void *param, void *work) {
    MedalInfoBeaconWork *logWork;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MEDAL_INFO, 0x31000);
    logWork = GFL_ProcInitSubsystem(proc, sizeof(MedalInfoBeaconWork), HEAPID_MEDAL_INFO);
    sys_memset(logWork, 0, sizeof(MedalInfoBeaconWork));
    logWork->param = param;
    MedalInfoBeacon_LoadBeacons(logWork);
    logWork->viewerParam.heapId = HEAPID_MEDAL_INFO;
    logWork->viewer = MedalInfoViewer_Create(&logWork->viewerParam);
    return TRUE;
}

BOOL MedalInfoBeacon_LogMain(GameProc *proc, u32 *state, void *param, void *work) {
    MedalInfoBeaconWork *logWork = work;

    return MedalInfoViewer_Main(logWork->viewer);
}

static BOOL MedalInfoBeacon_LogExit(GameProc *proc, u32 *state, void *param, void *work) {
    MedalInfoBeaconWork *logWork = work;
    BeaconStatus *status;
    GameBeaconInfoTable *log;
    u32 result = MedalInfoViewer_Delete(logWork->viewer);
    u32 i;
    int shownFirst;
    int shownLast;
    int lowest;
    int highest;
    int count;

    if (result == 0) {
        logWork->param->returnToField = 0;
    } else if (result == 1) {
        logWork->param->returnToField = 1;
    }
    status = func_020174d4(logWork->param->gameData);
    log = func_0202d7a8(status);
    // Scroll the log to keep the last beacon shown in view
    shownFirst = logWork->viewerParam.entries[logWork->viewerParam.startIndex].index;
    shownLast = logWork->viewerParam.entries[logWork->viewerParam.selected].index;
    count = GameBeaconInfoTable_GetCount(log);
    lowest = shownFirst - logWork->param->logRow;
    highest = shownFirst + (3 - logWork->param->logRow);
    if (shownLast < lowest || shownLast > highest) {
        if (count - shownLast < 4) {
            shownLast = count - 4;
        }
        func_0202d7b8(status, shownLast);
    }
    for (i = 0; i < logWork->viewerParam.count; i++) {
        GFL_StrBufFree(logWork->viewerParam.entries[i].name);
    }
    GFL_HeapFree(logWork->viewerParam.entries);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_MEDAL_INFO);
    return TRUE;
}

// Collects the entries of the log's beacons that carry a medal
static void MedalInfoBeacon_LoadBeacons(MedalInfoBeaconWork *work) {
    u16 n = 0;
    int i;
    int total;
    u16 time;
    u32 kind;
    GameBeacon *beacon = GameBeacon_Alloc(work->heapId);
    GameBeaconInfoTable *status = func_0202d7a8(func_020174d4(work->param->gameData));

    work->viewerParam.count = 0;
    work->viewerParam.mode = MEDAL_INFO_VIEWER_MODE_LIST;
    total = GameBeaconInfoTable_GetCount(status);
    for (i = 0; i < total; i++) {
        GameBeaconInfoTable_RingGet(status, beacon, &time, i);
        kind = GameBeacon_GetVersion(beacon);
        if (!GameBeacon_IsInvalid(beacon) && !GameBeacon_IsGiftType(beacon) && (kind == 0x16 || kind == 0x17)) {
            if (func_02013dc8(beacon) != MEDAL_INFO_NO_MEDAL) {
                work->viewerParam.count++;
            }
        }
    }
    work->viewerParam.entries = GFL_HeapAllocate(work->heapId, work->viewerParam.count * sizeof(MedalInfoEntry), TRUE,
                                                 "medal_info_beacon.c", 282);
    for (i = 0; i < total; i++) {
        GameBeaconInfoTable_RingGet(status, beacon, &time, i);
        kind = GameBeacon_GetVersion(beacon);
        if (!GameBeacon_IsInvalid(beacon) && !GameBeacon_IsGiftType(beacon) && (kind == 0x16 || kind == 0x17)) {
            if (work->param->beaconIndex == i) {
                work->viewerParam.startIndex = n;
            }
            if (func_02013dc8(beacon) != MEDAL_INFO_NO_MEDAL) {
                MedalInfoEntry *entry = &work->viewerParam.entries[n];

                entry->name = GFL_StrBufCreate(8, work->heapId);
                GameBeacon_GetName(beacon, entry->name);
                entry->medal = func_02013dc8(beacon);
                entry->count = GameBeacon_GetMedalCount(beacon);
                entry->rank = 0;
                entry->index = i;
                n++;
            }
        }
    }
    // BUG: The beacon is never freed
#ifdef BUGFIX
    GFL_HeapFree(beacon);
#endif
}
