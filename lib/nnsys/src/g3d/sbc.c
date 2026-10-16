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

// For a joint whose rotation is stored as its pivot (the one element that is 1 or -1, the others in its row and column
// being zero) and two numbers, the pivot's index and where the numbers go in the matrix
static const u8 NNS_COMPACT_ROT_LUT[9][4] = {
    { 4, 5, 7, 8 }, { 3, 5, 6, 8 }, { 3, 4, 6, 7 }, { 1, 2, 7, 8 }, { 0, 2, 6, 8 },
    { 0, 1, 6, 7 }, { 1, 2, 4, 5 }, { 0, 2, 3, 5 }, { 0, 1, 3, 4 },
};

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

// A little endian word at an address that need not be aligned
#define NNS_G3D_GET_U32(p) ((p)[0] | ((p)[1] << 8) | ((p)[2] << 16) | ((p)[3] << 24))

// NODEDESC: makes the matrix of a joint from the node's data and sets it as the current one, restoring the matrix it
// is relative to before, and storing it after, as the option says
void NNS_G3DRenderFuncApplyJointMatrix(NNSG3dRS *rs, u32 opt) {
    u32 size = 4;
    u32 nodeID = rs->c[1];
    u32 timing;
    u32 skip;

    rs->currentNodeDesc = nodeID;
    rs->flag |= NNS_G3D_RSFLAG_CURRENT_NODEDESC_VALID;

    if (rs->flag & NNS_G3D_RSFLAG_OPT_SKIP_SBCMTXCALC) {
        if (opt == NNS_G3D_SBCFLG_010 || opt == NNS_G3D_SBCFLG_011) {
            size++;
        }
        if (opt == NNS_G3D_SBCFLG_001 || opt == NNS_G3D_SBCFLG_011) {
            size++;
            if (!(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD)) {
                u32 stackID = rs->c[4];

                NNS_G3DExecOp(G3OP_MTX_RESTORE, &stackID, 1);
            }
        }
        rs->c += size;
        return;
    }

    if (opt == NNS_G3D_SBCFLG_010 || opt == NNS_G3D_SBCFLG_011) {
        u32 stackID;

        size++;
        if (opt == NNS_G3D_SBCFLG_010) {
            stackID = rs->c[4];
        } else {
            stackID = rs->c[5];
        }
        if (!(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD)) {
            NNS_G3DExecOp(G3OP_MTX_RESTORE, &stackID, 1);
        }
    }

    rs->pJntAnmResult = &rs->jntAnmResult;
    timing = GetCallBackTiming(rs, NNS_G3D_SBC_NODEDESC);
    if (!RunCallBack(rs, NNS_G3D_SBC_NODEDESC, &timing, NNS_G3D_SBC_CALLBACK_TIMING_A)) {
        NNSG3dJntAnmResult *result;
        BOOL isRecorded;

        if (rs->pRenderObj->recJntAnm != NULL) {
            result = &rs->pRenderObj->recJntAnm[nodeID];
            isRecorded = !(rs->flag & NNS_G3D_RSFLAG_OPT_RECORD);
        } else {
            isRecorded = FALSE;
            result = &rs->jntAnmResult;
        }

        if (!isRecorded) {
            result->flag = 0;
            if (rs->pRenderObj->anmJnt == NULL ||
                !rs->pRenderObj->funcBlendJnt(result, rs->pRenderObj->anmJnt, nodeID)) {
                const NNSG3dResNodeData *node = NNS_G3dGetNodeDataByIdx(rs->pResNodeInfo, nodeID);
                const u8 *data = (const u8 *)(node + 1);

                if (node->flag & NNS_G3D_SRTFLAG_TRANS_ZERO) {
                    result->flag |= NNS_G3D_JNTANM_RESULTFLAG_TRANS_ZERO;
                } else {
                    const fx32 *trans = (const fx32 *)data;

                    result->trans.x = trans[0];
                    result->trans.y = trans[1];
                    result->trans.z = trans[2];
                    data += sizeof(VecFx32);
                }

                if (node->flag & NNS_G3D_SRTFLAG_ROT_ZERO) {
                    result->flag |= NNS_G3D_JNTANM_RESULTFLAG_ROT_ZERO;
                } else if (node->flag & NNS_G3D_SRTFLAG_PIVOT_EXIST) {
                    const fx16 *rot = (const fx16 *)data;
                    u32 pivot;
                    fx32 b;
                    fx32 a;

                    a = rot[0];
                    b = rot[1];
                    pivot = (node->flag & NNS_G3D_SRTFLAG_IDXPIVOT_MASK) >> NNS_G3D_SRTFLAG_IDXPIVOT_SHIFT;

                    func_02079084(&result->rot);
                    result->rotElem[pivot] = (node->flag & NNS_G3D_SRTFLAG_PIVOT_MINUS) ? -FX32_ONE : FX32_ONE;
                    result->rotElem[NNS_COMPACT_ROT_LUT[pivot][0]] = a;
                    result->rotElem[NNS_COMPACT_ROT_LUT[pivot][1]] = b;
                    if (node->flag & NNS_G3D_SRTFLAG_SIGN_REVC) {
                        b = -b;
                    }
                    result->rotElem[NNS_COMPACT_ROT_LUT[pivot][2]] = b;
                    if (node->flag & NNS_G3D_SRTFLAG_SIGN_REVD) {
                        a = -a;
                    }
                    result->rotElem[NNS_COMPACT_ROT_LUT[pivot][3]] = a;
                    data += 2 * sizeof(fx16);
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
                    data += 8 * sizeof(fx16);
                }

                rs->funcJntScale(result, (const VecFx32 *)data, rs->c, node->flag);
            }
        }
        rs->pJntAnmResult = result;
    }

    skip = RunCallBack(rs, NNS_G3D_SBC_NODEDESC, &timing, NNS_G3D_SBC_CALLBACK_TIMING_B);
    if (!skip && !(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD)) {
        rs->funcJntMtx(rs->pJntAnmResult);
    }
    rs->pJntAnmResult = NULL;

    skip = RunCallBack(rs, NNS_G3D_SBC_NODEDESC, &timing, NNS_G3D_SBC_CALLBACK_TIMING_C);
    if (opt == NNS_G3D_SBCFLG_001 || opt == NNS_G3D_SBCFLG_011) {
        size++;
        if (!skip && !(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD)) {
            u32 stackID = rs->c[4];

            NNS_G3DExecOp(G3OP_MTX_STORE, &stackID, 1);
        }
    }
    rs->c += size;
}

// The geometry commands that a billboard sends: the pop of the matrix pushed to read the clip matrix, the matrix mode,
// the load of the billboard matrix and the scale after it. A billboard command makes the matrix from the clip matrix
typedef struct {
    u32 cmd;
    u32 popNum;
    u32 mtxMode;
    VecFx32 axis[3];
    VecFx32 trans;
    VecFx32 scale;
} BillboardCmd;

static BillboardCmd sBBCmd = {
    0x1b171012, // MTX_POP, MTX_MODE, MTX_LOAD_4x3, MTX_SCALE
    1,
    2,
    { { FX32_ONE, 0, 0 }, { 0, FX32_ONE, 0 }, { 0, 0, FX32_ONE } },
};

// BB: a billboard, facing the camera
void NNS_G3DRenderFuncSetBillboardMatrix1(NNSG3dRS *rs, u32 opt) {
    u32 size = 2;
    u32 timing;
    u32 skip;

    if (rs->flag & NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW) {
        if (opt == NNS_G3D_SBCFLG_010 || opt == NNS_G3D_SBCFLG_011) {
            size++;
        }
        if (opt == NNS_G3D_SBCFLG_001 || opt == NNS_G3D_SBCFLG_011) {
            size++;
        }
        rs->c += size;
        return;
    }

    if (opt == NNS_G3D_SBCFLG_010 || opt == NNS_G3D_SBCFLG_011) {
        size++;
        if (!(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD)) {
            u32 stackID;

            if (opt == NNS_G3D_SBCFLG_010) {
                stackID = rs->c[2];
            } else {
                stackID = rs->c[3];
            }
            NNS_G3DExecOp(G3OP_MTX_RESTORE, &stackID, 1);
        }
    }

    timing = GetCallBackTiming(rs, NNS_G3D_SBC_BB);
    skip = RunCallBack(rs, NNS_G3D_SBC_BB, &timing, NNS_G3D_SBC_CALLBACK_TIMING_A);
    if (!(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD) && !skip) {
        MtxFx44 clip;

        NNS_G3DWaitFIFO();
        reg_G3X_GXFIFO = 0x151110; // MTX_MODE, MTX_PUSH, MTX_IDENTITY
        reg_G3X_GXFIFO = 0;
        reg_G3X_GXFIFO = 0;
        while (gfxGetClipMatrix(&clip) != 0) {
        }

        if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_WVP) {
            MtxFx44 mv;

            MTX_Copy43To44_(NNS_G3DGetTempMVMatrix(), &mv);
            MTX_Concat44(&clip, &mv, &clip);
        } else if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_VP) {
            MtxFx44 cam;

            MTX_Copy43To44_(&NNS_G3dGlb.cameraMtx, &cam);
            MTX_Concat44(&clip, &cam, &clip);
        }

        VEC_Set(&sBBCmd.trans, clip.m[3][0], clip.m[3][1], clip.m[3][2]);
        VEC_Set(&sBBCmd.scale, VEC_Mag((const VecFx32 *)clip.m[0]), VEC_Mag((const VecFx32 *)clip.m[1]),
                VEC_Mag((const VecFx32 *)clip.m[2]));

        if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_WVP) {
            reg_G3X_GXFIFO = 0x171012; // MTX_POP, MTX_MODE, MTX_LOAD_4x3
            sys_memcpy_volatile(&sBBCmd.popNum, &reg_G3X_GXFIFO, 2 * sizeof(u32));
            sys_memcpy_volatile(NNS_G3DGetTempInvMVMatrix(), &reg_G3X_GXFIFO, sizeof(MtxFx43));
            reg_G3X_GXFIFO = 0x1b19; // MTX_MULT_4x3, MTX_SCALE
            sys_memcpy_volatile(sBBCmd.axis, &reg_G3X_GXFIFO, sizeof(BillboardCmd) - 3 * sizeof(u32));
        } else if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_VP) {
            reg_G3X_GXFIFO = 0x171012; // MTX_POP, MTX_MODE, MTX_LOAD_4x3
            sys_memcpy_volatile(&sBBCmd.popNum, &reg_G3X_GXFIFO, 2 * sizeof(u32));
            sys_memcpy_volatile(NNS_G3DGetInvViewMatrix(), &reg_G3X_GXFIFO, sizeof(MtxFx43));
            reg_G3X_GXFIFO = 0x1b19; // MTX_MULT_4x3, MTX_SCALE
            sys_memcpy_volatile(sBBCmd.axis, &reg_G3X_GXFIFO, sizeof(BillboardCmd) - 3 * sizeof(u32));
        } else {
            sys_memcpy_volatile(&sBBCmd, &reg_G3X_GXFIFO, sizeof(BillboardCmd));
        }
    }

    skip = RunCallBack(rs, NNS_G3D_SBC_BB, &timing, NNS_G3D_SBC_CALLBACK_TIMING_C);
    if (opt == NNS_G3D_SBCFLG_001 || opt == NNS_G3D_SBCFLG_011) {
        size++;
        if (!skip && !(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD)) {
            NNS_G3dGeStoreMtx(rs->c[2]);
        }
    }
    rs->c += size;
}

static BillboardCmd sBBYCmd = {
    0x1b171012, // MTX_POP, MTX_MODE, MTX_LOAD_4x3, MTX_SCALE
    1,
    2,
    { { FX32_ONE, 0, 0 }, { 0, FX32_ONE, 0 }, { 0, 0, FX32_ONE } },
};

// BBY: a billboard that turns about the y axis
void NNS_G3DRenderFuncSetBillboardMatrix2(NNSG3dRS *rs, u32 opt) {
    u32 size = 2;
    u32 timing;
    u32 skip;

    if (rs->flag & NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW) {
        if (opt == NNS_G3D_SBCFLG_010 || opt == NNS_G3D_SBCFLG_011) {
            size++;
        }
        if (opt == NNS_G3D_SBCFLG_001 || opt == NNS_G3D_SBCFLG_011) {
            size++;
        }
        rs->c += size;
        return;
    }

    if (opt == NNS_G3D_SBCFLG_010 || opt == NNS_G3D_SBCFLG_011) {
        size++;
        if (!(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD)) {
            u32 stackID;

            if (opt == NNS_G3D_SBCFLG_010) {
                stackID = rs->c[2];
            } else {
                stackID = rs->c[3];
            }
            NNS_G3DExecOp(G3OP_MTX_RESTORE, &stackID, 1);
        }
    }

    timing = GetCallBackTiming(rs, NNS_G3D_SBC_BBY);
    skip = RunCallBack(rs, NNS_G3D_SBC_BBY, &timing, NNS_G3D_SBC_CALLBACK_TIMING_A);
    if (!(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD) && !skip) {
        MtxFx44 clip;

        NNS_G3DWaitFIFO();
        reg_G3X_GXFIFO = 0x151110; // MTX_MODE, MTX_PUSH, MTX_IDENTITY
        reg_G3X_GXFIFO = 0;
        reg_G3X_GXFIFO = 0;
        while (gfxGetClipMatrix(&clip) != 0) {
        }

        if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_WVP) {
            MtxFx44 mv;

            MTX_Copy43To44_(NNS_G3DGetTempMVMatrix(), &mv);
            MTX_Concat44(&clip, &mv, &clip);
        } else if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_VP) {
            MtxFx44 cam;

            MTX_Copy43To44_(&NNS_G3dGlb.cameraMtx, &cam);
            MTX_Concat44(&clip, &cam, &clip);
        }

        sBBYCmd.trans.x = clip.m[3][0];
        sBBYCmd.trans.y = clip.m[3][1];
        sBBYCmd.trans.z = clip.m[3][2];
        sBBYCmd.scale.x = VEC_Mag((const VecFx32 *)clip.m[0]);
        sBBYCmd.scale.y = VEC_Mag((const VecFx32 *)clip.m[1]);
        sBBYCmd.scale.z = VEC_Mag((const VecFx32 *)clip.m[2]);

        // The y axis stays where the model has it, and the other two are made from it: x turns about z, z is across
        // them
        sBBYCmd.axis[1].x = clip.m[1][0];
        sBBYCmd.axis[1].y = clip.m[1][1];
        sBBYCmd.axis[1].z = clip.m[1][2];
        sBBYCmd.axis[0].x = sBBYCmd.axis[1].y;
        sBBYCmd.axis[0].y = -sBBYCmd.axis[1].x;
        sBBYCmd.axis[0].z = 0;
        VEC_Normalize(&sBBYCmd.axis[0], &sBBYCmd.axis[0]);
        VEC_Normalize(&sBBYCmd.axis[1], &sBBYCmd.axis[1]);
        VEC_CrossProduct(&sBBYCmd.axis[0], &sBBYCmd.axis[1], &sBBYCmd.axis[2]);

        if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_WVP) {
            reg_G3X_GXFIFO = 0x171012; // MTX_POP, MTX_MODE, MTX_LOAD_4x3
            sys_memcpy_volatile(&sBBYCmd.popNum, &reg_G3X_GXFIFO, 2 * sizeof(u32));
            sys_memcpy_volatile(NNS_G3DGetTempInvMVMatrix(), &reg_G3X_GXFIFO, sizeof(MtxFx43));
            reg_G3X_GXFIFO = 0x1b19; // MTX_MULT_4x3, MTX_SCALE
            sys_memcpy_volatile(sBBYCmd.axis, &reg_G3X_GXFIFO, sizeof(BillboardCmd) - 3 * sizeof(u32));
        } else if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_VP) {
            reg_G3X_GXFIFO = 0x171012; // MTX_POP, MTX_MODE, MTX_LOAD_4x3
            sys_memcpy_volatile(&sBBYCmd.popNum, &reg_G3X_GXFIFO, 2 * sizeof(u32));
            sys_memcpy_volatile(NNS_G3DGetInvViewMatrix(), &reg_G3X_GXFIFO, sizeof(MtxFx43));
            reg_G3X_GXFIFO = 0x1b19; // MTX_MULT_4x3, MTX_SCALE
            sys_memcpy_volatile(sBBYCmd.axis, &reg_G3X_GXFIFO, sizeof(BillboardCmd) - 3 * sizeof(u32));
        } else {
            sys_memcpy_volatile(&sBBYCmd, &reg_G3X_GXFIFO, sizeof(BillboardCmd));
        }
    }

    skip = RunCallBack(rs, NNS_G3D_SBC_BBY, &timing, NNS_G3D_SBC_CALLBACK_TIMING_C);
    if (opt == NNS_G3D_SBCFLG_001 || opt == NNS_G3D_SBCFLG_011) {
        size++;
        if (!skip && !(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD)) {
            NNS_G3dGeStoreMtx(rs->c[2]);
        }
    }
    rs->c += size;
}

// A fixed point product without the rounding that FX_Mul has
static inline fx32 MulFx(fx64 a, fx32 b) {
    return (fx32)((a * b) >> FX32_SHIFT);
}

// NODEMIX: makes a matrix from the matrices of several joints, each applied to the joint's inverse bind pose and
// weighted, and stores it. The matrices that the joints make are kept in g_NNSG3DIBPMatrices for the nodes that
// follow
void NNS_G3DRenderFuncApplySmoSkMatrix(NNSG3dRS *rs, u32 opt) {
    const NNSG3dResEvpMtx *evpMtx =
        (const NNSG3dResEvpMtx *)((u8 *)rs->pRenderObj->resMdl + rs->pRenderObj->resMdl->ofsEvpMtx);
    u32 numMtx = rs->c[2];
    const u8 *p = rs->c + 3;
    fx64 weight = 0;
    const MtxFx33 *nrm;
    NNSG3dResEvpMtx sum;
    u32 i;

    MI_CpuClearFast(&sum, sizeof(sum));
    NNS_G3DWaitFIFO();
    i = 0;
    reg_G3_MTX_MODE = 0;
    reg_G3_MTX_STORE = 1;
    reg_G3_MTX_IDENTITY = 0;
    reg_G3_MTX_MODE = 2;

    for (; i < numMtx; i++) {
        u32 jointID = p[1];
        u32 isCached = rs->isEnvMtxCached[jointID >> 5] & (1 << (jointID & 0x1f));
        MtxFx44 *clip = &g_NNSG3DIBPMatrices[jointID].clip;

        if (!isCached) {
            rs->isEnvMtxCached[jointID >> 5] |= 1 << (jointID & 0x1f);
            reg_G3_MTX_RESTORE = p[0];
            reg_G3_MTX_MODE = 1;
            gfxMultMatrix4x3(&evpMtx[jointID].invM);
        }

        if (i != 0) {
            sum.invN.m[0][0] += MulFx(weight, nrm->m[0][0]);
            sum.invN.m[0][1] += MulFx(weight, nrm->m[0][1]);
            sum.invN.m[0][2] += MulFx(weight, nrm->m[0][2]);
            sum.invN.m[1][0] += MulFx(weight, nrm->m[1][0]);
            sum.invN.m[1][1] += MulFx(weight, nrm->m[1][1]);
            sum.invN.m[1][2] += MulFx(weight, nrm->m[1][2]);
            sum.invN.m[2][0] += MulFx(weight, nrm->m[2][0]);
            sum.invN.m[2][1] += MulFx(weight, nrm->m[2][1]);
            sum.invN.m[2][2] += MulFx(weight, nrm->m[2][2]);
        }

        if (!isCached) {
            while (gfxGetClipMatrix(clip) != 0) {
            }
            reg_G3_MTX_MODE = 2;
            gfxMultMatrix3x3(&evpMtx[jointID].invN);
        }

        weight = p[2] << 4;
        sum.invM.m[0][0] += MulFx(weight, clip->m[0][0]);
        sum.invM.m[0][1] += MulFx(weight, clip->m[0][1]);
        sum.invM.m[0][2] += MulFx(weight, clip->m[0][2]);
        sum.invM.m[1][0] += MulFx(weight, clip->m[1][0]);
        sum.invM.m[1][1] += MulFx(weight, clip->m[1][1]);
        sum.invM.m[1][2] += MulFx(weight, clip->m[1][2]);
        sum.invM.m[2][0] += MulFx(weight, clip->m[2][0]);
        sum.invM.m[2][1] += MulFx(weight, clip->m[2][1]);
        sum.invM.m[2][2] += MulFx(weight, clip->m[2][2]);
        sum.invM.m[3][0] += MulFx(weight, clip->m[3][0]);
        sum.invM.m[3][1] += MulFx(weight, clip->m[3][1]);
        sum.invM.m[3][2] += MulFx(weight, clip->m[3][2]);
        p += 3;
        nrm = &g_NNSG3DIBPMatrices[jointID].normal;
        if (!isCached) {
            while (gfxGetNormalMatrix(&g_NNSG3DIBPMatrices[jointID].normal) != 0) {
            }
        }
    }

    sum.invN.m[0][0] += MulFx(weight, nrm->m[0][0]);
    sum.invN.m[0][1] += MulFx(weight, nrm->m[0][1]);
    sum.invN.m[0][2] += MulFx(weight, nrm->m[0][2]);
    sum.invN.m[1][0] += MulFx(weight, nrm->m[1][0]);
    sum.invN.m[1][1] += MulFx(weight, nrm->m[1][1]);
    sum.invN.m[1][2] += MulFx(weight, nrm->m[1][2]);
    sum.invN.m[2][0] += MulFx(weight, nrm->m[2][0]);
    sum.invN.m[2][1] += MulFx(weight, nrm->m[2][1]);
    sum.invN.m[2][2] += MulFx(weight, nrm->m[2][2]);
    gfxLoadMatrix4x3((const MtxFx43 *)&sum.invN);
    reg_G3_MTX_MODE = 1;
    gfxLoadMatrix4x3(&sum.invM);
    reg_G3_MTX_MODE = 0;
    reg_G3_MTX_RESTORE = 1;
    reg_G3_MTX_MODE = 2;
    reg_G3_MTX_STORE = rs->c[1];
    rs->c += 3 * (rs->c[2] + 1);
}

// CALLDL: sends a display list that the command holds the offset and size of
void NNS_G3DRenderFuncExecFIFOQueue(NNSG3dRS *rs, u32 opt) {
    u32 timing = GetCallBackTiming(rs, NNS_G3D_SBC_CALLDL);
    u32 skip = RunCallBack(rs, NNS_G3D_SBC_CALLDL, &timing, NNS_G3D_SBC_CALLBACK_TIMING_A);

    if (!(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD) && !skip) {
        const u8 *cmd = rs->c;

        NNS_G3DExecDisplayList(cmd + NNS_G3D_GET_U32(cmd + 1), NNS_G3D_GET_U32(cmd + 5));
    }

    RunCallBack(rs, NNS_G3D_SBC_CALLDL, &timing, NNS_G3D_SBC_CALLBACK_TIMING_C);
    rs->c += 9;
}

// POSSCALE: scales by the model's position scale, or with the option 000 off, its inverse
void NNS_G3DRenderFuncGlobalScale(NNSG3dRS *rs, u32 opt) {
    if (!(rs->flag & NNS_G3D_RSFLAG_OPT_NOGECMD) && !(rs->flag & NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW)) {
        VecFx32 scale;

        if (opt == NNS_G3D_SBCFLG_000) {
            scale.x = scale.y = scale.z = rs->posScale;
        } else {
            scale.x = scale.y = scale.z = rs->invPosScale;
        }
        NNS_G3dGeScaleVec(&scale);
    }
    rs->c += 1;
}

static u32 sEnvMapCmd[2] = { G3OP_TEXIMAGE_PARAM, 0 };

// ENVMAP: maps the texture as a sphere map about the normals
void NNS_G3DRenderFuncApplySphereMap(NNSG3dRS *rs, u32 opt) {
    if (!(rs->flag & NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW) && (rs->flag & NNS_G3D_RSFLAG_NODE_VISIBLE)) {
        MtxFx33 vec;
        VecFx32 scale;
        u32 timing;
        u32 skip;

        if ((rs->pMatAnmResult->prmTexImage & (3 << REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT)) !=
            (GX_TEXGEN_NORMAL << REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT)) {
            rs->pMatAnmResult->prmTexImage &= ~(3 << REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT);
            rs->pMatAnmResult->prmTexImage |= GX_TEXGEN_NORMAL << REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT;
            sEnvMapCmd[1] = rs->pMatAnmResult->prmTexImage;
            NNS_G3DExecOp(sEnvMapCmd[0], &sEnvMapCmd[1], 1);
        }
        NNS_G3dGeMtxMode(GX_MTXMODE_TEXTURE);

        timing = GetCallBackTiming(rs, NNS_G3D_SBC_ENVMAP);
        skip = RunCallBack(rs, NNS_G3D_SBC_ENVMAP, &timing, NNS_G3D_SBC_CALLBACK_TIMING_A);
        if (!skip) {
            u32 width = rs->pMatAnmResult->origWidth;
            u32 height = rs->pMatAnmResult->origHeight;

            scale.x = width << 15;
            scale.y = -height << 15;
            scale.z = 1 << 16;
            NNS_G3dGeScaleVec(&scale);
            NNS_G3dGeTexCoord(width << 11, height << 11);
        }

        skip = RunCallBack(rs, NNS_G3D_SBC_ENVMAP, &timing, NNS_G3D_SBC_CALLBACK_TIMING_B);
        if (!skip) {
            const NNSG3dResMatData *mat = NNS_G3dGetMatDataByIdx(rs->pResMat, rs->c[1]);

            if (mat->flag & NNS_G3D_MATFLAG_EFFECTMTX) {
                const fx32 *effectMtx = (const fx32 *)(mat + 1);

                if (!(mat->flag & NNS_G3D_MATFLAG_TEXMTX_SCALEONE)) {
                    effectMtx += 2;
                }
                if (!(mat->flag & NNS_G3D_MATFLAG_TEXMTX_ROTZERO)) {
                    effectMtx += 1;
                }
                if (!(mat->flag & NNS_G3D_MATFLAG_TEXMTX_TRANSZERO)) {
                    effectMtx += 2;
                }
                NNS_G3dGeMultMtx44((const MtxFx44 *)effectMtx);
            }
        }

        skip = RunCallBack(rs, NNS_G3D_SBC_ENVMAP, &timing, NNS_G3D_SBC_CALLBACK_TIMING_C);
        if (!skip) {
            NNS_G3dGeMtxMode(GX_MTXMODE_POSITION_VECTOR);
            NNS_G3DGetTransforms(NULL, &vec);
            NNS_G3dGeMtxMode(GX_MTXMODE_TEXTURE);
            if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_WVP) {
                NNS_G3dGeMultMtx33(&NNS_G3dGlb.cameraRot);
                NNS_G3dGeMultMtx33(&NNS_G3dGlb.prmBaseRot);
                NNS_G3dGeMultMtx33(&vec);
            } else if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_VP) {
                NNS_G3dGeMultMtx33(&NNS_G3dGlb.cameraRot);
                NNS_G3dGeMultMtx33(&vec);
            } else {
                NNS_G3dGeMultMtx33(&vec);
            }
        }
        NNS_G3dGeMtxMode(GX_MTXMODE_POSITION_VECTOR);
    }
    rs->c += 3;
}

static u32 sPrjMapCmd[2] = { G3OP_TEXIMAGE_PARAM, 0 };

// PRJMAP: maps the texture as if projected from the camera
void NNS_G3DRenderFuncApplyProjectionMap(NNSG3dRS *rs, u32 opt) {
    static MtxFx44 g_NNSTextureSpaceConvMatrix = {
        { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 1 << 16, 0 }, { 0, 0, 0, 1 << 16 } }
    };

    if (!(rs->flag & NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW) && (rs->flag & NNS_G3D_RSFLAG_NODE_VISIBLE)) {
        MtxFx43 pos;
        u32 timing;
        u32 skip;

        NNS_G3DGetTransforms(&pos, NULL);
        NNS_G3dGeStoreMtx(30);

        if ((rs->pMatAnmResult->prmTexImage & (3 << REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT)) !=
            (GX_TEXGEN_VERTEX << REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT)) {
            rs->pMatAnmResult->prmTexImage &= ~(3 << REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT);
            rs->pMatAnmResult->prmTexImage |= GX_TEXGEN_VERTEX << REG_G3_TEXIMAGE_PARAM_TGEN_SHIFT;
            sPrjMapCmd[1] = rs->pMatAnmResult->prmTexImage;
            NNS_G3DExecOp(sPrjMapCmd[0], &sPrjMapCmd[1], 1);
        }

        timing = GetCallBackTiming(rs, NNS_G3D_SBC_PRJMAP);
        skip = RunCallBack(rs, NNS_G3D_SBC_PRJMAP, &timing, NNS_G3D_SBC_CALLBACK_TIMING_A);
        if (!skip) {
            u32 width = rs->pMatAnmResult->origWidth;
            u32 height = rs->pMatAnmResult->origHeight;

            g_NNSTextureSpaceConvMatrix.m[0][0] = width << 15;
            g_NNSTextureSpaceConvMatrix.m[1][1] = -height << 15;
            g_NNSTextureSpaceConvMatrix.m[3][0] = width << 15;
            g_NNSTextureSpaceConvMatrix.m[3][1] = height << 15;
            NNS_G3dGeLoadMtx44(&g_NNSTextureSpaceConvMatrix);
        }

        skip = RunCallBack(rs, NNS_G3D_SBC_PRJMAP, &timing, NNS_G3D_SBC_CALLBACK_TIMING_B);
        if (!skip) {
            const NNSG3dResMatData *mat = NNS_G3dGetMatDataByIdx(rs->pResMat, rs->c[1]);

            if (mat->flag & NNS_G3D_MATFLAG_EFFECTMTX) {
                const fx32 *effectMtx = (const fx32 *)(mat + 1);

                if (!(mat->flag & NNS_G3D_MATFLAG_TEXMTX_SCALEONE)) {
                    effectMtx += 2;
                }
                if (!(mat->flag & NNS_G3D_MATFLAG_TEXMTX_ROTZERO)) {
                    effectMtx += 1;
                }
                if (!(mat->flag & NNS_G3D_MATFLAG_TEXMTX_TRANSZERO)) {
                    effectMtx += 2;
                }
                NNS_G3dGeMultMtx44((const MtxFx44 *)effectMtx);
            }
        }

        skip = RunCallBack(rs, NNS_G3D_SBC_PRJMAP, &timing, NNS_G3D_SBC_CALLBACK_TIMING_C);
        if (!skip) {
            MtxFx44 clip;

            if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_WVP) {
                NNS_G3dGeTranslateVec(&NNS_G3dGlb.prmBaseTrans);
                NNS_G3dGeMultMtx33(&NNS_G3dGlb.prmBaseRot);
                NNS_G3dGeMultMtx43(&pos);
            } else if (NNS_G3dGlb.flag & NNS_G3D_GLB_FLAG_FLUSH_VP) {
                NNS_G3dGeMultMtx43(&pos);
            } else {
                NNS_G3dGeMultMtx43(NNS_G3DGetInvViewMatrix());
                NNS_G3dGeMultMtx43(&pos);
            }

            NNS_G3DWaitFIFO();
            reg_G3_MTX_MODE = 0;
            reg_G3_MTX_PUSH = 0;
            reg_G3_MTX_IDENTITY = 0;
            while (gfxGetClipMatrix(&clip) != 0) {
            }
            reg_G3_MTX_POP = 1;
            reg_G3_MTX_MODE = 3;
            NNS_G3dGeLoadMtx44(&clip);
            NNS_G3dGeTexCoord(clip.m[3][0] >> 4, clip.m[3][1] >> 4);
        }

        NNS_G3dGeMtxMode(GX_MTXMODE_POSITION_VECTOR);
        NNS_G3dGeRestoreMtx(30);
    }
    rs->c += 3;
}
