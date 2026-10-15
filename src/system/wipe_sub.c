#include "types.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/wipe.h"
#include "system/wipe_sub.h"
#include "system/wipe_wnd.h"

// The wipe patterns: master brightness fades, lines covered from an edge, a window rectangle that grows or shrinks, and
// a circle drawn line by line in the H-blank. Positions and sizes are kept with 7 fraction bits while they move

// A run of lines from start to before end, covered before the moving edge if covered is set, or the other way round
typedef struct {
    u8 start;
    u8 end;
    u16 covered;
} WipeLineRange;

typedef struct {
    const WipeLineRange *ranges;
    u16 count;
    u16 isOut;
} WipeLinePattern;

// A window rectangle moving from start to end (x1, y1, x2, y2), with the planes inside and outside it
typedef struct {
    u8 start[4];
    u8 end[4];
    u8 window;
    u8 inPlanes;
    u8 outPlanes;
    u8 isOut;
} WipeRectParam;

typedef struct {
    s16 startRadius;
    s16 endRadius;
    s16 cx;
    s16 cy;
    u8 window;
    u8 inPlanes;
    u8 outPlanes;
    u8 isOut;
} WipeCircleParam;

typedef struct {
    s32 division;
    s32 sync;
    s32 syncCount;
    s32 value;
    s32 end;
    s32 step;
    s32 screen;
    s32 brightness;
    TCB *tcb;
} WipeBrightWork;

// A screen's lines, 1 where the window covers them, for each window
typedef struct {
    // A copy of the lines, which the VBlank makes and nothing reads
    u8 prev[192];
    u8 lines[192];
    u32 window;
} WipeLineBuf;

typedef struct {
    WipeLineBuf buf[2];
    u8 count;
    u8 screen;
} WipeLineWnd;

typedef struct {
    WipeLineWnd lines;
    const WipeLineRange *ranges;
    s32 count;
    s32 division;
    s32 step;
    s32 sync;
    s32 syncCount;
    BOOL isOut;
    u32 heapId;
    WipeWnd *wnd;
    WipeHBlank *hblank;
} WipeLineWork;

typedef struct {
    s32 cur[4];
    s32 step[4];
    s32 end[4];
    s32 screen;
    s32 window;
    s32 division;
    s32 sync;
    s32 syncCount;
    BOOL isOut;
    WipeWnd *wnd;
} WipeRectWork;

// The left and right edges of a window on each line
typedef struct {
    s16 left[192];
    s16 right[192];
} WipeScanLines;

typedef struct {
    WipeScanLines lines;
    WipeScanLines next;
    u32 window;
} WipeScanBuf;

typedef struct {
    WipeScanBuf *buf;
    s32 count;
    s32 screen;
} WipeScanWnd;

typedef struct {
    WipeScanWnd scan;
    s32 radius;
    s32 cx;
    s32 cy;
    s32 step;
    s32 division;
    s32 sync;
    s32 syncCount;
    u32 heapId;
    BOOL isOut;
    WipeWnd *wnd;
    WipeHBlank *hblank;
} WipeCircleWork;

static s32 WipeSub_GetStep(s32 start, s32 end, s32 division);
static void WipeRect_AddSteps(s32 *cur, s32 *step);
static void WipeRect_SetSteps(s32 *cur, s32 *end, s32 *step, const u8 *startPos, const u8 *endPos, s32 division);
static void WipeBright_Init(WipeScreen *screen, BOOL cover);
static BOOL WipeBright_Main(WipeScreen *screen);
static BOOL WipeBright_Step(WipeBrightWork *work);
static void WipeBright_VBlank(TCB *tcb, void *data);
static void WipeScanWnd_HBlank(void *data);
static void WipeScanWnd_Init(WipeScanWnd *scan, u32 window, s32 screen, u32 heapId);
static void WipeScanWnd_Exit(WipeScanWnd *scan);
static void WipeScanWnd_Free(WipeScanWnd *scan);
static WipeScanBuf *WipeScanWnd_GetBuf(WipeScanWnd *scan, s32 index);
static void WipeScanWnd_CopyTask(TCB *tcb, void *data);
static void WipeSub_ResetWnd(BOOL isOut, WipeWnd *wnd, s32 screen);
static void WipeSub_SetWnd(WipeWnd *wnd, u32 inPlanes, u32 outPlanes, u32 window, u32 screen, int x1, int y1, int x2,
                           int y2, BOOL deferred);
static void WipeSub_SetWndVisible(WipeWnd *wnd, u32 visible, s32 screen, BOOL deferred);
static void WipeLineWnd_Init(WipeLineWnd *lines, u32 screen, u32 count, u32 window0, u32 window1);
static void WipeLineWnd_StartCopy(WipeLineWnd *lines);
static void WipeLineWnd_AddHBlank(WipeHBlank *hblank, WipeLineWnd *lines, u32 heapId);
static void WipeLineWnd_RemoveHBlank(WipeHBlank *hblank, WipeLineWnd *lines, u32 heapId);
static void WipeLineWnd_CopyTask(TCB *tcb, void *data);
static void WipeLineWnd_HBlank(void *data);
static void WipeRect_Init(WipeScreen *screen, const WipeRectParam *param);
static BOOL WipeRect_Main(WipeScreen *screen);
static void WipeRectWork_Init(WipeRectWork *work, const WipeRectParam *param, s32 division, s32 sync, s32 screen,
                              WipeWnd *wnd);
static BOOL WipeRectWork_Step(WipeRectWork *work);
static void WipeCircle_Init(WipeScreen *screen, const WipeCircleParam *param);
static BOOL WipeCircle_Main(WipeScreen *screen);
static void WipeCircleWork_Init(WipeCircleWork *work, const WipeCircleParam *param, s32 division, s32 sync, s32 screen,
                                WipeWnd *wnd, WipeHBlank *hblank, u32 heapId);
static BOOL WipeCircleWork_Step(WipeCircleWork *work);
static void WipeCircle_GetSpan(s32 radius, s32 cx, s32 cy, s32 y, s32 *x1, s32 *x2);
static void WipeCircleWork_Compute(WipeCircleWork *work);
static void WipeLine_Init(WipeScreen *screen, WipeLinePattern *pattern);
static BOOL WipeLine_Main(WipeScreen *screen);
static void WipeLineWork_Init(WipeLineWork *work, WipeLinePattern *pattern, s32 division, s32 sync, u32 screen,
                              WipeWnd *wnd, WipeHBlank *hblank, u32 heapId);
static BOOL WipeLineWork_Step(WipeLineWork *work);
static void WipeLineWork_Exit(WipeLineWork *work);
static void WipeLineWork_Compute(WipeLineWork *work);
static void WipeLineRange_Apply(const WipeLineRange *range, WipeLineWnd *lines, s32 step, s32 division);

BOOL WipeFunc_BrightnessOut(WipeScreen *screen) {
    if (screen->seq == 0) {
        screen->endCovered = TRUE;
        screen->useBrightness = TRUE;
        WipeBright_Init(screen, TRUE);
        return FALSE;
    }
    return WipeBright_Main(screen);
}

BOOL WipeFunc_BrightnessIn(WipeScreen *screen) {
    if (screen->seq == 0) {
        screen->endCovered = FALSE;
        screen->useBrightness = TRUE;
        WipeBright_Init(screen, FALSE);
        return FALSE;
    }
    return WipeBright_Main(screen);
}

BOOL WipeFunc_LinesDownOut(WipeScreen *screen) {
    if (screen->seq == 0) {
        static const WipeLineRange range = { 0x00, 0xc0, TRUE };
        static WipeLinePattern pattern = { NULL, 1, TRUE };

        pattern.ranges = &range;
        Wipe_SetBackdropColor(screen->color);
        WipeLine_Init(screen, &pattern);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeLine_Main(screen);
}

BOOL WipeFunc_LinesDownIn(WipeScreen *screen) {
    if (screen->seq == 0) {
        static const WipeLineRange range = { 0x00, 0xc0, FALSE };
        static WipeLinePattern pattern = { NULL, 1, FALSE };

        pattern.ranges = &range;
        Wipe_SetBackdropColor(screen->color);
        WipeLine_Init(screen, &pattern);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeLine_Main(screen);
}

BOOL WipeFunc_LinesUpOut(WipeScreen *screen) {
    if (screen->seq == 0) {
        static const WipeLineRange range = { 0xc0, 0x00, TRUE };
        static WipeLinePattern pattern = { NULL, 1, TRUE };

        pattern.ranges = &range;
        Wipe_SetBackdropColor(screen->color);
        WipeLine_Init(screen, &pattern);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeLine_Main(screen);
}

BOOL WipeFunc_LinesUpIn(WipeScreen *screen) {
    if (screen->seq == 0) {
        static const WipeLineRange range = { 0xc0, 0x00, FALSE };
        static WipeLinePattern pattern = { NULL, 1, FALSE };

        pattern.ranges = &range;
        Wipe_SetBackdropColor(screen->color);
        WipeLine_Init(screen, &pattern);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeLine_Main(screen);
}

BOOL WipeFunc_ShrinkLeftOut(WipeScreen *screen) {
    if (screen->seq == 0) {
        static const WipeRectParam param = {
            { 0x00, 0x00, 0xff, 0xc0 }, { 0x00, 0x00, 0x00, 0xc0 }, 0, 0x3f, 0x20, TRUE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRect_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRect_Main(screen);
}

BOOL WipeFunc_GrowRightIn(WipeScreen *screen) {
    if (screen->seq == 0) {
        static const WipeRectParam param = {
            { 0x00, 0x00, 0x00, 0xc0 }, { 0x00, 0x00, 0xff, 0xc0 }, 0, 0x3f, 0x20, FALSE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRect_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRect_Main(screen);
}

BOOL WipeFunc_CircleOut(WipeScreen *screen) {
    if (screen->seq == 0) {
        static const WipeCircleParam param = { 256, 0, 128, 96, 0, 0x3f, 0x20, TRUE };

        Wipe_SetBackdropColor(screen->color);
        WipeCircle_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeCircle_Main(screen);
}

BOOL WipeFunc_CircleIn(WipeScreen *screen) {
    if (screen->seq == 0) {
        static const WipeCircleParam param = { 0, 256, 128, 96, 0, 0x3f, 0x20, FALSE };

        Wipe_SetBackdropColor(screen->color);
        WipeCircle_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeCircle_Main(screen);
}

BOOL WipeFunc_GrowRightOut(WipeScreen *screen) {
    if (screen->seq == 0) {
        static const WipeRectParam param = {
            { 0x00, 0x00, 0x00, 0xc0 }, { 0x00, 0x00, 0xff, 0xc0 }, 0, 0x20, 0x3f, TRUE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRect_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRect_Main(screen);
}

BOOL WipeFunc_ShrinkLeftIn(WipeScreen *screen) {
    if (screen->seq == 0) {
        static const WipeRectParam param = {
            { 0x00, 0x00, 0xff, 0xc0 }, { 0x00, 0x00, 0x00, 0xc0 }, 0, 0x20, 0x3f, FALSE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRect_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRect_Main(screen);
}

// The step a value moves each division, with 7 fraction bits
static s32 WipeSub_GetStep(s32 start, s32 end, s32 division) {
    return ((end - start) << 7) / division;
}

static void WipeRect_AddSteps(s32 *cur, s32 *step) {
    cur[0] += step[0];
    cur[1] += step[1];
    cur[2] += step[2];
    cur[3] += step[3];
}

static void WipeRect_SetSteps(s32 *cur, s32 *end, s32 *step, const u8 *startPos, const u8 *endPos, s32 division) {
    cur[0] = startPos[0] << 7;
    cur[1] = startPos[1] << 7;
    cur[2] = startPos[2] << 7;
    cur[3] = startPos[3] << 7;
    end[0] = endPos[0];
    end[1] = endPos[1];
    end[2] = endPos[2];
    end[3] = endPos[3];
    step[0] = WipeSub_GetStep(startPos[0], endPos[0], division);
    step[1] = WipeSub_GetStep(startPos[1], endPos[1], division);
    step[2] = WipeSub_GetStep(startPos[2], endPos[2], division);
    step[3] = WipeSub_GetStep(startPos[3], endPos[3], division);
}

static void WipeBright_Init(WipeScreen *screen, BOOL cover) {
    WipeBrightWork *work;
    s32 start, end;

    screen->work = GFL_HeapAllocate(screen->heapId, sizeof(WipeBrightWork), FALSE, "wipe_sub.c", 2476);
    sys_memset(screen->work, 0, sizeof(WipeBrightWork));
    work = screen->work;
    if (cover == FALSE) {
        if (screen->color == WIPE_COLOR_WHITE) {
            start = 16;
            end = 0;
        } else {
            start = -16;
            end = 0;
        }
    } else {
        if (screen->color == WIPE_COLOR_WHITE) {
            start = 0;
            end = 16;
        } else {
            start = 0;
            end = -16;
        }
    }
    setBrightnessForEngine(screen->screen, start);
    work->division = screen->division;
    work->sync = screen->sync;
    work->syncCount = 0;
    work->value = start << 7;
    work->end = end << 7;
    work->step = WipeSub_GetStep(start, end, screen->division);
    work->screen = screen->screen;
    work->brightness = start;
    screen->seq++;
    work->tcb = GFL_VBlankTCBAdd(WipeBright_VBlank, work, 0x80);
}

static BOOL WipeBright_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeBrightWork *work = screen->work;

    switch (screen->seq) {
    case 1:
        if (WipeBright_Step(work) == TRUE) {
            screen->seq++;
        }
        break;
    case 2:
        GFL_TCBRemove(work->tcb);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case 3:
        done = TRUE;
        break;
    }
    return done;
}

static BOOL WipeBright_Step(WipeBrightWork *work) {
    BOOL done = FALSE;

    work->syncCount++;
    if (work->syncCount >= work->sync) {
        work->syncCount = 0;
        if (work->division - 1 > 0) {
            work->division--;
            work->value += work->step;
        } else {
            work->value = work->end;
            done = TRUE;
        }
        work->brightness = work->value / 128;
    }
    return done;
}

static void WipeBright_VBlank(TCB *tcb, void *data) {
    WipeBrightWork *work = data;

    setBrightnessForEngine(work->screen, work->brightness);
}

// Sets a window's edges for the next line
static inline void WipeScanWnd_SetLine(WipeScanWnd *scan, s32 index, s32 line) {
    WipeScanBuf *buf = WipeScanWnd_GetBuf(scan, index);
    s32 screen = scan->screen;
    s16 right = buf->lines.right[line];
    s16 left = buf->lines.left[line];

    if (buf->window == 0) {
        if (screen == 0) {
            if (GX_IsHBlank()) {
                G2_SetWnd0Position(left, 0, right, 192);
            }
        } else {
            if (GX_IsHBlank()) {
                G2S_SetWnd0Position(left, 0, right, 192);
            }
        }
    } else {
        if (screen == 0) {
            if (GX_IsHBlank()) {
                G2_SetWnd1Position(left, 0, right, 192);
            }
        } else {
            if (GX_IsHBlank()) {
                G2S_SetWnd1Position(left, 0, right, 192);
            }
        }
    }
}

static void WipeScanWnd_HBlank(void *data) {
    WipeScanWnd *scan = data;
    s32 line = GX_GetVCount();

    if (line < 192) {
        line++;
        if (line > 191) {
            line -= 192;
        }
        if (scan->count == 1) {
            WipeScanWnd_SetLine(scan, 0, line);
        } else {
            WipeScanWnd_SetLine(scan, 0, line);
            WipeScanWnd_SetLine(scan, 1, line);
        }
    }
}

static void WipeScanWnd_Init(WipeScanWnd *scan, u32 window, s32 screen, u32 heapId) {
    int i;

    switch (window) {
    case 0:
    case 1: {
        WipeScanBuf *buf = GFL_HeapAllocate(heapId, sizeof(WipeScanBuf), FALSE, "wipe_sub.c", 2715);

        scan->count = 1;
        scan->buf = buf;
        scan->screen = screen;
        buf->window = window;
        break;
    }
    case 2:
        scan->buf = GFL_HeapAllocate(heapId, sizeof(WipeScanBuf) * 2, FALSE, "wipe_sub.c", 2723);
        scan->count = 2;
        scan->screen = screen;
        for (i = 0; i < 2; i++) {
            scan->buf[i].window = i;
        }
        break;
    }
}

static void WipeScanWnd_Exit(WipeScanWnd *scan) {
    WipeScanWnd_Free(scan);
}

static void WipeScanWnd_Free(WipeScanWnd *scan) {
    GFL_HeapFree(scan->buf);
    scan->buf = NULL;
}

static WipeScanBuf *WipeScanWnd_GetBuf(WipeScanWnd *scan, s32 index) {
    return &scan->buf[index];
}

// Copies the next edges into the ones the H-blank uses
static void WipeScanWnd_CopyTask(TCB *tcb, void *data) {
    WipeScanWnd *scan = data;
    int i;

    for (i = 0; i < scan->count; i++) {
        WipeScanBuf *buf = WipeScanWnd_GetBuf(scan, i);

        sys_memcpy(&buf->next, &buf->lines, sizeof(WipeScanLines));
    }
    GFL_TCBRemove(tcb);
}

// Leaves the windows as a wipe that ends covered or uncovered does
static void WipeSub_ResetWnd(BOOL isOut, WipeWnd *wnd, s32 screen) {
    if (isOut == FALSE) {
        WipeWnd_SetVisibleAtVBlank(wnd, 0, screen);
    } else {
        WipeWnd_SetVisibleAtVBlank(wnd, 1, screen);
        WipeWnd_SetInsidePlaneAtVBlank(wnd, 0x3f, FALSE, 0, screen);
        WipeWnd_SetPositionAtVBlank(wnd, 0, 0, 0, 0, 0, screen);
        WipeWnd_SetOutsidePlaneAtVBlank(wnd, 0x20, FALSE, screen);
    }
}

static void WipeSub_SetWnd(WipeWnd *wnd, u32 inPlanes, u32 outPlanes, u32 window, u32 screen, int x1, int y1, int x2,
                           int y2, BOOL deferred) {
    if (deferred == FALSE) {
        WipeWnd_SetInsidePlane(inPlanes, FALSE, window, screen);
        WipeWnd_SetOutsidePlane(outPlanes, FALSE, screen);
        WipeWnd_SetPosition(x1, y1, x2, y2, window, screen);
    } else {
        WipeWnd_SetInsidePlaneAtVBlank(wnd, inPlanes, FALSE, window, screen);
        WipeWnd_SetOutsidePlaneAtVBlank(wnd, outPlanes, FALSE, screen);
        WipeWnd_SetPositionAtVBlank(wnd, x1, y1, x2, y2, window, screen);
    }
}

static void WipeSub_SetWndVisible(WipeWnd *wnd, u32 visible, s32 screen, BOOL deferred) {
    if (deferred == FALSE) {
        WipeWnd_SetVisible(visible, screen);
    } else {
        WipeWnd_SetVisibleAtVBlank(wnd, visible, screen);
    }
}

static void WipeLineWnd_Init(WipeLineWnd *lines, u32 screen, u32 count, u32 window0, u32 window1) {
    sys_memset(lines, 0, sizeof(WipeLineWnd));
    if (count == 1) {
        lines->buf[0].window = window0;
        lines->count = count;
        lines->screen = screen;
    } else {
        lines->buf[0].window = window0;
        lines->buf[1].window = window1;
        lines->count = count;
        lines->screen = screen;
    }
}

static void WipeLineWnd_StartCopy(WipeLineWnd *lines) {
    GFL_VBlankTCBAdd(WipeLineWnd_CopyTask, lines, 0x3ff);
}

static void WipeLineWnd_AddHBlank(WipeHBlank *hblank, WipeLineWnd *lines, u32 heapId) {
    WipeHBlank_AddAtVBlank(hblank, lines, WipeLineWnd_HBlank, lines->screen, heapId);
}

static void WipeLineWnd_RemoveHBlank(WipeHBlank *hblank, WipeLineWnd *lines, u32 heapId) {
    WipeHBlank_RemoveAtVBlank(hblank, lines->screen, heapId);
}

static void WipeLineWnd_CopyTask(TCB *tcb, void *data) {
    WipeLineWnd *lines = data;
    int i;

    for (i = 0; i < 2; i++) {
        sys_memcpy(lines->buf[i].lines, lines->buf[i].prev, sizeof(lines->buf[i].lines));
    }
    GFL_TCBRemove(tcb);
}

static inline void WipeLineWnd_SetOutside(u8 screen, int planes) {
    if (screen == 0) {
        if (GX_IsHBlank()) {
            G2_SetWndOutsidePlane(planes, TRUE);
        }
    } else {
        if (GX_IsHBlank()) {
            G2S_SetWndOutsidePlane(planes, TRUE);
        }
    }
}

static inline void WipeLineWnd_SetInside(u8 screen, u32 window, int planes) {
    if (window == 0) {
        if (screen == 0) {
            if (GX_IsHBlank()) {
                G2_SetWnd0InsidePlane(planes, TRUE);
            }
        } else {
            if (GX_IsHBlank()) {
                G2S_SetWnd0InsidePlane(planes, TRUE);
            }
        }
    } else {
        if (screen == 0) {
            if (GX_IsHBlank()) {
                G2_SetWnd1InsidePlane(planes, TRUE);
            }
        } else {
            if (GX_IsHBlank()) {
                G2S_SetWnd1InsidePlane(planes, TRUE);
            }
        }
    }
}

// Shows the planes inside a window on the lines it covers and outside it on the others
static inline void WipeLineWnd_SetLine(WipeLineWnd *lines, int index, s32 line) {
    WipeLineBuf *buf = &lines->buf[index];

    if (buf->lines[line] == 0) {
        WipeLineWnd_SetOutside(lines->screen, 0x3f);
        WipeLineWnd_SetInside(lines->screen, buf->window, 0x20);
    } else {
        WipeLineWnd_SetOutside(lines->screen, 0x20);
        WipeLineWnd_SetInside(lines->screen, buf->window, 0x3f);
    }
}

static void WipeLineWnd_HBlank(void *data) {
    WipeLineWnd *lines = data;
    s32 line = GX_GetVCount();

    if (line < 192) {
        line++;
        if (line > 191) {
            line -= 192;
        }
        if (lines->count == 1) {
            WipeLineWnd_SetLine(lines, 0, line);
        } else {
            WipeLineWnd_SetLine(lines, 0, line);
            WipeLineWnd_SetLine(lines, 1, line);
        }
    }
}

static void WipeRect_Init(WipeScreen *screen, const WipeRectParam *param) {
    WipeRectWork *work = GFL_HeapAllocate(screen->heapId, sizeof(WipeRectWork), FALSE, "wipe_sub.c", 3196);

    screen->work = work;
    WipeRectWork_Init(work, param, screen->division, screen->sync, screen->screen, screen->wnd);
    if (param->window == 0) {
        WipeSub_SetWndVisible(screen->wnd, 1, work->screen, work->isOut);
    } else {
        WipeSub_SetWndVisible(screen->wnd, 2, work->screen, work->isOut);
    }
    screen->seq++;
}

static BOOL WipeRect_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeRectWork *work = screen->work;

    switch (screen->seq) {
    case 1:
        if (WipeRectWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, screen->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case 2:
        GFL_HeapFree(work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case 3:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeRectWork_Init(WipeRectWork *work, const WipeRectParam *param, s32 division, s32 sync, s32 screen,
                              WipeWnd *wnd) {
    WipeRect_SetSteps(work->cur, work->end, work->step, param->start, param->end, division);
    work->screen = screen;
    work->window = param->window;
    work->division = division;
    work->sync = sync;
    work->syncCount = 0;
    work->wnd = wnd;
    work->isOut = param->isOut;
    WipeSub_SetWnd(wnd, param->inPlanes, param->outPlanes, param->window, screen, param->start[0], param->start[1],
                   param->start[2], param->start[3], work->isOut);
}

static BOOL WipeRectWork_Step(WipeRectWork *work) {
    work->syncCount++;
    if (work->syncCount >= work->sync) {
        work->syncCount = 0;
        if (work->division - 1 > 0) {
            work->division--;
            WipeRect_AddSteps(work->cur, work->step);
        } else {
            WipeWnd_SetPositionAtVBlank(work->wnd, work->end[0], work->end[1], work->end[2], work->end[3], work->window,
                                        work->screen);
            return TRUE;
        }
        WipeWnd_SetPositionAtVBlank(work->wnd, work->cur[0] / 128, work->cur[1] / 128, work->cur[2] / 128,
                                    work->cur[3] / 128, work->window, work->screen);
    }
    return FALSE;
}

static void WipeCircle_Init(WipeScreen *screen, const WipeCircleParam *param) {
    screen->work = GFL_HeapAllocate(screen->heapId, sizeof(WipeCircleWork), FALSE, "wipe_sub.c", 3427);
    WipeCircleWork_Init(screen->work, param, screen->division, screen->sync, screen->screen, screen->wnd,
                        screen->hblank, screen->heapId);
    screen->seq++;
}

static BOOL WipeCircle_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeCircleWork *work = screen->work;

    switch (screen->seq) {
    case 1:
        if (WipeCircleWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, work->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case 2:
        WipeScanWnd_Exit(&work->scan);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case 3:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeCircleWork_Init(WipeCircleWork *work, const WipeCircleParam *param, s32 division, s32 sync, s32 screen,
                                WipeWnd *wnd, WipeHBlank *hblank, u32 heapId) {
    s32 step = WipeSub_GetStep(param->startRadius, param->endRadius, division);
    WipeScanBuf *buf;

    WipeScanWnd_Init(&work->scan, param->window, screen, heapId);
    work->wnd = wnd;
    work->radius = param->startRadius << 7;
    work->cx = param->cx;
    work->cy = param->cy;
    work->step = step;
    work->division = division;
    work->sync = sync;
    work->syncCount = 0;
    work->hblank = hblank;
    work->heapId = heapId;
    work->isOut = param->isOut;
    WipeCircleWork_Compute(work);
    GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
    buf = WipeScanWnd_GetBuf(&work->scan, 0);
    WipeSub_SetWnd(wnd, param->inPlanes, param->outPlanes, param->window, screen, buf->next.left[0], 0,
                   buf->next.right[0], 192, work->isOut);
    if (param->window == 0) {
        WipeSub_SetWndVisible(wnd, 1, screen, work->isOut);
    } else {
        WipeSub_SetWndVisible(wnd, 2, screen, work->isOut);
    }
    WipeHBlank_AddAtVBlank(work->hblank, &work->scan, WipeScanWnd_HBlank, screen, heapId);
}

static BOOL WipeCircleWork_Step(WipeCircleWork *work) {
    work->syncCount++;
    if (work->syncCount >= work->sync) {
        work->syncCount = 0;
        if (work->division - 1 > 0) {
            work->division--;
            work->radius += work->step;
            WipeCircleWork_Compute(work);
            GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
        } else {
            WipeHBlank_RemoveAtVBlank(work->hblank, work->scan.screen, work->heapId);
            return TRUE;
        }
    }
    return FALSE;
}

// The left and right edges of a circle's line y, or 0 and 0 for a line it doesn't reach
static void WipeCircle_GetSpan(s32 radius, s32 cx, s32 cy, s32 y, s32 *x1, s32 *x2) {
    s32 r = radius / 128;
    s32 dy = y - cy;
    s32 dx;

    if (dy < 0) {
        dy = -dy;
    }
    if (dy >= r) {
        *x1 = 0;
        *x2 = 0;
        return;
    }
    dx = FX_Sqrt(FX_Mul(r << FX32_SHIFT, r << FX32_SHIFT) - FX_Mul(dy << FX32_SHIFT, dy << FX32_SHIFT)) >> FX32_SHIFT;
    *x1 = cx - dx;
    if (*x1 < 0) {
        *x1 = 0;
    }
    // BUG: The right edge is measured from the clamped left edge, so a circle whose left edge is off the screen is
    // drawn too narrow. The two circle wipes in the ROM are centered at x 128 and clamp both edges, so it never shows
#ifdef BUGFIX
    *x2 = cx + dx;
#else
    *x2 = *x1 + dx * 2;
#endif
    if (*x2 > 255) {
        *x2 = 255;
    }
}

// The circle is symmetric about cy, so the lines below it copy the ones above
static void WipeCircleWork_Compute(WipeCircleWork *work) {
    WipeScanBuf *buf = WipeScanWnd_GetBuf(&work->scan, 0);
    s32 y;
    s32 x1, x2;

    for (y = 0; y < 192; y++) {
        if (y <= work->cy) {
            WipeCircle_GetSpan(work->radius, work->cx, work->cy, y, &x1, &x2);
        } else if (y <= work->cy * 2) {
            x1 = buf->next.left[work->cy * 2 - y];
            x2 = buf->next.right[work->cy * 2 - y];
        } else {
            WipeCircle_GetSpan(work->radius, work->cx, work->cy, y, &x1, &x2);
        }
        buf->next.left[y] = x1;
        buf->next.right[y] = x2;
    }
}

static void WipeLine_Init(WipeScreen *screen, WipeLinePattern *pattern) {
    screen->work = GFL_HeapAllocate(screen->heapId, sizeof(WipeLineWork), FALSE, "wipe_sub.c", 4948);
    sys_memset(screen->work, 0, sizeof(WipeLineWork));
    WipeLineWork_Init(screen->work, pattern, screen->division, screen->sync, screen->screen, screen->wnd,
                      screen->hblank, screen->heapId);
    screen->seq++;
}

static BOOL WipeLine_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeLineWork *work = screen->work;

    switch (screen->seq) {
    case 1:
        if (WipeLineWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, work->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case 2:
        WipeLineWork_Exit(work);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case 3:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeLineWork_Init(WipeLineWork *work, WipeLinePattern *pattern, s32 division, s32 sync, u32 screen,
                              WipeWnd *wnd, WipeHBlank *hblank, u32 heapId) {
    WipeLineWnd_Init(&work->lines, screen, 1, 0, 0);
    if (pattern->isOut == FALSE) {
        sys_memset(work->lines.buf[0].prev, TRUE, sizeof(work->lines.buf[0].prev));
        sys_memset(work->lines.buf[0].lines, TRUE, sizeof(work->lines.buf[0].lines));
    } else {
        sys_memset(work->lines.buf[0].prev, FALSE, sizeof(work->lines.buf[0].prev));
        sys_memset(work->lines.buf[0].lines, FALSE, sizeof(work->lines.buf[0].lines));
    }
    work->ranges = pattern->ranges;
    work->count = pattern->count;
    work->isOut = pattern->isOut;
    work->heapId = heapId;
    work->division = division;
    work->step = 0;
    work->sync = sync;
    work->syncCount = 0;
    work->wnd = wnd;
    work->hblank = hblank;
    WipeLineWnd_AddHBlank(hblank, &work->lines, heapId);
    if (pattern->isOut == TRUE) {
        WipeSub_SetWnd(wnd, 0x20, 0x3f, 0, screen, 0, 0, 0, 0, pattern->isOut);
    } else {
        WipeSub_SetWnd(wnd, 0x3f, 0x20, 0, screen, 0, 0, 0, 0, pattern->isOut);
    }
    WipeSub_SetWndVisible(wnd, 1, screen, work->isOut);
}

static BOOL WipeLineWork_Step(WipeLineWork *work) {
    work->syncCount++;
    if (work->syncCount >= work->sync) {
        work->syncCount = 0;
        if (work->step + 1 <= work->division) {
            work->step++;
            WipeLineWork_Compute(work);
            WipeLineWnd_StartCopy(&work->lines);
        } else {
            WipeLineWnd_RemoveHBlank(work->hblank, &work->lines, work->heapId);
            return TRUE;
        }
    }
    return FALSE;
}

static void WipeLineWork_Exit(WipeLineWork *work) {
}

static void WipeLineWork_Compute(WipeLineWork *work) {
    int i;

    for (i = 0; i < work->count; i++) {
        WipeLineRange_Apply(&work->ranges[i], &work->lines, work->step, work->division);
    }
}

// Marks a range's lines covered or not on either side of its edge, which moves from start to end over the divisions
static void WipeLineRange_Apply(const WipeLineRange *range, WipeLineWnd *lines, s32 step, s32 division) {
    u32 start = range->start;
    u32 end = range->end;
    // The u8 fields promote to int, so the original's division is signed (_s32_div_f)
    s32 edge = (range->end - range->start) * step / division;
    s32 i, to;
    BOOL covered;

    edge += start;
    if (start <= end) {
        i = start;
        to = end;
        covered = range->covered;
    } else {
        i = end;
        to = start;
        covered = range->covered ? FALSE : TRUE;
    }
    for (; i < to; i++) {
        if (i == edge) {
            if (covered == FALSE) {
                covered = TRUE;
            } else {
                covered = FALSE;
            }
        }
        lines->buf[0].lines[i] = covered;
    }
}
