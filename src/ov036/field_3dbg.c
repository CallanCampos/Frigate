#include "types.h"
#include "field/field_3dbg.h"
#include "gfl/arc.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/g3d.h"

// A flat textured quad drawn with the geometry engine behind the field (field_3dbg.c, after the file name that its
// allocations pass), such as the weather's 3D background images. The layouts are ours

typedef struct {
    u8 texParams[6];
    u8 loaded;
    u8 needsUpload;
    G3DResource *resource;
} Field3DBgTex;

typedef struct {
    fx32 halfWidth;
    fx32 halfHeight;
    fx32 depth;
    fx32 scale;
    fx32 texU0;
    fx32 texU1;
    fx32 texV0;
    fx32 texV1;
    MtxFx33 mtx;
} Field3DBgQuad;

struct Field3DBg {
    u16 width;
    u16 height;
    fx32 distance;
    u32 enabled;
    s32 texX;
    s32 texY;
    fx32 scaleX;
    fx32 scaleY;
    u16 angle;
    u8 polygonId;
    u8 alpha;
    Field3DBgTex tex;
    Field3DBgQuad quad;
};

static void func_ov036_02198d08(Field3DBgTex *tex, ArcTool *arc, const Field3DBgRes *res, u32 heapId);
static void func_ov036_02198d6c(Field3DBgTex *tex);
static void func_ov036_02198d84(Field3DBgTex *tex);
static u8 func_ov036_02198d9c(Field3DBgTex *tex);
static void func_ov036_02198da0(Field3DBgQuad *quad, const Field3DBg *bg, Field3DBgTex *tex, const MtxFx44 *proj);
static void func_ov036_02198e64(const Field3DBgQuad *quad);
static void func_ov036_02198f50(Field3DBg *bg);
static void func_ov036_02198f78(const Field3DBg *bg);

Field3DBg *func_ov036_02198bc0(const Field3DBgInit *init, u32 heapId) {
    Field3DBg *bg = GFL_HeapAllocate(heapId, sizeof(Field3DBg), TRUE, "field_3dbg.c", 149);

    bg->width = init->width;
    bg->height = init->height;
    bg->distance = init->distance;
    bg->polygonId = init->polygonId;
    bg->enabled = TRUE;
    return bg;
}

void func_ov036_02198bf0(Field3DBg *bg) {
    if (func_ov036_02198d9c(&bg->tex)) {
        func_ov036_02198d6c(&bg->tex);
    }
    GFL_HeapFree(bg);
}

void func_ov036_02198c10(Field3DBg *bg) {
    MtxFx44 proj;

    if (bg != NULL && bg->enabled != 0 && func_ov036_02198d9c(&bg->tex)) {
        NNS_G3dGeFlushBuffer();
        G3_MtxMode(GX_MTXMODE_PROJECTION);
        proj = NNS_G3dGlb.projMtx;
        G3_LoadMtx44(&proj);
        G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
        G3_Identity();
        G3_PushMtx();
        func_ov036_02198da0(&bg->quad, bg, &bg->tex, &proj);
        func_ov036_02198f50(bg);
        func_ov036_02198f78(bg);
        func_ov036_02198e64(&bg->quad);
        G3_PopMtx(1);
    }
}

void func_ov036_02198c8c(Field3DBg *bg) {
    if (bg != NULL) {
        func_ov036_02198d84(&bg->tex);
    }
}

void func_ov036_02198c9c(Field3DBg *bg) {
    if (func_ov036_02198d9c(&bg->tex)) {
        func_ov036_02198d6c(&bg->tex);
    }
}

void func_ov036_02198cb4(Field3DBg *bg, ArcTool *arc, const Field3DBgRes *res, u32 heapId) {
    if (func_ov036_02198d9c(&bg->tex)) {
        func_ov036_02198d6c(&bg->tex);
    }
    func_ov036_02198d08(&bg->tex, arc, res, heapId);
    bg->alpha = res->alpha;
    bg->scaleX = FX32_ONE;
    bg->scaleY = FX32_ONE;
    bg->angle = 0;
}

void func_ov036_02198cf0(Field3DBg *bg, u32 enabled) {
    bg->enabled = enabled;
}

void func_ov036_02198cf4(Field3DBg *bg, u8 polygonId) {
    bg->polygonId = polygonId;
}

void func_ov036_02198cf8(Field3DBg *bg, s32 texX) {
    bg->texX = texX;
}

void func_ov036_02198cfc(Field3DBg *bg, s32 texY) {
    bg->texY = texY;
}

void func_ov036_02198d00(Field3DBg *bg, fx32 scaleX) {
    bg->scaleX = scaleX;
}

void func_ov036_02198d04(Field3DBg *bg, fx32 scaleY) {
    bg->scaleY = scaleY;
}

static void func_ov036_02198d08(Field3DBgTex *tex, ArcTool *arc, const Field3DBgRes *res, u32 heapId) {
    tex->texParams[0] = res->texParams[0];
    tex->texParams[1] = res->texParams[1];
    tex->texParams[2] = res->texParams[2];
    tex->texParams[3] = res->texParams[3];
    tex->texParams[4] = res->texParams[4];
    tex->texParams[5] = res->texParams[5];
    tex->resource = GFL_HeapAllocate(heapId, GFL_G3DResGetAllocSize(), TRUE, "field_3dbg.c", 685);
    GFL_G3DResSetup(tex->resource, GFL_ArcToolReadHeapNew(arc, res->fileId, heapId));
    GFL_G3DResSetupTexData(tex->resource);
    tex->loaded = TRUE;
    tex->needsUpload = TRUE;
}

static void func_ov036_02198d6c(Field3DBgTex *tex) {
    GFL_G3DResFreeTexData(tex->resource);
    GFL_G3DResFree(tex->resource);
    tex->resource = NULL;
    tex->loaded = FALSE;
}

static void func_ov036_02198d84(Field3DBgTex *tex) {
    if (tex->needsUpload) {
        GFL_G3DResUploadTexDataCore(tex->resource);
        tex->needsUpload = FALSE;
    }
}

static u8 func_ov036_02198d9c(Field3DBgTex *tex) {
    return tex->loaded;
}

static void func_ov036_02198da0(Field3DBgQuad *quad, const Field3DBg *bg, Field3DBgTex *tex, const MtxFx44 *proj) {
    MtxFx33 scaling;
    fx32 u;
    fx32 v;

    quad->halfHeight = FX_Div(bg->distance, proj->m[1][1]);
    quad->halfWidth = FX_Div(bg->distance, proj->m[0][0]);
    quad->depth = bg->distance;
    if (quad->halfHeight > quad->halfWidth) {
        if (quad->halfHeight > quad->depth) {
            quad->scale = FX_Div(quad->halfHeight, FX32_CONST(4));
        } else {
            quad->scale = FX_Div(quad->depth, FX32_CONST(4));
        }
    } else if (quad->halfWidth > quad->depth) {
        quad->scale = FX_Div(quad->halfWidth, FX32_CONST(4));
    } else {
        quad->scale = FX_Div(quad->depth, FX32_CONST(4));
    }
    quad->halfWidth = FX_Div(quad->halfWidth, quad->scale);
    quad->halfHeight = FX_Div(quad->halfHeight, quad->scale);
    quad->depth = FX_Div(quad->depth, quad->scale);
    u = bg->texX << FX32_SHIFT;
    v = bg->texY << FX32_SHIFT;
    quad->texU0 = u;
    quad->texU1 = u + (bg->width << FX32_SHIFT);
    quad->texV0 = v;
    quad->texV1 = v + (bg->height << FX32_SHIFT);
    MTX_Identity33_(&quad->mtx);
    MTX_RotZ33_(&quad->mtx, FX_SinIdx(bg->angle), FX_CosIdx(bg->angle));
    MTX_Scale33_(&scaling, bg->scaleX, bg->scaleY, FX32_ONE);
    MTX_Concat33(&quad->mtx, &scaling, &quad->mtx);
}

static void func_ov036_02198e64(const Field3DBgQuad *quad) {
    G3_MtxMode(GX_MTXMODE_TEXTURE);
    G3_Identity();
    G3_MultMtx33(&quad->mtx);
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_Scale(quad->scale, quad->scale, quad->scale);
    G3_Begin(GX_BEGIN_QUADS);
    G3_TexCoord(quad->texU0, quad->texV0);
    G3_Vtx(-quad->halfWidth, quad->halfHeight, -quad->depth);
    G3_TexCoord(quad->texU0, quad->texV1);
    G3_Vtx(-quad->halfWidth, -quad->halfHeight, -quad->depth);
    G3_TexCoord(quad->texU1, quad->texV1);
    G3_Vtx(quad->halfWidth, -quad->halfHeight, -quad->depth);
    G3_TexCoord(quad->texU1, quad->texV0);
    G3_Vtx(quad->halfWidth, quad->halfHeight, -quad->depth);
    G3_End();
}

static void func_ov036_02198f50(Field3DBg *bg) {
    G3_MaterialColorDiffAmb(0x7fff, 0x7fff, TRUE);
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_BACK, bg->polygonId, bg->alpha,
                   GX_POLYGON_ATTR_MISC_FOG);
}

static void func_ov036_02198f78(const Field3DBg *bg) {
    G3_TexImageParam(bg->tex.texParams[4], GX_TEXGEN_TEXCOORD, bg->tex.texParams[0], bg->tex.texParams[1],
                     bg->tex.texParams[2], bg->tex.texParams[3], bg->tex.texParams[5],
                     NNS_GfdGetTexKeyAddr(GFL_G3DResGetTexVRAMHandle(bg->tex.resource)));
    G3_TexPlttBase(NNS_GfdGetPlttKeyAddr(GFL_G3DResGetPltVRAMHandle(bg->tex.resource)), bg->tex.texParams[4]);
}
