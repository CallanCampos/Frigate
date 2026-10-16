#ifndef POKEBW2_NITRO_MIC_H
#define POKEBW2_NITRO_MIC_H

#include "types.h"
#include "nitro/pm.h"

// NitroSDK's microphone (TwlSDK's libspi mic.c). Its amplifier is turned on by the power management of nitro/pm.h,
// which this header includes. The functions keep their default names; the comments give the SDK functions they are,
// from how the code works. The Limited functions hand over to the DSi's MICEX code on a DSi and otherwise call the
// plain ones (swan named them after these)

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

void func_0207e75c(void);                                                            // MIC_Init
MICResult func_0207e7a0(const MICAutoParam *param, MICCallback callback, void *arg); // MIC_StartAutoSamplingAsync
MICResult func_0207e870(const MICAutoParam *param);                                  // the synchronous start
MICResult func_0207e894(MICCallback callback, void *arg);                            // MIC_StopAutoSamplingAsync
MICResult func_0207e8d4(void);                                                       // the synchronous stop
void *func_0207e8f8(void);                                                           // MIC_GetLastSamplingAddress
MICResult func_0207e904(const MICAutoParam *param, MICCallback callback, void *arg); // MIC_StartLimitedSamplingAsync
MICResult MIC_StartAutoSampling(const MICAutoParam *param);                          // MIC_StartLimitedSampling
MICResult MIC_StopAutoSampling(void);                                                // MIC_StopLimitedSampling
MICWork *func_0207eb1c(void);                                                        // MICi_GetSysWork

#endif // POKEBW2_NITRO_MIC_H
