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
    u16 unk0;
    u16 unk2;
    u32 unk4;
    u32 unk8;
    u32 unkC;
    u32 unk10;
};

BOOL G3DTextDraw_CreateResource(void *texture, const char *texName, u32 a2, const char *plName, const StrBuf *text,
                                u16 a5, u16 a6, u16 color, HeapID heapId, G3DTextDrawResource *resource);
u32 func_ov012_02169fb0(void);
void GFXRegSetMasterBrightness(u32 reg, s32 brightness);
s32 gfxRegGetMasterBrightness(u32 reg);
void gfxClearColor(GXRgb color, int alpha, int depth, int polygonId, BOOL fog);
void gfxDisableLCDCBanks(void);
// G3_MultMtx33: multiplies the current matrix by mtx
void gfxMultMatrix3x3(const MtxFx33 *mtx);
void gfxRegSetAlphaBlend(u32 reg, u32 plane1, u32 plane2, s32 alpha1, s32 alpha2);
void gfxRegSetBrightnessBlend(u32 reg, u32 plane, s32 brightness);
void gfxRegSetBlend(u32 reg, u32 plane1, u32 plane2, s32 alpha1, s32 alpha2, u32 all);
void gfxRegAdjustBrightnessBlend(u32 reg, s32 brightness);
void gfxSetFog(BOOL enabled, int alphaMode, int depthShift, int offset);
void gfxSetLCDCBanks(u32 banks);
void gfxSetEdgeColorTable(const GXRgb *table);
void gfxSetFogTable(const u32 *table);

#endif // POKEBW2_GFL_GRAPHICS_H
