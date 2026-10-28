#ifndef POKEBW2_FIELD_FIELD_DISP_CONTROL_H
#define POKEBW2_FIELD_FIELD_DISP_CONTROL_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A request of each screen's blending, which FieldDispControl_Update makes once, by its proc
enum {
    FIELD_DISP_REQUEST_NONE,
    FIELD_DISP_REQUEST_RESET_BRIGHTNESS,
    FIELD_DISP_REQUEST_SET_ALPHA,
    FIELD_DISP_REQUEST_SET_BRIGHTNESS,
    FIELD_DISP_REQUEST_SET_ALL,
    FIELD_DISP_REQUEST_ADJUST_ALPHA,
    FIELD_DISP_REQUEST_ADJUST_BRIGHTNESS,
};

// A background whose shown state no request changes
#define FIELD_DISP_BG_UNCHANGED 0xff

struct FieldDispControl {
    // A FIELD_DISP_REQUEST_ value
    u16 requestA;
    u16 requestB;
    u32 alphaA;
    u32 betaA;
    u32 planeMaskA;
    u32 complementA;
    u32 allA;
    u32 alphaB;
    u32 betaB;
    u32 planeMaskB;
    u32 complementB;
    u32 allB;
    // TRUE, FALSE or FIELD_DISP_BG_UNCHANGED
    u8 bgEnabled[8];
    u8 brightnessRequest[2];
    s8 brightnessValue[2];
};

extern const char data_ov036_021d56f0[];

FieldDispControl *FieldDispControl_Create(HeapID heapId);
void FieldDispControl_Free(FieldDispControl *control);
void FieldDispControl_Update(FieldDispControl *control);
void FieldDispControlProc_ResetBrightness(const u32 *params, u32 screen);
void FieldDispControlProc_SetAlpha(const u32 *params, u32 screen);
void FieldDispControlProc_SetBrightness(const u32 *params, u32 screen);
void FieldDispControlProc_SetAll(const u32 *params, u32 screen);
void FieldDispControlProc_AdjustAlpha(const u32 *params, u32 screen);
void FieldDispControlProc_AdjustBrightness(const u32 *params, u32 screen);
void FieldDispControl_ReqAdjustAlphaA(FieldDispControl *control, u32 alpha, u32 complement);
void FieldDispControl_ReqSetAlphaA(FieldDispControl *control, u32 alpha, u32 beta, u32 planeMask, u32 complement);
void FieldDispControl_ReqSetAllA(FieldDispControl *control, u32 alpha, u32 beta, u32 planeMask, u32 complement,
                                 u32 all);
void FieldDispControl_ReqSetAlphaB(FieldDispControl *control, u32 alpha, u32 beta, u32 planeMask, u32 complement);
void FieldDispControl_ReqSetBGEnabled(FieldDispControl *control, u32 bgId, BOOL enabled);

#endif // POKEBW2_FIELD_FIELD_DISP_CONTROL_H
