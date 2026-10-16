#ifndef POKEBW2_TWL_SNDEX_H
#define POKEBW2_TWL_SNDEX_H

#include "types.h"
#include "nitro/pm.h"
#include "twl/pm.h"

// TwlSDK's sound extensions for the DSi (SNDEX), in the LTD autoload (lib/twl/src/snd/sndex.c): requests that the
// ARM9 sends the ARM7 over the PXI tag 0x18, one at a time. The ROM names none of it, so the functions keep their
// default names; the comments give what the code shows of them.

// What a request returns, and what the callback of an asynchronous one gets: the ARM7's answer is 0 for success or one
// of its error codes, which the library turns into the values below
typedef enum SNDEXResult {
    SNDEX_RESULT_SUCCESS = 0,
    SNDEX_RESULT_NOT_INITIALIZED = 1, // SNDEX_Init has not set the library up
    SNDEX_RESULT_ILLEGAL_PARAMETER = 2,
    SNDEX_RESULT_BUSY = 3,          // another request is under way, or the exclusive state is held
    SNDEX_RESULT_ILLEGAL_STATE = 4, // not on a DSi, or not in a thread
    SNDEX_RESULT_SEND_ERROR = 5,    // the PXI FIFO refused the word
    SNDEX_RESULT_ARM7_ERROR = 6,    // the ARM7 answered with its error 5
    SNDEX_RESULT_FATAL_ERROR = 7,   // an answer that no request expects, or the ARM7's errors 1 and 6 to 255
} SNDEXResult;

typedef void (*SNDEXCallback)(SNDEXResult result, void *arg);

// TwlSDK's SNDEX_Init, by its code: sets up the PXI channel to the ARM7's SNDEX and the callbacks that run before the
// console sleeps or exits, once. The game calls it only on a DSi
void SNDEX_Init(void);

// TwlSDK's SNDEX_SetIirFilter, by its five coefficients (an IIR biquad's n0, n1, n2, d1 and d2): sets the
// microphone's filter, which the game does on a DSi. Sends the target (the game passes 0) and the coefficients (the
// game's table has a sixth, which is not sent), and waits for the ARM7's answer. Not in a thread, or when the codec is
// not in its DSi mode, it returns SNDEX_RESULT_ILLEGAL_STATE
SNDEXResult SNDEX_SetIirFilter(int target, const u16 *coefficients);

// The request 0xfb, which the shutter sound makes: it waits for the answer, and the library then stays in a state in
// which every other request is refused, until func_02704940 ends it
SNDEXResult func_02704858(void);
// The request 0xfc, which ends that state; callback gets the answer. It needs the state that func_02704858 left
SNDEXResult func_02704940(SNDEXCallback callback, void *arg);

#endif // POKEBW2_TWL_SNDEX_H
