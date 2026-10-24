#include "dwc/vct_session.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "vct_adpcm.h"
#include "vct_audio_internal.h"
#include "vct_buffer_internal.h"
#include "vct_dsp_internal.h"
#include "vct_internal.h"

// The sound going out and coming in. A frame of the microphone is filtered, coded and sent to every machine that
// listens; the packets of the other machines are decoded, wait in a queue each (vct_buffer.c), and are mixed and played
// once their time comes.

// The bytes of a slot of the send queue: room for an uncoded frame, 0x450 bytes (the 12-byte header, 4 bytes of
// ADPCM state and 0x440 bytes of samples), most likely rounded up to 32 bytes so that both slots of the 32-aligned
// queue start on a cache line
#define VCT_SEND_BUFFER_SIZE 0x460

// The two frames going out: one is filled while the other is sent. Each is built as a packet in a slot of the buffer
// and coded in place before it is sent.
struct VCTSendInfo {
    u8 buffer[2 * VCT_SEND_BUFFER_SIZE];
    u32 read;
    u32 write;
    AudioHeader *ptr;
    u32 bitmap;
    u32 scheduleColumn;
    int scheduleRow;
};

static AudioHeader *vct_prepare_send_buffer(void);
static BOOL VCTi_ReceiveAudioData(u8 *dst, int size, int slot, u32 *heardMask);

// The frame sizes of the codecs in units of 0x44 bytes, and the bytes more
static const u8 sCodecInfo[VCT_CODEC_END][2] = { { 8, 0 }, { 8, 0 }, { 2, 4 }, { 3, 4 }, { 4, 4 } };

// How many packets to send in each of the ticks that follow a frame, in a row for each number of machines to send to
static const u8 sTransceiverBalance[7][3] = { { 1, 0, 0 }, { 1, 1, 0 }, { 1, 1, 1 }, { 1, 1, 2 },
                                              { 1, 2, 2 }, { 2, 2, 2 }, { 2, 2, 3 } };

// Whether to leave out frames with no voice in them
static int sEnableVAD = TRUE;

// Under -ipa file MWCC sorts these by size with a heapsort, which is not stable (docs/matching.md), and this order of
// declarations gives the ROM's layout: sAudioSequence, sBitmap, sChangeVAD, sAudioPacketSize, sAdpcmState, sMarker,
// sEnableEchoCancel, sCodec, sNumStream, sAudioDataSize, sVADStatus and sAudioSession in one section, then sStream,
// sMixBuffer and sSendInfo in sections of their own. The send queue starts on a cache line.
static VCTSendInfo sSendInfo ATTRIBUTE_ALIGN(32);
static s16 sMixBuffer[VCT_FRAME_SAMPLES];
static AudioStream sStream[VCT_SLOT_MAX];
static VCTSession *sAudioSession[VCT_SLOT_MAX];
static u16 sAudioSequence;
static AdpcmState sAdpcmState;
static BOOL sMarker;
// The machines that are heard, one bit for each ID
static u32 sBitmap;
// Set when the frame about to go out is the first of a stream
static BOOL sChangeVAD;
// The size of a packet, header and all
static int sAudioPacketSize;
static int sNumStream;
static u32 sAudioDataSize;
static int sVADStatus;
static VCTCodec sCodec;
static BOOL sEnableEchoCancel;

BOOL vct_init_audio(void) {
    u32 i;

    sAudioSequence = OS_GetTickLo();
    sEnableVAD = TRUE;
    sNumStream = 0;
    sBitmap = 0;
    if (vct.SSPMode == 1) {
        sCodec = VCT_CODEC_8BIT_RAW;
    } else {
        sCodec = VCT_CODEC_4BIT_ADPCM;
    }
    sAudioDataSize = VCT_FRAME_BYTES;
    sAudioPacketSize = sCodecInfo[sCodec][0] * 0x44 + sCodecInfo[sCodec][1] + 0xc;
    sSendInfo.read = 0;
    sSendInfo.write = 0;
    sSendInfo.bitmap = 0;
    sSendInfo.ptr = NULL;
    sSendInfo.scheduleColumn = 0;
    sSendInfo.scheduleRow = 0;
    vct_init_vad();
    for (i = 0; i < VCT_SLOT_MAX; i++) {
        sAudioSession[i] = NULL;
        vct_init_audio_queue(i);
        vct_init_stream(&sStream[i], i, -1);
    }
    vct_init_decoder();
    InitFIRFilter();
    return TRUE;
}

void vct_finish_audio(void) {
    sNumStream = 0;
}

BOOL VCT_StartStreaming(VCTSession *session) {
    u32 slot;

    if (!session) {
        return FALSE;
    }
    if (sNumStream == VCT_SLOT_MAX) {
        return FALSE;
    }
    if (session->state != 2 && session->state != 3) {
        return FALSE;
    }
    for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
        if (sAudioSession[slot] == session) {
            return TRUE;
        }
    }
    for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
        if (sAudioSession[slot] == NULL) {
            sAudioSession[slot] = session;
            break;
        }
    }
    sNumStream++;
    sBitmap |= session->aidBitmap;
    vct_init_stream(&sStream[slot], slot, session->talking);
    vct_init_audio_queue(slot);
    if (session->state != 2) {
        return TRUE;
    }
    if (sNumStream == 1) {
        VCT_ResetVAD();
        sAudioSequence = OS_GetTickLo();
        sAdpcmState.prevSample = 0;
        sAdpcmState.prevIndex = 0;
        sMarker = TRUE;
        sSendInfo.read = 0;
        sSendInfo.write = 0;
        sSendInfo.bitmap = 0;
        sSendInfo.ptr = NULL;
        sSendInfo.scheduleColumn = 0;
        sSendInfo.scheduleRow = 0;
    }
    return TRUE;
}

void VCT_StopStreaming(VCTSession *session) {
    u32 slot;

    for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
        if (sAudioSession[slot] == session) {
            sAudioSession[slot] = NULL;
            sNumStream--;
            vct_reset_stream(&sStream[slot]);
            vct_flush_audio_queue(slot);
            vct_flush_decoder(slot);
            sBitmap &= ~session->aidBitmap;
            break;
        }
    }
    if (sNumStream == 0) {
        sMarker = TRUE;
        VCT_ResetVAD();
        sBitmap = 0;
    }
}

// Hands a frame of the microphone's samples to the library to send, returning whether it took it
BOOL VCT_SendAudio(void *samples, u32 size) {
    AudioHeader *packet;
    u32 i;
    BOOL hasSender = FALSE;

    if (size != sAudioDataSize) {
        return FALSE;
    }
    if (sNumStream == 0) {
        return FALSE;
    }
    for (i = 0; i < VCT_SLOT_MAX; i++) {
        if (sAudioSession[i] != NULL && sAudioSession[i]->state == 2) {
            hasSender = TRUE;
            break;
        }
    }
    if (!hasSender) {
        return FALSE;
    }
    packet = (AudioHeader *)&sSendInfo.buffer[(sSendInfo.write & 1) ? 0 : VCT_SEND_BUFFER_SIZE];
    packet->magic = VCT_PACKET_MAGIC;
    if (sCodec >= VCT_CODEC_2BIT_ADPCM) {
        MIi_CpuCopyFast(samples, &packet->payload[4], size);
    } else {
        MIi_CpuCopyFast(samples, packet->payload, size);
    }
    packet->header = VCT_HEADER_AUDIO;
    packet->sequence = sAudioSequence++;
    packet->timestamp = (u32)(OS_GetTick() / 64);
    sSendInfo.write++;
    return TRUE;
}

// Takes the next frame of a slot's voice from its queue, if its time has come
static BOOL VCTi_ReceiveAudioData(u8 *dst, int size, int slot, u32 *heardMask) {
    int count;
    UDPPacket *packet;
    AudioStream *receiver = &sStream[slot];
    BOOL more = TRUE;
    u64 now;

    count = vct_count_audio_queue(slot);
    while (count != 0) {
        now = OS_GetTick();
        packet = vct_top_audio_queue(slot);

        if (packet != NULL) {
            if (packet->performance_time > now + 0x34) {
                more = FALSE;
                if (packet->marker == 0 && receiver->prevBuffer != NULL) {
                    receiver->playedCount++;
                    MIi_CpuCopyFast(receiver->prevBuffer->samples, dst, size);
                    vct_free_packet_buffer(receiver->prevBuffer);
                    receiver->prevBuffer = NULL;
                    if (heardMask != NULL) {
                        *heardMask |= 1 << packet->remote;
                    }
                    return TRUE;
                }
                return FALSE;
            }
            if (now > packet->performance_time + 0x8b4c) {
                receiver->lastPlayedSequence = packet->sequence;
                count = vct_save_previous_buffer(receiver, packet);
                vct_top_audio_queue(slot);
                more = FALSE;
            } else {
                if (receiver->lastPlayedSequence + 1 != packet->sequence && receiver->lastPlayedSequence != 0 &&
                    receiver->lastPlayedSequence <= packet->sequence) {
                    if (packet->marker == 0) {
                        receiver->lostCount++;
                    }
                }
                receiver->lastPlayedSequence = packet->sequence;
                if (more && receiver->unk70 != 0) {
                    count = vct_save_previous_buffer(receiver, packet);
                    vct_top_audio_queue(slot);
                    receiver->unk70--;
                } else {
                    u32 delay = now - packet->receive_time;
                    int change = delay - receiver->lastDelay;
                    u32 smoothed = delay + ((change - delay) >> 4);

                    receiver->lastDelay = delay;
                    receiver->avgDelayCopy = smoothed;
                    receiver->avgDelay = smoothed;
                    MIi_CpuCopyFast(packet->samples, dst, size);
                    vct_save_previous_buffer(receiver, packet);
                    if (heardMask != NULL) {
                        *heardMask |= 1 << packet->remote;
                    }
                    return TRUE;
                }
            }
        } else {
            receiver->queuedCount = count;
            return FALSE;
        }
    }
    if (receiver->prevBuffer != NULL) {
        vct_free_packet_buffer(receiver->prevBuffer);
        receiver->prevBuffer = NULL;
    }
    return FALSE;
}

// Mixes the voices that are due into a frame of samples, returning whether there is any, and sets the bits of the
// machines heard (libVCT 1.3.1's name for it)
static inline BOOL vct_receive_mixed_audio(void *dst, u32 size, u32 *heardMask) {
    BOOL heard = FALSE;
    u32 slot;

    if (size == sAudioDataSize && sNumStream != 0) {
        for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
            if (VCTi_ReceiveAudioData(dst, size, slot, heardMask) == TRUE) {
                heard = TRUE;
                break;
            }
        }
        if (heard) {
            u32 next = slot + 1;

            if (next == VCT_SLOT_MAX) {
                heard = TRUE;
            } else {
                for (; next < VCT_SLOT_MAX; next++) {
                    if (VCTi_ReceiveAudioData((u8 *)sMixBuffer, size, next, heardMask) == TRUE) {
                        vct_mix_audio(dst, sMixBuffer, dst, size);
                    }
                }
            }
        } else {
            MI_CpuClearFast(dst, size);
        }
        return heard;
    } else {
        MI_CpuClearFast(dst, size);
        return FALSE;
    }
}

// Receives a frame of mixed voices and, with echo cancelling on, gives it to the canceller as the speaker's samples
BOOL VCT_ReceiveAudio(void *dst, u32 size, u32 *heardMask) {
    BOOL heard;

    if (heardMask != NULL) {
        *heardMask = 0;
    }
    heard = vct_receive_mixed_audio(dst, size, heardMask);
    if (sEnableEchoCancel) {
        vct_set_speaker_samples(dst, size);
    }
    return heard;
}

BOOL VCT_SetCodec(VCTCodec codec) {
    if (codec >= VCT_CODEC_END) {
        return FALSE;
    }
    if (vct.SSPMode != 1 && (codec == VCT_CODEC_8BIT_RAW || codec == VCT_CODEC_G711_ULAW)) {
        return FALSE;
    }
    sAudioPacketSize = sCodecInfo[codec][0] * 0x44 + sCodecInfo[codec][1] + 0xc;
    sCodec = codec;
    sAdpcmState.prevSample = 0;
    sAdpcmState.prevIndex = 0;
    return TRUE;
}

void VCT_EnableVAD(BOOL enabled) {
    sEnableVAD = enabled;
    VCT_ResetVAD();
    if (!enabled) {
        sChangeVAD = TRUE;
    }
}

void VCT_EnableEchoCancel(BOOL enabled) {
    sEnableEchoCancel = enabled;
}

// Codes the frame waiting in the send queue, returning the packet or NULL if there is none or it is silent
static AudioHeader *vct_prepare_send_buffer(void) {
    u8 *samples;
    AudioHeader *packet;
    u32 energy;
    u32 rms;
    u32 queued = sSendInfo.write - sSendInfo.read;
    VCTCodec codec = sCodec;

    if (queued > 1) {
        sSendInfo.read = sSendInfo.write - 1;
        queued = 1;
    }
    if (queued == 0) {
        return NULL;
    }
    packet = (AudioHeader *)&sSendInfo.buffer[(sSendInfo.read & 1) ? 0 : VCT_SEND_BUFFER_SIZE];
    samples = packet->payload;
    if (codec >= VCT_CODEC_2BIT_ADPCM) {
        samples += 4;
    }
    energy = HPFFilter((s16 *)samples, sAudioDataSize);
    REG_SQRTCNT = 0;
    REG_SQRT_PARAM_LO = energy;
    while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
    }
    rms = REG_SQRT_RESULT;
    if (sEnableEchoCancel) {
        vct_process_fir((s16 *)samples, sAudioDataSize, rms);
    }
    packet->codec = codec;
    if (sEnableVAD) {
        sVADStatus = VCTi_GetVADStatus(energy, rms, NULL);
        switch (sVADStatus) {
        case 0:
            sSendInfo.read++;
            return NULL;
        case 1:
            packet->codec |= 0x80;
            break;
        case 3:
            packet->header = VCT_HEADER_AUDIO_END;
            break;
        }
    }
    if (!sEnableVAD) {
        if (sChangeVAD) {
            packet->codec |= 0x80;
            sChangeVAD = FALSE;
        }
    }
    if (sMarker) {
        packet->codec |= 0x80;
        sMarker = FALSE;
    }
    if (codec >= VCT_CODEC_2BIT_ADPCM) {
        if (packet->codec & 0x80) {
            sAdpcmState.prevSample = 0;
            sAdpcmState.prevIndex = 0;
        }
        vct_encode_adpcm(samples - 4, (s16 *)samples, sAudioDataSize / 2, &sAdpcmState, codec);
    } else if (codec == VCT_CODEC_G711_ULAW) {
        vct_encode_g711_ulaw(samples, (s16 *)samples, sAudioDataSize / 2);
    } else if (codec == VCT_CODEC_8BIT_RAW) {
        vct_encode_8bit_raw(samples, (s16 *)samples, sAudioDataSize / 2);
    }
    sSendInfo.bitmap = sBitmap;
    sSendInfo.bitmap &= ~(1 << vct.myAID);
    sSendInfo.scheduleRow = MATH_CountPopulation(sSendInfo.bitmap) - 1;
    if (sSendInfo.scheduleRow > 7) {
        return NULL;
    }
    sSendInfo.scheduleColumn = 0;
    return packet;
}

// Takes in a voice packet of the machine with the given ID, returning whether it has been queued
BOOL vct_handle_audio(int aid, AudioHeader *packet, int size, u64 now) {
    UDPPacket *queued;
    AudioStream *receiver = NULL;
    u32 i;
    int codec;
    u8 *data;

    if (sNumStream == 0) {
        return FALSE;
    }
    for (i = 0; i < VCT_SLOT_MAX; i++) {
        if (aid == sStream[i].aid) {
            receiver = &sStream[i];
            break;
        }
    }
    if (receiver == NULL) {
        return FALSE;
    }
    if (packet->header == VCT_HEADER_AUDIO_END) {
        receiver->syncSequence = packet->sequence;
        receiver->firstTimestamp = 0;
        receiver->warmup = 1;
        return FALSE;
    }
    if (packet->header != VCT_HEADER_AUDIO) {
        return FALSE;
    }
    codec = packet->codec & 0x7f;
    receiver->codec = codec;
    if (codec >= VCT_CODEC_END) {
        return FALSE;
    }
    if (size != sCodecInfo[codec][0] * 0x44 + sCodecInfo[codec][1] + 0xc) {
        return FALSE;
    }
    if (!(sBitmap & (aid != 0 ? 1 << aid : 1))) {
        return FALSE;
    }
    queued = vct_alloc_packet_buffer();
    if (queued == NULL) {
        vct_flush_audio_queue(receiver->channel);
        vct_flush_decoder(receiver->channel);
        queued = vct_alloc_packet_buffer();
        if (queued == NULL) {
            vct_flush_all_audio_queue();
            vct_flush_all_decoder();
            queued = vct_alloc_packet_buffer();
        }
        if (queued == NULL) {
            return FALSE;
        }
    }
    data = (u8 *)&queued->buffer[VCT_FRAME_SAMPLES] - (size - 0xc);
    MI_CpuCopy8(packet->payload, data, size - 0xc);
    queued->channel = receiver->channel;
    queued->codec = codec;
    queued->samples = data;
    queued->length = size - 0xc;
    queued->receive_time = now;
    queued->remote = aid;
    queued->marker = packet->codec & 0x80;
    queued->timestamp = packet->timestamp;
    queued->sequence = packet->sequence;
    if (!vct_calc_jitter_and_skew(receiver, queued)) {
        vct_free_packet_buffer(queued);
        return FALSE;
    }
    vct_insert_decoder(queued);
    return TRUE;
}

// Decodes every packet that has come in and puts it in its slot's queue, returning how many there were
int vct_decode_audio_buffer(void) {
    UDPPacket *first = vct_top_decoder();
    int i;
    int count;
    u32 slot;
    OSIntrMode irq;

    if (first == NULL) {
        return 0;
    }
    irq = OS_DisableInterrupts();
    slot = first->channel;
    count = vct_count_decoder_queue(slot);
    for (i = 0; i < count; i++) {
        UDPPacket *packet = vct_get_packet_from_decoder(slot);

        if (packet->codec >= VCT_CODEC_2BIT_ADPCM) {
            vct_decode_adpcm(packet->samples, packet->buffer, packet->length, packet->codec);
        } else if (packet->codec == VCT_CODEC_G711_ULAW) {
            vct_decode_g711_ulaw(packet->samples, packet->buffer, packet->length);
        } else {
            vct_decode_8bit_raw((s8 *)packet->samples, packet->buffer, packet->length);
        }
        packet->samples = (u8 *)packet->buffer;
        vct_remove_packet_from_decoder(packet);
        if (vct_insert_audio_queue(packet, packet->channel) < 0) {
            vct_free_packet_buffer(packet);
        }
    }
    OS_RestoreInterrupts(irq);
    return count;
}

// Sends the frame that has been coded to the machines it is due to, as many as this tick allows
BOOL vct_flush_send_buffer(void) {
    int i;
    int sends;

    if (sNumStream == 0) {
        return TRUE;
    }
    if (!sSendInfo.ptr) {
        sSendInfo.ptr = vct_prepare_send_buffer();
        if (sSendInfo.ptr && sEnableEchoCancel) {
            return FALSE;
        }
        return TRUE;
    }
    sends = 1;
    if (vct.SSPMode == 2) {
        sends = sTransceiverBalance[sSendInfo.scheduleRow][sSendInfo.scheduleColumn++];
    }
    for (i = 0; i < sends; i++) {
        int leading = MATH_CountLeadingZeros(sSendInfo.bitmap);

        if (leading == 32) {
            break;
        }
        sSendInfo.bitmap &= ~(0x80000000 >> leading);
        DWC_SendUnreliable(31 - leading, sSendInfo.ptr, sAudioPacketSize);
        if (sSendInfo.bitmap == 0) {
            sSendInfo.read++;
            sSendInfo.ptr = NULL;
            sSendInfo.bitmap = 0;
            break;
        }
    }
    return TRUE;
}
