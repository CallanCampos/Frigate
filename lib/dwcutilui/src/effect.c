#include "effect.h"
#include "dwc_util_heap.h"
#include "dwc_util_shared.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"

// The sub screen's fade, then the main screen's
typedef struct DWCUtilFade {
    DWCUtilTask *task;
    s16 frame;
    u16 length;
    u8 type;
    u8 active;
} DWCUtilFade;

static DWCUtilFade *sFades;

static void taskFade(DWCUtilTask *task, void *arg);
static void taskWait(DWCUtilTask *task, void *arg);

void DWCi_EFFlInit(void) {
    sFades = DWCi_HEAPlAllocEx(sizeof(DWCUtilFade) * 2, 4);
    G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, DWC_UTIL_FADE_PLANES, 16);
    G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, DWC_UTIL_FADE_PLANES, 16);
}

void DWCi_EFFlEnd(void) {
    DWCi_HEAPlFree_(&sFades);
}

u8 DWCi_EFFlCheck(int screen) {
    DWCUtilFade *fade;

    if (screen == 1) {
        fade = sFades;
    } else {
        fade = sFades + 1;
    }
    return fade->active;
}

BOOL DWCi_EFFlFade(int type, int screen, u32 planes, int length) {
    s8 start[4] = { -16, 0, 16, 0 };
    DWCUtilFade *fade;

    if (screen == 1) {
        fade = sFades;
    } else {
        fade = sFades + 1;
    }
    if (fade->active) {
        return FALSE;
    }
    if (screen == 1) {
        G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, planes, start[type]);
    } else {
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, planes, start[type]);
    }
    fade->task = DWCi_TSKlForm(1, taskFade, fade, 200);
    fade->frame = 0;
    fade->type = type;
    fade->length = length;
    fade->active = TRUE;
    return TRUE;
}

static void taskFade(DWCUtilTask *task, void *arg) {
    // Bit 0: the brightness falls to 0; bit 4: it is a darkening
    u8 flags[4] = { 0x11, 0x10, 0x01, 0x00 };
    s8 end[4] = { 0, -16, 0, 16 };
    DWCUtilFade *fade = arg;
    int brightness;

    fade->frame++;
    brightness = FX_DivS32(fade->frame * 16, fade->length);
    if (flags[fade->type] & 1) {
        brightness = 16 - brightness;
    }
    if (flags[fade->type] & 0x10) {
        brightness = -brightness;
    }
    G2x_ChangeBlendBrightness_(fade == sFades ? REG_DB_BLDCNT_ADDR : REG_BLDCNT_ADDR, brightness);
    if (fade->frame >= fade->length) {
        if (fade == sFades) {
            G2x_ChangeBlendBrightness_(REG_DB_BLDCNT_ADDR, end[fade->type]);
        } else {
            G2x_ChangeBlendBrightness_(REG_BLDCNT_ADDR, end[fade->type]);
        }
        fade->active = FALSE;
        DWCi_TSKlDelete(1, task);
    }
}

BOOL DWCi_EFFlWait(int length) {
    DWCUtilFade *fade = sFades;

    if (fade->active) {
        return FALSE;
    }
    fade->task = DWCi_TSKlForm(1, taskWait, fade, 200);
    fade->frame = 0;
    fade->length = length;
    fade->active = TRUE;
    return TRUE;
}

static void taskWait(DWCUtilTask *task, void *arg) {
    DWCUtilFade *fade = arg;

    fade->frame++;
    if (fade->frame >= fade->length) {
        fade->active = FALSE;
        DWCi_TSKlDelete(1, task);
    }
}
