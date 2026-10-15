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
SaveData *func_0203ad44(const SaveDataParams *params, u32 heapId, void *buffer, u32 svwk_size, BOOL init);
void freeSaveBlocks(SaveData *sv);
// The save's data and its size
void *func_0203ad7c(SaveData *sv, u32 *size);
void *SaveData_GetBlockPtr(SaveData *sv, u32 gmdataid);
// Initializes the blocks and marks every one to be written
void initSaveBlkCallbacks(SaveData *sv);
// Compares the save with the card: in *changed the size of the blocks that differ, and in *total of all of them
void func_0203adac(SaveData *sv, u32 *changed, u32 *total);
// Erases the save in the card
BOOL func_0203adb4(SaveData *sv, u32 heapId);
// Loads, returning a SAVE_LOAD_RESULT_*
u32 func_0203ae4c(SaveData *sv, u32 heapId);
// Saves a step at a time: func_0203aeb4 starts, func_0203aecc returns a SAVE_RESULT_*, and func_0203af00 cancels
void func_0203aeb4(SaveData *sv);
u32 func_0203aecc(SaveData *sv);
void func_0203af00(SaveData *sv);
// Saves two saves together: func_0203af08 starts, keeping the second's count in *count, and func_0203af18 continues
void func_0203af08(SaveData *sv, SaveData *sub, u32 *count);
u32 func_0203af18(SaveData *sv, SaveData *sub);
// Loads only a save whose count is count
u32 func_0203b018(SaveData *sv, u32 heapId, u32 count);
// The bytes the save in progress has written
u32 func_0203b080(SaveData *sv);
BOOL getLockIDStatus_inline_stub(void);
// Access to the card with the system's callbacks
BOOL func_0203bee8(u32 dest, const void *src, u32 size);
u16 func_0203bf00(u32 dest, const void *src, u32 size);
BOOL func_0203bf18(u16 lockId, BOOL *result);
BOOL func_0203bf30(u32 src, void *dest, u32 size);
// Reads a block from the main copy (0) or the backup (1)
BOOL func_0203bf48(SaveData *sv, u32 gmdataid, u32 copy, void *dest, u32 size);

#endif // POKEBW2_GFL_BACKUP_SYSTEM_H
