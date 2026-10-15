#ifndef POKEBW2_GFL_SAVEDATA_H
#define POKEBW2_GFL_SAVEDATA_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The table of a save's blocks: where each block is in the save, from the sizes the game's block definitions return.
// Each block is followed by a footer and starts 0x100-aligned

// A save block the game defines; the IDs count up from 0 in the table
struct SaveBlockDef {
    u32 gmdataID;
    u32 (*getSize)(void);
    void (*init)(void *block);
};

// footerSize and extraSize count towards the save's size, before and after the blocks' data
SaveDataTable *SaveData_InitTable(HeapID heapId, const SaveBlockDef *table, u32 count, u32 saveareaSize, u32 footerSize,
                                  u32 blockFooterSize, u32 extraSize);
void freeIntermediateSaveDataBlocks(SaveDataTable *svdt);
// Clears the save's data and initializes each block in it
void runSaveBlkInitializers(void *data, SaveDataTable *svdt);
u32 getStartOffsetOfSaveBlockDataInSave(SaveDataTable *svdt, u32 gmdataid);
u32 getSizeOfSaveBlockData(SaveDataTable *svdt, u32 gmdataid);
// The number of blocks
u32 getSaveSize(SaveDataTable *svdt);
u32 GetSaveDataSize(SaveDataTable *svdt);

#endif // POKEBW2_GFL_SAVEDATA_H
