#include "types.h"
#include "field/field_disp_control.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/hw.h"

// The procs of the requests, by request
static void (*FIELD_DISP_CONTROL_PROCS[])(const u32 *params, u32 screen) = {
    NULL,
    FieldDispControlProc_ResetBrightness,
    FieldDispControlProc_SetAlpha,
    FieldDispControlProc_SetBrightness,
    FieldDispControlProc_SetAll,
    FieldDispControlProc_AdjustAlpha,
    FieldDispControlProc_AdjustBrightness,
};

FieldDispControl *FieldDispControl_Create(HeapID heapId) {
    FieldDispControl *control;

    control = GFL_HeapAllocate(heapId, sizeof(FieldDispControl), TRUE, "fld_vreq.c", 128);
    MI_CpuFill8(control->bgEnabled, FIELD_DISP_BG_UNCHANGED, sizeof(control->bgEnabled));
    return control;
}

void FieldDispControl_Free(FieldDispControl *control) {
    GFL_HeapFree(control);
}

void FieldDispControl_Update(FieldDispControl *control) {
    void (*proc)(const u32 *, u32);
    s32 i;

    proc = FIELD_DISP_CONTROL_PROCS[control->requestA];
    if (proc != NULL) {
        proc(&control->alphaA, 1);
        control->requestA = FIELD_DISP_REQUEST_NONE;
    }

    proc = FIELD_DISP_CONTROL_PROCS[control->requestB];
    if (proc != NULL) {
        proc(&control->alphaB, 0);
        control->requestB = FIELD_DISP_REQUEST_NONE;
    }

    for (i = 0; i < 8; i++) {
        if (control->bgEnabled[i] != FIELD_DISP_BG_UNCHANGED) {
            GFL_BGSysSetBGEnabled(i, control->bgEnabled[i]);
            control->bgEnabled[i] = FIELD_DISP_BG_UNCHANGED;
        }
    }

    if (control->brightnessRequest[0] != 0) {
        GX_SetMasterBrightness(control->brightnessValue[0]);
        control->brightnessRequest[0] = 0;
    }
    if (control->brightnessRequest[1] != 0) {
        GXS_SetMasterBrightness(control->brightnessValue[1]);
        control->brightnessRequest[1] = 0;
    }
}

void FieldDispControl_ReqSetBGEnabled(FieldDispControl *control, u32 bgId, BOOL enabled) {
    control->bgEnabled[bgId] = enabled;
}

void FieldDispControl_ReqSetAlphaA(FieldDispControl *control, u32 alpha, u32 beta, u32 planeMask, u32 complement) {
    control->requestA = FIELD_DISP_REQUEST_SET_ALPHA;
    control->alphaA = alpha;
    control->betaA = beta;
    control->planeMaskA = planeMask;
    control->complementA = complement;
}

void FieldDispControl_ReqSetAllA(FieldDispControl *control, u32 alpha, u32 beta, u32 planeMask, u32 complement,
                                 u32 all) {
    control->requestA = FIELD_DISP_REQUEST_SET_ALL;
    control->alphaA = alpha;
    control->betaA = beta;
    control->planeMaskA = planeMask;
    control->complementA = complement;
    control->allA = all;
}

void FieldDispControl_ReqAdjustAlphaA(FieldDispControl *control, u32 alpha, u32 complement) {
    control->requestA = FIELD_DISP_REQUEST_ADJUST_ALPHA;
    control->alphaA = alpha;
    control->betaA = complement;
}

void FieldDispControl_ReqSetAlphaB(FieldDispControl *control, u32 alpha, u32 beta, u32 planeMask, u32 complement) {
    control->requestB = FIELD_DISP_REQUEST_SET_ALPHA;
    control->alphaB = alpha;
    control->betaB = beta;
    control->planeMaskB = planeMask;
    control->complementB = complement;
}

void FieldDispControlProc_ResetBrightness(const u32 *params, u32 screen) {
    if (screen != 0) {
        *(vu16 *)REG_BLDCNT_ADDR = 0;
    } else {
        *(vu16 *)REG_DB_BLDCNT_ADDR = 0;
    }
}

void FieldDispControlProc_SetAlpha(const u32 *params, u32 screen) {
    if (screen != 0) {
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, params[0], params[1], params[2], params[3]);
    } else {
        G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, params[0], params[1], params[2], params[3]);
    }
}

void FieldDispControlProc_SetBrightness(const u32 *params, u32 screen) {
    if (screen != 0) {
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, params[0], params[1]);
    } else {
        G2x_SetBlendBrightness_(REG_DB_BLDCNT_ADDR, params[0], params[1]);
    }
}

void FieldDispControlProc_SetAll(const u32 *params, u32 screen) {
    if (screen != 0) {
        G2x_SetBlendBrightnessExt_(REG_BLDCNT_ADDR, params[0], params[1], params[2], params[3], params[4]);
    } else {
        G2x_SetBlendBrightnessExt_(REG_DB_BLDCNT_ADDR, params[0], params[1], params[2], params[3], params[4]);
    }
}

void FieldDispControlProc_AdjustAlpha(const u32 *params, u32 screen) {
    if (screen != 0) {
        *(vu16 *)REG_BLDALPHA_ADDR = params[0] | (params[1] << 8);
    } else {
        *(vu16 *)REG_DB_BLDALPHA_ADDR = params[0] | (params[1] << 8);
    }
}

void FieldDispControlProc_AdjustBrightness(const u32 *params, u32 screen) {
    if (screen != 0) {
        G2x_ChangeBlendBrightness_(REG_BLDCNT_ADDR, params[0]);
    } else {
        G2x_ChangeBlendBrightness_(REG_DB_BLDCNT_ADDR, params[0]);
    }
}
