// Adds two frames of samples, clamping each sum. The last of four files that the ROM's vct_dsp.c range is (libVCT's
// reconstruction calls it mixer.c).

#include "vct_dsp_internal.h"

void VCT_MixSamples(s16 *a, s16 *b, s16 *out, u32 bytes) {
    u32 count = bytes >> 1;
    u32 i;

    for (i = 0; i < count; i += 4) {
        int sum = a[i] + b[i];

        if (sum > 0x7fff) {
            sum = 0x7fff;
        } else if (sum < -0x8000) {
            sum = -0x8000;
        }
        out[i] = sum;
        sum = a[i + 1] + b[i + 1];
        if (sum > 0x7fff) {
            sum = 0x7fff;
        } else if (sum < -0x8000) {
            sum = -0x8000;
        }
        out[i + 1] = sum;
        sum = a[i + 2] + b[i + 2];
        if (sum > 0x7fff) {
            sum = 0x7fff;
        } else if (sum < -0x8000) {
            sum = -0x8000;
        }
        out[i + 2] = sum;
        sum = a[i + 3] + b[i + 3];
        if (sum > 0x7fff) {
            sum = 0x7fff;
        } else if (sum < -0x8000) {
            sum = -0x8000;
        }
        out[i + 3] = sum;
    }
}
