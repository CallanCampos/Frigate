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
// a circle drawn line by line in the H-blank; and, dead-stripped (below), more line, rectangle and circle patterns,
// pairs of rectangles, a wedge, an hourglass, bands, a sweep, a cross, a clamp and a clock hand. Positions and sizes
// are kept with 7 fraction bits while they move

// DEAD-STRIPPED RECONSTRUCTIONS. The library has 43 patterns, and wipe.c's table uses 12 of them. The original link
// dropped the other 31 handlers and the code only they reached: WipeSub_GetTan, WipeSub_GetTanOffset,
// WipeSub_GetTanOffsets, WipeSub_GetTanHeight, WipeSub_ClampX, WipeSub_SwapWndPlanes and the WipeRectPair, WipeWedge,
// WipeHourglass, WipeBand, WipeSweep, WipeCross, WipeClamp and WipeClock engines. Each dead handler is marked
// "Dead-stripped" below with its pattern type in pret's pokeplatinum, or for the clock its number in pret's
// pokeheartgold, and each run of dead engine code starts with a "Dead-stripped" comment that names where it ends. Only
// the dead handlers' function-local statics stayed, pooled with the live handlers' into the file's .rodata and .data,
// so the code is written back for them, and the build strips it again (config/deadstrip.toml). The dead handlers of
// types 8 to 15, 18 to 37, 40 and 41 and their engines follow pret's pokeplatinum (src/screen_fade_funcs.c); the clock
// wipe (FadeFunc_42) and the plane swap (sub_02010FEC) follow pret's pokeheartgold's asm/unk_0201010C.s, and the
// clock's engine its asm/unk_02012DD8.s. All of it is rewritten in this file's names with BW2's GFL and wipe functions.
// The ROM has none of this code, so none of it can be checked against BW2's. It has no address either: it sits among
// the live functions in the order Platinum's and HGSS's code has, in which BW2's live functions come too. The line
// numbers of its allocations are unknown: each is a guess, spaced among the live ones (2476 to 4948) by the number of
// functions in between, and after 4948 for the clamp and clock wipes, whose code follows the last live allocation.
//
// The swap reads the planes back with WipeWnd_GetInsidePlane and WipeWnd_GetOutsidePlane, which were wipe_wnd.c's: HGSS
// has them in its window module (sub_020132E8 and sub_0201333C in asm/unk_02012DD8.s, between the counterparts of
// WipeWnd_SetOutsidePlane and WipeWnd_SetPosition), where BW2's ROM has neither, as the original link dropped them with
// the swap. wipe_wnd.c has them back, dead-stripped as well.

// An angle in degrees as the 16-bit angle FX_SinIdx and FX_CosIdx take, a full turn being 0xffff
#define WIPE_DEGREES(deg) ((0xffff * (deg)) / 360)

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

// A wedge from the middle of the top edge, opening or closing between two angles
typedef struct {
    u16 startAngle;
    u16 endAngle;
    u8 window;
    u8 inPlanes;
    u8 outPlanes;
    u8 isOut;
} WipeWedgeParam;

// An hourglass whose waist opens or closes as its angle moves between two angles
typedef struct {
    u16 startAngle;
    u16 endAngle;
    u8 window;
    u8 inPlanes;
    u8 outPlanes;
    u8 isOut;
} WipeHourglassParam;

// Bands of lines whose rectangles move from start to end (x1, y1, x2, y2)
typedef struct {
    const u8 (*start)[4];
    const u8 (*end)[4];
    u16 count;
    u16 window;
    u8 inPlanes;
    u8 outPlanes;
    u16 isOut;
} WipeBandParam;

// A line through the center turning from one angle to another, in both windows
typedef struct {
    u16 startAngle;
    u16 endAngle;
    u8 inPlanes;
    u8 outPlanes;
    u16 isOut;
} WipeSweepParam;

// Two lines through the center, at an angle from the vertical and from the horizontal, closing onto each other
typedef struct {
    u16 startAngle;
    u16 endAngle;
    u8 inPlanes;
    u8 outPlanes;
    u16 isOut;
} WipeCrossParam;

// A line pattern and a rectangle one after the other: the rectangle takes rectRate of the divisions
typedef struct {
    WipeRectParam rect;
    WipeLinePattern lines;
    fx32 rectRate;
} WipeClampParam;

// How far a clock hand reaches, in pixels: well past the screen's corners from any center on it
#define WIPE_CLOCK_LENGTH 0xffff

// A clock hand sweeping a full turn about (cx, cy) from angle
typedef struct {
    s16 cx;
    s16 cy;
    s32 angle;
    u8 window;
    u8 inPlanes;
    u8 outPlanes;
    u8 isOut;
} WipeClockParam;

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

typedef struct {
    WipeRectWork rects[2];
} WipeRectPairWork;

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

typedef struct {
    WipeScanWnd scan;
    s32 angle;
    s32 step;
    s32 division;
    s32 sync;
    s32 syncCount;
    BOOL isOut;
    WipeWnd *wnd;
    WipeHBlank *hblank;
    u32 heapId;
} WipeWedgeWork;

typedef struct {
    WipeScanWnd scan;
    // The half width the waist opens to
    fx32 width;
    s32 angle;
    s32 step;
    s32 division;
    s32 sync;
    s32 syncCount;
    BOOL isOut;
    WipeWnd *wnd;
    WipeHBlank *hblank;
    u32 heapId;
} WipeHourglassWork;

// A band's rectangle moving from start to end
typedef struct {
    s32 cur[4];
    s32 step[4];
    s32 end[4];
} WipeBandRect;

typedef struct {
    WipeScanWnd scan;
    WipeBandRect *bands;
    s32 count;
    s32 division;
    s32 sync;
    s32 syncCount;
    BOOL isOut;
    WipeWnd *wnd;
    WipeHBlank *hblank;
    u32 heapId;
} WipeBandWork;

// An angle moving from start over range as the steps go by
typedef struct {
    s32 cur;
    s32 start;
    s32 range;
} WipeSweepAngle;

typedef struct {
    WipeScanWnd scan;
    WipeSweepAngle angle;
    s32 division;
    s32 step;
    s32 sync;
    s32 syncCount;
    BOOL isOut;
    u32 heapId;
    WipeWnd *wnd;
    WipeHBlank *hblank;
} WipeSweepWork;

typedef struct {
    s32 cur;
    s32 start;
    s32 range;
} WipeCrossAngle;

typedef struct {
    WipeScanWnd scan;
    WipeCrossAngle angle;
    s32 division;
    s32 step;
    s32 sync;
    s32 syncCount;
    BOOL isOut;
    u32 heapId;
    WipeWnd *wnd;
    WipeHBlank *hblank;
} WipeCrossWork;

// The parts of a clamp wipe
enum {
    CLAMP_PHASE_FIRST,
    CLAMP_PHASE_SECOND,
    CLAMP_PHASE_DONE,
};

typedef struct {
    WipeRectWork rect;
    WipeLineWork lines;
    WipeClampParam *param;
    // The divisions of the part that runs second
    u8 nextDivision;
    u8 phase;
    u8 isOut;
} WipeClampWork;

typedef struct {
    WipeScanWnd scan;
    s32 cx;
    s32 cy;
    // How far the hand reaches, with 7 fraction bits
    s32 length;
    s32 startAngle;
    s32 angle;
    s32 step;
    s32 division;
    s32 sync;
    s32 syncCount;
    u32 heapId;
    BOOL isOut;
    u32 window;
    s32 screen;
    // Whether the window's planes are still to be swapped when the hand passes half a turn
    BOOL swapPending;
    WipeWnd *wnd;
    WipeHBlank *hblank;
} WipeClockWork;

static fx32 WipeSub_GetTan(s32 angle);
static s32 WipeSub_GetTanOffset(s32 angle, s32 height);
static void WipeSub_GetTanOffsets(s32 angle, s32 *offsets, s32 end, s32 start);
static fx32 WipeSub_GetTanHeight(s32 angle, s32 width);
static s32 WipeSub_GetStep(s32 start, s32 end, s32 division);
static s32 WipeSub_ClampX(s32 x, s32 offset);
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
static void WipeSub_SwapWndPlanes(WipeWnd *wnd, u32 window, u32 screen, BOOL deferred);
static void WipeSub_SetWndVisible(WipeWnd *wnd, u32 visible, s32 screen, BOOL deferred);
static void WipeLineWnd_Init(WipeLineWnd *lines, u32 screen, u32 count, u32 window0, u32 window1);
static void WipeLineWnd_StartCopy(WipeLineWnd *lines);
static void WipeLineWnd_AddHBlank(WipeHBlank *hblank, WipeLineWnd *lines, u32 heapId);
static void WipeLineWnd_RemoveHBlank(WipeHBlank *hblank, WipeLineWnd *lines, u32 heapId);
static void WipeLineWnd_CopyTask(TCB *tcb, void *data);
static void WipeLineWnd_HBlank(void *data);
static void WipeRect_Init(WipeScreen *screen, const WipeRectParam *param);
static BOOL WipeRect_Main(WipeScreen *screen);
static void WipeRectPair_Init(WipeScreen *screen, const WipeRectParam *param0, const WipeRectParam *param1);
static BOOL WipeRectPair_Main(WipeScreen *screen);
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
static void WipeWedge_Init(WipeScreen *screen, const WipeWedgeParam *param);
static BOOL WipeWedge_Main(WipeScreen *screen);
static void WipeWedgeWork_Init(WipeWedgeWork *work, const WipeWedgeParam *param, s32 division, s32 sync, s32 screen,
                               WipeWnd *wnd, WipeHBlank *hblank, u32 heapId);
static BOOL WipeWedgeWork_Step(WipeWedgeWork *work);
static void WipeWedgeWork_Compute(WipeWedgeWork *work);
static void WipeHourglass_Init(WipeScreen *screen, const WipeHourglassParam *param);
static BOOL WipeHourglass_Main(WipeScreen *screen);
static void WipeHourglassWork_Init(WipeHourglassWork *work, const WipeHourglassParam *param, s32 division, s32 sync,
                                   s32 screen, WipeWnd *wnd, WipeHBlank *hblank, u32 heapId);
static BOOL WipeHourglassWork_Step(WipeHourglassWork *work);
static void WipeHourglassWork_Compute(WipeHourglassWork *work);
static void WipeBand_Init(WipeScreen *screen, const WipeBandParam *param);
static BOOL WipeBand_Main(WipeScreen *screen);
static void WipeBandWork_Init(WipeBandWork *work, const WipeBandParam *param, s32 division, s32 sync, s32 screen,
                              WipeWnd *wnd, WipeHBlank *hblank, u32 heapId);
static BOOL WipeBandWork_Step(WipeBandWork *work);
static void WipeBandWork_Free(WipeBandWork *work);
static void WipeBandWork_Compute(WipeBandWork *work);
static void WipeBand_Draw(WipeScanWnd *scan, const s32 *cur);
static void WipeBandWork_Move(WipeBandWork *work);
static void WipeSweep_Init(WipeScreen *screen, const WipeSweepParam *param);
static BOOL WipeSweep_Main(WipeScreen *screen);
static void WipeSweepWork_Init(WipeSweepWork *work, const WipeSweepParam *param, s32 division, s32 sync, s32 screen,
                               WipeWnd *wnd, WipeHBlank *hblank, u32 heapId);
static BOOL WipeSweepWork_Step(WipeSweepWork *work);
static void WipeSweepWork_Exit(WipeSweepWork *work);
static void WipeSweepWork_Compute(WipeSweepWork *work);
static void WipeSweep_SetAngle(WipeSweepAngle *angle, s32 step, s32 division);
static void WipeCross_Init(WipeScreen *screen, const WipeCrossParam *param);
static BOOL WipeCross_Main(WipeScreen *screen);
static void WipeCrossWork_Init(WipeCrossWork *work, const WipeCrossParam *param, s32 division, s32 sync, s32 screen,
                               WipeWnd *wnd, WipeHBlank *hblank, u32 heapId);
static BOOL WipeCrossWork_Step(WipeCrossWork *work);
static void WipeCrossWork_Exit(WipeCrossWork *work);
static void WipeCrossWork_Compute(WipeCrossWork *work);
static void WipeCross_SetAngle(WipeCrossAngle *angle, s32 step, s32 division);
static void WipeLine_Init(WipeScreen *screen, WipeLinePattern *pattern);
static BOOL WipeLine_Main(WipeScreen *screen);
static void WipeLineWork_Init(WipeLineWork *work, WipeLinePattern *pattern, s32 division, s32 sync, u32 screen,
                              WipeWnd *wnd, WipeHBlank *hblank, u32 heapId);
static BOOL WipeLineWork_Step(WipeLineWork *work);
static void WipeLineWork_Exit(WipeLineWork *work);
static void WipeLineWork_Compute(WipeLineWork *work);
static void WipeLineRange_Apply(const WipeLineRange *range, WipeLineWnd *lines, s32 step, s32 division);
static void WipeClamp_Init(WipeScreen *screen, WipeClampParam *param);
static BOOL WipeClamp_Main(WipeScreen *screen);
static void WipeClampWork_InitRectFirst(WipeClampWork *work, WipeClampParam *param, s32 division, s32 sync, s32 screen,
                                        WipeWnd *wnd, WipeHBlank *hblank, u32 heapId);
static BOOL WipeClampWork_StepRectFirst(WipeClampWork *work, WipeScreen *screen);
static void WipeClampWork_InitLinesFirst(WipeClampWork *work, WipeClampParam *param, s32 division, s32 sync, s32 screen,
                                         WipeWnd *wnd, WipeHBlank *hblank, u32 heapId);
static BOOL WipeClampWork_StepLinesFirst(WipeClampWork *work, WipeScreen *screen);
static void WipeClock_Init(WipeScreen *screen, const WipeClockParam *param);
static BOOL WipeClock_Main(WipeScreen *screen);
static void WipeClockWork_Init(WipeClockWork *work, const WipeClockParam *param, s32 division, s32 sync, s32 screen,
                               WipeWnd *wnd, WipeHBlank *hblank, u32 heapId);
static BOOL WipeClockWork_Step(WipeClockWork *work);
static void WipeClock_GetSpan(s32 length, s32 cx, s32 cy, s32 y, s32 startAngle, s32 angle, s32 *x1, s32 *x2);
static void WipeClockWork_Compute(WipeClockWork *work);

// The dead-stripped handlers, in the library's order. They are global, so that MWCC emits them for the strip, and
// declared here rather than in wipe_sub.h, since no other file may call code the build removes
// Scanline wipes from both edges or the middle
BOOL WipeFunc_LinesInwardOut(WipeScreen *screen);
BOOL WipeFunc_LinesOutwardIn(WipeScreen *screen);
BOOL WipeFunc_LinesOutwardOut(WipeScreen *screen);
BOOL WipeFunc_LinesInwardIn(WipeScreen *screen);
// Window rectangles about the middle column
BOOL WipeFunc_ShrinkMiddleOut(WipeScreen *screen);
BOOL WipeFunc_GrowMiddleIn(WipeScreen *screen);
BOOL WipeFunc_GrowMiddleOut(WipeScreen *screen);
BOOL WipeFunc_ShrinkMiddleIn(WipeScreen *screen);
// H-blank circles centered below the screen
BOOL WipeFunc_LowCircleOut(WipeScreen *screen);
BOOL WipeFunc_LowCircleIn(WipeScreen *screen);
// A wedge from the middle of the top edge
BOOL WipeFunc_WedgeOut(WipeScreen *screen);
BOOL WipeFunc_WedgeIn(WipeScreen *screen);
// Window rectangles about the center
BOOL WipeFunc_ShrinkCenterOut(WipeScreen *screen);
BOOL WipeFunc_GrowCenterIn(WipeScreen *screen);
BOOL WipeFunc_GrowCenterOut(WipeScreen *screen);
BOOL WipeFunc_ShrinkCenterIn(WipeScreen *screen);
// An hourglass
BOOL WipeFunc_HourglassOut(WipeScreen *screen);
BOOL WipeFunc_HourglassIn(WipeScreen *screen);
// Bands sliding left and right
BOOL WipeFunc_BandsOut(WipeScreen *screen);
BOOL WipeFunc_BandsIn(WipeScreen *screen);
// Scanline wipes in three bands
BOOL WipeFunc_LinesThreeWayOut(WipeScreen *screen);
BOOL WipeFunc_LinesThreeWayIn(WipeScreen *screen);
// A line turning about the center
BOOL WipeFunc_SweepOut(WipeScreen *screen);
BOOL WipeFunc_SweepIn(WipeScreen *screen);
// Two windows closing onto the lines at 45 degrees through the center
BOOL WipeFunc_CrossOut(WipeScreen *screen);
BOOL WipeFunc_CrossIn(WipeScreen *screen);
// H-blank circles centered above the screen
BOOL WipeFunc_HighCircleOut(WipeScreen *screen);
BOOL WipeFunc_HighCircleIn(WipeScreen *screen);
// Scanlines closing onto a band, then a rectangle closing it to a point
BOOL WipeFunc_ClampOut(WipeScreen *screen);
BOOL WipeFunc_ClampIn(WipeScreen *screen);
// A clock hand sweeping a full turn
BOOL WipeFunc_ClockOut(WipeScreen *screen);

BOOL WipeFunc_BrightnessOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        screen->endCovered = TRUE;
        screen->useBrightness = TRUE;
        WipeBright_Init(screen, TRUE);
        return FALSE;
    }
    return WipeBright_Main(screen);
}

BOOL WipeFunc_BrightnessIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        screen->endCovered = FALSE;
        screen->useBrightness = TRUE;
        WipeBright_Init(screen, FALSE);
        return FALSE;
    }
    return WipeBright_Main(screen);
}

BOOL WipeFunc_LinesDownOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
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
    if (screen->seq == WIPE_SEQ_INIT) {
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
    if (screen->seq == WIPE_SEQ_INIT) {
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
    if (screen->seq == WIPE_SEQ_INIT) {
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
    if (screen->seq == WIPE_SEQ_INIT) {
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
    if (screen->seq == WIPE_SEQ_INIT) {
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

// Dead-stripped: Platinum's type 8
BOOL WipeFunc_LinesInwardOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeLineRange ranges[2] = {
            { 0x00, 0x60, TRUE },
            { 0xc0, 0x60, TRUE },
        };
        static WipeLinePattern pattern = { NULL, 2, TRUE };

        pattern.ranges = ranges;
        Wipe_SetBackdropColor(screen->color);
        WipeLine_Init(screen, &pattern);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeLine_Main(screen);
}

// Dead-stripped: Platinum's type 9
BOOL WipeFunc_LinesOutwardIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeLineRange ranges[2] = {
            { 0x60, 0x00, FALSE },
            { 0x60, 0xc0, FALSE },
        };
        static WipeLinePattern pattern = { NULL, 2, FALSE };

        pattern.ranges = ranges;
        Wipe_SetBackdropColor(screen->color);
        WipeLine_Init(screen, &pattern);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeLine_Main(screen);
}

// Dead-stripped: Platinum's type 10
BOOL WipeFunc_LinesOutwardOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeLineRange ranges[2] = {
            { 0x60, 0x00, TRUE },
            { 0x60, 0xc0, TRUE },
        };
        static WipeLinePattern pattern = { NULL, 2, TRUE };

        pattern.ranges = ranges;
        Wipe_SetBackdropColor(screen->color);
        WipeLine_Init(screen, &pattern);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeLine_Main(screen);
}

// Dead-stripped: Platinum's type 11
BOOL WipeFunc_LinesInwardIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeLineRange ranges[2] = {
            { 0x00, 0x60, FALSE },
            { 0xc0, 0x60, FALSE },
        };
        static WipeLinePattern pattern = { NULL, 2, FALSE };

        pattern.ranges = ranges;
        Wipe_SetBackdropColor(screen->color);
        WipeLine_Init(screen, &pattern);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeLine_Main(screen);
}

// Dead-stripped: Platinum's type 12
BOOL WipeFunc_ShrinkMiddleOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeRectParam param = {
            { 0x00, 0x00, 0xff, 0xc0 }, { 0x80, 0x00, 0x80, 0xc0 }, 0, 0x3f, 0x20, TRUE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRect_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRect_Main(screen);
}

// Dead-stripped: Platinum's type 13
BOOL WipeFunc_GrowMiddleIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeRectParam param = {
            { 0x80, 0x00, 0x80, 0xc0 }, { 0x00, 0x00, 0xff, 0xc0 }, 0, 0x3f, 0x20, FALSE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRect_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRect_Main(screen);
}

// Dead-stripped: Platinum's type 14
BOOL WipeFunc_GrowMiddleOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeRectParam param0 = {
            { 0x80, 0x00, 0x80, 0xc0 }, { 0x00, 0x00, 0x80, 0xc0 }, 0, 0x20, 0x3f, TRUE
        };
        static const WipeRectParam param1 = {
            { 0x80, 0x00, 0x80, 0xc0 }, { 0x80, 0x00, 0xff, 0xc0 }, 1, 0x20, 0x3f, TRUE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRectPair_Init(screen, &param0, &param1);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRectPair_Main(screen);
}

// Dead-stripped: Platinum's type 15
BOOL WipeFunc_ShrinkMiddleIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeRectParam param0 = {
            { 0x00, 0x00, 0x80, 0xc0 }, { 0x80, 0x00, 0x80, 0xc0 }, 0, 0x20, 0x3f, FALSE
        };
        static const WipeRectParam param1 = {
            { 0x80, 0x00, 0xff, 0xc0 }, { 0x80, 0x00, 0x80, 0xc0 }, 1, 0x20, 0x3f, FALSE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRectPair_Init(screen, &param0, &param1);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRectPair_Main(screen);
}

BOOL WipeFunc_CircleOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
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
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeCircleParam param = { 0, 256, 128, 96, 0, 0x3f, 0x20, FALSE };

        Wipe_SetBackdropColor(screen->color);
        WipeCircle_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeCircle_Main(screen);
}

// Dead-stripped: Platinum's type 18
BOOL WipeFunc_LowCircleOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeCircleParam param = { 512, 0, 128, 288, 0, 0x3f, 0x20, TRUE };

        Wipe_SetBackdropColor(screen->color);
        WipeCircle_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeCircle_Main(screen);
}

// Dead-stripped: Platinum's type 19
BOOL WipeFunc_LowCircleIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeCircleParam param = { 0, 512, 128, 288, 0, 0x3f, 0x20, FALSE };

        Wipe_SetBackdropColor(screen->color);
        WipeCircle_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeCircle_Main(screen);
}

// Dead-stripped: Platinum's type 20
BOOL WipeFunc_WedgeOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeWedgeParam param = { WIPE_DEGREES(90), WIPE_DEGREES(0), 0, 0x3f, 0x20, TRUE };

        Wipe_SetBackdropColor(screen->color);
        WipeWedge_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeWedge_Main(screen);
}

// Dead-stripped: Platinum's type 21
BOOL WipeFunc_WedgeIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeWedgeParam param = { WIPE_DEGREES(0), WIPE_DEGREES(90), 0, 0x3f, 0x20, FALSE };

        Wipe_SetBackdropColor(screen->color);
        WipeWedge_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeWedge_Main(screen);
}

// Dead-stripped: Platinum's type 22
BOOL WipeFunc_ShrinkCenterOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeRectParam param = {
            { 0x00, 0x00, 0xff, 0xc0 }, { 0x80, 0x60, 0x80, 0x60 }, 0, 0x3f, 0x20, TRUE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRect_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRect_Main(screen);
}

// Dead-stripped: Platinum's type 23
BOOL WipeFunc_GrowCenterIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeRectParam param = {
            { 0x80, 0x60, 0x80, 0x60 }, { 0x00, 0x00, 0xff, 0xc0 }, 0, 0x3f, 0x20, FALSE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRect_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRect_Main(screen);
}

// Dead-stripped: Platinum's type 24
BOOL WipeFunc_GrowCenterOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeRectParam param = {
            { 0x80, 0x60, 0x80, 0x60 }, { 0x00, 0x00, 0xff, 0xc0 }, 0, 0x20, 0x3f, TRUE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRect_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRect_Main(screen);
}

// Dead-stripped: Platinum's type 25
BOOL WipeFunc_ShrinkCenterIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeRectParam param = {
            { 0x00, 0x00, 0xff, 0xc0 }, { 0x80, 0x60, 0x80, 0x60 }, 0, 0x20, 0x3f, FALSE
        };

        Wipe_SetBackdropColor(screen->color);
        WipeRect_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeRect_Main(screen);
}

// Dead-stripped: Platinum's type 26
BOOL WipeFunc_HourglassOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeHourglassParam param = { WIPE_DEGREES(90), WIPE_DEGREES(0), 0, 0x3f, 0x20, TRUE };

        Wipe_SetBackdropColor(screen->color);
        WipeHourglass_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeHourglass_Main(screen);
}

// Dead-stripped: Platinum's type 27
BOOL WipeFunc_HourglassIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeHourglassParam param = { WIPE_DEGREES(0), WIPE_DEGREES(90), 0, 0x3f, 0x20, FALSE };

        Wipe_SetBackdropColor(screen->color);
        WipeHourglass_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeHourglass_Main(screen);
}

// Dead-stripped: Platinum's type 28
BOOL WipeFunc_BandsOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const u8 start[][4] = {
            { 0x00, 0x00, 0xff, 0x30 },
            { 0x00, 0x2f, 0xff, 0x60 },
            { 0x00, 0x60, 0xff, 0x90 },
            { 0x00, 0x90, 0xff, 0xc0 },
        };
        static const u8 end[][4] = {
            { 0x00, 0x00, 0x00, 0x30 },
            { 0xff, 0x2f, 0xff, 0x60 },
            { 0x00, 0x60, 0x00, 0x90 },
            { 0xff, 0x90, 0xff, 0xc0 },
        };
        WipeBandParam param;

        param.start = start;
        param.end = end;
        param.count = 4;
        param.window = 0;
        param.inPlanes = 0x3f;
        param.outPlanes = 0x20;
        param.isOut = TRUE;
        Wipe_SetBackdropColor(screen->color);
        WipeBand_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeBand_Main(screen);
}

// Dead-stripped: Platinum's type 29
BOOL WipeFunc_BandsIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const u8 start[][4] = {
            { 0xff, 0x00, 0xff, 0x30 },
            { 0x00, 0x2f, 0x00, 0x60 },
            { 0xff, 0x60, 0xff, 0x90 },
            { 0x00, 0x90, 0x00, 0xc0 },
        };
        static const u8 end[][4] = {
            { 0x00, 0x00, 0xff, 0x30 },
            { 0x00, 0x2f, 0xff, 0x60 },
            { 0x00, 0x60, 0xff, 0x90 },
            { 0x00, 0x90, 0xff, 0xc0 },
        };
        WipeBandParam param;

        param.start = start;
        param.end = end;
        param.count = 4;
        param.window = 0;
        param.inPlanes = 0x3f;
        param.outPlanes = 0x20;
        param.isOut = FALSE;
        Wipe_SetBackdropColor(screen->color);
        WipeBand_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeBand_Main(screen);
}

// Dead-stripped: Platinum's type 30
BOOL WipeFunc_LinesThreeWayOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeLineRange ranges[3] = {
            { 0x00, 0x40, TRUE },
            { 0x40, 0x80, TRUE },
            { 0x80, 0xc0, TRUE },
        };
        static WipeLinePattern pattern = { NULL, 3, TRUE };

        pattern.ranges = ranges;
        Wipe_SetBackdropColor(screen->color);
        WipeLine_Init(screen, &pattern);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeLine_Main(screen);
}

// Dead-stripped: Platinum's type 31
BOOL WipeFunc_LinesThreeWayIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeLineRange ranges[3] = {
            { 0x40, 0x00, FALSE },
            { 0x80, 0x40, FALSE },
            { 0xc0, 0x80, FALSE },
        };
        static WipeLinePattern pattern = { NULL, 3, FALSE };

        pattern.ranges = ranges;
        Wipe_SetBackdropColor(screen->color);
        WipeLine_Init(screen, &pattern);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeLine_Main(screen);
}

// Dead-stripped: Platinum's type 32
BOOL WipeFunc_SweepOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        WipeSweepParam param = { WIPE_DEGREES(0), WIPE_DEGREES(179), 0x20, 0x3f, TRUE };

        Wipe_SetBackdropColor(screen->color);
        WipeSweep_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeSweep_Main(screen);
}

// Dead-stripped: Platinum's type 33
BOOL WipeFunc_SweepIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        WipeSweepParam param = { WIPE_DEGREES(0), WIPE_DEGREES(179), 0x3f, 0x20, FALSE };

        Wipe_SetBackdropColor(screen->color);
        WipeSweep_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeSweep_Main(screen);
}

// Dead-stripped: Platinum's type 34
BOOL WipeFunc_CrossOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        WipeCrossParam param = { WIPE_DEGREES(0), WIPE_DEGREES(45), 0x3f, 0x20, TRUE };

        Wipe_SetBackdropColor(screen->color);
        WipeCross_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeCross_Main(screen);
}

// Dead-stripped: Platinum's type 35
BOOL WipeFunc_CrossIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        WipeCrossParam param = { WIPE_DEGREES(0), WIPE_DEGREES(45), 0x20, 0x3f, FALSE };

        Wipe_SetBackdropColor(screen->color);
        WipeCross_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeCross_Main(screen);
}

// Dead-stripped: Platinum's type 36
BOOL WipeFunc_HighCircleOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeCircleParam param = { 512, 0, 128, -80, 0, 0x3f, 0x20, TRUE };

        Wipe_SetBackdropColor(screen->color);
        WipeCircle_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeCircle_Main(screen);
}

// Dead-stripped: Platinum's type 37
BOOL WipeFunc_HighCircleIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeCircleParam param = { 0, 512, 128, -80, 0, 0x3f, 0x20, FALSE };

        Wipe_SetBackdropColor(screen->color);
        WipeCircle_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeCircle_Main(screen);
}

BOOL WipeFunc_GrowRightOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
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
    if (screen->seq == WIPE_SEQ_INIT) {
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

// Dead-stripped: Platinum's type 40
BOOL WipeFunc_ClampOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeLineRange ranges[2] = {
            { 0x00, 0x5e, TRUE },
            { 0xc0, 0x62, TRUE },
        };
        static WipeClampParam param = {
            { { 0x00, 0x5e, 0xff, 0x62 }, { 0x80, 0x60, 0x80, 0x60 }, 0, 0x3f, 0x20, TRUE },
            { NULL, 2, TRUE },
            FX32_CONST(0.7f),
        };

        param.lines.ranges = ranges;
        Wipe_SetBackdropColor(screen->color);
        WipeClamp_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeClamp_Main(screen);
}

// Dead-stripped: Platinum's type 41
BOOL WipeFunc_ClampIn(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeLineRange ranges[2] = {
            { 0x5e, 0x00, FALSE },
            { 0x62, 0xc0, FALSE },
        };
        static WipeClampParam param = {
            { { 0x80, 0x60, 0x80, 0x60 }, { 0x00, 0x5e, 0xff, 0x62 }, 0, 0x3f, 0x20, FALSE },
            { NULL, 2, FALSE },
            FX32_CONST(0.7f),
        };

        param.lines.ranges = ranges;
        Wipe_SetBackdropColor(screen->color);
        WipeClamp_Init(screen, &param);
        screen->endCovered = FALSE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeClamp_Main(screen);
}

// Dead-stripped: HGSS's FadeFunc_42, which Platinum doesn't have
BOOL WipeFunc_ClockOut(WipeScreen *screen) {
    if (screen->seq == WIPE_SEQ_INIT) {
        static const WipeClockParam param = { 128, 96, WIPE_DEGREES(200), 0, 0x20, 0x3f, TRUE };

        Wipe_SetBackdropColor(screen->color);
        WipeClock_Init(screen, &param);
        screen->endCovered = TRUE;
        screen->useBrightness = FALSE;
        return FALSE;
    }
    return WipeClock_Main(screen);
}

// Dead-stripped, to WipeSub_GetTanHeight: the tangent of an angle, and the three helpers after it, which the wedge,
// hourglass, sweep and cross wipes draw their slanted edges with
static fx32 WipeSub_GetTan(s32 angle) {
    return FX_Div(FX_SinIdx(angle), FX_CosIdx(angle));
}

// How far a line at the angle from the vertical moves sideways over a height
static s32 WipeSub_GetTanOffset(s32 angle, s32 height) {
    return FX_Mul(WipeSub_GetTan(angle), height << FX32_SHIFT) >> FX32_SHIFT;
}

// The same for each height from start to before end
static void WipeSub_GetTanOffsets(s32 angle, s32 *offsets, s32 end, s32 start) {
    s32 i;
    fx32 tan = WipeSub_GetTan(angle);

    for (i = start; i < end; i++) {
        offsets[i] = FX_Mul(tan, i << FX32_SHIFT) >> FX32_SHIFT;
    }
}

// The height over which a line at the angle moves half a width sideways
static fx32 WipeSub_GetTanHeight(s32 angle, s32 width) {
    return FX_Div((width / 2) << FX32_SHIFT, WipeSub_GetTan(angle));
}

// The step a value moves each division, with 7 fraction bits
static s32 WipeSub_GetStep(s32 start, s32 end, s32 division) {
    return ((end - start) << 7) / division;
}

// Dead-stripped, only this function: x moved by offset, kept on the screen
static s32 WipeSub_ClampX(s32 x, s32 offset) {
    s32 result = x + offset;

    if (result < 0) {
        result = 0;
    }
    if (result > 255) {
        result = 255;
    }
    return result;
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
    MI_CpuFill8(screen->work, 0, sizeof(WipeBrightWork));
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
    case WIPE_SEQ_MAIN:
        if (WipeBright_Step(work) == TRUE) {
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        GFL_TCBRemove(work->tcb);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
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

        MI_CpuCopy8(&buf->next, &buf->lines, sizeof(WipeScanLines));
    }
    GFL_TCBRemove(tcb);
}

// Leaves the windows as a wipe that ends covered or uncovered does
static void WipeSub_ResetWnd(BOOL isOut, WipeWnd *wnd, s32 screen) {
    if (isOut == FALSE) {
        WipeWnd_SetVisibleAtVBlank(wnd, GX_WNDMASK_NONE, screen);
    } else {
        WipeWnd_SetVisibleAtVBlank(wnd, GX_WNDMASK_W0, screen);
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

// Dead-stripped, only this function: swaps the planes inside a window and outside the windows
static void WipeSub_SwapWndPlanes(WipeWnd *wnd, u32 window, u32 screen, BOOL deferred) {
    GXWndPlane inside = WipeWnd_GetInsidePlane(window, screen);
    GXWndPlane outside = WipeWnd_GetOutsidePlane(screen);

    if (deferred == FALSE) {
        WipeWnd_SetInsidePlane(outside.planeMask, FALSE, window, screen);
        WipeWnd_SetOutsidePlane(inside.planeMask, FALSE, screen);
    } else {
        WipeWnd_SetInsidePlaneAtVBlank(wnd, outside.planeMask, FALSE, window, screen);
        WipeWnd_SetOutsidePlaneAtVBlank(wnd, inside.planeMask, FALSE, screen);
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
    MI_CpuFill8(lines, 0, sizeof(WipeLineWnd));
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
        MI_CpuCopy8(lines->buf[i].lines, lines->buf[i].prev, sizeof(lines->buf[i].lines));
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
    int line = GX_GetVCount();

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
        WipeSub_SetWndVisible(screen->wnd, GX_WNDMASK_W0, work->screen, work->isOut);
    } else {
        WipeSub_SetWndVisible(screen->wnd, GX_WNDMASK_W1, work->screen, work->isOut);
    }
    screen->seq++;
}

static BOOL WipeRect_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeRectWork *work = screen->work;

    switch (screen->seq) {
    case WIPE_SEQ_MAIN:
        if (WipeRectWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, screen->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        GFL_HeapFree(work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
        done = TRUE;
        break;
    }
    return done;
}

// Dead-stripped, to WipeRectPair_Main: two window rectangles moving together, for types 14 and 15
static void WipeRectPair_Init(WipeScreen *screen, const WipeRectParam *param0, const WipeRectParam *param1) {
    WipeRectPairWork *work = GFL_HeapAllocate(screen->heapId, sizeof(WipeRectPairWork), FALSE, "wipe_sub.c", 3266);

    screen->work = work;
    WipeRectWork_Init(&work->rects[0], param0, screen->division, screen->sync, screen->screen, screen->wnd);
    WipeRectWork_Init(&work->rects[1], param1, screen->division, screen->sync, screen->screen, screen->wnd);
    WipeSub_SetWndVisible(screen->wnd, GX_WNDMASK_W0 | GX_WNDMASK_W1, screen->screen, work->rects[0].isOut);
    screen->seq++;
}

static BOOL WipeRectPair_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeRectPairWork *work = screen->work;
    s32 doneCount;

    switch (screen->seq) {
    case WIPE_SEQ_MAIN:
        doneCount = WipeRectWork_Step(&work->rects[0]);
        doneCount += WipeRectWork_Step(&work->rects[1]);
        if (doneCount == 2) {
            WipeSub_ResetWnd(work->rects[0].isOut, screen->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        GFL_HeapFree(work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
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
    case WIPE_SEQ_MAIN:
        if (WipeCircleWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, work->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        WipeScanWnd_Exit(&work->scan);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
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
        WipeSub_SetWndVisible(wnd, GX_WNDMASK_W0, screen, work->isOut);
    } else {
        WipeSub_SetWndVisible(wnd, GX_WNDMASK_W1, screen, work->isOut);
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

// Dead-stripped, to WipeWedgeWork_Compute: the wedge of types 20 and 21
static void WipeWedge_Init(WipeScreen *screen, const WipeWedgeParam *param) {
    screen->work = GFL_HeapAllocate(screen->heapId, sizeof(WipeWedgeWork), FALSE, "wipe_sub.c", 3667);
    WipeWedgeWork_Init(screen->work, param, screen->division, screen->sync, screen->screen, screen->wnd, screen->hblank,
                       screen->heapId);
    screen->seq++;
}

static BOOL WipeWedge_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeWedgeWork *work = screen->work;

    switch (screen->seq) {
    case WIPE_SEQ_MAIN:
        if (WipeWedgeWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, work->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        WipeScanWnd_Exit(&work->scan);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeWedgeWork_Init(WipeWedgeWork *work, const WipeWedgeParam *param, s32 division, s32 sync, s32 screen,
                               WipeWnd *wnd, WipeHBlank *hblank, u32 heapId) {
    WipeScanBuf *buf;

    work->step = WipeSub_GetStep(param->startAngle, param->endAngle, division);
    WipeScanWnd_Init(&work->scan, param->window, screen, heapId);
    work->angle = param->startAngle << 7;
    work->division = division;
    work->sync = sync;
    work->syncCount = 0;
    work->wnd = wnd;
    work->hblank = hblank;
    work->heapId = heapId;
    work->isOut = param->isOut;
    WipeWedgeWork_Compute(work);
    GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
    buf = WipeScanWnd_GetBuf(&work->scan, 0);
    WipeSub_SetWnd(wnd, param->inPlanes, param->outPlanes, param->window, screen, buf->next.left[0], 0,
                   buf->next.right[0], 192, work->isOut);
    if (param->window == 0) {
        WipeSub_SetWndVisible(wnd, GX_WNDMASK_W0, screen, work->isOut);
    } else {
        WipeSub_SetWndVisible(wnd, GX_WNDMASK_W1, screen, work->isOut);
    }
    WipeHBlank_AddAtVBlank(work->hblank, &work->scan, WipeScanWnd_HBlank, screen, heapId);
}

static BOOL WipeWedgeWork_Step(WipeWedgeWork *work) {
    work->syncCount++;
    if (work->syncCount >= work->sync) {
        work->syncCount = 0;
        if (work->division - 1 > 0) {
            work->division--;
            work->angle += work->step;
            WipeWedgeWork_Compute(work);
            GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
        } else {
            WipeHBlank_RemoveAtVBlank(work->hblank, work->scan.screen, work->heapId);
            return TRUE;
        }
    }
    return FALSE;
}

// The wedge's edges leave the middle of the top edge at its angle from the vertical
static void WipeWedgeWork_Compute(WipeWedgeWork *work) {
    s32 y;
    s32 offsets[192];
    WipeScanBuf *buf = WipeScanWnd_GetBuf(&work->scan, 0);

    WipeSub_GetTanOffsets(work->angle / 128, offsets, 192, 0);
    for (y = 0; y < 192; y++) {
        buf->next.left[y] = WipeSub_ClampX(128, -offsets[y]);
        buf->next.right[y] = WipeSub_ClampX(128, offsets[y]);
    }
}

// Dead-stripped, to WipeHourglassWork_Compute: the hourglass of types 26 and 27
static void WipeHourglass_Init(WipeScreen *screen, const WipeHourglassParam *param) {
    screen->work = GFL_HeapAllocate(screen->heapId, sizeof(WipeHourglassWork), FALSE, "wipe_sub.c", 3867);
    WipeHourglassWork_Init(screen->work, param, screen->division, screen->sync, screen->screen, screen->wnd,
                           screen->hblank, screen->heapId);
    screen->seq++;
}

static BOOL WipeHourglass_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeHourglassWork *work = screen->work;

    switch (screen->seq) {
    case WIPE_SEQ_MAIN:
        if (WipeHourglassWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, work->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        WipeScanWnd_Exit(&work->scan);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeHourglassWork_Init(WipeHourglassWork *work, const WipeHourglassParam *param, s32 division, s32 sync,
                                   s32 screen, WipeWnd *wnd, WipeHBlank *hblank, u32 heapId) {
    s32 step = (param->endAngle - param->startAngle) / division;
    WipeScanBuf *buf;

    WipeScanWnd_Init(&work->scan, param->window, screen, heapId);
    work->width = 128 * FX32_ONE;
    work->angle = param->startAngle;
    work->step = step;
    work->division = division;
    work->sync = sync;
    work->syncCount = 0;
    work->wnd = wnd;
    work->hblank = hblank;
    work->heapId = heapId;
    work->isOut = param->isOut;
    WipeHourglassWork_Compute(work);
    GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
    buf = WipeScanWnd_GetBuf(&work->scan, 0);
    WipeSub_SetWnd(wnd, param->inPlanes, param->outPlanes, param->window, screen, buf->next.left[96], 0,
                   buf->next.right[96], 192, work->isOut);
    if (param->window == 0) {
        WipeSub_SetWndVisible(wnd, GX_WNDMASK_W0, screen, work->isOut);
    } else {
        WipeSub_SetWndVisible(wnd, GX_WNDMASK_W1, screen, work->isOut);
    }
    WipeHBlank_AddAtVBlank(work->hblank, &work->scan, WipeScanWnd_HBlank, screen, heapId);
}

static BOOL WipeHourglassWork_Step(WipeHourglassWork *work) {
    work->syncCount++;
    if (work->syncCount >= work->sync) {
        work->syncCount = 0;
        if (work->division - 1 > 0) {
            work->division--;
            work->angle += work->step;
            WipeHourglassWork_Compute(work);
            GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
        } else {
            WipeHBlank_RemoveAtVBlank(work->hblank, work->scan.screen, work->heapId);
            return TRUE;
        }
    }
    return FALSE;
}

// The waist in the middle line is as wide as the sine of the angle, and the sides widen from it to the top and bottom
// edges at a slope that steepens as the waist opens. The halves above and below the middle mirror each other
static void WipeHourglassWork_Compute(WipeHourglassWork *work) {
    s32 y;
    s32 height;
    s32 slope;
    s32 offsets[192];
    s32 dist;
    s32 waist;
    s32 half;
    s32 x1, x2;
    WipeScanBuf *buf = WipeScanWnd_GetBuf(&work->scan, 0);

    waist = FX_Mul(FX_SinIdx(work->angle), work->width) >> FX32_SHIFT;
    slope = 180 - ((waist * 2) / 21 + 1) * 2;
    slope = WIPE_DEGREES(slope) / 2;
    height = WipeSub_GetTanHeight(slope, 256) >> FX32_SHIFT;
    WipeSub_GetTanOffsets(slope, offsets, height, 0);
    for (y = 0; y < 96; y++) {
        dist = height - (y + 1);
        half = waist;
        if (dist > 0 && offsets[dist] > half) {
            half = offsets[dist];
        }
        x1 = WipeSub_ClampX(128, -half);
        x2 = WipeSub_ClampX(128, half);
        buf->next.left[y] = x1;
        buf->next.right[y] = x2;
        buf->next.left[191 - y] = x1;
        buf->next.right[191 - y] = x2;
    }
}

// Dead-stripped, to WipeBandWork_Move: the bands of types 28 and 29
static void WipeBand_Init(WipeScreen *screen, const WipeBandParam *param) {
    screen->work = GFL_HeapAllocate(screen->heapId, sizeof(WipeBandWork), FALSE, "wipe_sub.c", 4067);
    WipeBandWork_Init(screen->work, param, screen->division, screen->sync, screen->screen, screen->wnd, screen->hblank,
                      screen->heapId);
    screen->seq++;
}

static BOOL WipeBand_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeBandWork *work = screen->work;

    switch (screen->seq) {
    case WIPE_SEQ_MAIN:
        if (WipeBandWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, work->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        WipeBandWork_Free(work);
        WipeScanWnd_Exit(&work->scan);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeBandWork_Init(WipeBandWork *work, const WipeBandParam *param, s32 division, s32 sync, s32 screen,
                              WipeWnd *wnd, WipeHBlank *hblank, u32 heapId) {
    s32 i;
    WipeScanBuf *buf;

    work->bands = GFL_HeapAllocate(heapId, sizeof(WipeBandRect) * param->count, FALSE, "wipe_sub.c", 4147);
    work->count = param->count;
    for (i = 0; i < param->count; i++) {
        WipeRect_SetSteps(work->bands[i].cur, work->bands[i].end, work->bands[i].step, param->start[i], param->end[i],
                          division);
    }
    WipeScanWnd_Init(&work->scan, param->window, screen, heapId);
    work->division = division;
    work->sync = sync;
    work->syncCount = 0;
    work->wnd = wnd;
    work->hblank = hblank;
    work->heapId = heapId;
    work->isOut = param->isOut;
    WipeBandWork_Compute(work);
    GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
    buf = WipeScanWnd_GetBuf(&work->scan, 0);
    WipeSub_SetWnd(wnd, param->inPlanes, param->outPlanes, param->window, screen, buf->next.left[0], 0,
                   buf->next.right[0], 192, work->isOut);
    if (param->window == 0) {
        WipeSub_SetWndVisible(work->wnd, GX_WNDMASK_W0, screen, work->isOut);
    } else {
        WipeSub_SetWndVisible(work->wnd, GX_WNDMASK_W1, screen, work->isOut);
    }
    WipeHBlank_AddAtVBlank(work->hblank, &work->scan, WipeScanWnd_HBlank, screen, heapId);
}

static BOOL WipeBandWork_Step(WipeBandWork *work) {
    work->syncCount++;
    if (work->syncCount >= work->sync) {
        work->syncCount = 0;
        if (work->division - 1 > 0) {
            work->division--;
            WipeBandWork_Move(work);
            WipeBandWork_Compute(work);
            GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
        } else {
            WipeHBlank_RemoveAtVBlank(work->hblank, work->scan.screen, work->heapId);
            return TRUE;
        }
    }
    return FALSE;
}

static void WipeBandWork_Free(WipeBandWork *work) {
    GFL_HeapFree(work->bands);
    work->bands = NULL;
}

// Draws the bands from the last, so the first ones are drawn over the others where they meet
static void WipeBandWork_Compute(WipeBandWork *work) {
    s32 i;
    WipeScanBuf *buf = WipeScanWnd_GetBuf(&work->scan, 0);

    MI_CpuFill8(&buf->next, 0, sizeof(WipeScanLines));
    for (i = work->count - 1; i >= 0; i--) {
        WipeBand_Draw(&work->scan, work->bands[i].cur);
    }
}

// Sets the edges of a band's lines to its rectangle's
static void WipeBand_Draw(WipeScanWnd *scan, const s32 *cur) {
    s32 y;
    WipeScanBuf *buf = WipeScanWnd_GetBuf(scan, 0);
    s32 pos[4];

    pos[0] = cur[0] / 128;
    pos[1] = cur[1] / 128;
    pos[2] = cur[2] / 128;
    pos[3] = cur[3] / 128;
    for (y = pos[1]; y < pos[3]; y++) {
        buf->next.left[y] = pos[0];
        buf->next.right[y] = pos[2];
    }
}

static void WipeBandWork_Move(WipeBandWork *work) {
    s32 i;

    for (i = 0; i < work->count; i++) {
        WipeRect_AddSteps(work->bands[i].cur, work->bands[i].step);
    }
}

// Dead-stripped, to WipeSweep_SetAngle: the sweep of types 32 and 33
static void WipeSweep_Init(WipeScreen *screen, const WipeSweepParam *param) {
    WipeSweepWork *work;

    screen->work = GFL_HeapAllocate(screen->heapId, sizeof(WipeSweepWork), FALSE, "wipe_sub.c", 4387);
    MI_CpuFill8(screen->work, 0, sizeof(WipeSweepWork));
    work = screen->work;
    WipeSweepWork_Init(work, param, screen->division, screen->sync, screen->screen, screen->wnd, screen->hblank,
                       screen->heapId);
    screen->seq++;
}

static BOOL WipeSweep_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeSweepWork *work = screen->work;

    switch (screen->seq) {
    case WIPE_SEQ_MAIN:
        if (WipeSweepWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, work->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        WipeSweepWork_Exit(work);
        WipeScanWnd_Exit(&work->scan);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeSweepWork_Init(WipeSweepWork *work, const WipeSweepParam *param, s32 division, s32 sync, s32 screen,
                               WipeWnd *wnd, WipeHBlank *hblank, u32 heapId) {
    WipeScanBuf *buf0;
    WipeScanBuf *buf1;

    work->angle.cur = 0;
    work->angle.start = param->startAngle;
    work->angle.range = param->endAngle - param->startAngle;
    WipeScanWnd_Init(&work->scan, 2, screen, heapId);
    work->division = division;
    work->step = 0;
    work->sync = sync;
    work->syncCount = 0;
    work->wnd = wnd;
    work->hblank = hblank;
    work->heapId = heapId;
    work->isOut = param->isOut;
    WipeSweep_SetAngle(&work->angle, work->step, work->division);
    WipeSweepWork_Compute(work);
    GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
    buf0 = WipeScanWnd_GetBuf(&work->scan, 0);
    buf1 = WipeScanWnd_GetBuf(&work->scan, 1);
    WipeSub_SetWnd(wnd, param->inPlanes, param->outPlanes, 0, screen, buf0->next.left[0], 0, buf0->next.right[0], 192,
                   work->isOut);
    WipeSub_SetWnd(wnd, param->inPlanes, param->outPlanes, 1, screen, buf1->next.left[0], 0, buf1->next.right[0], 192,
                   work->isOut);
    WipeSub_SetWndVisible(wnd, GX_WNDMASK_W0 | GX_WNDMASK_W1, screen, work->isOut);
    WipeHBlank_AddAtVBlank(work->hblank, &work->scan, WipeScanWnd_HBlank, screen, heapId);
}

static BOOL WipeSweepWork_Step(WipeSweepWork *work) {
    work->syncCount++;
    if (work->syncCount >= work->sync) {
        work->syncCount = 0;
        if (work->step + 1 <= work->division) {
            work->step++;
            WipeSweep_SetAngle(&work->angle, work->step, work->division);
            WipeSweepWork_Compute(work);
            GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
        } else {
            WipeHBlank_RemoveAtVBlank(work->hblank, work->scan.screen, work->heapId);
            return TRUE;
        }
    }
    return FALSE;
}

static void WipeSweepWork_Exit(WipeSweepWork *work) {
}

// A line through the center turns from the vertical: window 0 fills the lower left quarter behind it and window 1 the
// upper right one, and past a right angle the upper left and lower right quarters
static void WipeSweepWork_Compute(WipeSweepWork *work) {
    WipeScanBuf *buf0;
    WipeScanBuf *buf1;
    u16 angle;
    s32 offset;
    s32 y;

    angle = work->angle.cur % WIPE_DEGREES(90);
    buf0 = WipeScanWnd_GetBuf(&work->scan, 0);
    buf1 = WipeScanWnd_GetBuf(&work->scan, 1);
    for (y = 0; y < 96; y++) {
        if (work->angle.cur < WIPE_DEGREES(90)) {
            offset = WipeSub_GetTanOffset(angle, 96 - y);
            if (offset > 127) {
                offset = 127;
            }
            buf0->next.left[191 - y] = 128 - offset;
            buf0->next.right[191 - y] = 128;
            buf1->next.left[y] = 128;
            buf1->next.right[y] = 128 + offset;
        } else {
            buf0->next.left[191 - y] = 0;
            buf0->next.right[191 - y] = 128;
            buf1->next.left[y] = 128;
            buf1->next.right[y] = 255;
        }
    }
    for (y = 96; y < 192; y++) {
        if (work->angle.cur < WIPE_DEGREES(90)) {
            buf0->next.left[191 - y] = 128;
            buf0->next.right[191 - y] = 128;
            buf1->next.left[y] = 128;
            buf1->next.right[y] = 128;
        } else {
            offset = WipeSub_GetTanOffset(WIPE_DEGREES(90) - angle, y - 96);
            if (offset > 127) {
                offset = 127;
            }
            buf0->next.left[191 - y] = 0;
            buf0->next.right[191 - y] = 128 - offset;
            buf1->next.left[y] = 128 + offset;
            buf1->next.right[y] = 255;
        }
    }
}

static void WipeSweep_SetAngle(WipeSweepAngle *angle, s32 step, s32 division) {
    angle->cur = angle->range * step / division + angle->start;
}

// Dead-stripped, to WipeCross_SetAngle: the cross of types 34 and 35
static void WipeCross_Init(WipeScreen *screen, const WipeCrossParam *param) {
    WipeCrossWork *work;

    screen->work = GFL_HeapAllocate(screen->heapId, sizeof(WipeCrossWork), FALSE, "wipe_sub.c", 4667);
    MI_CpuFill8(screen->work, 0, sizeof(WipeCrossWork));
    work = screen->work;
    WipeCrossWork_Init(work, param, screen->division, screen->sync, screen->screen, screen->wnd, screen->hblank,
                       screen->heapId);
    screen->seq++;
}

static BOOL WipeCross_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeCrossWork *work = screen->work;

    switch (screen->seq) {
    case WIPE_SEQ_MAIN:
        if (WipeCrossWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, work->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        WipeCrossWork_Exit(work);
        WipeScanWnd_Exit(&work->scan);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeCrossWork_Init(WipeCrossWork *work, const WipeCrossParam *param, s32 division, s32 sync, s32 screen,
                               WipeWnd *wnd, WipeHBlank *hblank, u32 heapId) {
    work->angle.cur = param->startAngle;
    work->angle.start = param->startAngle;
    work->angle.range = param->endAngle - param->startAngle;
    WipeScanWnd_Init(&work->scan, 2, screen, heapId);
    work->division = division;
    work->step = 0;
    work->sync = sync;
    work->syncCount = 0;
    work->wnd = wnd;
    work->hblank = hblank;
    work->heapId = heapId;
    work->isOut = param->isOut;
    WipeCrossWork_Compute(work);
    GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
    WipeSub_SetWnd(wnd, param->inPlanes, param->outPlanes, 0, screen, 0, 0, 255, 192, work->isOut);
    WipeSub_SetWnd(wnd, param->inPlanes, param->outPlanes, 1, screen, 0, 0, 255, 192, work->isOut);
    WipeSub_SetWndVisible(wnd, GX_WNDMASK_W0 | GX_WNDMASK_W1, screen, work->isOut);
    WipeHBlank_AddAtVBlank(work->hblank, &work->scan, WipeScanWnd_HBlank, screen, heapId);
}

static BOOL WipeCrossWork_Step(WipeCrossWork *work) {
    work->syncCount++;
    if (work->syncCount >= work->sync) {
        work->syncCount = 0;
        if (work->step + 1 <= work->division) {
            work->step++;
            WipeCross_SetAngle(&work->angle, work->step, work->division);
            WipeCrossWork_Compute(work);
            GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
        } else {
            WipeHBlank_RemoveAtVBlank(work->hblank, work->scan.screen, work->heapId);
            return TRUE;
        }
    }
    return FALSE;
}

static void WipeCrossWork_Exit(WipeCrossWork *work) {
}

// Window 0 lies left of the center and window 1 right of it, each between a line at the angle from the vertical and
// one at the angle from the horizontal, mirrored above and below the middle
static void WipeCrossWork_Compute(WipeCrossWork *work) {
    WipeScanBuf *buf0;
    WipeScanBuf *buf1;
    u16 angle;
    s32 inner, outer;
    s32 y;

    angle = work->angle.cur;
    buf0 = WipeScanWnd_GetBuf(&work->scan, 0);
    buf1 = WipeScanWnd_GetBuf(&work->scan, 1);
    for (y = 0; y < 96; y++) {
        inner = WipeSub_GetTanOffset(angle, 96 - y);
        outer = WipeSub_GetTanOffset(WIPE_DEGREES(90) - angle, 96 - y);
        if (inner > 127) {
            inner = 127;
        }
        if (outer > 127) {
            outer = 127;
        }
        buf0->next.left[y] = 128 - outer;
        buf0->next.right[y] = 128 - inner;
        buf0->next.left[191 - y] = 128 - outer;
        buf0->next.right[191 - y] = 128 - inner;
        buf1->next.left[y] = 128 + inner;
        buf1->next.right[y] = 128 + outer;
        buf1->next.left[191 - y] = 128 + inner;
        buf1->next.right[191 - y] = 128 + outer;
    }
}

static void WipeCross_SetAngle(WipeCrossAngle *angle, s32 step, s32 division) {
    angle->cur = angle->range * step / division + angle->start;
}

static void WipeLine_Init(WipeScreen *screen, WipeLinePattern *pattern) {
    screen->work = GFL_HeapAllocate(screen->heapId, sizeof(WipeLineWork), FALSE, "wipe_sub.c", 4948);
    MI_CpuFill8(screen->work, 0, sizeof(WipeLineWork));
    WipeLineWork_Init(screen->work, pattern, screen->division, screen->sync, screen->screen, screen->wnd,
                      screen->hblank, screen->heapId);
    screen->seq++;
}

static BOOL WipeLine_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeLineWork *work = screen->work;

    switch (screen->seq) {
    case WIPE_SEQ_MAIN:
        if (WipeLineWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, work->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        WipeLineWork_Exit(work);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeLineWork_Init(WipeLineWork *work, WipeLinePattern *pattern, s32 division, s32 sync, u32 screen,
                              WipeWnd *wnd, WipeHBlank *hblank, u32 heapId) {
    WipeLineWnd_Init(&work->lines, screen, 1, 0, 0);
    if (pattern->isOut == FALSE) {
        MI_CpuFill8(work->lines.buf[0].prev, TRUE, sizeof(work->lines.buf[0].prev));
        MI_CpuFill8(work->lines.buf[0].lines, TRUE, sizeof(work->lines.buf[0].lines));
    } else {
        MI_CpuFill8(work->lines.buf[0].prev, FALSE, sizeof(work->lines.buf[0].prev));
        MI_CpuFill8(work->lines.buf[0].lines, FALSE, sizeof(work->lines.buf[0].lines));
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
    WipeSub_SetWndVisible(wnd, GX_WNDMASK_W0, screen, work->isOut);
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

// Dead-stripped, to WipeClampWork_StepLinesFirst: the clamp of types 40 and 41, a line pattern closing onto a band
// and a rectangle shrinking it to a point when covering, and the same backwards when uncovering
static void WipeClamp_Init(WipeScreen *screen, WipeClampParam *param) {
    WipeClampWork *work;

    screen->work = GFL_HeapAllocate(screen->heapId, sizeof(WipeClampWork), FALSE, "wipe_sub.c", 5228);
    MI_CpuFill8(screen->work, 0, sizeof(WipeClampWork));
    work = screen->work;
    if (param->rect.isOut == FALSE) {
        WipeClampWork_InitRectFirst(work, param, screen->division, screen->sync, screen->screen, screen->wnd,
                                    screen->hblank, screen->heapId);
    } else {
        WipeClampWork_InitLinesFirst(work, param, screen->division, screen->sync, screen->screen, screen->wnd,
                                     screen->hblank, screen->heapId);
    }
    screen->seq++;
}

static BOOL WipeClamp_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeClampWork *work = screen->work;
    BOOL finished;

    switch (screen->seq) {
    case WIPE_SEQ_MAIN:
        if (work->isOut == FALSE) {
            finished = WipeClampWork_StepRectFirst(work, screen);
        } else {
            finished = WipeClampWork_StepLinesFirst(work, screen);
        }
        if (finished == TRUE) {
            WipeSub_ResetWnd(screen->endCovered, screen->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeClampWork_InitRectFirst(WipeClampWork *work, WipeClampParam *param, s32 division, s32 sync, s32 screen,
                                        WipeWnd *wnd, WipeHBlank *hblank, u32 heapId) {
    s32 rectDivision = FX_Mul(division * FX32_ONE, param->rectRate) >> FX32_SHIFT;

    work->nextDivision = division - rectDivision;
    work->param = param;
    work->isOut = param->rect.isOut;
    WipeRectWork_Init(&work->rect, &param->rect, rectDivision, sync, screen, wnd);
    if (param->rect.window == 0) {
        WipeSub_SetWndVisible(wnd, GX_WNDMASK_W0, screen, param->rect.isOut);
    } else {
        WipeSub_SetWndVisible(wnd, GX_WNDMASK_W1, screen, param->rect.isOut);
    }
    work->phase = CLAMP_PHASE_FIRST;
}

static BOOL WipeClampWork_StepRectFirst(WipeClampWork *work, WipeScreen *screen) {
    BOOL done = FALSE;

    switch (work->phase) {
    case CLAMP_PHASE_FIRST:
        if (WipeRectWork_Step(&work->rect) == TRUE) {
            work->phase++;
            WipeLineWork_Init(&work->lines, &work->param->lines, work->nextDivision, screen->sync, screen->screen,
                              screen->wnd, screen->hblank, screen->heapId);
        }
        break;
    case CLAMP_PHASE_SECOND:
        if (WipeLineWork_Step(&work->lines) == TRUE) {
            done = TRUE;
            work->phase++;
        }
        break;
    case CLAMP_PHASE_DONE:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeClampWork_InitLinesFirst(WipeClampWork *work, WipeClampParam *param, s32 division, s32 sync, s32 screen,
                                         WipeWnd *wnd, WipeHBlank *hblank, u32 heapId) {
    s32 lineDivision;

    work->nextDivision = FX_Mul(division * FX32_ONE, param->rectRate) >> FX32_SHIFT;
    lineDivision = division - work->nextDivision;
    work->param = param;
    work->isOut = param->rect.isOut;
    WipeLineWork_Init(&work->lines, &work->param->lines, lineDivision, sync, screen, wnd, hblank, heapId);
    work->phase = CLAMP_PHASE_FIRST;
}

static BOOL WipeClampWork_StepLinesFirst(WipeClampWork *work, WipeScreen *screen) {
    BOOL done = FALSE;

    switch (work->phase) {
    case CLAMP_PHASE_FIRST:
        if (WipeLineWork_Step(&work->lines) == TRUE) {
            work->phase++;
            WipeRectWork_Init(&work->rect, &work->param->rect, work->nextDivision, screen->sync, screen->screen,
                              screen->wnd);
            if (work->param->rect.window == 0) {
                WipeSub_SetWndVisible(screen->wnd, GX_WNDMASK_W0, screen->screen, work->param->rect.isOut);
            } else {
                WipeSub_SetWndVisible(screen->wnd, GX_WNDMASK_W1, screen->screen, work->param->rect.isOut);
            }
        }
        break;
    case CLAMP_PHASE_SECOND:
        if (WipeRectWork_Step(&work->rect) == TRUE) {
            done = TRUE;
            work->phase++;
        }
        break;
    case CLAMP_PHASE_DONE:
        done = TRUE;
        break;
    }
    return done;
}

// Dead-stripped, to the end of the file: the clock of HGSS's type 42, a hand sweeping a full turn about a point
static void WipeClock_Init(WipeScreen *screen, const WipeClockParam *param) {
    screen->work = GFL_HeapAllocate(screen->heapId, sizeof(WipeClockWork), FALSE, "wipe_sub.c", 5468);
    WipeClockWork_Init(screen->work, param, screen->division, screen->sync, screen->screen, screen->wnd, screen->hblank,
                       screen->heapId);
    screen->seq++;
}

static BOOL WipeClock_Main(WipeScreen *screen) {
    BOOL done = FALSE;
    WipeClockWork *work = screen->work;

    switch (screen->seq) {
    case WIPE_SEQ_MAIN:
        if (WipeClockWork_Step(work) == TRUE) {
            WipeSub_ResetWnd(work->isOut, work->wnd, screen->screen);
            screen->seq++;
        }
        break;
    case WIPE_SEQ_EXIT:
        WipeScanWnd_Exit(&work->scan);
        GFL_HeapFree(screen->work);
        screen->work = NULL;
        screen->seq++;
        done = TRUE;
        break;
    case WIPE_SEQ_DONE:
        done = TRUE;
        break;
    }
    return done;
}

static void WipeClockWork_Init(WipeClockWork *work, const WipeClockParam *param, s32 division, s32 sync, s32 screen,
                               WipeWnd *wnd, WipeHBlank *hblank, u32 heapId) {
    WipeScanBuf *buf;

    WipeScanWnd_Init(&work->scan, param->window, screen, heapId);
    work->length = WIPE_CLOCK_LENGTH << 7;
    work->cx = param->cx;
    work->cy = param->cy;
    work->startAngle = param->angle;
    work->angle = param->angle;
    work->step = WIPE_DEGREES(360) / division;
    work->division = division;
    work->sync = sync;
    work->syncCount = 0;
    work->wnd = wnd;
    work->hblank = hblank;
    work->heapId = heapId;
    work->isOut = param->isOut;
    work->window = param->window;
    work->screen = screen;
    work->swapPending = TRUE;
    WipeClockWork_Compute(work);
    GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
    buf = WipeScanWnd_GetBuf(&work->scan, 0);
    WipeSub_SetWnd(wnd, param->inPlanes, param->outPlanes, param->window, screen, buf->next.left[0], 0,
                   buf->next.right[0], 192, work->isOut);
    if (param->window == 0) {
        WipeSub_SetWndVisible(wnd, GX_WNDMASK_W0, screen, work->isOut);
    } else {
        WipeSub_SetWndVisible(wnd, GX_WNDMASK_W1, screen, work->isOut);
    }
    WipeHBlank_AddAtVBlank(work->hblank, &work->scan, WipeScanWnd_HBlank, screen, heapId);
}

static BOOL WipeClockWork_Step(WipeClockWork *work) {
    work->syncCount++;
    if (work->syncCount >= work->sync) {
        work->syncCount = 0;
        if (work->division - 1 > 0) {
            work->angle += work->step;
            work->division--;
            while (work->angle >= WIPE_DEGREES(360)) {
                work->angle -= WIPE_DEGREES(360);
            }
            while (work->angle < 0) {
                work->angle += WIPE_DEGREES(360);
            }
            // Once the hand passes half a turn, the window's inside and outside trade places
            if (work->angle >= WIPE_DEGREES(180) && work->angle < work->startAngle && work->swapPending == TRUE) {
                WipeSub_SwapWndPlanes(work->wnd, work->window, work->screen, work->isOut);
                work->swapPending = FALSE;
            }
            WipeClockWork_Compute(work);
            GFL_VBlankTCBAdd(WipeScanWnd_CopyTask, &work->scan, 0x3ff);
        } else {
            WipeHBlank_RemoveAtVBlank(work->hblank, work->scan.screen, work->heapId);
            return TRUE;
        }
    }
    return FALSE;
}

// The left and right edges on line y of what the hand has swept from startAngle to angle about (cx, cy), from where
// the line through the center and each end of the hand crosses it. As in HGSS's code, the hand's length is passed but
// not read: the constant it was set from is used
static void WipeClock_GetSpan(s32 length, s32 cx, s32 cy, s32 y, s32 startAngle, s32 angle, s32 *x1, s32 *x2) {
    s32 startX = cx + ((WIPE_CLOCK_LENGTH * FX_CosIdx(startAngle)) >> FX32_SHIFT);
    s32 startY = cy + ((WIPE_CLOCK_LENGTH * FX_SinIdx(startAngle)) >> FX32_SHIFT);
    s32 endX = cx + ((WIPE_CLOCK_LENGTH * FX_CosIdx(angle)) >> FX32_SHIFT);
    s32 endY = cy + ((WIPE_CLOCK_LENGTH * FX_SinIdx(angle)) >> FX32_SHIFT);
    s32 top, bottom;
    s32 x;

    // Half a turn covers the lines above the center
    if (angle - startAngle == WIPE_DEGREES(180)) {
        if (y >= 0 && y < cy) {
            *x1 = 0;
            *x2 = 255;
        }
        return;
    }
    top = startY < cy ? startY : cy;
    bottom = startY > cy ? startY : cy;
    if (angle >= 0 && angle < WIPE_DEGREES(180)) {
        // The hand points down, so the swept part runs to the right edge
        if (top <= y && y <= bottom) {
            *x1 = startX + (y - startY) * (startX - cx) / (startY - cy);
        } else {
            *x1 = endX + (y - endY) * (endX - cx) / (endY - cy);
        }
        if (*x1 > 255) {
            *x1 = 255;
        } else if (*x1 < 0) {
            *x1 = 0;
        }
        *x2 = 255;
        return;
    }
    if (top <= y && y <= bottom) {
        *x1 = startX + (y - startY) * (startX - cx) / (startY - cy);
        if (*x1 > 255) {
            *x1 = 255;
        } else if (*x1 < 0) {
            *x1 = 0;
        }
    } else {
        *x1 = 0;
    }
    top = endY < cy ? endY : cy;
    bottom = endY > cy ? endY : cy;
    if (top <= y && y <= bottom) {
        *x2 = endX + (y - endY) * (endX - cx) / (endY - cy);
        if (*x2 > 255) {
            *x2 = 255;
        } else if (*x2 < 0) {
            *x2 = 0;
        }
    } else {
        *x2 = *x1;
    }
    if (*x1 > *x2) {
        x = *x1;
        *x1 = *x2;
        *x2 = x;
    }
}

static void WipeClockWork_Compute(WipeClockWork *work) {
    WipeScanBuf *buf = WipeScanWnd_GetBuf(&work->scan, 0);
    s32 y;
    s32 x1, x2;

    for (y = 0; y < 192; y++) {
        WipeClock_GetSpan(work->length, work->cx, work->cy, y, work->startAngle, work->angle, &x1, &x2);
        buf->next.left[y] = x1;
        buf->next.right[y] = x2;
    }
}
