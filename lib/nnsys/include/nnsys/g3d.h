#ifndef POKEBW2_NNSYS_G3D_H
#define POKEBW2_NNSYS_G3D_H

#include "types.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "gfl/std.h"
#include "nnsys/fnd.h"
#include "nnsys/gfd.h"

// NitroSystem's 3D graphics (NNS_G3d): its resource files, the render objects that draw a model, the animation
// objects bound to them, and the global state of the geometry engine. Functions keep swan's names where it has them;
// the NitroSystem name follows in a comment

// A resource file and its blocks. The signatures are the files' first four characters
#define NNS_G3D_SIGNATURE_BMD0 0x30444d42
#define NNS_G3D_SIGNATURE_BTX0 0x30585442
#define NNS_G3D_SIGNATURE_BCA0 0x30414342
#define NNS_G3D_SIGNATURE_BVA0 0x30415642
#define NNS_G3D_SIGNATURE_BMA0 0x30414d42
#define NNS_G3D_SIGNATURE_BTA0 0x30415442
#define NNS_G3D_SIGNATURE_BTP0 0x30505442

typedef struct {
    u32 signature;
    u16 byteOrder;
    u16 version;
    u32 fileSize;
    u16 headerSize;
    u16 dataBlocks;
} NNSG3dResFileHeader;

typedef struct {
    u32 kind;
    u32 size;
} NNSG3dResDataBlockHeader;

// A dictionary: a Patricia tree of names, then each entry's data of sizeUnit bytes
typedef struct {
    u8 revision;
    u8 numEntry;
    u16 sizeDictBlk;
    u16 dummy_;
    u16 ofsEntry;
} NNSG3dResDict;

typedef struct {
    u16 sizeUnit;
    u16 ofsName;
    u8 data[4];
} NNSG3dResDictEntryHeader;

static inline void *NNS_G3dGetResDataByIdx(const NNSG3dResDict *dict, u32 idx) {
    if (dict != NULL && idx < dict->numEntry) {
        const NNSG3dResDictEntryHeader *hdr = (const NNSG3dResDictEntryHeader *)((u8 *)dict + dict->ofsEntry);

        return (void *)((u8 *)&hdr->data[0] + hdr->sizeUnit * idx);
    }
    return NULL;
}

// The texture block, with where its textures and palettes are and their VRAM keys once loaded
#define NNS_G3D_RESTEX_LOADED 0x0001
#define NNS_G3D_RESPLTT_LOADED 0x0001
#define NNS_G3D_RESPLTT_USEPLTT4 0x8000

typedef struct {
    NNSGfdTexKey vramKey;
    u16 sizeTex;
    u16 ofsDict;
    u16 flag;
    u16 dummy_;
    u32 ofsTex;
} NNSG3dResTexInfo;

typedef struct {
    NNSGfdTexKey vramKey;
    u16 sizeTex;
    u16 ofsDict;
    u16 flag;
    u16 dummy_;
    u32 ofsTex;
    u32 ofsTexPlttIdx;
} NNSG3dResTex4x4Info;

typedef struct {
    NNSGfdPlttKey vramKey;
    u16 sizePltt;
    u16 flag;
    u16 ofsDict;
    u16 dummy_;
    u32 ofsPlttData;
} NNSG3dResPlttInfo;

typedef struct {
    NNSG3dResDataBlockHeader header;
    NNSG3dResTexInfo texInfo;
    NNSG3dResTex4x4Info tex4x4Info;
    NNSG3dResPlttInfo plttInfo;
    NNSG3dResDict dict;
} NNSG3dResTex;

// A model, and the block of a file's models
typedef struct {
    u8 sbcType;
    u8 scalingRule;
    u8 texMtxMode;
    u8 numNode;
    u8 numMat;
    u8 numShp;
    u8 firstUnusedMtxStackID;
    u8 dummy_;
    fx32 posScale;
    fx32 invPosScale;
    u16 numVertex;
    u16 numPolygon;
    u16 numTriangle;
    u16 numQuad;
    // The box that holds the model, at boxPosScale
    fx16 boxX;
    fx16 boxY;
    fx16 boxZ;
    fx16 boxW;
    fx16 boxH;
    fx16 boxD;
    fx32 boxPosScale;
    fx32 boxInvPosScale;
} NNSG3dResMdlInfo;

typedef struct NNSG3dResMdl {
    u32 size;
    u32 ofsSbc;
    u32 ofsMat;
    u32 ofsShp;
    u32 ofsEvpMtx;
    NNSG3dResMdlInfo info;
} NNSG3dResMdl;

typedef struct {
    NNSG3dResDataBlockHeader header;
    NNSG3dResDict dict;
} NNSG3dResMdlSet;

static inline NNSG3dResMdl *NNS_G3dGetMdlByIdx(const NNSG3dResMdlSet *mdlSet, u32 idx) {
    const u32 *ofs = NNS_G3dGetResDataByIdx(&mdlSet->dict, idx);

    if (ofs != NULL) {
        return (NNSG3dResMdl *)((u8 *)mdlSet + *ofs);
    }
    return NULL;
}

static inline NNSG3dResMdlInfo *NNS_G3dGetMdlInfo(const NNSG3dResMdl *mdl) {
    return (NNSG3dResMdlInfo *)&mdl->info;
}

// The start of every animation
typedef struct {
    u8 category0;
    u8 revision;
    u16 category1;
    u16 numFrame;
} NNSG3dResAnmCommon;

// An animation applied to a model, at a frame
typedef struct {
    fx32 frame;
    fx32 ratio;
    const void *resAnm;
} NNSG3dAnmObj;

static inline fx32 NNS_G3dAnmObjGetNumFrame(const NNSG3dAnmObj *anmObj) {
    return ((const NNSG3dResAnmCommon *)anmObj->resAnm)->numFrame * FX32_ONE;
}

// The header of every animation resource
typedef struct {
    u8 category0;
    u8 revision;
    u16 category1;
} NNSG3dResAnmHeader;

// A joint animation. Each joint's tag, at ofsTag[joint], is a word of NNS_G3D_JNTANM_SRTINFO_* flags followed by the
// data of each component that isn't constant
typedef struct {
    NNSG3dResAnmHeader anmHeader;
    u16 numFrame;
    u16 numNode;
    u32 flag;
    u32 ofsRot3;
    u32 ofsRot5;
    u16 ofsTag[1];
} NNSG3dResJntAnm;

#define NNS_G3D_JNTANM_SRTINFO_IDENTITY 0x00000001
#define NNS_G3D_JNTANM_SRTINFO_IDENTITY_T 0x00000002
#define NNS_G3D_JNTANM_SRTINFO_BASE_T 0x00000004
#define NNS_G3D_JNTANM_SRTINFO_CONST_TX 0x00000008
#define NNS_G3D_JNTANM_SRTINFO_CONST_TY 0x00000010
#define NNS_G3D_JNTANM_SRTINFO_CONST_TZ 0x00000020

// The first word of a translation component's data; the second is the offset of its values from the animation
#define NNS_G3D_JNTANM_TINFO_STEP_1 0x00000000
#define NNS_G3D_JNTANM_TINFO_STEP_2 0x40000000
#define NNS_G3D_JNTANM_TINFO_STEP_4 0x80000000
#define NNS_G3D_JNTANM_TINFO_STEP_MASK 0xc0000000
#define NNS_G3D_JNTANM_TINFO_FX16ARR 0x20000000
#define NNS_G3D_JNTANM_TINFO_LAST_INTERP_MASK 0x1fff0000
#define NNS_G3D_JNTANM_TINFO_LAST_INTERP_SHIFT 16

// What draws a model, 0x54 bytes
typedef struct {
    u32 flag;
    NNSG3dResMdl *resMdl;
    void *anmMat;
    void *funcBlendMat;
    void *anmJnt;
    void *funcBlendJnt;
    void *anmVis;
    void *funcBlendVis;
    void *cbFunc;
    u8 cbCmd;
    u8 cbTiming;
    u16 dummy_;
    void *cbInitFunc;
    void *ptrUser;
    u8 *ptrUserSbc;
    void *recJntAnm;
    void *recMatAnm;
    u32 hintMatAnmExist[2];
    u32 hintJntAnmExist[2];
    u32 hintVisAnmExist[2];
} NNSG3dRenderObj;

// The blocks of a file: NNS_G3dGetMdlSet, NNS_G3dGetTex and NNS_G3dGetAnmByIdx
NNSG3dResMdlSet *NNS_G3DResGetMdlBlock(const NNSG3dResFileHeader *header);
NNSG3dResTex *NNS_G3DResGetTexBlock(const NNSG3dResFileHeader *header);
// The name of a dictionary entry, padded to sixteen characters with zeros
typedef union {
    char name[16];
    u32 val[4];
} NNSG3dResName;

// NitroSDK's NNS_G3dGetResDataByName: the data of the dictionary's entry of the name, or NULL
void *NNS_G3DFind(const NNSG3dResDict *dict, const NNSG3dResName *name);
// NitroSDK's NNS_G3dGetResDictIdxByName: the index of the dictionary's entry of the name, or -1
int NNS_G3DFindIndex(const NNSG3dResDict *dict, const NNSG3dResName *name);

static inline const NNSG3dResName *NNS_G3dGetResNameByIdx(const NNSG3dResDict *dict, u32 idx) {
    if (dict != NULL && idx < dict->numEntry) {
        const NNSG3dResDictEntryHeader *hdr = (const NNSG3dResDictEntryHeader *)((u8 *)dict + dict->ofsEntry);

        return (const NNSG3dResName *)((u8 *)hdr + hdr->ofsName) + idx;
    }
    return NULL;
}

// The data of a texture block's dictionary entry: the texture's GX_TEXIMAGE_PARAM, with its offset in the low bits
typedef struct {
    u32 texImageParam;
    u32 extraParam;
} NNSG3dResDictTexData;

// The data of a palette dictionary entry: the palette's offset in 8-byte units, and NNS_G3D_RESPLTT_USEPLTT4
typedef struct {
    u16 offset;
    u16 flag;
} NNSG3dResDictPlttData;
void *NNS_G3DResGetAnm(const NNSG3dResFileHeader *header, u32 idx);

// Textures and palettes in VRAM: NNS_G3dTexGetRequiredSize, NNS_G3dTexSetTexKey, NNS_G3dTexLoad,
// NNS_G3dPlttGetRequiredSize, NNS_G3dPlttSetPlttKey and NNS_G3dPlttLoad
u32 NNS_G3DResTexGetTexDataSize(const NNSG3dResTex *tex);
u32 NNS_G3dTexGetRequiredSize4x4(const NNSG3dResTex *tex);
void NNS_G3DResTexSetGfdKeys(NNSG3dResTex *tex, NNSGfdTexKey key, NNSGfdTexKey key4x4);
void NNS_G3DResTexUploadTex(NNSG3dResTex *tex, BOOL exec_begin_end);
void NNS_G3dTexReleaseTexKey(NNSG3dResTex *tex, NNSGfdTexKey *key, NNSGfdTexKey *key4x4);
u32 NNS_G3DResTexGetPaletteDataSize(const NNSG3dResTex *tex);
void NNS_G3DResTexSetPltGfdKey(NNSG3dResTex *tex, NNSGfdPlttKey plttKey);
void NNS_G3DResTexUploadPlt(NNSG3dResTex *tex, BOOL exec_begin_end);
NNSGfdPlttKey NNS_G3dPlttReleasePlttKey(NNSG3dResTex *tex);
BOOL NNS_G3dBindMdlTex(NNSG3dResMdl *mdl, const NNSG3dResTex *tex);
BOOL NNS_G3dBindMdlPltt(NNSG3dResMdl *mdl, const NNSG3dResTex *tex);

// Render and animation objects: NNS_G3dRenderObjInit, NNS_G3dAnmObjInit, NNS_G3dRenderObjAddAnmObj and
// NNS_G3dAllocAnmObj, and drawing one with NNS_G3dDraw
NNSG3dRenderObj *NNS_G3dAllocRenderObj(NNSFndAllocator *allocator);
void NNS_G3dFreeRenderObj(NNSFndAllocator *allocator, NNSG3dRenderObj *obj);
// NNS_G3dBindMdlTex and NNS_G3dBindMdlPltt
BOOL NNS_G3dBindMdlTex(NNSG3dResMdl *mdl, const NNSG3dResTex *tex);
BOOL NNS_G3dBindMdlPltt(NNSG3dResMdl *mdl, const NNSG3dResTex *tex);
// Binds one palette of the texture to the materials of a palette-to-material entry, whatever was bound, and releases
// every palette of the model
BOOL NNS_G3dForceBindMdlPltt(NNSG3dResMdl *mdl, const NNSG3dResTex *tex, u32 plttToMatListIdx, u32 plttNameIdx);
void NNS_G3dReleaseMdlPltt(NNSG3dResMdl *mdl);
void NNS_G3DModelAttachResource(NNSG3dRenderObj *obj, NNSG3dResMdl *mdl);
NNSG3dAnmObj *NNS_G3DAnimationAlloc(NNSFndAllocator *allocator, const void *anm, const NNSG3dResMdl *mdl);
u32 NNS_G3DAnimationCalcAllocSize(const void *anm, const NNSG3dResMdl *mdl);
void NNS_G3dFreeAnmObj(NNSFndAllocator *allocator, NNSG3dAnmObj *anmObj);
void NNS_G3DAnimationCreate(NNSG3dAnmObj *anmObj, void *anm, const NNSG3dResMdl *mdl, const NNSG3dResTex *tex);
void NNS_G3DAnimationBind(NNSG3dRenderObj *obj, NNSG3dAnmObj *anmObj);
void NNS_G3dRenderObjRemoveAnmObj(NNSG3dRenderObj *obj, NNSG3dAnmObj *anmObj);
void NNS_G3DDraw(NNSG3dRenderObj *obj);

// The buffer that geometry commands wait in, and sending one: NNS_G3dGeFlushBuffer and NNS_G3dGeBufferOP_N
BOOL NNS_G3dGeIsBufferExist(void);
void NNS_G3dGeSetBuffer(void *buffer);
void *NNS_G3dGeReleaseBuffer(void);
void NNS_G3DWaitFIFO(void);
void NNS_G3DExecOp(u32 op, const u32 *args, u32 num);

// NitroSystem's inline wrappers of the buffer, one per geometry command
static inline void NNS_G3dGePushMtx(void) {
    NNS_G3DExecOp(G3OP_MTX_PUSH, NULL, 0);
}

static inline void NNS_G3dGePopMtx(int num) {
    s32 param = num;

    NNS_G3DExecOp(G3OP_MTX_POP, (const u32 *)&param, 1);
}

static inline void NNS_G3dGeIdentity(void) {
    NNS_G3DExecOp(G3OP_MTX_IDENTITY, NULL, 0);
}

static inline void NNS_G3dGeLoadMtx43(const MtxFx43 *mtx) {
    NNS_G3DExecOp(G3OP_MTX_LOAD_4x3, (const u32 *)mtx, 12);
}

static inline void NNS_G3dGeMultMtx43(const MtxFx43 *mtx) {
    NNS_G3DExecOp(G3OP_MTX_MULT_4x3, (const u32 *)mtx, 12);
}

static inline void NNS_G3dGeScaleVec(const VecFx32 *scale) {
    NNS_G3DExecOp(G3OP_MTX_SCALE, (const u32 *)scale, 3);
}

static inline void NNS_G3dGeTranslateVec(const VecFx32 *trans) {
    NNS_G3DExecOp(G3OP_MTX_TRANS, (const u32 *)trans, 3);
}

static inline void NNS_G3dGePolygonAttr(int light, int polyMode, int cullMode, int polygonID, int alpha, int misc) {
    u32 param = GX_PACK_POLYGONATTR_PARAM(light, polyMode, cullMode, polygonID, alpha, misc);

    NNS_G3DExecOp(G3OP_POLYGON_ATTR, &param, 1);
}

static inline void NNS_G3dGeBegin(int primitive) {
    u32 param = primitive;

    NNS_G3DExecOp(G3OP_BEGIN, &param, 1);
}

static inline void NNS_G3dGeEnd(void) {
    NNS_G3DExecOp(G3OP_END, NULL, 0);
}

static inline void NNS_G3dGeBoxTest(const GXBoxTestParam *box) {
    NNS_G3DExecOp(G3OP_BOX_TEST, (const u32 *)box, 3);
}

// The global state of the geometry engine
typedef struct {
    u32 cmd0;
    u32 mtxmode_proj;
    MtxFx44 projMtx;
    u32 mtxmode_posvec;
    MtxFx43 cameraMtx;
    u32 cmd1;
    u32 lightVec[4];
    u32 cmd2;
    u32 prmMatColor0;
    u32 prmMatColor1;
    u32 prmPolygonAttr;
    u32 prmViewPort;
    u32 cmd3;
    u32 lightColor[4];
    u32 cmd4;
    MtxFx33 prmBaseRot;
    VecFx32 prmBaseTrans;
    VecFx32 prmBaseScale;
    u32 prmTexImageParam;
    u32 flag;
    // The inverse matrices that are made as needed
    u8 unk100[0x140];
    // Where NNS_G3dGlbLookAt last put the camera
    VecFx32 camPos;
    VecFx32 camUp;
    VecFx32 camTarget;
} NNSG3dGlb;

#define NNS_G3D_GLB_FLAG_INVBASE_UPTODATE 0x00000004
#define NNS_G3D_GLB_FLAG_INVCAMERA_UPTODATE 0x00000008
#define NNS_G3D_GLB_FLAG_INVPROJ_UPTODATE 0x00000010
#define NNS_G3D_GLB_FLAG_INVBASECAMERA_UPTODATE 0x00000020
#define NNS_G3D_GLB_FLAG_INVCAMERAPROJ_UPTODATE 0x00000040
#define NNS_G3D_GLB_FLAG_BASECAMERA_UPTODATE 0x00000080

extern NNSG3dGlb NNS_G3dGlb;

// NNS_G3dInit, and NNS_G3dGlbInit
void NNS_G3DInit(void);
void NNS_G3DRenderStateInit(void);
void NNS_G3dGlbSetBaseTrans(const VecFx32 *trans);
void NNS_G3dGlbSetBaseScale(const VecFx32 *scale);
// NNS_G3dGlbLightVector and NNS_G3dGlbLightColor
void NNS_G3DSetLightVector(int lightID, fx16 x, fx16 y, fx16 z);
void NNS_G3DSetLightColor(int lightID, GXRgb rgb);
// NNS_G3dGlbMaterialColorDiffAmb and NNS_G3dGlbMaterialColorSpecEmi
void NNS_G3DSetMatDifAmb(GXRgb diffuse, GXRgb ambient, BOOL isSetVtxColor);
void NNS_G3DSetMatSpeEmi(GXRgb specular, GXRgb emission, BOOL isShininess);
// Send the global state with the projection matrix, the view and projection, or the base, view and projection:
// NNS_G3dGlbFlushP, NNS_G3dGlbFlushVP and NNS_G3dGlbFlushWVP
void NNS_G3DFlushRenderState(void);
void NNS_G3DFlushViewProjection(void);
void NNS_G3DFlushModelViewProjection(void);

static inline void NNS_G3dGlbFlush(void) {
    NNS_G3DFlushRenderState();
}

static inline void NNS_G3dGlbSetBaseRot(const MtxFx33 *rot) {
    MI_Copy36B(rot, &NNS_G3dGlb.prmBaseRot);
    NNS_G3dGlb.flag &= ~(NNS_G3D_GLB_FLAG_BASECAMERA_UPTODATE | NNS_G3D_GLB_FLAG_INVBASE_UPTODATE |
                         NNS_G3D_GLB_FLAG_INVBASECAMERA_UPTODATE);
}

static inline const MtxFx44 *NNS_G3dGlbGetProjectionMtx(void) {
    return &NNS_G3dGlb.projMtx;
}

static inline void NNS_G3dGlbSetProjectionMtx(const MtxFx44 *mtx) {
    sys_memcpy32_fast(mtx, &NNS_G3dGlb.projMtx, sizeof(MtxFx44));
    NNS_G3dGlb.flag &= ~(NNS_G3D_GLB_FLAG_INVPROJ_UPTODATE | NNS_G3D_GLB_FLAG_INVCAMERAPROJ_UPTODATE);
}

static inline void NNS_G3dGlbPerspectiveW(fx32 fovySin, fx32 fovyCos, fx32 aspect, fx32 n, fx32 f, fx32 scaleW) {
    MTX_PerspectiveW(fovySin, fovyCos, aspect, n, f, scaleW, &NNS_G3dGlb.projMtx);
    NNS_G3dGlb.flag &= ~(NNS_G3D_GLB_FLAG_INVPROJ_UPTODATE | NNS_G3D_GLB_FLAG_INVCAMERAPROJ_UPTODATE);
}

static inline void NNS_G3dGlbPerspective(fx32 fovySin, fx32 fovyCos, fx32 aspect, fx32 n, fx32 f) {
    NNS_G3dGlbPerspectiveW(fovySin, fovyCos, aspect, n, f, FX32_ONE);
}

static inline void NNS_G3dGlbFrustumW(fx32 t, fx32 b, fx32 l, fx32 r, fx32 n, fx32 f, fx32 scaleW) {
    MTX_FrustumW(t, b, l, r, n, f, scaleW, &NNS_G3dGlb.projMtx);
    NNS_G3dGlb.flag &= ~(NNS_G3D_GLB_FLAG_INVPROJ_UPTODATE | NNS_G3D_GLB_FLAG_INVCAMERAPROJ_UPTODATE);
}

static inline void NNS_G3dGlbFrustum(fx32 t, fx32 b, fx32 l, fx32 r, fx32 n, fx32 f) {
    NNS_G3dGlbFrustumW(t, b, l, r, n, f, FX32_ONE);
}

static inline void NNS_G3dGlbOrthoW(fx32 t, fx32 b, fx32 l, fx32 r, fx32 n, fx32 f, fx32 scaleW) {
    MTX_OrthoW(t, b, l, r, n, f, scaleW, &NNS_G3dGlb.projMtx);
    NNS_G3dGlb.flag &= ~(NNS_G3D_GLB_FLAG_INVPROJ_UPTODATE | NNS_G3D_GLB_FLAG_INVCAMERAPROJ_UPTODATE);
}

static inline void NNS_G3dGlbOrtho(fx32 t, fx32 b, fx32 l, fx32 r, fx32 n, fx32 f) {
    NNS_G3dGlbOrthoW(t, b, l, r, n, f, FX32_ONE);
}

static inline void NNS_G3dGlbLookAt(const VecFx32 *camPos, const VecFx32 *camUp, const VecFx32 *target) {
    NNS_G3dGlb.camPos = *camPos;
    NNS_G3dGlb.camUp = *camUp;
    NNS_G3dGlb.camTarget = *target;
    MTX_LookAt(camPos, camUp, target, &NNS_G3dGlb.cameraMtx);
    NNS_G3dGlb.flag &= ~(NNS_G3D_GLB_FLAG_INVCAMERA_UPTODATE | NNS_G3D_GLB_FLAG_INVBASECAMERA_UPTODATE |
                         NNS_G3D_GLB_FLAG_INVCAMERAPROJ_UPTODATE | NNS_G3D_GLB_FLAG_BASECAMERA_UPTODATE);
}

// The alpha of a material of a model resource, from 0 to 31
u32 NNS_G3DResMdlGetMatAlpha(const NNSG3dResMdl *mdl, u32 matId);
void NNS_G3DResMdlSetMatAlpha(NNSG3dResMdl *mdl, u32 matId, u32 alpha);
// The alpha of every material of a model resource, from 0 to 31 (NitroSystem's NNS_G3dMdlSetMdlAlphaAll)
void func_02068410(NNSG3dResMdl *mdl, int alpha);
// Turns fog on or off for every material of a model resource
void NNS_G3dMdlSetMdlFogEnableFlag(NNSG3dResMdl *mdl, int matID, BOOL enable);
void NNS_G3dMdlSetMdlFogEnableFlagAll(NNSG3dResMdl *mdl, BOOL enable);

// NitroSystem's NNS_G3dWorldPosToScrPos: where a point of the world is on the screen
int NNS_G3DProject(const VecFx32 *world, int *x, int *y);

#endif // POKEBW2_NNSYS_G3D_H
