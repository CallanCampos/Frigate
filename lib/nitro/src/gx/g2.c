// NitroSDK libgx's g2.c: the 2D engines' helpers that take a register's address, so that the main and sub engines
// share them: a BG's affine transformation, blending and the brightness effect. The name is NitroSDK's (pokediamond's
// is GX_g2.c).
#include "nitro/gx.h"

// ARM code in a Thumb file: the SDK compiled this one under nitro/code32.h
#pragma thumb off
void G2x_SetBGyAffine_(u32 addr, const MtxFx22 *mtx, int centerX, int centerY, int x, int y) {
    vu32 *reg = (vu32 *)addr;
    fx32 a = mtx->_00;
    fx32 b = mtx->_01;
    fx32 c = mtx->_10;
    fx32 d = mtx->_11;
    int ddy = y - centerY;
    int ddx = x - centerX;
    s32 dx = (a * ddx + b * ddy) + (centerX << FX32_SHIFT);
    s32 dy = (c * ddx + d * ddy) + (centerY << FX32_SHIFT);
    reg[0] = (u16)(s16)(a >> 4) | ((u16)(s16)(b >> 4) << 16);
    reg[1] = (u16)(s16)(c >> 4) | ((u16)(s16)(d >> 4) << 16);
    reg[2] = dx >> 4;
    reg[3] = dy >> 4;
}
#pragma thumb on

void G2x_SetBlendAlpha_(u32 reg, u32 plane1, u32 plane2, s32 alpha1, s32 alpha2) {
    *(vu32 *)reg = (u32)((plane1 | REG_G2_BLDCNT_EFFECT_ALPHA | (plane2 << 8)) | ((alpha1 | (alpha2 << 8)) << 16));
}

void G2x_SetBlendBrightness_(u32 reg, u32 plane, s32 brightness) {
    if (brightness < 0) {
        *(vu16 *)reg = (u16)(plane | REG_G2_BLDCNT_EFFECT_DECREASE);
        *(vu16 *)(reg + 4) = (u16)-brightness;
    } else {
        *(vu16 *)reg = (u16)(plane | REG_G2_BLDCNT_EFFECT_INCREASE);
        *(vu16 *)(reg + 4) = (u16)brightness;
    }
}

void G2x_SetBlendBrightnessExt_(u32 reg, u32 plane1, u32 plane2, s32 alpha1, s32 alpha2, s32 brightness) {
    *(vu16 *)(reg + 2) = (u16)(alpha1 | (alpha2 << 8));
    if (brightness < 0) {
        *(vu16 *)reg = (u16)(plane1 | REG_G2_BLDCNT_EFFECT_DECREASE | (plane2 << 8));
        *(vu16 *)(reg + 4) = (u16)-brightness;
    } else {
        *(vu16 *)reg = (u16)(plane1 | REG_G2_BLDCNT_EFFECT_INCREASE | (plane2 << 8));
        *(vu16 *)(reg + 4) = (u16)brightness;
    }
}

void G2x_ChangeBlendBrightness_(u32 reg, s32 brightness) {
    u16 bldcnt = *(vu16 *)reg;
    if (brightness < 0) {
        if ((bldcnt & REG_G2_BLDCNT_EFFECT_MASK) == REG_G2_BLDCNT_EFFECT_INCREASE) {
            *(vu16 *)reg = (u16)((bldcnt & ~REG_G2_BLDCNT_EFFECT_MASK) | REG_G2_BLDCNT_EFFECT_DECREASE);
        }
        *(vu16 *)(reg + 4) = (u16)-brightness;
    } else {
        if ((bldcnt & REG_G2_BLDCNT_EFFECT_MASK) == REG_G2_BLDCNT_EFFECT_DECREASE) {
            *(vu16 *)reg = (u16)((bldcnt & ~REG_G2_BLDCNT_EFFECT_MASK) | REG_G2_BLDCNT_EFFECT_INCREASE);
        }
        *(vu16 *)(reg + 4) = (u16)brightness;
    }
}
