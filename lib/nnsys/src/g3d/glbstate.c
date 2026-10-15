#include "nitro/math.h"
#include "nnsys/g3d.h"

// NitroSystem's G3D global state: NNS_G3dGlb, with the projection and camera matrices, the lights, the material colors,
// the viewport and the base transformation that the models are drawn with, how it is sent to the geometry engine, and
// the inverse matrices made from it. The file is named after NitroSystem's own glbstate.c. The functions keep swan's
// names, and g3d.h gives NitroSystem's where the code shows them.
// The command words (cmd0 to cmd4) of NNS_G3dGlb are geometry commands, four to a word, 0x10 for MTX_MODE, 0x16 and
// 0x17 for MTX_LOAD_4x4 and MTX_LOAD_4x3 and so on, which gx.h has no names for yet. Each is followed in the structure
// by the words its commands take, so that a flush sends a run of the structure
// InvertMtx44, NNS_G3DGetInvProjectionMatrix and NNS_G3DPrecalcTempMVMatrix are static because only this file calls
// them

NNSG3dGlb NNS_G3dGlb;

static int InvertMtx44(const MtxFx44 *mtx, MtxFx44 *inv);
static const MtxFx44 *NNS_G3DGetInvProjectionMatrix(void);
static void NNS_G3DPrecalcTempMVMatrix(void);

void NNS_G3DRenderStateInit(void) {
    NNS_G3dGlb.cmd0 = 0x17101610; // MTX_MODE, MTX_LOAD_4x4, MTX_MODE, MTX_LOAD_4x3
    NNS_G3dGlb.mtxmode_proj = GX_MTXMODE_PROJECTION;
    NNS_G3dGlb.mtxmode_posvec = GX_MTXMODE_POSITION_VECTOR;
    NNS_G3dGlb.cmd1 = 0x32323232; // LIGHT_VECTOR four times
    NNS_G3dGlb.cmd2 = 0x60293130; // DIF_AMB, SPE_EMI, POLYGON_ATTR, VIEWPORT
    NNS_G3dGlb.cmd3 = 0x33333333; // LIGHT_COLOR four times
    NNS_G3dGlb.cmd4 = 0x002a1b19; // MTX_MULT_4x3, MTX_SCALE, TEXIMAGE_PARAM
    MTX_Identity43_(&NNS_G3dGlb.cameraMtx);
    MTX_Identity44_(&NNS_G3dGlb.projMtx);

    // Light 0 shines along (-1, -1, -1) and the others along -x, +x and -y, packed as GX_PACK_LIGHTVECTOR_PARAM does
    NNS_G3dGlb.lightVec[0] = 0x2d8b62d8;
    NNS_G3dGlb.lightVec[1] = 0x40000200;
    NNS_G3dGlb.lightVec[2] = 0x800001ff;
    NNS_G3dGlb.lightVec[3] = 0xc0080000;
    NNS_G3dGlb.prmMatColor0 = GX_PACK_DIFFAMB_PARAM(GX_RGB(16, 16, 16), GX_RGB(16, 16, 16), TRUE);
    NNS_G3dGlb.prmMatColor1 = GX_PACK_SPECEMI_PARAM(GX_RGB(16, 16, 16), GX_RGB(16, 16, 16), TRUE);
    NNS_G3dGlb.prmPolygonAttr =
        GX_PACK_POLYGONATTR_PARAM(GX_LIGHTMASK_0 | GX_LIGHTMASK_1 | GX_LIGHTMASK_2 | GX_LIGHTMASK_3,
                                  GX_POLYGONMODE_MODULATE, GX_CULL_BACK, 0, 31, 0);
    NNS_G3dGlb.prmViewPort = GX_PACK_VIEWPORT_PARAM(0, 0, 255, 191);
    NNS_G3dGlb.lightColor[0] = GX_PACK_LIGHTCOLOR_PARAM(GX_LIGHTID_0, GX_RGB(31, 31, 31));
    NNS_G3dGlb.lightColor[1] = GX_PACK_LIGHTCOLOR_PARAM(GX_LIGHTID_1, GX_RGB(31, 0, 0));
    NNS_G3dGlb.lightColor[2] = GX_PACK_LIGHTCOLOR_PARAM(GX_LIGHTID_2, GX_RGB(0, 31, 0));
    NNS_G3dGlb.lightColor[3] = GX_PACK_LIGHTCOLOR_PARAM(GX_LIGHTID_3, GX_RGB(0, 0, 31));

    NNS_G3dGlb.prmBaseTrans.x = 0;
    NNS_G3dGlb.prmBaseTrans.y = 0;
    NNS_G3dGlb.prmBaseTrans.z = 0;
    MTX_Identity33_(&NNS_G3dGlb.prmBaseRot);
    NNS_G3dGlb.prmBaseScale.x = FX32_ONE;
    NNS_G3dGlb.prmBaseScale.y = FX32_ONE;
    NNS_G3dGlb.prmBaseScale.z = FX32_ONE;
    NNS_G3dGlb.prmTexImageParam = 0;
    NNS_G3dGlb.flag = 0;

    NNS_G3dGlb.camPos.x = NNS_G3dGlb.camPos.y = NNS_G3dGlb.camPos.z = 0;
    NNS_G3dGlb.camUp.x = NNS_G3dGlb.camUp.z = 0;
    NNS_G3dGlb.camUp.y = FX32_ONE;
    NNS_G3dGlb.camTarget.x = NNS_G3dGlb.camTarget.y = 0;
    NNS_G3dGlb.camTarget.z = -FX32_ONE;
}

// Sends all of the global state, the structure after its first command word
void NNS_G3DFlushRenderState(void) {
    const u32 *glb = (const u32 *)&NNS_G3dGlb;

    NNS_G3DExecOp(glb[0], glb + 1, 62);
    NNS_G3dGlb.flag &= ~NNS_G3D_GLB_FLAG_FLUSH_WVP;
    NNS_G3dGlb.flag &= ~NNS_G3D_GLB_FLAG_FLUSH_VP;
}

// Sends the projection matrix with the camera matrix applied to it, then the rest of the state from cmd1: the position
// matrix starts as the identity, and gets the base transformation
void NNS_G3DFlushViewProjection(void) {
    NNS_G3DExecOp(0x1610, &NNS_G3dGlb.mtxmode_proj, 17); // MTX_MODE and MTX_LOAD_4x4
    NNS_G3dGeMultMtx43(&NNS_G3dGlb.cameraMtx);
    NNS_G3dGeMtxMode(GX_MTXMODE_POSITION_VECTOR);
    NNS_G3DExecOp(G3OP_MTX_IDENTITY, &NNS_G3dGlb.cmd1, 32);
    NNS_G3dGlb.flag &= ~NNS_G3D_GLB_FLAG_FLUSH_WVP;
    NNS_G3dGlb.flag |= NNS_G3D_GLB_FLAG_FLUSH_VP;
}

// Sends the projection matrix with the camera matrix and the base transformation applied to it, the inverse of the base
// rotation as the matrix that the light vectors are transformed by (the rotation is not in the position matrix), and
// the state from cmd1. Only the rotation of invBaseRot is set: its translation goes to the position matrix, which the
// identity right after replaces
void NNS_G3DFlushModelViewProjection(void) {
    MtxFx43 invBaseRot;

    NNS_G3DExecOp(0x1610, &NNS_G3dGlb.mtxmode_proj, 17); // MTX_MODE and MTX_LOAD_4x4
    NNS_G3dGeMultMtx43(&NNS_G3dGlb.cameraMtx);
    NNS_G3DExecOp(0x1b19, (const u32 *)&NNS_G3dGlb.prmBaseMtx, 15); // MTX_MULT_4x3 and MTX_SCALE
    NNS_G3dGeMtxMode(GX_MTXMODE_POSITION_VECTOR);
    MAT3_Invert(&NNS_G3dGlb.prmBaseRot, &invBaseRot.rt.rot);
    NNS_G3dGeLoadMtx43(&invBaseRot);
    NNS_G3DExecOp(NNS_G3dGlb.cmd1, (const u32 *)&NNS_G3dGlb.cmd1 + 1, 14);
    NNS_G3dGeIdentity();
    NNS_G3DExecOp(0x2a, &NNS_G3dGlb.prmTexImageParam, 1); // TEXIMAGE_PARAM
    NNS_G3dGlb.flag |= NNS_G3D_GLB_FLAG_FLUSH_WVP;
    NNS_G3dGlb.flag &= ~NNS_G3D_GLB_FLAG_FLUSH_VP;
}

void NNS_G3dGlbSetBaseTrans(const VecFx32 *trans) {
    if (trans != NULL) {
        NNS_G3dGlb.prmBaseTrans = *trans;
        NNS_G3dGlb.flag &= ~(NNS_G3D_GLB_FLAG_BASECAMERA_UPTODATE | NNS_G3D_GLB_FLAG_INVBASE_UPTODATE |
                             NNS_G3D_GLB_FLAG_INVBASECAMERA_UPTODATE);
    }
}

void NNS_G3dGlbSetBaseScale(const VecFx32 *scale) {
    if (scale != NULL) {
        NNS_G3dGlb.prmBaseScale = *scale;
        NNS_G3dGlb.flag &= ~(NNS_G3D_GLB_FLAG_BASECAMERA_UPTODATE | NNS_G3D_GLB_FLAG_INVBASE_UPTODATE |
                             NNS_G3D_GLB_FLAG_INVBASECAMERA_UPTODATE);
    }
}

void NNS_G3DSetLightVector(int lightID, fx16 x, fx16 y, fx16 z) {
    NNS_G3dGlb.lightVec[lightID] = GX_PACK_LIGHTVECTOR_PARAM(lightID, x, y, z);
}

void NNS_G3DSetLightColor(int lightID, GXRgb rgb) {
    NNS_G3dGlb.lightColor[lightID] = GX_PACK_LIGHTCOLOR_PARAM(lightID, rgb);
}

void NNS_G3DSetMatDifAmb(GXRgb diffuse, GXRgb ambient, BOOL isSetVtxColor) {
    NNS_G3dGlb.prmMatColor0 = GX_PACK_DIFFAMB_PARAM(diffuse, ambient, isSetVtxColor);
}

void NNS_G3DSetMatSpeEmi(GXRgb specular, GXRgb emission, BOOL isShininess) {
    NNS_G3dGlb.prmMatColor1 = GX_PACK_SPECEMI_PARAM(specular, emission, isShininess);
}

const MtxFx43 *NNS_G3DGetInvViewMatrix(void) {
    if (!(NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_INVCAMERA_UPTODATE)) {
        MTX_Inverse43(&NNS_G3dGlb.cameraMtx, &NNS_G3dGlb.invCameraMtx);
        NNS_G3dGlb.flag |= NNS_G3D_GLB_FLAG_INVCAMERA_UPTODATE;
    }
    return &NNS_G3dGlb.invCameraMtx;
}

// Inverts a 4x4 matrix by Gauss-Jordan elimination: tmp is reduced to the identity by row operations, and inv, which
// starts as the identity, gets the same ones. The pivot of each column is the largest entry at or below the diagonal.
// Returns -1 when a column has none
static int InvertMtx44(const MtxFx44 *mtx, MtxFx44 *inv) {
    MtxFx44 tmp;
    int col;
    int i;
    int j;

    MI_Copy64B(mtx, &tmp);
    MTX_Identity44_(inv);

    for (col = 0; col < 4; col++) {
        fx32 max = 0;
        int pivot = col;
        fx64 invPivot;

        for (i = col; i < 4; i++) {
            fx32 value = MATH_ABS(tmp.m[i][col]);

            if (value > max) {
                max = value;
                pivot = i;
            }
        }
        if (max == 0) {
            return -1;
        }

        if (pivot != col) {
            for (i = 0; i < 4; i++) {
                fx32 swap = tmp.m[col][i];

                tmp.m[col][i] = tmp.m[pivot][i];
                tmp.m[pivot][i] = swap;
                swap = inv->m[col][i];
                inv->m[col][i] = inv->m[pivot][i];
                inv->m[pivot][i] = swap;
            }
        }

        invPivot = FX_InvFx64c(tmp.m[col][col]);
        for (j = 0; j < 4; j++) {
            tmp.m[col][j] = FX_Mul32x64cFunc(tmp.m[col][j], invPivot);
            inv->m[col][j] = FX_Mul32x64cFunc(inv->m[col][j], invPivot);
        }

        for (i = 0; i < 4; i++) {
            if (i != col) {
                fx32 factor = tmp.m[i][col];

                for (j = 0; j < 4; j++) {
                    tmp.m[i][j] -= (fx32)(((fx64)factor * tmp.m[col][j]) >> FX32_SHIFT);
                    inv->m[i][j] -= (fx32)(((fx64)factor * inv->m[col][j]) >> FX32_SHIFT);
                }
            }
        }
    }
    return 0;
}

static const MtxFx44 *NNS_G3DGetInvProjectionMatrix(void) {
    if (!(NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_INVPROJ_UPTODATE)) {
        InvertMtx44(&NNS_G3dGlb.projMtx, &NNS_G3dGlb.invProjMtx);
        NNS_G3dGlb.flag |= NNS_G3D_GLB_FLAG_INVPROJ_UPTODATE;
    }
    return &NNS_G3dGlb.invProjMtx;
}

// Applies the base transformation to the view matrix, and inverts the result
static void NNS_G3DPrecalcTempMVMatrix(void) {
    MTX_Concat43(&NNS_G3dGlb.prmBaseMtx, &NNS_G3dGlb.cameraMtx, &NNS_G3dGlb.baseCameraMtx);
    MTX_ScaleApply43(&NNS_G3dGlb.baseCameraMtx, &NNS_G3dGlb.baseCameraMtx, NNS_G3dGlb.prmBaseScale.x,
                     NNS_G3dGlb.prmBaseScale.y, NNS_G3dGlb.prmBaseScale.z);
    MTX_Inverse43(&NNS_G3dGlb.baseCameraMtx, &NNS_G3dGlb.invBaseCameraMtx);
}

const MtxFx43 *NNS_G3DGetTempMVMatrix(void) {
    if (!(NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_BASECAMERA_UPTODATE)) {
        NNS_G3DPrecalcTempMVMatrix();
        NNS_G3dGlb.flag |= NNS_G3D_GLB_FLAG_BASECAMERA_UPTODATE;
    }
    return &NNS_G3dGlb.baseCameraMtx;
}

const MtxFx43 *NNS_G3DGetTempInvMVMatrix(void) {
    if (!(NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_BASECAMERA_UPTODATE)) {
        NNS_G3DPrecalcTempMVMatrix();
        NNS_G3dGlb.flag |= NNS_G3D_GLB_FLAG_BASECAMERA_UPTODATE;
    }
    return &NNS_G3dGlb.invBaseCameraMtx;
}

const MtxFx44 *NNS_G3DGetInvViewProjectionMatrix(void) {
    if (!(NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_INVCAMERAPROJ_UPTODATE)) {
        const MtxFx43 *invView = NNS_G3DGetInvViewMatrix();
        const MtxFx44 *invProj = NNS_G3DGetInvProjectionMatrix();
        MtxFx44 invView44;

        MTX_Copy43To44_(invView, &invView44);
        MTX_Concat44(invProj, &invView44, &NNS_G3dGlb.invCameraProjMtx);
        NNS_G3dGlb.flag |= NNS_G3D_GLB_FLAG_INVCAMERAPROJ_UPTODATE;
    }
    return &NNS_G3dGlb.invCameraProjMtx;
}

void NNS_G3DGetViewport(int *x1, int *y1, int *x2, int *y2) {
    if (x1 != NULL) {
        *x1 = NNS_G3dGlb.prmViewPort & 0xff;
    }
    if (y1 != NULL) {
        *y1 = (NNS_G3dGlb.prmViewPort >> 8) & 0xff;
    }
    if (x2 != NULL) {
        *x2 = (NNS_G3dGlb.prmViewPort >> 16) & 0xff;
    }
    if (y2 != NULL) {
        *y2 = (NNS_G3dGlb.prmViewPort >> 24) & 0xff;
    }
}
