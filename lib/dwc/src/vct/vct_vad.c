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
struct VCTVADInfo {
    // Whether somebody is speaking
    u32 activity;
    u32 scale;
    // How many quiet frames end the speaking
    int releaseCount;
    int releaseTime;
};

static VCTVADInfo sVADInfo = { 0, 0, 15, 0x3fc };

static u32 sNoiseThreshold;
static int sLostCount;
static u32 sIndex;
static u32 E2scales[4];

void VCT_ResetVAD(void) {
    int i;

    for (i = 0; i < 4; i++) {
        E2scales[i] = 0x1000000;
    }
    sIndex = 0;
    sNoiseThreshold = 0;
    sVADInfo.activity = 0;
    sVADInfo.scale = 0;
    sLostCount = 0;
}

void VCT_GetVADInfo(s32 *out) {
    if (out != NULL) {
        MI_CpuCopy8(&sVADInfo, out, sizeof(VCTVADInfo));
    }
}

// Decides from the loudness of a frame whether a voice has started, goes on or has ended: 0 for none, 1 for the start,
// 2 for the middle and 3 for the end
int VCTi_GetVADStatus(u32 energy, u32 rms, u8 *level) {
    u32 scaled = rms >> 8;
    int result;

    sVADInfo.scale = scaled;
    if (level != NULL) {
        *level = scaled;
    }
    if (sVADInfo.activity == 0) {
        u32 sum = 0;
        u32 floor;
        u32 j;
        int i;

        for (i = 0; i < 4; i++) {
            sum += E2scales[i];
        }
        REG_SQRTCNT = 0;
        REG_SQRT_PARAM_LO = sum / 4;
        while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
        }
        floor = REG_SQRT_RESULT;
        if (rms != 0 && rms >= floor * 2) {
            sum = 0;
            for (j = 0; j < 4; j++) {
                sum += E2scales[j];
            }
            REG_SQRTCNT = 0;
            REG_SQRT_PARAM_LO = sum / 4;
            while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
            }
            sNoiseThreshold = REG_SQRT_RESULT * 3 / 2;
            sIndex = 0;
            sVADInfo.activity = 1;
            result = 1;
        } else {
            result = 0;
        }
        E2scales[sIndex] = energy;
        sIndex = (sIndex + 1) & 3;
    } else {
        u32 sum = 0;
        int i;

        E2scales[sIndex] = energy;
        sIndex = (sIndex + 1) & 3;
        for (i = 0; i < 4; i++) {
            sum += E2scales[i];
        }
        REG_SQRTCNT = 0;
        REG_SQRT_PARAM_LO = sum / 4;
        while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
        }
        if (REG_SQRT_RESULT <= sNoiseThreshold) {
            sLostCount++;
            if (sLostCount > sVADInfo.releaseCount) {
                sVADInfo.activity = 0;
                sLostCount = 0;
                sIndex = 0;
                return 3;
            }
        } else {
            sLostCount = 0;
        }
        result = 2;
    }
    return result;
}

void vct_init_vad(void) {
    sVADInfo.releaseCount = 15;
    sVADInfo.releaseTime = sVADInfo.releaseCount * 0x44;
    VCT_ResetVAD();
}
