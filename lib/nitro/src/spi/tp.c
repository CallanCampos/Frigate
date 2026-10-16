// TwlSDK's libspi touch panel library (spi/ARM9/src/tp.c, which libspi.TWL.HYB.thumb.a holds first, before mic.c and
// pm.c): requests samples of the touch panel from the ARM7 through the PXI FIFO, reads the calibration in the console's
// user settings, and turns the raw samples into screen coordinates. The file name is the SDK's, and the split is
// certain: the sizes of the functions are those of tp.o and its .bss is the one static. The names are the SDK's.
#include "nitro/tp.h"
#include "nitro/cp.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"

// In the system area at the end of main memory (OSSystemWork), the console's user settings, which the ARM7 copies at
// boot, and the last sample of the touch panel, which it writes
#define HW_USER_SETTINGS 0x02fffc80
#define HW_TOUCHPANEL_BUF ((const u16 *)0x02ffffaa)

// The touch screen's calibration in the user settings: the raw values of two points, and where they are on the screen
typedef struct UserSettings {
    u8 unused0[0x58];
    u16 touchRawX1;
    u16 touchRawY1;
    u8 touchScreenX1;
    u8 touchScreenY1;
    u16 touchRawX2;
    u16 touchRawY2;
    u8 touchScreenX2;
    u8 touchScreenY2;
} UserSettings;

// A sample as the ARM7 writes it, in the two halfwords of the shared memory
typedef union TPRawData {
    u16 half[2];
    struct {
        u32 x : 12;
        u32 y : 12;
        u32 touch : 1;
        u32 validity : 2;
        u32 reserved : 5;
    } sample;
} TPRawData;

// A command is a series of words with the touch panel's tag: the first has TP_PXI_START and the command in bits 8 to
// 14, the last has TP_PXI_END, and each of the others carries its index in bits 16 to 22 and 16 bits of data. The
// ARM7 answers with the command in the same place and the result in the low byte
#define TP_PXI_START 0x02000000
#define TP_PXI_END 0x01000000
#define TP_PXI_INDEX(index) ((index) << 16)
#define TP_PXI_COMMAND(command) ((command) << 8)
#define TP_PXI_COMMAND_MASK 0x7f00
#define TP_PXI_RESULT_MASK 0xff

#define TP_PXI_COMMAND_SAMPLING 0
#define TP_PXI_COMMAND_AUTO_ON 1
#define TP_PXI_COMMAND_AUTO_OFF 2
// Sent by the ARM7 with every sample of the automatic sampling, which it has written to the shared memory
#define TP_PXI_COMMAND_AUTO_SAMPLE 0x10

#define TP_PXI_RESULT_SUCCESS 0
#define TP_PXI_RESULT_INVALID_COMMAND 1
#define TP_PXI_RESULT_INVALID_PARAMETER 2
#define TP_PXI_RESULT_ILLEGAL_STATUS 3
#define TP_PXI_RESULT_BUSY 4

// What the callback is told
#define TP_RESULT_SUCCESS 0
#define TP_RESULT_INVALID_PARAMETER 1
#define TP_RESULT_ILLEGAL_STATUS 2
#define TP_RESULT_BUSY 3
#define TP_RESULT_FATAL_ERROR 4

// The state of the automatic sampling (a guess)
#define TP_AUTO_OFF 0
#define TP_AUTO_ON 2

typedef void (*TPCallback)(u16 command, u8 result, u8 index);

// One axis of the calibration, from a TPCalibrateParam: the raw value of the screen's origin, the raw values for each
// pixel (in 24.8 fixed point) and 2^28 divided by that, which TP_GetCalibratedPoint multiplies by
typedef struct TPAxisCalibration {
    s32 origin;
    s32 dotSize;
    s32 dotSizeInv;
} TPAxisCalibration;

typedef struct TPCalibration {
    TPAxisCalibration x;
    TPAxisCalibration y;
} TPCalibration;

typedef struct TPWork {
    u16 initialized;
    TPCallback callback;
    TPData raw;      // the answer of the last request for one sample
    u16 index;       // of the newest sample of the automatic sampling in samples
    u16 frequency;   // the number of samples that the ARM7 takes in a frame
    TPData *samples; // the buffer of the automatic sampling, a ring
    u16 count;       // the length of samples
    TPCalibration calibration;
    u16 calibrated;
    u16 autoStatus; // set when the ARM7 answers the commands of the automatic sampling, and not read here
    u16 error;      // a bit for each command of TP_REQUEST_COMMAND_FLAG_*, set when it failed
    vu16 busy;      // a bit for each command that waits for the ARM7's answer, which the callback clears
} TPWork;

static TPWork tpState;

static void TPi_TpCallback(u32 tag, u32 data, BOOL err);

// Reads the sample that the ARM7 wrote to the shared memory
static inline void ReadSample(TPData *sample) {
    TPRawData raw;

    raw.half[0] = HW_TOUCHPANEL_BUF[0];
    raw.half[1] = HW_TOUCHPANEL_BUF[1];
    sample->x = raw.sample.x;
    sample->y = raw.sample.y;
    sample->touch = (u8)raw.sample.touch;
    sample->validity = (u8)raw.sample.validity;
}

// TPi_TpCallback: gets the ARM7's words with the touch panel's tag: a sample of the automatic sampling, or the last
// word of the answer to a command
static void TPi_TpCallback(u32 tag, u32 data, BOOL err) {
    u16 word = (u16)data;
    u16 command = (word & TP_PXI_COMMAND_MASK) >> 8;
    u32 result;

    if (err) {
        tpState.error |= 1 << command;
        if (tpState.callback) {
            tpState.callback(command, TP_RESULT_FATAL_ERROR, 0);
        }
        return;
    }

    if (command == TP_PXI_COMMAND_AUTO_SAMPLE) {
        tpState.index++;
        if (tpState.index >= tpState.count) {
            tpState.index = 0;
        }
        ReadSample(&tpState.samples[tpState.index]);
        if (tpState.callback) {
            tpState.callback(command, TP_RESULT_SUCCESS, (u8)tpState.index);
        }
        return;
    }

    if (!(data & TP_PXI_END)) {
        return;
    }

    switch ((u8)word) {
    case TP_PXI_RESULT_SUCCESS:
        switch (command) {
        case TP_PXI_COMMAND_SAMPLING:
            ReadSample(&tpState.raw);
            tpState.autoStatus = TP_AUTO_OFF;
            break;
        case TP_PXI_COMMAND_AUTO_ON:
            tpState.autoStatus = TP_AUTO_ON;
            break;
        case TP_PXI_COMMAND_AUTO_OFF:
            tpState.autoStatus = TP_AUTO_OFF;
            break;
        }
        tpState.busy &= ~(1 << command);
        if (tpState.callback) {
            tpState.callback(command, TP_RESULT_SUCCESS, 0);
        }
        return;
    case TP_PXI_RESULT_BUSY:
        result = TP_RESULT_BUSY;
        goto error;
    case TP_PXI_RESULT_INVALID_PARAMETER:
        result = TP_RESULT_INVALID_PARAMETER;
        goto error;
    case TP_PXI_RESULT_ILLEGAL_STATUS:
        result = TP_RESULT_ILLEGAL_STATUS;
    error:
        tpState.error |= 1 << command;
        tpState.busy &= ~(1 << command);
        if (tpState.callback) {
            tpState.callback(command, result, 0);
        }
        return;
    default:
        OS_Terminate();
        return;
    }
}

// TP_Init
void TP_Init(void) {
    if (tpState.initialized) {
        return;
    }
    tpState.initialized = TRUE;
    PXI_Init();
    tpState.index = 0;
    tpState.callback = NULL;
    tpState.samples = NULL;
    tpState.autoStatus = TP_AUTO_OFF;
    tpState.calibrated = FALSE;
    tpState.busy = 0;
    tpState.error = 0;
    while (!PXI_IsCallbackReady(PXI_FIFO_TAG_TOUCHPANEL, MI_PROCESSOR_ARM7)) {
    }
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_TOUCHPANEL, TPi_TpCallback);
}

// TP_GetUserInfo: the calibration in the user settings, or zeros when they hold none or an invalid one
BOOL TP_GetUserInfo(TPCalibrateParam *calibrate) {
    const UserSettings *settings = (const UserSettings *)HW_USER_SETTINGS;
    u16 rawX1 = settings->touchRawX1;
    u16 rawY1 = settings->touchRawY1;
    u8 screenX1 = settings->touchScreenX1;
    u8 screenY1 = settings->touchScreenY1;
    u16 rawX2 = settings->touchRawX2;
    u16 rawY2 = settings->touchRawY2;
    u8 screenX2 = settings->touchScreenX2;
    u8 screenY2 = settings->touchScreenY2;

    if ((rawX1 == 0 && rawX2 == 0 && rawY1 == 0 && rawY2 == 0) ||
        TP_CalcCalibrateParam(calibrate, rawX1, rawY1, screenX1, screenY1, rawX2, rawY2, screenX2, screenY2) != 0) {
        calibrate->x0 = 0;
        calibrate->y0 = 0;
        calibrate->xDotSize = 0;
        calibrate->yDotSize = 0;
        // BUG: TRUE although there is no calibration, so the defaults that the caller has for that case (initTouchpad)
        // are never used
#ifdef BUGFIX
        return FALSE;
#else
        return TRUE;
#endif
    }
    return TRUE;
}

// TP_SetCalibrateParam: sets the calibration that TP_GetCalibratedPoint applies, or turns it off with NULL
void TP_SetCalibrateParam(const TPCalibrateParam *calibrate) {
    u32 enabled;

    if (calibrate == NULL) {
        tpState.calibrated = FALSE;
        return;
    }

    enabled = OS_DisableInterrupts();
    if (calibrate->xDotSize != 0) {
        reg_CP_DIVCNT = 0;
        reg_CP_DIV_NUMER_LO = 1 << 28;
        reg_CP_DIV_DENOM = (u32)calibrate->xDotSize;
        tpState.calibration.x.origin = calibrate->x0;
        tpState.calibration.x.dotSize = calibrate->xDotSize;
        while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
        }
        tpState.calibration.x.dotSizeInv = reg_CP_DIV_RESULT_LO;
    } else {
        tpState.calibration.x.origin = 0;
        tpState.calibration.x.dotSize = 0;
        tpState.calibration.x.dotSizeInv = 0;
    }
    if (calibrate->yDotSize != 0) {
        reg_CP_DIVCNT = 0;
        reg_CP_DIV_NUMER_LO = 1 << 28;
        reg_CP_DIV_DENOM = (u32)calibrate->yDotSize;
        tpState.calibration.y.origin = calibrate->y0;
        tpState.calibration.y.dotSize = calibrate->yDotSize;
        while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
        }
        tpState.calibration.y.dotSizeInv = reg_CP_DIV_RESULT_LO;
    } else {
        tpState.calibration.y.origin = 0;
        tpState.calibration.y.dotSize = 0;
        tpState.calibration.y.dotSizeInv = 0;
    }
    OS_RestoreInterrupts(enabled);
    tpState.calibrated = TRUE;
}

// TP_RequestSamplingAsync: asks the ARM7 for one sample
void TP_RequestSamplingAsync(void) {
    u32 enabled = OS_DisableInterrupts();
    BOOL sent = FALSE;

    if (PXI_SendWordByFifo(PXI_FIFO_TAG_TOUCHPANEL, TP_PXI_START | TP_PXI_END | TP_PXI_COMMAND(TP_PXI_COMMAND_SAMPLING),
                           FALSE) >= 0) {
        sent = TRUE;
    }
    if (!sent) {
        OS_RestoreInterrupts(enabled);
        tpState.error |= TP_REQUEST_COMMAND_FLAG_SAMPLING;
        if (tpState.callback) {
            tpState.callback(TP_PXI_COMMAND_SAMPLING, TP_RESULT_FATAL_ERROR, 0);
        }
        return;
    }
    tpState.busy |= TP_REQUEST_COMMAND_FLAG_SAMPLING;
    tpState.error &= ~TP_REQUEST_COMMAND_FLAG_SAMPLING;
    OS_RestoreInterrupts(enabled);
}

// TP_WaitRawResult: waits for the sample that TP_RequestSamplingAsync asked for. 1 when the request failed
u32 TP_WaitRawResult(TPData *result) {
    TP_WaitBusy(TP_REQUEST_COMMAND_FLAG_SAMPLING);
    if (tpState.error & TP_REQUEST_COMMAND_FLAG_SAMPLING) {
        return 1;
    }
    *result = tpState.raw;
    return 0;
}

// Sends the command that starts the automatic sampling, in two words. FALSE when the FIFO is full
static inline BOOL SendAutoSamplingStart(u16 vcount, u16 frequency) {
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_TOUCHPANEL,
                           TP_PXI_START | TP_PXI_COMMAND(TP_PXI_COMMAND_AUTO_ON) | (u8)frequency, FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_TOUCHPANEL, TP_PXI_END | TP_PXI_INDEX(1) | vcount, FALSE) < 0) {
        return FALSE;
    }
    return TRUE;
}

// TP_RequestAutoSamplingStartAsync: asks the ARM7 to take frequency samples a frame, from the line vcount on, into a
// ring of count samples. The samples start as empty ones, which have no touch
void TP_RequestAutoSamplingStartAsync(u16 vcount, u16 frequency, TPData samples[], u16 count) {
    u32 enabled;
    u8 sent;
    u32 i;

    tpState.samples = samples;
    tpState.index = 0;
    tpState.frequency = frequency;
    tpState.count = count;
    for (i = 0; i < count; i++) {
        tpState.samples[i].touch = 0;
    }

    enabled = OS_DisableInterrupts();
    sent = SendAutoSamplingStart(vcount, frequency);
    if (!sent) {
        OS_RestoreInterrupts(enabled);
        tpState.error |= TP_REQUEST_COMMAND_FLAG_AUTO_ON;
        if (tpState.callback) {
            tpState.callback(TP_PXI_COMMAND_AUTO_ON, TP_RESULT_FATAL_ERROR, 0);
        }
        return;
    }
    tpState.busy |= TP_REQUEST_COMMAND_FLAG_AUTO_ON;
    tpState.error &= ~TP_REQUEST_COMMAND_FLAG_AUTO_ON;
    OS_RestoreInterrupts(enabled);
}

// TP_RequestAutoSamplingStopAsync
void TP_RequestAutoSamplingStopAsync(void) {
    u32 enabled = OS_DisableInterrupts();
    BOOL sent = FALSE;

    if (PXI_SendWordByFifo(PXI_FIFO_TAG_TOUCHPANEL, TP_PXI_START | TP_PXI_END | TP_PXI_COMMAND(TP_PXI_COMMAND_AUTO_OFF),
                           FALSE) >= 0) {
        sent = TRUE;
    }
    if (!sent) {
        OS_RestoreInterrupts(enabled);
        tpState.error |= TP_REQUEST_COMMAND_FLAG_AUTO_OFF;
        if (tpState.callback) {
            tpState.callback(TP_PXI_COMMAND_AUTO_OFF, TP_RESULT_FATAL_ERROR, 0);
        }
        return;
    }
    tpState.busy |= TP_REQUEST_COMMAND_FLAG_AUTO_OFF;
    tpState.error &= ~TP_REQUEST_COMMAND_FLAG_AUTO_OFF;
    OS_RestoreInterrupts(enabled);
}

// TP_GetLatestRawPointInAuto: the newest point of the automatic sampling. With one sample a frame, or a ring of one,
// it is the newest sample. Otherwise both coordinates start as invalid, and the search goes back from the newest
// sample through the frequency last ones: a sample without touch is the result as it is, and a coordinate that a
// sample has valid is taken from it (the point's validity is cleared from the second sample on only)
void TP_GetLatestRawPointInAuto(TPData *point) {
    int i;
    u32 index;

    point->validity = TP_VALIDITY_INVALID_XY;
    index = tpState.index;
    if (tpState.frequency == 1 || tpState.count == 1) {
        *point = tpState.samples[index];
        return;
    }

    for (i = 0; i < tpState.frequency && i < tpState.count - 1; i++) {
        int j = index - i;
        TPData *sample;

        if (j < 0) {
            j += tpState.count;
        }
        sample = &tpState.samples[j];
        if (sample->touch == 0) {
            *point = *sample;
            return;
        }
        if ((point->validity & TP_VALIDITY_INVALID_X) && !(sample->validity & TP_VALIDITY_INVALID_X)) {
            point->x = sample->x;
            if (i != 0) {
                point->validity &= ~TP_VALIDITY_INVALID_X;
            }
        }
        if ((point->validity & TP_VALIDITY_INVALID_Y) && !(sample->validity & TP_VALIDITY_INVALID_Y)) {
            point->y = sample->y;
            if (i != 0) {
                point->validity &= ~TP_VALIDITY_INVALID_Y;
            }
        }
        if (point->validity == TP_VALIDITY_VALID) {
            point->touch = 1;
            return;
        }
    }
    point->touch = 1;
}

// The index of the newest sample in the buffer of the automatic sampling
u16 TP_GetLatestIndexInAuto(void) {
    return tpState.index;
}

// TP_CalcCalibrateParam: works out the calibration from the raw values of two points and where they are on the screen.
// 1 when a value is out of range or the two points don't differ in a coordinate, or the result does not fit
BOOL TP_CalcCalibrateParam(TPCalibrateParam *calibrate, u16 rawX1, u16 rawY1, u16 screenX1, u16 screenY1, u16 rawX2,
                           u16 rawY2, u16 screenX2, u16 screenY2) {
    s32 dotSize;
    s32 origin;
    s32 rawYDiff;
    s32 screenYDiff;
    u32 enabled;

    if (rawX1 >= 0x1000 || rawY1 >= 0x1000 || rawX2 >= 0x1000 || rawY2 >= 0x1000) {
        return 1;
    }
    if (screenX1 >= 0x100 || screenX2 >= 0x100 || screenY1 >= 0xc0 || screenY2 >= 0xc0) {
        return 1;
    }
    if (screenX1 == screenX2 || screenY1 == screenY2 || rawX1 == rawX2 || rawY1 == rawY2) {
        return 1;
    }

    enabled = OS_DisableInterrupts();
    reg_CP_DIVCNT = 0;
    reg_CP_DIV_NUMER_LO = (rawX1 - rawX2) << 8;
    reg_CP_DIV_DENOM = (u32)(screenX1 - screenX2);
    rawYDiff = rawY1 - rawY2;
    screenYDiff = screenY1 - screenY2;
    while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
    }
    dotSize = reg_CP_DIV_RESULT_LO;
    reg_CP_DIVCNT = 0;
    reg_CP_DIV_NUMER_LO = rawYDiff << 8;
    reg_CP_DIV_DENOM = (u32)screenYDiff;
    if (dotSize >= 0x8000 || dotSize < -0x8000) {
        OS_RestoreInterrupts(enabled);
        return 1;
    }
    calibrate->xDotSize = dotSize;
    origin = ((((rawX1 + rawX2) << 8) - (screenX1 + screenX2) * calibrate->xDotSize) << 9) >> 16;
    if (origin >= 0x8000 || origin < -0x8000) {
        OS_RestoreInterrupts(enabled);
        return 1;
    }
    calibrate->x0 = origin;

    while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
    }
    dotSize = reg_CP_DIV_RESULT_LO;
    OS_RestoreInterrupts(enabled);
    if (dotSize >= 0x8000 || dotSize < -0x8000) {
        return 1;
    }
    calibrate->yDotSize = dotSize;
    origin = ((((rawY1 + rawY2) << 8) - (screenY1 + screenY2) * calibrate->yDotSize) << 9) >> 16;
    if (origin >= 0x8000 || origin < -0x8000) {
        return 1;
    }
    calibrate->y0 = origin;
    return 0;
}

// TP_GetCalibratedPoint: the point on the screen of a raw one. Without a calibration it is the raw one
void TP_GetCalibratedPoint(TPData *disp, const TPData *raw) {
    const TPCalibration *cal = &tpState.calibration;

    if (!tpState.calibrated) {
        *disp = *raw;
        return;
    }

    disp->touch = raw->touch;
    disp->validity = raw->validity;
    if (raw->touch == 0) {
        disp->x = 0;
        disp->y = 0;
        return;
    }

    // The screen is 256 by 192 pixels
    disp->x = ((s64)(raw->x << 2) - cal->x.origin) * cal->x.dotSizeInv >> 22;
    if ((s16)disp->x < 0) {
        disp->x = 0;
    } else if ((s16)disp->x > 255) {
        disp->x = 255;
    }
    disp->y = ((s64)(raw->y << 2) - cal->y.origin) * cal->y.dotSizeInv >> 22;
    if ((s16)disp->y < 0) {
        disp->y = 0;
    } else if ((s16)disp->y > 191) {
        disp->y = 191;
    }
}

// TP_WaitBusy: waits until the ARM7 has answered the commands (TP_REQUEST_COMMAND_FLAG_*)
void TP_WaitBusy(u32 command) {
    while (tpState.busy & command) {
    }
}

// TP_CheckError: which of the commands failed
u32 TP_CheckError(u32 command) {
    return command & tpState.error;
}
