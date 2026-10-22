#include "types.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/hw.h"

// A fade steps the brightness by step every wait frames until it reaches the end. updateFreq scales the step and
// divides the wait, for callers that update the fade less often than every frame
typedef struct {
    u32 mode;
    s32 brightnessStart;
    s32 brightness;
    s32 brightnessEnd;
    s32 wait;
    s32 waitFrames;
    s32 step;
    s32 updateFreq;
} FadeMgr;

static FadeMgr *sFadeMgr;

void GFL_FadeCreate(u32 heapId) {
    sFadeMgr = GFL_HeapAllocate(heapId, sizeof(FadeMgr), FALSE, "fade.c", 50);
    MI_CpuFill8(sFadeMgr, 0, sizeof(FadeMgr));
    GFL_FadeSetUpdateFreqOne();
}

void GFL_FadeUpdate(void) {
    if (sFadeMgr == NULL || !(sFadeMgr->mode & 0xf)) {
        return;
    }
    if (sFadeMgr->wait != 0) {
        sFadeMgr->wait--;
        return;
    }
    sFadeMgr->wait = sFadeMgr->waitFrames;
    sFadeMgr->brightness += sFadeMgr->step;
    if ((sFadeMgr->brightness >= sFadeMgr->brightnessEnd && sFadeMgr->step > 0) ||
        (sFadeMgr->brightness <= sFadeMgr->brightnessEnd && sFadeMgr->step < 0)) {
        sFadeMgr->brightness = sFadeMgr->brightnessEnd;
    }
    GFL_FadeFlush();
    if (sFadeMgr->brightness == sFadeMgr->brightnessEnd) {
        sFadeMgr->mode = 0;
    }
}

s32 GFL_FadeGetUpdateFreq(void) {
    return sFadeMgr->updateFreq;
}

void GFL_FadeSetUpdateFreq(s32 updateFreq) {
    sFadeMgr->updateFreq = updateFreq;
}

void GFL_FadeSetUpdateFreqOne(void) {
    sFadeMgr->updateFreq = 1;
}

void GFL_FadeSet(u32 mode, s32 brightnessStart, s32 brightnessEnd, s32 slowness) {
    sFadeMgr->mode = mode;
    sFadeMgr->brightnessStart = brightnessStart;
    sFadeMgr->brightnessEnd = brightnessEnd;
    // A negative slowness steps by more than 1 each frame instead of waiting
    if (slowness < 0) {
        sFadeMgr->step = (slowness < 0 ? -slowness : slowness) + 1;
        sFadeMgr->wait = 0;
        sFadeMgr->waitFrames = 0;
    } else {
        sFadeMgr->step = 1;
        sFadeMgr->wait = slowness;
        sFadeMgr->waitFrames = slowness;
    }
    sFadeMgr->step *= sFadeMgr->updateFreq;
    if (slowness > 0) {
        sFadeMgr->wait /= sFadeMgr->updateFreq;
        sFadeMgr->waitFrames = slowness;
    }
    if (sFadeMgr->brightnessStart > sFadeMgr->brightnessEnd) {
        sFadeMgr->step *= -1;
    }
    sFadeMgr->brightness = sFadeMgr->brightnessStart;
    GFL_FadeFlush();
}

BOOL GFL_FadeIsRunning(void) {
    return sFadeMgr->mode != 0 ? TRUE : FALSE;
}

void GFL_FadeFlush(void) {
    if (sFadeMgr->mode & FADE_ENGINE_A_BLACK) {
        GX_SetMasterBrightness(-sFadeMgr->brightness);
    }
    if (sFadeMgr->mode & FADE_ENGINE_B_BLACK) {
        GXS_SetMasterBrightness(-sFadeMgr->brightness);
    }
    if (sFadeMgr->mode & FADE_ENGINE_A_WHITE) {
        GX_SetMasterBrightness(sFadeMgr->brightness);
    }
    if (sFadeMgr->mode & FADE_ENGINE_B_WHITE) {
        GXS_SetMasterBrightness(sFadeMgr->brightness);
    }
}
