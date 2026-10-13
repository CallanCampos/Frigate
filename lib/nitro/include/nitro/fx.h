#ifndef POKEBW2_NITRO_FX_H
#define POKEBW2_NITRO_FX_H

#include "types.h"

// Fixed point numbers with 12 fractional bits
typedef s16 fx16;
typedef s32 fx32;
typedef s64 fx64;

#define FX16_ONE (1 << 12)
#define FX32_SHIFT 12
#define FX32_ONE (1 << FX32_SHIFT)
#define FX32_HALF (FX32_ONE / 2)
#define FX32_DEC_MASK (FX32_ONE - 1)
#define FX32_MAX ((fx32)0x7fffffff)
#define FX32_MIN ((fx32)0x80000000)
#define FX32_CONST(x) ((fx32)(((x) > 0) ? ((x) * FX32_ONE + 0.5f) : ((x) * FX32_ONE - 0.5f)))
#define FX16_CONST(x) ((fx16)(((x) > 0) ? ((x) * FX16_ONE + 0.5f) : ((x) * FX16_ONE - 0.5f)))
// Conversions between fixed point numbers and floats, rounded to the nearest
#define FX_F32_TO_FX16(x) ((fx16)(((x) > 0) ? (fx16)((x) * FX16_ONE + 0.5f) : (fx16)((x) * FX16_ONE - 0.5f)))
#define FX_F32_TO_FX32(x) ((fx32)(((x) > 0) ? (fx32)((x) * FX32_ONE + 0.5f) : (fx32)((x) * FX32_ONE - 0.5f)))
#define FX_FX16_TO_F32(x) ((f32)((x) / (f32)FX16_ONE))
#define FX_FX32_TO_F32(x) ((f32)((x) / (f32)FX32_ONE))
#define FX_Whole(a) ((s32)((a) >> FX32_SHIFT))

typedef struct {
    fx32 x;
    fx32 y;
    fx32 z;
} VecFx32;

typedef struct {
    fx16 x;
    fx16 y;
    fx16 z;
} VecFx16;

typedef union {
    struct {
        fx32 _00, _01, _10, _11;
    };
    fx32 m[2][2];
} MtxFx22;

// A 2D affine matrix, its last row the translation
typedef struct {
    fx32 _00;
    fx32 _01;
    fx32 _10;
    fx32 _11;
    fx32 _20;
    fx32 _21;
} MtxFx32;

typedef struct {
    fx32 m[3][3];
} MtxFx33;

typedef union {
    fx32 m[4][3];
    // The rotation and scale, then the translation
    struct {
        MtxFx33 rot;
        VecFx32 trans;
    } rt;
} MtxFx43;

typedef struct {
    fx32 m[4][4];
} MtxFx44;

// The sine and cosine of 4096 angles around the circle, as pairs
extern const fx16 FX_SIN_COS_TABLE[4096 * 2];

// An angle in whole degrees as the 16-bit angle that FX_SinIdx and FX_CosIdx take, 0x10000 for a full turn
#define DEG_TO_IDX(deg) ((u16)((deg) * 0x10000 / 360))

static inline fx16 FX_SinIdx(int idx) {
    return FX_SIN_COS_TABLE[(idx >> 4) << 1];
}

static inline fx16 FX_CosIdx(int idx) {
    return FX_SIN_COS_TABLE[((idx >> 4) << 1) + 1];
}

void MAT3_Identity(MtxFx33 *mtx);

// Rotations about an axis from its sine and cosine: NitroSDK's MTX_Rot22_, MTX_RotX33_ and the rest
void MAT2_Rotation(MtxFx22 *mtx, fx32 sin, fx32 cos);
// NitroSDK's MTX_Identity22 and MTX_Scale22
void MAT2_Identity(MtxFx22 *mtx);
void MAT2_Scaling(MtxFx22 *mtx, fx32 x, fx32 y);
void MAT43_RotationY(MtxFx43 *mtx, fx32 sin, fx32 cos);
void MAT43_RotationAxisAngle(MtxFx43 *mtx, const VecFx32 *axis, fx16 sin, fx16 cos);
void MAT3_RotationY(MtxFx33 *mtx, fx32 sin, fx32 cos);
void MAT3_RotationX(MtxFx33 *mtx, fx32 sin, fx32 cos);
void MAT3_RotationY(MtxFx33 *mtx, fx32 sin, fx32 cos);
void MAT3_RotationZ(MtxFx33 *mtx, fx32 sin, fx32 cos);
// NitroSDK's MTX_ScaleApply22 and MTX_Concat33
void MAT2_Scale(const MtxFx22 *src, MtxFx22 *dest, fx32 x, fx32 y);
void MAT3_Mul(const MtxFx33 *a, const MtxFx33 *b, MtxFx33 *ab);
void MAT3_MulVec(const VecFx32 *vec, const MtxFx33 *mtx, VecFx32 *dest);
void MAT43_MulVec(const VecFx32 *vec, const MtxFx43 *mtx, VecFx32 *dest);
// NitroSDK's MTX_Identity43, MTX_Scale43 and MTX_Concat43
void MAT43_Identity(MtxFx43 *mtx);
void MAT43_Scaling(MtxFx43 *mtx, fx32 x, fx32 y, fx32 z);
void MAT43_Mul(const MtxFx43 *a, const MtxFx43 *b, MtxFx43 *ab);
// Projection and camera matrices: NitroSDK's MTX_PerspectiveW, MTX_FrustumW, MTX_OrthoW and MTX_LookAt
void MAT4_SetPerspective(fx32 fovySin, fx32 fovyCos, fx32 aspect, fx32 n, fx32 f, fx32 scaleW, MtxFx44 *mtx);
void MAT4_SetFrustum(fx32 t, fx32 b, fx32 l, fx32 r, fx32 n, fx32 f, fx32 scaleW, MtxFx44 *mtx);
void MAT4_SetOrtho(fx32 t, fx32 b, fx32 l, fx32 r, fx32 n, fx32 f, fx32 scaleW, MtxFx44 *mtx);
void MAT43_LookAt(const VecFx32 *camPos, const VecFx32 *camUp, const VecFx32 *target, MtxFx43 *mtx);

void VEC_Add(const VecFx32 *a, const VecFx32 *b, VecFx32 *ab);
void VEC_Subtract(const VecFx32 *a, const VecFx32 *b, VecFx32 *ab);
void vecfx_normalize(const VecFx32 *src, VecFx32 *dest);
fx32 vecfx_dist(const VecFx32 *a, const VecFx32 *b);
void vecfx_muladd(fx32 scale, const VecFx32 *a, const VecFx32 *b, VecFx32 *dest);
void vecfx_mul(const VecFx32 *src, fx32 scale, VecFx32 *dest);
fx32 VEC_Mag(const VecFx32 *v);
fx32 vecfx_dot(const VecFx32 *a, const VecFx32 *b);
// NitroSDK's VEC_CrossProduct
void vecfx_cross(const VecFx32 *a, const VecFx32 *b, VecFx32 *axb);
// NitroSDK's VEC_Fx16 functions
fx32 vecfx_dot16(const VecFx16 *a, const VecFx16 *b);
void vecfx_cross16(const VecFx16 *a, const VecFx16 *b, VecFx16 *axb);
void vecfx_normalize16(const VecFx16 *src, VecFx16 *dest);
// NitroSDK's VEC_Fx16Add
void vecfx_add16(const VecFx16 *a, const VecFx16 *b, VecFx16 *ab);

// An angle in fixed point degrees as a 16-bit angle
#define FX64C_65536_360 ((s64)0x000000b60b60b60bLL)
#define FX_DEG_TO_IDX(deg) ((u16)(((deg) * FX64C_65536_360 + 0x80000000000LL) >> 44))

fx32 FX_Div(fx32 numer, fx32 denom);
s32 FX_ModS32(s32 numer, s32 denom);
s32 FX_DivS32(s32 numer, s32 denom);
fx32 FX_Inv(fx32 x);
// NitroSDK's FX_Mul as a function, rounding, and FX_Atan2Idx
fx32 fx_mul_round(fx32 v1, fx32 v2);
u16 fx_atan2(fx32 y, fx32 x);
// NitroSDK's FX_AcosIdx
u16 fx_acos(fx32 x);
// Returns the fraction of x and writes its whole part, both with the sign of x
fx32 fx_fract(fx32 x, fx32 *whole);
fx32 FX_Sqrt(fx32 x);
fx32 FX_InvSqrt(fx32 x);

static inline fx32 FX_Mul(fx32 v1, fx32 v2) {
    return (fx32)(((fx64)v1 * v2 + 0x800LL) >> FX32_SHIFT);
}

// The SDK's macro form, whose operands keep their types: an fx16 one makes a full 64-bit multiply
#define FX_MUL(v1, v2) ((fx32)(((fx64)(v1) * (v2) + 0x800LL) >> FX32_SHIFT))

static inline void VEC_Set(VecFx32 *v, fx32 x, fx32 y, fx32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
}
void MAT2_SetScaleRot(MtxFx22 *mtx, u16 rot, fx32 scaleX, fx32 scaleY, u8 mode);

static inline void VEC_Fx16Set(VecFx16 *v, fx16 x, fx16 y, fx16 z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

// NitroSDK's MTX_Inverse43, which returns -1 when the matrix has no inverse
int MAT43_Invert(const MtxFx43 *mtx, MtxFx43 *inv);
// NitroSDK's MTX_Inverse33
int MAT3_Invert(const MtxFx33 *mtx, MtxFx33 *inv);
// NitroSDK's MTX_Copy33To44, MTX_Copy43To44 and MTX_Concat44
void MAT3_To4x4(const MtxFx33 *src, MtxFx44 *dst);
void MAT43_To4x4(const MtxFx43 *src, MtxFx44 *dst);
void MAT4_Mul(const MtxFx44 *a, const MtxFx44 *b, MtxFx44 *ab);

#endif // POKEBW2_NITRO_FX_H
