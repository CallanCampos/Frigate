// TwlSDK's libspi touch panel library (spi/ARM9/src/tp.c, which libspi.TWL.HYB.thumb.a holds first, before mic.c and
// pm.c): requests samples of the touch panel from the ARM7 through the PXI FIFO, reads the calibration in the console's
// user settings, and turns the raw samples into screen coordinates. The file name is the SDK's, and the split is
// certain: the sizes of the functions are those of tp.o and its .bss is the one static. The names are the SDK's.
#include "nitro/tp.h"
#include "nitro/cp.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"

// A sample as the ARM7 writes it, in the two halfwords of the shared memory
typedef union SPITpData {
    u16 halfs[2];
    struct {
        u32 x : 12;
        u32 y : 12;
        u32 touch : 1;
        u32 validity : 2;
        u32 reserved : 5;
    } e;
} SPITpData;

// A command is a series of words with the touch panel's tag: the first has SPI_PXI_START_BIT and the command in bits 8
// to 14, the last has SPI_PXI_END_BIT, and each of the others carries its index in bits 16 to 22 and 16 bits of data.
// The ARM7 answers with the command in the same place and the result in the low byte
#define SPI_PXI_START_BIT 0x02000000
#define SPI_PXI_END_BIT 0x01000000
#define TP_PXI_INDEX(index) ((index) << 16)
#define TP_PXI_COMMAND(command) ((command) << 8)
#define SPI_PXI_RESULT_COMMAND_MASK 0x7f00
#define SPI_PXI_RESULT_DATA_MASK 0xff

#define SPI_PXI_COMMAND_TP_SAMPLING 0
#define SPI_PXI_COMMAND_TP_AUTO_ON 1
#define SPI_PXI_COMMAND_TP_AUTO_OFF 2
// Sent by the ARM7 with every sample of the automatic sampling, which it has written to the shared memory
#define SPI_PXI_COMMAND_TP_AUTO_SAMPLING 0x10

#define TP_PXI_RESULT_SUCCESS 0
#define TP_PXI_RESULT_INVALID_COMMAND 1
#define TP_PXI_RESULT_INVALID_PARAMETER 2
#define TP_PXI_RESULT_ILLEGAL_STATUS 3
#define SPI_PXI_RESULT_EXCLUSIVE 4

// What the callback is told
#define TP_RESULT_SUCCESS 0
#define TP_RESULT_INVALID_PARAMETER 1
#define TP_RESULT_ILLEGAL_STATUS 2
#define TP_RESULT_EXCLUSIVE 3
#define TP_RESULT_PXI_BUSY 4

// The state of the automatic sampling (a guess)
#define TP_STATE_READY 0
#define TP_STATE_AUTO_SAMPLING 2

typedef void (*TPRecvCallback)(u16 command, u8 result, u8 index);

// One axis of the calibration, from a TPCalibrateParam: the raw value of the screen's origin, the raw values for each
// pixel (in 24.8 fixed point) and 2^28 divided by that, which TP_GetCalibratedPoint multiplies by
typedef struct TPAxisCalibration {
    s32 origin;
    s32 dotSize;
    s32 dotSizeInv;
} TPAxisCalibration;

typedef struct TPiCalibrateParam {
    TPAxisCalibration x;
    TPAxisCalibration y;
} TPiCalibrateParam;

typedef struct TPWork {
    u16 initialized;
    TPRecvCallback callback;
    TPData buf;           // the answer of the last request for one sample
    u16 index;            // of the newest sample of the automatic sampling in samples
    u16 frequence;        // the number of samples that the ARM7 takes in a frame
    TPData *samplingBufs; // the buffer of the automatic sampling, a ring
    u16 bufSize;          // the length of samples
    TPiCalibrateParam calibrate;
    u16 calibrate_flg;
    u16 state;        // set when the ARM7 answers the commands of the automatic sampling, and not read here
    u16 err_flg;      // a bit for each command of TP_REQUEST_COMMAND_FLAG_*, set when it failed
    vu16 command_flg; // a bit for each command that waits for the ARM7's answer, which the callback clears
} TPWork;

static TPWork tpState;

static void TPi_TpCallback(PXIFifoTag tag, u32 data, BOOL err);

// Reads the sample that the ARM7 wrote to the shared memory
static inline void TPi_CopyTpFromSystemWork(TPData *sample) {
    SPITpData raw;

    raw.halfs[0] = OS_GetSystemWork()->touchPanelX;
    raw.halfs[1] = OS_GetSystemWork()->touchPanelY;
    sample->x = raw.e.x;
    sample->y = raw.e.y;
    sample->touch = (u8)raw.e.touch;
    sample->validity = (u8)raw.e.validity;
}

// TPi_TpCallback: gets the ARM7's words with the touch panel's tag: a sample of the automatic sampling, or the last
// word of the answer to a command
static void TPi_TpCallback(PXIFifoTag tag, u32 data, BOOL err) {
    u16 word = (u16)data;
    u16 command = (word & SPI_PXI_RESULT_COMMAND_MASK) >> 8;
    u32 result;

    if (err) {
        tpState.err_flg |= 1 << command;
        if (tpState.callback) {
            tpState.callback(command, TP_RESULT_PXI_BUSY, 0);
        }
        return;
    }

    if (command == SPI_PXI_COMMAND_TP_AUTO_SAMPLING) {
        tpState.index++;
        if (tpState.index >= tpState.bufSize) {
            tpState.index = 0;
        }
        TPi_CopyTpFromSystemWork(&tpState.samplingBufs[tpState.index]);
        if (tpState.callback) {
            tpState.callback(command, TP_RESULT_SUCCESS, (u8)tpState.index);
        }
        return;
    }

    if (!(data & SPI_PXI_END_BIT)) {
        return;
    }

    switch ((u8)word) {
    case TP_PXI_RESULT_SUCCESS:
        switch (command) {
        case SPI_PXI_COMMAND_TP_SAMPLING:
            TPi_CopyTpFromSystemWork(&tpState.buf);
            tpState.state = TP_STATE_READY;
            break;
        case SPI_PXI_COMMAND_TP_AUTO_ON:
            tpState.state = TP_STATE_AUTO_SAMPLING;
            break;
        case SPI_PXI_COMMAND_TP_AUTO_OFF:
            tpState.state = TP_STATE_READY;
            break;
        }
        tpState.command_flg &= ~(1 << command);
        if (tpState.callback) {
            tpState.callback(command, TP_RESULT_SUCCESS, 0);
        }
        return;
    case SPI_PXI_RESULT_EXCLUSIVE:
        result = TP_RESULT_EXCLUSIVE;
        goto error;
    case TP_PXI_RESULT_INVALID_PARAMETER:
        result = TP_RESULT_INVALID_PARAMETER;
        goto error;
    case TP_PXI_RESULT_ILLEGAL_STATUS:
        result = TP_RESULT_ILLEGAL_STATUS;
    error:
        tpState.err_flg |= 1 << command;
        tpState.command_flg &= ~(1 << command);
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
    tpState.samplingBufs = NULL;
    tpState.state = TP_STATE_READY;
    tpState.calibrate_flg = FALSE;
    tpState.command_flg = 0;
    tpState.err_flg = 0;
    while (!PXI_IsCallbackReady(PXI_FIFO_TAG_TOUCHPANEL, PXI_PROC_ARM7)) {
    }
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_TOUCHPANEL, TPi_TpCallback);
}

// TP_GetUserInfo: the calibration in the user settings, or zeros when they hold none or an invalid one
BOOL TP_GetUserInfo(TPCalibrateParam *calibrate) {
    const NVRAMConfig *settings = (const NVRAMConfig *)HW_NVRAM_USER_INFO;
    u16 rawX1 = settings->tp.raw_x1;
    u16 rawY1 = settings->tp.raw_y1;
    u8 screenX1 = settings->tp.dx1;
    u8 screenY1 = settings->tp.dy1;
    u16 rawX2 = settings->tp.raw_x2;
    u16 rawY2 = settings->tp.raw_y2;
    u8 screenX2 = settings->tp.dx2;
    u8 screenY2 = settings->tp.dy2;

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
    OSIntrMode enabled;

    if (calibrate == NULL) {
        tpState.calibrate_flg = FALSE;
        return;
    }

    enabled = OS_DisableInterrupts();
    if (calibrate->xDotSize != 0) {
        reg_CP_DIVCNT = 0;
        reg_CP_DIV_NUMER_L = 1 << 28;
        reg_CP_DIV_DENOM = (u32)calibrate->xDotSize;
        tpState.calibrate.x.origin = calibrate->x0;
        tpState.calibrate.x.dotSize = calibrate->xDotSize;
        while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
        }
        tpState.calibrate.x.dotSizeInv = reg_CP_DIV_RESULT_L;
    } else {
        tpState.calibrate.x.origin = 0;
        tpState.calibrate.x.dotSize = 0;
        tpState.calibrate.x.dotSizeInv = 0;
    }
    if (calibrate->yDotSize != 0) {
        reg_CP_DIVCNT = 0;
        reg_CP_DIV_NUMER_L = 1 << 28;
        reg_CP_DIV_DENOM = (u32)calibrate->yDotSize;
        tpState.calibrate.y.origin = calibrate->y0;
        tpState.calibrate.y.dotSize = calibrate->yDotSize;
        while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
        }
        tpState.calibrate.y.dotSizeInv = reg_CP_DIV_RESULT_L;
    } else {
        tpState.calibrate.y.origin = 0;
        tpState.calibrate.y.dotSize = 0;
        tpState.calibrate.y.dotSizeInv = 0;
    }
    OS_RestoreInterrupts(enabled);
    tpState.calibrate_flg = TRUE;
}

// TP_RequestSamplingAsync: asks the ARM7 for one sample
void TP_RequestSamplingAsync(void) {
    OSIntrMode enabled = OS_DisableInterrupts();
    BOOL sent = FALSE;

    if (PXI_SendWordByFifo(PXI_FIFO_TAG_TOUCHPANEL,
                           SPI_PXI_START_BIT | SPI_PXI_END_BIT | TP_PXI_COMMAND(SPI_PXI_COMMAND_TP_SAMPLING),
                           FALSE) >= 0) {
        sent = TRUE;
    }
    if (!sent) {
        OS_RestoreInterrupts(enabled);
        tpState.err_flg |= TP_REQUEST_COMMAND_FLAG_SAMPLING;
        if (tpState.callback) {
            tpState.callback(SPI_PXI_COMMAND_TP_SAMPLING, TP_RESULT_PXI_BUSY, 0);
        }
        return;
    }
    tpState.command_flg |= TP_REQUEST_COMMAND_FLAG_SAMPLING;
    tpState.err_flg &= ~TP_REQUEST_COMMAND_FLAG_SAMPLING;
    OS_RestoreInterrupts(enabled);
}

// TP_WaitRawResult: waits for the sample that TP_RequestSamplingAsync asked for. 1 when the request failed
u32 TP_WaitRawResult(TPData *result) {
    TP_WaitBusy(TP_REQUEST_COMMAND_FLAG_SAMPLING);
    if (tpState.err_flg & TP_REQUEST_COMMAND_FLAG_SAMPLING) {
        return 1;
    }
    *result = tpState.buf;
    return 0;
}

// Sends the command that starts the automatic sampling, in two words. FALSE when the FIFO is full
static inline BOOL TPi_AutoSamplingOn(u16 vcount, u16 frequency) {
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_TOUCHPANEL,
                           SPI_PXI_START_BIT | TP_PXI_COMMAND(SPI_PXI_COMMAND_TP_AUTO_ON) | (u8)frequency, FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_TOUCHPANEL, SPI_PXI_END_BIT | TP_PXI_INDEX(1) | vcount, FALSE) < 0) {
        return FALSE;
    }
    return TRUE;
}

// TP_RequestAutoSamplingStartAsync: asks the ARM7 to take frequency samples a frame, from the line vcount on, into a
// ring of count samples. The samples start as empty ones, which have no touch
void TP_RequestAutoSamplingStartAsync(u16 vcount, u16 frequency, TPData samples[], u16 count) {
    OSIntrMode enabled;
    u8 sent;
    u32 i;

    tpState.samplingBufs = samples;
    tpState.index = 0;
    tpState.frequence = frequency;
    tpState.bufSize = count;
    for (i = 0; i < count; i++) {
        tpState.samplingBufs[i].touch = 0;
    }

    enabled = OS_DisableInterrupts();
    sent = TPi_AutoSamplingOn(vcount, frequency);
    if (!sent) {
        OS_RestoreInterrupts(enabled);
        tpState.err_flg |= TP_REQUEST_COMMAND_FLAG_AUTO_ON;
        if (tpState.callback) {
            tpState.callback(SPI_PXI_COMMAND_TP_AUTO_ON, TP_RESULT_PXI_BUSY, 0);
        }
        return;
    }
    tpState.command_flg |= TP_REQUEST_COMMAND_FLAG_AUTO_ON;
    tpState.err_flg &= ~TP_REQUEST_COMMAND_FLAG_AUTO_ON;
    OS_RestoreInterrupts(enabled);
}

// TP_RequestAutoSamplingStopAsync
void TP_RequestAutoSamplingStopAsync(void) {
    OSIntrMode enabled = OS_DisableInterrupts();
    BOOL sent = FALSE;

    if (PXI_SendWordByFifo(PXI_FIFO_TAG_TOUCHPANEL,
                           SPI_PXI_START_BIT | SPI_PXI_END_BIT | TP_PXI_COMMAND(SPI_PXI_COMMAND_TP_AUTO_OFF),
                           FALSE) >= 0) {
        sent = TRUE;
    }
    if (!sent) {
        OS_RestoreInterrupts(enabled);
        tpState.err_flg |= TP_REQUEST_COMMAND_FLAG_AUTO_OFF;
        if (tpState.callback) {
            tpState.callback(SPI_PXI_COMMAND_TP_AUTO_OFF, TP_RESULT_PXI_BUSY, 0);
        }
        return;
    }
    tpState.command_flg |= TP_REQUEST_COMMAND_FLAG_AUTO_OFF;
    tpState.err_flg &= ~TP_REQUEST_COMMAND_FLAG_AUTO_OFF;
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
    if (tpState.frequence == 1 || tpState.bufSize == 1) {
        *point = tpState.samplingBufs[index];
        return;
    }

    for (i = 0; i < tpState.frequence && i < tpState.bufSize - 1; i++) {
        int j = index - i;
        TPData *sample;

        if (j < 0) {
            j += tpState.bufSize;
        }
        sample = &tpState.samplingBufs[j];
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
u32 TP_CalcCalibrateParam(TPCalibrateParam *calibrate, u16 rawX1, u16 rawY1, u16 screenX1, u16 screenY1, u16 rawX2,
                          u16 rawY2, u16 screenX2, u16 screenY2) {
    s32 dotSize;
    s32 origin;
    s32 rawYDiff;
    s32 screenYDiff;
    OSIntrMode enabled;

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
    reg_CP_DIV_NUMER_L = (rawX1 - rawX2) << 8;
    reg_CP_DIV_DENOM = (u32)(screenX1 - screenX2);
    rawYDiff = rawY1 - rawY2;
    screenYDiff = screenY1 - screenY2;
    while (reg_CP_DIVCNT & REG_CP_BUSY_MASK) {
    }
    dotSize = reg_CP_DIV_RESULT_L;
    reg_CP_DIVCNT = 0;
    reg_CP_DIV_NUMER_L = rawYDiff << 8;
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
    dotSize = reg_CP_DIV_RESULT_L;
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
    const TPiCalibrateParam *cal = &tpState.calibrate;

    if (!tpState.calibrate_flg) {
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
void TP_WaitBusy(TPRequestCommandFlag command) {
    while (tpState.command_flg & command) {
    }
}

// TP_CheckError: which of the commands failed
u32 TP_CheckError(TPRequestCommandFlag command) {
    return command & tpState.err_flg;
}
