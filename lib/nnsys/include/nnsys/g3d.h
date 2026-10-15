#ifndef POKEBW2_NNSYS_G3D_H
#define POKEBW2_NNSYS_G3D_H

#include "types.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
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

// A model's material block: which materials each texture and each palette is for, then the materials by name
typedef struct {
    u16 ofsDictTexToMatList;
    u16 ofsDictPlttToMatList;
    NNSG3dResDict dict;
} NNSG3dResMat;

// The start of a material. Binding a texture adds its place in VRAM to texImageParam, and sets magW and magH to scale
// the texture coordinates by how much the stored texture differs from the size the material was made for, from
// origWidth and origHeight. Binding a palette sets texPlttBase
typedef struct {
    u16 itemTag;
    u16 size;
    u32 diffAmb;
    u32 specEmi;
    u32 polyAttr;
    u32 polyAttrMask;
    u32 texImageParam;
    u32 texImageParamMask;
    u16 texPlttBase;
    u16 flag;
    u16 origWidth;
    u16 origHeight;
    fx32 magW;
    fx32 magH;
} NNSG3dResMatData;

// An entry of a texture-to-material or palette-to-material dictionary: numIdx material numbers, one byte each, at
// offset from the material block. flag has NNS_G3D_MATLIST_BOUND once the texture or palette is bound to them
typedef struct {
    u16 offset;
    u8 numIdx;
    u8 flag;
} NNSG3dResDictTexToMatListData;

typedef NNSG3dResDictTexToMatListData NNSG3dResDictPlttToMatListData;

#define NNS_G3D_MATLIST_BOUND 0x01

static inline NNSG3dResMat *NNS_G3dGetMat(const NNSG3dResMdl *mdl) {
    if (mdl != NULL && mdl->ofsMat != 0) {
        return (NNSG3dResMat *)((u8 *)mdl + mdl->ofsMat);
    }
    return NULL;
}

static inline NNSG3dResMatData *NNS_G3dGetMatDataByIdx(const NNSG3dResMat *mat, u32 idx) {
    if (mat != NULL) {
        const u32 *ofs = NNS_G3dGetResDataByIdx(&mat->dict, idx);

        if (ofs != NULL) {
            return (NNSG3dResMatData *)((u8 *)mat + *ofs);
        }
    }
    return NULL;
}

// The start of every animation
typedef struct {
    u8 category0;
    u8 revision;
    u16 category1;
    u16 numFrame;
} NNSG3dResAnmCommon;

// The category of an animation resource's header: which part of a model it animates
#define NNS_G3D_DATABLK_MAT_ANM 'M'
#define NNS_G3D_DATABLK_JNT_ANM 'J'
#define NNS_G3D_DATABLK_VIS_ANM 'V'

// mapData of an animation object has this for each material or node that the animation holds data for, with the index
// of the data in the animation in the low byte. The blends skip a target that has DISABLED set as well
#define NNS_G3D_ANMOBJ_MAPDATA_EXIST 0x0100
#define NNS_G3D_ANMOBJ_MAPDATA_DISABLED 0x0200
#define NNS_G3D_ANMOBJ_MAPDATA_DATAFIELD 0x00ff

// An animation applied to a model, at a frame. Animations bound to a render object are linked by priority through
// next. mapData maps each material or node of the model to the animation's data for it
typedef struct NNSG3dAnmObj {
    fx32 frame;
    fx32 ratio;
    const void *resAnm;
    void *funcAnm;
    struct NNSG3dAnmObj *next;
    const NNSG3dResTex *resTex;
    u8 priority;
    u8 numMapData;
    u16 mapData[1];
} NNSG3dAnmObj;

// The bytes an animation object takes for a model with numMapData materials or nodes, to a multiple of four
#define NNS_G3D_ANMOBJ_SIZE(numMapData) ((sizeof(NNSG3dAnmObj) + (numMapData) * sizeof(u16)) & ~3)

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

// The second category of an animation resource's header, which tells the kind of animation within what it animates
#define NNS_G3D_ANM_CATEGORY1_MA 'MA'
#define NNS_G3D_ANM_CATEGORY1_TP 'TP'
#define NNS_G3D_ANM_CATEGORY1_TA 'TA'
#define NNS_G3D_ANM_CATEGORY1_VA 'VA'
#define NNS_G3D_ANM_CATEGORY1_CA 'CA'

// What an animation computes for one material, joint or node. The function in an animation object's funcAnm fills one
// in from the data at the index of the animation's own data that mapData gives, and a blend combines the results of all
// the animation objects of a list. The material's, shared by the material, texture SRT and texture pattern animations,
// has no layout yet
typedef struct NNSG3dMatAnmResult NNSG3dMatAnmResult;

// A joint's flag says which of its parts are the identity: a scale of one, a zero rotation or translation. A blend
// keeps only those that all its animations have. The rotation is three rows of a matrix, of which a blend adds up the
// first two and makes the third
#define NNS_G3D_JNTANM_RESULTFLAG_SCALE_ONE 0x00000001
#define NNS_G3D_JNTANM_RESULTFLAG_ROT_ZERO 0x00000002
#define NNS_G3D_JNTANM_RESULTFLAG_TRANS_ZERO 0x00000004
#define NNS_G3D_JNTANM_RESULTFLAG_SCALEEX0_ONE 0x00000008
#define NNS_G3D_JNTANM_RESULTFLAG_SCALEEX1_ONE 0x00000010

typedef struct {
    u32 flag;
    VecFx32 scale;
    VecFx32 scaleEx0;
    VecFx32 scaleEx1;
    union {
        MtxFx33 rot;
        VecFx32 rotRow[3];
    };
    VecFx32 trans;
} NNSG3dJntAnmResult;

typedef struct {
    u32 isVisible;
} NNSG3dVisAnmResult;

typedef void (*NNSG3dFuncAnmMat)(NNSG3dMatAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx);
typedef void (*NNSG3dFuncAnmJnt)(NNSG3dJntAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx);
typedef void (*NNSG3dFuncAnmVis)(NNSG3dVisAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx);

// What combines the animations of a render object's list for a material or node, and returns whether any had data for
// it
typedef BOOL (*NNSG3dFuncBlendMat)(NNSG3dMatAnmResult *result, const NNSG3dAnmObj *anmObj, u32 matID);
typedef BOOL (*NNSG3dFuncBlendJnt)(NNSG3dJntAnmResult *result, const NNSG3dAnmObj *anmObj, u32 nodeID);
typedef BOOL (*NNSG3dFuncBlendVis)(NNSG3dVisAnmResult *result, const NNSG3dAnmObj *anmObj, u32 nodeID);

// Set in the flag of a render object when its animations changed, so that its hints are out of date
#define NNS_G3D_RENDEROBJ_FLAG_HINT_OBSOLETE 0x00000010

// What draws a model, 0x54 bytes. The animations bound to it are in three lists by what they animate, and hint*Exist
// has a bit for each material or node that one of them has data for
typedef struct {
    u32 flag;
    NNSG3dResMdl *resMdl;
    NNSG3dAnmObj *anmMat;
    NNSG3dFuncBlendMat funcBlendMat;
    NNSG3dAnmObj *anmJnt;
    NNSG3dFuncBlendJnt funcBlendJnt;
    NNSG3dAnmObj *anmVis;
    NNSG3dFuncBlendVis funcBlendVis;
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

// The data of a texture block's dictionary entry: the texture's GX_TEXIMAGE_PARAM, with its offset in the low bits, and
// the texture's original width and height in extraParam
#define NNS_G3D_RESDICTTEX_EXTRA_WIDTH_MASK 0x000007ff
#define NNS_G3D_RESDICTTEX_EXTRA_HEIGHT_MASK 0x003ff800
#define NNS_G3D_RESDICTTEX_EXTRA_HEIGHT_SHIFT 11

typedef struct {
    u32 texImageParam;
    u32 extraParam;
} NNSG3dResDictTexData;

// The data of a palette dictionary entry: the palette's offset in 8-byte units, and whether it is a 4-color palette
#define NNS_G3D_RESDICTPLTT_USEPLTT4 0x0001

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

// Binding a model's materials to a texture block's textures and palettes by name, which sets what the materials draw
// with. Both return FALSE if a name of the model is not in the texture block. A texture or palette that is bound
// already is left alone
BOOL NNS_G3dBindMdlTex(NNSG3dResMdl *mdl, const NNSG3dResTex *tex);
BOOL NNS_G3dBindMdlPltt(NNSG3dResMdl *mdl, const NNSG3dResTex *tex);
// Binds the palette at plttNameIdx of the texture block to the materials of the palette-to-material entry
// plttToMatListIdx, unless that entry is bound already, and returns whether it did. NNS_G3dReleaseMdlPltt clears the
// bound mark of every palette-to-material entry, so that they can be bound again
BOOL NNS_G3dForceBindMdlPltt(NNSG3dResMdl *mdl, const NNSG3dResTex *tex, u32 plttToMatListIdx, u32 plttNameIdx);
void NNS_G3dReleaseMdlPltt(NNSG3dResMdl *mdl);

// Render and animation objects: NNS_G3dRenderObjInit, NNS_G3dAnmObjInit, NNS_G3dRenderObjAddAnmObj and
// NNS_G3dAllocAnmObj, and drawing one with NNS_G3dDraw
NNSG3dRenderObj *NNS_G3dAllocRenderObj(NNSFndAllocator *allocator);
void NNS_G3dFreeRenderObj(NNSFndAllocator *allocator, NNSG3dRenderObj *obj);
void NNS_G3DModelAttachResource(NNSG3dRenderObj *obj, NNSG3dResMdl *mdl);
NNSG3dAnmObj *NNS_G3DAnimationAlloc(NNSFndAllocator *allocator, const void *anm, const NNSG3dResMdl *mdl);
u32 NNS_G3DAnimationCalcAllocSize(const void *anm, const NNSG3dResMdl *mdl);
void NNS_G3dFreeAnmObj(NNSFndAllocator *allocator, NNSG3dAnmObj *anmObj);
void NNS_G3DAnimationCreate(NNSG3dAnmObj *anmObj, void *anm, const NNSG3dResMdl *mdl, const NNSG3dResTex *tex);
void NNS_G3DAnimationBind(NNSG3dRenderObj *obj, NNSG3dAnmObj *anmObj);
void NNS_G3dRenderObjRemoveAnmObj(NNSG3dRenderObj *obj, NNSG3dAnmObj *anmObj);
void NNS_G3DDraw(NNSG3dRenderObj *obj);

// What starts an animation of a kind of resource: its header, and the function that fills in the animation object
typedef struct {
    NNSG3dResAnmHeader header;
    void (*init)(NNSG3dAnmObj *anmObj, const void *anm, const NNSG3dResMdl *mdl);
} NNSG3dAnmObjInit;

// The table that NNS_G3DAnimationCreate searches for the kind of animation it is given, and how many of its entries are
// in use (anm.c)
extern u32 NNS_G3D_ANIMATION_INIT_HEADER_COUNT;
extern NNSG3dAnmObjInit NNS_G3D_ANIMATION_INIT_HEADERS[];
// The functions that fill in an animation object of each kind, which its init function (NitroSystem's
// NNS_G3dAnmObjInitNsBma, NsBtp, NsBta, NsBva and NsBca, in nsbma.c, nsbtp.c, nsbta.c, nsbva.c and nsbca.c) copies
// from these variables to funcAnm (anm.c). The NNS_G3dFuncAnm names are a guess from the files
void func_020698d4(NNSG3dAnmObj *anmObj, const void *anm, const NNSG3dResMdl *mdl);
void NNS_G3DAnimationInit_TP(NNSG3dAnmObj *anmObj, const void *anm, const NNSG3dResMdl *mdl);
void NNS_G3DAnimationInit_TA(NNSG3dAnmObj *anmObj, const void *anm, const NNSG3dResMdl *mdl);
void func_02069ee4(NNSG3dAnmObj *anmObj, const void *anm, const NNSG3dResMdl *mdl);
void func_02068764(NNSG3dAnmObj *anmObj, const void *anm, const NNSG3dResMdl *mdl);
void func_02069968(NNSG3dMatAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx);
void func_02069cf8(NNSG3dMatAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx);
void func_02069ea0(NNSG3dMatAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx);
void func_02069f18(NNSG3dVisAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx);
void NNS_G3DAnimationApply_CA(NNSG3dJntAnmResult *result, const NNSG3dAnmObj *anmObj, u32 dataIdx);
extern void *NNS_G3dFuncAnmVisNsBva;
extern void *NNS_G3dFuncAnmJntNsBca;
extern void *NNS_G3dFuncAnmMatNsBtp;
extern void *NNS_G3dFuncAnmMatNsBta;
extern void *NNS_G3dFuncAnmMatNsBma;

// The functions that combine the animations of a render object, for its materials, joints and visibility (NitroSystem's
// NNSi_G3dAnmBlendMat, NNSi_G3dAnmBlendJnt and NNSi_G3dAnmBlendVis). The variables after them are what
// NNS_G3DModelAttachResource copies to funcBlendMat, funcBlendJnt and funcBlendVis (anm.c)
BOOL NNSi_G3dAnmBlendMat(NNSG3dMatAnmResult *result, const NNSG3dAnmObj *anmObj, u32 matID);
BOOL NNSi_G3dAnmBlendJnt(NNSG3dJntAnmResult *result, const NNSG3dAnmObj *anmObj, u32 nodeID);
BOOL NNSi_G3dAnmBlendVis(NNSG3dVisAnmResult *result, const NNSG3dAnmObj *anmObj, u32 nodeID);
extern NNSG3dFuncBlendMat NNS_G3dFuncBlendMat;
extern NNSG3dFuncBlendJnt NNS_G3dFuncBlendJnt;
extern NNSG3dFuncBlendVis NNS_G3dFuncBlendVis;

// The functions that a model's scaling rule and texture matrix mode choose between when it is drawn, in tables indexed
// by the rule or the mode (cgtool.c). A flush sends a joint's result to the geometry engine: its scale, rotation and
// translation, as the result's flag leaves them. A scale function sets the scale parts of a joint's result from the
// scale of the node, a vector of the scale and then one of its inverse, the node's command in the stream and its flags,
// where bit 2 is a scale of one. A texture matrix function sends the texture matrix of a material's result
typedef void (*NNSG3dFuncJointFlush)(const NNSG3dJntAnmResult *result);
typedef void (*NNSG3dFuncJointScale)(NNSG3dJntAnmResult *result, const VecFx32 *scale, const u8 *cmd, u32 srtFlag);
typedef void (*NNSG3dFuncTexMatrix)(const NNSG3dMatAnmResult *result);

// Standard (basic.c), Maya (maya.c) and Softimage (si3d.c) scaling rules
void func_02069f40(const NNSG3dJntAnmResult *result);
void func_02069fbc(const NNSG3dJntAnmResult *result);
void func_0206a714(const NNSG3dJntAnmResult *result);
void func_02069f94(NNSG3dJntAnmResult *result, const VecFx32 *scale, const u8 *cmd, u32 srtFlag);
void func_0206a044(NNSG3dJntAnmResult *result, const VecFx32 *scale, const u8 *cmd, u32 srtFlag);
void NNSi_G3dGetJointScaleSi3d(NNSG3dJntAnmResult *result, const VecFx32 *scale, const u8 *cmd, u32 srtFlag);
// Maya (maya.c), Softimage (si3d.c), 3ds Max (3dsmax.c) and XSI (xsi.c) texture matrix modes
void func_0206a5cc(const NNSG3dMatAnmResult *result);
void func_0206aa5c(const NNSG3dMatAnmResult *result);
void func_0206b124(const NNSG3dMatAnmResult *result);
void func_0206b74c(const NNSG3dMatAnmResult *result);

extern NNSG3dFuncJointFlush NNS_G3D_JOINT_FLUSH_FUNCS[3];
extern NNSG3dFuncJointScale NNS_G3D_JOINT_SCALE_FUNCS[3];
extern NNSG3dFuncTexMatrix NNS_G3D_TEX_MATRIX_FUNCS[4];

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

static inline void NNS_G3dGeMtxMode(int mode) {
    u32 param = mode;

    // MTX_MODE, 0x10 (gx.h has no name for it yet)
    NNS_G3DExecOp(0x10, &param, 1);
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

// The global state of the geometry engine. The cmd words hold the geometry commands that send what follows them, so
// that a flush is one run of the structure
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
    // The base transformation of the models: the rotation and translation, which are also one 4x3 matrix, and the scale
    union {
        MtxFx43 prmBaseMtx;
        struct {
            MtxFx33 prmBaseRot;
            VecFx32 prmBaseTrans;
        };
    };
    VecFx32 prmBaseScale;
    u32 prmTexImageParam;
    u32 flag;
    // The matrices that are made as needed, each with the flag that says it is up to date: the inverse of the camera
    // matrix (INVCAMERA), the base transformation applied to the camera matrix (BASECAMERA) and its inverse (made with
    // it), the inverse of the base transformation (INVBASE, which no code reads), the inverse of the projection matrix
    // (INVPROJ), and the inverse of the camera and projection matrices together (INVCAMERAPROJ)
    MtxFx43 invCameraMtx;
    MtxFx43 baseCameraMtx;
    MtxFx43 invBaseCameraMtx;
    MtxFx43 invBaseMtx;
    MtxFx44 invProjMtx;
    MtxFx44 invCameraProjMtx;
    // Where NNS_G3dGlbLookAt last put the camera
    VecFx32 camPos;
    VecFx32 camUp;
    VecFx32 camTarget;
} NNSG3dGlb;

// What the last flush sent: with neither flag, the projection matrix alone, the camera and the base transformation
// going to the position matrix; with FLUSH_VP, the camera joins the projection matrix; with FLUSH_WVP, so does the base
// transformation. The flush functions set and clear them
#define NNS_G3D_GLB_FLAG_FLUSH_WVP 0x00000001
#define NNS_G3D_GLB_FLAG_FLUSH_VP 0x00000002
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

// The inverse of the camera matrix; the base transformation applied to the camera matrix (the model view matrix of a
// model drawn at the origin) and its inverse; and the inverse of the camera and projection matrices together. Each is
// made when it is first asked for after what it depends on changed, and the pointers are into NNS_G3dGlb
const MtxFx43 *NNS_G3DGetInvViewMatrix(void);
const MtxFx43 *NNS_G3DGetTempMVMatrix(void);
const MtxFx43 *NNS_G3DGetTempInvMVMatrix(void);
const MtxFx44 *NNS_G3DGetInvViewProjectionMatrix(void);
// The viewport that the global state holds; a null pointer leaves its value out
void NNS_G3DGetViewport(int *x1, int *y1, int *x2, int *y2);

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
