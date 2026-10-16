#include "nnsys/g3d.h"

// NitroSystem's G3D material color animation (NSBMA, the "BMA0" files): the function that fills in an animation object
// for one, the function the object holds, which sets a material's colors and alpha at a frame, and the readers of the
// animation's values. The file is named after NitroSystem's nsbma.c. The two global functions are the ones that anm.c's
// tables hold; the readers are static because only this file calls them

// The red and blue bits of a color. They are far enough apart to be summed together, while the green bits are summed
// on their own, so that no sum carries into the next channel
#define COLOR_RB_MASK (GX_RGB_R_MASK | GX_RGB_B_MASK)

// Three parts of one color and one of the other, in the bits of a mask: the sum comes out shifted two bits down by the
// caller
#define BLEND_3_1(a, b, mask) (((a) & (mask)) + ((a) & (mask)) + ((a) & (mask)) + ((b) & (mask)))

static u16 getColorData_(const NNSG3dResMatCAnm *anm, u32 info, u32 frame);
static u16 getAlphaData_(const NNSG3dResMatCAnm *anm, u32 info, u32 frame);

// The color of a material at a frame, from the word that says how its values are kept. The color between two stored
// ones is made from them a group of bits at a time, red and blue then green
static u16 getColorData_(const NNSG3dResMatCAnm *anm, u32 info, u32 frame) {
    const u16 *data;
    u32 last;
    u32 idx;

    if (info & NNS_G3D_MATCANM_ELEM_CONST) {
        return info;
    }
    data = (const u16 *)((const u8 *)anm + (info & NNS_G3D_MATCANM_ELEM_OFFSET_MASK));
    if (!(info & NNS_G3D_MATCANM_ELEM_STEP_MASK)) {
        return data[frame];
    }
    last = (info & NNS_G3D_MATCANM_ELEM_LAST_INTERP_MASK) >> NNS_G3D_MATCANM_ELEM_LAST_INTERP_SHIFT;
    if (info & NNS_G3D_MATCANM_ELEM_STEP_2) {
        if (frame & 1) {
            if (frame > last) {
                return (data + (last >> 1))[1];
            }
            idx = frame >> 1;
        } else {
            return data[frame >> 1];
        }
    } else {
        u32 rem = frame & 3;

        if (rem != 0) {
            if (frame > last) {
                return (data + (last >> 2))[rem];
            }
            if (frame & 1) {
                // A quarter of the way from a stored value to the next is three parts of the nearer one and one of
                // the other
                u32 idxA;
                u32 idxB;
                u32 a;
                u32 b;
                u32 rb;
                u32 g;

                if (frame & 2) {
                    idxB = frame >> 2;
                    idxA = idxB + 1;
                } else {
                    idxA = frame >> 2;
                    idxB = idxA + 1;
                }
                a = *(data + idxA);
                b = *(data + idxB);
                rb = BLEND_3_1(a, b, COLOR_RB_MASK) >> 2;
                g = BLEND_3_1(a, b, GX_RGB_G_MASK) >> 2;
                return (rb & COLOR_RB_MASK) | (g & GX_RGB_G_MASK);
            }
            idx = frame >> 2;
        } else {
            return data[frame >> 2];
        }
    }
    {
        u32 a = *(data + idx);
        u32 b = *(data + idx + 1);
        u32 rb = ((a & COLOR_RB_MASK) + (b & COLOR_RB_MASK)) >> 1;
        u32 g = ((a & GX_RGB_G_MASK) + (b & GX_RGB_G_MASK)) >> 1;

        return (rb & COLOR_RB_MASK) | (g & GX_RGB_G_MASK);
    }
}

// The same for the alpha, a byte, where the one between two stored values is their average, rounded down
static u16 getAlphaData_(const NNSG3dResMatCAnm *anm, u32 info, u32 frame) {
    const u8 *data;
    u32 last;

    if (info & NNS_G3D_MATCANM_ELEM_CONST) {
        return info;
    }
    data = (const u8 *)anm + (info & NNS_G3D_MATCANM_ELEM_OFFSET_MASK);
    if (!(info & NNS_G3D_MATCANM_ELEM_STEP_MASK)) {
        return data[frame];
    }
    last = (info & NNS_G3D_MATCANM_ELEM_LAST_INTERP_MASK) >> NNS_G3D_MATCANM_ELEM_LAST_INTERP_SHIFT;
    if (info & NNS_G3D_MATCANM_ELEM_STEP_2) {
        if (frame & 1) {
            if (frame > last) {
                return *(data + (last >> 1) + 1);
            }
            return (*(data + (frame >> 1)) + *(data + (frame >> 1) + 1)) >> 1;
        }
        return data[frame >> 1];
    } else {
        u32 rem = frame & 3;

        if (rem != 0) {
            if (frame > last) {
                return (data + (last >> 2))[rem];
            }
            if (frame & 1) {
                // The nearer stored value is at idxA and the other at frame
                u32 idxA;

                if (frame & 2) {
                    frame >>= 2;
                    idxA = frame + 1;
                } else {
                    idxA = frame >> 2;
                    frame = idxA + 1;
                }
                return (data[idxA] + data[idxA] + data[idxA] + data[frame]) >> 2;
            }
            return (*(data + (frame >> 2)) + *(data + (frame >> 2) + 1)) >> 1;
        }
        return data[frame >> 2];
    }
}

// Fills in an animation object for a material color animation (NitroSystem's NNS_G3dAnmObjInitNsBma). It applies the
// animation with NNS_G3dFuncAnmMatNsBma, and maps each material of the model to the data of the animation's material
// of the same name, which is flagged as existing and holds the data's index
void NNS_G3DAnimationInit_MA(NNSG3dAnmObj *anmObj, const void *anm, const NNSG3dResMdl *mdl) {
    const NNSG3dResMatCAnm *matAnm = anm;
    const NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    u32 i;

    anmObj->funcAnm = NNS_G3dFuncAnmMatNsBma;
    anmObj->numMapData = mdl->info.numMat;
    MI_CpuClear16(anmObj->mapData, anmObj->numMapData * sizeof(u16));
    for (i = 0; i < matAnm->dict.numEntry; i++) {
        const NNSG3dResName *name = NNS_G3dGetResNameByIdx(&matAnm->dict, i);
        int idx = mat != NULL ? NNS_G3dGetResDictIdxByName(&mat->dict, name) : -1;

        if (idx >= 0) {
            anmObj->mapData[idx] = NNS_G3D_ANMOBJ_MAPDATA_EXIST | i;
        }
    }
}

// The function of an animation object of a material color animation: the colors and alpha of a material at the
// object's frame. The colors go in the material's color words, which keep their bit 15 (a flag of the diffuse and
// specular colors' words), and the alpha in the polygon attributes
void NNS_G3DAnimationApply_MA(NNSG3dMatAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx) {
    const NNSG3dResMatCAnm *matAnm = anmObj->resAnm;
    u32 frame = anmObj->frame >> FX32_SHIFT;
    const NNSG3dResMatCAnmData *data = NNS_G3dGetResDataByIdx(&matAnm->dict, (u16)dataIdx);
    u16 emission;
    u16 specular;
    u16 diffuse = getColorData_(matAnm, data->diffuse, frame);
    u16 ambient = getColorData_(matAnm, data->ambient, frame);

    result->prmMatColor0 =
        GX_PACK_DIFFAMB_PARAM(diffuse, ambient, result->prmMatColor0 & (1 << REG_G3_DIF_AMB_C_SHIFT));
    emission = getColorData_(matAnm, data->emission, frame);
    specular = getColorData_(matAnm, data->specular, frame);
    result->prmMatColor1 =
        GX_PACK_SPECEMI_PARAM(specular, emission, result->prmMatColor1 & (1 << REG_G3_SPE_EMI_S_SHIFT));
    result->prmPolygonAttr = (result->prmPolygonAttr & ~(0x1f << REG_G3_POLYGON_ATTR_ALPHA_SHIFT)) |
                             (getAlphaData_(matAnm, data->polygonAlpha, frame) << REG_G3_POLYGON_ATTR_ALPHA_SHIFT);
}
