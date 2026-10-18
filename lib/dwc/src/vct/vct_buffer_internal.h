#ifndef POKEBW2_DWC_VCT_BUFFER_INTERNAL_H
#define POKEBW2_DWC_VCT_BUFFER_INTERNAL_H

#include "types.h"
#include "struct_decls.h"

// What one machine's voice has gone through on the way to being played
struct VCTReceiver {
    u32 avgDelay;
    u32 playoutDelay;
    int queuedCount;
    int skewDifference;
    u32 lastSequenceSeen;
    int lostCount;
    int unk18;
    int lateCount;
    int playedCount;
    int codec;
    u32 lastDelay;
    u32 avgDelayCopy;
    u32 syncSequence;
    int slot;
    int aid;
    VCTPacket *held;
    int unk40;
    u64 firstArrival;
    u32 firstTimestamp;
    int unk50;
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
void VCT_ResetActiveList(void);
void VCT_FreeActiveOfSlot(int slot);
void VCT_FreeAllActive(void);
int VCT_AppendActive(VCTPacket *packet);
VCTPacket *VCT_GetActiveHead(void);
int VCT_GetActiveCount(int slot);
VCTPacket *VCT_FindActiveBySlot(int slot);
int VCT_RemoveActive(VCTPacket *packet);
void VCT_EncodeMulaw(u8 *dst, s16 *src, u32 count);
void VCT_DecodeMulaw(u8 *src, s16 *dst, u32 count);
void VCT_EncodePcm8(u8 *dst, s16 *src, u32 count);
void VCT_DecodePcm8(s8 *src, s16 *dst, u32 count);
void VCT_ClearSlotQueues(int slot);
void VCT_FlushSlotQueue(int slot);
void VCT_FlushAllQueues(void);
int VCT_InsertSorted(VCTPacket *packet, u32 slot);
VCTPacket *VCT_PeekQueue(int slot);
int VCT_GetQueueCount(int slot);
void VCT_InitReceiver(VCTReceiver *receiver, int slot, int aid);
void VCT_DropHeld(VCTReceiver *receiver);
int VCT_SetHeld(VCTReceiver *receiver, VCTPacket *packet);
BOOL VCT_UpdateTiming(VCTReceiver *receiver, VCTPacket *packet);

#endif // POKEBW2_DWC_VCT_BUFFER_INTERNAL_H
