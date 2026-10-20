#include "types.h"
#include "gfl/backup_system.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "save/cygnus_save_control.h"
#include "save/download_data.h"
#include "save/key_info.h"
#include "save/memory_link_savedata.h"
#include "save/musical_savedata.h"
#include "save/save_control.h"
#include "save/save_outside.h"
#include "save/save_table.h"
#include "save/trainer_data_savedata.h"

// The game's save: loading it when the game starts, saving it a step at a time, and the extra saves. The name is the
// ROM's string, and SaveControlWork, ctrl, sv_extra and extra_id are the names its assertions keep. The tables of the
// saves' blocks are save_table.c's.

#define OVERLAY_SAVE_OUTSIDE OVERLAY_ID(331)

// The data outside the save that save_outside.c (overlay 331) reads: the gifts, their backup copy and the keys
#define SAVE_OUTSIDE_GIFTS_ADDR 0x7e000
#define SAVE_OUTSIDE_GIFTS_BACKUP_ADDR 0x7e400
#define SAVE_OUTSIDE_KEYS_ADDR 0x7e800

// GFLib's GFL_STD_MemCopy, an inline over the copy. SaveControl_Create reads the destination before the call that
// gives the source only through it
static inline void GFL_STD_MemCopy(const void *src, void *dest, u32 size) {
    MI_CpuCopy8(src, dest, size);
}

static void SaveControl_ClearOutsideGifts(void);
static void SaveControl_ClearOutsideKeys(void);

static SaveControl *SaveControlWork;

SaveControl *SaveControl_Create(HeapID heapId) {
    SaveControl *ctrl;
    u32 result;
    u32 extraResult;
    SaveExtraID i;
    SaveOutside *outside;
    void *keyData;

    ctrl = GFL_HeapAllocate(heapId, sizeof(SaveControl), TRUE, "save_control.c", 81);
    SaveControlWork = ctrl;
    ctrl->newGame = TRUE;
    ctrl->unk2 = TRUE;
    ctrl->dataExists = FALSE;
    ctrl->sv = SaveData_InitMain(&save_block_data, heapId);
    ctrl->memoryLink = GFL_HeapAllocate(heapId, sizeof(MemoryLinkSave), TRUE, "save_control.c", 87);
    zero476Bytes(ctrl->memoryLink);

    result = SaveData_Load(ctrl->sv, HEAPID_USER);
    if ((result == SAVE_LOAD_RESULT_OK || result == SAVE_LOAD_RESULT_OK_PARTIAL) &&
        TrainerDataSave_IsIntroSave(ctrl) == TRUE) {
        result = SAVE_LOAD_RESULT_NONE;
    }
    ctrl->errors = 0;
    switch (result) {
    case SAVE_LOAD_RESULT_OK:
        ctrl->unk2 = FALSE;
        // fallthrough
    case SAVE_LOAD_RESULT_OK_PARTIAL:
        ctrl->newGame = FALSE;
        ctrl->dataExists = TRUE;
        if (result == SAVE_LOAD_RESULT_OK_PARTIAL) {
            ctrl->errors |= SAVE_CONTROL_ERROR_PARTIAL;
        }
        for (i = SAVE_EXTRA_ID_BATTLE_REC_0; i < SAVE_EXTRA_ID_MAX; i++) {
            if (TrainerDataSave_IsMarkerSet(ctrl, i) == TRUE) {
                extraResult = SaveControl_OpenExtraBlock(ctrl, i, HEAPID_USER);
                freeIntermediateSaveExtraBlksAfterLoad(ctrl, i);
                switch (extraResult) {
                case SAVE_LOAD_RESULT_OK:
                case SAVE_LOAD_RESULT_OK_PARTIAL:
                    if (i == SAVE_EXTRA_ID_MUSICAL_DIST) {
                        MusicalSave_SetHasDistProgram(getAddressOfMusicalDataInfo(ctrl), TRUE);
                    }
                    break;
                case SAVE_LOAD_RESULT_NONE:
                case SAVE_LOAD_RESULT_BROKEN:
                case SAVE_LOAD_RESULT_ERROR:
                    ctrl->errors |= SAVE_CONTROL_ERROR_EXTRA(i);
                    if (i == SAVE_EXTRA_ID_MUSICAL_DIST) {
                        MusicalSave_SetHasDistProgram(getAddressOfMusicalDataInfo(ctrl), FALSE);
                    }
                    break;
                }
            } else if (i == SAVE_EXTRA_ID_MUSICAL_DIST) {
                MusicalSave_SetHasDistProgram(getAddressOfMusicalDataInfo(ctrl), FALSE);
            }
        }
        break;
    case SAVE_LOAD_RESULT_BROKEN:
        ctrl->errors |= SAVE_CONTROL_ERROR_BROKEN;
        // fallthrough
    case SAVE_LOAD_RESULT_NONE:
    case SAVE_LOAD_RESULT_ERROR:
        initSaveBlkCallbacks(ctrl->sv);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }

    GFL_OvlLoad(OVERLAY_SAVE_OUTSIDE);
    outside = SaveOutside_Load(HEAPID_TAIL(HEAPID_USER));
    ctrl->giftsLoaded = SaveOutside_IsGiftsLoaded(outside);
    ctrl->giftsBroken = SaveOutside_IsGiftsBroken(outside);
    ctrl->keysLoaded = SaveOutside_IsKeysLoaded(outside);
    ctrl->keysBroken = SaveOutside_IsKeysBroken(outside);
    keyData = SaveOutside_GetKeyData(outside);
    if (ctrl->keysLoaded && !ctrl->keysBroken) {
        GFL_STD_MemCopy(KeySystemData_GetMemoryLink(keyData), ctrl->memoryLink, sizeof(MemoryLinkSave));
    }
    SaveOutside_Free(outside);
    GFL_OvlUnload(OVERLAY_SAVE_OUTSIDE);
    if (ctrl->giftsBroken == TRUE) {
        ctrl->errors |= SAVE_CONTROL_ERROR_GIFTS_BROKEN;
    }
    if (ctrl->keysBroken == TRUE) {
        ctrl->errors |= SAVE_CONTROL_ERROR_KEYS_BROKEN;
    }

    if (!DreamRadar_LoadAndCheckSaveData(HEAPID_TAIL(HEAPID_USER)) || ctrl->newGame == TRUE || !ctrl->dataExists) {
        ctrl->markDreamRadarRead = TRUE;
    } else {
        ctrl->markDreamRadarRead = FALSE;
    }
    return ctrl;
}

void SaveControl_Free(void) {
    if (SaveControlWork != NULL) {
        freeSaveBlocks(SaveControlWork->sv);
        GFL_HeapFree(SaveControlWork->memoryLink);
        GFL_HeapFree(SaveControlWork);
        SaveControlWork = NULL;
    }
}

SaveControl *SaveControl_GetInstance(void) {
    GFL_ASSERT(SaveControlWork != NULL);
    return SaveControlWork;
}

u32 SaveControl_Reload(SaveControl *save) {
    u32 result;
    u32 extraResult;
    MusicalSave *musical;
    SaveExtraID i;

    result = SaveData_Load(save->sv, HEAPID_USER);
    switch (result) {
    case SAVE_LOAD_RESULT_OK:
    case SAVE_LOAD_RESULT_OK_PARTIAL:
        save->dataExists = TRUE;
        musical = getAddressOfMusicalDataInfo(save);
        MusicalSave_SetHasDistProgram(musical, FALSE);
        if (TrainerDataSave_IsMarkerSet(save, SAVE_EXTRA_ID_MUSICAL_DIST) == TRUE) {
            extraResult = SaveControl_OpenExtraBlock(save, SAVE_EXTRA_ID_MUSICAL_DIST, HEAPID_USER);
            freeIntermediateSaveExtraBlksAfterLoad(save, SAVE_EXTRA_ID_MUSICAL_DIST);
            switch (extraResult) {
            case SAVE_LOAD_RESULT_OK:
            case SAVE_LOAD_RESULT_OK_PARTIAL:
                MusicalSave_SetHasDistProgram(musical, TRUE);
                break;
            }
        }
        if (save->errors != 0) {
            for (i = SAVE_EXTRA_ID_BATTLE_REC_0; i < SAVE_EXTRA_ID_MAX; i++) {
                if (save->errors & SAVE_CONTROL_ERROR_EXTRA(i)) {
                    TrainerDataSave_ClearMarker(save, i);
                }
            }
        }
        break;
    }
    return result;
}

void SaveControl_StartSave(SaveControl *save) {
    save->wasIntroSave = TrainerDataSave_IsIntroSave(save);
    TrainerDataSave_ClearIntroSave(save);
    SaveData_SaveStart(save->sv);
}

u32 SaveControl_ContinueSave(SaveControl *save) {
    u32 result;

    if (save->giftsLoaded == TRUE || save->giftsBroken == TRUE) {
        SaveControl_ClearOutsideGifts();
        save->giftsLoaded = FALSE;
        save->giftsBroken = FALSE;
        return SAVE_RESULT_CONTINUE;
    }
    if (save->keysBroken == TRUE) {
        SaveControl_ClearOutsideKeys();
        save->keysLoaded = FALSE;
        save->keysBroken = FALSE;
        return SAVE_RESULT_CONTINUE;
    }
    result = SaveData_SaveStep(save->sv);
    if (result == SAVE_RESULT_OK && TrainerDataSave_IsIntroSave(save) == FALSE) {
        if (save->markDreamRadarRead) {
            DreamRadar_MarkRead(save);
            save->markDreamRadarRead = FALSE;
        }
        save->newGame = FALSE;
        save->dataExists = TRUE;
    }
    return result;
}

void SaveControl_CancelSave(SaveControl *save) {
    SaveData_SaveCancel(save->sv);
    if (save->wasIntroSave == TRUE) {
        TrainerDataSave_SetIntroSave(save);
    }
}

u32 SaveControl_GetBytesWritten(SaveControl *save) {
    return SaveData_GetBytesWritten(save->sv);
}

void *SaveControl_GetBlockPtr(SaveControl *save, u32 block) {
    return SaveData_GetBlockPtr(save->sv, block);
}

void *SaveControl_GetUploadData(SaveControl *save, u32 *size) {
    return SaveData_GetData(save->sv, size);
}

u32 SaveControl_GetStatus(SaveControl *save) {
    return save->newGame;
}

u32 SaveControl_GetErrors(SaveControl *save) {
    return save->errors;
}

BOOL SaveControl_IsDataAlreadyPresent(SaveControl *save) {
    return save->dataExists;
}

BOOL func_0200746c(SaveControl *save) {
    if (SaveControl_GetStatus(save) == TRUE && SaveControl_IsDataAlreadyPresent(save) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

u8 SaveControl_GetGiftsLoaded(SaveControl *save) {
    return save->giftsLoaded;
}

void SaveControl_SetGiftsLoaded(SaveControl *save, u32 value) {
    save->giftsLoaded = value;
}

u8 SaveControl_GetGiftsBroken(SaveControl *save) {
    return save->giftsBroken;
}

void SaveControl_SetGiftsBroken(SaveControl *save, u32 value) {
    save->giftsBroken = value;
}

void SaveControl_InitNewGame(SaveControl *save) {
    save->newGame = TRUE;
    save->unk2 = TRUE;
    initSaveBlkCallbacks(save->sv);
}

void func_020074ac(SaveControl *save) {
    SaveData_Erase(save->sv, HEAPID_USER);
}

void func_020074b8(SaveControl *save, u32 *a1, u32 *a2) {
    SaveData_GetChangedSize(save->sv, a1, a2);
}

BOOL SaveControl_ReadBlockCopy(SaveControl *save, u32 block, u32 copy, void *dest, u32 size) {
    return SaveData_ReadBlockFromCard(save->sv, block, copy, dest, size);
}

void *SaveControl_GetMemoryLink(SaveControl *save) {
    return save->memoryLink;
}

u8 SaveControl_GetKeysLoaded(SaveControl *save) {
    return save->keysLoaded;
}

void SaveControl_SetKeysLoaded(SaveControl *save, u32 value) {
    save->keysLoaded = value;
}

u8 SaveControl_GetKeysBroken(SaveControl *save) {
    return save->keysBroken;
}

void SaveControl_SetKeysBroken(SaveControl *save, u32 value) {
    save->keysBroken = value;
}

u32 SaveControl_OpenExtraBlock(SaveControl *save, u32 block, HeapID heapId) {
    save->sv_extra[block] = SaveData_InitMain(&sv_extra_blk_data[block], heapId);
    if (TrainerDataSave_IsMarkerSet(save, block) == FALSE) {
        return SAVE_LOAD_RESULT_NONE;
    }
    return SaveData_LoadWithCount(save->sv_extra[block], heapId, *TrainerDataSave_GetExtraSaveCount(save, block));
}

void freeIntermediateSaveExtraBlksAfterLoad(SaveControl *ctrl, u32 extra_id) {
    GFL_ASSERT(ctrl->sv_extra[extra_id] != NULL);
    freeSaveBlocks(ctrl->sv_extra[extra_id]);
    ctrl->sv_extra[extra_id] = NULL;
}

u32 SaveControl_LoadExtraBlock(SaveControl *save, u32 block, HeapID heapId, void *buffer, u32 size) {
    save->sv_extra[block] = SaveData_InitWithBuffer(&sv_extra_blk_data[block], heapId, buffer, size, TRUE);
    if (TrainerDataSave_IsMarkerSet(save, block) == FALSE) {
        return SAVE_LOAD_RESULT_NONE;
    }
    return SaveData_LoadWithCount(save->sv_extra[block], heapId, *TrainerDataSave_GetExtraSaveCount(save, block));
}

void SaveControl_InitExtraBlock(SaveControl *save, u32 block, u32 heapId, void *buffer, u32 size) {
    save->sv_extra[block] = SaveData_InitWithBuffer(&sv_extra_blk_data[block], heapId, buffer, size, FALSE);
}

void freeIntermediateSaveExtraBlksAfterLoad2(SaveControl *ctrl, u32 extra_id) {
    GFL_ASSERT(ctrl->sv_extra[extra_id] != NULL);
    freeSaveBlocks(ctrl->sv_extra[extra_id]);
    ctrl->sv_extra[extra_id] = NULL;
}

void SaveControl_StartExtraSave(SaveControl *ctrl, u32 extra_id) {
    GFL_ASSERT(ctrl->sv_extra[extra_id] != NULL);
    TrainerDataSave_SetMarker(ctrl, extra_id);
    SaveData_DualSaveStart(ctrl->sv, ctrl->sv_extra[extra_id], TrainerDataSave_GetExtraSaveCount(ctrl, extra_id));
}

u32 SaveControl_ContinueExtraSave(SaveControl *ctrl, u32 extra_id) {
    GFL_ASSERT(ctrl->sv_extra[extra_id] != NULL);
    return SaveData_DualSaveStep(ctrl->sv, ctrl->sv_extra[extra_id]);
}

void *getAddressOfExtraSaveBlk(SaveControl *ctrl, u32 extra_id, u32 arg2) {
    GFL_ASSERT(ctrl->sv_extra[extra_id] != NULL);
    return SaveData_GetBlockPtr(ctrl->sv_extra[extra_id], arg2);
}

void SaveControl_ClearExtraBlock(SaveControl *ctrl, u32 extra_id, HeapID heapId) {
    GFL_ASSERT(ctrl->sv_extra[extra_id] != NULL);
    SaveData_Erase(ctrl->sv_extra[extra_id], heapId);
}

// Erases the gifts outside the save, by clearing the start of both copies' headers
static void SaveControl_ClearOutsideGifts(void) {
    u32 zero = 0;

    BackupSystem_WriteWait(SAVE_OUTSIDE_GIFTS_ADDR, &zero, sizeof(zero));
    BackupSystem_WriteWait(SAVE_OUTSIDE_GIFTS_BACKUP_ADDR, &zero, sizeof(zero));
}

// Erases the keys outside the save
static void SaveControl_ClearOutsideKeys(void) {
    u32 zero = 0;

    BackupSystem_WriteWait(SAVE_OUTSIDE_KEYS_ADDR, &zero, sizeof(zero));
}
