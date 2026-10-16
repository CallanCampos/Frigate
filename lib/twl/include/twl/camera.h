#ifndef POKEBW2_TWL_CAMERA_H
#define POKEBW2_TWL_CAMERA_H

#include "types.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "twl/mi.h"
#include "twl/pm.h"

// TwlSDK's camera library, in the DSi's LTD autoload. The game calls it through the SDK's inline functions, which only
// call the library when the game runs on a DSi (OS_IsRunOnTwl, swan's hw_isDSi), so the check is inlined at every call.
// The functions have the names of the SDK functions they implement, from how the Xtransceiver uses them.

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
    CAMERA_SIZE_DSI_VGA,  // 640 x 480
    CAMERA_SIZE_QVGA,     // 320 x 240
    CAMERA_SIZE_QQVGA,    // 160 x 120
    CAMERA_SIZE_CIF,      // 352 x 288
    CAMERA_SIZE_QCIF,     // 176 x 144
    CAMERA_SIZE_DS_LCD,   // 256 x 192
    CAMERA_SIZE_DS_LCDX4, // 512 x 384
} CAMERASize;

typedef enum {
    CAMERA_EFFECT_NONE,
} CAMERAEffect;

typedef enum {
    CAMERA_FLIP_NONE,
    CAMERA_FLIP_HORIZONTAL,
    CAMERA_FLIP_VERTICAL,
    CAMERA_FLIP_REVERSE,
} CAMERAFlip;

// The color temperatures of the white balance's presets; AUTO is what the sensor sets by itself
typedef enum {
    CAMERA_WHITE_BALANCE_AUTO,
    CAMERA_WHITE_BALANCE_3200K,
    CAMERA_WHITE_BALANCE_4150K,
    CAMERA_WHITE_BALANCE_5200K,
    CAMERA_WHITE_BALANCE_6000K,
    CAMERA_WHITE_BALANCE_7000K,
} CAMERAWhiteBalance;

typedef enum {
    CAMERA_OUTPUT_YUV,
    CAMERA_OUTPUT_RGB,
} CAMERAOutput;

// The camera's modes for a scene, by what camera_api.c sets the white balance, exposure and sharpness to for each (the
// names are from the DSi's camera settings)
typedef enum {
    CAMERA_PHOTO_MODE_NORMAL,
    CAMERA_PHOTO_MODE_PORTRAIT,
    CAMERA_PHOTO_MODE_LANDSCAPE,
    CAMERA_PHOTO_MODE_NIGHTVIEW,
    CAMERA_PHOTO_MODE_LETTER,
} CAMERAPhotoMode;

typedef void (*CAMERAIntrCallback)(CAMERAResult result);
// What the asynchronous I2C commands call when the ARM7 has answered
typedef void (*CAMERACallback)(CAMERAResult result, void *arg);

CAMERAResult CAMERA_InitCore(void);
void CAMERA_EndCore(void);
CAMERAResult func_02700bd0(void); // puts both cameras to standby, called before CAMERA_End
CAMERAResult CAMERA_I2CInitCore(CAMERASelect camera);
CAMERAResult CAMERA_I2CActivateCore(CAMERASelect camera);
CAMERAResult CAMERA_I2CSizeExCore(CAMERASelect camera, CAMERAContext context, CAMERASize size);
CAMERAResult CAMERA_I2CEffectExCore(CAMERASelect camera, CAMERAContext context, CAMERAEffect effect);
// CAMERA_I2CAutoExposureCore and CAMERA_I2CAutoWhiteBalanceCore, though which is which is a guess
CAMERAResult CAMERA_I2CAutoExposureCore(CAMERASelect camera, BOOL on);
CAMERAResult CAMERA_I2CAutoWhiteBalanceCore(CAMERASelect camera, BOOL on);
void CAMERA_SetVsyncCallbackCore(CAMERAIntrCallback callback);
void CAMERA_SetBufferErrorCallbackCore(CAMERAIntrCallback callback);
void CAMERA_SetRebootCallbackCore(CAMERAIntrCallback callback);
BOOL CAMERA_IsBusyCore(void);
void CAMERA_StartCaptureCore(void);
void CAMERA_StopCaptureCore(void);
void CAMERA_SetTrimmingCore(BOOL enabled);
void CAMERA_SetOutputFormatCore(CAMERAOutput output);
void CAMERA_ClearBufferCore(void);
void CAMERA_SetTransferLinesCore(int lines);
int CAMERA_GetMaxLinesRoundCore(u16 width, u16 height);
u32 CAMERA_GetBytesAtOnceCore(u16 width);
u16 CAMERA_SizeToWidthCore(CAMERASize size);
u16 CAMERA_SizeToHeightCore(CAMERASize size);

// camera.c's other functions on the camera interface's registers (SCFG_CLK and the control word at 0x04004202), which
// camera_api.c calls while it starts the camera, stops it and handles its interrupt. Their names are descriptive
void CAMERA_ResetCore(void);                 // resets the interface and its camera clock
BOOL CAMERA_IsBufferErrorCore(void);         // whether the interface reports a buffer error (bit 4 of its control)
void CAMERA_SetInterruptEnableCore(BOOL on); // sets or clears the interface's interrupt enable (bit 11 of its control)
void func_0270223c(u32 bits);                // replaces bits 8 and 9 of the interface's control
void func_02702268(BOOL on);                 // sets or clears bit 10 of the interface's control

static inline CAMERAResult CAMERA_Init(void) {
    if (hw_isDSi() == TRUE) {
        CAMERAResult result = CAMERA_InitCore();
        if (result == CAMERA_RESULT_SUCCESS) {
            result = CAMERA_I2CInitCore(CAMERA_SELECT_BOTH);
        }
        return result;
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline void CAMERA_End(void) {
    if (hw_isDSi() == TRUE) {
        CAMERA_EndCore();
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
        return CAMERA_I2CActivateCore(camera);
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline CAMERAResult CAMERA_I2CSizeEx(CAMERASelect camera, CAMERAContext context, CAMERASize size) {
    if (hw_isDSi() == TRUE) {
        return CAMERA_I2CSizeExCore(camera, context, size);
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline CAMERAResult CAMERA_I2CEffectEx(CAMERASelect camera, CAMERAContext context, CAMERAEffect effect) {
    if (hw_isDSi() == TRUE) {
        return CAMERA_I2CEffectExCore(camera, context, effect);
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline CAMERAResult CAMERA_I2CAutoExposure(CAMERASelect camera, BOOL on) {
    if (hw_isDSi() == TRUE) {
        return CAMERA_I2CAutoExposureCore(camera, on);
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline CAMERAResult CAMERA_I2CAutoWhiteBalance(CAMERASelect camera, BOOL on) {
    if (hw_isDSi() == TRUE) {
        return CAMERA_I2CAutoWhiteBalanceCore(camera, on);
    }
    return CAMERA_RESULT_FATAL_ERROR;
}

static inline void CAMERA_SetVsyncCallback(CAMERAIntrCallback callback) {
    if (hw_isDSi() == TRUE) {
        CAMERA_SetVsyncCallbackCore(callback);
    }
}

static inline void CAMERA_SetBufferErrorCallback(CAMERAIntrCallback callback) {
    if (hw_isDSi() == TRUE) {
        CAMERA_SetBufferErrorCallbackCore(callback);
    }
}

static inline void CAMERA_SetRebootCallback(CAMERAIntrCallback callback) {
    if (hw_isDSi() == TRUE) {
        CAMERA_SetRebootCallbackCore(callback);
    }
}

static inline BOOL CAMERA_IsBusy(void) {
    if (hw_isDSi() == TRUE) {
        return CAMERA_IsBusyCore();
    }
    return FALSE;
}

static inline void CAMERA_StartCapture(void) {
    if (hw_isDSi() == TRUE) {
        CAMERA_StartCaptureCore();
    }
}

static inline void CAMERA_StopCapture(void) {
    if (hw_isDSi() == TRUE) {
        CAMERA_StopCaptureCore();
    }
}

static inline void CAMERA_SetTrimming(BOOL enabled) {
    if (hw_isDSi() == TRUE) {
        CAMERA_SetTrimmingCore(enabled);
    }
}

static inline void CAMERA_SetOutputFormat(CAMERAOutput output) {
    if (hw_isDSi() == TRUE) {
        CAMERA_SetOutputFormatCore(output);
    }
}

static inline void CAMERA_ClearBuffer(void) {
    if (hw_isDSi() == TRUE) {
        CAMERA_ClearBufferCore();
    }
}

static inline BOOL CAMERA_IsBufferError(void) {
    if (hw_isDSi() == TRUE) {
        return CAMERA_IsBufferErrorCore();
    }
    return FALSE;
}

static inline void CAMERA_SetTransferLines(int lines) {
    if (hw_isDSi() == TRUE) {
        CAMERA_SetTransferLinesCore(lines);
    }
}

static inline int CAMERA_GetMaxLinesRound(u16 width, u16 height) {
    if (hw_isDSi() == TRUE) {
        return CAMERA_GetMaxLinesRoundCore(width, height);
    }
    return 0;
}

static inline u32 CAMERA_GetBytesAtOnce(u16 width) {
    if (hw_isDSi() == TRUE) {
        return CAMERA_GetBytesAtOnceCore(width);
    }
    return 0;
}

static inline u16 CAMERA_SizeToWidth(CAMERASize size) {
    if (hw_isDSi() == TRUE) {
        return CAMERA_SizeToWidthCore(size);
    }
    return 0;
}

static inline u16 CAMERA_SizeToHeight(CAMERASize size) {
    if (hw_isDSi() == TRUE) {
        return CAMERA_SizeToHeightCore(size);
    }
    return 0;
}

#define CAMERA_GET_FRAME_BYTES(width, height) ((width) * 2 * (height))

static inline void CAMERA_DmaRecvAsync(u32 ndmaNo, void *dest, u32 unit, u32 length, MINDmaCallback callback,
                                       void *arg) {
    func_02768378(ndmaNo, dest, unit / 4, length, 0, callback, arg);
}

// What camera_api.c uses of the other libraries, whose headers do not have it yet. Each block moves to the header it
// names with the chain that owns it.

#endif // POKEBW2_TWL_CAMERA_H
