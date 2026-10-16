#ifndef POKEBW2_NITRO_SVC_H
#define POKEBW2_NITRO_SVC_H

#include "types.h"

// The BIOS calls (NitroSDK's SVC_* functions, its libsyscall), Thumb stubs at the start of ARM9 main that each run one
// SWI. swan's names are noted at each

// SVC_WaitByLoop (swan's WaitByLoop): the BIOS's busy wait, for a number of turns of its loop
void SVC_WaitByLoop(u32 cycles);

// SVC_CpuSet: copies or fills the words or halfwords the mode gives (the count in its low bits, the flags below), from
// src to dest. A fill reads src once
#define SVC_CPUSET_FILL 0x01000000
#define SVC_CPUSET_32BIT 0x04000000
void SVC_CpuSet(const void *src, void *dest, u32 mode); // swan's CpuSet
// SVC_GetCRC16: the BIOS's CRC-16 of the data, from the first value of the sum
u16 SVC_GetCRC16(u16 start, const void *data, u32 length); // swan's GetCRC16

#endif // POKEBW2_NITRO_SVC_H
