// NitroSDK libgx's gx_load3d.c: the loads into the memory of the 3D engine, which copy by DMA on GX's channel, by NDMA
// or with the CPU: the textures, the texture palettes and the rear plane (clear) image. Each is loaded through the VRAM
// banks that the engine has for it, which are mapped to the CPU for the time: a Begin that takes them, the loads, and
// an End that waits for the copies and gives them back. The file has no strings; its name is the SDK's, from the
// layout of libgx. It needs `-ipa file` (its state is reached from one base). The linker's veneers for the NDMA copy,
// the NDMA copy with a callback and the NDMA wait in TwlSDK's LTD module are the ones that gx_load2d.c's callers made;
// they are not part of the source.
#include "nitro/gx.h"

// Where the CPU sees the banks of the texture palettes, as a multiple of 4 KB, indexed by the banks / 16: the first of
// E, F and G that the palette is in. E and G together are not a valid choice
static const u16 TEXPLTT_VRAM_BANK_ADDRS[8] = {
    0,                    // none
    HW_LCDC_VRAM_E >> 12, // E
    HW_LCDC_VRAM_F >> 12, // F
    HW_LCDC_VRAM_E >> 12, // E and F
    HW_LCDC_VRAM_G >> 12, // G
    0,                    // E and G
    HW_LCDC_VRAM_F >> 12, // F and G
    HW_LCDC_VRAM_E >> 12, // E, F and G
};

// Where the CPU sees the banks of the textures, as multiples of 4 KB, indexed by the banks (A is 1, B 2, C 4, D 8).
// An offset into the textures starts at addr. When the banks are not next to each other, the offsets from size on are
// in a second run that starts at addr2 (0 when there is none), and size is the length of the first run
typedef struct {
    u16 addr;
    u16 addr2;
    u16 size;
} TexBankRange;

#define TEX_RANGE(addr, addr2, size) { (addr) >> 12, (addr2) >> 12, (size) >> 12 }

static const TexBankRange TEX_VRAM_BANK_RANGES[16] = {
    TEX_RANGE(0, 0, 0),
    TEX_RANGE(HW_LCDC_VRAM_A, 0, 0),
    TEX_RANGE(HW_LCDC_VRAM_B, 0, 0),
    TEX_RANGE(HW_LCDC_VRAM_A, 0, 0),
    TEX_RANGE(HW_LCDC_VRAM_C, 0, 0),
    TEX_RANGE(HW_LCDC_VRAM_A, HW_LCDC_VRAM_C, 0x20000),
    TEX_RANGE(HW_LCDC_VRAM_B, 0, 0),
    TEX_RANGE(HW_LCDC_VRAM_A, 0, 0),
    TEX_RANGE(HW_LCDC_VRAM_D, 0, 0),
    TEX_RANGE(HW_LCDC_VRAM_A, HW_LCDC_VRAM_D, 0x20000),
    TEX_RANGE(HW_LCDC_VRAM_B, HW_LCDC_VRAM_D, 0x20000),
    TEX_RANGE(HW_LCDC_VRAM_A, HW_LCDC_VRAM_D, 0x40000),
    TEX_RANGE(HW_LCDC_VRAM_C, 0, 0),
    TEX_RANGE(HW_LCDC_VRAM_A, HW_LCDC_VRAM_C, 0x20000),
    TEX_RANGE(HW_LCDC_VRAM_B, 0, 0),
    TEX_RANGE(HW_LCDC_VRAM_A, 0, 0),
};

// The state of the loads: the banks that each Begin took (0 when none) and the End gives back, and where the CPU sees
// them. The first of the eight words is swan's g_GfxVRAMUploadBanksRearPlaneImage. MWCC lays the words out in a fixed
// permutation of their order here, which is why the declarations are not grouped by load
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
    sTexAddr = TEX_VRAM_BANK_RANGES[sTexBanks].addr << 12;
    sTexAddr2 = TEX_VRAM_BANK_RANGES[sTexBanks].addr2 << 12;
    sTexSize = TEX_VRAM_BANK_RANGES[sTexBanks].size << 12;
}

void GX_LoadTex(const void *src, u32 offset, u32 size) {
    u32 addr2 = sTexAddr2;
    void *dest;
    if (addr2 == 0) {
        // The banks are next to each other
        dest = (void *)(sTexAddr + offset);
    } else if (offset + size < sTexSize) {
        // The texture is in the first run
        dest = (void *)(sTexAddr + offset);
    } else if (offset >= sTexSize) {
        // The texture is in the second run
        dest = (void *)(addr2 + offset - sTexSize);
    } else {
        // The texture goes over from the first run to the second
        u32 firstSize = sTexSize - offset;
        GXi_DmaCopy32(GXi_DmaId, src, (void *)(sTexAddr + offset), firstSize);
        GXi_DmaCopy32Async(GXi_DmaId, (const u8 *)src + firstSize, (void *)addr2, size - firstSize, NULL, NULL);
        return;
    }
    GXi_DmaCopy32Async(GXi_DmaId, src, dest, size, NULL, NULL);
}

void GX_EndLoadTex(void) {
    GXi_WaitDma(GXi_DmaId);
    GX_SetBankForTex(sTexBanks);
    sTexSize = 0;
    sTexAddr2 = 0;
    sTexAddr = 0;
    sTexBanks = 0;
}

void GX_BeginLoadTexPltt(void) {
    sPlttBanks = GX_ResetBankForTexPltt();
    sPlttAddr = TEXPLTT_VRAM_BANK_ADDRS[sPlttBanks >> 4] << 12;
}

void GX_LoadTexPltt(const void *src, u32 offset, u32 size) {
    GXi_DmaCopy32Async(GXi_DmaId, src, (void *)(sPlttAddr + offset), size, NULL, NULL);
}

void GX_EndLoadTexPltt(void) {
    GXi_WaitDma(GXi_DmaId);
    GX_SetBankForTexPltt(sPlttBanks);
    sPlttBanks = 0;
    sPlttAddr = 0;
}

void GX_BeginLoadClearImage(void) {
    g_GfxVRAMUploadBanksRearPlaneImage = GX_ResetBankForClearImage();
    // The second image is in the highest bank given and the first 128 KB before it, so with a single bank the address
    // is of the bank before it
    switch (g_GfxVRAMUploadBanksRearPlaneImage) {
    case GX_VRAM_B:
    case GX_VRAM_A | GX_VRAM_B:
        sRearPlaneImageAddr = HW_LCDC_VRAM_A;
        break;
    case GX_VRAM_D:
    case GX_VRAM_C | GX_VRAM_D:
        sRearPlaneImageAddr = HW_LCDC_VRAM_C;
        break;
    case GX_VRAM_A:
        sRearPlaneImageAddr = HW_LCDC_VRAM_A - 0x20000;
        break;
    case GX_VRAM_C:
        sRearPlaneImageAddr = HW_LCDC_VRAM_B;
        break;
    }
}

void GX_LoadClearImageColor(const void *src, u32 size) {
    GXi_DmaCopy32Async(GXi_DmaId, src, (void *)sRearPlaneImageAddr, size, NULL, NULL);
}

void GX_LoadClearImageDepth(const void *src, u32 size) {
    GXi_DmaCopy32Async(GXi_DmaId, src, (void *)(sRearPlaneImageAddr + 0x20000), size, NULL, NULL);
}

void GX_EndLoadClearImage(void) {
    GXi_WaitDma(GXi_DmaId);
    GX_SetBankForClearImage(g_GfxVRAMUploadBanksRearPlaneImage);
    g_GfxVRAMUploadBanksRearPlaneImage = 0;
    sRearPlaneImageAddr = 0;
}
