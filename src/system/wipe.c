#include "types.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/wipe.h"
#include "system/wipe_sub.h"
#include "system/wipe_wnd.h"

// Screen wipes: the wipe of each screen runs its pattern's function from WIPE_FUNCTIONS once a frame until it is done

typedef BOOL (*WipeFunc)(WipeScreen *screen);

// A request to add a wipe's H-blank function at the next VBlank
typedef struct {
    WipeHBlank *hblank;
    void *work;
    WipeHBlankFunc func;
    int slot;
} WipeHBlankAddRequest;

// A request to remove one
typedef struct {
    WipeHBlank *hblank;
    int slot;
} WipeHBlankRemoveRequest;

// The orders the WIPE_MODE_*s run the screens' wipes in
#define WIPE_ORDER_BOTH 0
#define WIPE_ORDER_MAIN_FIRST 1
#define WIPE_ORDER_SUB_FIRST 2

typedef struct {
    u32 order;
    // Whether each screen's wipe is still running, and whether it wipes at all
    BOOL mainActive;
    BOOL subActive;
    BOOL mainOn;
    BOOL subOn;
} WipeControl;

typedef struct {
    WipeControl control;
    WipeScreen main;
    WipeScreen sub;
    WipeHBlank hblank;
    WipeWnd wnd;
    u16 active;
    // Whether each screen was left covered
    u8 mainCovered;
    u8 subCovered;
    // The color the last wipe covered with
    u16 color;
    TCB *hblankTcb;
} WipeSys;

static void Wipe_End(WipeSys *sys);
static BOOL Wipe_Update(WipeSys *sys, WipeScreen *main, WipeScreen *sub);
static void WipeScreen_Run(BOOL *active, WipeScreen *screen);
static BOOL WipeScreen_Call(WipeScreen *screen);
static void Wipe_SetMode(int mode, WipeSys *sys);
static void Wipe_SetControl(WipeControl *control, u32 order, BOOL mainOn, BOOL subOn);
static void WipeScreen_Init(WipeScreen *screen, u32 type, s32 division, s32 sync, s32 seq, void *work, s32 screenId,
                            WipeWnd *wnd, WipeHBlank *hblank, u32 heapId, u16 color);
static void WipeHBlank_Init(WipeHBlank *hblank);
static void WipeHBlank_Run(TCB *tcb, void *data);
static void WipeHBlank_Add(WipeHBlank *hblank, void *work, WipeHBlankFunc func, int slot);
static void WipeHBlank_Remove(WipeHBlank *hblank, int slot);
static void WipeHBlank_AddTask(TCB *tcb, void *data);
static void WipeHBlank_RemoveTask(TCB *tcb, void *data);
static void WipeHBlank_Nop(void *work);
static u16 Wipe_GetColor(WipeSys *sys, u16 color);
static u16 Wipe_GetEndColor(WipeSys *sys);
static void WipeScreen_ClearBrightnessTask(TCB *tcb, void *data);
static void WipeScreen_Start(WipeScreen *screen);
static void WipeScreen_Finish(WipeScreen *screen);
static void Wipe_Clear(WipeSys *sys);

static const WipeFunc WIPE_FUNCTIONS[] = {
    WipeFunc_BrightnessOut, WipeFunc_BrightnessIn, WipeFunc_LinesDownOut,  WipeFunc_LinesDownIn,
    WipeFunc_LinesUpOut,    WipeFunc_LinesUpIn,    WipeFunc_ShrinkLeftOut, WipeFunc_GrowRightIn,
    WipeFunc_CircleOut,     WipeFunc_CircleIn,     WipeFunc_GrowRightOut,  WipeFunc_ShrinkLeftIn,
};

static WipeSys sWipe;

void GFL_WipeSet(int mode, int typeMain, int typeSub, u16 color, s32 division, s32 sync, u32 heapId) {
    WipeSys *sys;

    sync /= GFL_FadeGetUpdateFreq();
    sys = &sWipe;

    Wipe_Clear(sys);
    Wipe_SetMode(mode, sys);
    WipeHBlank_Init(&sWipe.hblank);
    color = Wipe_GetColor(sys, color);
    WipeScreen_Init(&sWipe.main, typeMain, division, sync, 0, NULL, 0, &sWipe.wnd, &sWipe.hblank, heapId, color);
    WipeScreen_Init(&sWipe.sub, typeSub, division, sync, 0, NULL, 1, &sWipe.wnd, &sWipe.hblank, heapId, color);
    sWipe.active = TRUE;
    WipeScreen_Run(&sWipe.control.mainActive, &sWipe.main);
    WipeScreen_Run(&sWipe.control.subActive, &sWipe.sub);
    if (sys->control.mainOn) {
        WipeScreen_Start(&sys->main);
        sys->mainCovered = TRUE;
    }
    if (sys->control.subOn) {
        WipeScreen_Start(&sys->sub);
        sys->subCovered = TRUE;
    }
}

void GFL_WipeMain(void) {
    WipeSys *sys = &sWipe;

    if (sWipe.active) {
        if (Wipe_Update(sys, &sys->main, &sys->sub) == TRUE) {
            Wipe_End(sys);
        }
    }
}

BOOL GFL_WipeIsFinished(void) {
    if (sWipe.active == FALSE) {
        return TRUE;
    }
    return FALSE;
}

void GFL_WipeForceEnd(void) {
    WipeHBlank_Remove(&sWipe.hblank, 0);
    WipeHBlank_Remove(&sWipe.hblank, 1);
    if (sWipe.control.mainActive) {
        sWipe.main.seq = 2;
    }
    if (sWipe.control.subActive) {
        sWipe.sub.seq = 2;
    }
    WipeScreen_Run(&sWipe.control.mainActive, &sWipe.main);
    WipeScreen_Run(&sWipe.control.subActive, &sWipe.sub);
    sWipe.active = FALSE;
    sWipe.mainCovered = FALSE;
    sWipe.subCovered = FALSE;
    Wipe_Clear(&sWipe);
}

void Wipe_HideWindows(int screen) {
    WipeWnd_SetVisible(0, screen);
}

void killBrightnessEitherEngine(int screen) {
    setBrightnessForEngine(screen, 0);
}

void Wipe_SetScreenCovered(int screen, u16 color) {
    if (color == WIPE_COLOR_LAST) {
        color = sWipe.color;
    }
    setBrightnessForEngine(screen, color == WIPE_COLOR_WHITE ? 16 : -16);
}

void Wipe_SetCovered(u16 color) {
    int brightness;

    if (color == WIPE_COLOR_LAST) {
        color = sWipe.color;
    }
    if (color == WIPE_COLOR_WHITE) {
        brightness = 16;
    } else {
        brightness = -16;
    }
    setBrightnessForEngine(0, brightness);
    setBrightnessForEngine(1, brightness);
    sWipe.color = color;
}

void Wipe_SetBackdropColor(u16 color) {
    GX_LoadBGPltt(&color, 0, sizeof(color));
    GXS_LoadBGPltt(&color, 0, sizeof(color));
}

void setBrightnessForEngine(int screen, int brightness) {
    if (screen == 0) {
        GX_SetMasterBrightness(brightness);
    } else {
        GXS_SetMasterBrightness(brightness);
    }
}

static void Wipe_End(WipeSys *sys) {
    sys->active = FALSE;
    sys->color = Wipe_GetEndColor(sys);
    if (sys->control.mainOn) {
        WipeScreen_Finish(&sys->main);
        if (sys->main.endCovered == FALSE) {
            sWipe.mainCovered = FALSE;
        }
    }
    if (sys->control.subOn) {
        WipeScreen_Finish(&sys->sub);
        // BUG: The sub screen's flag is cleared by whether the main screen's wipe ended covered. Nothing reads the
        // flags, so it has no effect
#ifdef BUGFIX
        if (sys->sub.endCovered == FALSE) {
#else
        if (sys->main.endCovered == FALSE) {
#endif
            sWipe.subCovered = FALSE;
        }
    }
    Wipe_Clear(sys);
}

// Runs a frame of each screen's wipe in the control's order; TRUE once both are done
static BOOL Wipe_Update(WipeSys *sys, WipeScreen *main, WipeScreen *sub) {
    switch (sys->control.order) {
    case WIPE_ORDER_BOTH:
        WipeScreen_Run(&sys->control.mainActive, main);
        WipeScreen_Run(&sys->control.subActive, sub);
        break;
    case WIPE_ORDER_MAIN_FIRST:
        if (sys->control.mainActive) {
            WipeScreen_Run(&sys->control.mainActive, main);
        } else {
            WipeScreen_Run(&sys->control.subActive, sub);
        }
        break;
    case WIPE_ORDER_SUB_FIRST:
        if (sys->control.subActive) {
            WipeScreen_Run(&sys->control.subActive, sub);
        } else {
            WipeScreen_Run(&sys->control.mainActive, main);
        }
        break;
    }
    if (sys->control.mainActive == FALSE && sys->control.subActive == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static void WipeScreen_Run(BOOL *active, WipeScreen *screen) {
    if (*active) {
        if (WipeScreen_Call(screen) == TRUE) {
            *active = FALSE;
        }
    }
}

static BOOL WipeScreen_Call(WipeScreen *screen) {
    return WIPE_FUNCTIONS[screen->type](screen);
}

static void Wipe_SetMode(int mode, WipeSys *sys) {
    switch (mode) {
    case WIPE_MODE_BOTH:
        Wipe_SetControl(&sys->control, WIPE_ORDER_BOTH, TRUE, TRUE);
        break;
    case WIPE_MODE_MAIN_FIRST:
        Wipe_SetControl(&sys->control, WIPE_ORDER_MAIN_FIRST, TRUE, TRUE);
        break;
    case WIPE_MODE_SUB_FIRST:
        Wipe_SetControl(&sys->control, WIPE_ORDER_SUB_FIRST, TRUE, TRUE);
        break;
    case WIPE_MODE_MAIN:
        Wipe_SetControl(&sys->control, WIPE_ORDER_MAIN_FIRST, TRUE, FALSE);
        break;
    case WIPE_MODE_SUB:
        Wipe_SetControl(&sys->control, WIPE_ORDER_SUB_FIRST, FALSE, TRUE);
        break;
    }
}

static void Wipe_SetControl(WipeControl *control, u32 order, BOOL mainOn, BOOL subOn) {
    control->order = order;
    control->mainActive = mainOn;
    control->subActive = subOn;
    control->mainOn = mainOn;
    control->subOn = subOn;
}

static void WipeScreen_Init(WipeScreen *screen, u32 type, s32 division, s32 sync, s32 seq, void *work, s32 screenId,
                            WipeWnd *wnd, WipeHBlank *hblank, u32 heapId, u16 color) {
    screen->type = type;
    screen->division = division;
    screen->sync = sync;
    screen->seq = seq;
    screen->work = work;
    screen->screen = screenId;
    screen->wnd = wnd;
    screen->hblank = hblank;
    screen->heapId = heapId;
    screen->color = color;
}

static void WipeHBlank_Init(WipeHBlank *hblank) {
    int i;

    for (i = 0; i < 2; i++) {
        hblank->work[i] = NULL;
        hblank->func[i] = WipeHBlank_Nop;
        hblank->active[i] = FALSE;
    }
}

static void WipeHBlank_Run(TCB *tcb, void *data) {
    WipeHBlank *hblank = data;
    int i;

    for (i = 0; i < 2; i++) {
        hblank->func[i](hblank->work[i]);
    }
}

static void WipeHBlank_Add(WipeHBlank *hblank, void *work, WipeHBlankFunc func, int slot) {
    if (hblank->active[0] == FALSE && hblank->active[1] == FALSE) {
        sWipe.hblankTcb = GFL_HBlankTCBAdd(WipeHBlank_Run, hblank, 0);
    }
    hblank->work[slot] = work;
    if (func != NULL) {
        hblank->func[slot] = func;
    } else {
        hblank->func[slot] = WipeHBlank_Nop;
    }
    hblank->active[slot] = TRUE;
}

static void WipeHBlank_Remove(WipeHBlank *hblank, int slot) {
    hblank->active[slot] = FALSE;
    if (hblank->active[0] == FALSE && hblank->active[1] == FALSE) {
        if (sWipe.hblankTcb != NULL) {
            GFL_TCBRemove(sWipe.hblankTcb);
            sWipe.hblankTcb = NULL;
        }
    }
    hblank->func[slot] = WipeHBlank_Nop;
    hblank->work[slot] = NULL;
}

void WipeHBlank_AddAtVBlank(WipeHBlank *hblank, void *work, WipeHBlankFunc func, int slot, u32 heapId) {
    WipeHBlankAddRequest *request =
        GFL_HeapAllocate(HEAPID_TAIL((HeapID)heapId), sizeof(WipeHBlankAddRequest), FALSE, "wipe.c", 999);

    request->hblank = hblank;
    request->work = work;
    request->func = func;
    request->slot = slot;
    GFL_VBlankTCBAdd(WipeHBlank_AddTask, request, 0x400);
}

void WipeHBlank_RemoveAtVBlank(WipeHBlank *hblank, int slot, u32 heapId) {
    WipeHBlankRemoveRequest *request =
        GFL_HeapAllocate(HEAPID_TAIL((HeapID)heapId), sizeof(WipeHBlankRemoveRequest), FALSE, "wipe.c", 1024);

    request->hblank = hblank;
    request->slot = slot;
    GFL_VBlankTCBAdd(WipeHBlank_RemoveTask, request, 0x400);
}

static void WipeHBlank_AddTask(TCB *tcb, void *data) {
    WipeHBlankAddRequest *request = data;

    WipeHBlank_Add(request->hblank, request->work, request->func, request->slot);
    GFL_TCBRemove(tcb);
    GFL_HeapFree(request);
}

static void WipeHBlank_RemoveTask(TCB *tcb, void *data) {
    WipeHBlankRemoveRequest *request = data;

    WipeHBlank_Remove(request->hblank, request->slot);
    GFL_TCBRemove(tcb);
    GFL_HeapFree(request);
}

static void WipeHBlank_Nop(void *work) {
}

static u16 Wipe_GetColor(WipeSys *sys, u16 color) {
    if (color == WIPE_COLOR_LAST) {
        color = sys->color;
    }
    return color;
}

// The color the screens were left covered with, or the last one if the wipe uncovered them
static u16 Wipe_GetEndColor(WipeSys *sys) {
    WipeScreen *screen;

    if (sys->control.mainOn == TRUE) {
        screen = &sys->main;
    } else {
        screen = &sys->sub;
    }
    if (screen->endCovered == TRUE) {
        return screen->color;
    }
    return sys->color;
}

static void WipeScreen_ClearBrightnessTask(TCB *tcb, void *data) {
    WipeScreen *screen = data;

    setBrightnessForEngine(screen->screen, 0);
    GFL_TCBRemove(tcb);
}

// A wipe that uncovers a screen in black or white with a pattern clears the master brightness at the next VBlank
static void WipeScreen_Start(WipeScreen *screen) {
    if (screen->endCovered == FALSE && (screen->color == WIPE_COLOR_WHITE || screen->color == WIPE_COLOR_BLACK) &&
        screen->useBrightness == FALSE) {
        GFL_VBlankTCBAdd(WipeScreen_ClearBrightnessTask, screen, 0x400);
    }
}

// A wipe that covers a screen in black or white with a pattern leaves it covered with the master brightness, and its
// windows hidden
static void WipeScreen_Finish(WipeScreen *screen) {
    if (screen->endCovered == TRUE && (screen->color == WIPE_COLOR_WHITE || screen->color == WIPE_COLOR_BLACK) &&
        screen->useBrightness == FALSE) {
        Wipe_SetScreenCovered(screen->screen, screen->color);
        Wipe_HideWindows(screen->screen);
    }
}

static void Wipe_Clear(WipeSys *sys) {
    MI_CpuFill8(&sys->control, 0, sizeof(WipeControl));
    MI_CpuFill8(&sys->main, 0, sizeof(WipeScreen));
    MI_CpuFill8(&sys->sub, 0, sizeof(WipeScreen));
    MI_CpuFill8(&sys->hblank, 0, sizeof(WipeHBlank));
    MI_CpuFill8(&sys->wnd, 0, sizeof(WipeWnd));
}
