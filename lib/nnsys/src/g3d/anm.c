#include "nnsys/g3d.h"

// NitroSystem's G3D animation: the functions that combine the animations bound to a render object, for a material, a
// joint or a node's visibility, and the tables that make an animation object of each kind of animation resource. The
// file's name is a guess. The blend functions keep NitroSystem's names; the variables hold the functions that the
// animation objects and render objects copy

static void blendScale_(VecFx32 *dest, const VecFx32 *src, fx32 ratio, BOOL isOne);

// The data is sorted by size, and objects of one size come out in the reverse of their declaration order, so the words
// are declared in the reverse of the ROM's
NNSG3dFuncBlendMat NNS_G3dFuncBlendMat = NNSi_G3dAnmBlendMat;
NNSG3dFuncBlendJnt NNS_G3dFuncBlendJnt = NNSi_G3dAnmBlendJnt;
NNSG3dFuncBlendVis NNS_G3dFuncBlendVis = NNSi_G3dAnmBlendVis;

void *NNS_G3dFuncAnmMatNsBma = (void *)func_02069968;
void *NNS_G3dFuncAnmMatNsBta = (void *)func_02069ea0;
void *NNS_G3dFuncAnmMatNsBtp = (void *)func_02069cf8;
void *NNS_G3dFuncAnmJntNsBca = (void *)NNS_G3DAnimationApply_CA;
void *NNS_G3dFuncAnmVisNsBva = (void *)func_02069f18;

u32 NNS_G3D_ANIMATION_INIT_HEADER_COUNT = 5;

// The kinds of animation resource that NNS_G3DAnimationCreate knows, with room for five more
NNSG3dAnmObjInit NNS_G3D_ANIMATION_INIT_HEADERS[10] = {
    { { NNS_G3D_DATABLK_MAT_ANM, 0, NNS_G3D_ANM_CATEGORY1_MA }, func_020698d4 },
    { { NNS_G3D_DATABLK_MAT_ANM, 0, NNS_G3D_ANM_CATEGORY1_TP }, NNS_G3DAnimationInit_TP },
    { { NNS_G3D_DATABLK_MAT_ANM, 0, NNS_G3D_ANM_CATEGORY1_TA }, NNS_G3DAnimationInit_TA },
    { { NNS_G3D_DATABLK_VIS_ANM, 0, NNS_G3D_ANM_CATEGORY1_VA }, func_02069ee4 },
    { { NNS_G3D_DATABLK_JNT_ANM, 0, NNS_G3D_ANM_CATEGORY1_CA }, func_02068764 },
};

// Calls the animation function of every animation object that has data for the material, in the order of the list, so
// that a later animation overwrites the result of an earlier one
BOOL NNSi_G3dAnmBlendMat(NNSG3dMatAnmResult *result, const NNSG3dAnmObj *anmObj, u32 matID) {
    BOOL blended = FALSE;

    while (anmObj != NULL) {
        if (matID < anmObj->numMapData) {
            u16 data = anmObj->mapData[matID];

            if ((data & (NNS_G3D_ANMOBJ_MAPDATA_EXIST | NNS_G3D_ANMOBJ_MAPDATA_DISABLED)) ==
                    NNS_G3D_ANMOBJ_MAPDATA_EXIST &&
                anmObj->funcAnm != NULL) {
                ((NNSG3dFuncAnmMat)anmObj->funcAnm)(result, anmObj, data & NNS_G3D_ANMOBJ_MAPDATA_DATAFIELD);
                blended = TRUE;
            }
        }
        anmObj = anmObj->next;
    }
    return blended;
}

#pragma thumb off

// Adds ratio times the vector to the sum, or just ratio if the vector is one (every component of it one)
static void blendScale_(VecFx32 *dest, const VecFx32 *src, fx32 ratio, BOOL isOne) {
    if (isOne) {
        dest->x += ratio;
        dest->y += ratio;
        dest->z += ratio;
    } else {
        dest->x += (ratio * src->x) >> FX32_SHIFT;
        dest->y += (ratio * src->y) >> FX32_SHIFT;
        dest->z += (ratio * src->z) >> FX32_SHIFT;
    }
}

// Blends the joint animations that have data for the node, each weighted by its ratio, which is scaled down so that
// they add up to one if they come to more. With one animation, the result is the animation's own. The rotations are
// summed in their first two rows only, and the third row is made from them, with the first animation's rows to fall
// back on if a row comes to nothing
BOOL NNSi_G3dAnmBlendJnt(NNSG3dJntAnmResult *result, const NNSG3dAnmObj *anmObj, u32 nodeID) {
    const NNSG3dAnmObj *p;
    const NNSG3dAnmObj *last;
    fx32 sum;
    int count;
    int i;
    VecFx32 firstRow0;
    VecFx32 firstRow2;
    NNSG3dJntAnmResult tmp;

    if (anmObj == NULL) {
        return FALSE;
    }

    if (anmObj->next == NULL) {
        if (nodeID < anmObj->numMapData) {
            u16 data = anmObj->mapData[nodeID];

            if ((data & (NNS_G3D_ANMOBJ_MAPDATA_EXIST | NNS_G3D_ANMOBJ_MAPDATA_DISABLED)) ==
                NNS_G3D_ANMOBJ_MAPDATA_EXIST) {
                if (anmObj->funcAnm == NULL) {
                    return FALSE;
                }
                ((NNSG3dFuncAnmJnt)anmObj->funcAnm)(result, anmObj, data & NNS_G3D_ANMOBJ_MAPDATA_DATAFIELD);
                return TRUE;
            }
            return FALSE;
        }
        return FALSE;
    }

    sum = 0;
    count = 0;
    i = 0;
    p = anmObj;
    do {
        if (nodeID < p->numMapData &&
            (p->mapData[nodeID] & (NNS_G3D_ANMOBJ_MAPDATA_EXIST | NNS_G3D_ANMOBJ_MAPDATA_DISABLED)) ==
                NNS_G3D_ANMOBJ_MAPDATA_EXIST) {
            if (p->ratio > FX32_ONE) {
                sum += FX32_ONE;
            } else if (p->ratio > 0) {
                sum += p->ratio;
            }
            last = p;
            count++;
        }
        p = p->next;
    } while (p != NULL);

    if (sum == 0) {
        return FALSE;
    }

    if (count == 1) {
        u16 data = last->mapData[nodeID];

        if (last->funcAnm == NULL) {
            return FALSE;
        }
        ((NNSG3dFuncAnmJnt)last->funcAnm)(result, last, data & NNS_G3D_ANMOBJ_MAPDATA_DATAFIELD);
        return TRUE;
    }

    MI_CpuClearFast(result, sizeof(NNSG3dJntAnmResult));
    result->flag = -1;

    do {
        if (nodeID < anmObj->numMapData) {
            u16 data = anmObj->mapData[nodeID];

            if ((data & (NNS_G3D_ANMOBJ_MAPDATA_EXIST | NNS_G3D_ANMOBJ_MAPDATA_DISABLED)) ==
                    NNS_G3D_ANMOBJ_MAPDATA_EXIST &&
                anmObj->ratio > 0 && anmObj->funcAnm != NULL) {
                fx32 ratio;

                ((NNSG3dFuncAnmJnt)anmObj->funcAnm)(&tmp, anmObj, data & NNS_G3D_ANMOBJ_MAPDATA_DATAFIELD);
                // BUG: the rows to fall back on are saved only from the first animation object of the list, so if that
                // one has no data for the node, they are never set
                if (i == 0) {
                    sys_memcpy32(&tmp.rotRow[0], &firstRow0, sizeof(VecFx32));
                    sys_memcpy32(&tmp.rotRow[2], &firstRow2, sizeof(VecFx32));
                }

                if (sum == FX32_ONE) {
                    ratio = anmObj->ratio;
                } else {
                    ratio = FX_Div(anmObj->ratio, sum);
                }

                blendScale_(&result->scale, &tmp.scale, ratio, tmp.flag & NNS_G3D_JNTANM_RESULTFLAG_SCALE_ONE);
                blendScale_(&result->scaleEx0, &tmp.scaleEx0, ratio, tmp.flag & NNS_G3D_JNTANM_RESULTFLAG_SCALEEX0_ONE);
                blendScale_(&result->scaleEx1, &tmp.scaleEx1, ratio, tmp.flag & NNS_G3D_JNTANM_RESULTFLAG_SCALEEX1_ONE);

                if (!(tmp.flag & NNS_G3D_JNTANM_RESULTFLAG_TRANS_ZERO)) {
                    result->trans.x += (fx32)(((fx64)ratio * tmp.trans.x) >> FX32_SHIFT);
                    result->trans.y += (fx32)(((fx64)ratio * tmp.trans.y) >> FX32_SHIFT);
                    result->trans.z += (fx32)(((fx64)ratio * tmp.trans.z) >> FX32_SHIFT);
                }

                if (!(tmp.flag & NNS_G3D_JNTANM_RESULTFLAG_ROT_ZERO)) {
                    result->rot.m[0][0] += (ratio * tmp.rot.m[0][0]) >> FX32_SHIFT;
                    result->rot.m[0][1] += (ratio * tmp.rot.m[0][1]) >> FX32_SHIFT;
                    result->rot.m[0][2] += (ratio * tmp.rot.m[0][2]) >> FX32_SHIFT;
                    result->rot.m[1][0] += (ratio * tmp.rot.m[1][0]) >> FX32_SHIFT;
                    result->rot.m[1][1] += (ratio * tmp.rot.m[1][1]) >> FX32_SHIFT;
                    result->rot.m[1][2] += (ratio * tmp.rot.m[1][2]) >> FX32_SHIFT;
                } else {
                    result->rot.m[0][0] += ratio;
                    result->rot.m[1][1] += ratio;
                }

                result->flag &= tmp.flag;
#ifdef BUGFIX
                i++;
#endif
            }
        }
        anmObj = anmObj->next;
#ifndef BUGFIX
        i++;
#endif
    } while (anmObj != NULL);

    VEC_CrossProduct(&result->rotRow[0], &result->rotRow[1], &result->rotRow[2]);
    if (result->rot.m[0][0] == 0 && result->rot.m[0][1] == 0 && result->rot.m[0][2] == 0) {
        sys_memcpy32(&firstRow0, &result->rotRow[0], sizeof(VecFx32));
    } else {
        VEC_Normalize(&result->rotRow[0], &result->rotRow[0]);
    }
    if (result->rot.m[2][0] == 0 && result->rot.m[2][1] == 0 && result->rot.m[2][2] == 0) {
        sys_memcpy32(&firstRow2, &result->rotRow[2], sizeof(VecFx32));
    } else {
        VEC_Normalize(&result->rotRow[2], &result->rotRow[2]);
    }
    VEC_CrossProduct(&result->rotRow[2], &result->rotRow[0], &result->rotRow[1]);
    return TRUE;
}
#pragma thumb on

// ORs together the visibility that every animation object with data for the node gives
BOOL NNSi_G3dAnmBlendVis(NNSG3dVisAnmResult *result, const NNSG3dAnmObj *anmObj, u32 nodeID) {
    BOOL blended = FALSE;

    result->isVisible = 0;
    do {
        if (nodeID < anmObj->numMapData) {
            u16 data = anmObj->mapData[nodeID];

            if ((data & (NNS_G3D_ANMOBJ_MAPDATA_EXIST | NNS_G3D_ANMOBJ_MAPDATA_DISABLED)) ==
                    NNS_G3D_ANMOBJ_MAPDATA_EXIST &&
                anmObj->funcAnm != NULL) {
                NNSG3dVisAnmResult tmp;

                ((NNSG3dFuncAnmVis)anmObj->funcAnm)(&tmp, anmObj, data & NNS_G3D_ANMOBJ_MAPDATA_DATAFIELD);
                result->isVisible |= tmp.isVisible;
                blended = TRUE;
            }
        }
        anmObj = anmObj->next;
    } while (anmObj != NULL);
    return blended;
}
