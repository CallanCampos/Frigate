#ifndef POKEBW2_DWC_VCT_INTERNAL_H
#define POKEBW2_DWC_VCT_INTERNAL_H

#include "types.h"
#include "dwc/vct.h"

// The voice chat library libVCT (the ROM carries its version string, "[SDK+Nintendo:libVCT 1.4.1]"), which sends the
// microphone's sound to the machines of a Wi-Fi match and plays what they send. Overlay 29 holds it. The ROM names none
// of its functions, so these names say what the code does.

// libVCT's signature, "[SDK+Nintendo:libVCT 1.4.1]", in the main module, which VCT_Init registers
extern const char VCTi_Signature[];

// Sound is cut into frames of 0x440 bytes of 16-bit samples (544 samples) at 8 kHz, and each frame goes out as one
// packet. A packet starts with a 12-byte header, then holds the frame coded in one of five ways (see VCTCodec).
#define VCT_FRAME_SAMPLES 544
#define VCT_FRAME_BYTES (VCT_FRAME_SAMPLES * 2)
// The most machines whose voices are mixed
#define VCT_SLOT_MAX 3
// The most machines of a match
#define VCT_AID_MAX 32

// The magic number that starts every packet, "TCV_" in memory
#define VCT_PACKET_MAGIC 0x5f564354

// The kind of a packet
#define VCT_TYPE_CONTROL 0x00
#define VCT_TYPE_VOICE 0x40
#define VCT_TYPE_SYNC 0x41
#define VCT_TYPE_BROADCAST 0xff

// What the library keeps of its configuration
struct VCTSettings {
    u8 aid;
    u8 hostAid;
    u8 pad2[2];
    VCTCallback callback;
    void *userData;
    int unkC;
    int mode;
    int unk14;
};

extern VCTSettings gVCTSettings;

// Send a packet to the machine with the given ID, from DWC in overlay 11 (neither has a header yet): the first for
// voice, the second for control packets
BOOL func_ov011_02168b18(u8 aid, const void *data, int size);
BOOL func_ov011_02168a08(u8 aid, const void *data, int size);

// The hardware's divider and square root unit
#define REG_DIVCNT (*(vu16 *)0x04000280)
#define REG_DIV_NUMER (*(volatile u64 *)0x04000290)
#define REG_DIV_NUMER_LO (*(vu32 *)0x04000290)
#define REG_DIV_NUMER_HI (*(vu32 *)0x04000294)
#define REG_DIV_DENOM_LO (*(vu32 *)0x04000298)
#define REG_DIV_DENOM_HI (*(vu32 *)0x0400029c)
#define REG_DIV_RESULT_LO (*(vu32 *)0x040002a0)
#define REG_SQRTCNT (*(vu16 *)0x040002b0)
#define REG_SQRT_RESULT (*(vu32 *)0x040002b4)
#define REG_SQRT_PARAM_LO (*(vu32 *)0x040002b8)
#define REG_DIVCNT_BUSY 0x8000
#define REG_SQRTCNT_BUSY 0x8000

#endif // POKEBW2_DWC_VCT_INTERNAL_H
