#ifndef POKEBW2_DWC_VCT_BUFFER_INTERNAL_H
#define POKEBW2_DWC_VCT_BUFFER_INTERNAL_H

#include "types.h"
#include "dwc/vct_audio.h"
#include "struct_decls.h"

// What one machine's voice has gone through on the way to being played
struct AudioStream {
    u32 avgDelay;
    u32 playoutDelay;
    u32 queuedCount;
    int skewDifference;
    u32 lastSequenceSeen;
    u32 lostCount;
    u32 unk18;
    u32 lateCount;
    u32 playedCount;
    VCTCodec codec;
    u32 lastDelay;
    u32 avgDelayCopy;
    u32 syncSequence;
    u32 channel;
    int aid;
    UDPPacket *prevBuffer;
    int unk40;
    u64 firstArrival;
    u32 firstTimestamp;
    u32 unk50;
    u32 lastSequence;
    u32 lastPlayedSequence;
    int needsReset;
    u32 wrap;
    u32 expectedSequence;
    u16 sequence16;
    u8 pad6a[2];
    u32 warmup;
    int unk70;
    int skewAverage;
    int skewFirst;
    int jitterAverage;
    int lastSkew;
    int burstCount;
    int delayFloor;
    u32 delayCeiling;
    u32 belowCount;
};

// vct_buffer.c: the packets on their way out and in
void vct_init_decoder(void);
void vct_flush_decoder(u32 slot);
void vct_flush_all_decoder(void);
int vct_insert_decoder(UDPPacket *packet);
UDPPacket *vct_top_decoder(void);
int vct_count_decoder_queue(u32 slot);
UDPPacket *vct_get_packet_from_decoder(u32 slot);
int vct_remove_packet_from_decoder(UDPPacket *packet);
void vct_encode_g711_ulaw(u8 *dst, s16 *src, u32 count);
void vct_decode_g711_ulaw(u8 *src, s16 *dst, u32 count);
void vct_encode_8bit_raw(u8 *dst, s16 *src, u32 count);
void vct_decode_8bit_raw(s8 *src, s16 *dst, u32 count);
void vct_init_audio_queue(u32 slot);
void vct_flush_audio_queue(u32 slot);
void vct_flush_all_audio_queue(void);
int vct_insert_audio_queue(UDPPacket *packet, u32 slot);
UDPPacket *vct_top_audio_queue(u32 slot);
int vct_count_audio_queue(u32 slot);
void vct_init_stream(AudioStream *receiver, u32 slot, int aid);
void vct_reset_stream(AudioStream *receiver);
int vct_save_previous_buffer(AudioStream *receiver, UDPPacket *packet);
BOOL vct_calc_jitter_and_skew(AudioStream *receiver, UDPPacket *packet);

#endif // POKEBW2_DWC_VCT_BUFFER_INTERNAL_H
