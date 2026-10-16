#ifndef POKEBW2_NITRO_MIC_H
#define POKEBW2_NITRO_MIC_H

#include "types.h"
#include "nitro/pm.h"

// NitroSDK's microphone (TwlSDK's libspi mic.c). Its amplifier is turned on by the power management of nitro/pm.h,
// which this header includes. The functions have the names of the SDK functions they are, from how the code works. The
// Limited functions hand over to the DSi's MICEX code on a DSi and otherwise call the plain ones (swan named them after
// these)

typedef enum {
    MIC_RESULT_SUCCESS,
    MIC_RESULT_BUSY,
    MIC_RESULT_ILLEGAL_PARAMETER,
    MIC_RESULT_SEND_ERROR,
    MIC_RESULT_INVALID_COMMAND,
    MIC_RESULT_ILLEGAL_STATUS,
    MIC_RESULT_FATAL_ERROR,
} MICResult;

// How the ARM7 samples: 8 or 12 bits, signed or not, and with its filter on or off
typedef enum {
    MIC_SAMPLING_TYPE_8BIT,
    MIC_SAMPLING_TYPE_12BIT,
    MIC_SAMPLING_TYPE_SIGNED_8BIT,
    MIC_SAMPLING_TYPE_SIGNED_12BIT,
    MIC_SAMPLING_TYPE_12BIT_FILTER_OFF,
    MIC_SAMPLING_TYPE_SIGNED_12BIT_FILTER_OFF,
} MICSamplingType;

// The sampling rate is the number of ARM7 clocks (33.51 MHz) between two samples, and the buffer and its size are
// multiples of 32 bytes, which the cache needs
#define MIC_SAMPLING_RATE_MIN 0x400
// 8180 Hz
#define MIC_SAMPLING_RATE_8180 0x1001

typedef void (*MICCallback)(MICResult result, void *arg);

typedef struct {
    MICSamplingType type;
    void *buffer;
    u32 size;
    u32 rate;
    BOOL loop;
    MICCallback fullCallback;
    void *fullArg;
} MICAutoParam;

// The library's state, which the DSi's MICEX code in the LTD autoload reaches through MICi_GetSysWork. The busy flag is
// set from the call that starts or stops the sampling until the ARM7 answers
typedef struct {
    BOOL busy;
    MICCallback callback;
    void *callbackArg;
    MICResult result;
    MICCallback fullCallback;
    void *fullArg;
    u16 *sampleDest;
} MICWork;

void MIC_Init(void);
MICResult MIC_StartAutoSamplingAsync(const MICAutoParam *param, MICCallback callback, void *arg);
MICResult MIC_StartAutoSampling(const MICAutoParam *param); // the synchronous start
MICResult MIC_StopAutoSamplingAsync(MICCallback callback, void *arg);
MICResult MIC_StopAutoSampling(void); // the synchronous stop
void *MIC_GetLastSamplingAddress(void);
MICResult MIC_StartLimitedSamplingAsync(const MICAutoParam *param, MICCallback callback, void *arg);
MICResult MIC_StartLimitedSampling(const MICAutoParam *param); // swan's MIC_StartAutoSampling
MICResult MIC_StopLimitedSampling(void);                       // swan's MIC_StopAutoSampling
MICWork *MICi_GetSysWork(void);

#endif // POKEBW2_NITRO_MIC_H
