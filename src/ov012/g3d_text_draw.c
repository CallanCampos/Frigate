// Text drawn into a texture of a 3D model, as the Pokémon World Tournament's scoreboard shows it. Function names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the file's name is descriptive
#include "types.h"
#include "gfl/bmp.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nnsys/g3d.h"
#include "system/gf_font.h"
#include "system/printsys.h"

#define TEX_DIM_COUNT 7

// The texture sizes, with their GX_TEXSIZE_S and GX_TEXSIZE_T values
typedef struct {
    u16 size;
    u32 sizeS;
    u32 sizeT;
} TexDimension;

typedef struct {
    G3DTextDrawResource resource;
    Font *font;
} G3DTextDrawWork;

static const TexDimension TEX_DIM_LUT[TEX_DIM_COUNT] = {
    {0x10, 1, 1}, {0x20, 2, 2}, {0x40, 3, 3}, {0x80, 4, 4}, {0x100, 5, 5}, {0x200, 6, 6}, {0x400, 7, 7},
};

static void G3DTextDraw_PrepareBitmap(GFLBitmap *bitmap);
static void G3DTextDraw_RenderBitmap(const StrBuf *text, u16 x, u16 y, u16 color, GFLBitmap *bitmap, Font *font);
static void G3DTextDraw_BitmapTiledToLinear(GFLBitmap *tiled, GFLBitmap *linear);
static void G3DTextDraw_StringToTexture(G3DTextDrawWork *work, const StrBuf *text, u16 x, u16 y, u16 color,
                                        HeapID heapId);
static void G3DTextDraw_ConvertPalette(G3DTextDrawWork *work, const void *palette);

BOOL G3DTextDraw_CreateResource(void *texture, const char *texName, u32 a2, const char *plName, const StrBuf *text, u16 a5,
                                u16 a6, u16 color, HeapID heapId, G3DTextDrawResource *resource) {
    G3DTextDrawResource res;
    NNSG3dResName texResName;
    NNSG3dResName plResName;
    G3DTextDrawWork work;
    u16 dictOffset;
    NNSG3dResTex *texData;
    int i;
    u32 *texEntry;
    u32 texParam;
    u32 sizeS;
    u32 sizeT;
    u32 n;
    u16 *plEntry;
    Font *font;

    res.unk0 = 6;
    res.unk2 = 6;
    res.unkC = 0;
    res.unk10 = 0;
    texData = GFL_G3DResGetTexData(texture);
    res.unk4 = texData->texInfo.vramKey;
    res.unk8 = texData->plttInfo.vramKey;
    dictOffset = texData->texInfo.ofsDict;
    for (i = 0; i < NNS_STD_StrLen(texName); i++) {
        texResName.name[i] = texName[i];
    }
    for (; i < 16; i++) {
        texResName.name[i] = 0;
    }
    texEntry = NNS_G3DFind((NNSG3dResDict *)((u8 *)texData + dictOffset), &texResName);
    if (texEntry == NULL) {
        return FALSE;
    }
    texParam = texEntry[0];
    res.unkC = (texParam & 0xffff) << 3;
    sizeS = (texParam & 0x700000) >> 20;
    sizeT = (texParam & 0x3800000) >> 23;
    for (n = 0; n < TEX_DIM_COUNT; n++) {
        if (sizeS == TEX_DIM_LUT[n].sizeS) {
            res.unk0 = n;
            break;
        }
    }
    for (n = 0; n < TEX_DIM_COUNT; n++) {
        if (sizeT == TEX_DIM_LUT[n].sizeT) {
            res.unk2 = n;
            break;
        }
    }
    dictOffset = texData->plttInfo.ofsDict;
    for (i = 0; i < NNS_STD_StrLen(plName); i++) {
        plResName.name[i] = plName[i];
    }
    for (; i < 16; i++) {
        plResName.name[i] = 0;
    }
    plEntry = NNS_G3DFind((NNSG3dResDict *)((u8 *)texData + dictOffset), &plResName);
    if (plEntry == NULL) {
        return FALSE;
    }
    if (plEntry[1] == 1) {
        return FALSE;
    }
    res.unk10 = (plEntry[0] & 0xffff) << 3;
    font = GFL_FontCreate(0x17, 0, 0, FALSE, heapId);
    work.resource.unk0 = res.unk0;
    work.resource.unk2 = res.unk2;
    work.font = font;
    work.resource.unk4 = res.unk4;
    work.resource.unkC = res.unkC;
    work.resource.unk8 = res.unk8;
    work.resource.unk10 = res.unk10;
    G3DTextDraw_StringToTexture(&work, text, a5, a6, color, heapId);
    if (a2 != 0) {
        G3DTextDraw_ConvertPalette(&work, (const void *)a2);
    }
    GFL_FontFree(font);
    if (resource != NULL) {
        *resource = res;
    }
    return TRUE;
}

u32 func_ov012_02169fb0(void) {
    return TEX_DIM_COUNT;
}

static void G3DTextDraw_PrepareBitmap(GFLBitmap *bitmap) {
    u8 *pixels = GFL_BitmapGetPixelData(bitmap);
    u32 size = GFL_BitmapCalcPixelDataSize(bitmap);
    u32 i;

    for (i = 0; i < size; i++) {
        pixels[i] = 0;
    }
}

static void G3DTextDraw_RenderBitmap(const StrBuf *text, u16 x, u16 y, u16 color, GFLBitmap *bitmap, Font *font) {
    G3DTextDraw_PrepareBitmap(bitmap);
    GFL_TextRendererDrawToBitmapEx(bitmap, x, y, text, font, color);
}

static void G3DTextDraw_BitmapTiledToLinear(GFLBitmap *tiled, GFLBitmap *linear) {
    u16 width = GFL_BitmapGetWidth(tiled) / 8;
    u16 height = GFL_BitmapGetHeight(tiled) / 8;
    u8 bytesPerTile = GFL_BitmapGetBytesPerTile(tiled);
    u16 tileRow = bytesPerTile / 8;
    u8 *src = GFL_BitmapGetPixelData(tiled);
    u8 *dest = GFL_BitmapGetPixelData(linear);
    int i;
    int row;
    int rest;

    for (i = 0; i < width * height * bytesPerTile; i++) {
        row = i / (width * tileRow);
        rest = i % (width * tileRow);
        dest[i] = src[(rest % tileRow) + (row % 8) * tileRow + ((row / 8) * width + rest / tileRow) * bytesPerTile];
    }
}

static void G3DTextDraw_StringToTexture(G3DTextDrawWork *work, const StrBuf *text, u16 x, u16 y, u16 color,
                                        HeapID heapId) {
    u16 width = TEX_DIM_LUT[work->resource.unk0].size;
    u16 height = TEX_DIM_LUT[work->resource.unk2].size;
    GFLBitmap *linear = GFL_BitmapCreate(width / 8, height / 8, 0x20, heapId);
    GFLBitmap *tiled = GFL_BitmapCreate(width / 8, height / 8, 0x20, heapId);
    u8 *pixels;
    u32 texKey;
    u32 texOffset;
    u32 size;

    G3DTextDraw_RenderBitmap(text, x, y, color, tiled, work->font);
    G3DTextDraw_BitmapTiledToLinear(tiled, linear);
    pixels = GFL_BitmapGetPixelData(linear);
    texOffset = work->resource.unkC;
    texKey = (work->resource.unk4 & 0xffff) << 3;
    size = width / 8 * height / 8 * 32;
    gfxBeginTextureUpload();
    cp15_flushDC(pixels, size);
    gfxUploadTexture(pixels, texOffset + texKey, size);
    gfxEndTextureUpload();
    GFL_BitmapFree(tiled);
    GFL_BitmapFree(linear);
}

static void G3DTextDraw_ConvertPalette(G3DTextDrawWork *work, const void *palette) {
    u32 plttKey;
    u32 plttOffset = work->resource.unk10;

    plttKey = (work->resource.unk8 & 0xffff) << 3;

    gfxBeginPaletteUpload();
    cp15_flushDC(palette, 0x20);
    gfxUploadPalette(palette, plttOffset + plttKey, 0x20);
    gfxEndPaletteUpload();
}
