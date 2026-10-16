#ifndef POKEBW2_NITRO_SVC_H
#define POKEBW2_NITRO_SVC_H

#include "types.h"

// The BIOS calls (NitroSDK's SVC_* functions, its libsyscall), Thumb stubs at the start of ARM9 main that each run one
// SWI. They keep the names swan gives them

// SVC_CpuSet: copies or fills the words or halfwords the mode gives (the count in its low bits, the flags below), from
// src to dest. A fill reads src once
#define SVC_CPUSET_FILL 0x01000000
#define SVC_CPUSET_32BIT 0x04000000
void CpuSet(const void *src, void *dest, u32 mode);
// SVC_GetCRC16: the BIOS's CRC-16 of the data, from the first value of the sum
u16 GetCRC16(u16 start, const void *data, u32 length);

#endif // POKEBW2_NITRO_SVC_H
