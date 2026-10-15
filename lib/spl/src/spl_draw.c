#include "types.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "spl/spl.h"
#include "spl_internal.h"

// The drawing of particles: billboards facing the camera, or polygons in the plane of the emitter's cross axes, both
// stretched along the particle's velocity if directional

// A unit quad about (x, y) in the xy plane, its texture repeated s and t times
static void SPLDraw_XYPlane(fx32 s, fx32 t, fx32 x, fx32 y) {
    G3_Begin(GX_BEGIN_QUADS);
    G3_TexCoord(0, 0);
    G3_Vtx10(x - FX32_ONE, y + FX32_ONE, 0);
    G3_TexCoord(s, 0);
    G3_Vtx10(x + FX32_ONE, y + FX32_ONE, 0);
    G3_TexCoord(s, t);
    G3_Vtx10(x + FX32_ONE, y - FX32_ONE, 0);
    G3_TexCoord(0, t);
    G3_Vtx10(x - FX32_ONE, y - FX32_ONE, 0);
    G3_End();
}

// The same in the xz plane
static void SPLDraw_XZPlane(fx32 s, fx32 t, fx32 x, fx32 z) {
    G3_Begin(GX_BEGIN_QUADS);
    G3_TexCoord(0, 0);
    G3_Vtx10(x - FX32_ONE, 0, z + FX32_ONE);
    G3_TexCoord(s, 0);
    G3_Vtx10(x + FX32_ONE, 0, z + FX32_ONE);
    G3_TexCoord(s, t);
    G3_Vtx10(x + FX32_ONE, 0, z - FX32_ONE);
    G3_TexCoord(0, t);
    G3_Vtx10(x - FX32_ONE, 0, z - FX32_ONE);
    G3_End();
}

// A rotation about the axis (1, 1, 1)
static void SPLDraw_RotateXYZ(fx32 sin, fx32 cos, MtxFx43 *mtx) {
    fx32 a = FX_MUL(FX32_ONE - cos, (fx64)(FX32_ONE / 3));
    fx32 b = FX_MUL(sin, (fx64)0x93d);
    fx32 sum = a + b;
    fx32 diff = a - b;

    a += cos;
    mtx->m[0][0] = a;
    mtx->m[1][0] = sum;
    mtx->m[2][0] = diff;
    mtx->m[3][0] = 0;
    mtx->m[0][1] = diff;
    mtx->m[1][1] = a;
    mtx->m[2][1] = sum;
    mtx->m[3][1] = 0;
    mtx->m[0][2] = sum;
    mtx->m[1][2] = diff;
    mtx->m[2][2] = a;
    mtx->m[3][2] = 0;
}

// A rotation about the y axis
static void SPLDraw_RotateY(fx32 sin, fx32 cos, MtxFx43 *mtx) {
    mtx->m[0][0] = cos;
    mtx->m[1][0] = 0;
    mtx->m[2][0] = sin;
    mtx->m[3][0] = 0;
    mtx->m[0][1] = 0;
    mtx->m[1][1] = FX32_ONE;
    mtx->m[2][1] = 0;
    mtx->m[3][1] = 0;
    mtx->m[0][2] = -sin;
    mtx->m[1][2] = 0;
    mtx->m[2][2] = cos;
    mtx->m[3][2] = 0;
}

// Indexed by the resource's polygon reference plane and rotation axis
static void (*sPlaneFuncs[])(fx32 s, fx32 t, fx32 x, fx32 y) = { SPLDraw_XYPlane, SPLDraw_XZPlane };
static void (*sRotateFuncs[])(fx32 sin, fx32 cos, MtxFx43 *mtx) = { SPLDraw_RotateY, SPLDraw_RotateXYZ };

void SPLDraw_Billboard(SPLManager *mgr, SPLParticle *particle) {
    SPLEmitter *emitter;
    SPLResourceHeader *header;
    int alpha;
    u8 scaleAnimDir;
    fx16 aspectRatio;
    fx32 animScale;
    const MtxFx43 *viewMatrix;
    fx32 scaleX;
    fx32 scaleY;
    GXRgb color;
    GXRgb emitterColor;
    fx32 sin, cos;
    VecFx32 pos;
    MtxFx43 mtx;

    emitter = mgr->drawEmitter;
    header = emitter->resource->header;
    aspectRatio = header->aspectRatio;
    scaleAnimDir = header->scaleAnimDir;
    alpha = (particle->baseAlpha * (particle->animAlpha + 1)) >> 5;
    emitterColor = emitter->color;
    animScale = particle->animScale;
    color = particle->color;
    viewMatrix = mgr->viewMatrix;
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, particle->polygonId, alpha,
                   mgr->polygonAttr);
    (void)reg_G3_POLYGON_ATTR;
    if (alpha == 0) {
        return;
    }

    scaleX = FX_Mul(particle->baseScale, aspectRatio);
    scaleY = particle->baseScale;
    if (scaleAnimDir == SPL_SCALE_ANIM_DIR_XY) {
        scaleX = FX_Mul(scaleX, animScale);
        scaleY = FX_MUL(scaleY, animScale);
    } else if (scaleAnimDir == SPL_SCALE_ANIM_DIR_X) {
        scaleX = FX_Mul(scaleX, animScale);
    } else {
        scaleY = FX_MUL(scaleY, animScale);
    }

    if (!mgr->drawEmitter->resource->header->flags.relativeToBasePos) {
        pos.x = particle->position.x + particle->emitterPos.x;
        pos.y = particle->position.y + particle->emitterPos.y;
        pos.z = particle->position.z + particle->emitterPos.z;
        MTX_MultVec43(&pos, viewMatrix, &pos);
        sin = FX_SinIdx(particle->rotation);
        cos = FX_CosIdx(particle->rotation);
        mtx.m[0][0] = FX_Mul(cos, scaleX);
        mtx.m[0][1] = FX_Mul(sin, scaleX);
        mtx.m[0][2] = 0;
        mtx.m[1][0] = FX_Mul(-sin, scaleY);
        mtx.m[1][1] = FX_Mul(cos, scaleY);
        mtx.m[1][2] = 0;
        mtx.m[2][0] = 0;
        mtx.m[2][1] = 0;
        mtx.m[2][2] = FX32_ONE;
        mtx.m[3][0] = pos.x;
        mtx.m[3][1] = pos.y;
        mtx.m[3][2] = pos.z;
        G3_Identity();
        gfxMultMatrix4x3(&mtx);
    } else {
        pos.x = particle->position.x + particle->emitterPos.x - mgr->drawEmitter->resource->header->emitterBasePos.x;
        pos.y = particle->position.y + particle->emitterPos.y - mgr->drawEmitter->resource->header->emitterBasePos.y;
        pos.z = particle->position.z + particle->emitterPos.z - mgr->drawEmitter->resource->header->emitterBasePos.z;
        MTX_MultVec43(&pos, viewMatrix, &pos);
        sin = FX_SinIdx(particle->rotation);
        cos = FX_CosIdx(particle->rotation);
        mtx.m[0][0] = FX_Mul(cos, scaleX);
        mtx.m[0][1] = FX_Mul(sin, scaleX);
        mtx.m[0][2] = 0;
        mtx.m[1][0] = FX_Mul(-sin, scaleY);
        mtx.m[1][1] = FX_Mul(cos, scaleY);
        mtx.m[1][2] = 0;
        mtx.m[2][0] = 0;
        mtx.m[2][1] = 0;
        mtx.m[2][2] = FX32_ONE;
        mtx.m[3][0] = pos.x;
        mtx.m[3][1] = pos.y;
        mtx.m[3][2] = pos.z;
        G3_Identity();
        G3_Translate(mgr->drawEmitter->resource->header->emitterBasePos.x,
                     mgr->drawEmitter->resource->header->emitterBasePos.y,
                     mgr->drawEmitter->resource->header->emitterBasePos.z);
        gfxMultMatrix4x3(&mtx);
    }

    // The particle's color, tinted by the emitter's
    G3_Color(GX_RGB(((color & 0x1f) * (emitterColor & 0x1f)) >> 5, ((color & 0x3e0) * (emitterColor & 0x3e0)) >> 15,
                    ((color & 0x7c00) * (emitterColor & 0x7c00)) >> 25));
    SPLDraw_XYPlane(mgr->drawEmitter->textureS, mgr->drawEmitter->textureT,
                    mgr->drawEmitter->resource->header->polygonX, mgr->drawEmitter->resource->header->polygonY);
}
void SPLDraw_Child_Billboard(SPLManager *mgr, SPLParticle *particle) {
    SPLResourceHeader *header = mgr->drawEmitter->resource->header;
    fx16 aspectRatio = header->aspectRatio;
    int alpha = (particle->baseAlpha * (particle->animAlpha + 1)) >> 5;
    const MtxFx43 *viewMatrix = mgr->viewMatrix;
    fx32 scaleX, scaleY;
    fx32 sin, cos;
    VecFx32 pos;
    MtxFx43 mtx;

    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, particle->polygonId, alpha,
                   mgr->polygonAttr);
    (void)reg_G3_POLYGON_ATTR;
    if (alpha == 0) {
        return;
    }

    scaleX = FX_Mul(particle->baseScale, aspectRatio);
    scaleY = particle->baseScale;
    switch (mgr->drawEmitter->resource->header->scaleAnimDir) {
    case SPL_SCALE_ANIM_DIR_XY:
        scaleX = FX_MUL(scaleX, particle->animScale);
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_X:
        scaleX = FX_MUL(scaleX, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_Y:
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    }

    if (!mgr->drawEmitter->resource->header->flags.relativeToBasePos) {
        pos.x = particle->position.x + particle->emitterPos.x;
        pos.y = particle->position.y + particle->emitterPos.y;
        pos.z = particle->position.z + particle->emitterPos.z;
        MTX_MultVec43(&pos, viewMatrix, &pos);
        sin = FX_SinIdx(particle->rotation);
        cos = FX_CosIdx(particle->rotation);
        mtx.m[0][0] = FX_Mul(cos, scaleX);
        mtx.m[0][1] = FX_Mul(sin, scaleX);
        mtx.m[0][2] = 0;
        mtx.m[1][0] = FX_Mul(-sin, scaleY);
        mtx.m[1][1] = FX_Mul(cos, scaleY);
        mtx.m[1][2] = 0;
        mtx.m[2][0] = 0;
        mtx.m[2][1] = 0;
        mtx.m[2][2] = FX32_ONE;
        mtx.m[3][0] = pos.x;
        mtx.m[3][1] = pos.y;
        mtx.m[3][2] = pos.z;
        G3_Identity();
        gfxMultMatrix4x3(&mtx);
    } else {
        pos.x = particle->position.x + particle->emitterPos.x - mgr->drawEmitter->resource->header->emitterBasePos.x;
        pos.y = particle->position.y + particle->emitterPos.y - mgr->drawEmitter->resource->header->emitterBasePos.y;
        pos.z = particle->position.z + particle->emitterPos.z - mgr->drawEmitter->resource->header->emitterBasePos.z;
        MTX_MultVec43(&pos, viewMatrix, &pos);
        sin = FX_SinIdx(particle->rotation);
        cos = FX_CosIdx(particle->rotation);
        mtx.m[0][0] = FX_Mul(cos, scaleX);
        mtx.m[0][1] = FX_Mul(sin, scaleX);
        mtx.m[0][2] = 0;
        mtx.m[1][0] = FX_Mul(-sin, scaleY);
        mtx.m[1][1] = FX_Mul(cos, scaleY);
        mtx.m[1][2] = 0;
        mtx.m[2][0] = 0;
        mtx.m[2][1] = 0;
        mtx.m[2][2] = FX32_ONE;
        mtx.m[3][0] = pos.x;
        mtx.m[3][1] = pos.y;
        mtx.m[3][2] = pos.z;
        G3_Identity();
        G3_Translate(mgr->drawEmitter->resource->header->emitterBasePos.x,
                     mgr->drawEmitter->resource->header->emitterBasePos.y,
                     mgr->drawEmitter->resource->header->emitterBasePos.z);
        gfxMultMatrix4x3(&mtx);
    }

    G3_Color(GX_RGB(((particle->color & 0x1f) * (mgr->drawEmitter->color & 0x1f)) >> 5,
                    ((particle->color & 0x3e0) * (mgr->drawEmitter->color & 0x3e0)) >> 15,
                    ((particle->color & 0x7c00) * (mgr->drawEmitter->color & 0x7c00)) >> 25));
    SPLDraw_XYPlane(mgr->drawEmitter->childTextureS, mgr->drawEmitter->childTextureT, 0, 0);
}

// A billboard turned to the direction the particle moves on screen, and stretched along it the more it moves across
// the view
void SPLDraw_DirBillboard(SPLManager *mgr, SPLParticle *particle) {
    SPLResourceHeader *header = mgr->drawEmitter->resource->header;
    fx16 aspectRatio = header->aspectRatio;
    int alpha = (particle->baseAlpha * (particle->animAlpha + 1)) >> 5;
    const MtxFx43 *viewMatrix = mgr->viewMatrix;
    fx32 scaleX, scaleY;
    fx32 dot;
    VecFx32 vel;
    VecFx32 pos;
    VecFx32 dir;
    VecFx32 viewZ;
    MtxFx33 rot;
    MtxFx43 mtx;

    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, particle->polygonId, alpha,
                   mgr->polygonAttr);
    (void)reg_G3_POLYGON_ATTR;
    if (alpha == 0) {
        return;
    }

    scaleX = FX_Mul(particle->baseScale, aspectRatio);
    scaleY = particle->baseScale;
    switch (mgr->drawEmitter->resource->header->scaleAnimDir) {
    case SPL_SCALE_ANIM_DIR_XY:
        scaleX = FX_MUL(scaleX, particle->animScale);
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_X:
        scaleX = FX_MUL(scaleX, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_Y:
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    }

    if (!mgr->drawEmitter->resource->header->flags.relativeToBasePos) {
        pos.x = particle->position.x + particle->emitterPos.x;
        pos.y = particle->position.y + particle->emitterPos.y;
        pos.z = particle->position.z + particle->emitterPos.z;

        // The direction on screen, across the view's z axis
        dir = particle->velocity;
        viewZ.x = viewMatrix->m[0][2];
        viewZ.y = viewMatrix->m[1][2];
        viewZ.z = viewMatrix->m[2][2];
        VEC_CrossProduct(&dir, &viewZ, &dir);
        if (dir.x == 0 && dir.y == 0 && dir.z == 0) {
            return;
        }
        VEC_Normalize(&dir, &dir);
        MI_Copy36B(viewMatrix, &rot);
        MTX_MultVec33(&dir, &rot, &dir);
        MTX_MultVec43(&pos, viewMatrix, &pos);

        vel = particle->velocity;
        VEC_Normalize(&vel, &vel);
        dot = FX_Mul(vel.z, -viewMatrix->m[2][2]) +
              (FX_Mul(vel.x, -viewMatrix->m[0][2]) + FX_Mul(vel.y, -viewMatrix->m[1][2]));
        if (dot < 0) {
            dot = -dot;
        }
        scaleY = FX_Mul(scaleY, FX_Mul(FX32_ONE - dot, mgr->drawEmitter->resource->header->dirStretch) + FX32_ONE);

        mtx.m[0][0] = FX_Mul(dir.x, scaleX);
        mtx.m[1][0] = FX_Mul(-dir.y, scaleY);
        mtx.m[2][0] = 0;
        mtx.m[3][0] = pos.x;
        mtx.m[0][1] = FX_Mul(dir.y, scaleX);
        mtx.m[1][1] = FX_Mul(dir.x, scaleY);
        mtx.m[2][1] = 0;
        mtx.m[3][1] = pos.y;
        mtx.m[0][2] = 0;
        mtx.m[1][2] = 0;
        mtx.m[2][2] = FX32_ONE;
        mtx.m[3][2] = pos.z;
        G3_Identity();
        gfxMultMatrix4x3(&mtx);
    } else {
        pos.x = particle->position.x + particle->emitterPos.x - mgr->drawEmitter->resource->header->emitterBasePos.x;
        pos.y = particle->position.y + particle->emitterPos.y - mgr->drawEmitter->resource->header->emitterBasePos.y;
        pos.z = particle->position.z + particle->emitterPos.z - mgr->drawEmitter->resource->header->emitterBasePos.z;

        dir = particle->velocity;
        viewZ.x = viewMatrix->m[0][2];
        viewZ.y = viewMatrix->m[1][2];
        viewZ.z = viewMatrix->m[2][2];
        VEC_CrossProduct(&dir, &viewZ, &dir);
        if (dir.x == 0 && dir.y == 0 && dir.z == 0) {
            return;
        }
        VEC_Normalize(&dir, &dir);
        MI_Copy36B(viewMatrix, &rot);
        MTX_MultVec33(&dir, &rot, &dir);
        MTX_MultVec43(&pos, viewMatrix, &pos);

        vel = particle->velocity;
        VEC_Normalize(&vel, &vel);
        dot = FX_Mul(vel.z, -viewMatrix->m[2][2]) +
              (FX_Mul(vel.x, -viewMatrix->m[0][2]) + FX_Mul(vel.y, -viewMatrix->m[1][2]));
        if (dot < 0) {
            dot = -dot;
        }
        scaleY = FX_Mul(scaleY, FX_Mul(FX32_ONE - dot, mgr->drawEmitter->resource->header->dirStretch) + FX32_ONE);

        mtx.m[0][0] = FX_Mul(dir.x, scaleX);
        mtx.m[1][0] = FX_Mul(-dir.y, scaleY);
        mtx.m[2][0] = 0;
        mtx.m[3][0] = pos.x;
        mtx.m[0][1] = FX_Mul(dir.y, scaleX);
        mtx.m[1][1] = FX_Mul(dir.x, scaleY);
        mtx.m[2][1] = 0;
        mtx.m[3][1] = pos.y;
        mtx.m[0][2] = 0;
        mtx.m[1][2] = 0;
        mtx.m[2][2] = FX32_ONE;
        mtx.m[3][2] = pos.z;
        G3_Identity();
        G3_Translate(mgr->drawEmitter->resource->header->emitterBasePos.x,
                     mgr->drawEmitter->resource->header->emitterBasePos.y,
                     mgr->drawEmitter->resource->header->emitterBasePos.z);
        gfxMultMatrix4x3(&mtx);
    }

    G3_Color(GX_RGB(((particle->color & 0x1f) * (mgr->drawEmitter->color & 0x1f)) >> 5,
                    ((particle->color & 0x3e0) * (mgr->drawEmitter->color & 0x3e0)) >> 15,
                    ((particle->color & 0x7c00) * (mgr->drawEmitter->color & 0x7c00)) >> 25));
    SPLDraw_XYPlane(mgr->drawEmitter->textureS, mgr->drawEmitter->textureT,
                    mgr->drawEmitter->resource->header->polygonX, mgr->drawEmitter->resource->header->polygonY);
}

void SPLDraw_Child_DirBillboard(SPLManager *mgr, SPLParticle *particle) {
    SPLResourceHeader *header = mgr->drawEmitter->resource->header;
    fx16 aspectRatio = header->aspectRatio;
    int alpha = (particle->baseAlpha * (particle->animAlpha + 1)) >> 5;
    const MtxFx43 *viewMatrix = mgr->viewMatrix;
    fx32 scaleX, scaleY;
    fx32 dot;
    VecFx32 vel;
    VecFx32 pos;
    VecFx32 dir;
    VecFx32 viewZ;
    MtxFx33 rot;
    MtxFx43 mtx;

    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, particle->polygonId, alpha,
                   mgr->polygonAttr);
    (void)reg_G3_POLYGON_ATTR;
    if (alpha == 0) {
        return;
    }

    scaleX = FX_Mul(particle->baseScale, aspectRatio);
    scaleY = particle->baseScale;
    switch (mgr->drawEmitter->resource->header->scaleAnimDir) {
    case SPL_SCALE_ANIM_DIR_XY:
        scaleX = FX_MUL(scaleX, particle->animScale);
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_X:
        scaleX = FX_MUL(scaleX, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_Y:
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    }

    if (!mgr->drawEmitter->resource->header->flags.relativeToBasePos) {
        pos.x = particle->position.x + particle->emitterPos.x;
        pos.y = particle->position.y + particle->emitterPos.y;
        pos.z = particle->position.z + particle->emitterPos.z;

        // The direction on screen, across the view's z axis
        dir = particle->velocity;
        viewZ.x = viewMatrix->m[0][2];
        viewZ.y = viewMatrix->m[1][2];
        viewZ.z = viewMatrix->m[2][2];
        VEC_CrossProduct(&dir, &viewZ, &dir);
        if (dir.x == 0 && dir.y == 0 && dir.z == 0) {
            return;
        }
        VEC_Normalize(&dir, &dir);
        MI_Copy36B(viewMatrix, &rot);
        MTX_MultVec33(&dir, &rot, &dir);
        MTX_MultVec43(&pos, viewMatrix, &pos);

        vel = particle->velocity;
        VEC_Normalize(&vel, &vel);
        dot = FX_Mul(vel.z, -viewMatrix->m[2][2]) +
              (FX_Mul(vel.x, -viewMatrix->m[0][2]) + FX_Mul(vel.y, -viewMatrix->m[1][2]));
        if (dot < 0) {
            dot = -dot;
        }
        scaleY = FX_Mul(scaleY, FX_Mul(FX32_ONE - dot, mgr->drawEmitter->resource->header->dirStretch) + FX32_ONE);

        mtx.m[0][0] = FX_Mul(dir.x, scaleX);
        mtx.m[1][0] = FX_Mul(-dir.y, scaleY);
        mtx.m[2][0] = 0;
        mtx.m[3][0] = pos.x;
        mtx.m[0][1] = FX_Mul(dir.y, scaleX);
        mtx.m[1][1] = FX_Mul(dir.x, scaleY);
        mtx.m[2][1] = 0;
        mtx.m[3][1] = pos.y;
        mtx.m[0][2] = 0;
        mtx.m[1][2] = 0;
        mtx.m[2][2] = FX32_ONE;
        mtx.m[3][2] = pos.z;
        G3_Identity();
        gfxMultMatrix4x3(&mtx);
    } else {
        pos.x = particle->position.x + particle->emitterPos.x - mgr->drawEmitter->resource->header->emitterBasePos.x;
        pos.y = particle->position.y + particle->emitterPos.y - mgr->drawEmitter->resource->header->emitterBasePos.y;
        pos.z = particle->position.z + particle->emitterPos.z - mgr->drawEmitter->resource->header->emitterBasePos.z;

        dir = particle->velocity;
        viewZ.x = viewMatrix->m[0][2];
        viewZ.y = viewMatrix->m[1][2];
        viewZ.z = viewMatrix->m[2][2];
        VEC_CrossProduct(&dir, &viewZ, &dir);
        if (dir.x == 0 && dir.y == 0 && dir.z == 0) {
            return;
        }
        VEC_Normalize(&dir, &dir);
        MI_Copy36B(viewMatrix, &rot);
        MTX_MultVec33(&dir, &rot, &dir);
        MTX_MultVec43(&pos, viewMatrix, &pos);

        vel = particle->velocity;
        VEC_Normalize(&vel, &vel);
        dot = FX_Mul(vel.z, -viewMatrix->m[2][2]) +
              (FX_Mul(vel.x, -viewMatrix->m[0][2]) + FX_Mul(vel.y, -viewMatrix->m[1][2]));
        if (dot < 0) {
            dot = -dot;
        }
        scaleY = FX_Mul(scaleY, FX_Mul(FX32_ONE - dot, mgr->drawEmitter->resource->header->dirStretch) + FX32_ONE);

        mtx.m[0][0] = FX_Mul(dir.x, scaleX);
        mtx.m[1][0] = FX_Mul(-dir.y, scaleY);
        mtx.m[2][0] = 0;
        mtx.m[3][0] = pos.x;
        mtx.m[0][1] = FX_Mul(dir.y, scaleX);
        mtx.m[1][1] = FX_Mul(dir.x, scaleY);
        mtx.m[2][1] = 0;
        mtx.m[3][1] = pos.y;
        mtx.m[0][2] = 0;
        mtx.m[1][2] = 0;
        mtx.m[2][2] = FX32_ONE;
        mtx.m[3][2] = pos.z;
        G3_Identity();
        G3_Translate(mgr->drawEmitter->resource->header->emitterBasePos.x,
                     mgr->drawEmitter->resource->header->emitterBasePos.y,
                     mgr->drawEmitter->resource->header->emitterBasePos.z);
        gfxMultMatrix4x3(&mtx);
    }

    G3_Color(GX_RGB(((particle->color & 0x1f) * (mgr->drawEmitter->color & 0x1f)) >> 5,
                    ((particle->color & 0x3e0) * (mgr->drawEmitter->color & 0x3e0)) >> 15,
                    ((particle->color & 0x7c00) * (mgr->drawEmitter->color & 0x7c00)) >> 25));
    SPLDraw_XYPlane(mgr->drawEmitter->childTextureS, mgr->drawEmitter->childTextureT, 0, 0);
}

// A polygon in the reference plane, rotated about the resource's axis and placed in view space
void SPLDraw_Polygon(SPLManager *mgr, SPLParticle *particle) {
    int alpha = (particle->baseAlpha * (particle->animAlpha + 1)) >> 5;
    fx32 scaleX, scaleY;
    MtxFx43 mtx;
    MtxFx43 rot;
    MtxFx43 scale;

    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, particle->polygonId, alpha,
                   mgr->polygonAttr);
    (void)reg_G3_POLYGON_ATTR;
    if (alpha == 0) {
        return;
    }

    sRotateFuncs[mgr->drawEmitter->resource->header->flags.polygonRotAxis](FX_SinIdx(particle->rotation),
                                                                           FX_CosIdx(particle->rotation), &rot);
    scaleX = FX_Mul(particle->baseScale, mgr->drawEmitter->resource->header->aspectRatio);
    scaleY = particle->baseScale;
    switch (mgr->drawEmitter->resource->header->scaleAnimDir) {
    case SPL_SCALE_ANIM_DIR_XY:
        scaleX = FX_MUL(scaleX, particle->animScale);
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_X:
        scaleX = FX_MUL(scaleX, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_Y:
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    }
    MTX_Scale43_(&scale, scaleX, scaleY, scaleY);
    MTX_Concat43(&scale, &rot, &mtx);

    if (!mgr->drawEmitter->resource->header->flags.relativeToBasePos) {
        mtx.m[3][0] = particle->position.x + particle->emitterPos.x;
        mtx.m[3][1] = particle->position.y + particle->emitterPos.y;
        mtx.m[3][2] = particle->position.z + particle->emitterPos.z;
        gfxLoadMatrix4x3(mgr->viewMatrix);
        gfxMultMatrix4x3(&mtx);
    } else {
        mtx.m[3][0] =
            particle->position.x + particle->emitterPos.x - mgr->drawEmitter->resource->header->emitterBasePos.x;
        mtx.m[3][1] =
            particle->position.y + particle->emitterPos.y - mgr->drawEmitter->resource->header->emitterBasePos.y;
        mtx.m[3][2] =
            particle->position.z + particle->emitterPos.z - mgr->drawEmitter->resource->header->emitterBasePos.z;
        G3_Identity();
        G3_Translate(mgr->drawEmitter->resource->header->emitterBasePos.x,
                     mgr->drawEmitter->resource->header->emitterBasePos.y,
                     mgr->drawEmitter->resource->header->emitterBasePos.z);
        gfxMultMatrix4x3(mgr->viewMatrix);
        gfxMultMatrix4x3(&mtx);
    }

    G3_Color(GX_RGB(((particle->color & 0x1f) * (mgr->drawEmitter->color & 0x1f)) >> 5,
                    ((particle->color & 0x3e0) * (mgr->drawEmitter->color & 0x3e0)) >> 15,
                    ((particle->color & 0x7c00) * (mgr->drawEmitter->color & 0x7c00)) >> 25));
    sPlaneFuncs[mgr->drawEmitter->resource->header->flags.polygonReferencePlane](
        mgr->drawEmitter->textureS, mgr->drawEmitter->textureT, mgr->drawEmitter->resource->header->polygonX,
        mgr->drawEmitter->resource->header->polygonY);
}

void SPLDraw_Child_Polygon(SPLManager *mgr, SPLParticle *particle) {
    int alpha = (particle->baseAlpha * (particle->animAlpha + 1)) >> 5;
    fx32 scaleX, scaleY;
    MtxFx43 mtx;
    MtxFx43 rot;
    MtxFx43 scale;

    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, particle->polygonId, alpha,
                   mgr->polygonAttr);
    (void)reg_G3_POLYGON_ATTR;
    if (alpha == 0) {
        return;
    }

    sRotateFuncs[mgr->drawEmitter->resource->childResource->polygonRotAxis](FX_SinIdx(particle->rotation),
                                                                            FX_CosIdx(particle->rotation), &rot);
    scaleX = FX_Mul(particle->baseScale, mgr->drawEmitter->resource->header->aspectRatio);
    scaleY = particle->baseScale;
    switch (mgr->drawEmitter->resource->header->scaleAnimDir) {
    case SPL_SCALE_ANIM_DIR_XY:
        scaleX = FX_MUL(scaleX, particle->animScale);
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_X:
        scaleX = FX_MUL(scaleX, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_Y:
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    }
    MTX_Scale43_(&scale, scaleX, scaleY, scaleY);
    MTX_Concat43(&rot, &scale, &mtx);

    if (!mgr->drawEmitter->resource->header->flags.relativeToBasePos) {
        mtx.m[3][0] = particle->position.x + particle->emitterPos.x;
        mtx.m[3][1] = particle->position.y + particle->emitterPos.y;
        mtx.m[3][2] = particle->position.z + particle->emitterPos.z;
        gfxLoadMatrix4x3(mgr->viewMatrix);
        gfxMultMatrix4x3(&mtx);
    } else {
        mtx.m[3][0] =
            particle->position.x + particle->emitterPos.x - mgr->drawEmitter->resource->header->emitterBasePos.x;
        mtx.m[3][1] =
            particle->position.y + particle->emitterPos.y - mgr->drawEmitter->resource->header->emitterBasePos.y;
        mtx.m[3][2] =
            particle->position.z + particle->emitterPos.z - mgr->drawEmitter->resource->header->emitterBasePos.z;
        G3_Identity();
        G3_Translate(mgr->drawEmitter->resource->header->emitterBasePos.x,
                     mgr->drawEmitter->resource->header->emitterBasePos.y,
                     mgr->drawEmitter->resource->header->emitterBasePos.z);
        gfxMultMatrix4x3(mgr->viewMatrix);
        gfxMultMatrix4x3(&mtx);
    }

    G3_Color(GX_RGB(((particle->color & 0x1f) * (mgr->drawEmitter->color & 0x1f)) >> 5,
                    ((particle->color & 0x3e0) * (mgr->drawEmitter->color & 0x3e0)) >> 15,
                    ((particle->color & 0x7c00) * (mgr->drawEmitter->color & 0x7c00)) >> 25));
    sPlaneFuncs[mgr->drawEmitter->resource->childResource->polygonReferencePlane](
        mgr->drawEmitter->childTextureS, mgr->drawEmitter->childTextureT, 0, 0);
}

// A polygon turned to face along the particle's direction, then as SPLDraw_Polygon
void SPLDraw_DirPolygon(SPLManager *mgr, SPLParticle *particle) {
    int alpha = (particle->baseAlpha * (particle->animAlpha + 1)) >> 5;
    fx32 scaleX, scaleY;
    fx32 dot;
    MtxFx43 mtx;
    MtxFx43 rot;
    MtxFx43 scale;
    VecFx32 dir;
    VecFx32 side;
    VecFx32 normal;
    VecFx32 up;
    MtxFx43 dirMtx;

    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, particle->polygonId, alpha,
                   mgr->polygonAttr);
    (void)reg_G3_POLYGON_ATTR;
    if (alpha == 0) {
        return;
    }

    sRotateFuncs[mgr->drawEmitter->resource->header->flags.polygonRotAxis](FX_SinIdx(particle->rotation),
                                                                           FX_CosIdx(particle->rotation), &rot);

    // A frame with its y axis along the direction
    MTX_Identity43_(&dirMtx);
    if (!mgr->drawEmitter->resource->header->dirFromPosition) {
        VEC_Normalize(&particle->velocity, &dir);
    } else {
        VEC_Normalize(&particle->position, &dir);
        dir.x = -dir.x;
        dir.y = -dir.y;
        dir.z = -dir.z;
    }
    up.x = 0;
    up.y = FX32_ONE;
    up.z = 0;
    dot = VEC_DotProduct(&dir, &up);
    if (dot > FX32_CONST(0.8) || dot < FX32_CONST(-0.8)) {
        up.x = FX32_ONE;
        up.y = 0;
        up.z = 0;
    }
    VEC_CrossProduct(&dir, &up, &side);
    VEC_CrossProduct(&dir, &side, &normal);
    dirMtx.m[0][0] = side.x;
    dirMtx.m[0][1] = side.y;
    dirMtx.m[0][2] = side.z;
    dirMtx.m[1][0] = dir.x;
    dirMtx.m[1][1] = dir.y;
    dirMtx.m[1][2] = dir.z;
    dirMtx.m[2][0] = normal.x;
    dirMtx.m[2][1] = normal.y;
    dirMtx.m[2][2] = normal.z;
    MTX_Concat43(&rot, &dirMtx, &rot);

    scaleX = FX_Mul(particle->baseScale, mgr->drawEmitter->resource->header->aspectRatio);
    scaleY = particle->baseScale;
    switch (mgr->drawEmitter->resource->header->scaleAnimDir) {
    case SPL_SCALE_ANIM_DIR_XY:
        scaleX = FX_MUL(scaleX, particle->animScale);
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_X:
        scaleX = FX_MUL(scaleX, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_Y:
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    }
    MTX_Scale43_(&scale, scaleX, scaleY, scaleY);
    MTX_Concat43(&scale, &rot, &mtx);

    if (!mgr->drawEmitter->resource->header->flags.relativeToBasePos) {
        mtx.m[3][0] = particle->position.x + particle->emitterPos.x;
        mtx.m[3][1] = particle->position.y + particle->emitterPos.y;
        mtx.m[3][2] = particle->position.z + particle->emitterPos.z;
        gfxLoadMatrix4x3(mgr->viewMatrix);
        gfxMultMatrix4x3(&mtx);
    } else {
        mtx.m[3][0] =
            particle->position.x + particle->emitterPos.x - mgr->drawEmitter->resource->header->emitterBasePos.x;
        mtx.m[3][1] =
            particle->position.y + particle->emitterPos.y - mgr->drawEmitter->resource->header->emitterBasePos.y;
        mtx.m[3][2] =
            particle->position.z + particle->emitterPos.z - mgr->drawEmitter->resource->header->emitterBasePos.z;
        G3_Identity();
        G3_Translate(mgr->drawEmitter->resource->header->emitterBasePos.x,
                     mgr->drawEmitter->resource->header->emitterBasePos.y,
                     mgr->drawEmitter->resource->header->emitterBasePos.z);
        gfxMultMatrix4x3(mgr->viewMatrix);
        gfxMultMatrix4x3(&mtx);
    }

    G3_Color(GX_RGB(((particle->color & 0x1f) * (mgr->drawEmitter->color & 0x1f)) >> 5,
                    ((particle->color & 0x3e0) * (mgr->drawEmitter->color & 0x3e0)) >> 15,
                    ((particle->color & 0x7c00) * (mgr->drawEmitter->color & 0x7c00)) >> 25));
    sPlaneFuncs[mgr->drawEmitter->resource->header->flags.polygonReferencePlane](
        mgr->drawEmitter->textureS, mgr->drawEmitter->textureT, mgr->drawEmitter->resource->header->polygonX,
        mgr->drawEmitter->resource->header->polygonY);
}

void SPLDraw_Child_DirPolygon(SPLManager *mgr, SPLParticle *particle) {
    int alpha = (particle->baseAlpha * (particle->animAlpha + 1)) >> 5;
    fx32 scaleX, scaleY;
    fx32 dot;
    MtxFx43 mtx;
    MtxFx43 rot;
    MtxFx43 scale;
    VecFx32 dir;
    VecFx32 side;
    VecFx32 normal;
    VecFx32 up;
    MtxFx43 dirMtx;

    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, particle->polygonId, alpha,
                   mgr->polygonAttr);
    (void)reg_G3_POLYGON_ATTR;
    if (alpha == 0) {
        return;
    }

    sRotateFuncs[mgr->drawEmitter->resource->childResource->polygonRotAxis](FX_SinIdx(particle->rotation),
                                                                            FX_CosIdx(particle->rotation), &rot);

    // A frame with its y axis along the direction
    MTX_Identity43_(&dirMtx);
    if (!mgr->drawEmitter->resource->childResource->dirFromPosition) {
        VEC_Normalize(&particle->velocity, &dir);
    } else {
        VEC_Normalize(&particle->position, &dir);
        dir.x = -dir.x;
        dir.y = -dir.y;
        dir.z = -dir.z;
    }
    up.x = 0;
    up.y = FX32_ONE;
    up.z = 0;
    dot = VEC_DotProduct(&dir, &up);
    if (dot > FX32_CONST(0.8) || dot < FX32_CONST(-0.8)) {
        up.x = FX32_ONE;
        up.y = 0;
        up.z = 0;
    }
    VEC_CrossProduct(&dir, &up, &side);
    VEC_CrossProduct(&dir, &side, &normal);
    dirMtx.m[0][0] = side.x;
    dirMtx.m[0][1] = side.y;
    dirMtx.m[0][2] = side.z;
    dirMtx.m[1][0] = dir.x;
    dirMtx.m[1][1] = dir.y;
    dirMtx.m[1][2] = dir.z;
    dirMtx.m[2][0] = normal.x;
    dirMtx.m[2][1] = normal.y;
    dirMtx.m[2][2] = normal.z;
    MTX_Concat43(&rot, &dirMtx, &rot);

    scaleX = FX_Mul(particle->baseScale, mgr->drawEmitter->resource->header->aspectRatio);
    scaleY = particle->baseScale;
    switch (mgr->drawEmitter->resource->header->scaleAnimDir) {
    case SPL_SCALE_ANIM_DIR_XY:
        scaleX = FX_MUL(scaleX, particle->animScale);
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_X:
        scaleX = FX_MUL(scaleX, particle->animScale);
        break;
    case SPL_SCALE_ANIM_DIR_Y:
        scaleY = FX_MUL(scaleY, particle->animScale);
        break;
    }
    MTX_Scale43_(&scale, scaleX, scaleY, scaleY);
    MTX_Concat43(&rot, &scale, &mtx);

    if (!mgr->drawEmitter->resource->header->flags.relativeToBasePos) {
        mtx.m[3][0] = particle->position.x + particle->emitterPos.x;
        mtx.m[3][1] = particle->position.y + particle->emitterPos.y;
        mtx.m[3][2] = particle->position.z + particle->emitterPos.z;
        gfxLoadMatrix4x3(mgr->viewMatrix);
        gfxMultMatrix4x3(&mtx);
    } else {
        mtx.m[3][0] =
            particle->position.x + particle->emitterPos.x - mgr->drawEmitter->resource->header->emitterBasePos.x;
        mtx.m[3][1] =
            particle->position.y + particle->emitterPos.y - mgr->drawEmitter->resource->header->emitterBasePos.y;
        mtx.m[3][2] =
            particle->position.z + particle->emitterPos.z - mgr->drawEmitter->resource->header->emitterBasePos.z;
        G3_Identity();
        G3_Translate(mgr->drawEmitter->resource->header->emitterBasePos.x,
                     mgr->drawEmitter->resource->header->emitterBasePos.y,
                     mgr->drawEmitter->resource->header->emitterBasePos.z);
        gfxMultMatrix4x3(mgr->viewMatrix);
        gfxMultMatrix4x3(&mtx);
    }

    G3_Color(GX_RGB(((particle->color & 0x1f) * (mgr->drawEmitter->color & 0x1f)) >> 5,
                    ((particle->color & 0x3e0) * (mgr->drawEmitter->color & 0x3e0)) >> 15,
                    ((particle->color & 0x7c00) * (mgr->drawEmitter->color & 0x7c00)) >> 25));
    sPlaneFuncs[mgr->drawEmitter->resource->childResource->polygonReferencePlane](
        mgr->drawEmitter->childTextureS, mgr->drawEmitter->childTextureT, 0, 0);
}
