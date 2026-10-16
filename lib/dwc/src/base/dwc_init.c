#include "dwc/bm.h"
#include "dwc/dwc.h"
#include "dwc/ext.h"
#include "dwc/init.h"
#include "dwc/memfunc.h"
#include "nitro/os.h"
#include "nitro/rtc.h"

// DWC's start (TWL-DWC 5.3's dwc_init.c, a guessed name; the ROM names none of it, and the file is ARM code in ARM9
// main like the rest of the library's base): starts the timer, the alarms and the clock, makes the Wi-Fi ID if the
// console has none, and gives GameSpy its memory functions and the host name of the stats server.

// The name that the work area of the Wi-Fi settings is asked for with
#define DWC_MEM_INIT 2
// The size of that work area
#define DWC_INIT_WORK_SIZE 0x1000

typedef struct DWCInit {
    // The game code that the library was started with
    u32 gameCode;
    // 1 once it has started
    int started;
} DWCInit;

static DWCInit sInit;

static int DWCi_Init(int server, const char *gameName, u32 gameCode, DWCAllocFunc alloc, DWCFreeFunc free);
static void DWCi_SetStarted(int started);

int DWC_InitForProduction(const char *gameName, u32 gameCode, void *(*alloc)(u32, u32, int),
                          void (*free)(u32, void *, u32)) {
    OSi_ReferSymbol(DWCi_Signature);
    return DWCi_Init(1, gameName, gameCode, alloc, free);
}

static int DWCi_Init(int server, const char *gameName, u32 gameCode, DWCAllocFunc alloc, DWCFreeFunc free) {
    int checked;
    int result;
    BOOL created = FALSE;
    void *work;

    DWCi_SetMemFunc(alloc, free);
    OS_InitTick();
    OS_InitAlarm();
    RTC_Init();

    work = DWC_Alloc(DWC_MEM_INIT, DWC_INIT_WORK_SIZE);
    if (work == NULL) {
        return DWC_INIT_RESULT_OUT_OF_MEMORY;
    }
    checked = DWC_BM_Init(work);
    if (DWCBm_HasNoWifiId()) {
        DWCBm_MakeAndSaveWifiId(work);
        created = TRUE;
    }
    DWC_Free(DWC_MEM_INIT, work, 0);

    if (checked < 0) {
        if (created) {
            result = DWC_INIT_RESULT_DESTROY_USERID;
        } else {
            result = DWC_INIT_RESULT_DESTROY_OTHER_SETTING;
        }
    } else {
        if (created) {
            result = DWC_INIT_RESULT_CREATE_USERID;
        } else {
            result = DWC_INIT_RESULT_NORMAL;
        }
    }

    func_ov011_02169df8(server);
    sInit.gameCode = gameCode;
    func_ov011_0216ddc8(DWCi_GsAlloc, DWCi_GsFree, DWCi_GsRealloc, DWCi_GsMemalign);
    if (gameName != NULL) {
        strcpy(data_ov011_021917bc, gameName);
    }
    if (server == 0) {
        strcpy(data_ov011_02188fe4, "sdkdev.gamespy.com");
    } else {
        strcpy(data_ov011_02188fe4, "gamestats.gs.nintendowifi.net");
    }
    DWCi_SetStarted(1);
    return result;
}

u32 DWCi_GetGameCode(void) {
    return sInit.gameCode;
}

static void DWCi_SetStarted(int started) {
    sInit.started = started;
}
