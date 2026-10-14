#include "vct_internal.h"

#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/math.h"

// The sound going out and coming in. A frame of the microphone is filtered, coded and sent to every machine that
// listens; the packets of the other machines wait in a queue each, and are decoded, mixed and played once their time
// comes.

// What the library keeps of the stream going out
typedef struct VCTAudio {
    u16 sequence;
    // The machines that are heard, one bit for each ID
    u32 activeMask;
    // Set when the frame about to go out is the first of a stream
    BOOL needsSync;
    // The size of a packet, header and all
    int packetSize;
    VCTAdpcmState adpcmState;
    BOOL resync;
    BOOL echoCancel;
    VCTCodec codec;
    int sessionCount;
    u32 frameBytes;
    int voiceLevel;
} VCTAudio;

// The two frames going out: one is filled while the other is sent
typedef struct VCTSendQueue {
    VCTVoicePacket frames[2];
    u32 readIndex;
    u32 writeIndex;
    VCTVoicePacket *sending;
    u32 pendingMask;
    u32 scheduleColumn;
    int scheduleRow;
} VCTSendQueue;

typedef struct VCTList {
    VCTPacket *head;
    VCTPacket *tail;
} VCTList;

static void ResetActiveList(void);
static void FreeActiveOfSlot(int slot);
static void FreeAllActive(void);
static int AppendActive(VCTPacket *packet);
static VCTPacket *GetActiveHead(void);
static int GetActiveCount(int slot);
static VCTPacket *FindActiveBySlot(int slot);
static int RemoveActive(VCTPacket *packet);
static void EncodeMulaw(u8 *dst, s16 *src, u32 count);
static void DecodeMulaw(u8 *src, s16 *dst, u32 count);
static void EncodePcm8(u8 *dst, s16 *src, u32 count);
static void DecodePcm8(s8 *src, s16 *dst, u32 count);
static void ClearSlotQueues(int slot);
static void FlushSlotQueue(int slot);
static void FlushAllQueues(void);
static int InsertSorted(VCTPacket *packet, u32 slot);
static VCTPacket *PeekQueue(int slot);
static int PopQueue(int slot);
static int GetQueueCount(int slot);
static void InitReceiver(VCTReceiver *receiver, int slot, int aid);
static void DropHeld(VCTReceiver *receiver);
static int SetHeld(VCTReceiver *receiver, VCTPacket *packet);
static void ResetSequence(VCTReceiver *receiver, u16 sequence);
static BOOL UnwrapSequence(VCTReceiver *receiver, VCTPacket *packet, u16 sequence);
static BOOL UpdateTiming(VCTReceiver *receiver, VCTPacket *packet);
static VCTVoicePacket *EncodeNextFrame(void);
static BOOL ReceiveSlot(u8 *dst, int size, int slot, u32 *heardMask);

// The frame sizes of the codecs in units of 0x44 bytes, and the bytes more
static const u8 sFrameSizes[VCT_CODEC_MAX][2] = {{8, 0}, {8, 0}, {2, 4}, {3, 4}, {4, 4}};

// How many packets to send in each of the ticks that follow a frame, in a row for each number of machines to send to
static const u8 sSendSchedule[22] = {1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 2, 1, 2, 2, 2, 2, 2, 2, 2, 3, 0};

// Whether to leave out frames with no voice in them
static int sVadEnabled = TRUE;

// The ends of the segments of the mu-law curve
static int sSegmentEnds[8] = {0x3f, 0x7f, 0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff};

// MWCC lays these out in reverse, so they are declared from the last to the first
// Each slot's packets in the order of their sequence numbers
static VCTPacket *sQueueTails[VCT_SLOT_MAX];
static int sQueueCounts[VCT_SLOT_MAX];
static VCTPacket *sQueueHeads[VCT_SLOT_MAX];
// The packets in the order they came, and how many of each slot's
static int sActiveCounts[VCT_SLOT_MAX];
static VCTList sActiveList;
static VCTSendQueue sSendQueue;
static s16 sMixBuffer[VCT_FRAME_SAMPLES];
static VCTReceiver sReceivers[VCT_SLOT_MAX];
static VCTSession *sSlotSessions[VCT_SLOT_MAX];
static VCTAudio sAudio;

BOOL VCT_InitAudio(void)
{
    u32 i;
    VCTCodec codec;

    sAudio.sequence = func_0207bb84();
    sVadEnabled = TRUE;
    sAudio.sessionCount = 0;
    sAudio.activeMask = 0;
    codec = gVCTSettings.mode == 1 ? VCT_CODEC_PCM8 : VCT_CODEC_ADPCM4;
    sAudio.codec = codec;
    sAudio.packetSize = (&sFrameSizes[0][0])[codec * 2] * 0x44 + (&sFrameSizes[0][0])[codec * 2 + 1] + 0xc;
    sAudio.frameBytes = VCT_FRAME_BYTES;
    sSendQueue.readIndex = 0;
    sSendQueue.writeIndex = 0;
    sSendQueue.pendingMask = 0;
    sSendQueue.sending = NULL;
    sSendQueue.scheduleColumn = 0;
    sSendQueue.scheduleRow = 0;
    VCT_InitCapture();
    for (i = 0; i < VCT_SLOT_MAX; i++) {
        sSlotSessions[i] = NULL;
        ClearSlotQueues(i);
        InitReceiver(&sReceivers[i], i, -1);
    }
    ResetActiveList();
    VCT_InitEcho();
    return TRUE;
}

void VCT_ClearSessionCount(void)
{
    sAudio.sessionCount = 0;
}

BOOL VCT_AttachSession(VCTSession *session)
{
    int slot;

    if (session == NULL) {
        return FALSE;
    }
    if (sAudio.sessionCount == VCT_SLOT_MAX) {
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
    sAudio.sessionCount++;
    sAudio.activeMask |= session->mask;
    InitReceiver(&sReceivers[slot], slot, session->origin);
    ClearSlotQueues(slot);
    if (session->state != 2) {
        return TRUE;
    }
    if (sAudio.sessionCount == 1) {
        VCT_ResetCapture();
        sAudio.sequence = func_0207bb84();
        sAudio.adpcmState.predictor = 0;
        sAudio.adpcmState.stepIndex = 0;
        sAudio.resync = TRUE;
        sSendQueue.readIndex = 0;
        sSendQueue.writeIndex = 0;
        sSendQueue.pendingMask = 0;
        sSendQueue.sending = NULL;
        sSendQueue.scheduleColumn = 0;
        sSendQueue.scheduleRow = 0;
    }
    return TRUE;
}

void VCT_DetachSession(VCTSession *session)
{
    u32 slot;

    for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
        if (sSlotSessions[slot] == session) {
            sSlotSessions[slot] = NULL;
            sAudio.sessionCount--;
            DropHeld(&sReceivers[slot]);
            FlushSlotQueue(slot);
            FreeActiveOfSlot(slot);
            sAudio.activeMask &= ~session->mask;
            break;
        }
    }
    if (sAudio.sessionCount == 0) {
        sAudio.resync = TRUE;
        VCT_ResetCapture();
        sAudio.activeMask = 0;
    }
}

// Hands a frame of the microphone's samples to the library to send, returning whether it took it
BOOL VCT_SubmitSamples(void *samples, int size)
{
    BOOL hasSender = FALSE;
    int i;
    VCTVoicePacket *packet;

    if (size != sAudio.frameBytes) {
        return FALSE;
    }
    if (sAudio.sessionCount == 0) {
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
    packet = (sSendQueue.writeIndex & 1) ? &sSendQueue.frames[0] : &sSendQueue.frames[1];
    packet->magic = VCT_PACKET_MAGIC;
    if (sAudio.codec >= VCT_CODEC_ADPCM2) {
        sys_memcpy32_fast(samples, &packet->payload[4], size);
    } else {
        sys_memcpy32_fast(samples, packet->payload, size);
    }
    packet->type = VCT_TYPE_VOICE;
    packet->sequence = sAudio.sequence++;
    packet->timestamp = clock() / 64;
    sSendQueue.writeIndex++;
    return TRUE;
}

// Takes the next frame of a slot's voice from its queue, if its time has come
static BOOL ReceiveSlot(u8 *dst, int size, int slot, u32 *heardMask)
{
    VCTReceiver *receiver = &sReceivers[slot];
    BOOL more = TRUE;
    int count = GetQueueCount(slot);

    while (count != 0) {
        u64 now = clock();
        VCTPacket *packet = PeekQueue(slot);

        if (packet == NULL) {
            receiver->queuedCount = count;
            return FALSE;
        }
        if (packet->playTime > now + 0x34) {
            more = FALSE;
            if (packet->silence == 0 && receiver->held != NULL) {
                receiver->playedCount++;
                sys_memcpy32_fast(receiver->held->data, dst, size);
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
            count = SetHeld(receiver, packet);
            PeekQueue(slot);
            more = FALSE;
        } else {
            u32 lastSequence = receiver->lastPlayedSequence;

            if (lastSequence + 1 != packet->sequence && lastSequence != 0 && lastSequence <= packet->sequence) {
                if (packet->silence == 0) {
                    receiver->lostCount++;
                }
            }
            receiver->lastPlayedSequence = packet->sequence;
            if (more && receiver->unk70 != 0) {
                count = SetHeld(receiver, packet);
                PeekQueue(slot);
                receiver->unk70--;
            } else {
                u32 delay = (u32)now - (u32)packet->arrival;
                u32 smoothed = delay + ((u32)(delay - receiver->lastDelay - delay) >> 4);

                receiver->lastDelay = delay;
                receiver->avgDelayCopy = smoothed;
                receiver->avgDelay = smoothed;
                sys_memcpy32_fast(packet->data, dst, size);
                SetHeld(receiver, packet);
                if (heardMask != NULL) {
                    *heardMask |= 1 << packet->aid;
                }
                return TRUE;
            }
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
BOOL VCT_ReadSamples(void *dst, int size, u32 *heardMask)
{
    BOOL heard;
    int slot;

    if (heardMask != NULL) {
        *heardMask = 0;
    }
    heard = FALSE;
    if (size == sAudio.frameBytes && sAudio.sessionCount != 0) {
        for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
            if (ReceiveSlot(dst, size, slot, heardMask) == TRUE) {
                heard = TRUE;
                break;
            }
        }
        if (heard) {
            int next = slot + 1;

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
            sys_memset32_fast(0, dst, size);
        }
    } else {
        heard = FALSE;
        sys_memset32_fast(0, dst, size);
    }
    if (sAudio.echoCancel) {
        VCT_SaveFarEnd(dst, size);
    }
    return heard;
}

BOOL VCT_SetCodec(int codec)
{
    if (codec >= VCT_CODEC_MAX) {
        return FALSE;
    }
    if (gVCTSettings.mode != 1 && (codec == VCT_CODEC_PCM8 || codec == VCT_CODEC_MULAW)) {
        return FALSE;
    }
    sAudio.packetSize = sFrameSizes[codec][0] * 0x44 + sFrameSizes[codec][1] + 0xc;
    sAudio.codec = codec;
    sAudio.adpcmState.predictor = 0;
    sAudio.adpcmState.stepIndex = 0;
    return TRUE;
}

void VCT_SetVad(BOOL enabled)
{
    sVadEnabled = enabled;
    VCT_ResetCapture();
    if (!enabled) {
        sAudio.needsSync = TRUE;
    }
}

void VCT_SetEchoCancel(BOOL enabled)
{
    sAudio.echoCancel = enabled;
}

// Codes the frame waiting in the send queue, returning the packet or NULL if there is none or it is silent
static VCTVoicePacket *EncodeNextFrame(void)
{
    VCTCodec codec = sAudio.codec;
    u32 queued = sSendQueue.writeIndex - sSendQueue.readIndex;
    VCTVoicePacket *packet;
    u8 *samples;
    u32 energy;
    u32 rms;

    if (queued > 1) {
        sSendQueue.readIndex = sSendQueue.writeIndex - 1;
        queued = 1;
    }
    if (queued == 0) {
        return NULL;
    }
    packet = (sSendQueue.readIndex & 1) ? &sSendQueue.frames[0] : &sSendQueue.frames[1];
    samples = packet->payload;
    if (codec >= VCT_CODEC_ADPCM2) {
        samples += 4;
    }
    energy = VCT_FilterFrame((s16 *)samples, sAudio.frameBytes);
    REG_SQRTCNT = 0;
    REG_SQRT_PARAM_LO = energy;
    while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
    }
    rms = REG_SQRT_RESULT;
    if (sAudio.echoCancel) {
        VCT_CancelEcho((s16 *)samples, sAudio.frameBytes, rms);
    }
    packet->codec = codec;
    if (sVadEnabled) {
        sAudio.voiceLevel = VCT_UpdateVad(energy, rms, NULL);
        switch (sAudio.voiceLevel) {
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
        if (sAudio.needsSync) {
            packet->codec |= 0x80;
            sAudio.needsSync = FALSE;
        }
    }
    if (sAudio.resync) {
        packet->codec |= 0x80;
        sAudio.resync = FALSE;
    }
    if (codec >= VCT_CODEC_ADPCM2) {
        if (packet->codec & 0x80) {
            sAudio.adpcmState.predictor = 0;
            sAudio.adpcmState.stepIndex = 0;
        }
        VCT_EncodeAdpcm(samples - 4, (s16 *)samples, sAudio.frameBytes / 2, &sAudio.adpcmState, codec);
    } else if (codec == VCT_CODEC_MULAW) {
        EncodeMulaw(samples, (s16 *)samples, sAudio.frameBytes / 2);
    } else if (codec == VCT_CODEC_PCM8) {
        EncodePcm8(samples, (s16 *)samples, sAudio.frameBytes / 2);
    }
    sSendQueue.pendingMask = sAudio.activeMask;
    sSendQueue.pendingMask &= ~(1 << gVCTSettings.aid);
    sSendQueue.scheduleRow = MATH_CountPopulation(sSendQueue.pendingMask) - 1;
    if (sSendQueue.scheduleRow > 7) {
        return NULL;
    }
    sSendQueue.scheduleColumn = 0;
    return packet;
}

// Takes in a voice packet of the machine with the given ID, returning whether it has been queued
BOOL VCT_HandleVoicePacket(int aid, VCTVoicePacket *packet, int size, u64 now)
{
    VCTReceiver *receiver = NULL;
    VCTPacket *queued;
    int i;
    int codec;
    int dataSize;
    u8 *data;

    if (sAudio.sessionCount == 0) {
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
    if (!(sAudio.activeMask & (aid != 0 ? 1 << aid : 1))) {
        return FALSE;
    }
    queued = VCT_AllocPacket();
    if (queued == NULL) {
        FlushSlotQueue(receiver->slot);
        FreeActiveOfSlot(receiver->slot);
        queued = VCT_AllocPacket();
        if (queued == NULL) {
            FlushAllQueues();
            FreeAllActive();
            queued = VCT_AllocPacket();
            if (queued == NULL) {
                return FALSE;
            }
        }
    }
    dataSize = size - 0xc;
    data = (u8 *)queued->samples + sizeof(queued->samples) - dataSize;
    sys_memcpy(packet->payload, data, dataSize);
    queued->slot = receiver->slot;
    queued->codec = codec;
    queued->data = data;
    queued->dataSize = dataSize;
    queued->arrival = now;
    queued->aid = aid;
    queued->silence = packet->codec & 0x80;
    queued->timestamp = packet->timestamp;
    queued->sequence = packet->sequence;
    if (!UpdateTiming(receiver, queued)) {
        VCT_FreePacket(queued);
        return FALSE;
    }
    AppendActive(queued);
    return TRUE;
}

// Decodes every packet that has come in and puts it in its slot's queue, returning how many there were
int VCT_DecodeReceived(void)
{
    VCTPacket *first = GetActiveHead();
    u32 irq;
    int slot;
    int count;
    int i;

    if (first == NULL) {
        return 0;
    }
    irq = CPU_IRQDisable();
    slot = first->slot;
    count = GetActiveCount(slot);
    for (i = 0; i < count; i++) {
        VCTPacket *packet = FindActiveBySlot(slot);

        if (packet->codec >= VCT_CODEC_ADPCM2) {
            VCT_DecodeAdpcm(packet->data, packet->samples, packet->dataSize, packet->codec);
        } else if (packet->codec == VCT_CODEC_MULAW) {
            DecodeMulaw(packet->data, packet->samples, packet->dataSize);
        } else {
            DecodePcm8((s8 *)packet->data, packet->samples, packet->dataSize);
        }
        packet->data = (u8 *)packet->samples;
        RemoveActive(packet);
        if (InsertSorted(packet, packet->slot) < 0) {
            VCT_FreePacket(packet);
        }
    }
    CPU_SetIRQMask(irq);
    return count;
}

// Sends the frame that has been coded to the machines it is due to, as many as this tick allows
BOOL VCT_SendPending(void)
{
    int sends;
    int i;

    if (sAudio.sessionCount == 0) {
        return TRUE;
    }
    if (sSendQueue.sending == NULL) {
        sSendQueue.sending = EncodeNextFrame();
        return sSendQueue.sending != NULL && sAudio.echoCancel != 0 ? FALSE : TRUE;
    }
    sends = 1;
    if (gVCTSettings.mode == 2) {
        sends = sSendSchedule[sSendQueue.scheduleRow * 3 + sSendQueue.scheduleColumn++];
    }
    for (i = 0; i < sends; i++) {
        int leading = __clz(sSendQueue.pendingMask);

        if (leading == 32) {
            break;
        }
        sSendQueue.pendingMask &= ~(0x80000000 >> leading);
        func_ov011_02168b18(31 - leading, sSendQueue.sending, sAudio.packetSize);
        if (sSendQueue.pendingMask == 0) {
            sSendQueue.readIndex++;
            sSendQueue.sending = NULL;
            sSendQueue.pendingMask = 0;
            break;
        }
    }
    return TRUE;
}

static void ResetActiveList(void)
{
    u32 irq = CPU_IRQDisable();
    int i;

    sActiveList.tail = NULL;
    sActiveList.head = NULL;
    for (i = 0; i < VCT_SLOT_MAX; i++) {
        sActiveCounts[i] = 0;
    }
    CPU_SetIRQMask(irq);
}

// Frees the packets in the order of arrival that belong to a slot
static void FreeActiveOfSlot(int slot)
{
    u32 irq = CPU_IRQDisable();
    VCTPacket *packet;

    for (packet = FindActiveBySlot(slot); packet != NULL; packet = FindActiveBySlot(slot)) {
        RemoveActive(packet);
        VCT_FreePacket(packet);
    }
    sActiveCounts[slot] = 0;
    CPU_SetIRQMask(irq);
}

static void FreeAllActive(void)
{
    u32 irq = CPU_IRQDisable();
    VCTPacket *packet = sActiveList.head;
    int i;

    while (packet != NULL) {
        VCTPacket *current = packet;

        packet = packet->next;
        VCT_FreePacket(current);
    }
    sActiveList.head = NULL;
    sActiveList.tail = NULL;
    for (i = 0; i < VCT_SLOT_MAX; i++) {
        sActiveCounts[i] = 0;
    }
    CPU_SetIRQMask(irq);
}

static int AppendActive(VCTPacket *packet)
{
    u32 irq = CPU_IRQDisable();

    if (sActiveList.head == NULL) {
        sActiveList.head = packet;
        packet->prev = NULL;
        sActiveList.head->next = NULL;
        sActiveList.tail = sActiveList.head;
    } else {
        packet->prev = sActiveList.tail;
        packet->next = NULL;
        sActiveList.tail->next = packet;
        sActiveList.tail = packet;
    }
    sActiveCounts[packet->slot]++;
    CPU_SetIRQMask(irq);
    return sActiveCounts[packet->slot];
}

static VCTPacket *GetActiveHead(void)
{
    return sActiveList.head;
}

static int GetActiveCount(int slot)
{
    return sActiveCounts[slot];
}

static VCTPacket *FindActiveBySlot(int slot)
{
    VCTPacket *packet = sActiveList.head;
    u32 irq = CPU_IRQDisable();

    for (; packet != NULL; packet = packet->next) {
        if (packet->slot == slot) {
            CPU_SetIRQMask(irq);
            return packet;
        }
    }
    CPU_SetIRQMask(irq);
    return NULL;
}

static int RemoveActive(VCTPacket *packet)
{
    u32 irq = CPU_IRQDisable();

    if (packet->prev != NULL) {
        packet->prev->next = packet->next;
    } else {
        sActiveList.head = packet->next;
        if (sActiveList.head != NULL) {
            sActiveList.head->prev = NULL;
        }
    }
    if (packet->next != NULL) {
        packet->next->prev = packet->prev;
    } else {
        sActiveList.tail = packet->prev;
        if (sActiveList.tail != NULL) {
            sActiveList.tail->next = NULL;
        }
    }
    sActiveCounts[packet->slot]--;
    CPU_SetIRQMask(irq);
    return sActiveCounts[packet->slot];
}

// Codes samples as 8-bit mu-law
static void EncodeMulaw(u8 *dst, s16 *src, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++) {
        int sample = src[i] >> 2;
        int sign;
        int segment;

        if (sample < 0) {
            sign = 0x7f;
            sample = -sample;
        } else {
            sign = 0xff;
        }
        if (sample > 0x1fdf) {
            sample = 0x1fdf;
        }
        for (segment = 0; segment < 8; segment++) {
            if (sample + 0x21 <= sSegmentEnds[segment]) {
                break;
            }
        }
        if (segment >= 8) {
            dst[i] = sign ^ 0x7f;
        } else {
            dst[i] = ((segment << 4) | (((sample + 0x21) >> (segment + 1)) & 0xf)) ^ sign;
        }
    }
}

static void DecodeMulaw(u8 *src, s16 *dst, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++) {
        int v = ~src[i];
        int t = (((v & 0xf) << 3) + 0x84) << ((v & 0x70) >> 4);

        dst[i] = (v & 0x80) ? 0x84 - t : t - 0x84;
    }
}

// Keeps the top eight bits of the samples
static void EncodePcm8(u8 *dst, s16 *src, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++) {
        dst[i] = src[i] >> 8;
    }
}

static void DecodePcm8(s8 *src, s16 *dst, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++) {
        dst[i] = src[i] << 8;
    }
}

static void ClearSlotQueues(int slot)
{
    u32 irq = CPU_IRQDisable();

    sQueueTails[slot] = NULL;
    sQueueHeads[slot] = NULL;
    sQueueCounts[slot] = 0;
    CPU_SetIRQMask(irq);
}

static void FlushSlotQueue(int slot)
{
    u32 irq = CPU_IRQDisable();
    VCTPacket *packet = sQueueHeads[slot];

    while (packet != NULL) {
        VCTPacket *next = packet->next;

        sQueueHeads[slot] = next;
        if (next != NULL) {
            next->prev = NULL;
        }
        VCT_FreePacket(packet);
        packet = sQueueHeads[slot];
    }
    sQueueTails[slot] = NULL;
    sQueueCounts[slot] = 0;
    CPU_SetIRQMask(irq);
}

static void FlushAllQueues(void)
{
    u32 irq = CPU_IRQDisable();
    int slot;

    for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
        VCTPacket *packet = sQueueHeads[slot];

        while (packet != NULL) {
            VCTPacket *next = packet->next;

            sQueueHeads[slot] = next;
            if (next != NULL) {
                next->prev = NULL;
            }
            VCT_FreePacket(packet);
            packet = sQueueHeads[slot];
        }
        sQueueTails[slot] = NULL;
        sQueueCounts[slot] = 0;
    }
    CPU_SetIRQMask(irq);
}

// Puts a packet in a slot's queue before the packets with higher sequence numbers
static int InsertSorted(VCTPacket *packet, u32 slot)
{
    u32 irq;
    VCTPacket *at;

    if (packet == NULL) {
        return -1;
    }
    if (slot >= VCT_SLOT_MAX) {
        return -1;
    }
    irq = CPU_IRQDisable();
    if (sQueueHeads[slot] == NULL) {
        sQueueHeads[slot] = packet;
        packet->prev = NULL;
        sQueueHeads[slot]->next = NULL;
        sQueueTails[slot] = sQueueHeads[slot];
    } else {
        for (at = sQueueTails[slot]; at != NULL; at = at->prev) {
            if (at->sequence < packet->sequence) {
                packet->prev = at;
                packet->next = at->next;
                at->next = packet;
                if (packet->next != NULL) {
                    packet->next->prev = packet;
                }
                if (at == sQueueTails[slot]) {
                    sQueueTails[slot] = packet;
                }
                goto inserted;
            }
        }
        packet->prev = NULL;
        packet->next = sQueueHeads[slot];
        sQueueHeads[slot]->prev = packet;
        sQueueHeads[slot] = packet;
    }
inserted:
    sQueueCounts[slot]++;
    CPU_SetIRQMask(irq);
    return sQueueCounts[slot];
}

static VCTPacket *PeekQueue(int slot)
{
    u32 irq = CPU_IRQDisable();

    if (sQueueHeads[slot] != NULL) {
        CPU_SetIRQMask(irq);
        return sQueueHeads[slot];
    }
    CPU_SetIRQMask(irq);
    return NULL;
}

static int PopQueue(int slot)
{
    u32 irq = CPU_IRQDisable();

    if (sQueueHeads[slot] != NULL) {
        VCTPacket *next = sQueueHeads[slot]->next;

        sQueueHeads[slot] = next;
        sQueueCounts[slot]--;
        if (next != NULL) {
            next->prev = NULL;
        } else {
            sQueueTails[slot] = NULL;
        }
    }
    CPU_SetIRQMask(irq);
    return sQueueCounts[slot];
}

static int GetQueueCount(int slot)
{
    return sQueueCounts[slot];
}

static void InitReceiver(VCTReceiver *receiver, int slot, int aid)
{
    sys_memset(receiver, 0, 0x28);
    receiver->playoutDelay = 0x134df;
    receiver->lastDelay = 0;
    receiver->avgDelayCopy = 0;
    receiver->slot = slot;
    receiver->aid = aid;
    receiver->jitterAverage = 0;
    receiver->lastSkew = 0;
    receiver->burstCount = 0;
    receiver->delayFloor = 0;
    receiver->delayCeiling = 0x134df;
    receiver->belowCount = 0;
    receiver->firstTimestamp = 0;
    receiver->firstArrival = 0;
    receiver->warmup = 0;
    receiver->unk70 = 0;
    receiver->unk50 = 0;
    receiver->lastSequence = 0;
    receiver->lastPlayedSequence = 0;
    receiver->needsReset = TRUE;
    receiver->held = NULL;
    receiver->unk40 = 0;
    receiver->syncSequence = 0;
}

// Lets go of the packet held back for the machine, and of the machine
static void DropHeld(VCTReceiver *receiver)
{
    if (receiver->held != NULL) {
        VCT_FreePacket(receiver->held);
        receiver->held = NULL;
    }
    receiver->aid = -1;
}

// Holds a packet back in place of the last, taking it off the front of its queue
static int SetHeld(VCTReceiver *receiver, VCTPacket *packet)
{
    if (receiver->held != NULL) {
        VCT_FreePacket(receiver->held);
    }
    receiver->held = packet;
    return PopQueue(receiver->slot);
}

static void ResetSequence(VCTReceiver *receiver, u16 sequence)
{
    receiver->sequence16 = sequence;
    receiver->expectedSequence = 0x10001;
    receiver->wrap = 0;
    receiver->needsReset = FALSE;
}

// Turns the 16-bit sequence number of a packet into one that goes on past 65535, returning FALSE for one so far off
// that the machine has probably started over
static BOOL UnwrapSequence(VCTReceiver *receiver, VCTPacket *packet, u16 sequence)
{
    u16 distance = sequence - receiver->sequence16;

    if (distance < 3000) {
        if (sequence < receiver->sequence16) {
            receiver->wrap += 0x10000;
        }
        receiver->sequence16 = sequence;
    } else if (distance <= 0xff9c) {
        if (sequence == receiver->expectedSequence) {
            ResetSequence(receiver, sequence);
            receiver->lastSequence = (u16)(sequence - 1);
        } else {
            receiver->expectedSequence = (u16)(sequence + 1);
            return FALSE;
        }
    } else {
        u32 unwrapped = sequence + receiver->wrap;

        if (unwrapped > receiver->lastSequence + 0xff9c) {
            packet->sequence = unwrapped - 0x10000;
            return TRUE;
        }
    }
    packet->sequence = sequence + receiver->wrap;
    return TRUE;
}

// Works out when a packet is to be played from the time it came and the time its machine stamped it with, and keeps
// track of how much the delay between the two wanders so as to widen or narrow the margin the playing is kept behind.
// Returns FALSE for a packet that is a repeat, out of range or too late or early to be of use
static BOOL UpdateTiming(VCTReceiver *receiver, VCTPacket *packet)
{
    u32 tickDelta;
    u32 arrivalDelta;
    int skew;
    int trend = 0;
    u64 playTime;
    u64 now;

    if (receiver->needsReset) {
        ResetSequence(receiver, packet->sequence);
    } else {
        if (!UnwrapSequence(receiver, packet, packet->sequence)) {
            receiver->lostCount++;
            return FALSE;
        }
        receiver->lastSequenceSeen = packet->sequence;
        if (receiver->lastSequence == packet->sequence) {
            receiver->lostCount++;
            return FALSE;
        }
        if (receiver->lastSequence + 1 != packet->sequence && packet->silence == 0) {
            int distance = packet->sequence - receiver->lastSequence;

            if (distance < 0) {
                distance = -distance;
            }
            if (distance > 100) {
                receiver->lastSequence = packet->sequence;
                return FALSE;
            }
            if (receiver->lastSequence > packet->sequence) {
                receiver->lateCount++;
            }
        }
    }
    receiver->lastSequence = packet->sequence;
    if (receiver->firstTimestamp == 0) {
        receiver->firstTimestamp = packet->timestamp;
        receiver->firstArrival = packet->arrival;
        receiver->warmup = 0;
    }
    tickDelta = (packet->timestamp - receiver->firstTimestamp) << 6;
    arrivalDelta = (u32)packet->arrival - (u32)receiver->firstArrival;
    skew = tickDelta - arrivalDelta;
    if (receiver->lastSkew == 0 || packet->silence != 0) {
        receiver->lastSkew = skew;
    } else {
        int change = skew - receiver->lastSkew;

        if (change < 0) {
            change = -change;
        }
        receiver->lastSkew = skew;
        if ((s64)change > (s64)((receiver->jitterAverage + 0x8b18) * 2) + 0xcc8d) {
            receiver->burstCount = change / 0x8b18 + 2;
        }
        if (receiver->burstCount <= 0) {
            receiver->jitterAverage += (change - receiver->jitterAverage) >> 4;
            receiver->unk18 = receiver->jitterAverage;
        }
    }
    playTime = (u64)tickDelta + receiver->firstArrival + receiver->playoutDelay;
    packet->playTime = playTime;
    now = clock();
    if (packet->playTime + 0x8b18 < now) {
        return FALSE;
    }
    if (packet->playTime > now + 0x7fd88) {
        return FALSE;
    }
    if (receiver->burstCount <= 0) {
        int average = receiver->jitterAverage;
        int scaled = average * 3;
        int quotient = scaled / 0x8b18;

        if ((u32)scaled > receiver->delayCeiling) {
            receiver->delayCeiling = (quotient + 1) * 0x8b18 + 0x134df;
            receiver->delayFloor = receiver->delayCeiling - 0xd0a4;
            receiver->playoutDelay = quotient * 0x8b18 + 0x134df;
            receiver->belowCount = 0;
        } else if (scaled < receiver->delayFloor) {
            receiver->belowCount++;
            if (receiver->belowCount > 0x46) {
                receiver->delayCeiling = (quotient + 1) * 0x8b18 + 0x134df;
                receiver->delayFloor = quotient * 0x8b18 - 0x458c;
                if (receiver->delayFloor < 0) {
                    receiver->delayFloor = 0;
                }
                receiver->playoutDelay = quotient * 0x8b18 + 0x134df;
                receiver->belowCount = 0;
            }
        }
        if (receiver->playoutDelay > 0x68520) {
            receiver->playoutDelay = 0x68520;
        }
        if (receiver->warmup < 0x10) {
            receiver->warmup++;
            receiver->skewAverage = skew;
            receiver->skewFirst = skew;
        } else {
            receiver->skewAverage = (receiver->skewAverage * 31 + skew) / 32;
        }
        receiver->skewDifference = receiver->skewFirst - receiver->skewAverage;
        if (receiver->skewDifference > 0x8701) {
            trend = 1;
        }
        if (receiver->skewDifference < -0x8701) {
            trend = -1;
            receiver->unk70++;
        }
        if (trend != 0) {
            receiver->warmup = 0;
            receiver->firstTimestamp = 0;
        }
    }
    receiver->burstCount--;
    return TRUE;
}
