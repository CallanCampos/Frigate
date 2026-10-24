#ifndef POKEBW2_FIELD_FIELD_3DBG_H
#define POKEBW2_FIELD_FIELD_3DBG_H

#include "types.h"
#include "gfl/arc.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// Overlay 36's field_3dbg.c: a textured quad drawn behind the field, at a distance from the camera. Our layouts

// The quad's size and distance, and its polygon ID
typedef struct {
    u16 width;
    u16 height;
    fx32 distance;
    u8 polygonId;
} Field3DBgInit;

// The quad's texture: a file of the archive, its texture parameters and its alpha. The weathers' data holds two
typedef struct {
    u16 fileId;
    u8 texParams[6];
    u8 alpha;
    u8 unk09[3];
} Field3DBgRes;

Field3DBg *Field3DBG_CreateBillboard(const Field3DBgInit *init, u32 heapId);
void Field3DBG_DeleteBillboard(Field3DBg *bg);
void Field3DBG_DrawBillboard(Field3DBg *bg);
// Uploads a texture loaded since the last call
void Field3DBG_ReleaseBillboardTextureVram(Field3DBg *bg);
// Frees the texture
void Field3DBG_FreeBillboardTexture(Field3DBg *bg);
// Loads the texture, in place of the last one
void Field3DBG_LoadBillboardTexture(Field3DBg *bg, ArcTool *arc, const Field3DBgRes *res, u32 heapId);
void Field3DBG_SetBillboardVisible(Field3DBg *bg, u32 enabled);
void Field3DBG_SetBillboardPolygonId(Field3DBg *bg, u8 polygonId);
// Where the texture starts, in texels, and how it is scaled
void Field3DBG_SetBillboardTexOffsetX(Field3DBg *bg, s32 texX);
void Field3DBG_SetBillboardTexOffsetY(Field3DBg *bg, s32 texY);
void Field3DBG_SetBillboardScaleX(Field3DBg *bg, fx32 scaleX);
void Field3DBG_SetBillboardScaleY(Field3DBg *bg, fx32 scaleY);

#endif // POKEBW2_FIELD_FIELD_3DBG_H
