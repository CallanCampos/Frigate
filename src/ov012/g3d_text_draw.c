// Text drawn into a texture of a 3D model, as the Pokémon World Tournament's scoreboard shows it. Function names from
// swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the file's name is descriptive
#include "types.h"
#include "constants/arc.h"
#include "gfl/bmp.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/std.h"
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
    { 0x10, 1, 1 }, { 0x20, 2, 2 }, { 0x40, 3, 3 }, { 0x80, 4, 4 }, { 0x100, 5, 5 }, { 0x200, 6, 6 }, { 0x400, 7, 7 },
};

static void G3DTextDraw_PrepareBitmap(GFLBitmap *bitmap);
static void G3DTextDraw_RenderBitmap(const StrBuf *text, u16 x, u16 y, u16 color, GFLBitmap *bitmap, Font *font);
static void G3DTextDraw_BitmapTiledToLinear(GFLBitmap *tiled, GFLBitmap *linear);
static void G3DTextDraw_StringToTexture(G3DTextDrawWork *work, const StrBuf *text, u16 x, u16 y, u16 color,
                                        HeapID heapId);
static void G3DTextDraw_ConvertPalette(G3DTextDrawWork *work, const void *palette);

BOOL G3DTextDraw_CreateResource(G3DResource *texture, const char *texName, uptr palette, const char *plName,
                                const StrBuf *text, u16 x, u16 y, u16 color, HeapID heapId,
                                G3DTextDrawResource *resource) {
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

    res.widthIndex = 6;
    res.heightIndex = 6;
    res.texOffset = 0;
    res.plttOffset = 0;
    texData = GFL_G3DResGetTexData(texture);
    res.texVramKey = texData->texInfo.vramKey;
    res.plttVramKey = texData->plttInfo.vramKey;
    dictOffset = texData->texInfo.ofsDict;
    for (i = 0; i < STD_GetStringLength(texName); i++) {
        texResName.name[i] = texName[i];
    }
    for (; i < 16; i++) {
        texResName.name[i] = 0;
    }
    texEntry = NNS_G3dGetResDataByName((NNSG3dResDict *)((u8 *)texData + dictOffset), &texResName);
    if (texEntry == NULL) {
        return FALSE;
    }
    texParam = texEntry[0];
    res.texOffset = (texParam & 0xffff) << 3;
    sizeS = (texParam & 0x700000) >> 20;
    sizeT = (texParam & 0x3800000) >> 23;
    for (n = 0; n < TEX_DIM_COUNT; n++) {
        if (sizeS == TEX_DIM_LUT[n].sizeS) {
            res.widthIndex = n;
            break;
        }
    }
    for (n = 0; n < TEX_DIM_COUNT; n++) {
        if (sizeT == TEX_DIM_LUT[n].sizeT) {
            res.heightIndex = n;
            break;
        }
    }
    dictOffset = texData->plttInfo.ofsDict;
    for (i = 0; i < STD_GetStringLength(plName); i++) {
        plResName.name[i] = plName[i];
    }
    for (; i < 16; i++) {
        plResName.name[i] = 0;
    }
    plEntry = NNS_G3dGetResDataByName((NNSG3dResDict *)((u8 *)texData + dictOffset), &plResName);
    if (plEntry == NULL) {
        return FALSE;
    }
    if (plEntry[1] == 1) {
        return FALSE;
    }
    res.plttOffset = (plEntry[0] & 0xffff) << 3;
    font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    work.resource.widthIndex = res.widthIndex;
    work.resource.heightIndex = res.heightIndex;
    work.font = font;
    work.resource.texVramKey = res.texVramKey;
    work.resource.texOffset = res.texOffset;
    work.resource.plttVramKey = res.plttVramKey;
    work.resource.plttOffset = res.plttOffset;
    G3DTextDraw_StringToTexture(&work, text, x, y, color, heapId);
    if (palette != 0) {
        G3DTextDraw_ConvertPalette(&work, (const void *)palette);
    }
    GFL_FontFree(font);
    if (resource != NULL) {
        *resource = res;
    }
    return TRUE;
}

u32 G3DTextDraw_GetTexDimCount(void) {
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
    u16 width = TEX_DIM_LUT[work->resource.widthIndex].size;
    u16 height = TEX_DIM_LUT[work->resource.heightIndex].size;
    GFLBitmap *linear = GFL_BitmapCreate(width / 8, height / 8, 0x20, heapId);
    GFLBitmap *tiled = GFL_BitmapCreate(width / 8, height / 8, 0x20, heapId);
    u8 *pixels;
    u32 texKey;
    u32 texOffset;
    u32 size;

    G3DTextDraw_RenderBitmap(text, x, y, color, tiled, work->font);
    G3DTextDraw_BitmapTiledToLinear(tiled, linear);
    pixels = GFL_BitmapGetPixelData(linear);
    texOffset = work->resource.texOffset;
    texKey = (work->resource.texVramKey & 0xffff) << 3;
    size = width / 8 * height / 8 * 32;
    GX_BeginLoadTex();
    DC_FlushRange(pixels, size);
    GX_LoadTex(pixels, texOffset + texKey, size);
    GX_EndLoadTex();
    GFL_BitmapFree(tiled);
    GFL_BitmapFree(linear);
}

static void G3DTextDraw_ConvertPalette(G3DTextDrawWork *work, const void *palette) {
    u32 plttKey;
    u32 plttOffset = work->resource.plttOffset;

    plttKey = (work->resource.plttVramKey & 0xffff) << 3;

    GX_BeginLoadTexPltt();
    DC_FlushRange(palette, 0x20);
    GX_LoadTexPltt(palette, plttOffset + plttKey, 0x20);
    GX_EndLoadTexPltt();
}
