#include "nitro/mi.h"
#include "nitro/os.h"
#include "vct_buffer_internal.h"
#include "vct_dsp_internal.h"
#include "vct_internal.h"

// The packets that have come in, in the order they came, each machine's packets in the order of their sequence numbers,
// and when each is to be played, with the mu-law and 8-bit codecs. The ROM names neither this file nor vct_audio.c;
// vct_buffer.c is a guess from what it holds. It is a file of its own, built without -ipa file: its functions reach
// each static from a literal of its own, and its .bss is laid out in reverse, where vct_audio.c's functions reach
// theirs from one base (-ipa file). Its .text, .data and .bss each follow vct_audio.c's.

// The packets in the order they came
struct VCTList {
    VCTPacket *head;
    VCTPacket *tail;
};

static int PopQueue(int slot);
static void ResetSequence(VCTReceiver *receiver, u16 sequence);
static BOOL UnwrapSequence(VCTReceiver *receiver, VCTPacket *packet, u16 sequence);

// The ends of the segments of the mu-law curve
static int sSegmentEnds[8] = { 0x3f, 0x7f, 0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff };

// MWCC lays these out in reverse, so they are declared from the last to the first
// Each machine's packets in the order of their sequence numbers
static VCTPacket *sQueueTails[VCT_SLOT_MAX];
static int sQueueCounts[VCT_SLOT_MAX];
static VCTPacket *sQueueHeads[VCT_SLOT_MAX];
// The packets in the order they came, and how many of each slot's
static int sActiveCounts[VCT_SLOT_MAX];
static VCTList sActiveList;

void VCT_ResetActiveList(void) {
    u32 irq = OS_DisableInterrupts();
    int i;

    sActiveList.tail = NULL;
    sActiveList.head = NULL;
    for (i = 0; i < VCT_SLOT_MAX; i++) {
        sActiveCounts[i] = 0;
    }
    OS_RestoreInterrupts(irq);
}

// Frees the packets in the order of arrival that belong to a slot
void VCT_FreeActiveOfSlot(int slot) {
    u32 irq = OS_DisableInterrupts();
    VCTPacket *packet;

    for (packet = VCT_FindActiveBySlot(slot); packet != NULL; packet = VCT_FindActiveBySlot(slot)) {
        VCT_RemoveActive(packet);
        VCT_FreePacket(packet);
    }
    sActiveCounts[slot] = 0;
    OS_RestoreInterrupts(irq);
}

void VCT_FreeAllActive(void) {
    u32 irq = OS_DisableInterrupts();
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
    OS_RestoreInterrupts(irq);
}

int VCT_AppendActive(VCTPacket *packet) {
    u32 irq = OS_DisableInterrupts();

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
    OS_RestoreInterrupts(irq);
    return sActiveCounts[packet->slot];
}

VCTPacket *VCT_GetActiveHead(void) {
    return sActiveList.head;
}

int VCT_GetActiveCount(int slot) {
    return sActiveCounts[slot];
}

VCTPacket *VCT_FindActiveBySlot(int slot) {
    VCTPacket *packet = sActiveList.head;
    u32 irq = OS_DisableInterrupts();

    for (; packet != NULL; packet = packet->next) {
        if (packet->slot == slot) {
            OS_RestoreInterrupts(irq);
            return packet;
        }
    }
    OS_RestoreInterrupts(irq);
    return NULL;
}

int VCT_RemoveActive(VCTPacket *packet) {
    u32 irq = OS_DisableInterrupts();

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
    OS_RestoreInterrupts(irq);
    return sActiveCounts[packet->slot];
}

// Finds the segment of a mu-law magnitude, as search() does in the public-domain G.711 code whose table and constants
// these are
static inline void SearchSegment(int value, const int *table, int size, int *segment) {
    *segment = 0;
    while (*segment < size) {
        if (value <= *table++) {
            return;
        }
        (*segment)++;
    }
    *segment = size;
}

// Codes samples as 8-bit mu-law
void VCT_EncodeMulaw(u8 *dst, s16 *src, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        int segment;
        int sign;
        int sample = src[i] >> 2;

        if (sample < 0) {
            sign = 0x7f;
            sample = -sample;
        } else {
            sign = 0xff;
        }
        if (sample > 0x1fdf) {
            sample = 0x1fdf;
        }
        sample += 0x21;
        SearchSegment(sample, sSegmentEnds, 8, &segment);
        if (segment >= 8) {
            dst[i] = sign ^ 0x7f;
        } else {
            int code = (segment << 4) | ((sample >> (segment + 1)) & 0xf);

            code ^= sign;
            dst[i] = code;
        }
    }
}

void VCT_DecodeMulaw(u8 *src, s16 *dst, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        int v = ~src[i];
        int t = (((v & 0xf) << 3) + 0x84) << ((v & 0x70) >> 4);

        dst[i] = (v & 0x80) ? 0x84 - t : t - 0x84;
    }
}

// Keeps the top eight bits of the samples
void VCT_EncodePcm8(u8 *dst, s16 *src, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        dst[i] = src[i] >> 8;
    }
}

void VCT_DecodePcm8(s8 *src, s16 *dst, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        dst[i] = src[i] << 8;
    }
}

void VCT_ClearSlotQueues(int slot) {
    u32 irq = OS_DisableInterrupts();

    sQueueTails[slot] = NULL;
    sQueueHeads[slot] = NULL;
    sQueueCounts[slot] = 0;
    OS_RestoreInterrupts(irq);
}

void VCT_FlushSlotQueue(int slot) {
    u32 irq = OS_DisableInterrupts();
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
    OS_RestoreInterrupts(irq);
}

void VCT_FlushAllQueues(void) {
    u32 irq = OS_DisableInterrupts();
    int slot;

    for (slot = 0; slot < VCT_SLOT_MAX; slot++) {
        while (sQueueHeads[slot] != NULL) {
            VCTPacket *packet = sQueueHeads[slot];
            VCTPacket *next = packet->next;

            sQueueHeads[slot] = next;
            if (next != NULL) {
                next->prev = NULL;
            }
            VCT_FreePacket(packet);
        }
        sQueueTails[slot] = NULL;
        sQueueCounts[slot] = 0;
    }
    OS_RestoreInterrupts(irq);
}

// Puts a packet in a slot's queue before the packets with higher sequence numbers
int VCT_InsertSorted(VCTPacket *packet, u32 slot) {
    u32 irq;
    VCTPacket *at;

    if (packet == NULL) {
        return -1;
    }
    if (slot >= VCT_SLOT_MAX) {
        return -1;
    }
    irq = OS_DisableInterrupts();
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
    OS_RestoreInterrupts(irq);
    return sQueueCounts[slot];
}

VCTPacket *VCT_PeekQueue(int slot) {
    u32 irq = OS_DisableInterrupts();

    if (sQueueHeads[slot] != NULL) {
        OS_RestoreInterrupts(irq);
        return sQueueHeads[slot];
    }
    OS_RestoreInterrupts(irq);
    return NULL;
}

static int PopQueue(int slot) {
    u32 irq = OS_DisableInterrupts();

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
    OS_RestoreInterrupts(irq);
    return sQueueCounts[slot];
}

int VCT_GetQueueCount(int slot) {
    return sQueueCounts[slot];
}

void VCT_InitReceiver(VCTReceiver *receiver, int slot, int aid) {
    MI_CpuFill8(receiver, 0, 0x28);
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
void VCT_DropHeld(VCTReceiver *receiver) {
    if (receiver->held != NULL) {
        VCT_FreePacket(receiver->held);
        receiver->held = NULL;
    }
    receiver->aid = -1;
}

// Holds a packet back in place of the last, taking it off the front of its queue
int VCT_SetHeld(VCTReceiver *receiver, VCTPacket *packet) {
    if (receiver->held != NULL) {
        VCT_FreePacket(receiver->held);
    }
    receiver->held = packet;
    return PopQueue(receiver->slot);
}

static void ResetSequence(VCTReceiver *receiver, u16 sequence) {
    receiver->sequence16 = sequence;
    receiver->expectedSequence = 0x10001;
    receiver->wrap = 0;
    receiver->needsReset = FALSE;
}

// Turns the 16-bit sequence number of a packet into one that goes on past 65535, returning FALSE for one so far off
// that the machine has probably started over
static BOOL UnwrapSequence(VCTReceiver *receiver, VCTPacket *packet, u16 sequence) {
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
BOOL VCT_UpdateTiming(VCTReceiver *receiver, VCTPacket *packet) {
    u64 tickDelta;
    int skew;
    int trend = 0;
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
    skew = tickDelta - (packet->arrival - receiver->firstArrival);
    if (receiver->lastSkew == 0 || packet->silence != 0) {
        receiver->lastSkew = skew;
    } else {
        int change = skew - receiver->lastSkew;

        if (change < 0) {
            change = -change;
        }
        receiver->lastSkew = skew;
        if (change > (receiver->jitterAverage + 0x8b18) * 2 + OS_MilliSecondsToTicks(100)) {
            receiver->burstCount = change / 0x8b18 + 2;
        }
        if (receiver->burstCount <= 0) {
            receiver->jitterAverage += (change - receiver->jitterAverage) >> 4;
            receiver->unk18 = receiver->jitterAverage;
        }
    }
    packet->playTime = tickDelta + receiver->firstArrival + receiver->playoutDelay;
    now = OS_GetTick();
    if (packet->playTime + 0x8b18 < now) {
        return FALSE;
    }
    if (packet->playTime > now + 0x7fd88) {
        return FALSE;
    }
    if (receiver->burstCount <= 0) {
        int scaled = receiver->jitterAverage * 3;
        u32 quotient = scaled / 0x8b18;

        if (scaled > receiver->delayCeiling) {
            receiver->delayCeiling = (quotient + 1) * 0x8b18 + 0x134df;
            receiver->delayFloor = receiver->delayCeiling - 0xd0a4;
            receiver->playoutDelay = quotient * 0x8b18 + 0x134df;
            receiver->belowCount = 0;
        } else if (scaled < receiver->delayFloor) {
            receiver->belowCount++;
            if (receiver->belowCount > 0x46) {
                receiver->delayCeiling = (quotient + 1) * 0x8b18 + 0x134df;
                receiver->delayFloor = (int)quotient * 0x8b18 - 0x458c;
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
