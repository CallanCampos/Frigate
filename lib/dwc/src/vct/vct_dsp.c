// The high-pass filter on the microphone and the canceller of the speaker's echo. The third of four files that the
// ROM's vct_dsp.c range is; it keeps the old name, since the configs can only shrink an entry, not rename it (libVCT's
// reconstruction calls it fir.c). The speaker's frame goes through a filter of the echo's way back, and the microphone
// is turned down by more the louder the echo is. The divider and the square root unit of the hardware do the sums.

#include "nitro/cp.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "vct_dsp_internal.h"
#include "vct_internal.h"

static int sCounterIn = 0;
static int sCounterOut = 0;
static s32 sH[6];
static s32 sTap[6];
static int sPrevOut;
static int sPrevIn;
static s16 sSpkBuffer[VCT_FRAME_SAMPLES];
static s16 sDelayLine[2048];

// Filters a frame in place with a high-pass filter and returns the mean of the squares of the result
u32 HPFFilter(s16 *samples, int bytes) {
    s16 input = samples[0];
    int scaled = input * 0xe9f;
    int minusInput = -scaled;
    u32 i;
    u64 sum;
    int output = (scaled + sPrevIn + sPrevOut * 0xd3e) >> 12;

    bytes /= sizeof(s16);
    sum = output * output;
    sPrevOut = output;
    sPrevIn = minusInput;
    i = 1;
    if (i < bytes) {
        do {
            int scaled = samples[i] * 0xe9f;
            int next = scaled + minusInput + output * 0xd3e;

            samples[i - 1] = output;
            output = next >> 12;
            sum += output * output;
            minusInput = -scaled;
        } while (++i < bytes);
        sPrevIn = minusInput;
        sPrevOut = output;
    }
    *(samples + bytes - 1) = output;
    CP_SetDiv64_32(sum, bytes);
    return CP_GetDivResult32();
}

#define MAX_DELAY_LINE (sizeof(sDelayLine) / sizeof(s16))

static inline s16 process_sample_unroll(s16 spkSample) {
    int acc = 0;

    sDelayLine[sCounterIn] = spkSample;
    sTap[0] = sDelayLine[sCounterOut];
    acc = sTap[5] * sH[5];
    acc += sTap[4] * sH[4];
    sTap[5] = sTap[4];
    acc += sTap[3] * sH[3];
    sTap[4] = sTap[3];
    acc += sTap[2] * sH[2];
    sTap[3] = sTap[2];
    acc += sTap[1] * sH[1];
    sTap[2] = sTap[1];
    acc += sTap[0] * sH[0];
    sTap[1] = sTap[0];
    if (acc > 0x7fff << 15) {
        acc = 0x7fff << 15;
    } else if (acc < -0x8000 << 15) {
        acc = -0x8000 << 15;
    }
    sCounterIn = (sCounterIn + 1) % MAX_DELAY_LINE;
    sCounterOut = (sCounterOut + 1) % MAX_DELAY_LINE;
    return acc >> 15;
}

static s16 sInitialCoefficients[6] = { -5637, 16445, -14404, 11473, -12246, 892 };

void InitFIRFilter(void) {
    s16 *coeffs = sInitialCoefficients;
    int delay = 1635;
    int i;

    sCounterIn = 0;
    sCounterOut = 2048 - delay;
    for (i = 0; i < 2048; i++) {
        sDelayLine[i] = 0;
    }
    for (i = 0; i < 6; i++) {
        sTap[i] = 0;
        sH[i] = coeffs[i];
    }
}

// Keeps the frame that was played, to know what the microphone heard of it
void vct_set_speaker_samples(s16 *samples, int bytes) {
    MIi_CpuCopyFast(samples, sSpkBuffer, bytes);
}

// Runs the speaker's frame through a filter of the echo's way back, and turns the microphone down by more the louder
// the echo is
static void DoFIRFilter(s16 *mic, s16 *far, u32 bytes, u32 rms) {
    int i;
    u64 sum = 0;
    u32 gain;
    static s16 shift = 0;

    bytes /= sizeof(s16);
    for (i = 0; i < bytes; i++) {
        far[i] = process_sample_unroll(far[i]);
        sum += far[i] * far[i];
        far[i] = mic[i];
    }
    CP_SetDiv64_32(sum, bytes);
    gain = CP_GetDivResult32();
    REG_SQRTCNT = 0;
    REG_SQRT_PARAM_LO = gain;
    CP_WaitSqrt();
    gain = CP_GetSqrtResultImm32();
    if (gain * 3 < rms * 2) {
        shift = 0;
        return;
    }
    if (rms < gain) {
        shift = 4;
    }
    if (shift < 4) {
        shift++;
    }
    for (i = 0; i < bytes; i++) {
        mic[i] >>= shift;
    }
}

void vct_process_fir(s16 *samples, int bytes, u32 rms) {
    DoFIRFilter(samples, sSpkBuffer, bytes, rms);
}
