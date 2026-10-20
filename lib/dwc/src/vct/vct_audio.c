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

// How the samples of a frame are coded
typedef enum VCTCodec {
    VCT_CODEC_PCM8,
    VCT_CODEC_MULAW,
    VCT_CODEC_ADPCM2,
    VCT_CODEC_ADPCM3,
    VCT_CODEC_ADPCM4,
    VCT_CODEC_MAX,
} VCTCodec;

// The bytes of a slot of the send queue: room for an uncoded frame, 0x450 bytes (the 12-byte header, 4 bytes of
// ADPCM state and 0x440 bytes of samples), most likely rounded up to 32 bytes so that both slots of the 32-aligned
// queue start on a cache line
#define VCT_SEND_SLOT_SIZE 0x460

// The two frames going out: one is filled while the other is sent. Each is built as a packet in a slot of the buffer
// and coded in place before it is sent.
struct VCTSendQueue {
    u8 buffer[2 * VCT_SEND_SLOT_SIZE];
    u32 readIndex;
    u32 writeIndex;
    VCTVoicePacket *sending;
    u32 pendingMask;
    u32 scheduleColumn;
    int scheduleRow;
};

static VCTVoicePacket *EncodeNextFrame(void);
static BOOL ReceiveSlot(u8 *dst, int size, int slot, u32 *heardMask);

// The frame sizes of the codecs in units of 0x44 bytes, and the bytes more
static const u8 sFrameSizes[VCT_CODEC_MAX][2] = { { 8, 0 }, { 8, 0 }, { 2, 4 }, { 3, 4 }, { 4, 4 } };

// How many packets to send in each of the ticks that follow a frame, in a row for each number of machines to send to
static const u8 sSendSchedule[7][3] = { { 1, 0, 0 }, { 1, 1, 0 }, { 1, 1, 1 }, { 1, 1, 2 },
                                        { 1, 2, 2 }, { 2, 2, 2 }, { 2, 2, 3 } };

// Whether to leave out frames with no voice in them
static int sVadEnabled = TRUE;

// Under -ipa file MWCC sorts these by size with a heapsort, which is not stable (docs/matching.md), and this order of
// declarations gives the ROM's layout: sSequence, sActiveMask, sNeedsSync, sPacketSize, sAdpcmState, sResync,
// sEchoCancel, sCodec, sSessionCount, sFrameBytes, sVoiceLevel and sSlotSessions in one section, then sReceivers,
// sMixBuffer and sSendQueue in sections of their own. The send queue starts on a cache line.
static VCTSendQueue sSendQueue ATTRIBUTE_ALIGN(32);
static s16 sMixBuffer[VCT_FRAME_SAMPLES];
static VCTReceiver sReceivers[VCT_SLOT_MAX];
static VCTSession *sSlotSessions[VCT_SLOT_MAX];
static u16 sSequence;
static VCTAdpcmState sAdpcmState;
static BOOL sResync;
// The machines that are heard, one bit for each ID
static u32 sActiveMask;
// Set when the frame about to go out is the first of a stream
static BOOL sNeedsSync;
// The size of a packet, header and all
static int sPacketSize;
static int sSessionCount;
static u32 sFrameBytes;
static int sVoiceLevel;
static VCTCodec sCodec;
static BOOL sEchoCancel;

BOOL VCT_InitAudio(void) {
    u32 i;

    sSequence = OS_GetTickLo();
    sVadEnabled = TRUE;
    sSessionCount = 0;
    sActiveMask = 0;
    if (gVCTSettings.mode == 1) {
        sCodec = VCT_CODEC_PCM8;
    } else {
        sCodec = VCT_CODEC_ADPCM4;
    }
    sFrameBytes = VCT_FRAME_BYTES;
    sPacketSize = sFrameSizes[sCodec][0] * 0x44 + sFrameSizes[sCodec][1] + 0xc;
    sSendQueue.readIndex = 0;
    sSendQueue.writeIndex = 0;
    sSendQueue.pendingMask = 0;
    sSendQueue.sending = NULL;
    sSendQueue.scheduleColumn = 0;
    sSendQueue.scheduleRow = 0;
    VCT_InitCapture();
    for (i = 0; i < VCT_SLOT_MAX; i++) {
        sSlotSessions[i] = NULL;
        VCT_ClearSlotQueues(i);
        VCT_InitReceiver(&sReceivers[i], i, -1);
    }
    VCT_ResetActiveList();
    VCT_InitEcho();
    return TRUE;
}

void VCT_ClearSessionCount(void) {
    sSessionCount = 0;
}

BOOL VCT_AttachSession(VCTSession *session) {
    u32 slot;

    if (!session) {
        return FALSE;
    }
    if (sSessionCount == VCT_SLOT_MAX) {
        return FALSE;
    }
    if (session->state != 2 && session->state != 3) {
        return FALSE;
    }
    for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
        if (sSlotSessions[slot] == session) {
            return TRUE;
        }
    }
    for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
        if (sSlotSessions[slot] == NULL) {
            sSlotSessions[slot] = session;
            break;
        }
    }
    sSessionCount++;
    sActiveMask |= session->mask;
    VCT_InitReceiver(&sReceivers[slot], slot, session->origin);
    VCT_ClearSlotQueues(slot);
    if (session->state != 2) {
        return TRUE;
    }
    if (sSessionCount == 1) {
        VCT_ResetCapture();
        sSequence = OS_GetTickLo();
        sAdpcmState.predictor = 0;
        sAdpcmState.stepIndex = 0;
        sResync = TRUE;
        sSendQueue.readIndex = 0;
        sSendQueue.writeIndex = 0;
        sSendQueue.pendingMask = 0;
        sSendQueue.sending = NULL;
        sSendQueue.scheduleColumn = 0;
        sSendQueue.scheduleRow = 0;
    }
    return TRUE;
}

void VCT_DetachSession(VCTSession *session) {
    u32 slot;

    for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
        if (sSlotSessions[slot] == session) {
            sSlotSessions[slot] = NULL;
            sSessionCount--;
            VCT_DropHeld(&sReceivers[slot]);
            VCT_FlushSlotQueue(slot);
            VCT_FreeActiveOfSlot(slot);
            sActiveMask &= ~session->mask;
            break;
        }
    }
    if (sSessionCount == 0) {
        sResync = TRUE;
        VCT_ResetCapture();
        sActiveMask = 0;
    }
}

// Hands a frame of the microphone's samples to the library to send, returning whether it took it
BOOL VCT_SubmitSamples(void *samples, int size) {
    VCTVoicePacket *packet;
    u32 i;
    BOOL hasSender = FALSE;

    if (size != sFrameBytes) {
        return FALSE;
    }
    if (sSessionCount == 0) {
        return FALSE;
    }
    for (i = 0; i < VCT_SLOT_MAX; i++) {
        if (sSlotSessions[i] != NULL && sSlotSessions[i]->state == 2) {
            hasSender = TRUE;
            break;
        }
    }
    if (!hasSender) {
        return FALSE;
    }
    packet = (VCTVoicePacket *)&sSendQueue.buffer[(sSendQueue.writeIndex & 1) ? 0 : VCT_SEND_SLOT_SIZE];
    packet->magic = VCT_PACKET_MAGIC;
    if (sCodec >= VCT_CODEC_ADPCM2) {
        MIi_CpuCopyFast(samples, &packet->payload[4], size);
    } else {
        MIi_CpuCopyFast(samples, packet->payload, size);
    }
    packet->type = VCT_TYPE_VOICE;
    packet->sequence = sSequence++;
    packet->timestamp = (u32)(OS_GetTick() / 64);
    sSendQueue.writeIndex++;
    return TRUE;
}

// Takes the next frame of a slot's voice from its queue, if its time has come
static BOOL ReceiveSlot(u8 *dst, int size, int slot, u32 *heardMask) {
    int count;
    VCTPacket *packet;
    VCTReceiver *receiver = &sReceivers[slot];
    BOOL more = TRUE;
    u64 now;

    count = VCT_GetQueueCount(slot);
    while (count != 0) {
        now = OS_GetTick();
        packet = VCT_PeekQueue(slot);

        if (packet != NULL) {
            if (packet->playTime > now + 0x34) {
                more = FALSE;
                if (packet->silence == 0 && receiver->held != NULL) {
                    receiver->playedCount++;
                    MIi_CpuCopyFast(receiver->held->data, dst, size);
                    VCT_FreePacket(receiver->held);
                    receiver->held = NULL;
                    if (heardMask != NULL) {
                        *heardMask |= 1 << packet->aid;
                    }
                    return TRUE;
                }
                return FALSE;
            }
            if (now > packet->playTime + 0x8b4c) {
                receiver->lastPlayedSequence = packet->sequence;
                count = VCT_SetHeld(receiver, packet);
                VCT_PeekQueue(slot);
                more = FALSE;
            } else {
                if (receiver->lastPlayedSequence + 1 != packet->sequence && receiver->lastPlayedSequence != 0 &&
                    receiver->lastPlayedSequence <= packet->sequence) {
                    if (packet->silence == 0) {
                        receiver->lostCount++;
                    }
                }
                receiver->lastPlayedSequence = packet->sequence;
                if (more && receiver->unk70 != 0) {
                    count = VCT_SetHeld(receiver, packet);
                    VCT_PeekQueue(slot);
                    receiver->unk70--;
                } else {
                    u32 delay = now - packet->arrival;
                    int change = delay - receiver->lastDelay;
                    u32 smoothed = delay + ((change - delay) >> 4);

                    receiver->lastDelay = delay;
                    receiver->avgDelayCopy = smoothed;
                    receiver->avgDelay = smoothed;
                    MIi_CpuCopyFast(packet->data, dst, size);
                    VCT_SetHeld(receiver, packet);
                    if (heardMask != NULL) {
                        *heardMask |= 1 << packet->aid;
                    }
                    return TRUE;
                }
            }
        } else {
            receiver->queuedCount = count;
            return FALSE;
        }
    }
    if (receiver->held != NULL) {
        VCT_FreePacket(receiver->held);
        receiver->held = NULL;
    }
    return FALSE;
}

// Mixes the voices that are due into a frame of samples, returning whether there is any, and sets the bits of the
// machines heard
BOOL VCT_ReadSamples(void *dst, int size, u32 *heardMask) {
    BOOL heard;
    u32 slot;

    if (heardMask != NULL) {
        *heardMask = 0;
    }
    heard = FALSE;
    if (size == sFrameBytes && sSessionCount != 0) {
        for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
            if (ReceiveSlot(dst, size, slot, heardMask) == TRUE) {
                heard = TRUE;
                break;
            }
        }
        if (heard) {
            u32 next = slot + 1;

            if (next == VCT_SLOT_MAX) {
                heard = TRUE;
            } else if (next < VCT_SLOT_MAX) {
                for (; next < VCT_SLOT_MAX; next++) {
                    if (ReceiveSlot((u8 *)sMixBuffer, size, next, heardMask) == TRUE) {
                        VCT_MixSamples(dst, sMixBuffer, dst, size);
                    }
                }
            }
        } else {
            MIi_CpuClearFast(0, dst, size);
        }
    } else {
        heard = FALSE;
        MIi_CpuClearFast(0, dst, size);
    }
    if (sEchoCancel) {
        VCT_SaveFarEnd(dst, size);
    }
    return heard;
}

BOOL VCT_SetCodec(int codec) {
    if (codec >= VCT_CODEC_MAX) {
        return FALSE;
    }
    if (gVCTSettings.mode != 1 && (codec == VCT_CODEC_PCM8 || codec == VCT_CODEC_MULAW)) {
        return FALSE;
    }
    sPacketSize = sFrameSizes[codec][0] * 0x44 + sFrameSizes[codec][1] + 0xc;
    sCodec = codec;
    sAdpcmState.predictor = 0;
    sAdpcmState.stepIndex = 0;
    return TRUE;
}

void VCT_SetVad(BOOL enabled) {
    sVadEnabled = enabled;
    VCT_ResetCapture();
    if (!enabled) {
        sNeedsSync = TRUE;
    }
}

void VCT_SetEchoCancel(BOOL enabled) {
    sEchoCancel = enabled;
}

// Codes the frame waiting in the send queue, returning the packet or NULL if there is none or it is silent
static VCTVoicePacket *EncodeNextFrame(void) {
    u8 *samples;
    VCTVoicePacket *packet;
    u32 energy;
    u32 rms;
    u32 queued = sSendQueue.writeIndex - sSendQueue.readIndex;
    VCTCodec codec = sCodec;

    if (queued > 1) {
        sSendQueue.readIndex = sSendQueue.writeIndex - 1;
        queued = 1;
    }
    if (queued == 0) {
        return NULL;
    }
    packet = (VCTVoicePacket *)&sSendQueue.buffer[(sSendQueue.readIndex & 1) ? 0 : VCT_SEND_SLOT_SIZE];
    samples = packet->payload;
    if (codec >= VCT_CODEC_ADPCM2) {
        samples += 4;
    }
    energy = VCT_FilterFrame((s16 *)samples, sFrameBytes);
    REG_SQRTCNT = 0;
    REG_SQRT_PARAM_LO = energy;
    while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
    }
    rms = REG_SQRT_RESULT;
    if (sEchoCancel) {
        VCT_CancelEcho((s16 *)samples, sFrameBytes, rms);
    }
    packet->codec = codec;
    if (sVadEnabled) {
        sVoiceLevel = VCT_UpdateVad(energy, rms, NULL);
        switch (sVoiceLevel) {
        case 0:
            sSendQueue.readIndex++;
            return NULL;
        case 1:
            packet->codec |= 0x80;
            break;
        case 3:
            packet->type = VCT_TYPE_SYNC;
            break;
        }
    }
    if (!sVadEnabled) {
        if (sNeedsSync) {
            packet->codec |= 0x80;
            sNeedsSync = FALSE;
        }
    }
    if (sResync) {
        packet->codec |= 0x80;
        sResync = FALSE;
    }
    if (codec >= VCT_CODEC_ADPCM2) {
        if (packet->codec & 0x80) {
            sAdpcmState.predictor = 0;
            sAdpcmState.stepIndex = 0;
        }
        VCT_EncodeAdpcm(samples - 4, (s16 *)samples, sFrameBytes / 2, &sAdpcmState, codec);
    } else if (codec == VCT_CODEC_MULAW) {
        VCT_EncodeMulaw(samples, (s16 *)samples, sFrameBytes / 2);
    } else if (codec == VCT_CODEC_PCM8) {
        VCT_EncodePcm8(samples, (s16 *)samples, sFrameBytes / 2);
    }
    sSendQueue.pendingMask = sActiveMask;
    sSendQueue.pendingMask &= ~(1 << gVCTSettings.aid);
    sSendQueue.scheduleRow = MATH_CountPopulation(sSendQueue.pendingMask) - 1;
    if (sSendQueue.scheduleRow > 7) {
        return NULL;
    }
    sSendQueue.scheduleColumn = 0;
    return packet;
}

// Takes in a voice packet of the machine with the given ID, returning whether it has been queued
BOOL VCT_HandleVoicePacket(int aid, VCTVoicePacket *packet, int size, u64 now) {
    VCTPacket *queued;
    VCTReceiver *receiver = NULL;
    u32 i;
    int codec;
    u8 *data;

    if (sSessionCount == 0) {
        return FALSE;
    }
    for (i = 0; i < VCT_SLOT_MAX; i++) {
        if (aid == sReceivers[i].aid) {
            receiver = &sReceivers[i];
            break;
        }
    }
    if (receiver == NULL) {
        return FALSE;
    }
    if (packet->type == VCT_TYPE_SYNC) {
        receiver->syncSequence = packet->sequence;
        receiver->firstTimestamp = 0;
        receiver->warmup = 1;
        return FALSE;
    }
    if (packet->type != VCT_TYPE_VOICE) {
        return FALSE;
    }
    codec = packet->codec & 0x7f;
    receiver->codec = codec;
    if (codec >= VCT_CODEC_MAX) {
        return FALSE;
    }
    if (size != sFrameSizes[codec][0] * 0x44 + sFrameSizes[codec][1] + 0xc) {
        return FALSE;
    }
    if (!(sActiveMask & (aid != 0 ? 1 << aid : 1))) {
        return FALSE;
    }
    queued = VCT_AllocPacket();
    if (queued == NULL) {
        VCT_FlushSlotQueue(receiver->slot);
        VCT_FreeActiveOfSlot(receiver->slot);
        queued = VCT_AllocPacket();
        if (queued == NULL) {
            VCT_FlushAllQueues();
            VCT_FreeAllActive();
            queued = VCT_AllocPacket();
        }
        if (queued == NULL) {
            return FALSE;
        }
    }
    data = (u8 *)&queued->samples[VCT_FRAME_SAMPLES] - (size - 0xc);
    MI_CpuCopy8(packet->payload, data, size - 0xc);
    queued->slot = receiver->slot;
    queued->codec = codec;
    queued->data = data;
    queued->dataSize = size - 0xc;
    queued->arrival = now;
    queued->aid = aid;
    queued->silence = packet->codec & 0x80;
    queued->timestamp = packet->timestamp;
    queued->sequence = packet->sequence;
    if (!VCT_UpdateTiming(receiver, queued)) {
        VCT_FreePacket(queued);
        return FALSE;
    }
    VCT_AppendActive(queued);
    return TRUE;
}

// Decodes every packet that has come in and puts it in its slot's queue, returning how many there were
int VCT_DecodeReceived(void) {
    VCTPacket *first = VCT_GetActiveHead();
    int i;
    int count;
    int slot;
    u32 irq;

    if (first == NULL) {
        return 0;
    }
    irq = OS_DisableInterrupts();
    slot = first->slot;
    count = VCT_GetActiveCount(slot);
    for (i = 0; i < count; i++) {
        VCTPacket *packet = VCT_FindActiveBySlot(slot);

        if (packet->codec >= VCT_CODEC_ADPCM2) {
            VCT_DecodeAdpcm(packet->data, packet->samples, packet->dataSize, packet->codec);
        } else if (packet->codec == VCT_CODEC_MULAW) {
            VCT_DecodeMulaw(packet->data, packet->samples, packet->dataSize);
        } else {
            VCT_DecodePcm8((s8 *)packet->data, packet->samples, packet->dataSize);
        }
        packet->data = (u8 *)packet->samples;
        VCT_RemoveActive(packet);
        if (VCT_InsertSorted(packet, packet->slot) < 0) {
            VCT_FreePacket(packet);
        }
    }
    OS_RestoreInterrupts(irq);
    return count;
}

// Sends the frame that has been coded to the machines it is due to, as many as this tick allows
BOOL VCT_SendPending(void) {
    int i;
    int sends;

    if (sSessionCount == 0) {
        return TRUE;
    }
    if (!sSendQueue.sending) {
        sSendQueue.sending = EncodeNextFrame();
        if (sSendQueue.sending && sEchoCancel) {
            return FALSE;
        }
        return TRUE;
    }
    sends = 1;
    if (gVCTSettings.mode == 2) {
        sends = sSendSchedule[sSendQueue.scheduleRow][sSendQueue.scheduleColumn++];
    }
    for (i = 0; i < sends; i++) {
        int leading = MATH_CountLeadingZeros(sSendQueue.pendingMask);

        if (leading == 32) {
            break;
        }
        sSendQueue.pendingMask &= ~(0x80000000 >> leading);
        DWC_SendUnreliable(31 - leading, sSendQueue.sending, sPacketSize);
        if (sSendQueue.pendingMask == 0) {
            sSendQueue.readIndex++;
            sSendQueue.sending = NULL;
            sSendQueue.pendingMask = 0;
            break;
        }
    }
    return TRUE;
}
