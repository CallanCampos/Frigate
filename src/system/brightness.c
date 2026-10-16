#include "types.h"
#include "gfl/graphics.h"
#include "gfl/std.h"
#include "nitro/hw.h"
#include "system/brightness.h"

// Brightness blends of the screens that move to a target over a number of frames. The file's name is a guess: the
// ROM has no string for it

typedef struct {
    u32 planes;
    u32 screen;
    // The frames the transition takes
    u16 steps;
    s16 target;
    u8 unkC[4];
    // Toward the target, 1 or -1
    s8 direction;
    // The distance to the target, and how much of it each frame moves: quotient, plus 1 whenever the remainders add up
    // to a frame's worth
    s16 distance;
    s16 quotient;
    s16 remainder;
    u16 remainderSum;
    s16 brightness;
    BOOL active;
} BrightnessData;

static void BrightnessData_Step(BrightnessData *data);
static void BrightnessData_Init(BrightnessData *data, u16 steps, s16 target, s16 start, u32 planes, u32 screen);

static BrightnessData sMainBrightness;
static BrightnessData sSubBrightness;

static void BrightnessData_Step(BrightnessData *data) {
    s8 direction = data->direction;
    int step = direction * data->quotient;
    s16 brightness = data->brightness;
    s16 target = data->target;
    BOOL done = FALSE;

    if (brightness + step != target && brightness != target) {
        data->brightness += step;
        data->remainderSum += data->remainder;
        if (data->remainderSum >= data->steps) {
            data->brightness += direction;
            if (data->brightness != target) {
                data->remainderSum -= data->steps;
            } else {
                done = TRUE;
            }
        }
    } else {
        data->brightness = target;
        done = TRUE;
    }
    if (data->screen & BRIGHTNESS_MAIN_SCREEN) {
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, data->planes, data->brightness);
    } else if (data->screen & BRIGHTNESS_SUB_SCREEN) {
        G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, data->planes, data->brightness);
    }
    if (done == TRUE) {
        data->active = FALSE;
    }
}

static void BrightnessData_Init(BrightnessData *data, u16 steps, s16 target, s16 start, u32 planes, u32 screen) {
    data->active = TRUE;
    data->planes = (u8)planes;
    data->screen = (u8)screen;
    data->distance = start - target;
    data->steps = steps;
    data->brightness = start;
    data->target = target;
    if (data->distance > 0) {
        data->direction = -1;
    } else {
        data->direction = 1;
        data->distance *= -1;
    }
    data->quotient = data->distance / steps;
    data->remainder = data->distance % steps;
    data->remainderSum = 0;
}

void BrightnessController_StartTransition(u8 steps, s16 targetBrightness, s16 startBrightness, u32 planes,
                                          u32 screens) {
    if (steps == 0) {
        return;
    }
    if (screens & BRIGHTNESS_MAIN_SCREEN) {
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, planes, startBrightness);
        BrightnessData_Init(&sMainBrightness, steps, targetBrightness, startBrightness, planes, BRIGHTNESS_MAIN_SCREEN);
    }
    if (screens & BRIGHTNESS_SUB_SCREEN) {
        G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, planes, startBrightness);
        BrightnessData_Init(&sSubBrightness, steps, targetBrightness, startBrightness, planes, BRIGHTNESS_SUB_SCREEN);
    }
}

void BrightnessController_SetScreenBrightness(s16 brightness, u32 planes, u32 screens) {
    if (screens & BRIGHTNESS_MAIN_SCREEN) {
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, planes, brightness);
    }
    if (screens & BRIGHTNESS_SUB_SCREEN) {
        G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, planes, brightness);
    }
    BrightnessController_Reset(screens);
}

void BrightnessController_Reset(u32 screens) {
    if (screens & BRIGHTNESS_MAIN_SCREEN) {
        MI_CpuFill8(&sMainBrightness, 0, sizeof(BrightnessData));
        sMainBrightness.active = FALSE;
    }
    if (screens & BRIGHTNESS_SUB_SCREEN) {
        MI_CpuFill8(&sSubBrightness, 0, sizeof(BrightnessData));
        sSubBrightness.active = FALSE;
    }
}

void BrightnessController_Update(void) {
    if (sMainBrightness.active) {
        BrightnessData_Step(&sMainBrightness);
    }
    if (sSubBrightness.active) {
        BrightnessData_Step(&sSubBrightness);
    }
}

BOOL BrightnessController_IsTransitionComplete(u32 screens) {
    if (screens == BRIGHTNESS_BOTH_SCREENS) {
        if (sMainBrightness.active == FALSE && sSubBrightness.active == FALSE) {
            return TRUE;
        }
    } else if (screens == BRIGHTNESS_MAIN_SCREEN) {
        if (sMainBrightness.active == FALSE) {
            return TRUE;
        }
    } else if (screens == BRIGHTNESS_SUB_SCREEN) {
        if (sSubBrightness.active == FALSE) {
            return TRUE;
        }
    }
    return FALSE;
}
