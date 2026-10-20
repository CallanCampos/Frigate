#ifndef POKEBW2_DWC_VCT_DSP_INTERNAL_H
#define POKEBW2_DWC_VCT_DSP_INTERNAL_H

#include "types.h"
#include "dwc/vct_dsp.h"
#include "struct_decls.h"
#include "vct_internal.h"

// A voice packet that has come in, on its way to being played
struct VCTPacket {
    VCTPacket *nextFree;
    VCTPacket *prev;
    VCTPacket *next;
    int codec;
    u8 aid;
    u8 slot;
    u8 pad12[2];
    int dataSize;
    u32 sequence;
    s16 samples[VCT_FRAME_SAMPLES];
    int silence;
    u32 timestamp;
    u64 arrival;
    u64 playTime;
    u8 *data;
};

// vct_packet_pool.c: packet memory; vct_vad.c: microphone levels; vct_dsp.c: the filter and the echo; vct_mixer.c
BOOL VCT_InitPacketPool(void *buffer, u32 size);
void VCT_ClearPacketPool(void);
VCTPacket *VCT_AllocPacket(void);
void VCT_FreePacket(VCTPacket *packet);
void VCT_ResetCapture(void);
int VCT_UpdateVad(u32 energy, u32 rms, u8 *level);
void VCT_InitCapture(void);
u32 VCT_FilterFrame(s16 *samples, int bytes);
void VCT_InitEcho(void);
void VCT_SaveFarEnd(s16 *samples, int bytes);
void VCT_CancelEcho(s16 *samples, int bytes, u32 rms);
void VCT_MixSamples(s16 *a, s16 *b, s16 *out, u32 bytes);

#endif // POKEBW2_DWC_VCT_DSP_INTERNAL_H
