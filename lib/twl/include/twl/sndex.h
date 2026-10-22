#ifndef POKEBW2_TWL_SNDEX_H
#define POKEBW2_TWL_SNDEX_H

#include "types.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "twl/pm.h"

// TwlSDK's sound extensions for the DSi (SNDEX), in the LTD autoload (lib/twl/src/snd/sndex.c): requests that the
// ARM9 sends the ARM7 over the PXI tag 0x18, one at a time. The ROM names none of it: the public types and inlines
// have the names of the TWL-SDK Function Reference Manual (SNDEX_SetIirFilter, SNDEXIirFilterParam,
// SNDEXIirFilterTarget), and the library's functions and the other result codes those of libntr's reconstruction of
// TwlSDK's sndex.h, where the code shows them.

// What a request returns, and what the callback of an asynchronous one gets: the ARM7's answer is 0 for success or one
// of its error codes, which the library turns into the values below
typedef enum SNDEXResult {
    SNDEX_RESULT_SUCCESS = 0,
    SNDEX_RESULT_BEFORE_INIT = 1, // SNDEXi_Init has not set the library up
    SNDEX_RESULT_INVALID_PARAM = 2,
    SNDEX_RESULT_EXCLUSIVE = 3,      // another request is under way, or the exclusive state is held
    SNDEX_RESULT_ILLEGAL_STATE = 4,  // not on a DSi, or not in a thread
    SNDEX_RESULT_PXI_SEND_ERROR = 5, // the PXI FIFO refused the word
    SNDEX_RESULT_DEVICE_ERROR = 6,   // the ARM7 answered with its error 5
    SNDEX_RESULT_FATAL_ERROR = 7,    // an answer that no request expects, or the ARM7's errors 1 and 6 to 255
} SNDEXResult;

typedef void (*SNDEXCallback)(SNDEXResult result, void *arg);

// The coefficients of an IIR biquad filter
typedef struct SNDEXIirFilterParam {
    u16 n0;
    u16 n1;
    u16 n2;
    u16 d1;
    u16 d2;
} SNDEXIirFilterParam;

// Where a filter goes: the five filters of the microphone's ADC, and those of the left, right or both DAC channels
typedef enum SNDEXIirFilterTarget {
    SNDEX_IIR_FILTER_ADC_1 = 0,
    SNDEX_IIR_FILTER_ADC_2,
    SNDEX_IIR_FILTER_ADC_3,
    SNDEX_IIR_FILTER_ADC_4,
    SNDEX_IIR_FILTER_ADC_5,
    SNDEX_IIR_FILTER_DAC_LEFT_1,
    SNDEX_IIR_FILTER_DAC_LEFT_2,
    SNDEX_IIR_FILTER_DAC_LEFT_3,
    SNDEX_IIR_FILTER_DAC_LEFT_4,
    SNDEX_IIR_FILTER_DAC_LEFT_5,
    SNDEX_IIR_FILTER_DAC_RIGHT_1,
    SNDEX_IIR_FILTER_DAC_RIGHT_2,
    SNDEX_IIR_FILTER_DAC_RIGHT_3,
    SNDEX_IIR_FILTER_DAC_RIGHT_4,
    SNDEX_IIR_FILTER_DAC_RIGHT_5,
    SNDEX_IIR_FILTER_DAC_BOTH_1,
    SNDEX_IIR_FILTER_DAC_BOTH_2,
    SNDEX_IIR_FILTER_DAC_BOTH_3,
    SNDEX_IIR_FILTER_DAC_BOTH_4,
    SNDEX_IIR_FILTER_DAC_BOTH_5,
    SNDEX_IIR_FILTER_TARGET_MAX
} SNDEXIirFilterTarget;

// Sets up the PXI channel to the ARM7's SNDEX and the callbacks that run before the console sleeps or exits, once
void SNDEXi_Init(void);

// Sets the filter of the target and waits for the ARM7's answer. Not in a thread, or when the codec is not in its DSi
// mode, it returns SNDEX_RESULT_ILLEGAL_STATE
SNDEXResult SNDEXi_SetIirFilter(SNDEXIirFilterTarget target, const SNDEXIirFilterParam *pParam);

// The request 0xfb, which the shutter sound makes: it waits for the answer, and the library then stays in a state in
// which every other request is refused, until SNDEXi_PostProcessForShutterSound ends it
SNDEXResult SNDEXi_PreProcessForShutterSound(void);
// The request 0xfc, which ends that state; callback gets the answer. It needs the state that
// SNDEXi_PreProcessForShutterSound left
SNDEXResult SNDEXi_PostProcessForShutterSound(SNDEXCallback callback, void *arg);

// The SDK's inlines over them, which only call the library on a DSi
static inline void SNDEX_Init(void) {
    if (OS_IsRunOnTwl() == TRUE) {
        SNDEXi_Init();
    }
}

static inline SNDEXResult SNDEX_SetIirFilter(SNDEXIirFilterTarget target, const SNDEXIirFilterParam *pParam) {
    return OS_IsRunOnTwl() == TRUE ? SNDEXi_SetIirFilter(target, pParam) : SNDEX_RESULT_ILLEGAL_STATE;
}

#endif // POKEBW2_TWL_SNDEX_H
