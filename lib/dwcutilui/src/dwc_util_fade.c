#include "dwc_util_fade.h"
#include "dwc_util_heap.h"
#include "dwc_util_shared.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"

#define DWC_UTIL_FADE_PLANES                                                                                           \
    (GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |               \
     GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD)

// The sub screen's fade, then the main screen's
typedef struct DWCUtilFade {
    DWCUtilTask *task;
    s16 frame;
    u16 length;
    u8 type;
    u8 active;
} DWCUtilFade;

static DWCUtilFade *sFades;

static void func_ov182_021bbb40(DWCUtilTask *task, void *arg);
static void func_ov182_021bbc2c(DWCUtilTask *task, void *arg);

void func_ov182_021bba6c(void) {
    sFades = func_ov182_021bc4d8(sizeof(DWCUtilFade) * 2, 4);
    G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, DWC_UTIL_FADE_PLANES, 16);
    G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, DWC_UTIL_FADE_PLANES, 16);
}

void func_ov182_021bba9c(void) {
    func_ov182_021bc4f0(&sFades);
}

u8 func_ov182_021bbaac(int screen) {
    DWCUtilFade *fade;

    if (screen == 1) {
        fade = sFades;
    } else {
        fade = sFades + 1;
    }
    return fade->active;
}

BOOL func_ov182_021bbac4(int type, int screen, u32 planes, int length) {
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
    fade->task = func_ov182_021bd200(1, func_ov182_021bbb40, fade, 200);
    fade->frame = 0;
    fade->type = type;
    fade->length = length;
    fade->active = TRUE;
    return TRUE;
}

static void func_ov182_021bbb40(DWCUtilTask *task, void *arg) {
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
        func_ov182_021bd268(1, task);
    }
}

BOOL func_ov182_021bbbf4(int length) {
    DWCUtilFade *fade = sFades;

    if (fade->active) {
        return FALSE;
    }
    fade->task = func_ov182_021bd200(1, func_ov182_021bbc2c, fade, 200);
    fade->frame = 0;
    fade->length = length;
    fade->active = TRUE;
    return TRUE;
}

static void func_ov182_021bbc2c(DWCUtilTask *task, void *arg) {
    DWCUtilFade *fade = arg;

    fade->frame++;
    if (fade->frame >= fade->length) {
        fade->active = FALSE;
        func_ov182_021bd268(1, task);
    }
}
