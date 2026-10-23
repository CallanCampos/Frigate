#include "types.h"
#include "field/field_map.h"
#include "field/field_prop.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// Patches of map land data (land_data_patch.c, after the file name that its allocation passes): a file of archive 0x9a
// that starts with the offsets of its cell grid and its buildings. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the layouts are ours

// A map cell
typedef struct LandPatchCell {
    u8 data[0x18];
} LandPatchCell;

// A grid of cells
struct LandPatchGrid {
    u16 width;
    u16 height;
    LandPatchCell cells[1];
};

// The buildings of a patch: how many, then where each is
typedef struct LandPatchBuildings {
    u32 count;
    FieldPropSourceInfo infos[1];
} LandPatchBuildings;

struct LandDataPatch {
    u8 *data;
};

static LandPatchGrid *func_ov036_021c2dcc(LandDataPatch *patch);
static LandPatchBuildings *GetLandDataPatchBMDataPtr(LandDataPatch *patch);

LandDataPatch *ReadLandDataPatchA154Data(u16 fileId, HeapID heapId) {
    LandDataPatch *patch = GFL_HeapAllocate(heapId, sizeof(LandDataPatch), TRUE, "land_data_patch.c", 79);

    patch->data = GFL_ArcSysReadHeapNewLZ(0x9a, fileId, FALSE, heapId);
    return patch;
}

void FreeLandDataPatch(LandDataPatch *patch) {
    GFL_HeapFree(patch->data);
    GFL_HeapFree(patch);
}

void func_ov036_021c2d04(LandDataPatch *patch, LandPatchGrid *src, u32 srcX, u32 srcY, u32 destX, u32 destY, u32 width,
                         u32 height) {
    LandPatchGrid *dest = func_ov036_021c2dcc(patch);
    LandPatchCell *srcCells = src->cells;
    LandPatchCell *destCells = dest->cells;
    u32 i;

    for (i = 0; i < height; i++) {
        MI_CpuCopy8(&destCells[srcX + (srcY + i) * dest->width], &srcCells[destX + (destY + i) * src->width],
                    width * sizeof(LandPatchCell));
    }
}

u32 LoadLandDataPatchBuildings(LandDataPatch *patch, FieldPropSystem *a1, FieldChunk *a2, u32 count, u32 x, u32 y) {
    LandPatchBuildings *buildings = GetLandDataPatchBMDataPtr(patch);
    FieldPropSourceInfo *infos = buildings->infos;
    u32 i;

    for (i = 0; i < buildings->count; i++) {
        FieldPropSourceInfo info;

        MI_CpuCopy8(&infos[i], &info, sizeof(FieldPropSourceInfo));
        info.x += x << 16;
        info.z -= y << 16;
        FieldPropSystem_InstantiateFromInfo(a1, a2, &info, count + i);
    }
    return count + buildings->count;
}

static LandPatchGrid *func_ov036_021c2dcc(LandDataPatch *patch) {
    return (LandPatchGrid *)(patch->data + *(u16 *)patch->data);
}

static LandPatchBuildings *GetLandDataPatchBMDataPtr(LandDataPatch *patch) {
    return (LandPatchBuildings *)(patch->data + *(u16 *)(patch->data + 2));
}
