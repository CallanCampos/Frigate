#include "nitro/gx.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_OamSoftwareSpriteDraw.c: drawing an OAM as a textured quad with the 3D engine, for the renderer's
// 3D surface. The file name and the names are NitroSystem's, as ntrtwl's reconstruction of NitroSystem has them, except
// texFmtTbl_ and numCharsInRowTbl_, which are ours; the ROM has no string for the file. The texture setup follows the
// public decompilation of NitroSystem that pret's pokeplatinum and pokeheartgold build (github.com/ntrtwl/NitroSystem)

// The texture coordinates of an OAM's corners, and its size in pixels
typedef struct {
    fx32 u0;
    fx32 v0;
    fx32 u1;
    fx32 v1;
    int sx;
    int sy;
} SpriteParams;

static BOOL bAutoZOffsetAdd_;
static fx32 zOffset_;
static NNS_G2dOamSoftEmuUVFlipCorrectFunc s_pUVFlipCorrectFunc;
static fx32 zOffsetStep_ = -FX32_ONE;

// The texture format of each OAM color mode
static const GXTexFmt texFmtTbl_[2] = { GX_TEXFMT_PLTT16, GX_TEXFMT_PLTT256 };
// The characters in a row of a 2D mapped image, by its texture width
static const u32 numCharsInRowTbl_[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
// The texture width and height of each OAM shape and size, for 1D mapped images
static const GXTexSizeS gxTexSizeSTbl[3][4] = {
    { GX_TEXSIZE_S8, GX_TEXSIZE_S16, GX_TEXSIZE_S32, GX_TEXSIZE_S64 },
    { GX_TEXSIZE_S16, GX_TEXSIZE_S32, GX_TEXSIZE_S32, GX_TEXSIZE_S64 },
    { GX_TEXSIZE_S8, GX_TEXSIZE_S8, GX_TEXSIZE_S16, GX_TEXSIZE_S32 },
};
static const GXTexSizeT gxTexSizeTTbl[3][4] = {
    { GX_TEXSIZE_T8, GX_TEXSIZE_T16, GX_TEXSIZE_T32, GX_TEXSIZE_T64 },
    { GX_TEXSIZE_T8, GX_TEXSIZE_T8, GX_TEXSIZE_T16, GX_TEXSIZE_T32 },
    { GX_TEXSIZE_T16, GX_TEXSIZE_T32, GX_TEXSIZE_T32, GX_TEXSIZE_T64 },
};

static inline u32 GetNumTexChar_(GXTexSizeS texSize) {
    return numCharsInRowTbl_[texSize];
}

static inline void DoFlip_(BOOL flipH, BOOL flipV, fx32 *u0, fx32 *u1, fx32 *v0, fx32 *v1) {
    fx32 temp;

    if (flipH) {
        temp = *u0;
        *u0 = *u1;
        *u1 = temp;
    }
    if (flipV) {
        temp = *v0;
        *v0 = *v1;
        *v1 = temp;
    }
    if (s_pUVFlipCorrectFunc != NULL) {
        s_pUVFlipCorrectFunc(u0, v0, u1, v1, flipH, flipV);
    }
}

static inline u32 GetOffsetByteSizeOfPlt_(GXTexFmt fmt, BOOL extendedPltt, u16 plttNo) {
    if (extendedPltt) {
        return (u32)(plttNo << 9);
    } else if (fmt == GX_TEXFMT_PLTT256) {
        return 0;
    } else {
        return (u32)(plttNo << 5);
    }
}

// An OAM's flips as BOOLs. Read through these, the flips take the original's registers, which they don't as locals
// or read in the call
static inline BOOL NNSi_G2dGetOamFlipH(const GXOamAttr *oam) {
    return (BOOL)oam->flipH;
}

static inline BOOL NNSi_G2dGetOamFlipV(const GXOamAttr *oam) {
    return (BOOL)oam->flipV;
}

// The texture coordinates of a 2D mapped image's character
static inline void CalcUVFor3DDirect2DMap_(const NNSG2dImageAttr *texImageAttr, u16 charName, fx32 *u0, fx32 *v0) {
    if (texImageAttr->fmt == GX_TEXFMT_PLTT256) {
        charName >>= 1;
    }
    {
        const u32 numCharPerLine = GetNumTexChar_(texImageAttr->sizeS);

        *u0 = (fx32)(((charName & (numCharPerLine - 1)) << 3) << FX32_SHIFT);
        *v0 = ((charName >> texImageAttr->sizeS) << 3) << FX32_SHIFT;
    }
}

static inline void CalcUVFor3DDirect1DMap_(fx32 *u0, fx32 *v0) {
    *u0 = 0;
    *v0 = 0;
}

// The texture of a 1D mapped image's character, by the OAM's shape and size. The shape and size indices are the same
// expressions as NNS_G2dGetOamSizeX's, so MWCC computes them once, as the original does
static inline void SetTextureParamsFor3DDirect1DMap_(const NNSG2dImageAttr *texImageAttr, u32 texBaseAddr,
                                                     GXOamShape shape, u16 charName) {
    const u16 shapeBit = (u16)((shape & GX_OAM_ATTR0_SHAPE_MASK) >> GX_OAM_ATTR01_SHAPE_SHIFT);
    const u16 sizeBit = (u16)((shape & GX_OAM_ATTR01_SIZE_MASK) >> GX_OAM_ATTR01_SIZE_SHIFT);
    const int shiftBit = 5 + ((texImageAttr->mappingType & REG_GX_DISPCNT_EXOBJ_MASK) >> REG_GX_DISPCNT_EXOBJ_SHIFT);

    reg_G3_TEXIMAGE_PARAM = GX_PACK_TEXIMAGE_PARAM(
        texImageAttr->fmt, GX_TEXGEN_TEXCOORD, gxTexSizeSTbl[shapeBit][sizeBit], gxTexSizeTTbl[shapeBit][sizeBit],
        GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, texImageAttr->plttUse, texBaseAddr + (charName << shiftBit));
}

static inline void SetTextureParamsFor3DDirect2DMap_(const NNSG2dImageAttr *texImageAttr, u32 texBaseAddr) {
    reg_G3_TEXIMAGE_PARAM =
        GX_PACK_TEXIMAGE_PARAM(texImageAttr->fmt, GX_TEXGEN_TEXCOORD, texImageAttr->sizeS, texImageAttr->sizeT,
                               GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, texImageAttr->plttUse, texBaseAddr);
}

static inline void SetPaletteParamsFor3DDirect_(const NNSG2dImageAttr *texImageAttr, u32 pltBaseAddr,
                                                const GXOamAttr *oam) {
    const GXTexFmt fmt = texFmtTbl_[oam->colorMode];
    const u32 pltOffset = GetOffsetByteSizeOfPlt_(fmt, texImageAttr->bExtendedPlt, (u16)oam->cParam);

    G3_TexPlttBase(pltBaseAddr + pltOffset, fmt);
}

// Sets the texture and palette of an OAM's characters, and gives its texture coordinates, flipped as it is. Its
// shape and its helpers follow the public decompilation github.com/ntrtwl/NitroSystem
static void CalcSpriteParams_(const GXOamAttr *oam, const NNSG2dImageAttr *texImageAttr, u32 texBaseAddr,
                              u32 pltBaseAddr, SpriteParams *coord) {
    const GXOamShape shape = oam->attr01 & GX_OAM_ATTR01_SHAPE_MASK;
    const u16 charName = (u16)oam->charNo;

    coord->sx = NNS_G2dGetOamSizeX(&shape);
    coord->sy = NNS_G2dGetOamSizeY(&shape);

    if (texImageAttr->mappingType == GX_OBJVRAMMODE_CHAR_2D) {
        SetTextureParamsFor3DDirect2DMap_(texImageAttr, texBaseAddr);
        CalcUVFor3DDirect2DMap_(texImageAttr, charName, &coord->u0, &coord->v0);
    } else {
        SetTextureParamsFor3DDirect1DMap_(texImageAttr, texBaseAddr, shape, charName);
        CalcUVFor3DDirect1DMap_(&coord->u0, &coord->v0);
    }

    coord->u1 = coord->u0 + (coord->sx << FX32_SHIFT);
    coord->v1 = coord->v0 + (coord->sy << FX32_SHIFT);

    DoFlip_(NNSi_G2dGetOamFlipH(oam), NNSi_G2dGetOamFlipV(oam), &coord->u0, &coord->u1, &coord->v0, &coord->v1);

    SetPaletteParamsFor3DDirect_(texImageAttr, pltBaseAddr, oam);
}

void NNSi_G2dSetOamSoftEmuAutoZOffsetFlag(BOOL flag) {
    bAutoZOffsetAdd_ = flag;
}

void NNSi_G2dResetOamSoftEmuAutoZOffset(void) {
    zOffset_ = 0;
}

void NNSi_G2dSetOamSoftEmuAutoZOffsetStep(fx32 step) {
    zOffsetStep_ = step;
}

fx32 NNSi_G2dGetOamSoftEmuAutoZOffsetStep(void) {
    return zOffsetStep_;
}

// Draws an OAM at a position, as a quad in the current matrix. A double size affine OAM's position is the corner of
// its double size area, so it is moved to the corner of the OAM
void NNS_G2dDrawOneOam3DDirectWithPosFast(s16 posX, s16 posY, s16 posZ, const GXOamAttr *oam,
                                          const NNSG2dImageAttr *texImageAttr, u32 texBaseAddr, u32 pltBaseAddr) {
    SpriteParams coord;
    fx32 z;

    CalcSpriteParams_(oam, texImageAttr, texBaseAddr, pltBaseAddr, &coord);

    if (G2_GetOBJEffect(oam) == GX_OAM_EFFECT_AFFINE_DOUBLE) {
        GXOamShape shape = oam->attr01 & GX_OAM_ATTR01_SHAPE_MASK;
        int halfW = NNS_G2dGetOamSizeX(&shape) >> 1;
        int halfH = NNS_G2dGetOamSizeY(&shape) >> 1;

        if (bAutoZOffsetAdd_) {
            z = (posZ << FX32_SHIFT) + zOffset_;
        } else {
            z = posZ << FX32_SHIFT;
        }
        G3_Translate((posX + halfW) << FX32_SHIFT, (posY + halfH) << FX32_SHIFT, z);
    } else {
        if (bAutoZOffsetAdd_) {
            z = (posZ << FX32_SHIFT) + zOffset_;
        } else {
            z = posZ << FX32_SHIFT;
        }
        G3_Translate(posX << FX32_SHIFT, posY << FX32_SHIFT, z);
    }

    G3_Scale(coord.sx << FX32_SHIFT, coord.sy << FX32_SHIFT, FX32_ONE);
    G3_Begin(GX_BEGIN_QUADS);
    G3_TexCoord(coord.u0, coord.v1);
    G3_Vtx10(0, FX16_ONE, 0);
    G3_TexCoord(coord.u1, coord.v1);
    G3_Vtx10(FX16_ONE, FX16_ONE, 0);
    G3_TexCoord(coord.u1, coord.v0);
    G3_Vtx10(FX16_ONE, 0, 0);
    G3_TexCoord(coord.u0, coord.v0);
    G3_Vtx10(0, 0, 0);
    G3_End();

    if (bAutoZOffsetAdd_) {
        zOffset_ += zOffsetStep_;
    }
}
