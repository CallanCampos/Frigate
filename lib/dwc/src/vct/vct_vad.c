// The microphone's loudness: whether somebody is speaking, from the loudness of the frames before. The second of four
// files that the ROM's vct_dsp.c range is, with its own `.data` and `.bss` (libVCT's reconstruction calls it vad.c).
// Its threshold, index and quiet frame count are scalars: with `-ipa file` they take the one base and the order of the
// ROM.

#include "nitro/cp.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "vct_dsp_internal.h"
#include "vct_internal.h"

// What the library has made of the microphone's loudness
struct VCTCapture {
    // Whether somebody is speaking
    u32 speaking;
    u32 level;
    // How many quiet frames end the speaking
    int quietFrameLimit;
    int unkC;
};

static VCTCapture sCapture = { 0, 0, 15, 0x3fc };

static u32 sNoiseThreshold;
static int sQuietFrames;
static u32 sNoiseIndex;
static u32 sLoudnessHistory[4];

void VCT_ResetCapture(void) {
    int i;

    for (i = 0; i < 4; i++) {
        sLoudnessHistory[i] = 0x1000000;
    }
    sNoiseIndex = 0;
    sNoiseThreshold = 0;
    sCapture.speaking = 0;
    sCapture.level = 0;
    sQuietFrames = 0;
}

void VCT_GetCaptureLevels(s32 *out) {
    if (out != NULL) {
        MI_CpuCopy8(&sCapture, out, sizeof(VCTCapture));
    }
}

// Decides from the loudness of a frame whether a voice has started, goes on or has ended: 0 for none, 1 for the start,
// 2 for the middle and 3 for the end
int VCT_UpdateVad(u32 energy, u32 rms, u8 *level) {
    u32 scaled = rms >> 8;
    int result;

    sCapture.level = scaled;
    if (level != NULL) {
        *level = scaled;
    }
    if (sCapture.speaking == 0) {
        u32 sum = 0;
        u32 floor;
        u32 j;
        int i;

        for (i = 0; i < 4; i++) {
            sum += sLoudnessHistory[i];
        }
        REG_SQRTCNT = 0;
        REG_SQRT_PARAM_LO = sum / 4;
        while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
        }
        floor = REG_SQRT_RESULT;
        if (rms != 0 && rms >= floor * 2) {
            sum = 0;
            for (j = 0; j < 4; j++) {
                sum += sLoudnessHistory[j];
            }
            REG_SQRTCNT = 0;
            REG_SQRT_PARAM_LO = sum / 4;
            while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
            }
            sNoiseThreshold = REG_SQRT_RESULT * 3 / 2;
            sNoiseIndex = 0;
            sCapture.speaking = 1;
            result = 1;
        } else {
            result = 0;
        }
        sLoudnessHistory[sNoiseIndex] = energy;
        sNoiseIndex = (sNoiseIndex + 1) & 3;
    } else {
        u32 sum = 0;
        int i;

        sLoudnessHistory[sNoiseIndex] = energy;
        sNoiseIndex = (sNoiseIndex + 1) & 3;
        for (i = 0; i < 4; i++) {
            sum += sLoudnessHistory[i];
        }
        REG_SQRTCNT = 0;
        REG_SQRT_PARAM_LO = sum / 4;
        while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
        }
        if (REG_SQRT_RESULT <= sNoiseThreshold) {
            sQuietFrames++;
            if (sQuietFrames > sCapture.quietFrameLimit) {
                sCapture.speaking = 0;
                sQuietFrames = 0;
                sNoiseIndex = 0;
                return 3;
            }
        } else {
            sQuietFrames = 0;
        }
        result = 2;
    }
    return result;
}

void VCT_InitCapture(void) {
    sCapture.quietFrameLimit = 15;
    sCapture.unkC = sCapture.quietFrameLimit * 0x44;
    VCT_ResetCapture();
}
