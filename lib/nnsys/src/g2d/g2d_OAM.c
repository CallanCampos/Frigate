#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"

// NitroSystem's OAM managers. Each type of OAM has a buffer of its OAMs and affine parameters, which the managers share
// out in ranges, and a manager fills its range each frame and sends it to the hardware. The file also defines the OAM
// size tables that g2d.h's inlines read, which fill the gap before g2d_CellTransferManager.c's .rodata: pokediamond
// links the OAM manager as one object, g2d_oam.o, whose .rodata is these two tables. Its linker file cuts object names
// at 15 characters (g2d_celltransfe, g2d_oamsoftware), so g2d_oam is the whole name, and the case is NitroSystem's, as
// ntrtwl's reconstruction of NitroSystem has it. The names are NitroSystem's, as that reconstruction has them, except
// LoadOams_, which is ours

#define NUM_HW_OAM_ATTR 128
#define NUM_HW_OAM_AFFINE 32
#define OAM_NOT_USED 0xffff

// What owns each OAM and affine parameter of a type, by manager ID, and the OAMs, with the affine parameters in their
// fourth halfwords
typedef struct {
    u16 reservationTable[NUM_HW_OAM_ATTR];
    GXOamAttr oamBuffer[NUM_HW_OAM_ATTR];
    u16 affineUsedBy[NUM_HW_OAM_AFFINE];
} OamCache;

typedef void (*OBJLoadFunction)(const void *src, u32 offset, u32 size);

static void CpuLoadOAMSub_(const void *src, u32 offset, u32 size);
static void CpuLoadOAMMain_(const void *src, u32 offset, u32 size);

static u16 numRegisterdInstance_;
static OamCache oamCache_[NNS_G2D_OAMTYPE_MAX];

// The width and height in pixels of each OAM shape and size, by [shape][size]. Objects of one size come out in the
// reverse of their definition order, so the widths are defined first and the ROM has the heights first
const u16 NNSi_objSizeWTbl[3][4] = {
    { 8, 16, 32, 64 },  // Square
    { 16, 32, 32, 64 }, // Wide
    { 8, 8, 16, 32 },   // Tall
};

const u16 NNSi_objSizeHTbl[3][4] = {
    { 8, 16, 32, 64 },  // Square
    { 8, 8, 16, 32 },   // Wide
    { 16, 32, 32, 64 }, // Tall
};

static void CpuLoadOAMSub_(const void *src, u32 offset, u32 size) {
    MI_CpuCopy16(src, (void *)(HW_DB_OAM + offset), size);
}

static void CpuLoadOAMMain_(const void *src, u32 offset, u32 size) {
    MI_CpuCopy16(src, (void *)(HW_OAM + offset), size);
}

static inline OamCache *GetOamCachePtr_(NNSG2dOamType type) {
    return &oamCache_[type];
}

// The function that copies to the type's OAM, which the software-emulated type doesn't have
static inline OBJLoadFunction GetOBJLoadFunction_(NNSG2dOamType type) {
    static OBJLoadFunction funcTbl[NNS_G2D_OAMTYPE_SOFTWAREEMULATION + 1] = { CpuLoadOAMMain_, CpuLoadOAMSub_ };

    return funcTbl[type];
}

// Whether every entry of usedBy from from to last is free
static inline BOOL IsOamNotUsed_(const u16 *usedBy, u16 from, u16 last) {
    const u16 *p = usedBy + from;
    const u16 *end = usedBy + last;

    while (p <= end) {
        if (*p != OAM_NOT_USED) {
            return FALSE;
        }
        p++;
    }
    return TRUE;
}

// Gives the entries from from to last to the manager
static inline void DoReserveArea_(u16 *usedBy, u16 from, u16 last, u16 managerID) {
    MI_CpuFill16(usedBy + from, managerID, (last - from + 1) * sizeof(u16));
}

static inline BOOL IsManageAreaValid_(const NNSG2dOAMManageArea *region) {
    return region->currentIdx <= region->toIdx + 1 && region->fromIdx <= region->toIdx;
}

// How many entries of the region are still free
static inline u16 GetCapacity_(const NNSG2dOAMManageArea *region) {
    if (IsManageAreaValid_(region)) {
        return region->toIdx - region->currentIdx + 1;
    }
    return 0;
}

// Whether num more entries fit in the region
static inline BOOL HasEnoughCapacity_(const NNSG2dOAMManageArea *region, u16 num) {
    return (BOOL)(GetCapacity_(region) >= num);
}

// The size in bytes of the region's OAMs
static inline u32 GetSizeOfManageArea_(const NNSG2dOAMManageArea *region) {
    if (IsManageAreaValid_(region)) {
        return (u16)(region->toIdx - region->fromIdx + 1) * sizeof(GXOamAttr);
    }
    return 0;
}

void NNS_G2dInitOamManagerModule(void) {
    MI_CpuFill16(oamCache_[NNS_G2D_OAMTYPE_MAIN].oamBuffer, 0xc0, sizeof(oamCache_[0].oamBuffer));
    MI_CpuFill16(oamCache_[NNS_G2D_OAMTYPE_SUB].oamBuffer, 0xc0, sizeof(oamCache_[0].oamBuffer));
    MI_CpuFill16(oamCache_[NNS_G2D_OAMTYPE_SOFTWAREEMULATION].oamBuffer, 0xc0, sizeof(oamCache_[0].oamBuffer));

    MI_CpuFill16(oamCache_[NNS_G2D_OAMTYPE_MAIN].reservationTable, OAM_NOT_USED, sizeof(oamCache_[0].reservationTable));
    MI_CpuFill16(oamCache_[NNS_G2D_OAMTYPE_SUB].reservationTable, OAM_NOT_USED, sizeof(oamCache_[0].reservationTable));
    MI_CpuFill16(oamCache_[NNS_G2D_OAMTYPE_SOFTWAREEMULATION].reservationTable, OAM_NOT_USED,
                 sizeof(oamCache_[0].reservationTable));

    MI_CpuFill16(oamCache_[NNS_G2D_OAMTYPE_MAIN].affineUsedBy, OAM_NOT_USED, sizeof(oamCache_[0].affineUsedBy));
    MI_CpuFill16(oamCache_[NNS_G2D_OAMTYPE_SUB].affineUsedBy, OAM_NOT_USED, sizeof(oamCache_[0].affineUsedBy));
    MI_CpuFill16(oamCache_[NNS_G2D_OAMTYPE_SOFTWAREEMULATION].affineUsedBy, OAM_NOT_USED,
                 sizeof(oamCache_[0].affineUsedBy));
}

BOOL NNS_G2dGetNewOamManagerInstanceAsFastTransferMode(NNSG2dOamManagerInstance *man, u16 from, u16 num,
                                                       NNSG2dOamType type) {
    {
        u16 *usedBy = GetOamCachePtr_(type)->reservationTable;
        u16 last = from + (num - 1);

        if (IsOamNotUsed_(usedBy, from, last)) {
            man->GUID = numRegisterdInstance_++;
            man->managedAttrArea.fromIdx = from;
            man->managedAttrArea.toIdx = last;
            man->managedAttrArea.currentIdx = from;
            DoReserveArea_(usedBy, from, last, man->GUID);
        } else {
            return FALSE;
        }
    }

    {
        // Four OAMs hold each affine parameter
        u16 affineFrom = from / 4;
        u16 affineNum = num / 4;
        u16 affineLast = affineFrom + (affineNum - 1);
        u16 *usedBy = GetOamCachePtr_(type)->affineUsedBy;

        if (IsOamNotUsed_(usedBy, affineFrom, affineLast)) {
            man->managedAffineArea.fromIdx = affineFrom;
            man->managedAffineArea.toIdx = affineLast;
            man->managedAffineArea.currentIdx = affineFrom;
            DoReserveArea_(usedBy, affineFrom, affineLast, man->GUID);
        } else {
            return FALSE;
        }
    }

    man->bFastTransferEnable = TRUE;
    man->type = type;
    return TRUE;
}

BOOL NNS_G2dEntryOamManagerOamWithAffineIdx(NNSG2dOamManagerInstance *man, const GXOamAttr *oam, u16 affineIdx) {
    if (HasEnoughCapacity_(&man->managedAttrArea, 1)) {
        GXOamAttr *dst = &GetOamCachePtr_(man->type)->oamBuffer[man->managedAttrArea.currentIdx];

        dst->attr0 = oam->attr0;
        dst->attr1 = oam->attr1;
        dst->attr2 = oam->attr2;
        if (affineIdx != NNS_G2D_OAM_AFFINE_IDX_NONE && (dst->rsMode & 1)) {
            dst->rsParam = affineIdx;
        }
        man->managedAttrArea.currentIdx++;
        return TRUE;
    }
    return FALSE;
}

u16 NNS_G2dEntryOamManagerAffine(NNSG2dOamManagerInstance *man, const MtxFx22 *mtx) {
    if (HasEnoughCapacity_(&man->managedAffineArea, 1)) {
        GXOamAttr *dst = &GetOamCachePtr_(man->type)->oamBuffer[man->managedAffineArea.currentIdx * 4];
        u16 affineIdx = man->managedAffineArea.currentIdx;

        dst[0]._3 = mtx->_00 >> 4;
        dst[1]._3 = mtx->_01 >> 4;
        dst[2]._3 = mtx->_10 >> 4;
        dst[3]._3 = mtx->_11 >> 4;
        man->managedAffineArea.currentIdx++;
        return affineIdx;
    }
    return NNS_G2D_OAM_AFFINE_IDX_NONE;
}

// Sends the OAMs from from to to of the type to the hardware at once
static inline void LoadOams_(u32 type, u16 from, u16 to) {
    const GXOamAttr *src = &oamCache_[type].oamBuffer[from];
    u16 offset = from * sizeof(GXOamAttr);
    u16 size = (to - from + 1) * sizeof(GXOamAttr);

    DC_FlushRange(src, size);
    switch (type) {
    case NNS_G2D_OAMTYPE_MAIN:
        GX_LoadOAM(src, offset, size);
        break;
    case NNS_G2D_OAMTYPE_SUB:
        GXS_LoadOAM(src, offset, size);
        break;
    }
}

// Sends each OAM from from to to of the type, without its affine parameter, through the type's load function
static inline void LoadOam_(u32 type, u16 from, u16 to) {
    u16 i;
    const GXOamAttr *src = &oamCache_[type].oamBuffer[from];
    u16 num = to - from + 1;
    u16 offset = from * sizeof(GXOamAttr);
    OBJLoadFunction load = GetOBJLoadFunction_(type);

    for (i = 0; i < num; i++) {
        load(src, offset, 3 * sizeof(u16));
        offset += sizeof(GXOamAttr);
        src++;
    }
}

// Sends the affine parameters from from to to of the type alone, through the type's load function
static inline void LoadAffine_(u32 type, u16 from, u16 to) {
    u16 i;
    const GXOamAffine *src = (const GXOamAffine *)oamCache_[type].oamBuffer + from;
    u16 num = to - from + 1;
    u16 offset = from * sizeof(GXOamAffine);
    OBJLoadFunction load = GetOBJLoadFunction_(type);

    for (i = 0; i < num; i++) {
        load(&src->PA, offset + 6, sizeof(u16));
        load(&src->PB, offset + 14, sizeof(u16));
        load(&src->PC, offset + 22, sizeof(u16));
        load(&src->PD, offset + 30, sizeof(u16));
        offset += sizeof(GXOamAffine);
        src++;
    }
}

void NNS_G2dApplyOamManagerToHW(NNSG2dOamManagerInstance *man) {
    if (man->bFastTransferEnable) {
        LoadOams_(man->type, man->managedAttrArea.fromIdx, man->managedAttrArea.toIdx);
    } else {
        LoadOam_(man->type, man->managedAttrArea.fromIdx, man->managedAttrArea.toIdx);
        if (IsManageAreaValid_(&man->managedAffineArea)) {
            LoadAffine_(man->type, man->managedAffineArea.fromIdx, man->managedAffineArea.toIdx);
        }
    }
}

void NNS_G2dResetOamManagerBuffer(NNSG2dOamManagerInstance *man) {
    if (man->bFastTransferEnable) {
        u32 size = GetSizeOfManageArea_(&man->managedAttrArea);
        GXOamAttr *oam = &GetOamCachePtr_(man->type)->oamBuffer[man->managedAttrArea.fromIdx];

        DC_InvalidateRange(oam, size);
        GXi_DmaFill32(GXi_DmaId, oam, 0xc0, size);
    } else {
        u16 i;
        GXOamAttr *oam = &GetOamCachePtr_(man->type)->oamBuffer[man->managedAttrArea.fromIdx];
        u16 num = man->managedAttrArea.toIdx - man->managedAttrArea.fromIdx + 1;

        for (i = 0; i < num; i++) {
            oam->attr0 = 0xc0;
            oam++;
        }
    }
    man->managedAttrArea.currentIdx = man->managedAttrArea.fromIdx;
    man->managedAffineArea.currentIdx = man->managedAffineArea.fromIdx;
}
