// NitroSDK libgx's gx_load3d.c on the host: the loads into the 3D engine's memory (textures, texture palettes and the
// rear plane image), each through the banks the engine has for it, which a Begin takes into LCDC mode for the time, the
// loads copy into, and an End gives back. port/hal/replaced.txt keeps the decomp's lib/nitro/src/gx/gx_load3d.c out of
// the host build: it keeps the banks' LCDC addresses shifted right by 12 in static u16 tables, which a host address
// can't be, and the addresses of a load in static u32s, which would cut one.
//
// The functions are the decomp's, with two changes. The addresses stay DS addresses (the tables hold them whole, and
// the statics are DS addresses, never host ones), and only the copy turns them into host memory. And the copies, which
// the SDK starts on GX's DMA channel and the End waits for, are made at once by port_gx_load_copy, which also gives the
// pages they reach new generations for the 3D renderer's caches; the End has no DMA to wait for. The decomp's statics
// are laid out in MWCC's order there; here the order doesn't matter.
#include "hal/gx/gx_internal.h"
#include "nitro/gx.h"

// The banks' addresses in LCDC mode, where the CPU sees them (HW_LCDC_VRAM_A-G)
#define LCDC_VRAM_A 0x06800000u
#define LCDC_VRAM_B 0x06820000u
#define LCDC_VRAM_C 0x06840000u
#define LCDC_VRAM_D 0x06860000u
#define LCDC_VRAM_E 0x06880000u
#define LCDC_VRAM_F 0x06890000u
#define LCDC_VRAM_G 0x06894000u

// Where the CPU sees the banks of the texture palettes, indexed by the banks / 16: the first of E, F and G that the
// palette is in. E and G together are not a valid choice
static const u32 TEXPLTT_VRAM_BANK_ADDRS[8] = {
    0,           // none
    LCDC_VRAM_E, // E
    LCDC_VRAM_F, // F
    LCDC_VRAM_E, // E and F
    LCDC_VRAM_G, // G
    0,           // E and G
    LCDC_VRAM_F, // F and G
    LCDC_VRAM_E, // E, F and G
};

// Where the CPU sees the banks of the textures, indexed by the banks (A is 1, B 2, C 4, D 8). An offset into the
// textures starts at addr. When the banks are not next to each other, the offsets from size on are in a second run that
// starts at addr2 (0 when there is none), and size is the length of the first run
typedef struct {
    u32 addr;
    u32 addr2;
    u32 size;
} TexBankRange;

static const TexBankRange TEX_VRAM_BANK_RANGES[16] = {
    { 0, 0, 0 },
    { LCDC_VRAM_A, 0, 0 },
    { LCDC_VRAM_B, 0, 0 },
    { LCDC_VRAM_A, 0, 0 },
    { LCDC_VRAM_C, 0, 0 },
    { LCDC_VRAM_A, LCDC_VRAM_C, 0x20000 },
    { LCDC_VRAM_B, 0, 0 },
    { LCDC_VRAM_A, 0, 0 },
    { LCDC_VRAM_D, 0, 0 },
    { LCDC_VRAM_A, LCDC_VRAM_D, 0x20000 },
    { LCDC_VRAM_B, LCDC_VRAM_D, 0x20000 },
    { LCDC_VRAM_A, LCDC_VRAM_D, 0x40000 },
    { LCDC_VRAM_C, 0, 0 },
    { LCDC_VRAM_A, LCDC_VRAM_C, 0x20000 },
    { LCDC_VRAM_B, 0, 0 },
    { LCDC_VRAM_A, 0, 0 },
};

// The state of the loads: the banks that each Begin took (0 when none) and the End gives back, and where the CPU sees
// them, as DS addresses. g_GfxVRAMUploadBanksRearPlaneImage is swan's name
static GXVRamTexPltt sPlttBanks;
static u32 sPlttAddr;
static u32 sTexAddr;
static u32 sTexSize;
static u32 g_GfxVRAMUploadBanksRearPlaneImage;
static u32 sRearPlaneImageAddr;
static u32 sTexBanks;
static u32 sTexAddr2;

void GX_BeginLoadTex(void) {
    sTexBanks = GX_ResetBankForTex();
    sTexAddr = TEX_VRAM_BANK_RANGES[sTexBanks].addr;
    sTexAddr2 = TEX_VRAM_BANK_RANGES[sTexBanks].addr2;
    sTexSize = TEX_VRAM_BANK_RANGES[sTexBanks].size;
}

void GX_LoadTex(const void *src, u32 offset, u32 size) {
    u32 addr2 = sTexAddr2;
    u32 dest;
    if (addr2 == 0) {
        // The banks are next to each other
        dest = sTexAddr + offset;
    } else if (offset + size < sTexSize) {
        // The texture is in the first run
        dest = sTexAddr + offset;
    } else if (offset >= sTexSize) {
        // The texture is in the second run
        dest = addr2 + offset - sTexSize;
    } else {
        // The texture goes over from the first run to the second
        u32 firstSize = sTexSize - offset;
        port_gx_load_copy(sTexAddr + offset, src, firstSize);
        port_gx_load_copy(addr2, (const u8 *)src + firstSize, size - firstSize);
        return;
    }
    port_gx_load_copy(dest, src, size);
}

void GX_EndLoadTex(void) {
    GX_SetBankForTex(sTexBanks);
    sTexSize = 0;
    sTexAddr2 = 0;
    sTexAddr = 0;
    sTexBanks = 0;
}

void GX_BeginLoadTexPltt(void) {
    sPlttBanks = GX_ResetBankForTexPltt();
    sPlttAddr = TEXPLTT_VRAM_BANK_ADDRS[sPlttBanks >> 4];
}

void GX_LoadTexPltt(const void *src, u32 offset, u32 size) {
    port_gx_load_copy(sPlttAddr + offset, src, size);
}

void GX_EndLoadTexPltt(void) {
    GX_SetBankForTexPltt(sPlttBanks);
    sPlttBanks = 0;
    sPlttAddr = 0;
}

void GX_BeginLoadClearImage(void) {
    g_GfxVRAMUploadBanksRearPlaneImage = GX_ResetBankForClearImage();
    // The second image is in the highest bank given and the first 128 KB before it, so with a single bank the address
    // is of the bank before it (for A alone, 0x067E0000: the DS's mirror of OBJ-B, where the CPU stores go too)
    switch (g_GfxVRAMUploadBanksRearPlaneImage) {
    case GX_VRAM_CLEARIMAGE_128_B:
    case GX_VRAM_CLEARIMAGE_256_AB:
        sRearPlaneImageAddr = LCDC_VRAM_A;
        break;
    case GX_VRAM_CLEARIMAGE_128_D:
    case GX_VRAM_CLEARIMAGE_256_CD:
        sRearPlaneImageAddr = LCDC_VRAM_C;
        break;
    case GX_VRAM_CLEARIMAGE_128_A:
        sRearPlaneImageAddr = LCDC_VRAM_A - 0x20000;
        break;
    case GX_VRAM_CLEARIMAGE_128_C:
        sRearPlaneImageAddr = LCDC_VRAM_B;
        break;
    }
}

void GX_LoadClearImageColor(const void *src, u32 size) {
    port_gx_load_copy(sRearPlaneImageAddr, src, size);
}

void GX_LoadClearImageDepth(const void *src, u32 size) {
    port_gx_load_copy(sRearPlaneImageAddr + 0x20000, src, size);
}

void GX_EndLoadClearImage(void) {
    GX_SetBankForClearImage(g_GfxVRAMUploadBanksRearPlaneImage);
    g_GfxVRAMUploadBanksRearPlaneImage = 0;
    sRearPlaneImageAddr = 0;
}
