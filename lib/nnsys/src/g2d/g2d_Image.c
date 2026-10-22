#include "nitro/os.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_Image.c: image and palette proxies, which keep where an image or a palette was loaded in VRAM for
// each engine and how to draw it, and the functions that load character and palette data into VRAM and fill them in

// The texture size of a side of a 2D-mapped image, from its length in characters
static inline u32 GetPow_(u16 numChars) {
    switch (numChars) {
    case 1:
        return GX_TEXSIZE_S8;
    case 2:
        return GX_TEXSIZE_S16;
    case 4:
        return GX_TEXSIZE_S32;
    case 8:
        return GX_TEXSIZE_S64;
    case 16:
        return GX_TEXSIZE_S128;
    case 32:
        return GX_TEXSIZE_S256;
    default:
        return GX_TEXSIZE_S8;
    }
}

// Sets the OBJ character mapping of the engine the image is loaded for
static inline void SetOBJVRamModeCharacterMapping_(NNS_G2D_VRAM_TYPE type, GXOBJVRamModeChar mode) {
    switch (type) {
    case NNS_G2D_VRAM_TYPE_3DMAIN:
        break;
    case NNS_G2D_VRAM_TYPE_2DMAIN:
        GX_SetOBJVRamModeChar(mode);
        break;
    case NNS_G2D_VRAM_TYPE_2DSUB:
        GXS_SetOBJVRamModeChar(mode);
        break;
    }
}

static inline void DoLoadingToVram_(const NNSG2dCharacterData *data, u32 baseAddr, NNS_G2D_VRAM_TYPE type) {
    switch (type) {
    case NNS_G2D_VRAM_TYPE_3DMAIN:
        GX_BeginLoadTex();
        GX_LoadTex(data->pRawData, baseAddr, data->szByte);
        GX_EndLoadTex();
        break;
    case NNS_G2D_VRAM_TYPE_2DMAIN:
        GX_LoadOBJ(data->pRawData, baseAddr, data->szByte);
        break;
    case NNS_G2D_VRAM_TYPE_2DSUB:
        GXS_LoadOBJ(data->pRawData, baseAddr, data->szByte);
        break;
    }
}

// Fills in the proxy's attributes from the character data and records where it was loaded
static inline void SetupImageProxyPrams_(const NNSG2dCharacterData *data, u32 baseAddr, NNS_G2D_VRAM_TYPE type,
                                         NNSG2dImageProxy *proxy) {
    if (data->mappingType == GX_OBJVRAMMODE_CHAR_2D) {
        proxy->attr.sizeS = GetPow_(data->W);
        proxy->attr.sizeT = GetPow_(data->H);
    } else {
        proxy->attr.sizeS = data->W;
        proxy->attr.sizeT = data->H;
    }
    proxy->attr.fmt = data->pixelFmt;
    proxy->attr.bExtendedPlt = FALSE;
    proxy->attr.plttUse = TRUE;
    proxy->attr.mappingType = data->mappingType;
    NNS_G2dSetImageLocation(proxy, type, baseAddr);
}

static inline void DoLoadingPalette_(const NNSG2dPaletteData *data, const void *src, u32 addr, u32 size,
                                     NNS_G2D_VRAM_TYPE type) {
    switch (type) {
    case NNS_G2D_VRAM_TYPE_2DMAIN:
        if (data->bExtendedPlt) {
            GX_BeginLoadOBJExtPltt();
            GX_LoadOBJExtPltt(src, addr, size);
            GX_EndLoadOBJExtPltt();
        } else {
            GX_LoadOBJPltt(src, addr, size);
        }
        break;
    case NNS_G2D_VRAM_TYPE_2DSUB:
        if (data->bExtendedPlt) {
            GXS_BeginLoadOBJExtPltt();
            GXS_LoadOBJExtPltt(src, addr, size);
            GXS_EndLoadOBJExtPltt();
        } else {
            GXS_LoadOBJPltt(src, addr, size);
        }
        break;
    case NNS_G2D_VRAM_TYPE_3DMAIN:
        GX_BeginLoadTexPltt();
        GX_LoadTexPltt(src, addr, size);
        GX_EndLoadTexPltt();
        break;
    }
}

void NNS_G2dInitImageProxy(NNSG2dImageProxy *proxy) {
    int i;

    for (i = 0; i < NNS_G2D_VRAM_TYPE_MAX; i++) {
        proxy->vramLocation.baseAddrOfVram[i] = NNS_G2D_VRAM_ADDR_NONE;
    }
}

void NNS_G2dSetImageLocation(NNSG2dImageProxy *proxy, NNS_G2D_VRAM_TYPE type, u32 addr) {
    proxy->vramLocation.baseAddrOfVram[type] = addr;
}

u32 NNS_G2dGetImageLocation(const NNSG2dImageProxy *proxy, NNS_G2D_VRAM_TYPE type) {
    return proxy->vramLocation.baseAddrOfVram[type];
}

BOOL NNS_G2dIsImageReadyToUse(const NNSG2dImageProxy *proxy, NNS_G2D_VRAM_TYPE type) {
    return proxy->vramLocation.baseAddrOfVram[type] != NNS_G2D_VRAM_ADDR_NONE;
}

void NNS_G2dInitImagePaletteProxy(NNSG2dImagePaletteProxy *proxy) {
    int i;

    for (i = 0; i < NNS_G2D_VRAM_TYPE_MAX; i++) {
        proxy->vramLocation.baseAddrOfVram[i] = NNS_G2D_VRAM_ADDR_NONE;
    }
}

void NNS_G2dSetImagePaletteLocation(NNSG2dImagePaletteProxy *proxy, NNS_G2D_VRAM_TYPE type, u32 addr) {
    proxy->vramLocation.baseAddrOfVram[type] = addr;
}

u32 NNS_G2dGetImagePaletteLocation(const NNSG2dImagePaletteProxy *proxy, NNS_G2D_VRAM_TYPE type) {
    return proxy->vramLocation.baseAddrOfVram[type];
}

BOOL NNS_G2dIsImagePaletteReadyToUse(const NNSG2dImagePaletteProxy *proxy, NNS_G2D_VRAM_TYPE type) {
    return proxy->vramLocation.baseAddrOfVram[type] != NNS_G2D_VRAM_ADDR_NONE;
}

void NNS_G2dLoadImage1DMapping(const NNSG2dCharacterData *data, u32 baseAddr, NNS_G2D_VRAM_TYPE type,
                               NNSG2dImageProxy *proxy) {
    SetOBJVRamModeCharacterMapping_(type, (GXOBJVRamModeChar)data->mappingType);
    DC_FlushRange(data->pRawData, data->szByte);
    DoLoadingToVram_(data, baseAddr, type);
    SetupImageProxyPrams_(data, baseAddr, type, proxy);
}

void NNS_G2dLoadImage2DMapping(const NNSG2dCharacterData *data, u32 baseAddr, NNS_G2D_VRAM_TYPE type,
                               NNSG2dImageProxy *proxy) {
    SetOBJVRamModeCharacterMapping_(type, (GXOBJVRamModeChar)data->mappingType);
    DC_FlushRange(data->pRawData, data->szByte);
    DoLoadingToVram_(data, baseAddr, type);
    SetupImageProxyPrams_(data, baseAddr, type, proxy);
}

// For an image whose characters are transferred to VRAM as its cells are shown: only flushes it and sets the mapping
void NNS_G2dLoadImageVramTransfer(const NNSG2dCharacterData *data, u32 baseAddr, NNS_G2D_VRAM_TYPE type,
                                  NNSG2dImageProxy *proxy) {
    DC_FlushRange(data->pRawData, data->szByte);
    SetOBJVRamModeCharacterMapping_(type, (GXOBJVRamModeChar)data->mappingType);
    SetupImageProxyPrams_(data, baseAddr, type, proxy);
}

void NNS_G2dLoadPalette(const NNSG2dPaletteData *data, u32 addr, NNS_G2D_VRAM_TYPE type,
                        NNSG2dImagePaletteProxy *proxy) {
    const void *src;
    u32 size;

    size = data->szByte;
    src = data->pRawData;
    DC_FlushRange(src, size);
    DoLoadingPalette_(data, src, addr, size, type);
    proxy->fmt = data->fmt;
    proxy->bExtendedPlt = data->bExtendedPlt;
    NNS_G2dSetImagePaletteLocation(proxy, type, addr);
}

// Loads the palettes of a file that holds only some of them, each to its own place
void NNS_G2dLoadPaletteEx(const NNSG2dPaletteData *data, const NNSG2dPaletteCompressInfo *cmpInfo, u32 addr,
                          NNS_G2D_VRAM_TYPE type, NNSG2dImagePaletteProxy *proxy) {
    const u32 size = data->fmt == GX_TEXFMT_PLTT16 ? 16 * sizeof(GXRgb) : 256 * sizeof(GXRgb);
    const u16 numPalette = cmpInfo->numPalette;
    u16 i;

    for (i = 0; i < numPalette; i++) {
        const u8 *src;
        u32 srcOffset;
        u32 dstOffset;

        dstOffset = size * ((u16 *)cmpInfo->pPlttIdxTbl)[i];
        src = data->pRawData;
        srcOffset = size * i;

        DC_FlushRange(src, data->szByte);
        DoLoadingPalette_(data, src + srcOffset, addr + dstOffset, size, type);
    }
    proxy->fmt = data->fmt;
    proxy->bExtendedPlt = data->bExtendedPlt;
    NNS_G2dSetImagePaletteLocation(proxy, type, addr);
}
