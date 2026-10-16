#ifndef POKEBW2_NITRO_TP_H
#define POKEBW2_NITRO_TP_H

#include "types.h"

// NitroSDK's touch panel library

typedef struct {
    u16 x;
    u16 y;
    u16 touch;
    u16 validity;
} TPData;

typedef struct {
    s16 x0;
    s16 y0;
    s16 xDotSize;
    s16 yDotSize;
} TPCalibrateParam;

// Which coordinates of a sample are valid
#define TP_VALIDITY_VALID 0
#define TP_VALIDITY_INVALID_X 1
#define TP_VALIDITY_INVALID_Y 2
#define TP_VALIDITY_INVALID_XY 3

#define TP_REQUEST_COMMAND_FLAG_SAMPLING 0x1
#define TP_REQUEST_COMMAND_FLAG_AUTO_ON 0x2
#define TP_REQUEST_COMMAND_FLAG_AUTO_OFF 0x4

void TP_Init(void);
BOOL TP_GetUserInfo(TPCalibrateParam *calibrate);
void TP_SetCalibrateParam(const TPCalibrateParam *calibrate);
void TP_RequestSamplingAsync(void);
u32 TP_WaitRawResult(TPData *result);
void TP_RequestAutoSamplingStartAsync(u16 vcount, u16 frequency, TPData samples[], u16 count);
void TP_RequestAutoSamplingStopAsync(void);
void TP_GetLatestRawPointInAuto(TPData *point);
// The index in the sample buffer of the newest sample of the automatic sampling
u16 TP_GetLatestIndexInAuto(void);
BOOL TP_CalcCalibrateParam(TPCalibrateParam *calibrate, u16 rawX1, u16 rawY1, u16 screenX1, u16 screenY1, u16 rawX2,
                           u16 rawY2, u16 screenX2, u16 screenY2);
void TP_GetCalibratedPoint(TPData *disp, const TPData *raw);
void TP_WaitBusy(u32 command);
u32 TP_CheckError(u32 command);

#endif // POKEBW2_NITRO_TP_H
