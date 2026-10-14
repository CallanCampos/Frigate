#include "types.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/os.h"
#include "nitro/tp.h"

// A sampling attempt is retried this many times before the sample is dropped
#define SAMPLING_RETRIES 32

#define AUTO_SAMPLES 9

// A circle in a table of TouchRect
typedef struct {
    u8 code;
    u8 x;
    u8 y;
    u8 radius;
} TouchCircle;

struct TouchpadManager {
    u8 unk0[0xc];
    TPData samples[AUTO_SAMPLES];
    u8 unk54[4];
    u16 autoSampling;
    // Whether auto sampling is stopped while the system sleeps
    u16 stopped;
    u16 x;
    u16 y;
    u16 pressed;
    u16 held;
    // The input of two frames, at 30 frames per second, and of the frame before
    u16 x30;
    u16 y30;
    u16 pressed30;
    u16 held30;
    u16 xNext;
    u16 yNext;
    u16 pressedNext;
    u16 heldNext;
    u8 autoMode;
    u8 frequency;
    u8 unk76;
    u8 unk77[5];
};

static TouchpadManager *getTouchpadBlock(void);
static u16 GetTouchX(SystemUI *ui);
static u16 GetTouchY(SystemUI *ui);
static u16 GetTouchHeld(SystemUI *ui);
static u16 GetTouchPressed(SystemUI *ui);
static void getTouchpadInput(SystemUI *ui);
static BOOL HitCircle(const TouchRect *rect, u32 x, u32 y);
static BOOL HitRect(const TouchRect *rect, u32 x, u32 y);
static s32 GetHitRect(const TouchRect *rects, u16 x, u16 y);
static s32 GetHeldRect(SystemUI *ui, const TouchRect *rects);
static s32 GetPressedRect(SystemUI *ui, const TouchRect *rects);
static BOOL IsTouchHeld(SystemUI *ui);
static BOOL IsTouchPressed(SystemUI *ui);
static BOOL GetHeldPoint(SystemUI *ui, u32 *x, u32 *y);
static BOOL GetPressedPoint(SystemUI *ui, u32 *x, u32 *y);
static u32 StopAutoSampling(TouchpadManager *tp);
static void RestartSampling(SystemUI *ui);
static void StopSampling(SystemUI *ui);
static u32 StartAutoSampling(TouchpadManager *tp);

static TouchpadManager *sTouchpad;

static TouchpadManager *getTouchpadBlock(void) {
    return sTouchpad;
}

static u16 GetTouchX(SystemUI *ui) {
    TouchpadManager *tp = getTouchpadBlock();

    if (ui->updateRate == 30) {
        return tp->x30;
    }
    return tp->x;
}

static u16 GetTouchY(SystemUI *ui) {
    TouchpadManager *tp = getTouchpadBlock();

    if (ui->updateRate == 30) {
        return tp->y30;
    }
    return tp->y;
}

static u16 GetTouchHeld(SystemUI *ui) {
    TouchpadManager *tp = getTouchpadBlock();

    if (ui->updateRate == 30) {
        return tp->held30;
    }
    return tp->held;
}

static u16 GetTouchPressed(SystemUI *ui) {
    TouchpadManager *tp = getTouchpadBlock();

    if (ui->updateRate == 30) {
        return tp->pressed30;
    }
    return tp->pressed;
}

TouchpadManager *initTouchpad(HeapID heapId) {
    TPCalibrateParam calibrate;
    TouchpadManager *tp = GFL_HeapAllocate(heapId, sizeof(TouchpadManager), FALSE, "touchpanel.c", 131);

    sys_memset(tp, 0, sizeof(TouchpadManager));
    tp->autoSampling = 0;
    tp->stopped = 0;
    TP_Init();
    if (TP_GetUserInfo(&calibrate) == TRUE) {
        TP_SetCalibrateParam(&calibrate);
    } else {
        // Defaults for a system without its touch screen calibrated
        calibrate.x0 = 0x2ae;
        calibrate.y0 = 0x58c;
        calibrate.xDotSize = 0xe25;
        calibrate.yDotSize = 0x1208;
        TP_SetCalibrateParam(&calibrate);
    }
    tp->frequency = 4;
    tp->unk76 = 0;
    sTouchpad = tp;
    return tp;
}

static void getTouchpadInput(SystemUI *ui) {
    TPData raw;
    TPData disp;
    BOOL failed = FALSE;
    TouchpadManager *tp = getTouchpadBlock();

    if (tp == NULL) {
        return;
    }
    if (PAD_DetectFold()) {
        sys_memset(tp, 0, sizeof(TouchpadManager));
        return;
    }
    if (tp->autoMode == 0) {
        u8 retries = 0;

        TP_RequestSamplingAsync();
        while (TP_WaitRawResult(&raw) != 0) {
            retries++;
            if (retries >= SAMPLING_RETRIES) {
                failed = TRUE;
                disp.validity = TP_VALIDITY_INVALID_XY;
                break;
            }
            TP_RequestSamplingAsync();
        }
    } else {
        TP_GetLatestRawPointInAuto(&raw);
    }
    if (!failed) {
        TP_GetCalibratedPoint(&disp, &raw);
    }
    if (disp.validity == TP_VALIDITY_VALID) {
        tp->x = disp.x;
        tp->y = disp.y;
    } else if (tp->held) {
        // Keep the coordinate that is not valid
        switch (disp.validity) {
        case TP_VALIDITY_INVALID_X:
            tp->y = disp.y;
            break;
        case TP_VALIDITY_INVALID_Y:
            tp->x = disp.x;
            break;
        case TP_VALIDITY_INVALID_XY:
            break;
        }
    } else {
        disp.touch = 0;
    }
    tp->pressed = disp.touch & (disp.touch ^ tp->held);
    tp->held = disp.touch;
    tp->pressedNext |= tp->pressed;
    tp->heldNext |= tp->held;
    if (tp->y != 0) {
        tp->yNext = tp->y;
    }
    if (tp->x != 0) {
        tp->xNext = tp->x;
    }
    if (ui->frameCount % 2 == 0) {
        tp->x30 = tp->xNext;
        tp->y30 = tp->yNext;
        tp->pressed30 = tp->pressedNext;
        tp->held30 = tp->heldNext;
        tp->xNext = 0;
        tp->yNext = 0;
        tp->pressedNext = 0;
        tp->heldNext = 0;
    }
}

void touchpadData(void) {
    getTouchpadInput(GCTX_HIDGetInstance());
}

static BOOL HitCircle(const TouchRect *rect, u32 x, u32 y) {
    const TouchCircle *circle = (const TouchCircle *)rect;
    u32 dx = circle->x - x;
    u32 dy = circle->y - y;

    if (dx * dx + dy * dy < circle->radius * circle->radius) {
        return TRUE;
    }
    return FALSE;
}

static BOOL HitRect(const TouchRect *rect, u32 x, u32 y) {
    if (x - rect->left <= (u32)(rect->right - rect->left) && y - rect->top <= (u32)(rect->bottom - rect->top)) {
        return TRUE;
    }
    return FALSE;
}

static s32 GetHitRect(const TouchRect *rects, u16 x, u16 y) {
    int i;

    for (i = 0; rects[i].top != TOUCH_RECT_END; i++) {
        if (rects[i].top == TOUCH_RECT_SKIP) {
            continue;
        }
        if (rects[i].top == TOUCH_RECT_CIRCLE) {
            if (HitCircle(&rects[i], x, y)) {
                return i;
            }
        } else if (HitRect(&rects[i], x, y)) {
            return i;
        }
    }
    return TOUCH_RECT_NONE;
}

static s32 GetHeldRect(SystemUI *ui, const TouchRect *rects) {
    getTouchpadBlock();
    if (GetTouchHeld(ui)) {
        return GetHitRect(rects, GetTouchX(ui), GetTouchY(ui));
    }
    return TOUCH_RECT_NONE;
}

s32 func_0203d9c8(const TouchRect *rects) {
    return GetHeldRect(GCTX_HIDGetInstance(), rects);
}

static s32 GetPressedRect(SystemUI *ui, const TouchRect *rects) {
    getTouchpadBlock();
    if (GetTouchPressed(ui)) {
        return GetHitRect(rects, GetTouchX(ui), GetTouchY(ui));
    }
    return TOUCH_RECT_NONE;
}

s32 func_0203da0c(const TouchRect *rects) {
    return GetPressedRect(GCTX_HIDGetInstance(), rects);
}

static BOOL IsTouchHeld(SystemUI *ui) {
    getTouchpadBlock();
    return GetTouchHeld(ui);
}

BOOL func_0203da2c(void) {
    return IsTouchHeld(GCTX_HIDGetInstance());
}

static BOOL IsTouchPressed(SystemUI *ui) {
    getTouchpadBlock();
    return GetTouchPressed(ui);
}

BOOL func_0203da48(void) {
    return IsTouchPressed(GCTX_HIDGetInstance());
}

static BOOL GetHeldPoint(SystemUI *ui, u32 *x, u32 *y) {
    getTouchpadBlock();
    if (GetTouchHeld(ui)) {
        *x = GetTouchX(ui);
        *y = GetTouchY(ui);
        return TRUE;
    }
    return FALSE;
}

BOOL func_0203da84(u32 *x, u32 *y) {
    return GetHeldPoint(GCTX_HIDGetInstance(), x, y);
}

static BOOL GetPressedPoint(SystemUI *ui, u32 *x, u32 *y) {
    getTouchpadBlock();
    if (GetTouchPressed(ui)) {
        *x = GetTouchX(ui);
        *y = GetTouchY(ui);
        return TRUE;
    }
    return FALSE;
}

BOOL func_0203dac8(u32 *x, u32 *y) {
    return GetPressedPoint(GCTX_HIDGetInstance(), x, y);
}

s32 func_0203dadc(const TouchRect *rects, u32 x, u32 y) {
    return GetHitRect(rects, x, y);
}

static u32 StopAutoSampling(TouchpadManager *tp) {
    if (tp->autoSampling == 0) {
        return 1;
    }
    TP_RequestAutoSamplingStopAsync();
    TP_WaitBusy(TP_REQUEST_COMMAND_FLAG_AUTO_OFF);
    if (TP_CheckError(TP_REQUEST_COMMAND_FLAG_AUTO_OFF)) {
        return 2;
    }
    return 1;
}

static void RestartSampling(SystemUI *ui) {
    TouchpadManager *tp = getTouchpadBlock();

    if (tp != NULL && tp->stopped != 0 && tp->autoSampling != 0) {
        StartAutoSampling(tp);
        tp->stopped = 0;
    }
}

void func_0203db44(void) {
    RestartSampling(GCTX_HIDGetInstance());
}

static void StopSampling(SystemUI *ui) {
    TouchpadManager *tp = getTouchpadBlock();

    if (tp != NULL && tp->stopped != 1 && tp->autoSampling != 0) {
        StopAutoSampling(tp);
        tp->stopped = 1;
    }
}

void func_0203db7c(void) {
    StopSampling(GCTX_HIDGetInstance());
}

static u32 StartAutoSampling(TouchpadManager *tp) {
    TP_RequestAutoSamplingStartAsync(0, tp->frequency, tp->samples, AUTO_SAMPLES);
    TP_WaitBusy(TP_REQUEST_COMMAND_FLAG_AUTO_ON);
    if (TP_CheckError(TP_REQUEST_COMMAND_FLAG_AUTO_ON)) {
        return 2;
    }
    return 1;
}

void GFL_HIDClearTouchState(SystemUI *ui) {
    TouchpadManager *tp = getTouchpadBlock();

    tp->x30 = 0;
    tp->y30 = 0;
    tp->pressed30 = 0;
    tp->held30 = 0;
    tp->xNext = 0;
    tp->yNext = 0;
    tp->pressedNext = 0;
    tp->heldNext = 0;
}

void func_0203dbec(SystemUI *ui, u16 x, u16 y, u16 x30, u16 y30) {
    TouchpadManager *tp = getTouchpadBlock();

    tp->x30 = x30;
    tp->y30 = y30;
    tp->x = x;
    tp->y = y;
}

void func_0203dc14(SystemUI *ui, u16 pressed, u16 pressed30) {
    TouchpadManager *tp = getTouchpadBlock();

    tp->pressed30 = pressed30;
    tp->pressed = pressed;
}

void func_0203dc2c(SystemUI *ui, u16 held, u16 held30) {
    TouchpadManager *tp = getTouchpadBlock();

    tp->held30 = held30;
    tp->held = held;
}
