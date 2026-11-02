#ifndef POKEBW2_GFL_GRAPHICS_H
#define POKEBW2_GFL_GRAPHICS_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "struct_decls.h"

struct G3DTextDrawResource {
    // The texture's width and height, as indexes into g3d_text_draw.c's TEX_DIM_LUT
    u16 widthIndex;
    u16 heightIndex;
    u32 texVramKey;
    u32 plttVramKey;
    // Where the texture's and the palette's data are, from the addresses of their VRAM keys
    u32 texOffset;
    u32 plttOffset;
};

BOOL G3DTextDraw_CreateResource(G3DResource *texture, const char *texName, uptr palette, const char *plName,
                                const StrBuf *text, u16 x, u16 y, u16 color, HeapID heapId,
                                G3DTextDrawResource *resource);
u32 G3DTextDraw_GetTexDimCount(void);
// NitroSDK's GX functions are in nitro/gx.h

#endif // POKEBW2_GFL_GRAPHICS_H
