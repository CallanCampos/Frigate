#include "nnsys/g3d.h"

// NitroSystem's G3D model renderer: draws a render object by running the commands of its model's byte code (the SBC),
// each of which sets up the node, material or shape it names and sends the geometry commands for it. The file is named
// after NitroSystem's sbc.c. The functions keep swan's names, and g3d.h gives NitroSystem's where the code shows them.
// NNS_G3DRenderCodeExec, NNS_G3DDrawCore and updateHintVec_ are static because only this file calls them

static void NNS_G3DRenderCodeExec(NNSG3dRS *rs);
static void NNS_G3DDrawCore(NNSG3dRS *rs, NNSG3dRenderObj *obj);
static void updateHintVec_(u32 *hint, const NNSG3dAnmObj *anmObj);

// The mask of the parts of a color word that a material sets, for the diffuse, ambient and vertex color flags (or the
// specular, emission and shininess flags) as the three bits of the index
static const u32 MAT_COLOR_MASKS[8] = { 0x00000000, 0x00007fff, 0x7fff0000, 0x7fff7fff,
                                        0x00008000, 0x0000ffff, 0x7fff8000, 0x7fffffff };

// The functions that draw a shape or a material of the item tag in its data, with room for three more of each; and the
// function of each command, of which the model's byte code has fourteen
NNSG3dFuncSbcShp NNS_G3D_SHAPE_DRAW_FUNCS[4] = { NNS_G3DRenderFuncDrawMeshDefault };
NNSG3dFuncSbcMat NNS_G3D_MATERIAL_APPLY_FUNCS[4] = { NNS_G3DRenderFuncApplyMaterialDefault };
NNSG3dFuncSbc NNS_G3D_RENDER_FUNC[NNS_G3D_SBC_COMMAND_NUM] = {
    NNS_G3DRenderFuncNop,
    NNS_G3DRenderFuncReturn,
    NNS_G3DRenderFuncSetVisGroup,
    NNS_G3DRenderFuncLoadMatrix,
    NNS_G3DRenderFuncApplyMaterial,
    NNS_G3DRenderFuncDrawMesh,
    NNS_G3DRenderFuncApplyJointMatrix,
    NNS_G3DRenderFuncSetBillboardMatrix1,
    NNS_G3DRenderFuncSetBillboardMatrix2,
    NNS_G3DRenderFuncApplySmoSkMatrix,
    NNS_G3DRenderFuncExecFIFOQueue,
    NNS_G3DRenderFuncGlobalScale,
    NNS_G3DRenderFuncApplySphereMap,
    NNS_G3DRenderFuncApplyProjectionMap,
};

// The matrices that NNS_G3DRenderFuncApplySmoSkMatrix makes for a joint of the envelope: the clip matrix and the normal
// matrix, 100 bytes together
typedef struct {
    MtxFx44 clip;
    MtxFx33 normal;
} IbpMatrix;

// The results of the materials that a model's nodes share, which a node reuses once one has made it. The scaling rules
// set and read the scale of each joint
NNSG3dMatAnmResult g_NNSMaterials[NNS_G3D_SIZE_MAT_MAX];
NNSG3dJointScale g_NNSG3DJointScales[NNS_G3D_SIZE_JNT_MAX];
IbpMatrix g_NNSG3DIBPMatrices[NNS_G3D_SIZE_JNT_MAX];
NNSG3dRS *g_NNSG3DModelRenderer;

// The timing of the command's callback, or zero when there is none
static inline u32 GetCallBackTiming(const NNSG3dRS *rs, u32 cmd) {
    return rs->cbVecFunc[cmd] != NULL ? rs->cbVecTiming[cmd] : 0;
}

// Runs the callback of the command if its timing is the one given, then reads the timing again, since the callback can
// change it, and returns whether the callback set SKIP
static inline u32 RunCallBack(NNSG3dRS *rs, u32 cmd, u32 *timing, u32 when) {
    if (*timing == when) {
        rs->flag &= ~NNS_G3D_RSFLAG_SKIP;
        rs->cbVecFunc[cmd](rs);
        *timing = GetCallBackTiming(rs, cmd);
        return rs->flag & NNS_G3D_RSFLAG_SKIP;
    }
    return 0;
}

static void NNS_G3DRenderCodeExec(NNSG3dRS *rs) {
    do {
        rs->flag &= ~NNS_G3D_RSFLAG_SKIP;
        NNS_G3D_RENDER_FUNC[*rs->c & 0x1f](rs, *rs->c & 0xe0);
    } while (!(rs->flag & NNS_G3D_RSFLAG_RETURN));
}

static void NNS_G3DDrawCore(NNSG3dRS *rs, NNSG3dRenderObj *obj) {
    MI_CpuClearFast(rs, sizeof(NNSG3dRS));
    // The first joint, which has no parent, has a scale of one
    rs->isScaleOne[0] = 1;
    rs->flag = NNS_G3D_RSFLAG_NODE_VISIBLE;
    if (obj->ptrUserSbc != NULL) {
        rs->c = obj->ptrUserSbc;
    } else {
        rs->c = (u8 *)obj->resMdl + obj->resMdl->ofsSbc;
    }
    rs->pRenderObj = obj;
    rs->pResNodeInfo = NNS_G3dGetNodeInfo(obj->resMdl);
    rs->pResMat = NNS_G3dGetMat(obj->resMdl);
    rs->pResShp = NNS_G3dGetShp(obj->resMdl);
    rs->funcJntScale = NNS_G3D_JOINT_SCALE_FUNCS[NNS_G3dGetMdlInfo(obj->resMdl)->scalingRule];
    rs->funcJntMtx = NNS_G3D_JOINT_FLUSH_FUNCS[NNS_G3dGetMdlInfo(obj->resMdl)->scalingRule];
    rs->funcTexMtx = NNS_G3D_TEX_MATRIX_FUNCS[NNS_G3dGetMdlInfo(obj->resMdl)->texMtxMode];
    rs->posScale = NNS_G3dGetMdlInfo(obj->resMdl)->posScale;
    rs->invPosScale = NNS_G3dGetMdlInfo(obj->resMdl)->invPosScale;

    if (obj->cbFunc != NULL && obj->cbCmd < NNS_G3D_SBC_COMMAND_NUM) {
        rs->cbVecFunc[obj->cbCmd] = obj->cbFunc;
        rs->cbVecTiming[obj->cbCmd] = obj->cbTiming;
    }

    if (obj->flag & NNS_G3D_RENDEROBJ_FLAG_RECORD) {
        rs->flag |= NNS_G3D_RSFLAG_OPT_RECORD;
    }
    if (obj->flag & NNS_G3D_RENDEROBJ_FLAG_NOGECMD) {
        rs->flag |= NNS_G3D_RSFLAG_OPT_NOGECMD;
    }
    if (obj->flag & NNS_G3D_RENDEROBJ_FLAG_SKIP_SBCDRAW) {
        rs->flag |= NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW;
    }
    if (obj->flag & NNS_G3D_RENDEROBJ_FLAG_SKIP_SBCMTXCALC) {
        rs->flag |= NNS_G3D_RSFLAG_OPT_SKIP_SBCMTXCALC;
    }

    if (obj->cbInitFunc != NULL) {
        obj->cbInitFunc(rs);
    }
    NNS_G3DRenderCodeExec(rs);
    obj->flag &= ~NNS_G3D_RENDEROBJ_FLAG_RECORD;
}

// Sets the bit of every material or node that an animation of the list has data for
static void updateHintVec_(u32 *hint, const NNSG3dAnmObj *anmObj) {
    while (anmObj != NULL) {
        int i;

        for (i = 0; i < anmObj->numMapData; i++) {
            if (anmObj->mapData[i] & NNS_G3D_ANMOBJ_MAPDATA_EXIST) {
                hint[i >> 5] |= 1 << (i & 0x1f);
            }
        }
        anmObj = anmObj->next;
    }
}

void NNS_G3DDraw(NNSG3dRenderObj *obj) {
    NNSG3dRS rs;

    if ((obj->flag & NNS_G3D_RENDEROBJ_FLAG_HINT_OBSOLETE) == NNS_G3D_RENDEROBJ_FLAG_HINT_OBSOLETE) {
        MI_CpuClear32(obj->hintMatAnmExist, sizeof(obj->hintMatAnmExist));
        MI_CpuClear32(obj->hintJntAnmExist, sizeof(obj->hintJntAnmExist));
        MI_CpuClear32(obj->hintVisAnmExist, sizeof(obj->hintVisAnmExist));
        if (obj->anmMat != NULL) {
            updateHintVec_(obj->hintMatAnmExist, obj->anmMat);
        }
        if (obj->anmJnt != NULL) {
            updateHintVec_(obj->hintJntAnmExist, obj->anmJnt);
        }
        if (obj->anmVis != NULL) {
            updateHintVec_(obj->hintVisAnmExist, obj->anmVis);
        }
        obj->flag &= ~NNS_G3D_RENDEROBJ_FLAG_HINT_OBSOLETE;
    }

    if (g_NNSG3DModelRenderer != NULL) {
        NNS_G3DDrawCore(g_NNSG3DModelRenderer, obj);
    } else {
        g_NNSG3DModelRenderer = &rs;
        NNS_G3DDrawCore(&rs, obj);
        g_NNSG3DModelRenderer = NULL;
    }
}

void NNS_G3DRenderFuncNop(NNSG3dRS *rs, u32 opt) {
    if (rs->cbVecFunc[NNS_G3D_SBC_NOP] != NULL) {
        rs->cbVecFunc[NNS_G3D_SBC_NOP](rs);
    }
    rs->c += 1;
}

void NNS_G3DRenderFuncReturn(NNSG3dRS *rs, u32 opt) {
    if (rs->cbVecFunc[NNS_G3D_SBC_RET] != NULL) {
        rs->cbVecFunc[NNS_G3D_SBC_RET](rs);
    }
    rs->flag |= NNS_G3D_RSFLAG_RETURN;
}

// NODE: sets the node that the commands that follow draw, and whether it is shown
void NNS_G3DRenderFuncSetVisGroup(NNSG3dRS *rs, u32 opt) {
    if (!(rs->flag & NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW)) {
        u32 nodeID;
        u32 timing;

        rs->currentNode = rs->c[1];
        nodeID = rs->currentNode;
        rs->flag |= NNS_G3D_RSFLAG_CURRENT_NODE_VALID;
        rs->pVisAnmResult = &rs->visAnmResult;

        timing = GetCallBackTiming(rs, NNS_G3D_SBC_NODE);
        if (!RunCallBack(rs, NNS_G3D_SBC_NODE, &timing, NNS_G3D_SBC_CALLBACK_TIMING_A)) {
            const NNSG3dRenderObj *obj = rs->pRenderObj;

            if (obj->anmVis == NULL || !(obj->hintVisAnmExist[nodeID >> 5] & (1 << (nodeID & 0x1f))) ||
                !obj->funcBlendVis(rs->pVisAnmResult, obj->anmVis, nodeID)) {
                rs->pVisAnmResult->isVisible = rs->c[2] & 1;
            }
        }

        if (!RunCallBack(rs, NNS_G3D_SBC_NODE, &timing, NNS_G3D_SBC_CALLBACK_TIMING_B)) {
            if (rs->pVisAnmResult->isVisible) {
                rs->flag |= NNS_G3D_RSFLAG_NODE_VISIBLE;
            } else {
                rs->flag &= ~NNS_G3D_RSFLAG_NODE_VISIBLE;
            }
        }

        RunCallBack(rs, NNS_G3D_SBC_NODE, &timing, NNS_G3D_SBC_CALLBACK_TIMING_C);
    }
    rs->c += 3;
}

// MTX: restores a matrix that a node stored in the matrix stack
void NNS_G3DRenderFuncLoadMatrix(NNSG3dRS *rs, u32 opt) {
    if (!(rs->flag & NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW) && (rs->flag & NNS_G3D_RSFLAG_NODE_VISIBLE)) {
        u32 timing = GetCallBackTiming(rs, NNS_G3D_SBC_MTX);

        if (!RunCallBack(rs, NNS_G3D_SBC_MTX, &timing, NNS_G3D_SBC_CALLBACK_TIMING_A)) {
            u32 param = rs->c[1];

            if (!(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD)) {
                NNS_G3DExecOp(0x14, &param, 1); // MTX_RESTORE
            }
        }

        RunCallBack(rs, NNS_G3D_SBC_MTX, &timing, NNS_G3D_SBC_CALLBACK_TIMING_C);
    }
    rs->c += 2;
}

// MAT's default function: makes the result for the material, from its own values and the global state's, then the
// animations'; and sends it to the geometry engine. With the option 010 a result that is made stays in g_NNSMaterials,
// for the nodes that follow with the material, and with 001 one that stayed there is used
void NNS_G3DRenderFuncApplyMaterialDefault(NNSG3dRS *rs, u32 opt, const NNSG3dResMatData *mat, u32 matID) {
    u32 timing;

    rs->currentMat = matID;
    rs->flag |= NNS_G3D_RSFLAG_CURRENT_MAT_VALID;
    rs->pMatAnmResult = &rs->matAnmResult;

    timing = GetCallBackTiming(rs, NNS_G3D_SBC_MAT);
    if (!RunCallBack(rs, NNS_G3D_SBC_MAT, &timing, NNS_G3D_SBC_CALLBACK_TIMING_A)) {
        NNSG3dMatAnmResult *result;

        if (rs->pRenderObj->recMatAnm != NULL && !(rs->flag & NNS_G3D_RSFLAG_OPT_RECORD)) {
            result = &rs->pRenderObj->recMatAnm[matID];
        } else if ((opt == NNS_G3D_SBCFLG_001 || opt == NNS_G3D_SBCFLG_010) &&
                   (rs->isMatCached[matID >> 5] & (1 << (matID & 0x1f)))) {
            if (rs->pRenderObj->recMatAnm != NULL) {
                result = &rs->pRenderObj->recMatAnm[matID];
            } else {
                result = &g_NNSMaterials[matID];
            }
        } else {
            if (rs->pRenderObj->recMatAnm != NULL) {
                rs->isMatCached[matID >> 5] |= 1 << (matID & 0x1f);
                result = &rs->pRenderObj->recMatAnm[matID];
            } else if (opt == NNS_G3D_SBCFLG_010) {
                rs->isMatCached[matID >> 5] |= 1 << (matID & 0x1f);
                result = &g_NNSMaterials[matID];
            } else {
                result = &rs->matAnmResult;
            }

            result->flag = 0;
            if (NNS_G3dGetMatDataByIdx(rs->pResMat, matID)->flag & NNS_G3D_MATFLAG_WIREFRAME) {
                result->flag |= NNS_G3D_MATANM_RESULTFLAG_WIREFRAME;
            }

            {
                u32 mask =
                    MAT_COLOR_MASKS[(mat->flag >> NNS_G3D_MATFLAG_DIFFAMB_SHIFT) & NNS_G3D_MATFLAG_COLOR_INDEX_MASK];

                result->prmMatColor0 = (NNS_G3dGlb.prmMatColor0 & ~mask) | (mat->diffAmb & mask);
            }
            {
                u32 mask =
                    MAT_COLOR_MASKS[(mat->flag >> NNS_G3D_MATFLAG_SPECEMI_SHIFT) & NNS_G3D_MATFLAG_COLOR_INDEX_MASK];

                result->prmMatColor1 = (NNS_G3dGlb.prmMatColor1 & ~mask) | (mat->specEmi & mask);
            }
            result->prmPolygonAttr =
                (NNS_G3dGlb.prmPolygonAttr & ~mat->polyAttrMask) | (mat->polyAttr & mat->polyAttrMask);
            result->prmTexImage = mat->texImageParam;
            result->prmTexPltt = mat->texPlttBase;

            if (mat->flag & NNS_G3D_MATFLAG_TEXMTX_USE) {
                const fx32 *texMtx = (const fx32 *)(mat + 1);

                if (!(mat->flag & NNS_G3D_MATFLAG_TEXMTX_SCALEONE)) {
                    result->scaleS = texMtx[0];
                    result->scaleT = texMtx[1];
                    texMtx += 2;
                } else {
                    result->flag |= NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SCALEONE;
                }

                if (!(mat->flag & NNS_G3D_MATFLAG_TEXMTX_ROTZERO)) {
                    const fx16 *rot = (const fx16 *)texMtx;

                    result->sinR = rot[0];
                    result->cosR = rot[1];
                    texMtx += 1;
                } else {
                    result->flag |= NNS_G3D_MATANM_RESULTFLAG_TEXMTX_ROTZERO;
                }

                if (!(mat->flag & NNS_G3D_MATFLAG_TEXMTX_TRANSZERO)) {
                    result->transS = texMtx[0];
                    result->transT = texMtx[1];
                } else {
                    result->flag |= NNS_G3D_MATANM_RESULTFLAG_TEXMTX_TRANSZERO;
                }

                result->flag |= NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SET;
            }

            if (rs->pRenderObj->anmMat != NULL &&
                (rs->pRenderObj->hintMatAnmExist[matID >> 5] & (1 << (matID & 0x1f)))) {
                rs->pRenderObj->funcBlendMat(result, rs->pRenderObj->anmMat, matID);
            }

            if (result->flag & (NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SET | NNS_G3D_MATANM_RESULTFLAG_TEXMTX_MULT)) {
                result->origWidth = mat->origWidth;
                result->origHeight = mat->origHeight;
                result->magW = mat->magW;
                result->magH = mat->magH;
            }
        }
        rs->pMatAnmResult = result;
    }

    if (!RunCallBack(rs, NNS_G3D_SBC_MAT, &timing, NNS_G3D_SBC_CALLBACK_TIMING_B)) {
        NNSG3dMatAnmResult *result = rs->pMatAnmResult;

        if (result->prmPolygonAttr & (0x1f << REG_G3_POLYGON_ATTR_ALPHA_SHIFT)) {
            u32 cmd[7];

            if (result->flag & NNS_G3D_MATANM_RESULTFLAG_WIREFRAME) {
                result->prmPolygonAttr &= ~(0x1f << REG_G3_POLYGON_ATTR_ALPHA_SHIFT);
            }
            rs->flag &= ~NNS_G3D_RSFLAG_MAT_TRANSPARENT;

            if (!(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD)) {
                cmd[0] = 0x293130; // DIF_AMB, SPE_EMI, POLYGON_ATTR
                cmd[1] = result->prmMatColor0;
                cmd[2] = result->prmMatColor1;
                cmd[3] = result->prmPolygonAttr;
                cmd[4] = 0x2b2a; // TEXIMAGE_PARAM, PLTT_BASE
                cmd[5] = result->prmTexImage;
                cmd[6] = result->prmTexPltt;
                NNS_G3DExecOp(cmd[0], &cmd[1], 6);

                if (result->flag & (NNS_G3D_MATANM_RESULTFLAG_TEXMTX_SET | NNS_G3D_MATANM_RESULTFLAG_TEXMTX_MULT)) {
                    rs->funcTexMtx(result);
                }
            }
        } else {
            rs->flag |= NNS_G3D_RSFLAG_MAT_TRANSPARENT;
        }
    }

    RunCallBack(rs, NNS_G3D_SBC_MAT, &timing, NNS_G3D_SBC_CALLBACK_TIMING_C);
}

// MAT: draws what follows with the material, unless the node is hidden and that material is the current one
void NNS_G3DRenderFuncApplyMaterial(NNSG3dRS *rs, u32 opt) {
    if (!(rs->flag & NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW)) {
        u32 matID = rs->c[1];

        if ((rs->flag & NNS_G3D_RSFLAG_NODE_VISIBLE) || !(rs->flag & NNS_G3D_RSFLAG_CURRENT_MAT_VALID) ||
            matID != rs->currentMat) {
            const NNSG3dResMatData *mat = NNS_G3dGetMatDataByIdx(rs->pResMat, matID);

            NNS_G3D_MATERIAL_APPLY_FUNCS[mat->itemTag](rs, opt, mat, matID);
        }
    }
    rs->c += 2;
}

// SHP's default function: sends the display list of the shape
void NNS_G3DRenderFuncDrawMeshDefault(NNSG3dRS *rs, u32 opt, const NNSG3dResShpData *shp, u32 shpID) {
    u32 timing = GetCallBackTiming(rs, NNS_G3D_SBC_SHP);

    if (!RunCallBack(rs, NNS_G3D_SBC_SHP, &timing, NNS_G3D_SBC_CALLBACK_TIMING_A)) {
        if (!(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD)) {
            NNS_G3DExecDisplayList((u8 *)shp + shp->ofsDL, shp->sizeDL);
        }
    }

    RunCallBack(rs, NNS_G3D_SBC_SHP, &timing, NNS_G3D_SBC_CALLBACK_TIMING_B);
    RunCallBack(rs, NNS_G3D_SBC_SHP, &timing, NNS_G3D_SBC_CALLBACK_TIMING_C);
}

// SHP: draws the shape, if the node and the material are shown
void NNS_G3DRenderFuncDrawMesh(NNSG3dRS *rs, u32 opt) {
    if (!(rs->flag & NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW) && (rs->flag & NNS_G3D_RSFLAG_NODE_VISIBLE) &&
        !(rs->flag & NNS_G3D_RSFLAG_MAT_TRANSPARENT)) {
        u32 shpID = rs->c[1];
        const NNSG3dResShpData *shp = NNS_G3dGetShpDataByIdx(rs->pResShp, shpID);

        NNS_G3D_SHAPE_DRAW_FUNCS[shp->itemTag](rs, opt, shp, shpID);
    }
    rs->c += 2;
}

// TODO: the remaining render functions, NNS_G3DRenderFuncApplyJointMatrix to NNS_G3DRenderFuncApplyProjectionMap
//
// The data they use, in the ROM's order (Black 2 addresses): .rodata NNS_COMPACT_ROT_LUT, a 9 by 4 table of bytes, at
// 0x02094434; .data two 8-byte pairs {0x2a, 0} (the TEXIMAGE_PARAM command and its argument) at 0x0209b320 for
// ApplyProjectionMap and sEnvMapCmd at 0x0209b328 for ApplySphereMap, both accessed from the start of the section;
// g_NNSTextureSpaceConvMatrix, a 4x4 matrix (64 bytes) at 0x0209b350 that is all zero but m[2][2] and m[3][3], 0x10000;
// and two 72-byte billboard command buffers at 0x0209b390 (SetBillboardMatrix1) and 0x0209b3d8 (SetBillboardMatrix2).
// g_NNSG3DIBPMatrices is above, without the function that uses it: nothing in this file refers to it yet, and it has a
// section of its own, as the ROM has it, which it may lose once something does
