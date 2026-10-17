// NitroSDK's sound driver interface on the host: the decomp's lib/nitro/src/snd/snd_interface.c, compiled unchanged
// through this file (port/hal/snd7/replaced.txt keeps it out of the game's own sources). Its commands carry pointers in
// 32-bit arguments, which hold only the low half of a host pointer, so the functions that send the data the driver
// reads or writes, or a range of it, are renamed out of the way and wrapped: each notes its pointers under the sound's
// tag for the host's ARM7 (port_pxi_note_pointer, snd7.c), then calls the decomp's. They are the sequence and its bank
// (SND_PrepareSeq), the buffer of the driver's state (SND_ReadDriverInfo), a channel's wave (SND_SetupChannelPcm, for
// the real driver; the null driver plays nothing) and the range of sequence data that goes (SND_InvalidateSeqData,
// whose start's note carries the range's size, so that the driver compares host pointers). The null driver ignores
// SND_InvalidateBankData and SND_InvalidateWaveData, which the real driver will need wrapped the same way.
#define SND_PrepareSeq SNDi_DsPrepareSeq
#define SND_ReadDriverInfo SNDi_DsReadDriverInfo
#define SND_SetupChannelPcm SNDi_DsSetupChannelPcm
#define SND_InvalidateSeqData SNDi_DsInvalidateSeqData
#include "lib/nitro/src/snd/snd_interface.c"
#undef SND_PrepareSeq
#undef SND_ReadDriverInfo
#undef SND_SetupChannelPcm
#undef SND_InvalidateSeqData

#include "hal/pxi/pxi_bus.h"
#include "nitro/pxi.h"
#include <stdint.h>

// snd.h's prototypes were renamed with the functions
void SND_PrepareSeq(int playerNo, const void *seqBase, u32 seqOffset, const SNDBankData *bank);
void SND_ReadDriverInfo(SNDDriverInfo *info);
void SND_SetupChannelPcm(int chNo, int format, const void *dataAddr, int loop, int loopStart, int loopLen, int volume,
                         int shift, int timer, int pan);
void SND_InvalidateSeqData(const void *start, const void *end);

void SND_PrepareSeq(int playerNo, const void *seqBase, u32 seqOffset, const SNDBankData *bank) {
    port_pxi_note_pointer(PXI_FIFO_TAG_SOUND, seqBase, 0);
    port_pxi_note_pointer(PXI_FIFO_TAG_SOUND, bank, 0);
    SNDi_DsPrepareSeq(playerNo, seqBase, seqOffset, bank);
}

void SND_ReadDriverInfo(SNDDriverInfo *info) {
    port_pxi_note_pointer(PXI_FIFO_TAG_SOUND, info, sizeof(*info));
    SNDi_DsReadDriverInfo(info);
}

// The wave's words: the part before the loop and the loop
void SND_SetupChannelPcm(int chNo, int format, const void *dataAddr, int loop, int loopStart, int loopLen, int volume,
                         int shift, int timer, int pan) {
    port_pxi_note_pointer(PXI_FIFO_TAG_SOUND, dataAddr, (u32)(loopStart + loopLen) * sizeof(u32));
    SNDi_DsSetupChannelPcm(chNo, format, dataAddr, loop, loopStart, loopLen, volume, shift, timer, pan);
}

void SND_InvalidateSeqData(const void *start, const void *end) {
    uintptr_t size = (uintptr_t)end > (uintptr_t)start ? (uintptr_t)end - (uintptr_t)start : 0;

    port_pxi_note_pointer(PXI_FIFO_TAG_SOUND, start, (u32)size);
    SNDi_DsInvalidateSeqData(start, end);
}
