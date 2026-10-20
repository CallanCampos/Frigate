#ifndef POKEBW2_GFL_BACKUP_CARD_H
#define POKEBW2_GFL_BACKUP_CARD_H

#include "types.h"
#include "nitro/card.h"

// Reading and writing the card's flash for backup_system.c (backup_card.c, a name the ROM does not embed). A write
// runs in the background and returns the lock ID that finishBackupGetResult and cancelCardProcess take

// Called when a read fails
typedef void (*BackupReadErrorCallback)(void);
// Called when a write fails, with whether the card stopped responding
typedef void (*BackupWriteErrorCallback)(BOOL noResponse);

// Whether the card has the 4-megabit flash the game expects
BOOL identifyGameCardBackup(void);
// Writes and waits, returning whether the write succeeded
BOOL writeFlashWaitSuccess(u32 dest, const void *src, u32 size, BackupWriteErrorCallback callback);
BOOL readFlashWaitSuccess(u32 src, void *dest, u32 size, BackupReadErrorCallback callback);
u16 startWriteFlash(u32 dest, const void *src, u32 size, BackupWriteErrorCallback callback);
// Returns whether the write has ended, and if so whether it succeeded in *result
BOOL finishBackupGetResult(u16 lockId, BOOL *result, BackupWriteErrorCallback callback);
void cancelCardProcess(u16 lockId);
// Whether the backup is locked for a write
BOOL getLockIDStatus(void);

#endif // POKEBW2_GFL_BACKUP_CARD_H
