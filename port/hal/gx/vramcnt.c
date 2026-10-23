// The SDK's functions that write VRAMCNT, wrapped so that the host's VRAM views follow each change (ADR 0004). The
// decomp writes VRAMCNT only in gx_vramcnt.c (the GX_SetBankFor*, GX_ResetBankFor* and GX_DisableBankFor* functions,
// through its statics GX_VRAMCNT_SetLCDC_, resetBankForX_ and disableBankForX_, which only those reach) and in
// gx_state.c (GX_InitGXState). port/cmake/gx.cmake compiles those files, and mi_wram.c, with every exported function
// renamed port_sdk_<name>; each function of the original name here calls the SDK's and then applies the registers
// (port/hal/gx/vram.h). That is after the whole call, as the spike recommends: nothing touches VRAM inside these
// functions, and the moment inside a GX_SetBankFor* where the new banks are written before the old ones go back to
// LCDC, which would put two banks on one slot, never shows.
//
// Applying once per GFL_BGSysSetVRAMBanks (its 20 SDK calls) would save remaps at scene changes, but needs a hook in
// gx_layers.c, a game change; per call costs about 50 µs aliased and 150 µs copied at a scene change (the spike's
// measurements).
#include "hal/gx/vram.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include <stdio.h>

// After an SDK call that wrote VRAMCNT. Between two SDK calls no slot should have two banks; if one does (two uses
// given the same slot, which the DS would show as the OR of both banks), say so once
static void after_vramcnt(const char *name) {
    static BOOL sReported;

    port_vram_sync();
    if (port_vram_layout()->overlaps != 0 && !sReported) {
        sReported = TRUE;
        fprintf(stderr, "port: VRAM: after %s two banks share %d slots; the lower bank shows\n", name,
                port_vram_layout()->overlaps);
    }
}

// A GX_SetBankFor<Use>(banks), a GX_ResetBankFor<Use>() that returns the banks it took, a GX_DisableBankFor<Use>(), and
// a GX_GetBankFor<Use>(), which writes nothing and is renamed with the others only because the file is
#define WRAP_SET(name, type)                                                                                           \
    void port_sdk_##name(type banks);                                                                                  \
    void name(type banks) {                                                                                            \
        port_sdk_##name(banks);                                                                                        \
        after_vramcnt(#name);                                                                                          \
    }
#define WRAP_RESET(name, type)                                                                                         \
    type port_sdk_##name(void);                                                                                        \
    type name(void) {                                                                                                  \
        type banks = port_sdk_##name();                                                                                \
        after_vramcnt(#name);                                                                                          \
        return banks;                                                                                                  \
    }
#define WRAP_DISABLE(name)                                                                                             \
    u32 port_sdk_##name(void);                                                                                         \
    u32 name(void) {                                                                                                   \
        u32 banks = port_sdk_##name();                                                                                 \
        after_vramcnt(#name);                                                                                          \
        return banks;                                                                                                  \
    }
#define WRAP_VOID(name)                                                                                                \
    void port_sdk_##name(void);                                                                                        \
    void name(void) {                                                                                                  \
        port_sdk_##name();                                                                                             \
        after_vramcnt(#name);                                                                                          \
    }
#define WRAP_GET(name, type)                                                                                           \
    type port_sdk_##name(void);                                                                                        \
    type name(void) {                                                                                                  \
        return port_sdk_##name();                                                                                      \
    }

WRAP_SET(GX_SetBankForBG, GXVRamBG)
WRAP_SET(GX_SetBankForOBJ, GXVRamOBJ)
WRAP_SET(GX_SetBankForBGExtPltt, GXVRamBGExtPltt)
WRAP_SET(GX_SetBankForOBJExtPltt, GXVRamOBJExtPltt)
WRAP_SET(GX_SetBankForTex, GXVRamTex)
WRAP_SET(GX_SetBankForTexPltt, GXVRamTexPltt)
WRAP_SET(GX_SetBankForClearImage, GXVRamClearImage)
WRAP_SET(GX_SetBankForARM7, GXVRamARM7)
WRAP_SET(GX_SetBankForLCDC, int)
WRAP_SET(GX_SetBankForSubBG, GXVRamSubBG)
WRAP_SET(GX_SetBankForSubOBJ, GXVRamSubOBJ)
WRAP_SET(GX_SetBankForSubBGExtPltt, GXVRamSubBGExtPltt)
WRAP_SET(GX_SetBankForSubOBJExtPltt, GXVRamSubOBJExtPltt)

WRAP_RESET(GX_ResetBankForBG, u32)
WRAP_RESET(GX_ResetBankForOBJ, u32)
WRAP_RESET(GX_ResetBankForBGExtPltt, GXVRamBGExtPltt)
WRAP_RESET(GX_ResetBankForOBJExtPltt, GXVRamOBJExtPltt)
WRAP_RESET(GX_ResetBankForTex, u32)
WRAP_RESET(GX_ResetBankForTexPltt, GXVRamTexPltt)
WRAP_RESET(GX_ResetBankForClearImage, u32)
WRAP_RESET(GX_ResetBankForSubBG, u32)
WRAP_RESET(GX_ResetBankForSubOBJ, u32)
WRAP_RESET(GX_ResetBankForSubBGExtPltt, u32)
WRAP_RESET(GX_ResetBankForSubOBJExtPltt, u32)

WRAP_DISABLE(GX_DisableBankForBG)
WRAP_DISABLE(GX_DisableBankForOBJ)
WRAP_DISABLE(GX_DisableBankForBGExtPltt)
WRAP_DISABLE(GX_DisableBankForOBJExtPltt)
WRAP_DISABLE(GX_DisableBankForTex)
WRAP_DISABLE(GX_DisableBankForTexPltt)
WRAP_DISABLE(GX_DisableBankForClearImage)
WRAP_DISABLE(GX_DisableBankForARM7)
WRAP_DISABLE(GX_DisableBankForLCDC)
WRAP_DISABLE(GX_DisableBankForSubBG)
WRAP_DISABLE(GX_DisableBankForSubOBJ)
WRAP_DISABLE(GX_DisableBankForSubBGExtPltt)
WRAP_DISABLE(GX_DisableBankForSubOBJExtPltt)

WRAP_GET(GX_GetBankForOBJ, GXVRamOBJ)
WRAP_GET(GX_GetBankForOBJExtPltt, u32)
WRAP_GET(GX_GetBankForTex, u32)
WRAP_GET(GX_GetBankForLCDC, u32)
WRAP_GET(GX_GetBankForSubOBJ, GXVRamSubOBJ)
WRAP_GET(GX_GetBankForSubOBJExtPltt, u32)

// gx_state.c: every use loses its banks and every VRAMCNT goes to 0, so every bank is in no place
WRAP_VOID(GX_InitGXState)

// mi_wram.c: WRAMCNT, which sits between VRAMCNT_G and VRAMCNT_H, gives the shared WRAM's two 16 KiB halves to the
// ARM9 or the ARM7. The host's ARM7 side is host code with memory of its own, so nothing is applied: region 0x03's
// window stays the ARM9's plain memory whatever WRAMCNT says, where the DS leaves the ARM9 none after MI_Init's
// MI_WRAM_ARM7_ALL. The register keeps the value for whoever reads it
void port_sdk_MI_SetWramBank(MIWram bank);

void MI_SetWramBank(MIWram bank) {
    port_sdk_MI_SetWramBank(bank);
}
