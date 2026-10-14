#include "types.h"
#include "app/musical/musical_mcss.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "nnsys/g3d.h"
#include "pml/personal.h"
#include "pml/poke_graphic.h"
#include "system/mcss.h"

// Overlay 209's musical_mcss.c: the musical's copy of MCSS. Each sprite is a multi-cell animation whose cells are drawn
// as textured quads of one image in texture VRAM, described by the sprite's bin file: for each cell, one or two quads
// and the marker cells that it carries. A marker cell is not drawn but reported to the draw callback, which places the
// props by them

// The bin file: a cell's quads, then the marker cells' table
typedef struct {
    fx32 x;
    fx32 y;
    fx32 width;
    fx32 height;
    fx32 s;
    fx32 t;
} MusicalMcssQuad;

typedef struct {
    MusicalMcssQuad quad;
    // Drawn when its width is not 0
    MusicalMcssQuad quad2;
    // The marker cells, 0xff after the last
    u8 marks[8];
} MusicalMcssCell;

typedef struct {
    u32 unk0;
    u32 kind;
    fx32 x;
    fx32 y;
    // In units of 0x800
    u32 rotation;
} MusicalMcssMark;

typedef struct {
    u32 count;
    MusicalMcssCell cells[];
} MusicalMcssBin;

struct MusicalMcssSys {
    int count;
    MusicalMcss **mcss;
    // Whether the sprites are drawn with an orthographic projection of their own
    u32 ortho : 1;
    u32 texBase;
    u32 plttBase;
    u32 heapId;
    // The V-blank count at the last update, to step the animations by the frames since
    u32 vblankCount;
};

struct MusicalMcss {
    void *cellFile;
    NNSG2dCellDataBank *cellBank;
    void *mcFile;
    NNSG2dMultiCellDataBank *mcBank;
    void *animFile;
    NNSG2dAnimBankData *animBank;
    void *mcAnimFile;
    NNSG2dAnimBankData *mcAnimBank;
    MusicalMcssBin *bin;
    MusicalMcssMark *marks;
    NNSG2dMultiCellAnimation mcAnim;
    void *mcWork;
    NNSG2dImageProxy imageProxy;
    NNSG2dImagePaletteProxy plttProxy;
    VecFx32 pos;
    VecFx32 scale;
    VecFx32 unkE0;
    u32 flip : 1;
    u32 stopAnime : 1;
    u32 hidden : 1;
    // Its slot in the system
    u32 index;
    u32 heapId;
    void *work;
    u16 rotation;
    // The center of rotation, which marker cell 5 sets, and how far rotating moves it
    VecFx32 center;
    VecFx32 centerOffset;
    VecFx32 unk118;
};

// A sprite's characters and palette, loaded into VRAM at once or at the next V-blank
typedef struct {
    NNSG2dCharacterData *chars;
    NNSG2dPaletteData *pltt;
    NNSG2dImageProxy *imageProxy;
    NNSG2dImagePaletteProxy *plttProxy;
    void *charFile;
    void *plttFile;
    u32 charAddr;
    u32 plttAddr;
    MusicalMcss *mcss;
} MusicalMcssLoad;

// The files of Black and White's layout of the Pokémon graphics archive
#define MUSICAL_POKEGRA_EGG 650
#define MUSICAL_POKEGRA_FORM_FILES 0x32f0
#define MUSICAL_POKEGRA_FORM_PALETTES 0x37ad
#define MUSICAL_POKEGRA_SPINDA_BIN (SPECIES_SPINDA * POKEGRA_FILE_COUNT + POKEGRA_FILE_BIN)

// The files of each species' sprite, as in pokegra.c
#define POKEGRA_FILE_SINGLE_CELL_CHARS 0
#define POKEGRA_FILE_CHARS 2
#define POKEGRA_FILE_CELLS 4
#define POKEGRA_FILE_CELL_ANIMS 5
#define POKEGRA_FILE_MULTI_CELLS 6
#define POKEGRA_FILE_MULTI_CELL_ANIMS 7
#define POKEGRA_FILE_BIN 8
#define POKEGRA_FILE_BACK 9
#define POKEGRA_FILE_PALETTE 18
#define POKEGRA_FILE_COUNT 20

static void MusicalMcss_DrawQuad(MusicalMcss *mcss, fx32 x, fx32 y, fx32 width, fx32 height, fx32 s, fx32 t,
                                 const NNSG2dAnimDataSRT *cellSrt, const NNSG2dAnimDataSRT *mcSrt, u32 node, BOOL ortho,
                                 fx32 *z, u8 flip);
static void MusicalMcss_Load(MusicalMcssSys *sys, int index, MCSSLoadInfo *info, BOOL atVBlank);
static void MusicalMcss_LoadTask(TCB *tcb, void *data);
static void MusicalMcss_InitMCAnime(MusicalMcss *mcss, u32 mcType);
static void MusicalMcss_SetMaterial(void);
static void MusicalMcss_MulVecMtx44(const VecFx32 *vec, const MtxFx44 *mtx, VecFx32 *dest, fx32 *w);
static void MusicalMcss_GetDataIDBase(u32 arcId, int species, int form, u32 sex, BOOL rare, u32 dir, BOOL egg,
                                      u32 *base, u32 *dirOffset, u32 *sexOffset, u32 *rareOffset, u32 *formPalette,
                                      BOOL singleCell);

static const VecFx32 MUSICAL_MCSS_ORTHO_POS = { 0, 0, FX32_CONST(301) };
static const VecFx32 MUSICAL_MCSS_ORTHO_TARGET = { 0, 0, 0 };
static const VecFx32 MUSICAL_MCSS_ORTHO_UP = { 0, FX32_ONE, 0 };

MusicalMcssSys *MusicalMcss_InitSystem(u32 count, HeapID heapId) {
    MusicalMcssSys *sys = GFL_HeapAllocate(heapId, sizeof(MusicalMcssSys), TRUE, "musical_mcss.c", 132);

    sys->count = count;
    sys->heapId = heapId;
    sys->mcss = GFL_HeapAllocate(heapId, count * sizeof(MusicalMcss *), TRUE, "musical_mcss.c", 137);
    sys->vblankCount = OS_GetVBlankCount();
    sys->texBase = 0x30000;
    sys->plttBase = 0x1000;
    return sys;
}

void MusicalMcss_TermSystem(MusicalMcssSys *sys) {
    int i;

    for (i = 0; i < sys->count; i++) {
        if (sys->mcss[i] != NULL) {
            MusicalMcss_Del(sys, sys->mcss[i]);
        }
    }
    GFL_HeapFree(sys->mcss);
    GFL_HeapFree(sys);
}

void MusicalMcss_UpdateSystem(MusicalMcssSys *sys) {
    int i;
    u32 vblankCount = OS_GetVBlankCount();
    u32 frames = vblankCount - sys->vblankCount;

    for (i = 0; i < sys->count; i++) {
        if (sys->mcss[i] != NULL && sys->mcss[i]->stopAnime == FALSE) {
            NNS_G2dTickMCAnimation(&sys->mcss[i]->mcAnim, frames * FX32_ONE);
        }
    }
    sys->vblankCount = vblankCount;
}

// The current frame of an animation, as the SRT it may hold
static inline void MusicalMcss_GetAnimSrt(const NNSG2dAnimController *animCtrl, NNSG2dAnimDataSRT *srt) {
    switch ((u16)animCtrl->pAnimSequence->animType) {
    case NNS_G2D_ANIMELEM_INDEX:
        srt->index = *(u16 *)animCtrl->pCurrent->pContent;
        srt->rotZ = 0;
        srt->sx = FX32_ONE;
        srt->sy = FX32_ONE;
        srt->px = 0;
        srt->py = 0;
        break;
    case NNS_G2D_ANIMELEM_INDEX_SRT:
        *srt = *(NNSG2dAnimDataSRT *)animCtrl->pCurrent->pContent;
        break;
    case NNS_G2D_ANIMELEM_INDEX_T: {
        NNSG2dAnimDataT *data = animCtrl->pCurrent->pContent;

        srt->index = data->index;
        srt->rotZ = 0;
        srt->sx = FX32_ONE;
        srt->sy = FX32_ONE;
        srt->px = data->px;
        srt->py = data->py;
        break;
    }
    }
}

void MusicalMcss_DrawSystem(MusicalMcssSys *sys, MusicalMcssCellCallback callback) {
    const MusicalMcssCell *cell;
    MusicalMcss *mcss;
    NNSG2dImageProxy *imageProxy;
    int i;
    int j;
    NNSG2dMCCellAnimation **cellAnims;
    NNSG2dAnimDataSRT mcSrt;
    NNSG2dAnimDataSRT cellSrt;
    VecFx32 pos;
    MtxFx44 billboard;
    MtxFx43 invCamera;
    MtxFx33 rotation;
    MtxFx44 viewProj;
    MtxFx44 viewport;
    MtxFx33 centerRotation;
    MusicalMcssCellInfo info;
    fx32 x;
    fx32 y;
    fx32 z;
    fx32 w;
    u8 k;
    int rot;
    fx32 markX;
    fx32 markY;
    fx32 nodeX;
    fx32 nodeY;
    u8 flip;

    G3_PushMtx();
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_StoreMtx(0);
    G3_MtxMode(GX_MTXMODE_TEXTURE);
    G3_Identity();
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    if (sys->ortho == FALSE) {
        gfxLookAt(&NNS_G3dGlb.camPos, &NNS_G3dGlb.camUp, &NNS_G3dGlb.camTarget, TRUE, NULL);
    } else {
        gfxLookAt(&MUSICAL_MCSS_ORTHO_POS, &MUSICAL_MCSS_ORTHO_UP, &MUSICAL_MCSS_ORTHO_TARGET, TRUE, NULL);
    }
    MTX_Inverse43(&NNS_G3dGlb.cameraMtx, &invCamera);
    MI_Copy36B(&invCamera, &rotation);
    MAT3_To4x4(&rotation, &billboard);

    for (i = 0; i < sys->count; i++) {
        NNSG2dMultiCellAnimation *mcAnim;

        if (sys->mcss[i] == NULL || sys->mcss[i]->hidden) {
            continue;
        }
        G3_PushMtx();
        mcss = sys->mcss[i];
        imageProxy = &mcss->imageProxy;
        mcAnim = &mcss->mcAnim;
        cellAnims = (NNSG2dMCCellAnimation **)&mcss->mcAnim.multiCellInstance.pCellAnimArray;
        z = 0;
        MusicalMcss_GetAnimSrt(&mcAnim->animCtrl, &mcSrt);
        pos.x = mcss->pos.x;
        pos.y = mcss->pos.y;
        pos.z = mcss->pos.z;
        if (sys->ortho == FALSE) {
            x = mcSrt.px << 8;
            y = -mcSrt.py << 8;
        } else {
            viewport.m[0][0] = FX32_CONST(128);
            viewport.m[0][1] = 0;
            viewport.m[0][2] = 0;
            viewport.m[0][3] = FX32_CONST(128);
            viewport.m[1][0] = 0;
            viewport.m[1][1] = FX32_CONST(96);
            viewport.m[1][2] = 0;
            viewport.m[1][3] = FX32_CONST(-96);
            viewport.m[2][0] = 0;
            viewport.m[2][1] = 0;
            viewport.m[2][2] = -FX32_ONE;
            viewport.m[2][3] = FX32_ONE;
            viewport.m[3][0] = 0;
            viewport.m[3][1] = 0;
            viewport.m[3][2] = 0;
            viewport.m[3][3] = -FX32_ONE;
            MTX_Copy43To44_(&NNS_G3dGlb.cameraMtx, &viewProj);
            MTX_Concat44(&viewProj, &NNS_G3dGlb.projMtx, &viewProj);
            MusicalMcss_MulVecMtx44(&pos, &viewProj, &pos, &w);
            pos.x = FX_Div(pos.x, w);
            pos.y = FX_Div(pos.y, w);
            MusicalMcss_MulVecMtx44(&pos, &viewport, &pos, &w);
            x = FX32_CONST(mcSrt.px);
            y = FX32_CONST(-mcSrt.py);
        }
        pos.z = mcss->pos.z;
        flip = 0;
        if (mcss->scale.x < 0) {
            flip++;
        }
        G3_Translate(pos.x, pos.y, pos.z);
        if (sys->ortho == FALSE) {
            gfxMultMatrix4x4(&billboard);
        }
        G3_Translate(x, y, 0);
        {
            u16 rot = (flip & 1) ? 0x10000 - mcSrt.rotZ : mcSrt.rotZ;

            gfxRotateZ(-FX_SinIdx(rot), FX_CosIdx(rot));
        }
        G3_Scale(FX_Mul(mcSrt.sx, mcss->scale.x), FX_Mul(mcSrt.sy, mcss->scale.y), FX32_ONE);
        MTX_RotZ33_(&centerRotation, -FX_SinIdx(mcss->rotation), FX_CosIdx(mcss->rotation));
        MTX_MultVec33(&mcss->center, &centerRotation, &mcss->centerOffset);
        VEC_Subtract(&mcss->center, &mcss->centerOffset, &mcss->centerOffset);
        mcss->centerOffset.x = FX_Div(mcss->centerOffset.x, mcss->scale.x);
        mcss->centerOffset.y = FX_Div(mcss->centerOffset.y, mcss->scale.y);
        G3_StoreMtx(29);
        MusicalMcss_SetMaterial();
        G3_TexImageParam(imageProxy->attr.fmt, GX_TEXGEN_TEXCOORD, imageProxy->attr.sizeS, imageProxy->attr.sizeT,
                         GX_TEXREPEAT_ST, GX_TEXFLIP_ST, imageProxy->attr.plttUse,
                         imageProxy->vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_3DMAIN]);

        for (j = 0; j < mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt.index].numNodes; j++) {
            NNSG2dMCCellAnimation *cellAnim;

            cellAnim =
                &(*cellAnims)
                    [(mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt.index].pHierDataArray[j].nodeAttr &
                      0xff00) >>
                     8];
            MusicalMcss_GetAnimSrt(&cellAnim->cellAnim.animCtrl, &cellSrt);
            cell = &mcss->bin->cells[cellSrt.index];
            if (cell->quad2.width != 0) {
                if (mcss->flip) {
                    MusicalMcss_DrawQuad(mcss, cell->quad2.x, cell->quad2.y, cell->quad2.width, cell->quad2.height,
                                         cell->quad2.s, cell->quad2.t + cell->quad2.height, &cellSrt, &mcSrt, j,
                                         sys->ortho, &z, flip);
                } else {
                    MusicalMcss_DrawQuad(mcss, cell->quad2.x, cell->quad2.y, cell->quad2.width, cell->quad2.height,
                                         cell->quad2.s, cell->quad2.t, &cellSrt, &mcSrt, j, sys->ortho, &z, flip);
                }
            }
            MusicalMcss_DrawQuad(mcss, cell->quad.x, cell->quad.y, cell->quad.width, cell->quad.height, cell->quad.s,
                                 cell->quad.t, &cellSrt, &mcSrt, j, sys->ortho, &z, flip);

            for (k = 0; k < 8; k++) {
                u8 mark;
                fx32 offsetX;
                fx32 offsetY;
                fx32 cos;
                fx32 sin;
                fx32 scaleX;
                fx32 scaleY;

                mark = cell->marks[k];
                if (mark == 0xff) {
                    break;
                }
                rot = (u16)(mcSrt.rotZ + cellSrt.rotZ);
                markX = mcss->marks[mark].x;
                markY = mcss->marks[mark].y;
                sin = FX_SinIdx(rot);
                cos = FX_CosIdx(rot);
                scaleX = FX_Mul(mcSrt.sx, cellSrt.sx);
                scaleY = FX_Mul(mcSrt.sy, cellSrt.sy);
                offsetX = FX_Mul(markX * cos - markY * sin, scaleX);
                offsetY = FX_Mul(markX * sin + markY * cos, scaleY);
                nodeX =
                    mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt.index].pHierDataArray[j].pos.x;
                nodeX += mcSrt.px;
                nodeY =
                    mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt.index].pHierDataArray[j].pos.y;
                nodeY += mcSrt.py;
                offsetX += FX_Mul(nodeX * FX_CosIdx(mcSrt.rotZ) - nodeY * FX_SinIdx(mcSrt.rotZ), mcSrt.sx);
                offsetY += FX_Mul(nodeX * FX_SinIdx(mcSrt.rotZ) + nodeY * FX_CosIdx(mcSrt.rotZ), mcSrt.sy);
                offsetX += FX32_CONST(cellSrt.px);
                offsetY += FX32_CONST(cellSrt.py);

                info.pos = pos;
                info.pos.y = -info.pos.y;
                info.offset.x = FX_Mul(offsetX, mcss->scale.x / 16);
                info.offset.y = FX_Mul(offsetY, mcss->scale.y / 16);
                info.offset.z = z;
                info.center = mcss->center;
                info.rotation = mcss->rotation;
                info.scale = mcss->scale;
                info.cellRotation = rot + mcss->marks[mark].rotation * 0x800;
                callback((u8)mcss->marks[mark].kind, &info, mcss->work);
                if (mcss->marks[mark].kind == 5) {
                    VEC_Set(&mcss->center, info.offset.x, info.offset.y, 0);
                }
            }
        }
        G3_PopMtx(1);
    }
    G3_PopMtx(1);
}

static void MusicalMcss_DrawQuad(MusicalMcss *mcss, fx32 x, fx32 y, fx32 width, fx32 height, fx32 s, fx32 t,
                                 const NNSG2dAnimDataSRT *cellSrt, const NNSG2dAnimDataSRT *mcSrt, u32 node, BOOL ortho,
                                 fx32 *z, u8 flip) {
    u16 rot;
    fx32 nodeX;
    fx32 nodeY;

    if (ortho) {
        gfxOrtho(FX32_CONST(96), FX32_CONST(-95), FX32_CONST(-127), FX32_CONST(128), FX32_ONE, FX32_CONST(1000),
                 FX32_ONE, TRUE, NULL);
        G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    }
    G3_RestoreMtx(29);
    G3_TexPlttBase(mcss->plttProxy.vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_3DMAIN], mcss->plttProxy.fmt);
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 0, 31, 0);
    nodeX =
        (cellSrt->px + mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt->index].pHierDataArray[node].pos.x)
        << 8;
    nodeY =
        -(cellSrt->py + mcss->mcAnim.pMultiCellDataBank->pMultiCellDataArray[mcSrt->index].pHierDataArray[node].pos.y)
        << 8;
    if (flip == 0) {
        rot = mcss->rotation;
    } else if (flip & 1) {
        rot = 0x10000 - mcss->rotation;
    }
    gfxRotateZ(-FX_SinIdx(rot), FX_CosIdx(rot));
    G3_Translate(nodeX, nodeY, *z);
    gfxRotateZ(-FX_SinIdx(cellSrt->rotZ), FX_CosIdx(cellSrt->rotZ));
    if (flip == 0) {
        G3_Scale(cellSrt->sx, cellSrt->sy, FX32_ONE);
        G3_Translate(x, y, 0);
        G3_Scale(width, height, FX32_ONE);
    } else if (flip & 1) {
        G3_Scale(-cellSrt->sx, cellSrt->sy, FX32_ONE);
        G3_Translate(-x, y, 0);
        G3_Scale(width, height, FX32_ONE);
        G3_Translate(-0x100, 0, 0);
        s = FX32_CONST(512) - s - width;
    }
    G3_Begin(GX_BEGIN_QUADS);
    G3_TexCoord(s, t);
    G3_Vtx(0, 0, 0);
    G3_TexCoord(s + width, t);
    G3_Vtx(0x100, 0, 0);
    G3_TexCoord(s + width, t + height);
    G3_Vtx(0x100, -0x100, 0);
    G3_TexCoord(s, t + height);
    G3_Vtx(0, -0x100, 0);
    G3_End();
    G3_MtxMode(GX_MTXMODE_PROJECTION);
    G3_RestoreMtx(0);
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    if (ortho == FALSE) {
        *z -= 0x100;
    } else {
        *z -= 0x400;
    }
}

MusicalMcss *MusicalMcss_Add(MusicalMcssSys *sys, fx32 x, fx32 y, fx32 z, MCSSLoadInfo *info, void *work,
                             BOOL loadAtVBlank) {
    int i;

    for (i = 0; i < sys->count; i++) {
        if (sys->mcss[i] == NULL) {
            sys->mcss[i] = GFL_HeapAllocate(sys->heapId, sizeof(MusicalMcss), TRUE, "musical_mcss.c", 763);
            sys->mcss[i]->index = i;
            sys->mcss[i]->heapId = sys->heapId;
            sys->mcss[i]->pos.x = x;
            sys->mcss[i]->pos.y = y;
            sys->mcss[i]->pos.z = z;
            sys->mcss[i]->rotation = 0;
            sys->mcss[i]->scale.x = FX32_ONE;
            sys->mcss[i]->scale.y = FX32_ONE;
            sys->mcss[i]->scale.z = FX32_ONE;
            sys->mcss[i]->work = work;
            MusicalMcss_Load(sys, i, info, loadAtVBlank);
            break;
        }
    }
    return sys->mcss[i];
}

void MusicalMcss_Del(MusicalMcssSys *sys, MusicalMcss *mcss) {
    GFL_HeapFree(mcss->cellFile);
    GFL_HeapFree(mcss->animFile);
    GFL_HeapFree(mcss->mcFile);
    GFL_HeapFree(mcss->mcAnimFile);
    GFL_HeapFree(mcss->mcWork);
    GFL_HeapFree(mcss->bin);
    sys->mcss[mcss->index] = NULL;
    GFL_HeapFree(mcss);
}

void MusicalMcss_SetOrthoMode(MusicalMcssSys *sys) {
    sys->ortho = TRUE;
}

void MusicalMcss_SetTexBase(MusicalMcssSys *sys, u32 base) {
    sys->texBase = base;
}

void MusicalMcss_SetPlttBase(MusicalMcssSys *sys, u32 base) {
    sys->plttBase = base;
}

void MusicalMcss_SetPosition(MusicalMcss *mcss, VecFx32 *pos) {
    mcss->pos.x = pos->x;
    mcss->pos.y = pos->y;
    mcss->pos.z = pos->z;
}

void MusicalMcss_SetScale(MusicalMcss *mcss, VecFx32 *scale) {
    mcss->scale.x = scale->x;
    mcss->scale.y = scale->y;
    mcss->scale.z = scale->z;
}

void MusicalMcss_SetRotation(MusicalMcss *mcss, u16 rotation) {
    mcss->rotation = rotation;
}

void MusicalMcss_SetFlip(MusicalMcss *mcss) {
    mcss->flip = TRUE;
}

void MusicalMcss_ResetFlip(MusicalMcss *mcss) {
    mcss->flip = FALSE;
}

void MusicalMcss_StopAnime(MusicalMcss *mcss) {
    mcss->stopAnime = TRUE;
}

void MusicalMcss_StartAnime(MusicalMcss *mcss) {
    mcss->stopAnime = FALSE;
}

void MusicalMcss_ChangeAnime(MusicalMcss *mcss, u16 anime) {
    const NNSG2dAnimSequence *seq = NNS_G2dGetAnimSequenceByIdx(mcss->mcAnimBank, anime);

    // BUG: The work of the old animation is never freed
#ifdef BUGFIX
    GFL_HeapFree(mcss->mcWork);
#endif
    MusicalMcss_InitMCAnime(mcss, NNS_G2D_MCTYPE_SHARE_CELLANIM);
    NNS_G2dSetAnimSequenceToMCAnimation(&mcss->mcAnim, seq);
}

BOOL MusicalMcss_IsHidden(MusicalMcss *mcss) {
    return mcss->hidden;
}

void MusicalMcss_Hide(MusicalMcss *mcss) {
    mcss->hidden = TRUE;
}

void MusicalMcss_Show(MusicalMcss *mcss) {
    mcss->hidden = FALSE;
}

void MusicalMcss_CopyState(MusicalMcss *src, MusicalMcss *dst) {
    dst->pos = src->pos;
    dst->scale = src->scale;
    dst->unkE0 = src->unkE0;
    dst->flip = src->flip;
    dst->stopAnime = src->stopAnime;
    dst->hidden = src->hidden;
    dst->rotation = src->rotation;
    dst->center = src->center;
    dst->centerOffset = src->centerOffset;
}

static void MusicalMcss_Load(MusicalMcssSys *sys, int index, MCSSLoadInfo *info, BOOL atVBlank) {
    MusicalMcss *mcss = sys->mcss[index];
    const NNSG2dAnimSequence *seq;
    MusicalMcssLoad *load;

    mcss->hidden = TRUE;
    NNS_G2dInitImageProxy(&mcss->imageProxy);
    NNS_G2dInitImagePaletteProxy(&mcss->plttProxy);
    mcss->cellFile = GFL_G2DIOReadNCER(info->arcId, info->cells, FALSE, &mcss->cellBank, mcss->heapId);
    mcss->animFile = GFL_G2DIOReadNANR(info->arcId, info->cellAnime, TRUE, &mcss->animBank, mcss->heapId);
    mcss->mcFile = GFL_G2DIOReadNMCR(info->arcId, info->multiCells, FALSE, &mcss->mcBank, mcss->heapId);
    mcss->mcAnimFile = GFL_G2DIOReadNMAR(info->arcId, info->multiCellAnime, FALSE, &mcss->mcAnimBank, mcss->heapId);
    seq = NNS_G2dGetAnimSequenceByIdx(mcss->mcAnimBank, 0);
    MusicalMcss_InitMCAnime(mcss, NNS_G2D_MCTYPE_SHARE_CELLANIM);
    NNS_G2dSetAnimSequenceToMCAnimation(&mcss->mcAnim, seq);
    mcss->bin = GFL_ArcSysReadHeapNew(info->arcId, info->bin, mcss->heapId);
    mcss->marks = (MusicalMcssMark *)&mcss->bin->cells[mcss->bin->count];

    load = GFL_HeapAllocate(mcss->heapId, sizeof(MusicalMcssLoad), TRUE, "musical_mcss.c", 1104);
    load->imageProxy = &mcss->imageProxy;
    load->plttProxy = &mcss->plttProxy;
    load->charAddr = sys->texBase + index * 0x4000;
    load->plttAddr = sys->plttBase + index * 0x20;
    load->mcss = mcss;
    load->charFile = GFL_G2DIOReadBGNCGR(info->arcId, info->character, TRUE, &load->chars, mcss->heapId);
    load->plttFile = GFL_G2DIOReadNCLR(info->arcId, info->palette, &load->pltt, mcss->heapId);
    if (info->bin == MUSICAL_POKEGRA_SPINDA_BIN) {
        MCSS_DrawSpindaSpots(load->chars->rawData, info->unk20);
    }
    if (atVBlank == TRUE) {
        GFL_VBlankTCBAdd(MusicalMcss_LoadTask, load, 0);
    } else {
        MusicalMcss_LoadTask(NULL, load);
    }
}

static void MusicalMcss_LoadTask(TCB *tcb, void *data) {
    MusicalMcssLoad *load = data;

    if (load->mcss != NULL) {
        load->mcss->hidden = FALSE;
    }
    if (load->charFile != NULL) {
        NNS_G2dLoadImage2DMapping(load->chars, load->charAddr, NNS_G2D_VRAM_TYPE_3DMAIN, load->imageProxy);
        GFL_HeapFree(load->charFile);
    }
    if (load->pltt != NULL) {
        NNS_G2dLoadPalette(load->pltt, load->plttAddr, NNS_G2D_VRAM_TYPE_3DMAIN, load->plttProxy);
        GFL_HeapFree(load->plttFile);
    }
    GFL_HeapFree(load);
    if (tcb != NULL) {
        GFL_TCBRemove(tcb);
    }
}

static void MusicalMcss_InitMCAnime(MusicalMcss *mcss, u32 mcType) {
    u32 size = NNS_G2dGetMCWorkAreaSize(mcss->mcBank, mcType);

    mcss->mcWork = GFL_HeapAllocate(mcss->heapId, size, FALSE, "musical_mcss.c", 1216);
    NNS_G2dInitMCAnimationInstance(&mcss->mcAnim, mcss->mcWork, mcss->animBank, mcss->cellBank, mcss->mcBank, mcType);
}

static void MusicalMcss_SetMaterial(void) {
    G3_MaterialColorDiffAmb(GX_RGB(31, 31, 31), GX_RGB(16, 16, 16), TRUE);
    G3_MaterialColorSpecEmi(GX_RGB(16, 16, 16), GX_RGB(0, 0, 0), FALSE);
    G3_PolygonAttr(GX_LIGHTMASK_NONE, GX_POLYGONMODE_MODULATE, GX_CULL_NONE, 0, 31, 0);
}

static void MusicalMcss_MulVecMtx44(const VecFx32 *vec, const MtxFx44 *mtx, VecFx32 *dest, fx32 *w) {
    fx64 y = vec->y;
    fx64 x = vec->x;
    fx64 z = vec->z;

    dest->x = (fx32)((x * mtx->m[0][0] + y * mtx->m[1][0] + z * mtx->m[2][0]) >> FX32_SHIFT);
    dest->x += mtx->m[3][0];
    dest->y = (fx32)((x * mtx->m[0][1] + y * mtx->m[1][1] + z * mtx->m[2][1]) >> FX32_SHIFT);
    dest->y += mtx->m[3][1];
    dest->z = (fx32)((x * mtx->m[0][2] + y * mtx->m[1][2] + z * mtx->m[2][2]) >> FX32_SHIFT);
    dest->z += mtx->m[3][2];
    *w = (fx32)((x * mtx->m[0][3] + y * mtx->m[1][3] + z * mtx->m[2][3]) >> FX32_SHIFT);
    *w += mtx->m[3][3];
}

u32 MusicalMcss_GetCharacterDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg) {
    u32 base, dirOffset, sexOffset;

    MusicalMcss_GetDataIDBase(arcId, species, form, sex, rare, dir, egg, &base, &dirOffset, &sexOffset, NULL, NULL,
                              FALSE);
    return base + dirOffset + POKEGRA_FILE_CHARS + sexOffset;
}

u32 MusicalMcss_GetPaletteDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg) {
    u32 base, rareOffset;
    u32 formPalette = 0;

    MusicalMcss_GetDataIDBase(arcId, species, form, sex, rare, dir, egg, &base, NULL, NULL, &rareOffset, &formPalette,
                              FALSE);
    if (formPalette == 0) {
        formPalette = base + POKEGRA_FILE_PALETTE + rareOffset;
    }
    return formPalette;
}

u32 MusicalMcss_GetCellsDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg) {
    u32 base, dirOffset;

    MusicalMcss_GetDataIDBase(arcId, species, form, sex, rare, dir, egg, &base, &dirOffset, NULL, NULL, NULL, FALSE);
    return base + dirOffset + POKEGRA_FILE_CELLS;
}

u32 MusicalMcss_GetCellAnimeDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg) {
    u32 base, dirOffset;

    MusicalMcss_GetDataIDBase(arcId, species, form, sex, rare, dir, egg, &base, &dirOffset, NULL, NULL, NULL, FALSE);
    return base + dirOffset + POKEGRA_FILE_CELL_ANIMS;
}

u32 MusicalMcss_GetMultiCellsDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg) {
    u32 base, dirOffset;

    MusicalMcss_GetDataIDBase(arcId, species, form, sex, rare, dir, egg, &base, &dirOffset, NULL, NULL, NULL, FALSE);
    return base + dirOffset + POKEGRA_FILE_MULTI_CELLS;
}

u32 MusicalMcss_GetMultiCellAnimeDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg) {
    u32 base, dirOffset;

    MusicalMcss_GetDataIDBase(arcId, species, form, sex, rare, dir, egg, &base, &dirOffset, NULL, NULL, NULL, FALSE);
    return base + dirOffset + POKEGRA_FILE_MULTI_CELL_ANIMS;
}

u32 MusicalMcss_GetBinFileDataNo(u32 arcId, u32 species, u32 form, u32 sex, BOOL rare, u32 dir, BOOL egg) {
    u32 base, dirOffset;

    MusicalMcss_GetDataIDBase(arcId, species, form, sex, rare, dir, egg, &base, &dirOffset, NULL, NULL, NULL, FALSE);
    return base + dirOffset + POKEGRA_FILE_BIN;
}

// pokegra.c's GetPokemonDataIDBase for the older layout, where the egg's sprite comes earlier and the species' files
// start at the first species
static void MusicalMcss_GetDataIDBase(u32 arcId, int species, int form, u32 sex, BOOL rare, u32 dir, BOOL egg,
                                      u32 *base, u32 *dirOffset, u32 *sexOffset, u32 *rareOffset, u32 *formPalette,
                                      BOOL singleCell) {
    u32 file = species * POKEGRA_FILE_COUNT;
    u32 dirFile = dir == POKEGRA_DIR_FRONT ? 0 : POKEGRA_FILE_BACK;
    u32 charsFile;

    if (singleCell) {
        charsFile = dir == POKEGRA_DIR_FRONT ? POKEGRA_FILE_SINGLE_CELL_CHARS
                                             : POKEGRA_FILE_BACK + POKEGRA_FILE_SINGLE_CELL_CHARS;
    } else {
        charsFile = dir == POKEGRA_DIR_FRONT ? POKEGRA_FILE_CHARS : POKEGRA_FILE_BACK + POKEGRA_FILE_CHARS;
    }

    if (egg) {
        file = (MUSICAL_POKEGRA_EGG + (species == SPECIES_MANAPHY)) * POKEGRA_FILE_COUNT;
    } else if (form != 0) {
        u32 spriteOffset = PML_PersonalGetParamSingleBW1(species, 0, PERSONAL_FORM_SPRITE_OFFSET);
        u32 paletteForms = PML_PersonalGetParamSingleBW1(species, 0, PERSONAL_PALETTE_FORMS);
        int formCount = PML_PersonalGetParamSingleBW1(species, 0, PERSONAL_FORM_COUNT);

        if (form >= formCount) {
            form = 0;
        }
        if (paletteForms) {
            if (formPalette != NULL && form != 0) {
                *formPalette = MUSICAL_POKEGRA_FORM_PALETTES + (spriteOffset + form - 1) * 2 + rare;
            }
        } else if (form != 0) {
            file = MUSICAL_POKEGRA_FORM_FILES + (spriteOffset + form - 1) * POKEGRA_FILE_COUNT;
        }
    }

    switch (sex) {
    case GENDER_MALE:
        break;
    case GENDER_FEMALE:
        sex = GFL_ArcSysGetDataLength(arcId, file + charsFile + 1) != 0 ? GENDER_FEMALE : GENDER_MALE;
        break;
    case GENDER_UNKNOWN:
        sex = GENDER_MALE;
        break;
    }

    if (base != NULL) {
        *base = file;
    }
    if (dirOffset != NULL) {
        *dirOffset = dirFile;
    }
    if (sexOffset != NULL) {
        *sexOffset = sex;
    }
    if (rareOffset != NULL) {
        *rareOffset = rare;
    }
}
