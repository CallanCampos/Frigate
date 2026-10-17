// NitroSDK's sound driver interface on the host: the decomp's lib/nitro/src/snd/snd_interface.c, compiled unchanged
// through this file (port/hal/snd7/replaced.txt keeps it out of the game's own sources). Its commands carry pointers in
// 32-bit arguments, which hold only the low half of a host pointer, so the functions that send the data the driver
// reads or writes, or a range of it, are renamed out of the way and wrapped: each notes its pointers under the sound's
// tag for the host's ARM7 (port_pxi_note_pointer, snd7.c), then calls the decomp's. They are the sequence and its bank
// (SND_PrepareSeq; the bank with its file's size), the buffer of the driver's state (SND_ReadDriverInfo), a channel's
// wave (SND_SetupChannelPcm) and the ranges of sequence, bank and wave data that go (SND_InvalidateSeqData,
// SND_InvalidateBankData, SND_InvalidateWaveData, whose start's note carries the range's size, so that the driver
// compares host pointers).
#define SND_PrepareSeq SNDi_DsPrepareSeq
#define SND_ReadDriverInfo SNDi_DsReadDriverInfo
#define SND_SetupChannelPcm SNDi_DsSetupChannelPcm
#define SND_InvalidateSeqData SNDi_DsInvalidateSeqData
#define SND_InvalidateBankData SNDi_DsInvalidateBankData
#define SND_InvalidateWaveData SNDi_DsInvalidateWaveData
#include "lib/nitro/src/snd/snd_interface.c"
#undef SND_PrepareSeq
#undef SND_ReadDriverInfo
#undef SND_SetupChannelPcm
#undef SND_InvalidateSeqData
#undef SND_InvalidateBankData
#undef SND_InvalidateWaveData

#include "hal/pxi/pxi_bus.h"
#include "nitro/pxi.h"
#include <stdint.h>
#include <string.h>

// snd.h's prototypes were renamed with the functions
void SND_PrepareSeq(int playerNo, const void *seqBase, u32 seqOffset, const SNDBankData *bank);
void SND_ReadDriverInfo(SNDDriverInfo *info);
void SND_SetupChannelPcm(int chNo, int format, const void *dataAddr, int loop, int loopStart, int loopLen, int volume,
                         int shift, int timer, int pan);
void SND_InvalidateSeqData(const void *start, const void *end);
void SND_InvalidateBankData(const void *start, const void *end);
void SND_InvalidateWaveData(const void *start, const void *end);

// The bank's file: its size is the word at 8 of its head
void SND_PrepareSeq(int playerNo, const void *seqBase, u32 seqOffset, const SNDBankData *bank) {
    u32 bankSize = 0;

    if (bank != NULL) {
        memcpy(&bankSize, (const u8 *)bank + 8, sizeof(bankSize));
    }
    port_pxi_note_pointer(PXI_FIFO_TAG_SOUND, seqBase, 0);
    port_pxi_note_pointer(PXI_FIFO_TAG_SOUND, bank, bankSize);
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

static void NoteRange(const void *start, const void *end) {
    uintptr_t size = (uintptr_t)end > (uintptr_t)start ? (uintptr_t)end - (uintptr_t)start : 0;

    port_pxi_note_pointer(PXI_FIFO_TAG_SOUND, start, (u32)size);
}

void SND_InvalidateSeqData(const void *start, const void *end) {
    NoteRange(start, end);
    SNDi_DsInvalidateSeqData(start, end);
}

void SND_InvalidateBankData(const void *start, const void *end) {
    NoteRange(start, end);
    SNDi_DsInvalidateBankData(start, end);
}

void SND_InvalidateWaveData(const void *start, const void *end) {
    NoteRange(start, end);
    SNDi_DsInvalidateWaveData(start, end);
}
