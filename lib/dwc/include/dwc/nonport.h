#ifndef POKEBW2_DWC_NONPORT_H
#define POKEBW2_DWC_NONPORT_H

#include "types.h"

// GameSpy's own integer and boolean types, which its code (in overlays 11, 189 and 260) declares its values with
// instead of NitroSDK's: its 32-bit types are int and unsigned int, where the SDK's u32 and s32 are long ones. The
// types are those that GameSpy's nonport.h gives the DS (_NITRO), as RushRE's Sonic Rush Adventure decompilation has
// it (lib/include/dwc/gs/nonport.h); pret/pokerevo's Wii build has the header too (include/SDK/DWC/gamespy/nonport.h)

typedef char gsi_i8;
typedef unsigned char gsi_u8;
typedef short gsi_i16;
typedef unsigned short gsi_u16;
typedef int gsi_i32;
typedef unsigned int gsi_u32;
typedef s64 gsi_i64;
typedef u64 gsi_u64;

typedef int gsi_bool;
#define gsi_false ((gsi_bool)0)
#define gsi_true ((gsi_bool)1)

typedef unsigned int gsi_time;

#define gsi_char char

#endif // POKEBW2_DWC_NONPORT_H
