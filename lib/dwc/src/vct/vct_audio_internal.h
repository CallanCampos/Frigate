#ifndef POKEBW2_DWC_VCT_AUDIO_INTERNAL_H
#define POKEBW2_DWC_VCT_AUDIO_INTERNAL_H

#include "types.h"
#include "dwc/vct_audio.h"
#include "struct_decls.h"

// A voice packet as it goes over the wire
struct VCTVoicePacket {
    u32 magic;
    u8 type;
    // The codec, with the top bit set on the first frame after a silence
    u8 codec;
    u16 sequence;
    u32 timestamp;
    // The frame, coded in place from its 0x440 bytes of samples, after 4 bytes of state for the ADPCM codecs
    u8 payload[0x444];
};

// vct_audio.c: the frames going out and coming in
BOOL VCT_InitAudio(void);
void VCT_ClearSessionCount(void);
BOOL VCT_HandleVoicePacket(int aid, VCTVoicePacket *packet, int size, u64 now);
int VCT_DecodeReceived(void);
BOOL VCT_SendPending(void);

#endif // POKEBW2_DWC_VCT_AUDIO_INTERNAL_H
