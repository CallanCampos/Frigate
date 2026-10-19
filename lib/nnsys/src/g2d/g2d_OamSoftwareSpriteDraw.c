#include "nitro/gx.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_OamSoftwareSpriteDraw.c: drawing an OAM as a textured quad with the 3D engine, for the renderer's
// 3D surface. The file name is a guess, as the ROM has no string for it; the names of the public functions are the
// SDK's, and the statics and the helpers are named here

// The texture coordinates of an OAM's corners, and its size in pixels
typedef struct {
    fx32 u0;
    fx32 v0;
    fx32 u1;
    fx32 v1;
    int w;
    int h;
} OamTexCoord;

static BOOL bAutoZOffsetAdd_;
static fx32 zOffset_;
static NNSG2dOamSoftEmuUVFlipCorrectFunc s_pUVFlipCorrectFunc;
static fx32 zOffsetStep_ = -FX32_ONE;

// The texture format of each OAM color mode
static const GXTexFmt texFmtTbl_[2] = { GX_TEXFMT_PLTT16, GX_TEXFMT_PLTT256 };
// The characters in a row of a 2D mapped image, by its texture width
static const u32 numCharsInRowTbl_[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
// The texture width and height of each OAM shape and size, for 1D mapped images
static const GXTexSizeS texSizeSTbl_[3][4] = {
    { GX_TEXSIZE_S8, GX_TEXSIZE_S16, GX_TEXSIZE_S32, GX_TEXSIZE_S64 },
    { GX_TEXSIZE_S16, GX_TEXSIZE_S32, GX_TEXSIZE_S32, GX_TEXSIZE_S64 },
    { GX_TEXSIZE_S8, GX_TEXSIZE_S8, GX_TEXSIZE_S16, GX_TEXSIZE_S32 },
};
static const GXTexSizeT texSizeTTbl_[3][4] = {
    { GX_TEXSIZE_T8, GX_TEXSIZE_T16, GX_TEXSIZE_T32, GX_TEXSIZE_T64 },
    { GX_TEXSIZE_T8, GX_TEXSIZE_T8, GX_TEXSIZE_T16, GX_TEXSIZE_T32 },
    { GX_TEXSIZE_T16, GX_TEXSIZE_T32, GX_TEXSIZE_T32, GX_TEXSIZE_T64 },
};

// Sets the texture and palette of an OAM's characters, and gives its texture coordinates, flipped as it is
static void SetTexParamsAndGetTexCoord_(const GXOamAttr *oam, const NNSG2dImageAttr *texImageAttr, u32 texBaseAddr,
                                        u32 pltBaseAddr, OamTexCoord *coord) {
    GXOamShape shape = oam->attr01 & GX_OAM_ATTR01_SHAPE_MASK;
    int shapeIdx = (shape & GX_OAM_ATTR0_SHAPE_MASK) >> GX_OAM_ATTR01_SHAPE_SHIFT;
    int sizeIdx = (shape & GX_OAM_ATTR01_SIZE_MASK) >> GX_OAM_ATTR01_SIZE_SHIFT;
    u16 charName;
    BOOL flipV;
    BOOL flipH;
    GXTexFmt fmt;
    u16 cParam;
    u32 pltOffset;

    coord->w = NNSi_objSizeWTbl[shapeIdx][sizeIdx];
    coord->h = NNSi_objSizeHTbl[shapeIdx][sizeIdx];

    charName = oam->charNo;
    if (texImageAttr->mappingType == GX_OBJVRAMMODE_CHAR_2D) {
        G3_TexImageParam(texImageAttr->fmt, GX_TEXGEN_TEXCOORD, texImageAttr->sizeS, texImageAttr->sizeT,
                         GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, texImageAttr->plttUse, texBaseAddr);
        if (texImageAttr->fmt == GX_TEXFMT_PLTT256) {
            charName >>= 1;
        }
        coord->u0 = ((numCharsInRowTbl_[texImageAttr->sizeS] - 1) & charName) << (3 + FX32_SHIFT);
        coord->v0 = (charName >> texImageAttr->sizeS) << (3 + FX32_SHIFT);
    } else {
        G3_TexImageParam(
            texImageAttr->fmt, GX_TEXGEN_TEXCOORD, texSizeSTbl_[(u16)shapeIdx][(u16)sizeIdx],
            texSizeTTbl_[(u16)shapeIdx][(u16)sizeIdx], GX_TEXREPEAT_NONE, GX_TEXFLIP_NONE, texImageAttr->plttUse,
            texBaseAddr +
                (charName << (((texImageAttr->mappingType & REG_GX_DISPCNT_EXOBJ_MASK) >> REG_GX_DISPCNT_EXOBJ_SHIFT) +
                              5)));
        coord->u0 = 0;
        coord->v0 = 0;
    }
    coord->u1 = coord->u0 + (coord->w << FX32_SHIFT);
    coord->v1 = coord->v0 + (coord->h << FX32_SHIFT);

    flipV = oam->flipV;
    flipH = oam->flipH;
    if (flipH) {
        fx32 tmp = coord->u0;
        coord->u0 = coord->u1;
        coord->u1 = tmp;
    }
    if (flipV) {
        fx32 tmp = coord->v0;
        coord->v0 = coord->v1;
        coord->v1 = tmp;
    }
    if (s_pUVFlipCorrectFunc != NULL) {
        s_pUVFlipCorrectFunc(&coord->u0, &coord->v0, &coord->u1, &coord->v1, flipH, flipV);
    }

    fmt = texFmtTbl_[oam->colorMode];
    cParam = oam->cParam;
    if (texImageAttr->bExtendedPlt) {
        pltOffset = cParam * 0x200;
    } else if (fmt == GX_TEXFMT_PLTT256) {
        pltOffset = 0;
    } else {
        pltOffset = cParam * 0x20;
    }
    G3_TexPlttBase(pltBaseAddr + pltOffset, fmt);
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
    OamTexCoord coord;
    fx32 z;

    SetTexParamsAndGetTexCoord_(oam, texImageAttr, texBaseAddr, pltBaseAddr, &coord);

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

    G3_Scale(coord.w << FX32_SHIFT, coord.h << FX32_SHIFT, FX32_ONE);
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
