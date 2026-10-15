#include "types.h"
#include "gfl/backup_card.h"
#include "gfl/backup_system.h"
#include "gfl/heap.h"
#include "gfl/savedata.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "nitro/math.h"

// The copies of a save, as func_0203b0b4 takes them
#define SAVE_COPY_MAIN 1
#define SAVE_COPY_BACKUP 2

// What each block's status in SaveData's map says: whether a copy of the block has to be written. The map also has
// one entry for the CRCs and one for the footer, after the blocks
enum {
    BLOCK_STATUS_OK,
    BLOCK_STATUS_WRITE_MAIN,
    BLOCK_STATUS_WRITE_BACKUP,
};

#define SAVE_FOOTER_SIZE 0x10
// How much of the card func_0203b81c compares in a frame
#define COMPARE_SIZE_PER_FRAME 0x800
#define COMPARE_CHUNK_SIZE 0x100

typedef struct {
    BOOL cardOk;
    MATHCRC16Table crcTable;
    BackupReadErrorCallback readErrorCallback;
    BackupWriteErrorCallback writeErrorCallback;
    // The write in progress
    u16 lockId;
} BackupSystem;

// What follows each block
typedef struct {
    // How many times the block has been saved
    u16 count;
    u16 crc;
} BlockFooter;

// What ends a save
typedef struct {
    u32 count;
    u32 size;
    u32 magic;
    u16 unkC;
    // Of the blocks' CRCs
    u16 crc;
} SaveFooter;

typedef struct {
    // 0 if the footer is right, 1 if only its CRC is wrong, 2 otherwise
    u32 result;
    u32 count;
} FooterCheck;

struct SaveData {
    u32 mainOffset;
    u32 backupOffset;
    u32 savearea_size;
    u32 magic;
    // The count before the save in progress, to go back to if it fails
    u32 prevCount;
    u32 blockIndex;
    u8 state;
    // 1 while writing the backup
    u8 pass;
    u8 unk1A[2];
    u8 dataExists;
    u8 ownsData;
    u8 dualState;
    u8 dualResult;
    u32 *dualCount;
    u32 unk24;
    // How many times the save has been saved
    u32 count;
    SaveDataTable *table;
    u32 saveSize;
    u8 *data;
    u8 compareBuffer[COMPARE_CHUNK_SIZE];
    u32 *blockStatus;
    u32 blockStatusSize;
    u32 crcsSize;
    u32 written;
    u32 compareRemaining;
    u32 compareSize;
    u32 comparePos;
};

static SaveData *SaveData_Init(const SaveDataParams *params, u32 heapId, void *buffer, u32 svwk_size, BOOL init);
static u16 func_0203b090(SaveData *sv, u8 *data);
static u32 func_0203b0b4(SaveData *sv, u32 copy);
static u16 *func_0203b0c8(SaveData *sv);
static SaveFooter *func_0203b0dc(SaveData *sv, u8 *data);
static u32 func_0203b0e4(SaveData *sv, u8 *data, u32 copy);
static void func_0203b120(SaveData *sv, u8 *data);
static void func_0203b144(SaveData *sv, u32 count);
static void func_0203b148(FooterCheck *check, SaveData *sv, u8 *data, u32 copy);
static u32 func_0203b170(SaveData *sv, u32 heapId, BOOL checkCount, u32 count);
static u32 func_0203b598(SaveData *sv, BOOL checkCount, u32 count);
static u16 func_0203b714(SaveData *sv, u32 copy);
static u16 func_0203b748(SaveData *sv, u32 copy);
static void func_0203b774(SaveData *sv, BOOL blockSleep);
static void func_0203b798(SaveData *sv, u32 *changed, u32 *total);
static u32 func_0203b81c(SaveData *sv, u32 gmdataid, u32 copy, u32 *remaining, u32 *frameSize, u32 *pos,
    BOOL *next_frame);
static u16 func_0203b92c(SaveData *sv, u32 gmdataid, u32 copy, u32 pass);
static u16 func_0203b988(SaveData *sv, u32 copy);
static u32 func_0203b9bc(SaveData *sv, BOOL *passEnded);
static void func_0203bcd0(SaveData *sv, u32 result, BOOL unblockSleep);
static void func_0203bcec(SaveData *sv);
static void func_0203bd0c(SaveData *sv, u32 copy);
static u16 func_0203bd48(SaveData *sv, u32 gmdataid);
static u16 getCrcOfSaveBlockStoreAtEnd(SaveData *sv, u32 gmdataid);
static u32 func_0203bda0(SaveData *sv, u32 index);
static void func_0203bdbc(SaveData *sv, u32 index, u32 status);
static void func_0203bdf0(SaveData *sv);
static void func_0203be74(SaveData *sv);
static void func_0203be8c(SaveData *sv, u32 status);

static BackupSystem *svsys;

void mainBackupSystemInit(u32 heapId, BackupReadErrorCallback readErrorCallback,
    BackupWriteErrorCallback writeErrorCallback) {
    svsys = GFL_HeapAllocate(heapId, sizeof(BackupSystem), FALSE, "backup_system.c", 223);
    sys_memset32(0, svsys, sizeof(BackupSystem));
    MATHi_CRC16InitTable(&svsys->crcTable, 0x1021);
    svsys->readErrorCallback = readErrorCallback;
    svsys->writeErrorCallback = writeErrorCallback;
    svsys->cardOk = identifyGameCardBackup();
}

static SaveData *SaveData_Init(const SaveDataParams *params, u32 heapId, void *buffer, u32 svwk_size, BOOL init) {
    SaveData *sv;
    u32 entries;

    GFL_ASSERT(svsys != NULL);
    sv = GFL_HeapAllocate(heapId, sizeof(SaveData), TRUE, "backup_system.c", 270);
    sv->mainOffset = params->mainOffset;
    sv->backupOffset = params->backupOffset;
    sv->savearea_size = params->size;
    sv->magic = params->magic;
    if (buffer != NULL) {
        GFL_ASSERT(svwk_size >= sv->savearea_size);
    }
    sv->crcsSize = params->count * sizeof(u16);
    if (params->count & 1) {
        sv->crcsSize += sizeof(u16);
    }
    entries = params->count + 2;
    sv->blockStatusSize = entries / 16 * sizeof(u32);
    if (entries % 16) {
        sv->blockStatusSize += sizeof(u32);
    }
    sv->blockStatus = GFL_HeapAllocate(heapId, sv->blockStatusSize, TRUE, "backup_system.c", 293);
    sv->table = SaveData_InitTable(heapId, params->table, params->count, sv->savearea_size, SAVE_FOOTER_SIZE,
        sizeof(BlockFooter), sv->crcsSize);
    sv->saveSize = GetSaveDataSize(sv->table);
    if (buffer == NULL) {
        sv->data = GFL_HeapAllocate(heapId, sv->savearea_size, TRUE, "backup_system.c", 305);
        sv->ownsData = TRUE;
    } else {
        sv->data = buffer;
        sv->ownsData = FALSE;
    }
    sv->dataExists = FALSE;
    sv->count = 0;
    sv->unk24 = 0;
    if (init == TRUE) {
        initSaveBlkCallbacks(sv);
    } else {
        func_0203be8c(sv, BLOCK_STATUS_WRITE_MAIN);
    }
    return sv;
}


SaveData *SaveData_InitMain(const SaveDataParams *params, u32 heapId) {
    return SaveData_Init(params, heapId, NULL, 0, TRUE);
}

SaveData *func_0203ad44(const SaveDataParams *params, u32 heapId, void *buffer, u32 svwk_size, BOOL init) {
    return SaveData_Init(params, heapId, buffer, svwk_size, init);
}

void freeSaveBlocks(SaveData *sv) {
    freeIntermediateSaveDataBlocks(sv->table);
    GFL_HeapFree(sv->blockStatus);
    if (sv->ownsData == TRUE) {
        GFL_HeapFree(sv->data);
    }
    GFL_HeapFree(sv);
}

void *func_0203ad7c(SaveData *sv, u32 *size) {
    *size = sv->saveSize;
    return sv->data;
}

void *SaveData_GetBlockPtr(SaveData *sv, u32 gmdataid) {
    return sv->data + getStartOffsetOfSaveBlockDataInSave(sv->table, gmdataid);
}

void initSaveBlkCallbacks(SaveData *sv) {
    runSaveBlkInitializers(sv->data, sv->table);
    func_0203be8c(sv, BLOCK_STATUS_WRITE_MAIN);
}

void func_0203adac(SaveData *sv, u32 *changed, u32 *total) {
    func_0203b798(sv, changed, total);
}

BOOL func_0203adb4(SaveData *sv, u32 heapId) {
    u8 *buffer = GFL_HeapAllocate(heapId, 0x1000, FALSE, "backup_system.c", 488);
    u32 i;

    GCTX_HIDBlockSleep(1);
    func_0203bd0c(sv, SAVE_COPY_MAIN);
    if (sv->mainOffset != sv->backupOffset) {
        func_0203bd0c(sv, SAVE_COPY_BACKUP);
    }
    sys_memset32(0xffffffff, buffer, 0x1000);
    for (i = 0; i < sv->savearea_size; i += 0x1000) {
        func_0203bfd4(i + sv->mainOffset, buffer, 0x1000, svsys->writeErrorCallback);
    }
    GFL_HeapFree(buffer);
    initSaveBlkCallbacks(sv);
    sv->dataExists = FALSE;
    GCTX_HIDUnblockSleep(1);
    return TRUE;
}

u32 func_0203ae4c(SaveData *sv, u32 heapId) {
    u32 result;

    if (!svsys->cardOk) {
        return SAVE_LOAD_RESULT_ERROR;
    }
    if (sv->mainOffset != sv->backupOffset) {
        result = func_0203b170(sv, heapId, FALSE, 0);
    } else {
        result = func_0203b598(sv, FALSE, 0);
    }
    switch (result) {
    case SAVE_LOAD_RESULT_OK:
        sv->dataExists = TRUE;
        return SAVE_LOAD_RESULT_OK;
    case SAVE_LOAD_RESULT_OK_PARTIAL:
        sv->dataExists = TRUE;
        return SAVE_LOAD_RESULT_OK_PARTIAL;
    case SAVE_LOAD_RESULT_BROKEN:
        return SAVE_LOAD_RESULT_BROKEN;
    case SAVE_LOAD_RESULT_NONE:
        return SAVE_LOAD_RESULT_NONE;
    case SAVE_LOAD_RESULT_ERROR:
        return SAVE_LOAD_RESULT_ERROR;
    }
    return SAVE_LOAD_RESULT_ERROR;
}

void func_0203aeb4(SaveData *sv) {
    if (svsys->cardOk) {
        func_0203b774(sv, TRUE);
    }
}

u32 func_0203aecc(SaveData *sv) {
    BOOL passEnded;
    u32 result;

    if (!svsys->cardOk) {
        return SAVE_RESULT_NG;
    }
    result = func_0203b9bc(sv, &passEnded);
    if (result != SAVE_RESULT_CONTINUE && result != SAVE_RESULT_LAST) {
        func_0203bcd0(sv, result, TRUE);
    }
    return result;
}

void func_0203af00(SaveData *sv) {
    func_0203bcec(sv);
}

void func_0203af08(SaveData *sv, SaveData *sub, u32 *count) {
    sv->dualCount = count;
    sv->dualState = 0;
    sv->dualResult = SAVE_RESULT_OK;
    sub->dualState = 0;
    sub->dualResult = SAVE_RESULT_OK;
}

u32 func_0203af18(SaveData *sv, SaveData *sub) {
    BOOL passEnded;
    u32 result;

    switch (sv->dualState) {
    case 0:
        func_0203b774(sub, TRUE);
        sv->dualState++;
        break;
    case 1:
        result = func_0203b9bc(sub, &passEnded);
        if (result != SAVE_RESULT_CONTINUE && result != SAVE_RESULT_LAST) {
            func_0203bcd0(sub, result, TRUE);
            if (result == SAVE_RESULT_NG) {
                return SAVE_RESULT_NG;
            }
            sv->dualState++;
        } else if (passEnded == TRUE) {
            sv->dualState++;
        }
        break;
    case 2:
        *sv->dualCount = sub->count;
        func_0203b774(sv, FALSE);
        sv->dualState++;
        break;
    case 3:
        result = func_0203b9bc(sv, &passEnded);
        if (result != SAVE_RESULT_CONTINUE && result != SAVE_RESULT_LAST) {
            func_0203bcd0(sv, result, TRUE);
            if (sub->mainOffset != sub->backupOffset) {
                func_0203bcd0(sub, result, TRUE);
            }
            return SAVE_RESULT_NG;
        } else if (passEnded == TRUE) {
            sv->dualState++;
        }
        break;
    case 4:
        if (sub->mainOffset != sub->backupOffset) {
            result = func_0203b9bc(sub, &passEnded);
            if (result != SAVE_RESULT_CONTINUE && result != SAVE_RESULT_LAST) {
                func_0203bcd0(sub, result, FALSE);
                sub->dualResult = result;
                sv->dualState++;
            }
        } else {
            sv->dualState++;
        }
        break;
    case 5:
        result = func_0203b9bc(sv, &passEnded);
        if (result != SAVE_RESULT_CONTINUE && result != SAVE_RESULT_LAST) {
            func_0203bcd0(sv, result, TRUE);
            if (sub->dualResult == SAVE_RESULT_NG) {
                result = SAVE_RESULT_NG;
            }
            return result;
        }
        break;
    }
    return SAVE_RESULT_CONTINUE;
}

u32 func_0203b018(SaveData *sv, u32 heapId, u32 count) {
    u32 result;

    if (!svsys->cardOk) {
        return SAVE_LOAD_RESULT_ERROR;
    }
    if (sv->mainOffset != sv->backupOffset) {
        result = func_0203b170(sv, heapId, TRUE, count);
    } else {
        result = func_0203b598(sv, TRUE, count);
    }
    switch (result) {
    case SAVE_LOAD_RESULT_OK:
        sv->dataExists = TRUE;
        return SAVE_LOAD_RESULT_OK;
    case SAVE_LOAD_RESULT_OK_PARTIAL:
        sv->dataExists = TRUE;
        return SAVE_LOAD_RESULT_OK_PARTIAL;
    case SAVE_LOAD_RESULT_BROKEN:
        return SAVE_LOAD_RESULT_BROKEN;
    case SAVE_LOAD_RESULT_NONE:
        return SAVE_LOAD_RESULT_NONE;
    case SAVE_LOAD_RESULT_ERROR:
        return SAVE_LOAD_RESULT_ERROR;
    }
    return SAVE_LOAD_RESULT_ERROR;
}

u32 func_0203b080(SaveData *sv) {
    return sv->written;
}

BOOL getLockIDStatus_inline_stub(void) {
    return getLockIDStatus();
}

// The CRC of the blocks' CRCs
static u16 func_0203b090(SaveData *sv, u8 *data) {
    return MATH_CalcCRC16CCITT(&svsys->crcTable, data + (sv->saveSize - SAVE_FOOTER_SIZE - sv->crcsSize), sv->crcsSize);
}

// Where a copy of the save starts in the card
static u32 func_0203b0b4(SaveData *sv, u32 copy) {
    if (sv->mainOffset == sv->backupOffset) {
        return sv->mainOffset;
    }
    return copy == SAVE_COPY_BACKUP ? sv->backupOffset : sv->mainOffset;
}

// The blocks' CRCs
static u16 *func_0203b0c8(SaveData *sv) {
    return (u16 *)(sv->data + (sv->saveSize - SAVE_FOOTER_SIZE - sv->crcsSize));
}

static SaveFooter *func_0203b0dc(SaveData *sv, u8 *data) {
    return (SaveFooter *)(data + (sv->saveSize - SAVE_FOOTER_SIZE));
}

static u32 func_0203b0e4(SaveData *sv, u8 *data, u32 copy) {
    SaveFooter *footer = func_0203b0dc(sv, data);

    if (footer->size != sv->saveSize) {
        return 2;
    }
    if (footer->magic != sv->magic) {
        return 2;
    }
    if (footer->crc != func_0203b090(sv, data)) {
        return 1;
    }
    return 0;
}

static void func_0203b120(SaveData *sv, u8 *data) {
    SaveFooter *footer = func_0203b0dc(sv, data);

    footer->count = sv->count;
    footer->size = sv->saveSize;
    footer->magic = sv->magic;
    footer->crc = func_0203b090(sv, data);
}

static void func_0203b144(SaveData *sv, u32 count) {
    sv->count = count;
}

static void func_0203b148(FooterCheck *check, SaveData *sv, u8 *data, u32 copy) {
    SaveFooter *footer = func_0203b0dc(sv, data);

    check->result = func_0203b0e4(sv, data, copy);
    check->count = footer->count;
}

// Loads a save with a backup, taking each block from a copy that has it right
static u32 func_0203b170(SaveData *sv, u32 heapId, BOOL checkCount, u32 count) {
    u8 *mainData;
    u8 *backupData;
    u16 *crcs;
    u32 offset;
    u32 size;
    u32 end;
    u32 copyIndex;
    u8 *data;
    u32 i;
    FooterCheck backupCheck;
    BlockFooter *dataFooter;
    FooterCheck mainCheck;
    u8 *datas[2];
    u32 counts[2];
    u32 copies[2];
    BOOL mainRead;
    BOOL backupRead;
    BlockFooter *mainFooter;
    BlockFooter *backupFooter;

    func_0203be74(sv);
    mainData = sv->data;
    backupData = GFL_HeapAllocate(heapId, sv->saveSize, FALSE, "backup_system.c", 1065);
    mainRead = readFlashWaitSuccess(func_0203b0b4(sv, SAVE_COPY_MAIN), mainData, sv->saveSize,
        svsys->readErrorCallback);
    backupRead = readFlashWaitSuccess(func_0203b0b4(sv, SAVE_COPY_BACKUP), backupData, sv->saveSize,
        svsys->readErrorCallback);
    if (mainRead == FALSE && backupRead == FALSE) {
        func_0203be8c(sv, BLOCK_STATUS_WRITE_MAIN);
        sys_memset32(0, mainData, sv->saveSize);
        GFL_HeapFree(backupData);
        return SAVE_LOAD_RESULT_NONE;
    }
    if (mainRead == TRUE && backupRead == FALSE) {
        func_0203be8c(sv, BLOCK_STATUS_WRITE_BACKUP);
        sys_memcpy16(mainData, backupData, sv->saveSize);
    } else if (mainRead == FALSE && backupRead == TRUE) {
        func_0203be8c(sv, BLOCK_STATUS_WRITE_MAIN);
        sys_memcpy16(backupData, mainData, sv->saveSize);
    }
    func_0203b148(&mainCheck, sv, mainData, 0);
    func_0203b148(&backupCheck, sv, backupData, 1);
    if (checkCount == TRUE) {
        if (mainCheck.count != count) {
            mainCheck.result = 2;
        }
        if (backupCheck.count != count) {
            backupCheck.result = 2;
        }
    }
    if (mainCheck.result == 0) {
        if (backupCheck.result != 0) {
            func_0203bdbc(sv, getSaveSize(sv->table), BLOCK_STATUS_WRITE_BACKUP);
        func_0203bdbc(sv, getSaveSize(sv->table) + 1, BLOCK_STATUS_WRITE_BACKUP);
        offset = sv->saveSize - SAVE_FOOTER_SIZE - sv->crcsSize;
        sys_memcpy16(mainData + offset, backupData + offset, sv->crcsSize + SAVE_FOOTER_SIZE);
        }
    } else if (backupCheck.result == 0) {
        if (mainCheck.result != 0) {
            func_0203bdbc(sv, getSaveSize(sv->table), BLOCK_STATUS_WRITE_MAIN);
        func_0203bdbc(sv, getSaveSize(sv->table) + 1, BLOCK_STATUS_WRITE_MAIN);
        offset = sv->saveSize - SAVE_FOOTER_SIZE - sv->crcsSize;
        sys_memcpy16(backupData + offset, mainData + offset, sv->crcsSize + SAVE_FOOTER_SIZE);
        }
    } else {
        if (mainCheck.result == 1 || backupCheck.result == 1) {
            func_0203be8c(sv, BLOCK_STATUS_WRITE_MAIN);
            GFL_HeapFree(backupData);
            return SAVE_LOAD_RESULT_BROKEN;
        }
        func_0203be8c(sv, BLOCK_STATUS_WRITE_MAIN);
        GFL_HeapFree(backupData);
        return SAVE_LOAD_RESULT_NONE;
    }
    // Try the newer copy's CRCs first
    if (mainCheck.result == 0
        && ((mainCheck.count == 0 && backupCheck.count == 0xffffffff) || mainCheck.count >= backupCheck.count
            || backupCheck.result != 0)) {
        datas[0] = mainData;
        copies[0] = SAVE_COPY_MAIN;
        datas[1] = backupData;
        copies[1] = SAVE_COPY_BACKUP;
        counts[0] = mainCheck.count;
        counts[1] = backupCheck.count;
    } else {
        datas[0] = backupData;
        copies[0] = SAVE_COPY_BACKUP;
        datas[1] = mainData;
        copies[1] = SAVE_COPY_MAIN;
        counts[0] = backupCheck.count;
        counts[1] = mainCheck.count;
    }
    for (copyIndex = 0; copyIndex < 2; copyIndex++) {
        data = datas[copyIndex];
        crcs = (u16 *)(data + (sv->saveSize - SAVE_FOOTER_SIZE - sv->crcsSize));
        for (i = 0; i < getSaveSize(sv->table); i++) {
            offset = getStartOffsetOfSaveBlockDataInSave(sv->table, i);
            size = getSizeOfSaveBlockData(sv->table, i);
            end = offset + size;
            dataFooter = (BlockFooter *)(data + end);
            mainFooter = (BlockFooter *)(mainData + end);
            backupFooter = (BlockFooter *)(backupData + end);
            func_0203bdbc(sv, i, BLOCK_STATUS_OK);
            if (mainFooter->crc != crcs[i] && backupFooter->crc != crcs[i]) {
                break;
            }
            if (mainFooter->crc == crcs[i]) {
                if (mainFooter->crc != MATH_CalcCRC16CCITT(&svsys->crcTable, mainData + offset, size)) {
                    func_0203bdbc(sv, i, BLOCK_STATUS_WRITE_MAIN);
                }
            } else {
                func_0203bdbc(sv, i, BLOCK_STATUS_WRITE_MAIN);
            }
            if (backupFooter->crc == crcs[i]) {
                if (backupFooter->crc != MATH_CalcCRC16CCITT(&svsys->crcTable, backupData + offset, size)) {
                    if (func_0203bda0(sv, i) == BLOCK_STATUS_WRITE_MAIN) {
                        break;
                    }
                    func_0203bdbc(sv, i, BLOCK_STATUS_WRITE_BACKUP);
                }
            } else {
                if (func_0203bda0(sv, i) == BLOCK_STATUS_WRITE_MAIN) {
                    break;
                }
                func_0203bdbc(sv, i, BLOCK_STATUS_WRITE_BACKUP);
            }
            if (func_0203bda0(sv, i) == BLOCK_STATUS_OK) {
                if (dataFooter->count != mainFooter->count) {
                    func_0203bdbc(sv, i, BLOCK_STATUS_WRITE_MAIN);
                }
                if (dataFooter->count != backupFooter->count) {
                    if (func_0203bda0(sv, i) == BLOCK_STATUS_WRITE_MAIN) {
                        break;
                    }
                    func_0203bdbc(sv, i, BLOCK_STATUS_WRITE_BACKUP);
                }
            }
        }
        if (i >= getSaveSize(sv->table)) {
            u32 status = copies[copyIndex ^ 1];

            func_0203bdbc(sv, getSaveSize(sv->table) + 1, status);
            func_0203bdbc(sv, getSaveSize(sv->table), status);
            break;
        }
    }
    if (copyIndex >= 2) {
        func_0203be8c(sv, BLOCK_STATUS_WRITE_MAIN);
        GFL_HeapFree(backupData);
        return SAVE_LOAD_RESULT_BROKEN;
    }
    offset = sv->saveSize - SAVE_FOOTER_SIZE - sv->crcsSize;
    sys_memcpy16(datas[copyIndex] + offset, mainData + offset, sv->crcsSize + SAVE_FOOTER_SIZE);
    for (i = 0; i < getSaveSize(sv->table); i++) {
        if (func_0203bda0(sv, i) == BLOCK_STATUS_WRITE_MAIN) {
            offset = getStartOffsetOfSaveBlockDataInSave(sv->table, i);
            size = getSizeOfSaveBlockData(sv->table, i);
            sys_memcpy16(backupData + offset, mainData + offset, size + sizeof(BlockFooter));
        }
    }
    func_0203b144(sv, counts[copyIndex]);
    GFL_HeapFree(backupData);
    for (i = 0; i < getSaveSize(sv->table); i++) {
        func_0203bda0(sv, i);
    }
    return SAVE_LOAD_RESULT_OK;
}

// Loads a save without a backup
static u32 func_0203b598(SaveData *sv, BOOL checkCount, u32 count) {
    u8 *data;
    u16 *crcs;
    u32 savedCount;
    u32 size;
    FooterCheck check;
    u32 i;
    u32 offset;
    BlockFooter *footer;

    func_0203be74(sv);
    data = sv->data;
    if (!readFlashWaitSuccess(func_0203b0b4(sv, SAVE_COPY_MAIN), data, sv->saveSize, svsys->readErrorCallback)) {
        func_0203be8c(sv, BLOCK_STATUS_WRITE_MAIN);
        sys_memset32(0, data, sv->saveSize);
        return SAVE_LOAD_RESULT_NONE;
    }
    func_0203b148(&check, sv, data, 0);
    if (checkCount == TRUE && check.count != count) {
        check.result = 2;
    }
    if (check.result != 0) {
        if (check.result == 1) {
            func_0203be8c(sv, BLOCK_STATUS_WRITE_MAIN);
            return SAVE_LOAD_RESULT_BROKEN;
        }
        func_0203be8c(sv, BLOCK_STATUS_WRITE_MAIN);
        return SAVE_LOAD_RESULT_NONE;
    }
    savedCount = check.count;
    crcs = (u16 *)(data + (sv->saveSize - SAVE_FOOTER_SIZE - sv->crcsSize));
    for (i = 0; i < getSaveSize(sv->table); i++) {
        offset = getStartOffsetOfSaveBlockDataInSave(sv->table, i);
        size = getSizeOfSaveBlockData(sv->table, i);
        footer = (BlockFooter *)(data + (offset + size));
        func_0203bdbc(sv, i, BLOCK_STATUS_OK);
        if (footer->crc != crcs[i]) {
            break;
        }
        if (footer->crc == crcs[i] && footer->crc != MATH_CalcCRC16CCITT(&svsys->crcTable, data + offset, size)) {
            func_0203bdbc(sv, i, BLOCK_STATUS_WRITE_MAIN);
            break;
        }
    }
    if (i >= getSaveSize(sv->table)) {
        func_0203bdbc(sv, getSaveSize(sv->table) + 1, BLOCK_STATUS_WRITE_MAIN);
        func_0203bdbc(sv, getSaveSize(sv->table), BLOCK_STATUS_WRITE_MAIN);
    } else {
        func_0203be8c(sv, BLOCK_STATUS_WRITE_MAIN);
        return SAVE_LOAD_RESULT_BROKEN;
    }
    func_0203b144(sv, savedCount);
    for (i = 0; i < getSaveSize(sv->table); i++) {
        func_0203bda0(sv, i);
    }
    return SAVE_LOAD_RESULT_OK;
}

// Writes the footer but its last byte
static u16 func_0203b714(SaveData *sv, u32 copy) {
    u32 size = sv->saveSize;
    u32 dest = size + func_0203b0b4(sv, copy) - SAVE_FOOTER_SIZE;
    u8 *data = sv->data;

    func_0203b120(sv, data);
    return func_0203c07c(dest, data + (size - SAVE_FOOTER_SIZE), SAVE_FOOTER_SIZE - 1, svsys->writeErrorCallback);
}

// Writes the footer's last byte, which makes the save valid
static u16 func_0203b748(SaveData *sv, u32 copy) {
    u32 size = sv->saveSize;

    return func_0203c07c(size + func_0203b0b4(sv, copy) - 1, sv->data + (size - 1), 1, svsys->writeErrorCallback);
}

static void func_0203b774(SaveData *sv, BOOL blockSleep) {
    sv->prevCount = sv->count;
    sv->count++;
    sv->state = 0;
    sv->blockIndex = 0;
    sv->pass = 0;
    sv->written = 0;
    if (blockSleep == TRUE) {
        GCTX_HIDBlockSleep(1);
    }
}

static void func_0203b798(SaveData *sv, u32 *changed, u32 *total) {
    u32 i = 0;
    u32 size;
    u32 result;
    u32 remaining;
    u32 frameSize;
    u32 pos;
    BOOL next_frame;

    GCTX_HIDBlockSleep(1);
    *changed = 0;
    *total = 0;
    remaining = 0;
    frameSize = 0;
    pos = 0;
    do {
        size = getSizeOfSaveBlockData(sv->table, i);
        *total += size;
        if (func_0203bda0(sv, i) == BLOCK_STATUS_OK) {
            do {
                result = func_0203b81c(sv, i, SAVE_COPY_MAIN, &remaining, &frameSize, &pos, &next_frame);
                if (result == 2) {
                    size = 0;
                }
            } while (result == 0);
        }
        *changed += size;
        i++;
    } while (i < getSaveSize(sv->table));
    GCTX_HIDUnblockSleep(1);
}

// Compares a block with the card, a frame's worth at a time. Returns 0 to continue next frame, 1 if the block differs
// and 2 if it is the same
static u32 func_0203b81c(SaveData *sv, u32 gmdataid, u32 copy, u32 *remaining, u32 *frameSize, u32 *pos,
    BOOL *next_frame) {
    u32 offset = func_0203b0b4(sv, copy) + getStartOffsetOfSaveBlockDataInSave(sv->table, gmdataid);
    u32 size = *remaining;
    u32 blockPos;
    u8 *block;
    u32 chunk;
    u8 *buffer;

    if (size == 0) {
        size = getSizeOfSaveBlockData(sv->table, gmdataid);
        blockPos = 0;
    } else {
        blockPos = *pos;
        offset += blockPos;
    }
    block = SaveData_GetBlockPtr(sv, gmdataid);
    *remaining = 0;
    *pos = 0;
    *next_frame = FALSE;
    buffer = sv->compareBuffer;
    do {
        if (size > COMPARE_CHUNK_SIZE) {
            chunk = COMPARE_CHUNK_SIZE;
            size -= COMPARE_CHUNK_SIZE;
        } else {
            chunk = size;
            size = 0;
        }
        *frameSize += chunk;
        if (*frameSize > COMPARE_SIZE_PER_FRAME) {
            *next_frame = TRUE;
            *frameSize = 0;
        }
        if (!readFlashWaitSuccess(offset, buffer, chunk, svsys->readErrorCallback)) {
            return 1;
        }
        if (GFL_STD_MemCmp(buffer, block + blockPos, chunk) != 0) {
            return 1;
        }
        blockPos += chunk;
        offset += chunk;
        if (*next_frame == TRUE) {
            if (size == 0) {
                return 2;
            }
            *remaining = size;
            *pos = blockPos;
            return 0;
        }
    } while (size != 0);
    return 2;
}

static u16 func_0203b92c(SaveData *sv, u32 gmdataid, u32 copy, u32 pass) {
    u16 *crcs = func_0203b0c8(sv);
    u32 base;
    u32 offset;
    u32 size;

    crcs[gmdataid] = getCrcOfSaveBlockStoreAtEnd(sv, gmdataid);
    base = func_0203b0b4(sv, copy);
    offset = getStartOffsetOfSaveBlockDataInSave(sv->table, gmdataid);
    size = getSizeOfSaveBlockData(sv->table, gmdataid);
    return func_0203c07c(base + offset, SaveData_GetBlockPtr(sv, gmdataid), size + sizeof(BlockFooter),
        svsys->writeErrorCallback);
}

// Writes the blocks' CRCs
static u16 func_0203b988(SaveData *sv, u32 copy) {
    u32 dest = sv->saveSize + func_0203b0b4(sv, copy) - SAVE_FOOTER_SIZE - sv->crcsSize;
    u16 *crcs = func_0203b0c8(sv);

    return func_0203c07c(dest, crcs, sv->crcsSize, svsys->writeErrorCallback);
}


// Saves a step at a time: finds the blocks that differ from the card, then writes them, the CRCs and the footer, to
// the main copy and then the backup. *passEnded tells when the main copy is done
static u32 func_0203b9bc(SaveData *sv, BOOL *passEnded) {
    BOOL result;
    u32 status;

    *passEnded = FALSE;
    switch (sv->state) {
    case 0:
        sv->compareRemaining = 0;
        sv->compareSize = 0;
        sv->comparePos = 0;
        sv->state++;
    case 1: {
        BOOL next_frame = FALSE;

        do {
            if (next_frame == TRUE) {
                return SAVE_RESULT_CONTINUE;
            }
            if (func_0203bda0(sv, sv->blockIndex) == BLOCK_STATUS_OK) {
                u32 compare = func_0203b81c(sv, sv->blockIndex, SAVE_COPY_MAIN, &sv->compareRemaining,
                    &sv->compareSize, &sv->comparePos, &next_frame);

                if (compare == 1) {
                    func_0203bdbc(sv, sv->blockIndex, BLOCK_STATUS_WRITE_MAIN);
                    func_0203bd48(sv, sv->blockIndex);
                } else if (compare == 0) {
                    GFL_ASSERT(next_frame == TRUE);
                    return SAVE_RESULT_CONTINUE;
                }
            } else {
                func_0203bd48(sv, sv->blockIndex);
            }
            sv->blockIndex++;
        } while (sv->blockIndex < getSaveSize(sv->table));
        sv->state++;
        break;
    }
    case 2:
        sv->blockIndex = 0;
        sv->state++;
    case 3:
        for (; sv->blockIndex < getSaveSize(sv->table); sv->blockIndex++) {
            status = func_0203bda0(sv, sv->blockIndex);
            if (status != BLOCK_STATUS_OK) {
                svsys->lockId = func_0203b92c(sv, sv->blockIndex, status, sv->pass);
                sv->state++;
                break;
            }
        }
        if (sv->blockIndex >= getSaveSize(sv->table)) {
            sv->blockIndex = 0;
            sv->state = 5;
        }
        break;
    case 4:
        if (finishBackupGetResult(svsys->lockId, &result, svsys->writeErrorCallback)) {
            if (result == FALSE) {
                if (sv->pass == 0) {
                    return SAVE_RESULT_NG;
                } else {
                    return SAVE_RESULT_OK;
                }
            }
            sv->written += getSizeOfSaveBlockData(sv->table, sv->blockIndex);
            sv->blockIndex++;
            sv->state = 3;
        }
        break;
    case 5:
        status = func_0203bda0(sv, getSaveSize(sv->table));
        if (status == BLOCK_STATUS_OK) {
            status = BLOCK_STATUS_WRITE_MAIN;
            func_0203bdbc(sv, getSaveSize(sv->table), BLOCK_STATUS_WRITE_MAIN);
        }
        svsys->lockId = func_0203b988(sv, status);
        sv->state++;
    case 6:
        if (finishBackupGetResult(svsys->lockId, &result, svsys->writeErrorCallback)) {
            if (result == FALSE) {
                if (sv->pass == 0) {
                    return SAVE_RESULT_NG;
                } else {
                    return SAVE_RESULT_OK;
                }
            }
            sv->state++;
        }
        break;
    case 7:
        status = func_0203bda0(sv, getSaveSize(sv->table) + 1);
        if (status == BLOCK_STATUS_OK) {
            status = BLOCK_STATUS_WRITE_MAIN;
            func_0203bdbc(sv, getSaveSize(sv->table) + 1, BLOCK_STATUS_WRITE_MAIN);
        }
        svsys->lockId = func_0203b714(sv, status);
        sv->state++;
    case 8:
        if (finishBackupGetResult(svsys->lockId, &result, svsys->writeErrorCallback)) {
            if (result == FALSE) {
                if (sv->pass == 0) {
                    return SAVE_RESULT_NG;
                } else {
                    return SAVE_RESULT_OK;
                }
            }
            sv->state++;
            return SAVE_RESULT_LAST;
        }
        break;
    case 9:
        svsys->lockId = func_0203b748(sv, func_0203bda0(sv, getSaveSize(sv->table) + 1));
        sv->state++;
    case 10:
        if (finishBackupGetResult(svsys->lockId, &result, svsys->writeErrorCallback)) {
            if (result == FALSE) {
                if (sv->pass == 0) {
                    return SAVE_RESULT_NG;
                } else {
                    return SAVE_RESULT_OK;
                }
            }
            sv->pass++;
            if (sv->pass == 1 && sv->mainOffset != sv->backupOffset) {
                func_0203bdf0(sv);
                sv->state = 2;
                *passEnded = TRUE;
                return SAVE_RESULT_CONTINUE;
            }
            func_0203be74(sv);
            sv->state++;
            return SAVE_RESULT_OK;
        }
        break;
    case 11:
        return SAVE_RESULT_OK;
    }
    return SAVE_RESULT_CONTINUE;
}

static void func_0203bcd0(SaveData *sv, u32 result, BOOL unblockSleep) {
    if (result == SAVE_RESULT_NG) {
        sv->count = sv->prevCount;
    } else {
        sv->dataExists = TRUE;
    }
    if (unblockSleep == TRUE) {
        GCTX_HIDUnblockSleep(1);
    }
}

static void func_0203bcec(SaveData *sv) {
    sv->count = sv->prevCount;
    cancelCardProcess(svsys->lockId);
    GCTX_HIDUnblockSleep(1);
}

// Erases a copy's footer
static void func_0203bd0c(SaveData *sv, u32 copy) {
    u8 footer[SAVE_FOOTER_SIZE];

    sys_memset(footer, 0xff, SAVE_FOOTER_SIZE);
    func_0203bfd4(sv->saveSize + func_0203b0b4(sv, copy) - SAVE_FOOTER_SIZE, footer, SAVE_FOOTER_SIZE,
        svsys->writeErrorCallback);
}

static u16 func_0203bd48(SaveData *sv, u32 gmdataid) {
    u32 offset = getStartOffsetOfSaveBlockDataInSave(sv->table, gmdataid);
    u32 size = getSizeOfSaveBlockData(sv->table, gmdataid);
    BlockFooter *footer = (BlockFooter *)(sv->data + (offset + size));

    footer->count++;
    return footer->count;
}

static u16 getCrcOfSaveBlockStoreAtEnd(SaveData *sv, u32 gmdataid) {
    u32 offset = getStartOffsetOfSaveBlockDataInSave(sv->table, gmdataid);
    u32 size = getSizeOfSaveBlockData(sv->table, gmdataid);
    BlockFooter *footer = (BlockFooter *)(sv->data + (offset + size));

    footer->crc = MATH_CalcCRC16CCITT(&svsys->crcTable, sv->data + offset, size);
    return footer->crc;
}

static u32 func_0203bda0(SaveData *sv, u32 index) {
    return (sv->blockStatus[index / 16] >> (index % 16 * 2)) & 3;
}

static void func_0203bdbc(SaveData *sv, u32 index, u32 status) {
    u32 shift = index % 16 * 2;
    u32 word = index / 16;

    sv->blockStatus[word] &= 0xffffffff ^ (3 << shift);
    sv->blockStatus[word] |= status << shift;
}


// Swaps which copy each block is to be written to, for the backup's pass
static void func_0203bdf0(SaveData *sv) {
    u32 count = getSaveSize(sv->table) + 2;
    u32 i = 0;
    u32 word = 0;
    u32 shift = 0;

    for (; i < count; i++) {
        u32 status = (sv->blockStatus[word] >> shift) & 3;

        if (status != BLOCK_STATUS_OK) {
            if (status == BLOCK_STATUS_WRITE_MAIN) {
                status = BLOCK_STATUS_WRITE_BACKUP;
            } else if (status == BLOCK_STATUS_WRITE_BACKUP) {
                status = BLOCK_STATUS_WRITE_MAIN;
            }
            sv->blockStatus[word] &= 0xffffffff ^ (3 << shift);
            sv->blockStatus[word] |= status << shift;
        }
        shift += 2;
        if (shift >= 32) {
            shift -= 32;
            word++;
        }
    }
}

static void func_0203be74(SaveData *sv) {
    sys_memset32(0, sv->blockStatus, sv->blockStatusSize);
}

static void func_0203be8c(SaveData *sv, u32 status) {
    u32 word = 0;
    u32 count;
    u32 shift;
    u32 i;

    sys_memset32(0, sv->blockStatus, sv->blockStatusSize);
    count = getSaveSize(sv->table) + 2;
    shift = 0;
    for (i = 0; i < count; i++) {
        sv->blockStatus[word] |= status << shift;
        shift += 2;
        if (shift >= 32) {
            shift -= 32;
            word++;
        }
    }
}

BOOL func_0203bee8(u32 dest, const void *src, u32 size) {
    return func_0203bfd4(dest, src, size, svsys->writeErrorCallback);
}

u16 func_0203bf00(u32 dest, const void *src, u32 size) {
    return func_0203c07c(dest, src, size, svsys->writeErrorCallback);
}

BOOL func_0203bf18(u16 lockId, BOOL *result) {
    return finishBackupGetResult(lockId, result, svsys->writeErrorCallback);
}

BOOL func_0203bf30(u32 src, void *dest, u32 size) {
    return readFlashWaitSuccess(src, dest, size, svsys->readErrorCallback);
}

BOOL func_0203bf48(SaveData *sv, u32 gmdataid, u32 copy, void *dest, u32 size) {
    return readFlashWaitSuccess(func_0203b0b4(sv, copy + 1) + getStartOffsetOfSaveBlockDataInSave(sv->table, gmdataid),
        dest, size, svsys->readErrorCallback);
}
