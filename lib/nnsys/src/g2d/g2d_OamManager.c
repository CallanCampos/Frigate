#include "nnsys/g2d.h"

#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// NitroSystem's g2d_OamManager.c: the OAM managers. Each type of OAM has a buffer of its OAMs and affine parameters,
// which the managers share out in ranges, and a manager fills its range each frame and sends it to the hardware

#define OAM_NUM 128
#define AFFINE_NUM 32
#define OAM_NOT_USED 0xffff

// What owns each OAM and affine parameter of a type, by manager ID, and the OAMs, with the affine parameters in their
// fourth halfwords
typedef struct {
    u16 oamUsedBy[OAM_NUM];
    GXOamAttr oamBuffer[OAM_NUM];
    u16 affineUsedBy[AFFINE_NUM];
} OamBuffer;

typedef void (*OamLoadFunction)(const void *src, u32 offset, u32 size);

static void CpuLoadOAMSub_(const void *src, u32 offset, u32 size);
static void CpuLoadOAMMain_(const void *src, u32 offset, u32 size);

static u16 managerID_;
static OamBuffer oamBuffers_[NNS_G2D_OAMTYPE_MAX];

static OamLoadFunction loadFunctions_[NNS_G2D_OAMTYPE_SOFTWAREEMULATION + 1] = { CpuLoadOAMMain_, CpuLoadOAMSub_ };

static void CpuLoadOAMSub_(const void *src, u32 offset, u32 size) {
    MI_CpuCopy16(src, (void *)(HW_DB_OAM + offset), size);
}

static void CpuLoadOAMMain_(const void *src, u32 offset, u32 size) {
    MI_CpuCopy16(src, (void *)(HW_OAM + offset), size);
}

// Whether the num entries of usedBy from from are free
static inline BOOL IsRegionFree_(const u16 *usedBy, u16 from, u16 num) {
    u16 last = from + (num - 1);
    const u16 *p;

    for (p = &usedBy[from]; p <= &usedBy[last]; p++) {
        if (*p != OAM_NOT_USED) {
            return FALSE;
        }
    }
    return TRUE;
}

// Gives the num entries from from to the manager. The fill reads the manager's ID back after the stores to the region
static inline void SetRegion_(NNSG2dOamManagerInstance *man, NNSG2dOamManagedRegion *region, u16 *usedBy, u16 from,
                              u16 num) {
    u16 last = from + (num - 1);

    region->fromIdx = from;
    region->toIdx = last;
    region->currentIdx = from;
    MI_CpuFill16(&usedBy[from], man->managerID, (last - from + 1) * sizeof(u16));
}

static inline BOOL IsRegionValid_(const NNSG2dOamManagedRegion *region) {
    return region->currentIdx <= region->toIdx + 1 && region->fromIdx <= region->toIdx;
}

// How many entries of the region are still free
static inline u16 GetCapacity_(const NNSG2dOamManagedRegion *region) {
    if (IsRegionValid_(region)) {
        return region->toIdx - region->currentIdx + 1;
    }
    return 0;
}

// Whether num more entries fit in the region
static inline BOOL IsCapacityEnough_(const NNSG2dOamManagedRegion *region, u16 num) {
    return (BOOL)(GetCapacity_(region) >= num);
}

// The size in bytes of the region's OAMs
static inline u32 GetOamSize_(const NNSG2dOamManagedRegion *region) {
    if (IsRegionValid_(region)) {
        return (u16)(region->toIdx - region->fromIdx + 1) * sizeof(GXOamAttr);
    }
    return 0;
}

void NNS_G2dInitOamManagerModule(void) {
    MI_CpuFill16(oamBuffers_[NNS_G2D_OAMTYPE_MAIN].oamBuffer, 0xc0, sizeof(oamBuffers_[0].oamBuffer));
    MI_CpuFill16(oamBuffers_[NNS_G2D_OAMTYPE_SUB].oamBuffer, 0xc0, sizeof(oamBuffers_[0].oamBuffer));
    MI_CpuFill16(oamBuffers_[NNS_G2D_OAMTYPE_SOFTWAREEMULATION].oamBuffer, 0xc0, sizeof(oamBuffers_[0].oamBuffer));

    MI_CpuFill16(oamBuffers_[NNS_G2D_OAMTYPE_MAIN].oamUsedBy, OAM_NOT_USED, sizeof(oamBuffers_[0].oamUsedBy));
    MI_CpuFill16(oamBuffers_[NNS_G2D_OAMTYPE_SUB].oamUsedBy, OAM_NOT_USED, sizeof(oamBuffers_[0].oamUsedBy));
    MI_CpuFill16(oamBuffers_[NNS_G2D_OAMTYPE_SOFTWAREEMULATION].oamUsedBy, OAM_NOT_USED,
                 sizeof(oamBuffers_[0].oamUsedBy));

    MI_CpuFill16(oamBuffers_[NNS_G2D_OAMTYPE_MAIN].affineUsedBy, OAM_NOT_USED, sizeof(oamBuffers_[0].affineUsedBy));
    MI_CpuFill16(oamBuffers_[NNS_G2D_OAMTYPE_SUB].affineUsedBy, OAM_NOT_USED, sizeof(oamBuffers_[0].affineUsedBy));
    MI_CpuFill16(oamBuffers_[NNS_G2D_OAMTYPE_SOFTWAREEMULATION].affineUsedBy, OAM_NOT_USED,
                 sizeof(oamBuffers_[0].affineUsedBy));
}

BOOL NNS_G2dGetNewOamManagerInstanceAsFastTransferMode(NNSG2dOamManagerInstance *man, u16 from, u16 num, u32 type) {
    OamBuffer *buffer = &oamBuffers_[type];

    if (IsRegionFree_(buffer->oamUsedBy, from, num)) {
        man->managerID = managerID_++;
        SetRegion_(man, &man->managedAttrRegion, buffer->oamUsedBy, from, num);
    } else {
        return FALSE;
    }
    // Four OAMs hold each affine parameter
    if (IsRegionFree_(buffer->affineUsedBy, from / 4, num / 4)) {
        SetRegion_(man, &man->managedAffineRegion, buffer->affineUsedBy, from / 4, num / 4);
    } else {
        return FALSE;
    }
    man->bFastTransferMode = TRUE;
    man->type = type;
    return TRUE;
}

BOOL NNS_G2dEntryOamManagerOamWithAffineIdx(NNSG2dOamManagerInstance *man, const GXOamAttr *oam, u16 affineIdx) {
    if (IsCapacityEnough_(&man->managedAttrRegion, 1)) {
        GXOamAttr *dst = &oamBuffers_[man->type].oamBuffer[man->managedAttrRegion.currentIdx];

        dst->attr0 = oam->attr0;
        dst->attr1 = oam->attr1;
        dst->attr2 = oam->attr2;
        if (affineIdx != NNS_G2D_OAM_AFFINE_IDX_NONE && (dst->rsMode & 1)) {
            dst->rsParam = affineIdx;
        }
        man->managedAttrRegion.currentIdx++;
        return TRUE;
    }
    return FALSE;
}

u16 NNS_G2dEntryOamManagerAffine(NNSG2dOamManagerInstance *man, const MtxFx22 *mtx) {
    if (IsCapacityEnough_(&man->managedAffineRegion, 1)) {
        GXOamAttr *dst = &oamBuffers_[man->type].oamBuffer[man->managedAffineRegion.currentIdx * 4];
        u16 affineIdx = man->managedAffineRegion.currentIdx;

        dst[0].affineParam = mtx->_00 >> 4;
        dst[1].affineParam = mtx->_01 >> 4;
        dst[2].affineParam = mtx->_10 >> 4;
        dst[3].affineParam = mtx->_11 >> 4;
        man->managedAffineRegion.currentIdx++;
        return affineIdx;
    }
    return NNS_G2D_OAM_AFFINE_IDX_NONE;
}

// Sends the OAMs from from to to of the type to the hardware at once
static inline void LoadOams_(u32 type, u16 from, u16 to) {
    const GXOamAttr *src = &oamBuffers_[type].oamBuffer[from];
    u16 offset = from * sizeof(GXOamAttr);
    u16 size = (to - from + 1) * sizeof(GXOamAttr);

    cp15_flushDC(src, size);
    switch (type) {
    case NNS_G2D_OAMTYPE_MAIN:
        gfxUploadOAMA(src, offset, size);
        break;
    case NNS_G2D_OAMTYPE_SUB:
        gfxUploadOAMB(src, offset, size);
        break;
    }
}

// Sends each OAM from from to to of the type, without its affine parameter, through the type's load function
static inline void LoadOamsByFunction_(u32 type, u16 from, u16 to) {
    u16 i;
    const GXOamAttr *src = &oamBuffers_[type].oamBuffer[from];
    u16 num = to - from + 1;
    u16 offset = from * sizeof(GXOamAttr);
    OamLoadFunction load = loadFunctions_[type];

    for (i = 0; i < num; i++) {
        load(src, offset, 3 * sizeof(u16));
        offset += sizeof(GXOamAttr);
        src++;
    }
}

// Sends the affine parameters from from to to of the type alone, through the type's load function
static inline void LoadAffinesByFunction_(u32 type, u16 from, u16 to) {
    u16 i;
    const GXOamAttr *src = &oamBuffers_[type].oamBuffer[from * 4];
    u16 num = to - from + 1;
    u16 offset = from * 4 * sizeof(GXOamAttr);
    OamLoadFunction load = loadFunctions_[type];

    for (i = 0; i < num; i++) {
        load(&src[0].affineParam, offset + 6, sizeof(u16));
        load(&src[1].affineParam, offset + 14, sizeof(u16));
        load(&src[2].affineParam, offset + 22, sizeof(u16));
        load(&src[3].affineParam, offset + 30, sizeof(u16));
        offset += 4 * sizeof(GXOamAttr);
        src += 4;
    }
}

void NNS_G2dApplyOamManagerToHW(NNSG2dOamManagerInstance *man) {
    if (man->bFastTransferMode) {
        LoadOams_(man->type, man->managedAttrRegion.fromIdx, man->managedAttrRegion.toIdx);
    } else {
        LoadOamsByFunction_(man->type, man->managedAttrRegion.fromIdx, man->managedAttrRegion.toIdx);
        if (IsRegionValid_(&man->managedAffineRegion)) {
            LoadAffinesByFunction_(man->type, man->managedAffineRegion.fromIdx, man->managedAffineRegion.toIdx);
        }
    }
}

void NNS_G2dResetOamManagerBuffer(NNSG2dOamManagerInstance *man) {
    if (man->bFastTransferMode) {
        u32 size = GetOamSize_(&man->managedAttrRegion);
        GXOamAttr *oam = &oamBuffers_[man->type].oamBuffer[man->managedAttrRegion.fromIdx];

        cp15_invalidateDC(oam, size);
        GXi_DmaFill32(GXi_DmaId, oam, 0xc0, size);
    } else {
        u16 i;
        GXOamAttr *oam = &oamBuffers_[man->type].oamBuffer[man->managedAttrRegion.fromIdx];
        u16 num = man->managedAttrRegion.toIdx - man->managedAttrRegion.fromIdx + 1;

        for (i = 0; i < num; i++) {
            oam->attr0 = 0xc0;
            oam++;
        }
    }
    man->managedAttrRegion.currentIdx = man->managedAttrRegion.fromIdx;
    man->managedAffineRegion.currentIdx = man->managedAffineRegion.fromIdx;
}
