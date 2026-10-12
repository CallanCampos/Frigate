#include "types.h"
#include "system/nsbtx_to_clwk.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "nnsys/g3d.h"
#include "system/palanm.h"

// Shows a texture of a model file (NSBTX), such as an overworld sprite, on an actor: the texture's bitmap is cut
// into the 8x8 tiles of OBJ characters

static void NsbtxToClwk_ConvertTiles(const u8 *src, int width, int x, int y, int tilesX, int tilesY, void *dest);
static void NsbtxToClwk_CopyTile(const u8 *src, u8 *dest, int *srcOffset, int *destOffset, int size, int stride);

// NitroSystem's, which g3d.h does not have yet
static inline const NNSG3dResDictTexData *NNS_G3dGetTexDataByIdx(const NNSG3dResTex *tex, u32 idx) {
    if (tex != NULL) {
        return NNS_G3dGetResDataByIdx(&tex->dict, idx);
    }
    return NULL;
}

// Cuts tilesX by tilesY tiles out of a 16-color bitmap that is width tiles wide, starting at the tile (x, y)
static void NsbtxToClwk_ConvertTiles(const u8 *src, int width, int x, int y, int tilesX, int tilesY, void *dest) {
    u8 *out = dest;
    int i, j;
    int srcOffset;
    int destOffset = 0;
    int stride = width * 4;

    for (i = y; i < y + tilesY; i++) {
        for (j = x; j < x + tilesX; j++) {
            srcOffset = j * 4 + i * stride * 8;
            NsbtxToClwk_CopyTile(src, out, &srcOffset, &destOffset, 4, stride);
        }
    }
}

// Copies the 8 rows of a tile
static void NsbtxToClwk_CopyTile(const u8 *src, u8 *dest, int *srcOffset, int *destOffset, int size, int stride) {
    int i;

    for (i = 0; i < 8; i++) {
        sys_memcpy(src + *srcOffset, dest + *destOffset, size);
        *srcOffset += stride;
        *destOffset += size;
    }
}

void NsbtxToClwk_LoadTexture(ClActor *actor, u32 arcId, u32 fileId, u8 texture, u16 width, u16 height, u32 a6, u32 a7,
                   HeapID heapId) {
    NsbtxToClwk_LoadTextureEx(actor, arcId, fileId, texture, width, height, a6, a7, NULL, 0, heapId);
}

void NsbtxToClwk_LoadTextureEx(ClActor *actor, u32 arcId, u32 fileId, u8 texture, u16 width, u16 height, u32 offset, u32 vramType,
                   PaletteFade *fade, u8 palette, HeapID heapId) {
    NNSG2dImagePaletteProxy proxy;
    u32 size = width * height * 32;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(arcId, heapId);
    void *file = GFL_ArcToolReadHeapNew(arc, fileId, heapId);
    NNSG3dResTex *tex = NNS_G3DResGetTexBlock(file);
    u8 *bitmap;
    u32 texOffset;
    u8 *chars;
    void *colors;

    texOffset = (NNS_G3dGetTexDataByIdx(tex, texture)->texImageParam & 0xffff) << 3;
    bitmap = (u8 *)tex + tex->texInfo.ofsTex;
    chars = GFL_HeapAllocate(HEAPID_TAIL(heapId), size, FALSE, "nsbtx_to_clwk.c", 119);
    NsbtxToClwk_ConvertTiles(bitmap + texOffset, width, 0, 0, width, height, chars);
    func_0204bab8(func_0204c428(actor), chars, size, offset, vramType);
    GFL_HeapFree(chars);

    colors = (u8 *)tex + tex->plttInfo.ofsPlttData;
    cp15_flushDC(colors, 32);
    func_0204c3d0(actor, &proxy);

    if (vramType == CLACT_VRAM_MAIN || vramType == CLACT_VRAM_BOTH) {
        if (fade == NULL) {
            gfxUploadStdPaletteObjA(colors, NNS_G2dGetImagePaletteLocation(&proxy, NNS_G2D_VRAM_TYPE_2DMAIN), 32);
        } else {
            PaletteFade_LoadData(fade, colors, PALFADE_BUFFER_MAIN_OBJ, palette * 16, 32);
        }
    }
    if (vramType == CLACT_VRAM_SUB || vramType == CLACT_VRAM_BOTH) {
        if (fade == NULL) {
            gfxUploadStdPaletteObjB(colors, NNS_G2dGetImagePaletteLocation(&proxy, NNS_G2D_VRAM_TYPE_2DSUB), 32);
        } else {
            PaletteFade_LoadData(fade, colors, PALFADE_BUFFER_SUB_OBJ, palette * 16, 32);
        }
    }

    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
}
