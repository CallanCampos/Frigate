#include "nnsys/g3d.h"

// NitroSystem's G3D joint animation (NSBCA, the "BCA0" files): the function that fills in an animation object for one,
// the function the object holds, which works out a joint's pose at a frame, and the readers of the animation's values.
// A joint's translation, rotation and scale are each the identity, the node's own, a constant of the animation or a
// curve of values read at the frame, blended with the next frame's where the animation says so. The file is named after
// NitroSystem's nsbca.c. The two global functions are the ones that anm.c's tables hold; the others are static because
// only this file calls them. The names are NitroSystem's, as ntrtwl's reconstruction of NitroSystem has them, except
// ScaleData, ScaleData16, makeRotRow2_ and NNS_COMPACT_ROT_LUT_CA, which are ours, the last after swan's
// NNS_COMPACT_ROT_LUT in sbc.c

// A scale component as the animation keeps it, the scale and its inverse, in fx32 or in fx16
typedef struct {
    fx32 scale;
    fx32 invScale;
} ScaleData;

typedef struct {
    fx16 scale;
    fx16 invScale;
} ScaleData16;

static void getJntSRTAnmResult_(const NNSG3dResJntAnm *jntAnm, u32 dataIdx, fx32 frame, NNSG3dJntAnmResult *result);
static void getMdlTrans_(NNSG3dJntAnmResult *result);
static void getMdlScale_(NNSG3dJntAnmResult *result);
static void getMdlRot_(NNSG3dJntAnmResult *result);
static void getTransData_(fx32 *value, fx32 frame, const NNSG3dJntAnmTInfo *tInfo, const NNSG3dResJntAnm *jntAnm);
static void getTransDataEx_(fx32 *value, fx32 frame, const NNSG3dJntAnmTInfo *tInfo, const NNSG3dResJntAnm *jntAnm);
static void getScaleData_(ScaleData *value, fx32 frame, const NNSG3dJntAnmTInfo *tInfo, const NNSG3dResJntAnm *jntAnm);
static void getScaleDataEx_(ScaleData *value, fx32 frame, const NNSG3dJntAnmTInfo *tInfo,
                            const NNSG3dResJntAnm *jntAnm);
static void getRotData_(VecFx32 *rot, fx32 frame, const NNSG3dJntAnmTInfo *tInfo, const NNSG3dResJntAnm *jntAnm);
static void getRotDataEx_(VecFx32 *rot, fx32 frame, const NNSG3dJntAnmTInfo *tInfo, const NNSG3dResJntAnm *jntAnm);
static BOOL getRotDataByIdx_(VecFx32 *rot, const void *rot3Table, const void *rot5Table, NNSG3dJntAnmRIdx rInfo);

// For a rotation stored as its pivot (the one element that is 1 or -1, the others in its row and column being zero)
// and two numbers, the pivot's index and where the numbers go in the matrix. sbc.c has the same table
static const u8 NNS_COMPACT_ROT_LUT_CA[9][4] = {
    { 4, 5, 7, 8 }, { 3, 5, 6, 8 }, { 3, 4, 6, 7 }, { 1, 2, 7, 8 }, { 0, 2, 6, 8 },
    { 0, 1, 6, 7 }, { 1, 2, 4, 5 }, { 0, 2, 3, 5 }, { 0, 1, 3, 4 },
};

// Makes the third row of a rotation from the first two rows, as their cross product
static inline void makeRotRow2_(VecFx32 *rot) {
    fx32 z1 = rot[1].z;
    fx32 x0 = rot[0].x;
    fx32 x1 = rot[1].x;
    fx32 z0 = rot[0].z;
    fx32 y0 = rot[0].y;
    fx32 y1 = rot[1].y;

    rot[2].x = (y0 * z1 - z0 * y1) >> FX32_SHIFT;
    rot[2].y = (z0 * x1 - x0 * z1) >> FX32_SHIFT;
    rot[2].z = (x0 * y1 - y0 * x1) >> FX32_SHIFT;
}

// Fills in an animation object for a joint animation. It applies the animation with NNS_G3dFuncAnmJntNsBcaDefault, and
// maps each node of the model to the data at its index, which is flagged as existing and holds the index that the top
// byte of the data's tag gives
void NNSi_G3dAnmObjInitNsBca(NNSG3dAnmObj *anmObj, void *anm, const NNSG3dResMdl *mdl) {
    const NNSG3dResJntAnm *jntAnm = anm;
    const u16 *ofsTag;
    u32 i;

    anmObj->resAnm = anm;
    anmObj->funcAnm = (void *)NNS_G3dFuncAnmJntNsBcaDefault;
    anmObj->numMapData = mdl->info.numNode;
    MI_CpuClear16(anmObj->mapData, anmObj->numMapData * sizeof(u16));
    ofsTag = jntAnm->ofsTag;
    for (i = 0; i < jntAnm->numNode; i++) {
        u32 tag = *(const u32 *)((const u8 *)jntAnm + *ofsTag++);

        anmObj->mapData[i] = NNS_G3D_ANMOBJ_MAPDATA_EXIST | (tag >> NNS_G3D_JNTANM_SRTINFO_NODE_SHIFT);
    }
}

// The function of an animation object of a joint animation: the pose of a joint at the object's frame, kept within the
// animation's frames
void NNSi_G3dAnmCalcNsBca(NNSG3dJntAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx) {
    const NNSG3dResJntAnm *jntAnm = anmObj->resAnm;
    fx32 numFrame = jntAnm->numFrame << FX32_SHIFT;
    fx32 frame = anmObj->frame;

    if (frame >= numFrame) {
        frame = numFrame - 1;
    } else if (frame < 0) {
        frame = 0;
    }
    getJntSRTAnmResult_(jntAnm, dataIdx, frame, result);
}

// What the node that the render state is at (the node of the command being run) has of its own, for a joint animation
// that leaves its translation, rotation or scale to the node: the node's translation and rotation, and the scale
// through the render state's scaling rule
static void getMdlTrans_(NNSG3dJntAnmResult *result) {
    NNSG3dRS *rs = NNS_G3dRS;
    const NNSG3dResNodeData *node = NNS_G3dGetNodeDataByIdx(rs->pResNodeInfo, rs->c[1]);

    if (node->flag & NNS_G3D_SRTFLAG_TRANS_ZERO) {
        result->flag |= NNS_G3D_JNTANM_RESULTFLAG_TRANS_ZERO;
    } else {
        const fx32 *trans = (const fx32 *)(node + 1);

        result->trans.x = trans[0];
        result->trans.y = trans[1];
        result->trans.z = trans[2];
    }
}

static void getMdlScale_(NNSG3dJntAnmResult *result) {
    NNSG3dRS *rs = NNS_G3dRS;
    const u8 *cmd = rs->c;
    const NNSG3dResNodeData *node = NNS_G3dGetNodeDataByIdx(rs->pResNodeInfo, cmd[1]);
    const u8 *data = (const u8 *)(node + 1);

    if (!(node->flag & NNS_G3D_SRTFLAG_TRANS_ZERO)) {
        data += sizeof(VecFx32);
    }
    if (!(node->flag & NNS_G3D_SRTFLAG_ROT_ZERO)) {
        if (node->flag & NNS_G3D_SRTFLAG_PIVOT_EXIST) {
            data += 2 * sizeof(fx16);
        } else {
            data += 8 * sizeof(fx16);
        }
    }
    rs->funcJntScale(result, (const fx32 *)data, cmd, node->flag);
}

static void getMdlRot_(NNSG3dJntAnmResult *result) {
    NNSG3dRS *rs = NNS_G3dRS;
    const NNSG3dResNodeData *node = NNS_G3dGetNodeDataByIdx(rs->pResNodeInfo, rs->c[1]);
    const u8 *data = (const u8 *)(node + 1);

    if (!(node->flag & NNS_G3D_SRTFLAG_TRANS_ZERO)) {
        data += sizeof(VecFx32);
    }

    if (!(node->flag & NNS_G3D_SRTFLAG_ROT_ZERO)) {
        if (node->flag & NNS_G3D_SRTFLAG_PIVOT_EXIST) {
            const fx16 *rot = (const fx16 *)data;
            u32 pivot;
            fx32 b;
            fx32 a;

            a = rot[0];
            b = rot[1];
            pivot = (node->flag & NNS_G3D_SRTFLAG_IDXPIVOT_MASK) >> NNS_G3D_SRTFLAG_IDXPIVOT_SHIFT;

            MI_Zero36B(&result->rot);
            result->rotElem[pivot] = (node->flag & NNS_G3D_SRTFLAG_PIVOT_MINUS) ? -FX32_ONE : FX32_ONE;
            result->rotElem[NNS_COMPACT_ROT_LUT_CA[pivot][0]] = a;
            result->rotElem[NNS_COMPACT_ROT_LUT_CA[pivot][1]] = b;
            if (node->flag & NNS_G3D_SRTFLAG_SIGN_REVC) {
                b = -b;
            }
            result->rotElem[NNS_COMPACT_ROT_LUT_CA[pivot][2]] = b;
            if (node->flag & NNS_G3D_SRTFLAG_SIGN_REVD) {
                a = -a;
            }
            result->rotElem[NNS_COMPACT_ROT_LUT_CA[pivot][3]] = a;
        } else {
            const fx16 *rot = (const fx16 *)data;

            result->rotElem[0] = node->_00;
            result->rotElem[1] = rot[0];
            result->rotElem[2] = rot[1];
            result->rotElem[3] = rot[2];
            result->rotElem[4] = rot[3];
            result->rotElem[5] = rot[4];
            result->rotElem[6] = rot[5];
            result->rotElem[7] = rot[6];
            result->rotElem[8] = rot[7];
        }
    } else {
        result->flag |= NNS_G3D_JNTANM_RESULTFLAG_ROT_ZERO;
    }
}

// The pose of a joint at a frame, from the data at dataIdx of the animation. The tag of the data starts with a flag
// word, the SRTINFO, that says for the translation, the rotation and the scale whether it is the identity, the node's
// own or the animation's. What it has of the animation's follows the word: for each of the three translation
// components, the rotation and the three scale components, a constant or the TInfo of the values. The values at a
// frame with a fraction are blended with the next frame's if the animation says to
static void getJntSRTAnmResult_(const NNSG3dResJntAnm *jntAnm, u32 dataIdx, fx32 frame, NNSG3dJntAnmResult *result) {
    const u32 *tagPtr;
    const u32 *p;
    BOOL interpolate;
    u32 tag;
    fx32 scale[6];
    ScaleData scaleX;
    ScaleData scaleY;
    ScaleData scaleZ;

    tagPtr = (const u32 *)((const u8 *)jntAnm + jntAnm->ofsTag[dataIdx]);
    tag = *tagPtr;
    interpolate = TRUE;

    if (tag & NNS_G3D_JNTANM_SRTINFO_IDENTITY) {
        result->flag = NNS_G3D_JNTANM_RESULTFLAG_SCALE_ONE | NNS_G3D_JNTANM_RESULTFLAG_ROT_ZERO |
                       NNS_G3D_JNTANM_RESULTFLAG_TRANS_ZERO;
    } else {
        p = tagPtr + 1;
        if ((frame & 0xfff) == 0 || !(jntAnm->flag & NNS_G3D_JNTANM_OPTION_INTERPOLATION)) {
            interpolate = FALSE;
        }
        result->flag = 0;

        if (!(tag & (NNS_G3D_JNTANM_SRTINFO_IDENTITY_T | NNS_G3D_JNTANM_SRTINFO_BASE_T))) {
            if (!(tag & NNS_G3D_JNTANM_SRTINFO_CONST_TX)) {
                if (interpolate) {
                    getTransDataEx_(&result->trans.x, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                } else {
                    getTransData_(&result->trans.x, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                }
                p += 2;
            } else {
                result->trans.x = *p;
                p++;
            }

            if (!(tag & NNS_G3D_JNTANM_SRTINFO_CONST_TY)) {
                if (interpolate) {
                    getTransDataEx_(&result->trans.y, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                } else {
                    getTransData_(&result->trans.y, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                }
                p += 2;
            } else {
                result->trans.y = *p;
                p++;
            }

            if (!(tag & NNS_G3D_JNTANM_SRTINFO_CONST_TZ)) {
                if (interpolate) {
                    getTransDataEx_(&result->trans.z, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                } else {
                    getTransData_(&result->trans.z, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                }
                p += 2;
            } else {
                result->trans.z = *p;
                p++;
            }
        } else if (tag & NNS_G3D_JNTANM_SRTINFO_IDENTITY_T) {
            result->flag |= NNS_G3D_JNTANM_RESULTFLAG_TRANS_ZERO;
        } else {
            getMdlTrans_(result);
        }

        if (!(tag & (NNS_G3D_JNTANM_SRTINFO_IDENTITY_R | NNS_G3D_JNTANM_SRTINFO_BASE_R))) {
            if (!(tag & NNS_G3D_JNTANM_SRTINFO_CONST_R)) {
                if (interpolate) {
                    getRotDataEx_(result->rotRow, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                } else {
                    getRotData_(result->rotRow, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                }
                p += 2;
            } else {
                if (getRotDataByIdx_(result->rotRow, (const u8 *)jntAnm + jntAnm->ofsRot3,
                                     (const u8 *)jntAnm + jntAnm->ofsRot5, *p)) {
                    makeRotRow2_(result->rotRow);
                }
                p++;
            }
        } else if (tag & NNS_G3D_JNTANM_SRTINFO_IDENTITY_R) {
            result->flag |= NNS_G3D_JNTANM_RESULTFLAG_ROT_ZERO;
        } else {
            getMdlRot_(result);
        }

        if (!(tag & (NNS_G3D_JNTANM_SRTINFO_IDENTITY_S | NNS_G3D_JNTANM_SRTINFO_BASE_S))) {
            if (!(tag & NNS_G3D_JNTANM_SRTINFO_CONST_SX)) {
                if (interpolate) {
                    getScaleDataEx_(&scaleX, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                } else {
                    getScaleData_(&scaleX, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                }
                scale[0] = scaleX.scale;
                scale[3] = scaleX.invScale;
            } else {
                const ScaleData *c = (const ScaleData *)p;

                scale[0] = c->scale;
                scale[3] = c->invScale;
            }
            p += 2;

            if (!(tag & NNS_G3D_JNTANM_SRTINFO_CONST_SY)) {
                if (interpolate) {
                    getScaleDataEx_(&scaleY, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                } else {
                    getScaleData_(&scaleY, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                }
                scale[1] = scaleY.scale;
                scale[4] = scaleY.invScale;
            } else {
                const ScaleData *c = (const ScaleData *)p;

                scale[1] = c->scale;
                scale[4] = c->invScale;
            }
            p += 2;

            if (!(tag & NNS_G3D_JNTANM_SRTINFO_CONST_SZ)) {
                if (interpolate) {
                    getScaleDataEx_(&scaleZ, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                } else {
                    getScaleData_(&scaleZ, frame, (const NNSG3dJntAnmTInfo *)p, jntAnm);
                }
                scale[2] = scaleZ.scale;
                scale[5] = scaleZ.invScale;
            } else {
                const ScaleData *c = (const ScaleData *)p;

                scale[2] = c->scale;
                scale[5] = c->invScale;
            }
        } else if (tag & NNS_G3D_JNTANM_SRTINFO_IDENTITY_S) {
            result->flag |= NNS_G3D_JNTANM_RESULTFLAG_SCALE_ONE;
        } else {
            getMdlScale_(result);
            return;
        }
    }

    NNS_G3dRS->funcJntScale(result, scale, NNS_G3dRS->c,
                            (result->flag & NNS_G3D_JNTANM_RESULTFLAG_SCALE_ONE) ? NNS_G3D_SRTFLAG_SCALE_ONE : 0);
}

// The value of a translation component at the frame, without its fraction. With a step of two or four the animation
// keeps a value for every second or fourth frame, and the frames between are made from the values around them, up to
// the last frame that is interpolated. This reader and the others end in blocks that several of their cases jump to:
// `average` makes a frame between two stored values and `read` reads a stored one, and at a frame with a fraction
// `plain` sets up the blend of a frame without a step and `blend` blends it with the next
static void getTransData_(fx32 *value, fx32 frame, const NNSG3dJntAnmTInfo *tInfo, const NNSG3dResJntAnm *jntAnm) {
    u32 f = frame >> FX32_SHIFT;
    const void *data = (const u8 *)jntAnm + tInfo->offset;
    u32 info = tInfo->info;
    u32 last;
    u32 idx;

    if (!(info & NNS_G3D_JNTANM_TINFO_STEP_MASK)) {
        idx = f;
        goto read;
    }

    last = (info & NNS_G3D_JNTANM_TINFO_LAST_INTERP_MASK) >> NNS_G3D_JNTANM_TINFO_LAST_INTERP_SHIFT;
    if (info & NNS_G3D_JNTANM_TINFO_STEP_2) {
        if (f & 1) {
            if (f > last) {
                idx = (last >> 1) + 1;
                goto read;
            }
            idx = f >> 1;
            goto average;
        }
        idx = f >> 1;
        goto read;
    }
    if (f & 3) {
        if (f > last) {
            idx = (last >> 2) + (f & 3);
            goto read;
        }
        if (f & 1) {
            u32 idxB;
            fx32 a;
            fx32 b;

            // The quarters next to a value are three parts of it and one of the other
            if (f & 2) {
                idxB = f >> 2;
                idx = idxB + 1;
            } else {
                idx = f >> 2;
                idxB = idx + 1;
            }
            if (info & NNS_G3D_JNTANM_TINFO_FX16ARRAY) {
                const fx16 *v = data;

                a = v[idx];
                b = v[idxB];
                *value = (a + a + a + b) >> 2;
            } else {
                const fx32 *v = data;

                a = v[idx];
                b = v[idxB];
                *value = ((fx64)a + a + a + b) >> 2;
            }
            return;
        }
        idx = f >> 2;
        goto average;
    }
    idx = f >> 2;
    goto read;

average:
    // The frame between two stored values is their average
    if (info & NNS_G3D_JNTANM_TINFO_FX16ARRAY) {
        const fx16 *v = data;

        *value = (*(v + idx) + *(v + idx + 1)) >> 1;
    } else {
        const fx32 *v = data;

        *value = (*(v + idx) >> 1) + (*(v + idx + 1) >> 1);
    }
    return;

read:
    if (info & NNS_G3D_JNTANM_TINFO_FX16ARRAY) {
        *value = ((const fx16 *)data)[idx];
    } else {
        *value = ((const fx32 *)data)[idx];
    }
}

// The same at a frame with a fraction: the value is blended with the one of the next frame, by the fraction. The last
// frame blends with the first if the animation says to
static void getTransDataEx_(fx32 *value, fx32 frame, const NNSG3dJntAnmTInfo *tInfo, const NNSG3dResJntAnm *jntAnm) {
    const void *data = (const u8 *)jntAnm + tInfo->offset;
    u32 info = tInfo->info;
    u32 f = frame >> FX32_SHIFT;
    u32 last;
    fx32 ratio;
    int mul;
    int shift;
    fx32 v0;
    fx32 v1;

    if (f == jntAnm->numFrame - 1) {
        if (info & NNS_G3D_JNTANM_TINFO_STEP_MASK) {
            if (info & NNS_G3D_JNTANM_TINFO_STEP_2) {
                f = (f >> 1) + (f & 1);
            } else {
                f = (f >> 2) + (f & 3);
            }
        }
        if (jntAnm->flag & NNS_G3D_JNTANM_OPTION_END_TO_START_INTERPOLATION) {
            ratio = frame & 0xfff;
            if (info & NNS_G3D_JNTANM_TINFO_FX16ARRAY) {
                v0 = ((const fx16 *)data)[f];
                v1 = ((const fx16 *)data)[0];
            } else {
                v0 = ((const fx32 *)data)[f];
                v1 = ((const fx32 *)data)[0];
            }
            *value = v0 + (((v1 - v0) * ratio) >> FX32_SHIFT);
        } else if (info & NNS_G3D_JNTANM_TINFO_FX16ARRAY) {
            *value = ((const fx16 *)data)[f];
        } else {
            *value = ((const fx32 *)data)[f];
        }
        return;
    }

    if (info & NNS_G3D_JNTANM_TINFO_STEP_MASK) {
        last = (info & NNS_G3D_JNTANM_TINFO_LAST_INTERP_MASK) >> NNS_G3D_JNTANM_TINFO_LAST_INTERP_SHIFT;
        if (info & NNS_G3D_JNTANM_TINFO_STEP_2) {
            if (f >= last) {
                f = last >> 1;
                goto plain;
            }
            f >>= 1;
            ratio = frame & 0x1fff;
            mul = 2;
            shift = 1;
            goto blend;
        }
        if (f >= last) {
            f = (f >> 2) + (f & 3);
            goto plain;
        }
        f >>= 2;
        ratio = frame & 0x3fff;
        mul = 4;
        shift = 2;
        goto blend;
    }

plain:
    ratio = frame & 0xfff;
    mul = 1;
    shift = 0;

blend:
    if (info & NNS_G3D_JNTANM_TINFO_FX16ARRAY) {
        const fx16 *v = data;

        v0 = *(v + f);
        v1 = *(v + f + 1);
    } else {
        const fx32 *v = data;

        v0 = *(v + f);
        v1 = *(v + f + 1);
    }
    *value = (v0 * mul + (((v1 - v0) * ratio) >> FX32_SHIFT)) >> shift;
}

// The same for the two values of a scale component, the scale and its inverse
static void getScaleData_(ScaleData *value, fx32 frame, const NNSG3dJntAnmTInfo *tInfo, const NNSG3dResJntAnm *jntAnm) {
    u32 f = frame >> FX32_SHIFT;
    const void *data = (const u8 *)jntAnm + tInfo->offset;
    u32 info = tInfo->info;
    u32 idx;

    if (info & NNS_G3D_JNTANM_TINFO_STEP_MASK) {
        u32 last = (info & NNS_G3D_JNTANM_TINFO_LAST_INTERP_MASK) >> NNS_G3D_JNTANM_TINFO_LAST_INTERP_SHIFT;

        if (info & NNS_G3D_JNTANM_TINFO_STEP_2) {
            if (f & 1) {
                if (f > last) {
                    idx = (last >> 1) + 1;
                } else {
                    idx = f >> 1;
                    goto average;
                }
            } else {
                idx = f >> 1;
            }
        } else {
            u32 rem = f & 3;

            if (rem != 0) {
                if (f > last) {
                    idx = (last >> 2) + rem;
                } else if (f & 1) {
                    // The quarters next to a value are three parts of it and one of the other
                    u32 idxB;
                    fx32 a;
                    fx32 b;

                    if (f & 2) {
                        idxB = f >> 2;
                        idx = idxB + 1;
                    } else {
                        idx = f >> 2;
                        idxB = idx + 1;
                    }
                    if (info & NNS_G3D_JNTANM_TINFO_FX16ARRAY) {
                        const ScaleData16 *v = data;

                        a = v[idx].scale;
                        b = v[idxB].scale;
                        value->scale = (a + (a << 1) + b) >> 2;
                        a = v[idx].invScale;
                        b = v[idxB].invScale;
                        value->invScale = (a + (a << 1) + b) >> 2;
                    } else {
                        const ScaleData *v = data;

                        a = v[idx].scale;
                        b = v[idxB].scale;
                        value->scale = ((fx64)a + a + a + b) >> 2;
                        a = v[idx].invScale;
                        b = v[idxB].invScale;
                        value->invScale = ((fx64)a + a + a + b) >> 2;
                    }
                    return;
                } else {
                    idx = f >> 2;
                    goto average;
                }
            } else {
                idx = f >> 2;
            }
        }
    } else {
        idx = f;
    }
    if (info & NNS_G3D_JNTANM_TINFO_FX16ARRAY) {
        const ScaleData16 *v = data;

        value->scale = v[idx].scale;
        value->invScale = v[idx].invScale;
    } else {
        const ScaleData *v = data;

        value->scale = v[idx].scale;
        value->invScale = v[idx].invScale;
    }
    return;

average:
    // The frame between two stored values is their average
    if (info & NNS_G3D_JNTANM_TINFO_FX16ARRAY) {
        const ScaleData16 *v = data;

        value->scale = ((v + idx)->scale + (v + idx + 1)->scale) >> 1;
        value->invScale = ((v + idx)->invScale + (v + idx + 1)->invScale) >> 1;
    } else {
        const ScaleData *v = data;

        value->scale = ((v + idx)->scale + (v + idx + 1)->scale) >> 1;
        value->invScale = ((v + idx)->invScale + (v + idx + 1)->invScale) >> 1;
    }
}

static void getScaleDataEx_(ScaleData *value, fx32 frame, const NNSG3dJntAnmTInfo *tInfo,
                            const NNSG3dResJntAnm *jntAnm) {
    const void *data = (const u8 *)jntAnm + tInfo->offset;
    u32 info = tInfo->info;
    u32 f = frame >> FX32_SHIFT;
    u32 last;
    u32 next;
    fx32 ratio;
    int mul;
    int shift;
    fx32 v0;
    fx32 w0;
    fx32 v1;
    fx32 w1;

    if (f == jntAnm->numFrame - 1) {
        if (info & NNS_G3D_JNTANM_TINFO_STEP_MASK) {
            if (info & NNS_G3D_JNTANM_TINFO_STEP_2) {
                f = (f >> 1) + (f & 1);
            } else {
                f = (f >> 2) + (f & 3);
            }
        }
        if (jntAnm->flag & NNS_G3D_JNTANM_OPTION_END_TO_START_INTERPOLATION) {
            // The last frame blends with the first
            next = 0;
            goto plain;
        }
        if (info & NNS_G3D_JNTANM_TINFO_FX16ARRAY) {
            const ScaleData16 *v = data;

            value->scale = v[f].scale;
            value->invScale = v[f].invScale;
        } else {
            const ScaleData *v = data;

            value->scale = v[f].scale;
            value->invScale = v[f].invScale;
        }
        return;
    }

    if (info & NNS_G3D_JNTANM_TINFO_STEP_MASK) {
        last = (info & NNS_G3D_JNTANM_TINFO_LAST_INTERP_MASK) >> NNS_G3D_JNTANM_TINFO_LAST_INTERP_SHIFT;
        if (info & NNS_G3D_JNTANM_TINFO_STEP_2) {
            if (f >= last) {
                f = last >> 1;
                next = f + 1;
                goto plain;
            }
            f >>= 1;
            next = f + 1;
            ratio = frame & 0x1fff;
            mul = 2;
            shift = 1;
            goto blend;
        }
        if (f >= last) {
            f = (f >> 2) + (f & 3);
            next = f + 1;
            goto plain;
        }
        f >>= 2;
        next = f + 1;
        ratio = frame & 0x3fff;
        mul = 4;
        shift = 2;
        goto blend;
    }
    next = f + 1;

plain:
    ratio = frame & 0xfff;
    mul = 1;
    shift = 0;

blend:
    if (info & NNS_G3D_JNTANM_TINFO_FX16ARRAY) {
        const ScaleData16 *v = data;

        v0 = v[f].scale;
        w0 = v[f].invScale;
        v1 = v[next].scale;
        w1 = v[next].invScale;
    } else {
        const ScaleData *v = data;

        v0 = v[f].scale;
        w0 = v[f].invScale;
        v1 = v[next].scale;
        w1 = v[next].invScale;
    }
    value->scale = (v0 * mul + (((v1 - v0) * ratio) >> FX32_SHIFT)) >> shift;
    value->invScale = (w0 * mul + (((w1 - w0) * ratio) >> FX32_SHIFT)) >> shift;
}

// The same for the rotation, whose values are the indices of rotation matrices: the matrices of the frames around the
// frame are added up, with weights for the quarters, and normalized
static void getRotData_(VecFx32 *rot, fx32 frame, const NNSG3dJntAnmTInfo *tInfo, const NNSG3dResJntAnm *jntAnm) {
    u32 ofsRot5;
    u32 ofsRot3;
    u32 f;
    u32 info;
    const u16 *data;
    u32 last;
    u32 idx;

    ofsRot3 = jntAnm->ofsRot3;
    ofsRot5 = jntAnm->ofsRot5;
    f = frame >> FX32_SHIFT;
    info = tInfo->info;
    data = (const u16 *)((const u8 *)jntAnm + tInfo->offset);

    if (!(info & NNS_G3D_JNTANM_TINFO_STEP_MASK)) {
        idx = f;
        goto read;
    }

    last = (info & NNS_G3D_JNTANM_TINFO_LAST_INTERP_MASK) >> NNS_G3D_JNTANM_TINFO_LAST_INTERP_SHIFT;
    if (info & NNS_G3D_JNTANM_TINFO_STEP_2) {
        if (f & 1) {
            if (f > last) {
                idx = (last >> 1) + 1;
                goto read;
            }
            idx = f >> 1;
            goto average;
        }
        idx = f >> 1;
        goto read;
    }
    if (f & 3) {
        if (f > last) {
            idx = (last >> 2) + (f & 3);
            goto read;
        }
        if (f & 1) {
            VecFx32 tmp[3];
            BOOL rot5 = FALSE;
            u32 idxB;

            // The quarters next to a value are three parts of it and one of the other
            if (f & 2) {
                idxB = f >> 2;
                idx = idxB + 1;
            } else {
                idx = f >> 2;
                idxB = idx + 1;
            }
            rot5 |= getRotDataByIdx_(rot, (const u8 *)jntAnm + ofsRot3, (const u8 *)jntAnm + ofsRot5, data[idx]);
            rot5 |= getRotDataByIdx_(tmp, (const u8 *)jntAnm + ofsRot3, (const u8 *)jntAnm + ofsRot5, data[idxB]);
            rot[0].x = 3 * rot[0].x + tmp[0].x;
            rot[0].y = 3 * rot[0].y + tmp[0].y;
            rot[0].z = 3 * rot[0].z + tmp[0].z;
            rot[1].x = 3 * rot[1].x + tmp[1].x;
            rot[1].y = 3 * rot[1].y + tmp[1].y;
            rot[1].z = 3 * rot[1].z + tmp[1].z;
            VEC_Normalize(&rot[0], &rot[0]);
            VEC_Normalize(&rot[1], &rot[1]);
            if (!rot5) {
                rot[2].x = 3 * rot[2].x + tmp[2].x;
                rot[2].y = 3 * rot[2].y + tmp[2].y;
                rot[2].z = 3 * rot[2].z + tmp[2].z;
                VEC_Normalize(&rot[2], &rot[2]);
            } else {
                makeRotRow2_(rot);
            }
            return;
        }
        idx = f >> 2;
        goto average;
    }
    idx = f >> 2;
    goto read;

average:
    // The frame between two stored values is their average
    {
        VecFx32 tmp[3];
        BOOL rot5 = FALSE;

        rot5 |= getRotDataByIdx_(rot, (const u8 *)jntAnm + ofsRot3, (const u8 *)jntAnm + ofsRot5, data[idx]);
        rot5 |= getRotDataByIdx_(tmp, (const u8 *)jntAnm + ofsRot3, (const u8 *)jntAnm + ofsRot5, *(data + idx + 1));
        rot[0].x += tmp[0].x;
        rot[0].y += tmp[0].y;
        rot[0].z += tmp[0].z;
        rot[1].x += tmp[1].x;
        rot[1].y += tmp[1].y;
        rot[1].z += tmp[1].z;
        VEC_Normalize(&rot[0], &rot[0]);
        VEC_Normalize(&rot[1], &rot[1]);
        if (!rot5) {
            rot[2].x += tmp[2].x;
            rot[2].y += tmp[2].y;
            rot[2].z += tmp[2].z;
            VEC_Normalize(&rot[2], &rot[2]);
        } else {
            makeRotRow2_(rot);
        }
    }
    return;

read:
    if (getRotDataByIdx_(rot, (const u8 *)jntAnm + ofsRot3, (const u8 *)jntAnm + ofsRot5, data[idx])) {
        makeRotRow2_(rot);
    } else {
        VEC_Normalize(&rot[2], &rot[2]);
    }
}

// The same at a frame with a fraction: the matrices of the frame and the next are blended by the fraction and
// normalized. Where a step of two or four applies, the fraction counts the frames since the stored one
static void getRotDataEx_(VecFx32 *rot, fx32 frame, const NNSG3dJntAnmTInfo *tInfo, const NNSG3dResJntAnm *jntAnm) {
    u32 ofsRot5;
    u32 ofsRot3;
    u32 info;
    u32 f;
    u32 last;
    u32 next;
    const u16 *data;
    fx32 ratio;
    int mul;
    VecFx32 tmp0[3];
    VecFx32 tmp1[3];
    BOOL rot5;

    ofsRot3 = jntAnm->ofsRot3;
    ofsRot5 = jntAnm->ofsRot5;
    f = frame >> FX32_SHIFT;
    info = tInfo->info;
    data = (const u16 *)((const u8 *)jntAnm + tInfo->offset);

    if (f == jntAnm->numFrame - 1) {
        if (info & NNS_G3D_JNTANM_TINFO_STEP_MASK) {
            if (info & NNS_G3D_JNTANM_TINFO_STEP_2) {
                f = (f >> 1) + (f & 1);
            } else {
                f = (f >> 2) + (f & 3);
            }
        }
        if (jntAnm->flag & NNS_G3D_JNTANM_OPTION_END_TO_START_INTERPOLATION) {
            // The last frame blends with the first
            next = 0;
            goto plain;
        }
        if (getRotDataByIdx_(rot, (const u8 *)jntAnm + ofsRot3, (const u8 *)jntAnm + ofsRot5, data[f])) {
            makeRotRow2_(rot);
        } else {
            VEC_Normalize(&rot[2], &rot[2]);
        }
        return;
    }

    if (info & NNS_G3D_JNTANM_TINFO_STEP_MASK) {
        last = (info & NNS_G3D_JNTANM_TINFO_LAST_INTERP_MASK) >> NNS_G3D_JNTANM_TINFO_LAST_INTERP_SHIFT;
        if (info & NNS_G3D_JNTANM_TINFO_STEP_2) {
            if (f >= last) {
                f = last >> 1;
                next = f + 1;
                goto plain;
            }
            f >>= 1;
            next = f + 1;
            ratio = frame & 0x1fff;
            mul = 2;
            goto blend;
        }
        if (f >= last) {
            f = (f >> 2) + (f & 3);
            next = f + 1;
            goto plain;
        }
        f >>= 2;
        next = f + 1;
        ratio = frame & 0x3fff;
        mul = 4;
        goto blend;
    }
    next = f + 1;

plain:
    ratio = frame & 0xfff;
    mul = 1;

blend:
    rot5 = FALSE;
    rot5 |= getRotDataByIdx_(tmp0, (const u8 *)jntAnm + ofsRot3, (const u8 *)jntAnm + ofsRot5, data[f]);
    rot5 |= getRotDataByIdx_(tmp1, (const u8 *)jntAnm + ofsRot3, (const u8 *)jntAnm + ofsRot5, data[next]);
    rot[0].x = tmp0[0].x * mul + (((tmp1[0].x - tmp0[0].x) * ratio) >> FX32_SHIFT);
    rot[0].y = tmp0[0].y * mul + (((tmp1[0].y - tmp0[0].y) * ratio) >> FX32_SHIFT);
    rot[0].z = tmp0[0].z * mul + (((tmp1[0].z - tmp0[0].z) * ratio) >> FX32_SHIFT);
    rot[1].x = tmp0[1].x * mul + (((tmp1[1].x - tmp0[1].x) * ratio) >> FX32_SHIFT);
    rot[1].y = tmp0[1].y * mul + (((tmp1[1].y - tmp0[1].y) * ratio) >> FX32_SHIFT);
    rot[1].z = tmp0[1].z * mul + (((tmp1[1].z - tmp0[1].z) * ratio) >> FX32_SHIFT);
    VEC_Normalize(&rot[0], &rot[0]);
    VEC_Normalize(&rot[1], &rot[1]);
    if (!rot5) {
        rot[2].x = tmp0[2].x * mul + (((tmp1[2].x - tmp0[2].x) * ratio) >> FX32_SHIFT);
        rot[2].y = tmp0[2].y * mul + (((tmp1[2].y - tmp0[2].y) * ratio) >> FX32_SHIFT);
        rot[2].z = tmp0[2].z * mul + (((tmp1[2].z - tmp0[2].z) * ratio) >> FX32_SHIFT);
        VEC_Normalize(&rot[2], &rot[2]);
    } else {
        makeRotRow2_(rot);
    }
}

// Makes the rotation matrix that an index stands for, from the animation's table of rotations of three values (a
// pivot and two numbers: bits of the first say which element is the pivot, whether it is -1, and which of the numbers
// to negate) or of five (the first two rows' elements but one, 13 bits each with the rest of the bits of the sixth
// in the low three of them). Returns whether the third row still has to be made from the first two, which a rotation
// of five values leaves to
static BOOL getRotDataByIdx_(VecFx32 *rot, const void *rot3Table, const void *rot5Table, NNSG3dJntAnmRIdx rInfo) {
    fx32 *elem = (fx32 *)rot;

    if (rInfo & NNS_G3D_JNTANM_RIDX_PIVOT) {
        const s16 *p;
        fx32 b;
        fx32 a;
        u32 pivot;

        elem[0] = elem[1] = elem[2] = elem[3] = elem[4] = elem[5] = elem[6] = elem[7] = elem[8] = 0;
        p = (const s16 *)rot3Table + 3 * (rInfo & NNS_G3D_JNTANM_RIDX_IDXDATA_MASK);
        a = p[1];
        b = p[2];
        pivot = p[0] & 0xf;
        elem[pivot] = (p[0] & 0x10) ? -FX32_ONE : FX32_ONE;
        elem[NNS_COMPACT_ROT_LUT_CA[pivot][0]] = a;
        elem[NNS_COMPACT_ROT_LUT_CA[pivot][1]] = b;
        if (p[0] & 0x20) {
            b = -b;
        }
        elem[NNS_COMPACT_ROT_LUT_CA[pivot][2]] = b;
        if (p[0] & 0x40) {
            a = -a;
        }
        elem[NNS_COMPACT_ROT_LUT_CA[pivot][3]] = a;
        return FALSE;
    } else {
        const s16 *p = (const s16 *)rot5Table + 5 * (rInfo & NNS_G3D_JNTANM_RIDX_IDXDATA_MASK);
        s16 bits;

        bits = p[4] & 7;
        elem[4] = p[4] >> 3;
        bits = (bits << 3) | (p[0] & 7);
        elem[0] = p[0] >> 3;
        bits = (bits << 3) | (p[1] & 7);
        elem[1] = p[1] >> 3;
        bits = (bits << 3) | (p[2] & 7);
        elem[2] = p[2] >> 3;
        bits = (bits << 3) | (p[3] & 7);
        elem[3] = p[3] >> 3;
        elem[5] = (bits << 19) >> 19;
        return TRUE;
    }
}
