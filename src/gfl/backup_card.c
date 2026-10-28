#include "types.h"
#include "gfl/backup_card.h"
#include "gfl/std.h"
#include "nitro/card.h"
#include "nitro/os.h"

static void setCardWrittenVerified(void *arg);
static void unlockBackupAndReportError(u16 lockId, BOOL noResponse, BackupWriteErrorCallback callback);

// Whether the last write has ended
static BOOL sWriteDone = TRUE;

static BOOL sLocked;

BOOL identifyGameCardBackup(void) {
    s32 lock_id = OS_GetLockID();
    CARDBackupType type;

    GFL_ASSERT(lock_id != OS_LOCK_ID_ERROR);
    CARD_LockBackup(lock_id);
    type = CARD_BACKUP_TYPE_FLASH_4MBITS_EX;
    if (!CARD_IdentifyBackup(type)) {
        type = 0;
    }
    CARD_UnlockBackup(lock_id);
    OS_ReleaseLockID(lock_id);
    return type != 0 ? TRUE : FALSE;
}

BOOL writeFlashWaitSuccess(u32 dest, const void *src, u32 size, BackupWriteErrorCallback callback) {
    BOOL result;
    u16 lockId = startWriteFlash(dest, src, size, callback);

    while (!finishBackupGetResult(lockId, &result, callback)) {
#ifdef PLATFORM_PC
        // Native cooperative execution must yield while the genuine CARD task awaits its service reply.
        // The finisher remains responsible for errors and releasing the backup lock.
        CARD_WaitBackupAsync();
#endif
    }
    return result;
}

BOOL readFlashWaitSuccess(u32 src, void *dest, u32 size, BackupReadErrorCallback callback) {
    s32 lock_id = OS_GetLockID();
    BOOL result;

    GFL_ASSERT(lock_id != OS_LOCK_ID_ERROR);
    CARD_LockBackup(lock_id);
    CARD_ReadBackupAsync(src, dest, size, NULL, NULL);
    result = CARD_WaitBackupAsync();
    CARD_UnlockBackup(lock_id);
    OS_ReleaseLockID(lock_id);
    if (!result && callback != NULL) {
        callback();
    }
    return result;
}

static void setCardWrittenVerified(void *arg) {
    sWriteDone = TRUE;
}

u16 startWriteFlash(u32 dest, const void *src, u32 size, BackupWriteErrorCallback callback) {
    u32 header;
    s32 lock_id = OS_GetLockID();

    GFL_ASSERT(lock_id != OS_LOCK_ID_ERROR);
    CARD_LockBackup(lock_id);
    sLocked = TRUE;
    if (!CARD_ReadBackup(0, &header, sizeof(header))) {
        unlockBackupAndReportError(lock_id, TRUE, callback);
    }
    sWriteDone = FALSE;
    CARD_WriteAndVerifyBackupAsync(dest, src, size, setCardWrittenVerified, NULL);
    return lock_id;
}

BOOL finishBackupGetResult(u16 lockId, BOOL *result, BackupWriteErrorCallback callback) {
    if (sWriteDone == TRUE) {
        CARD_UnlockBackup(lockId);
        OS_ReleaseLockID(lockId);
        sLocked = FALSE;
        switch (CARD_GetResultCode()) {
        case CARD_RESULT_SUCCESS:
            *result = TRUE;
            break;
        default:
        case CARD_RESULT_FAILURE:
        case CARD_RESULT_INVALID_PARAM:
        case CARD_RESULT_UNSUPPORTED:
        case CARD_RESULT_TIMEOUT:
        case CARD_RESULT_ERROR:
        case CARD_RESULT_CANCELED:
            *result = FALSE;
            if (callback != NULL) {
                callback(FALSE);
            }
            break;
        case CARD_RESULT_NO_RESPONSE:
            *result = FALSE;
            if (callback != NULL) {
                callback(TRUE);
            }
            break;
        }
        return TRUE;
    }
    return FALSE;
}

void cancelCardProcess(u16 lockId) {
    if (!CARD_TryWaitBackupAsync()) {
        CARD_CancelBackupAsync();
        CARD_WaitBackupAsync();
    }
    if (sLocked == TRUE) {
        CARD_UnlockBackup(lockId);
        OS_ReleaseLockID(lockId);
        sLocked = FALSE;
    }
}

static void unlockBackupAndReportError(u16 lockId, BOOL noResponse, BackupWriteErrorCallback callback) {
    CARD_UnlockBackup(lockId);
    OS_ReleaseLockID(lockId);
    sLocked = FALSE;
    if (callback != NULL) {
        callback(noResponse);
    }
}

BOOL getLockIDStatus(void) {
    return sLocked;
}
