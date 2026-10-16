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

// The quad's texture: a file of the archive, its texture parameters and its alpha
typedef struct {
    u16 fileId;
    u8 texParams[6];
    u8 alpha;
} Field3DBgRes;

Field3DBg *func_ov036_02198bc0(const Field3DBgInit *init, u32 heapId);
void func_ov036_02198bf0(Field3DBg *bg);
void func_ov036_02198c10(Field3DBg *bg);
// Uploads a texture loaded since the last call
void func_ov036_02198c8c(Field3DBg *bg);
// Frees the texture
void func_ov036_02198c9c(Field3DBg *bg);
// Loads the texture, in place of the last one
void func_ov036_02198cb4(Field3DBg *bg, ArcTool *arc, const Field3DBgRes *res, u32 heapId);
void func_ov036_02198cf0(Field3DBg *bg, u32 enabled);
void func_ov036_02198cf4(Field3DBg *bg, u8 polygonId);
// Where the texture starts, in texels, and how it is scaled
void func_ov036_02198cf8(Field3DBg *bg, s32 texX);
void func_ov036_02198cfc(Field3DBg *bg, s32 texY);
void func_ov036_02198d00(Field3DBg *bg, fx32 scaleX);
void func_ov036_02198d04(Field3DBg *bg, fx32 scaleY);

#endif // POKEBW2_FIELD_FIELD_3DBG_H
