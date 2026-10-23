#ifndef POKEBW2_DWC_VCT_DSP_INTERNAL_H
#define POKEBW2_DWC_VCT_DSP_INTERNAL_H

#include "types.h"
#include "dwc/vct_dsp.h"
#include "struct_decls.h"
#include "vct_internal.h"

// A voice packet that has come in, on its way to being played
struct UDPPacket {
    UDPPacket *free;
    UDPPacket *prev;
    UDPPacket *next;
    int codec;
    u8 remote;
    u8 channel;
    u8 pad12[2];
    int length;
    u32 sequence;
    s16 buffer[VCT_FRAME_SAMPLES];
    int marker;
    u32 timestamp;
    u64 receive_time;
    u64 performance_time;
    u8 *samples;
};

// vct_packet_pool.c: packet memory; vct_vad.c: microphone levels; vct_dsp.c: the filter and the echo; vct_mixer.c
BOOL vct_init_packet_buffer(void *buffer, u32 size);
void vct_cleanup_packet_buffer(void);
UDPPacket *vct_alloc_packet_buffer(void);
void vct_free_packet_buffer(UDPPacket *packet);
void VCT_ResetVAD(void);
int VCTi_GetVADStatus(u32 energy, u32 rms, u8 *level);
void vct_init_vad(void);
u32 HPFFilter(s16 *samples, int bytes);
void InitFIRFilter(void);
void vct_set_speaker_samples(s16 *samples, int bytes);
void vct_process_fir(s16 *samples, int bytes, u32 rms);
void vct_mix_audio(s16 *a, s16 *b, s16 *out, u32 bytes);

#endif // POKEBW2_DWC_VCT_DSP_INTERNAL_H
