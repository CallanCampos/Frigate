#ifndef POKEBW2_DWC_VCT_AUDIO_H
#define POKEBW2_DWC_VCT_AUDIO_H

#include "types.h"
#include "struct_decls.h"

// Hands a frame of 0x440 bytes of the microphone's samples over to be sent
BOOL VCT_SubmitSamples(void *samples, u32 size);
// Fills a frame with the voices that are due, returning whether there was any, and gives the machines heard as bits
BOOL VCT_ReadSamples(void *dst, u32 size, u32 *heardMask);
// The five ways to code a frame, from 0: 8-bit samples, mu-law and ADPCM at 2, 3 and 4 bits. Not all are allowed
// in every mode. The SDK's VCTCodec (ntrtwl's libvct, include/vct.h)
typedef enum VCTCodec {
    VCT_CODEC_PCM8,
    VCT_CODEC_MULAW,
    VCT_CODEC_ADPCM2,
    VCT_CODEC_ADPCM3,
    VCT_CODEC_ADPCM4,
    VCT_CODEC_MAX,
} VCTCodec;
BOOL VCT_SetCodec(VCTCodec codec);
// Whether frames with no voice in them are left out
void VCT_SetVad(BOOL enabled);
// Whether to keep the microphone from picking up what the speaker plays
void VCT_SetEchoCancel(BOOL enabled);
BOOL VCT_AttachSession(VCTSession *session);
void VCT_DetachSession(VCTSession *session);

#endif // POKEBW2_DWC_VCT_AUDIO_H
