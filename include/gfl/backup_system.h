#ifndef POKEBW2_GFL_BACKUP_SYSTEM_H
#define POKEBW2_GFL_BACKUP_SYSTEM_H

#include "types.h"
#include "gfl/backup_card.h"
#include "gfl/savedata.h"
#include "struct_decls.h"

// Saves in the card's flash: a save's blocks, each followed by a count and a CRC, then a CRC of each block and a
// footer. A save with a backup keeps two copies, and loading takes each block from a copy whose CRC is right. Saving
// writes only the blocks that changed, a step at a time, then the CRCs and the footer, the footer's last byte last

// Where a save is and what it holds
typedef struct {
    const SaveBlockDef *table;
    u32 count;
    u32 mainOffset;
    // The same as mainOffset for a save without a backup
    u32 backupOffset;
    u32 size;
    // Kept in the footer, which a load checks
    u32 magic;
} SaveDataParams;

enum {
    SAVE_LOAD_RESULT_NONE,
    SAVE_LOAD_RESULT_OK,
    // Loaded, with a block that only one copy had right
    SAVE_LOAD_RESULT_OK_PARTIAL,
    SAVE_LOAD_RESULT_BROKEN,
    SAVE_LOAD_RESULT_ERROR,
};

enum {
    SAVE_RESULT_CONTINUE,
    // Writing the footer, the last step
    SAVE_RESULT_LAST,
    SAVE_RESULT_OK,
    SAVE_RESULT_NG,
};

void mainBackupSystemInit(u32 heapId, BackupReadErrorCallback readErrorCallback,
                          BackupWriteErrorCallback writeErrorCallback);
SaveData *SaveData_InitMain(const SaveDataParams *params, u32 heapId);
// A save in the caller's memory, of svwk_size bytes. init tells whether to initialize the blocks
SaveData *SaveData_InitWithBuffer(const SaveDataParams *params, u32 heapId, void *buffer, u32 svwk_size, BOOL init);
void freeSaveBlocks(SaveData *sv);
// The save's data and its size
void *SaveData_GetData(SaveData *sv, u32 *size);
void *SaveData_GetBlockPtr(SaveData *sv, u32 gmdataid);
// Initializes the blocks and marks every one to be written
void initSaveBlkCallbacks(SaveData *sv);
// Compares the save with the card: in *changed the size of the blocks that differ, and in *total of all of them
void SaveData_GetChangedSize(SaveData *sv, u32 *changed, u32 *total);
// Erases the save in the card
BOOL SaveData_Erase(SaveData *sv, u32 heapId);
// Loads, returning a SAVE_LOAD_RESULT_*
u32 SaveData_Load(SaveData *sv, u32 heapId);
// Saves a step at a time: SaveData_SaveStart starts, SaveData_SaveStep returns a SAVE_RESULT_*, and SaveData_SaveCancel
// cancels
void SaveData_SaveStart(SaveData *sv);
u32 SaveData_SaveStep(SaveData *sv);
void SaveData_SaveCancel(SaveData *sv);
// Saves two saves together: SaveData_DualSaveStart starts, keeping the second's count in *count, and
// SaveData_DualSaveStep continues
void SaveData_DualSaveStart(SaveData *sv, SaveData *sub, u32 *count);
u32 SaveData_DualSaveStep(SaveData *sv, SaveData *sub);
// Loads only a save whose count is count
u32 SaveData_LoadWithCount(SaveData *sv, u32 heapId, u32 count);
// The bytes the save in progress has written
u32 SaveData_GetBytesWritten(SaveData *sv);
BOOL getLockIDStatus_inline_stub(void);
// Access to the card with the system's callbacks
BOOL BackupSystem_WriteWait(u32 dest, const void *src, u32 size);
u16 BackupSystem_WriteStart(u32 dest, const void *src, u32 size);
BOOL BackupSystem_FinishWrite(u16 lockId, BOOL *result);
BOOL BackupSystem_Read(u32 src, void *dest, u32 size);
// Reads a block from the main copy (0) or the backup (1)
BOOL SaveData_ReadBlockFromCard(SaveData *sv, u32 gmdataid, u32 copy, void *dest, u32 size);

#endif // POKEBW2_GFL_BACKUP_SYSTEM_H
