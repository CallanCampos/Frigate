// card.h's backup reads and writes with the game's pointer kept whole. card.h's inline functions CARD_ReadBackup,
// CARD_ReadBackupAsync and CARD_WriteAndVerifyBackupAsync cast the game's buffer to u32 for CARDi_RequestStreamCommand,
// as the DS's 32-bit pointers allow, and a 64-bit host can't get the pointer back from what is left of it. Here the
// three names call the host's functions (port/hal/card/card_backup_host.c), which take the pointer as it is.
//
// port/cmake/rom.cmake forces this header into the files that call them (src/gfl/backup_card.c), where it comes before
// the file's own includes: card.h is read here first, with its inline functions, and the file's calls of the three
// names then reach the host's. Nothing in the file changes. Once card.h's interface carries the address as uptr (a game
// change: CARDiCommon's src and dst, CARDi_RequestStreamCommand's parameters and the three casts), the decomp's own
// functions work on the host and this header goes away.
#ifndef PORT_HAL_CARD_CARD_BACKUP_CALLS_H
#define PORT_HAL_CARD_CARD_BACKUP_CALLS_H

#include "nitro/card.h"

// Reads size bytes of the backup from src into dest, calling the callback with its argument at the end, and waits for
// the end unless async (card.h's CARD_ReadBackup and CARD_ReadBackupAsync)
BOOL port_card_read_backup(u32 src, void *dest, u32 size, MIDmaCallback callback, void *arg, BOOL async);
// Writes size bytes from src to the backup at dest, then verifies them, without waiting
// (card.h's CARD_WriteAndVerifyBackupAsync)
BOOL port_card_write_and_verify_backup(u32 dest, const void *src, u32 size, MIDmaCallback callback, void *arg);

#define CARD_ReadBackup(src, dest, size) port_card_read_backup((src), (dest), (size), NULL, NULL, FALSE)
#define CARD_ReadBackupAsync(src, dest, size, callback, arg)                                                           \
    port_card_read_backup((src), (dest), (size), (callback), (arg), TRUE)
#define CARD_WriteAndVerifyBackupAsync(dest, src, size, callback, arg)                                                 \
    port_card_write_and_verify_backup((dest), (src), (size), (callback), (arg))

#endif // PORT_HAL_CARD_CARD_BACKUP_CALLS_H
