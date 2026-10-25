#include "input.h"
#include "foundation.h"
#include "heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/tp.h"

// The buttons and keys that PAD_Read reports, X and Y among them
#define PAD_ALL_MASK 0x2fff

#define DWC_UTIL_TOUCH_SAMPLES 5
#define DWC_UTIL_KEY_COUNT 14
#define DWC_UTIL_REPEAT_START 40
#define DWC_UTIL_REPEAT_NEXT 47

typedef struct DWCUtilTouchPoint {
    u16 x;
    u16 y;
} DWCUtilTouchPoint;

typedef struct DWCUtilInput {
    TPData samples[DWC_UTIL_TOUCH_SAMPLES];
    DWCUtilTouchPoint point;
    DWCUtilTouchPoint prevPoint;
    u16 held;
    u16 pressed;
    u16 repeated;
    u16 released;
    u8 touching : 1;
    u8 trigger : 1;
    u8 repeat : 1;
    u8 release : 1;
} DWCUtilInput;

typedef struct DWCUtilTouch {
    u8 lidClosed;
    u8 repeatCount;
    DWCUtilInput *input;
} DWCUtilTouch;

static DWCUtilTouch sTouch;
static u8 sKeyRepeats[DWC_UTIL_KEY_COUNT];

// NitroSDK's PAD_Read, as in key.c
static inline u16 PAD_Read(void) {
    return (u16)(((reg_PAD_KEYINPUT | *(vu16 *)HW_BUTTON_XY_BUF) ^ PAD_ALL_MASK) & PAD_ALL_MASK);
}

void DWCi_IPTlInit(void) {
    TPCalibrateParam calibrate;

    sTouch.input = DWCi_HEAPlAllocEx(sizeof(DWCUtilInput), 4);
    if (!TP_GetUserInfo(&calibrate)) {
        OS_Terminate();
    }
    TP_SetCalibrateParam(&calibrate);
    TP_RequestAutoSamplingStartAsync(0, 4, sTouch.input->samples, DWC_UTIL_TOUCH_SAMPLES);
    TP_WaitBusy(TP_REQUEST_COMMAND_FLAG_AUTO_ON);
    if (TP_CheckError(TP_REQUEST_COMMAND_FLAG_AUTO_ON)) {
        OS_Terminate();
    }
    DWCi_IPTlRead();
}

void DWCi_IPTlEnd(void) {
    do {
        TP_RequestAutoSamplingStopAsync();
        TP_WaitBusy(TP_REQUEST_COMMAND_FLAG_AUTO_OFF);
    } while (TP_CheckError(TP_REQUEST_COMMAND_FLAG_AUTO_OFF));
    DWCi_HEAPlFree_(&sTouch.input);
}

void DWCi_IPTlRead(void) {
    readKey();
    readTouch();
}

void readKey(void) {
    u16 *held;
    int i;
    u16 pad;

    held = &sTouch.input->held;
    pad = PAD_Read();
    // Up hides down, and left hides right
    pad = pad & ~((pad & 0x40) << 1) & ~((pad & 0x20) >> 1);
    sTouch.input->pressed = (*held ^ pad) & pad;
    sTouch.input->released = (pad ^ *held) & *held;
    sTouch.input->held = pad;
    sTouch.input->repeated = sTouch.input->pressed;
    for (i = 0; i < DWC_UTIL_KEY_COUNT; i++) {
        u16 bit = 1 << i;

        if (!(pad & bit)) {
            sKeyRepeats[i] = 0;
        } else {
            sKeyRepeats[i]++;
            if (sKeyRepeats[i] == DWC_UTIL_REPEAT_START) {
                sTouch.input->repeated |= bit;
            } else if (sKeyRepeats[i] == DWC_UTIL_REPEAT_NEXT) {
                sTouch.input->repeated |= bit;
                sKeyRepeats[i] = DWC_UTIL_REPEAT_START;
            }
        }
    }
}

void readTouch(void) {
    int i;
    BOOL wasTouching = sTouch.input->touching ? TRUE : FALSE;
    u8 touching = FALSE;
    int index = TP_GetLatestIndexInAuto();
    TPData point;

    sTouch.input->prevPoint = sTouch.input->point;
    // The latest valid sample of the last four
    for (i = 0; i < 4; i++, index = FX_ModS32(index + DWC_UTIL_TOUCH_SAMPLES - 1, DWC_UTIL_TOUCH_SAMPLES)) {
        TPData *sample = &sTouch.input->samples[index];

        if (sample->touch == 1 && sample->validity == TP_VALIDITY_VALID) {
            touching = TRUE;
            TP_GetCalibratedPoint(&point, sample);
            DWCi_InputCoord(point.x, point.y, &sTouch.input->point.x);
            break;
        }
    }
    sTouch.input->trigger = touching & (touching ^ wasTouching);
    sTouch.input->release = (touching ^ wasTouching) & wasTouching;
    sTouch.input->touching = touching;
    sTouch.input->repeat = sTouch.input->trigger;
    if (!touching) {
        sTouch.repeatCount = 0;
        return;
    }
    sTouch.repeatCount++;
    if (sTouch.repeatCount == DWC_UTIL_REPEAT_START) {
        sTouch.input->repeat = TRUE;
    } else if (sTouch.repeatCount == DWC_UTIL_REPEAT_NEXT) {
        sTouch.input->repeat = TRUE;
        sTouch.repeatCount = DWC_UTIL_REPEAT_START;
    }
}

BOOL DWCi_IPTlCheckTrigger(u16 keys) {
    if (PAD_DetectFold()) {
        return FALSE;
    }
    return keys == (sTouch.input->pressed & keys);
}

BOOL DWCi_IPTlCheckRepeat(u16 keys) {
    if (PAD_DetectFold()) {
        return FALSE;
    }
    return keys == (sTouch.input->repeated & keys);
}

BOOL DWCi_IPTlCheckRelease(u16 keys) {
    if (PAD_DetectFold()) {
        return FALSE;
    }
    return keys == (sTouch.input->released & keys);
}

BOOL DWCi_TPlCheck(const u16 *rect) {
    u16 x;
    u16 y;

    if (PAD_DetectFold()) {
        return FALSE;
    }
    if (!sTouch.input->touching) {
        return FALSE;
    }
    x = sTouch.input->point.x;
    if (rect[0] > x) {
        return FALSE;
    }
    if (rect[2] < x) {
        return FALSE;
    }
    y = sTouch.input->point.y;
    if (rect[1] > y) {
        return FALSE;
    }
    if (rect[3] < y) {
        return FALSE;
    }
    return TRUE;
}

BOOL DWCi_TPlCheckTrigger(const u16 *rect) {
    u16 x;
    u16 y;

    if (PAD_DetectFold()) {
        return FALSE;
    }
    if (!sTouch.input->trigger) {
        return FALSE;
    }
    x = sTouch.input->point.x;
    if (rect[0] > x) {
        return FALSE;
    }
    if (rect[2] < x) {
        return FALSE;
    }
    y = sTouch.input->point.y;
    if (rect[1] > y) {
        return FALSE;
    }
    if (rect[3] < y) {
        return FALSE;
    }
    return TRUE;
}

BOOL DWCi_TPlCheckRepeat(const u16 *rect) {
    u16 x;
    u16 y;

    if (PAD_DetectFold()) {
        return FALSE;
    }
    if (!sTouch.input->repeat) {
        return FALSE;
    }
    x = sTouch.input->point.x;
    if (rect[0] > x) {
        return FALSE;
    }
    if (rect[2] < x) {
        return FALSE;
    }
    y = sTouch.input->point.y;
    if (rect[1] > y) {
        return FALSE;
    }
    if (rect[3] < y) {
        return FALSE;
    }
    return TRUE;
}

BOOL DWCi_TPlCheckRelease(const u16 *rect) {
    u16 x;
    u16 y;

    if (PAD_DetectFold()) {
        return FALSE;
    }
    if (!sTouch.input->release) {
        return FALSE;
    }
    x = sTouch.input->point.x;
    if (rect[0] > x) {
        return FALSE;
    }
    if (rect[2] < x) {
        return FALSE;
    }
    y = sTouch.input->point.y;
    if (rect[1] > y) {
        return FALSE;
    }
    if (rect[3] < y) {
        return FALSE;
    }
    return TRUE;
}

BOOL DWCi_TPlCheckATrigger(const u16 *area) {
    u16 rect[4];

    if (PAD_DetectFold()) {
        return FALSE;
    }
    rect[0] = area[0];
    rect[1] = area[1];
    rect[2] = area[0] + area[2];
    rect[3] = area[1] + area[3];
    return DWCi_TPlCheckTrigger(rect);
}

BOOL DWCi_TPlGet(u16 *point) {
    DWCUtilInput *input;

    if (PAD_DetectFold()) {
        return FALSE;
    }
    input = sTouch.input;
    if (!input->touching) {
        point[0] = input->prevPoint.x;
        point[1] = input->prevPoint.y;
        return FALSE;
    }
    point[0] = input->point.x;
    point[1] = input->point.y;
    return TRUE;
}

void DWCi_IPTlCheckFold(void) {
    if (sTouch.lidClosed) {
        if (!PAD_DetectFold()) {
            if (PM_SetLCDPower(TRUE)) {
                OS_WaitVBlankIntr();
                GX_DispOn();
                GXS_DispOn();
                OS_WaitVBlankIntr();
                sTouch.lidClosed = FALSE;
            }
        }
    } else {
        if (PAD_DetectFold()) {
            OS_WaitVBlankIntr();
            GX_DispOff();
            GXS_DispOff();
            OS_WaitVBlankIntr();
            if (PM_SetLCDPower(FALSE)) {
                sTouch.lidClosed = TRUE;
            }
        }
    }
}
