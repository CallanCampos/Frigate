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

static int func_02058520(int server, const char *gameName, u32 gameCode, DWCAllocFunc alloc, DWCFreeFunc free);
static void func_02058694(int started);

int func_020584e4(const char *gameName, u32 gameCode, void *(*alloc)(u32, u32, int), void (*free)(u32, void *, u32)) {
    OSi_ReferSymbol(DWCi_Signature);
    return func_02058520(1, gameName, gameCode, alloc, free);
}

static int func_02058520(int server, const char *gameName, u32 gameCode, DWCAllocFunc alloc, DWCFreeFunc free) {
    int checked;
    int result;
    BOOL created = FALSE;
    void *work;

    func_020586d0(alloc, free);
    clock_init();
    OS_InitAlarm();
    func_0207cb88();

    work = func_020586e4(DWC_MEM_INIT, DWC_INIT_WORK_SIZE);
    if (work == NULL) {
        return DWC_INIT_RESULT_OUT_OF_MEMORY;
    }
    checked = func_02056f48(work);
    if (func_02057580()) {
        func_02057300(work);
        created = TRUE;
    }
    func_02058728(DWC_MEM_INIT, work, 0);

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
    func_ov011_0216ddc8(func_020587e4, func_02058814, func_020587f8, func_0205882c);
    if (gameName != NULL) {
        strcpy(data_ov011_021917bc, gameName);
    }
    if (server == 0) {
        strcpy(data_ov011_02188fe4, "sdkdev.gamespy.com");
    } else {
        strcpy(data_ov011_02188fe4, "gamestats.gs.nintendowifi.net");
    }
    func_02058694(1);
    return result;
}

u32 func_02058684(void) {
    return sInit.gameCode;
}

static void func_02058694(int started) {
    sInit.started = started;
}
