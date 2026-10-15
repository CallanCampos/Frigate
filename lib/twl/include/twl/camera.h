#ifndef POKEBW2_TWL_CAMERA_H
#define POKEBW2_TWL_CAMERA_H

#include "types.h"
#include "nitro/os.h"
#include "twl/mi.h"

// TwlSDK's camera library, in the DSi's LTD autoload. The game calls it through the SDK's inline functions, which
// only call the library when the game runs on a DSi (OS_IsRunOnTwl, swan's hw_isDSi), so the check is inlined at
// every call. The library keeps its default names until the LTD autoload is analyzed; the comments give the SDK
// functions they implement, from how the Xtransceiver uses them.

typedef enum {
    CAMERA_RESULT_SUCCESS = 0,
    CAMERA_RESULT_SUCCESS_TRUE = 0,
    CAMERA_RESULT_SUCCESS_FALSE,
    CAMERA_RESULT_BUSY,
    CAMERA_RESULT_ILLEGAL_PARAMETER,
    CAMERA_RESULT_SEND_ERROR,
    CAMERA_RESULT_INVALID_COMMAND,
    CAMERA_RESULT_ILLEGAL_STATUS,
    CAMERA_RESULT_FATAL_ERROR,
} CAMERAResult;

typedef enum {
    CAMERA_SELECT_NONE,
    CAMERA_SELECT_IN,
    CAMERA_SELECT_OUT,
    CAMERA_SELECT_BOTH,
} CAMERASelect;

typedef enum {
    CAMERA_CONTEXT_A = 1,
    CAMERA_CONTEXT_B,
    CAMERA_CONTEXT_BOTH,
} CAMERAContext;

typedef enum {
    CAMERA_SIZE_DSI_VGA, // 640 x 480
    CAMERA_SIZE_QVGA,    // 320 x 240
    CAMERA_SIZE_QQVGA,   // 160 x 120
    CAMERA_SIZE_CIF,     // 352 x 288
    CAMERA_SIZE_QCIF,    // 176 x 144
    CAMERA_SIZE_DS_LCD,  // 256 x 192
} CAMERASize;

typedef enum {
    CAMERA_EFFECT_NONE,
} CAMERAEffect;

typedef enum {
    CAMERA_OUTPUT_YUV,
    CAMERA_OUTPUT_RGB,
} CAMERAOutput;

typedef void (*CAMERAIntrCallback)(CAMERAResult result);

CAMERAResult func_02700938(void);                // CAMERA_InitCore
void func_02700b34(void);                        // CAMERA_EndCore
CAMERAResult func_02700bd0(void);                // puts both cameras to standby, called before CAMERA_End
CAMERAResult func_02700cb4(CAMERASelect camera); // CAMERA_I2CInitCore
CAMERAResult func_02700db8(CAMERASelect camera); // CAMERA_I2CActivateCore
// CAMERA_I2CSizeExCore
CAMERAResult func_02700fe4(CAMERASelect camera, CAMERAContext context, CAMERASize size);
// CAMERA_I2CEffectExCore
CAMERAResult func_02701248(CAMERASelect camera, CAMERAContext context, CAMERAEffect effect);
// CAMERA_I2CAutoExposureCore and CAMERA_I2CAutoWhiteBalanceCore, though which is which is a guess
CAMERAResult func_027017dc(CAMERASelect camera, BOOL on);
CAMERAResult func_02701908(CAMERASelect camera, BOOL on);
void func_027000f8(CAMERAIntrCallback callback); // CAMERA_SetVsyncCallbackCore
void func_02700108(CAMERAIntrCallback callback); // CAMERA_SetBufferErrorCallbackCore
void func_02700118(CAMERAIntrCallback callback); // CAMERA_SetRebootCallbackCore
BOOL func_027020f0(void);                        // CAMERA_IsBusyCore
void func_02702108(void);                        // CAMERA_StartCaptureCore
void func_0270212c(void);                        // CAMERA_StopCaptureCore
void func_02702158(BOOL enabled);                // CAMERA_SetTrimmingCore
void func_0270218c(CAMERAOutput output);         // CAMERA_SetOutputFormatCore
void func_027021e4(void);                        // CAMERA_ClearBufferCore
void func_0270229c(u16 lines);                   // CAMERA_SetTransferLinesCore
u16 func_027022f8(u16 width, u16 height);        // CAMERA_GetMaxLinesRoundCore
u32 func_0270234c(u16 width);                    // CAMERA_GetBytesAtOnceCore
u16 func_02702364(CAMERASize size);              // CAMERA_SizeToWidthCore
u16 func_027023cc(CAMERASize size);              // CAMERA_SizeToHeightCore

static inline CAMERAResult CAMERA_Init(void) {
    if (hw_isDSi() == TRUE) {
        CAMERAResult result = func_02700938();
        if (result == CAMERA_RESULT_SUCCESS) {
            result = func_02700cb4(CAMERA_SELECT_BOTH);
        }
        return result;
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline void CAMERA_End(void) {
    if (hw_isDSi() == TRUE) {
        func_02700b34();
    }
}

// Not an SDK name: what the SDK calls the function is unknown
static inline CAMERAResult CAMERA_I2CStop(void) {
    if (hw_isDSi() == TRUE) {
        return func_02700bd0();
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline CAMERAResult CAMERA_I2CActivate(CAMERASelect camera) {
    if (hw_isDSi() == TRUE) {
        return func_02700db8(camera);
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline CAMERAResult CAMERA_I2CSizeEx(CAMERASelect camera, CAMERAContext context, CAMERASize size) {
    if (hw_isDSi() == TRUE) {
        return func_02700fe4(camera, context, size);
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline CAMERAResult CAMERA_I2CEffectEx(CAMERASelect camera, CAMERAContext context, CAMERAEffect effect) {
    if (hw_isDSi() == TRUE) {
        return func_02701248(camera, context, effect);
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline CAMERAResult CAMERA_I2CAutoExposure(CAMERASelect camera, BOOL on) {
    if (hw_isDSi() == TRUE) {
        return func_027017dc(camera, on);
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline CAMERAResult CAMERA_I2CAutoWhiteBalance(CAMERASelect camera, BOOL on) {
    if (hw_isDSi() == TRUE) {
        return func_02701908(camera, on);
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline void CAMERA_SetVsyncCallback(CAMERAIntrCallback callback) {
    if (hw_isDSi() == TRUE) {
        func_027000f8(callback);
    }
}

static inline void CAMERA_SetBufferErrorCallback(CAMERAIntrCallback callback) {
    if (hw_isDSi() == TRUE) {
        func_02700108(callback);
    }
}

static inline void CAMERA_SetRebootCallback(CAMERAIntrCallback callback) {
    if (hw_isDSi() == TRUE) {
        func_02700118(callback);
    }
}

static inline BOOL CAMERA_IsBusy(void) {
    if (hw_isDSi() == TRUE) {
        return func_027020f0();
    }
    return FALSE;
}

static inline void CAMERA_StartCapture(void) {
    if (hw_isDSi() == TRUE) {
        func_02702108();
    }
}

static inline void CAMERA_StopCapture(void) {
    if (hw_isDSi() == TRUE) {
        func_0270212c();
    }
}

static inline void CAMERA_SetTrimming(BOOL enabled) {
    if (hw_isDSi() == TRUE) {
        func_02702158(enabled);
    }
}

static inline void CAMERA_SetOutputFormat(CAMERAOutput output) {
    if (hw_isDSi() == TRUE) {
        func_0270218c(output);
    }
}

static inline void CAMERA_ClearBuffer(void) {
    if (hw_isDSi() == TRUE) {
        func_027021e4();
    }
}

static inline void CAMERA_SetTransferLines(u16 lines) {
    if (hw_isDSi() == TRUE) {
        func_0270229c(lines);
    }
}

static inline u16 CAMERA_GetMaxLinesRound(u16 width, u16 height) {
    if (hw_isDSi() == TRUE) {
        return func_027022f8(width, height);
    }
    return 0;
}

static inline u32 CAMERA_GetBytesAtOnce(u16 width) {
    if (hw_isDSi() == TRUE) {
        return func_0270234c(width);
    }
    return 0;
}

static inline u16 CAMERA_SizeToWidth(CAMERASize size) {
    if (hw_isDSi() == TRUE) {
        return func_02702364(size);
    }
    return 0;
}

static inline u16 CAMERA_SizeToHeight(CAMERASize size) {
    if (hw_isDSi() == TRUE) {
        return func_027023cc(size);
    }
    return 0;
}

#define CAMERA_GET_FRAME_BYTES(width, height) ((width) * 2 * (height))

static inline void CAMERA_DmaRecvAsync(u32 ndmaNo, void *dest, u32 unit, u32 length, MINDmaCallback callback,
                                       void *arg) {
    func_02768378(ndmaNo, dest, unit / 4, length, 0, callback, arg);
}

#endif // POKEBW2_TWL_CAMERA_H
