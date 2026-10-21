#ifndef POKEBW2_NITRO_SVC_H
#define POKEBW2_NITRO_SVC_H

#include "types.h"

// The BIOS calls (NitroSDK's SVC_* functions, its libsyscall), Thumb stubs at the start of ARM9 main that each run one
// SWI (secure_area.c). swan's names are noted at each

// SVC_WaitByLoop (swan's WaitByLoop): the BIOS's busy wait, for a number of turns of its loop
void SVC_WaitByLoop(u32 cycles);

// SVC_CpuSet: copies or fills the words or halfwords the mode gives (the count in its low bits, the flags below), from
// src to dest. A fill reads src once
#define SVC_CPUSET_FILL 0x01000000
#define SVC_CPUSET_32BIT 0x04000000
void SVC_CpuSet(const void *src, void *dest, u32 mode); // swan's CpuSet
// SVC_CpuSetFast: SVC_CpuSet for words, 32 bytes at a time
void SVC_CpuSetFast(const void *src, void *dest, u32 mode); // swan's CpuFastSet
// SVC_GetCRC16: the BIOS's CRC-16 of the data, from the first value of the sum
u16 SVC_GetCRC16(u16 start, const void *data, u32 length); // swan's GetCRC16

// SVC_Div and SVC_DivRem: the quotient and the remainder of a signed division
s32 SVC_Div(s32 number, s32 denom);    // swan's Div
s32 SVC_DivRem(s32 number, s32 denom); // swan's Mod
// SVC_Sqrt: the integer square root
u16 SVC_Sqrt(u32 value); // swan's Sqrt

// SVC_UnpackBits: widens each unit of src's bits to the destination's unit, by the parameters at param
void SVC_UnpackBits(const void *src, void *dest, const void *param); // swan's BitUnPack
// The BIOS's decompressors: LZ77 and run-length data to memory a byte at a time (SVC_UncompressLZ8, swan's
// LZ77UnCompReadNormalWrite8bit, and SVC_UncompressRL8, swan's RLUnCompReadNormalWrite8bit), and LZ77, Huffman and
// run-length data read through the stream callbacks to memory a halfword at a time, which return the size written or a
// negative error (SVC_UncompressLZ16FromDevice, SVC_UncompressHuffmanFromDevice and SVC_UncompressRL16FromDevice,
// swan's LZ77UnCompReadByCallbackWrite16bit, HuffUnCompReadByCallback and RLUnCompReadByCallbackWrite16bit)
void SVC_UncompressLZ8(const void *src, void *dest);
void SVC_UncompressRL8(const void *src, void *dest);
s32 SVC_UncompressLZ16FromDevice(const void *src, void *dest, const void *param, const void *callbacks);
s32 SVC_UncompressHuffmanFromDevice(const void *src, void *dest, u8 *tree, const void *callbacks);
s32 SVC_UncompressRL16FromDevice(const void *src, void *dest, const void *param, const void *callbacks);

// SVC_WaitIntr: halts until one of the interrupts of irqFlags occurs, after clearing those that already have if clear
void SVC_WaitIntr(BOOL clear, u32 irqFlags); // swan's IntrWait
// SVC_WaitVBlankIntr: halts until the next V-blank interrupt
void SVC_WaitVBlankIntr(void); // swan's VBlankIntrWait
// SVC_Halt: halts until an interrupt
void SVC_Halt(void); // swan's Halt
// SVC_SoftReset: restarts the program
void SVC_SoftReset(void); // swan's SoftReset
// SVC_IsMmemExpanded: TRUE when main memory is the debugger's 8 MB
BOOL SVC_IsMmemExpanded(void); // swan's IsDebugger

#endif // POKEBW2_NITRO_SVC_H
