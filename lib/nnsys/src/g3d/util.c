#include "nnsys/g3d.h"

// NitroSystem's G3D utilities (util.c): reading the geometry engine's matrices back, starting G3D, and the
// conversions between the world and the screen through the global state's camera and projection matrices. The
// functions have NitroSystem's names, and g3d.h notes swan's

// Gets the position matrix and the vector matrix that the geometry engine is at, each unless its pointer is null. The
// engine gives only the clip matrix, which is the projection matrix times the position matrix, so the projection
// matrix is pushed and replaced by the identity first
void NNS_G3dGetCurrentMtx(MtxFx43 *pos, MtxFx33 *nrm) {
    MtxFx44 clip;

    NNS_G3dGeFlushBuffer();
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_PushMtx();
    G3_Identity();
    if (pos != NULL) {
        while (G3X_GetClipMtx(&clip) != 0) {
        }
        MTX_Copy44To43_(&clip, pos);
    }
    if (nrm != NULL) {
        while (G3X_GetVectorMtx(nrm) != 0) {
        }
    }
    G3_PopMtx(1);
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
}

void NNS_G3dInit(void) {
    G3X_Init();
    NNS_G3DRenderStateInit();
    G3X_SetFifoIntrCond(GX_FIFOINTR_COND_EMPTY);
}

// Where a point of the world is on the screen, in the viewport of the global state, and whether it is within it: 0
// when it is, or -1. The point goes through the camera matrix and then the projection matrix, and its x and y are
// divided by the w that comes out (the divider starts as soon as w is known). They become the fractions of the
// viewport's width and height that the point is at
int NNS_G3dWorldPosToScrPos(const VecFx32 *world, int *x, int *y) {
    const MtxFx44 *proj = &NNS_G3dGlb.projMtx;
    VecFx32 view;
    fx32 w;
    fx32 clipX;
    fx32 clipY;
    fx64 invW;
    fx32 fracX;
    fx32 fracY;
    int result;
    int x1;
    int y1;
    int x2;
    int y2;

    MTX_MultVec43(world, &NNS_G3dGlb.cameraMtx, &view);
    w = (fx32)(((fx64)view.z * proj->m[2][3] + ((fx64)view.x * proj->m[0][3] + (fx64)view.y * proj->m[1][3])) >>
               FX32_SHIFT);
    w += proj->m[3][3];
    FX_InvAsync(w);
    clipX = (fx32)(((fx64)view.z * proj->m[2][0] + ((fx64)view.x * proj->m[0][0] + (fx64)view.y * proj->m[1][0])) >>
                   FX32_SHIFT);
    clipX += proj->m[3][0];
    clipY = (fx32)(((fx64)view.z * proj->m[2][1] + ((fx64)view.x * proj->m[0][1] + (fx64)view.y * proj->m[1][1])) >>
                   FX32_SHIFT);
    clipY += proj->m[3][1];
    invW = FX_GetDivResultFx64c();
    fracX = (FX_Mul32x64cFunc(clipX, invW) + FX32_ONE) / 2;
    fracY = (FX_Mul32x64cFunc(clipY, invW) + FX32_ONE) / 2;
    if (fracX < 0 || fracY < 0 || fracX > FX32_ONE || fracY > FX32_ONE) {
        result = -1;
    } else {
        result = 0;
    }
    // The viewport's y counts up from the bottom of the screen, whose last line is 191
    NNS_G3DGetViewport(&x1, &y1, &x2, &y2);
    {
        int width = x2 - x1;
        int height = y2 - y1;

        *x = x1 + ((fracX * width + FX32_HALF) >> FX32_SHIFT);
        *y = 191 - y1 - ((fracY * height + FX32_HALF) >> FX32_SHIFT);
    }
    return result;
}

// The near and far ends of the line in the world that a screen position is on, found by taking its place in the
// viewport back through the inverse of the camera and projection matrices with depths of -1 and 1. The far end is left
// out for a null pointer. Returns 0 if the position is within the viewport, or -1
int NNS_G3dScrPosToWorldLine(int px, int py, VecFx32 *nearPos, VecFx32 *farPos) {
    int x1;
    int y1;
    int x2;
    int y2;
    int height;
    fx32 relX;
    fx32 relY;
    int result;
    fx32 ndcX;
    fx32 ndcY;
    const MtxFx44 *inv;
    fx32 wBase;
    fx32 wFar;
    fx32 x;
    fx32 y;
    fx32 z;
    fx32 farX;
    fx32 farY;
    fx32 farZ;
    fx64 invW;

    NNS_G3DGetViewport(&x1, &y1, &x2, &y2);
    height = y2 - y1;
    relX = FX_Div((px - x1) << FX32_SHIFT, (x2 - x1) << FX32_SHIFT);
    relY = FX_Div((py + y1 - 191) << FX32_SHIFT, -height << FX32_SHIFT);
    if (relX < 0 || relY < 0 || relX > FX32_ONE || relY > FX32_ONE) {
        result = -1;
    } else {
        result = 0;
    }
    ndcX = (relX - FX32_HALF) * 2;
    ndcY = (relY - FX32_HALF) * 2;
    inv = NNS_G3DGetInvViewProjectionMatrix();
    wBase = (fx32)(((fx64)ndcX * inv->m[0][3] + (fx64)ndcY * inv->m[1][3]) >> FX32_SHIFT);
    wBase += inv->m[3][3];
    FX_InvAsync(wBase - inv->m[2][3]);
    x = (fx32)(((fx64)ndcX * inv->m[0][0] + (fx64)ndcY * inv->m[1][0]) >> FX32_SHIFT);
    x += inv->m[3][0];
    y = (fx32)(((fx64)ndcX * inv->m[0][1] + (fx64)ndcY * inv->m[1][1]) >> FX32_SHIFT);
    y += inv->m[3][1];
    z = (fx32)(((fx64)ndcX * inv->m[0][2] + (fx64)ndcY * inv->m[1][2]) >> FX32_SHIFT);
    z += inv->m[3][2];
    if (farPos != NULL) {
        farX = x + inv->m[2][0];
        farY = y + inv->m[2][1];
        farZ = z + inv->m[2][2];
        wFar = wBase + inv->m[2][3];
    }
    x -= inv->m[2][0];
    y -= inv->m[2][1];
    z -= inv->m[2][2];
    invW = FX_GetDivResultFx64c();
    if (farPos != NULL) {
        FX_InvAsync(wFar);
    }
    nearPos->x = FX_Mul32x64cFunc(x, invW);
    nearPos->y = FX_Mul32x64cFunc(y, invW);
    nearPos->z = FX_Mul32x64cFunc(z, invW);
    if (farPos != NULL) {
        invW = FX_GetDivResultFx64c();
        farPos->x = FX_Mul32x64cFunc(farX, invW);
        farPos->y = FX_Mul32x64cFunc(farY, invW);
        farPos->z = FX_Mul32x64cFunc(farZ, invW);
    }
    return result;
}
