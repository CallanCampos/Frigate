#ifndef POKEBW2_DWC_VCT_AUDIO_INTERNAL_H
#define POKEBW2_DWC_VCT_AUDIO_INTERNAL_H

#include "types.h"
#include "dwc/vct_audio.h"
#include "struct_decls.h"

// A voice packet as it goes over the wire
struct AudioHeader {
    u32 magic;
    u8 header;
    // The codec, with the top bit set on the first frame after a silence
    u8 codec;
    u16 sequence;
    u32 timestamp;
    // The frame, coded in place from its 0x440 bytes of samples, after 4 bytes of state for the ADPCM codecs
    u8 payload[0x444];
};

// vct_audio.c: the frames going out and coming in
BOOL vct_init_audio(void);
void vct_finish_audio(void);
BOOL vct_handle_audio(int aid, AudioHeader *packet, int size, u64 now);
int vct_decode_audio_buffer(void);
BOOL vct_flush_send_buffer(void);

#endif // POKEBW2_DWC_VCT_AUDIO_INTERNAL_H
