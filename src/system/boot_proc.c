#include "system/boot_proc.h"
#include "types.h"
#include "app/boot_screens.h"
#include "app/cdemo.h"
#include "app/title.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// The first process of the game: the boot screens, the Game Freak logo, the opening and the title screen, over and
// over. The file's name is a guess, from what it does: the ROM has no string for it

// The demos of overlay 264
#define CDEMO_GF_LOGO 0
#define CDEMO_OPENING 2

static BOOL StartMenuEventInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL StartMenuEventTick(GameProc *proc, u32 *state, void *param, void *work);
static BOOL StartMenuEventEnd(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions EVENT_TITLE_FUNCS = {
    StartMenuEventInit,
    StartMenuEventTick,
    StartMenuEventEnd,
};

static BootScreensParam sBootScreensParam;
static CDemoParam sCDemoParam;

static BOOL StartMenuEventInit(GameProc *proc, u32 *state, void *param, void *work) {
    return TRUE;
}

// Never ends: the title screen's process is queued after it, and once that ends it starts over, showing only the
// copyright notice of the boot screens
static BOOL StartMenuEventTick(GameProc *proc, u32 *state, void *param, void *work) {
    switch (*state) {
    case 0:
        GCTX_ProcMgrQueueProc(OVERLAY_ID(162), &BOOT_SCREENS_PROC_FUNCTIONS, &sBootScreensParam);
        *state = 1;
        break;
    case 1:
        sBootScreensParam.skipLogos = TRUE;
        sCDemoParam.demo = CDEMO_GF_LOGO;
        sCDemoParam.canSkip = TRUE;
        sCDemoParam.skipped = FALSE;
        GCTX_ProcMgrQueueProc(OVERLAY_ID(264), &CDEMO_PROC_FUNCTIONS, &sCDemoParam);
        *state = 2;
        break;
    case 2:
        sCDemoParam.demo = CDEMO_OPENING;
        sCDemoParam.skipped = FALSE;
        GCTX_ProcMgrQueueProc(OVERLAY_ID(264), &CDEMO_PROC_FUNCTIONS, &sCDemoParam);
        *state = 3;
        break;
    case 3:
        GCTX_ProcMgrQueueProc(OVERLAY_ID(162), &TITLE_PROC_FUNCTIONS, NULL);
        *state = 0;
        break;
    }
    return FALSE;
}

static BOOL StartMenuEventEnd(GameProc *proc, u32 *state, void *param, void *work) {
    return TRUE;
}
