#ifndef POKEBW2_DWC_VCT_AUDIO_H
#define POKEBW2_DWC_VCT_AUDIO_H

#include "types.h"
#include "struct_decls.h"

// Hands a frame of 0x440 bytes of the microphone's samples over to be sent
BOOL VCT_SendAudio(void *samples, u32 size);
// Fills a frame with the voices that are due, returning whether there was any, and gives the machines heard as bits
BOOL VCT_ReceiveAudio(void *dst, u32 size, u32 *heardMask);
// The five ways to code a frame, from 0: 8-bit samples, mu-law and ADPCM at 2, 3 and 4 bits. Not all are allowed
// in every mode. The SDK's VCTCodec (ntrtwl's libvct, include/vct.h)
typedef enum VCTCodec {
    VCT_CODEC_8BIT_RAW,
    VCT_CODEC_G711_ULAW,
    VCT_CODEC_2BIT_ADPCM,
    VCT_CODEC_3BIT_ADPCM,
    VCT_CODEC_4BIT_ADPCM,
    VCT_CODEC_END,
} VCTCodec;
BOOL VCT_SetCodec(VCTCodec codec);
// Whether frames with no voice in them are left out
void VCT_EnableVAD(BOOL enabled);
// Whether to keep the microphone from picking up what the speaker plays
void VCT_EnableEchoCancel(BOOL enabled);
BOOL VCT_StartStreaming(VCTSession *session);
void VCT_StopStreaming(VCTSession *session);

#endif // POKEBW2_DWC_VCT_AUDIO_H
