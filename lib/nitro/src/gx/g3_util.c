// NitroSDK libgx's g3_util.c: the projection and camera matrices built into the geometry engine's matrix stack, and the
// rotations about the three axes. The name is NitroSDK's (g3_util.h). Its first function, G3i_FrustumW_, is not in the
// game.
#include "nitro/gx.h"

void G3i_PerspectiveW_(fx32 fovySin, fx32 fovyCos, fx32 aspect, fx32 n, fx32 f, fx32 scaleW, BOOL isLoad,
                       MtxFx44 *mtx) {
    MtxFx44 tmp;
    if (mtx == NULL) {
        mtx = &tmp;
    }
    MTX_PerspectiveW(fovySin, fovyCos, aspect, n, f, scaleW, mtx);
    if (isLoad) {
        G3_MtxMode(GX_MTXMODE_PROJECTION);
        G3_LoadMtx44(mtx);
    }
}

void G3i_OrthoW_(fx32 t, fx32 b, fx32 l, fx32 r, fx32 n, fx32 f, fx32 scaleW, BOOL isLoad, MtxFx44 *mtx) {
    MtxFx44 tmp;
    if (mtx == NULL) {
        mtx = &tmp;
    }
    MTX_OrthoW(t, b, l, r, n, f, scaleW, mtx);
    if (isLoad) {
        G3_MtxMode(GX_MTXMODE_PROJECTION);
        G3_LoadMtx44(mtx);
    }
}

void G3i_LookAt_(const VecFx32 *camPos, const VecFx32 *camUp, const VecFx32 *target, BOOL isLoad, MtxFx43 *mtx) {
    MtxFx43 tmp;
    if (mtx == NULL) {
        mtx = &tmp;
    }
    MTX_LookAt(camPos, camUp, target, mtx);
    if (isLoad) {
        G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
        G3_LoadMtx43(mtx);
    }
}

void G3_RotX(fx32 sin, fx32 cos) {
    reg_G3_MTX_MULT_3x3 = FX32_ONE;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = cos;
    reg_G3_MTX_MULT_3x3 = sin;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = -sin;
    reg_G3_MTX_MULT_3x3 = cos;
}

void G3_RotY(fx32 sin, fx32 cos) {
    reg_G3_MTX_MULT_3x3 = cos;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = -sin;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = FX32_ONE;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = sin;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = cos;
}

void G3_RotZ(fx32 sin, fx32 cos) {
    reg_G3_MTX_MULT_3x3 = cos;
    reg_G3_MTX_MULT_3x3 = sin;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = -sin;
    reg_G3_MTX_MULT_3x3 = cos;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = 0;
    reg_G3_MTX_MULT_3x3 = FX32_ONE;
}
