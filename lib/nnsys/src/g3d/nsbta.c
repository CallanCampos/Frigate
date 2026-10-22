#include "nnsys/g3d.h"

// NitroSystem's G3D texture SRT animation (NSBTA, the "BTA0" files): the readers of the animation's values, the
// function that works out a material's texture matrix at a frame, the function that fills in an animation object for
// one, and the function the object holds. The file is named after NitroSystem's nsbta.c. The two global functions are
// the ones that anm.c's tables hold; the others are static because only this file calls them. The names are
// NitroSystem's, as ntrtwl's reconstruction of NitroSystem has them, except SinCos, which is ours

// A rotation as the animation keeps it: its sine and cosine, which are also read as one word
typedef union {
    struct {
        fx16 sinR;
        fx16 cosR;
    };
    u32 word;
} SinCos;

static fx32 GetTexSRTAnmVectorVal_(const NNSG3dResTexSRTAnm *anm, u32 info, u32 offset, u32 frame);
static u32 GetTexSRTAnmSinCosVal_(const NNSG3dResTexSRTAnm *anm, u32 info, u32 offset, u32 frame);
static void GetTexSRTAnm_(const NNSG3dResTexSRTAnm *anm, u32 idx, u32 frame, NNSG3dMatAnmResult *result);

// The value of a scale or translation at a frame, from the words that say how its values are kept. The frames between
// the stored ones are made from them, up to the last frame that is interpolated. As in the rotation's reader, the two
// cases between stored values jump to one block at the end, `average`
static fx32 GetTexSRTAnmVectorVal_(const NNSG3dResTexSRTAnm *anm, u32 info, u32 offset, u32 frame) {
    const u8 *data;
    u32 last;
    u32 idx;

    if (info & NNS_G3D_TEXSRTANM_ELEM_CONST) {
        return offset;
    }
    data = (const u8 *)anm + offset;
    if (info & NNS_G3D_TEXSRTANM_ELEM_STEP_MASK) {
        last = info & NNS_G3D_TEXSRTANM_ELEM_LAST_INTERP_MASK;
        if (info & NNS_G3D_TEXSRTANM_ELEM_STEP_2) {
            if (frame & 1) {
                if (frame > last) {
                    frame = (last >> 1) + 1;
                } else {
                    idx = frame >> 1;
                    goto average;
                }
            } else {
                frame >>= 1;
            }
        } else {
            u32 rem = frame & 3;

            if (rem != 0) {
                if (frame > last) {
                    frame = (last >> 2) + rem;
                } else if (frame & 1) {
                    // The quarters next to a stored value are three parts of it and one of the other
                    u32 idxB;
                    fx32 a;
                    fx32 b;

                    if (frame & 2) {
                        idxB = frame >> 2;
                        idx = idxB + 1;
                    } else {
                        idx = frame >> 2;
                        idxB = idx + 1;
                    }
                    if (info & NNS_G3D_TEXSRTANM_ELEM_FX16) {
                        a = ((const fx16 *)data)[idx];
                        b = ((const fx16 *)data)[idxB];
                    } else {
                        a = ((const fx32 *)data)[idx];
                        b = ((const fx32 *)data)[idxB];
                    }
                    a *= 3;
                    return (a + b) >> 2;
                } else {
                    idx = frame >> 2;
                    goto average;
                }
            } else {
                frame >>= 2;
            }
        }
    }
    if (info & NNS_G3D_TEXSRTANM_ELEM_FX16) {
        return ((const fx16 *)data)[frame];
    }
    return ((const fx32 *)data)[frame];

average:
    // The frame between two stored values is their average
    {
        fx32 a;
        fx32 b;

        if (info & NNS_G3D_TEXSRTANM_ELEM_FX16) {
            a = *((const fx16 *)data + idx);
            b = *((const fx16 *)data + idx + 1);
        } else {
            a = *((const fx32 *)data + idx);
            b = *((const fx32 *)data + idx + 1);
        }
        return (a + b) >> 1;
    }
}

// The same for a rotation, the sine and the cosine as one word, the cosine in the high half. Each is blended by itself
static u32 GetTexSRTAnmSinCosVal_(const NNSG3dResTexSRTAnm *anm, u32 info, u32 offset, u32 frame) {
    const SinCos *data;
    const SinCos *a;
    u32 last;
    u32 idx;

    if (info & NNS_G3D_TEXSRTANM_ELEM_CONST) {
        return offset;
    }
    data = (const SinCos *)((const u8 *)anm + offset);
    if (info & NNS_G3D_TEXSRTANM_ELEM_STEP_MASK) {
        last = info & NNS_G3D_TEXSRTANM_ELEM_LAST_INTERP_MASK;
        if (info & NNS_G3D_TEXSRTANM_ELEM_STEP_2) {
            if (frame & 1) {
                if (frame > last) {
                    frame = (last >> 1) + 1;
                } else {
                    idx = frame >> 1;
                    goto average;
                }
            } else {
                frame >>= 1;
            }
        } else {
            u32 rem = frame & 3;

            if (rem != 0) {
                if (frame > last) {
                    frame = (last >> 2) + rem;
                } else if (frame & 1) {
                    u32 idxB;
                    fx32 sinR;
                    fx32 cosR;

                    if (frame & 2) {
                        idxB = frame >> 2;
                        idx = idxB + 1;
                    } else {
                        idx = frame >> 2;
                        idxB = idx + 1;
                    }
                    sinR = data[idx].sinR;
                    cosR = data[idx].cosR;
                    sinR = (3 * sinR + data[idxB].sinR) >> 2;
                    cosR = (3 * cosR + data[idxB].cosR) >> 2;
                    return (u16)sinR | (cosR << 16);
                } else {
                    idx = frame >> 2;
                    goto average;
                }
            } else {
                frame >>= 2;
            }
        }
    }
    return data[frame].word;

average:
    // The frame between two stored values is their average
    a = data + idx;
    return (u16)((a->sinR + (a + 1)->sinR) >> 1) | (((a->cosR + (a + 1)->cosR) >> 1) << 16);
}

// Sets the texture matrix of a material in a result for the data at idx, at a frame: the scale, rotation and
// translation are kept only if they are not the identity, which is flagged otherwise
static void GetTexSRTAnm_(const NNSG3dResTexSRTAnm *anm, u32 idx, u32 frame, NNSG3dMatAnmResult *result) {
    const NNSG3dResDictTexSRTAnmData *data = NNS_G3dGetResDataByIdx(&anm->dict, idx);
    u32 flag = result->flag;
    fx32 transS = GetTexSRTAnmVectorVal_(anm, data->transS.info, data->transS.offset, frame);
    fx32 transT = GetTexSRTAnmVectorVal_(anm, data->transT.info, data->transT.offset, frame);
    u32 rot;
    fx32 scaleS;
    fx32 scaleT;

    if (transS == 0 && transT == 0) {
        flag |= NNS_G3D_MATANM_RESULTFLAG_TEXMTX_TRANSZERO;
    } else {
        flag &= ~NNS_G3D_MATANM_RESULTFLAG_TEXMTX_TRANSZERO;
        result->transS = transS;
        result->transT = transT;
    }

    rot = GetTexSRTAnmSinCosVal_(anm, data->rot.info, data->rot.offset, frame);
    if (rot == NNS_G3D_SRTANM_ROT_ZERO) {
        flag |= NNS_G3D_MATANM_RESULTFLAG_TEXMTX_ROTZERO;
    } else {
        result->sinR = rot;
        result->cosR = rot >> 16;
        flag &= ~NNS_G3D_MATANM_RESULTFLAG_TEXMTX_ROTZERO;
    }

    scaleS = GetTexSRTAnmVectorVal_(anm, data->scaleS.info, data->scaleS.offset, frame);
    scaleT = GetTexSRTAnmVectorVal_(anm, data->scaleT.info, data->scaleT.offset, frame);
    if (scaleS == FX32_ONE && scaleT == FX32_ONE) {
        flag |= NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SCALEONE;
    } else {
        flag &= ~NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SCALEONE;
        result->scaleS = scaleS;
        result->scaleT = scaleT;
    }
    result->flag = flag;
}

// Fills in an animation object for a texture SRT animation. It applies the animation with
// NNS_G3dFuncAnmMatNsBtaDefault, and maps each material of the model to the data of the animation's material of the
// same name, which is flagged as existing and holds the data's index
void NNSi_G3dAnmObjInitNsBta(NNSG3dAnmObj *anmObj, void *anm, const NNSG3dResMdl *mdl) {
    const NNSG3dResTexSRTAnm *srtAnm = anm;
    const NNSG3dResMat *mat = NNS_G3dGetMat(mdl);
    u32 i;

    anmObj->funcAnm = (void *)NNS_G3dFuncAnmMatNsBtaDefault;
    anmObj->numMapData = mdl->info.numMat;
    MI_CpuClear16(anmObj->mapData, anmObj->numMapData * sizeof(u16));
    for (i = 0; i < srtAnm->dict.numEntry; i++) {
        const NNSG3dResName *name = NNS_G3dGetResNameByIdx(&srtAnm->dict, i);
        int idx = mat != NULL ? NNS_G3dGetResDictIdxByName(&mat->dict, name) : -1;

        if (idx >= 0) {
            anmObj->mapData[idx] = NNS_G3D_ANMOBJ_MAPDATA_EXIST | i;
        }
    }
}

// The function of an animation object of a texture SRT animation: the material's texture matrix at the object's frame,
// with the texture coordinates taken as they are (texture coordinate generation mode one)
void NNSi_G3dAnmCalcNsBta(NNSG3dMatAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx) {
    GetTexSRTAnm_(anmObj->resAnm, (u16)dataIdx, anmObj->frame >> FX32_SHIFT, result);
    result->prmTexImage = (result->prmTexImage & ~(3 << REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT)) |
                          (GX_TEXGEN_TEXCOORD << REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT);
    result->flag |= NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SET;
}
