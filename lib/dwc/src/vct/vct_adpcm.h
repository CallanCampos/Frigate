#ifndef POKEBW2_DWC_VCT_ADPCM_H
#define POKEBW2_DWC_VCT_ADPCM_H

#include "types.h"
#include "dwc/vct_audio.h"
#include "struct_decls.h"

// The predictor and step index that every sample of an ADPCM stream is coded against, which the stream's packets
// start with
struct AdpcmState {
    s16 prevSample;
    u8 prevIndex;
    u8 pad;
};

// vct_adpcm.c
void vct_decode_adpcm(u8 *packet, s16 *dst, u32 size, VCTCodec bits);
void vct_encode_adpcm(u8 *packet, s16 *src, u32 count, AdpcmState *state, VCTCodec bits);

#endif // POKEBW2_DWC_VCT_ADPCM_H
