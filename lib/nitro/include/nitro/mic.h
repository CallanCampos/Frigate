#ifndef POKEBW2_NITRO_MIC_H
#define POKEBW2_NITRO_MIC_H

#include "types.h"

// NitroSDK's microphone, and the power management calls that turn its amplifier on. The functions keep their default
// names; the comments give the SDK functions they appear to be from how the Xtransceiver uses them

typedef enum {
    MIC_RESULT_SUCCESS,
} MICResult;

#define MIC_SAMPLING_TYPE_8BIT 0
#define MIC_SAMPLING_TYPE_SIGNED_12BIT 3
// 8180 Hz, in ARM7 clocks
#define MIC_SAMPLING_RATE_8180 0x1001

typedef void (*MICCallback)(MICResult result, void *arg);

typedef struct {
    int type;
    void *buffer;
    u32 size;
    u32 rate;
    BOOL loop;
    MICCallback fullCallback;
    void *fullArg;
} MICAutoParam;

void func_0207e75c(void);  // MIC_Init
void *func_0207e8f8(void); // MIC_GetLastSamplingAddress
MICResult MIC_StartAutoSampling(MICAutoParam *param);
MICResult MIC_StopAutoSampling(void);

#define PM_AMP_OFF 0
#define PM_AMP_ON 1

void func_0207ebb8(void);     // PM_Init
void func_0207efc4(int amp);  // PM_SetAmp
void func_0207f008(int gain); // PM_SetAmpGain

#endif // POKEBW2_NITRO_MIC_H
