#ifndef POKEBW2_DWC_VCT_ADPCM_H
#define POKEBW2_DWC_VCT_ADPCM_H

#include "types.h"
#include "struct_decls.h"

// The predictor and step index that every sample of an ADPCM stream is coded against, which the stream's packets
// start with
struct VCTAdpcmState {
    s16 predictor;
    u8 stepIndex;
    u8 pad;
};

// vct_adpcm.c
void VCT_DecodeAdpcm(u8 *packet, s16 *dst, int size, int bits);
void VCT_EncodeAdpcm(u8 *packet, s16 *src, int count, VCTAdpcmState *state, int bits);

#endif // POKEBW2_DWC_VCT_ADPCM_H
