#include "types.h"
#include "gfl/heap.h"
#include "gfl/savedata.h"
#include "gfl/std.h"

typedef struct {
    u32 gmdataID;
    u32 size;
    // From the start of the save's data
    u32 offset;
} SaveBlockInfo;

struct SaveDataTable {
    const SaveBlockDef *table;
    u32 table_max;
    u32 footerSize;
    // Every block, with its footer and alignment, and the save's footer and extra data
    u32 total_size;
    u32 savearea_size;
    SaveBlockInfo *blocks;
    u32 extraSize;
};

static u32 getSizeSaveTblBlk(SaveDataTable *svdt, u32 id);
static void SaveData_BuildTable(SaveDataTable *svdt, u32 blockFooterSize);

SaveDataTable *SaveData_InitTable(HeapID heapId, const SaveBlockDef *table, u32 count, u32 saveareaSize, u32 footerSize,
                                  u32 blockFooterSize, u32 extraSize) {
    SaveDataTable *svdt = GFL_HeapAllocate(heapId, sizeof(SaveDataTable), FALSE, "savedata.c", 73);

    MIi_CpuClear32(0, svdt, sizeof(SaveDataTable));
    svdt->table = table;
    svdt->table_max = count;
    svdt->savearea_size = saveareaSize;
    svdt->footerSize = footerSize;
    svdt->blocks = GFL_HeapAllocate(heapId, count * sizeof(SaveBlockInfo), TRUE, "savedata.c", 79);
    svdt->extraSize = extraSize;
    SaveData_BuildTable(svdt, blockFooterSize);
    return svdt;
}

void freeIntermediateSaveDataBlocks(SaveDataTable *svdt) {
    GFL_HeapFree(svdt->blocks);
    GFL_HeapFree(svdt);
}

void runSaveBlkInitializers(void *data, SaveDataTable *svdt) {
    const SaveBlockDef *table = svdt->table;
    u32 count = svdt->table_max;
    u32 i;

    MIi_CpuClear32(0, data, svdt->savearea_size);
    for (i = 0; i < count; i++) {
        u32 offset = svdt->blocks[i].offset;

        MIi_CpuClear32(0, (u8 *)data + offset, svdt->blocks[i].size);
        table[i].init((u8 *)data + offset);
    }
}

static u32 getSizeSaveTblBlk(SaveDataTable *svdt, u32 id) {
    const SaveBlockDef *table = svdt->table;

    GFL_ASSERT(id < svdt->table_max);
    return (table[id].getSize() + 3) & ~3;
}

static void SaveData_BuildTable(SaveDataTable *svdt, u32 blockFooterSize) {
    const SaveBlockDef *table = svdt->table;
    SaveBlockInfo *blocks = svdt->blocks;
    u32 i;
    u32 offset = 0;
    u32 count = svdt->table_max;

    for (i = 0; i < count; i++) {
        SaveBlockInfo *block;
        u32 size;

        GFL_ASSERT(table[i].gmdataID == i);
        block = &blocks[i];
        blocks[i].gmdataID = table[i].gmdataID;
        block->size = getSizeSaveTblBlk(svdt, i);
        block->offset = offset;
        size = blockFooterSize + block->size;
        offset += size + ((u8)size != 0 ? 0x100 - (u8)size : 0);
    }
    svdt->total_size = svdt->footerSize + (offset + svdt->extraSize);
    // clang-format off
    GFL_ASSERT_MSG(svdt->total_size <= svdt->savearea_size, "SaveDataSize=0x%08xbytes, SaveAreaSize=0x%08xbytes",
        svdt->total_size, svdt->savearea_size);
    // clang-format on
}

u32 getStartOffsetOfSaveBlockDataInSave(SaveDataTable *svdt, u32 gmdataid) {
    GFL_ASSERT(gmdataid < svdt->table_max);
    return svdt->blocks[gmdataid].offset;
}

u32 getSizeOfSaveBlockData(SaveDataTable *svdt, u32 gmdataid) {
    GFL_ASSERT(gmdataid < svdt->table_max);
    return svdt->blocks[gmdataid].size;
}

u32 getSaveSize(SaveDataTable *svdt) {
    return svdt->table_max;
}

u32 GetSaveDataSize(SaveDataTable *svdt) {
    return svdt->total_size;
}
